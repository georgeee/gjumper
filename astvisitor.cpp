#include "astvisitor.h"
#include "datatypes.h"

#include <iostream>
#include <tuple>
#include <vector>

#include "llvm/Support/raw_ostream.h"

const GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::emptyHintTypeDetails
        = make_tuple(SourceRange(), std::string(), std::string());

void GJRecursiveASTVisitor::addHint(TypeLoc tl){
    addHint(tl.getSourceRange(), tl.getTypePtr());
}

void GJRecursiveASTVisitor::addHint(SourceRange refRange, QualType qType){
    addHint(refRange, qType.getTypePtr(), qType.getQualifiers());
}

void GJRecursiveASTVisitor::addHint(SourceRange refRange, const Type * type){
    addHint(refRange, type, Qualifiers());
}

#define TRY_TYPE_IMPL(NAME) \
    if(isa<NAME##Type>(type)){ \
        FUNC(getDetailsFor##NAME##Type((NAME##Type*) type, qualifiers)) \
    }

GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::getDetailsForType(Type const * type, Qualifiers qualifiers){
#define FUNC(DETAILS) return DETAILS;
    GJ_HINT_TYPE_LIST(TRY_TYPE_IMPL)
    return emptyHintTypeDetails;
#undef FUNC
}

void GJRecursiveASTVisitor::addHint(SourceRange refRange, const Type * type, Qualifiers qualifiers){
#define FUNC(DETAILS) addHint(refRange, std::get<0>(DETAILS), \
                        std::get<1>(DETAILS), std::string("type ") + std::get<2>(DETAILS)); return;
    GJ_HINT_TYPE_LIST(TRY_TYPE_IMPL)
#undef FUNC
}
#undef TRY_TYPE_IMPL

void GJRecursiveASTVisitor::addHint(SourceRange refRange, SourceRange declRange, std::string declName, std::string refName){
    addHint(refRange.getBegin(), refRange.getEnd(),
            declRange.getBegin(), declRange.getEnd(), declName, refName);
}

void GJRecursiveASTVisitor::addHint(SourceLocation refBegin, SourceLocation refEnd, SourceLocation declBegin, SourceLocation declEnd, std::string declName, std::string refName){
    if(refBegin.isInvalid() || refEnd.isInvalid()
            || declBegin.isInvalid() || declEnd.isInvalid())
        return;
    SourceManager & SM = sourceManager;

    typedef std::tuple<PresumedLoc, PresumedLoc, hint_target_type> loc;
    auto processPair = [&SM](SourceLocation beginLoc, SourceLocation endLoc){
        std::vector<loc> locs;
        auto pushFromFileIdLocation = [&SM, &locs](SourceLocation beginLoc, SourceLocation endLoc, hint_target_type target_type){
            PresumedLoc beginLocPLoc = SM.getPresumedLoc(beginLoc);
            PresumedLoc endLocPLoc = SM.getPresumedLoc(endLoc);
            locs.push_back(loc(beginLocPLoc, endLocPLoc, target_type));
        };
        if (beginLoc.isFileID()) {
            pushFromFileIdLocation(beginLoc, endLoc, REGULAR_HTT);
        }else{
            pushFromFileIdLocation(SM.getExpansionLoc(beginLoc), SM.getExpansionLoc(endLoc), EXPANSION_HTT);
            pushFromFileIdLocation(SM.getSpellingLoc(beginLoc), SM.getSpellingLoc(endLoc), SPELLING_HTT);
        }
        return locs;
    };
    
    std::vector<loc> refLocs = processPair(refBegin, refEnd);
    std::vector<loc> declLocs = processPair(declBegin, declEnd);

    for(int i = 0; i < refLocs.size(); ++i){
        const loc & ref = refLocs[i];
        const PresumedLoc & refBegin = std::get<0>(ref);
        const PresumedLoc & refEnd = std::get<1>(ref);
        pos_t refBeginPos(refBegin.getLine(), refBegin.getColumn());
        pos_t refEndPos(refEnd.getLine(), refEnd.getColumn());
        hint_target_type refTargetType = std::get<2>(ref);
        for(int j = 0; j < declLocs.size(); ++j){
            const loc & decl = declLocs[j];
            const PresumedLoc & declBegin = std::get<0>(decl);
            const PresumedLoc & declEnd = std::get<1>(decl);
            pos_t declBeginPos(declBegin.getLine(), declBegin.getColumn());
            pos_t declEndPos(declEnd.getLine(), declEnd.getColumn());
            hint_target_type declTargetType = std::get<2>(decl);

            std::string refFilename = refBegin.getFilename();
            std::string declFilename = declBegin.getFilename();

           
            hint_t refHint (range_t(refBeginPos, refEndPos),
                            abs_pos_t(declFilename, declBeginPos),
                            DECL_HT, refName, declName, declTargetType);
            hint_t declHint (range_t(declBeginPos, declEndPos),
                            abs_pos_t(refFilename, refBeginPos),
                            REF_HT, declName, refName, refTargetType);
            bool differentFiles = refFilename != declFilename; 
            if(differentFiles || declBeginPos < refBeginPos
                    || refEndPos < declBeginPos)
                hint_base.add(refFilename, refHint);
            if(differentFiles || refBeginPos < declBeginPos
                    || declEndPos < refBeginPos)
                hint_base.add(declFilename, declHint);
        }
    }

}


bool GJRecursiveASTVisitor::VisitTypeLoc(TypeLoc tl){
    addHint(tl);

    tl.getType().dump((std::string("Some typeLoc's type:")
                + tl.getTypePtr()->getTypeClassName()).c_str());
    llvm::errs() << "   ";
    if(const AutoType* type = dyn_cast<AutoType>(tl.getType().getTypePtr())){
        llvm::errs() << (type->isDeduced()?"true":"false") << " ";
    }
    tl.getSourceRange().getBegin().dump(sourceManager);
    llvm::errs() << " ";
    tl.getSourceRange().getEnd().dump(sourceManager);
    llvm::errs() << "\n";
    return true;
}

bool GJRecursiveASTVisitor::VisitStmt(Stmt * stmt){
    stmt->dumpColor();
    return true;
}

bool GJRecursiveASTVisitor::VisitDecl(Decl * decl){
    decl->dumpColor();
    return true;
}

bool GJRecursiveASTVisitor::VisitType(Type * type){
    type->dump();
    return true;
}


std::string GJRecursiveASTVisitor::getTemplateParametersAsString(TemplateParameterList * tpl) const{
    SourceLocation left = tpl->getLAngleLoc();
    SourceLocation right = tpl->getRAngleLoc();
    FileID startFileID, endFileID;
    unsigned startOff = getLocationOffsetAndFileID(left, startFileID);
    unsigned endOff   = getLocationOffsetAndFileID(right, endFileID);
    const char *ptr = sourceManager.getCharacterData(left);
    return std::string(ptr, endOff - startOff + 1);
}

unsigned GJRecursiveASTVisitor::getLocationOffsetAndFileID(SourceLocation Loc, FileID &FID) const {
    assert(Loc.isValid() && "Invalid location");
    std::pair<FileID,unsigned> V = sourceManager.getDecomposedLoc(Loc);
    FID = V.first;
    return V.second;
}

std::string GJRecursiveASTVisitor::getTemplateDeclAsString(TemplateDecl * decl) const{
    std::string name = decl->getNameAsString();
    return name + getTemplateParametersAsString(decl->getTemplateParameters());
}
