#include "astvisitor.h"
#include "datatypes.h"

#include <iostream>
#include <tuple>
#include <vector>

#include "llvm/Support/raw_ostream.h"

void GJRecursiveASTVisitor::addHint(TypeLoc tl){
    addHint(tl.getSourceRange(), tl.getTypePtr());
}
#define TRY_TYPE_IMPL(NAME) \
    if(isa<NAME##Type>(type)){ \
        NAME##Type* casted = (NAME##Type*) type; \
        FUNC(getRangeFor##NAME##Type(casted), getNameFor##NAME##Type(casted)) \
    }

std::string GJRecursiveASTVisitor::getNameForType(clang::Type const* type){
#define FUNC(RANGE, NAME) return NAME;
    GJ_HINT_TYPE_LIST(TRY_TYPE_IMPL)
    return std::string();
#undef FUNC
}

SourceRange GJRecursiveASTVisitor::getRangeForType(clang::Type const* type){
#define FUNC(RANGE, NAME) return RANGE;
    GJ_HINT_TYPE_LIST(TRY_TYPE_IMPL)
    return SourceRange();
#undef FUNC
}
void GJRecursiveASTVisitor::addHint(SourceRange refRange, const Type * type){
#define FUNC(RANGE, NAME) addHint(refRange, RANGE, NAME); return;
    GJ_HINT_TYPE_LIST(TRY_TYPE_IMPL)
#undef FUNC
}
#undef TRY_TYPE_IMPL

void GJRecursiveASTVisitor::addHint(SourceRange refRange, SourceRange declRange, string name){
    addHint(refRange.getBegin(), refRange.getEnd(),
            declRange.getBegin(), declRange.getEnd(), name);
}

void GJRecursiveASTVisitor::addHint(SourceLocation refBegin, SourceLocation refEnd, SourceLocation declBegin, SourceLocation declEnd, std::string name){
    if(refBegin.isInvalid() || refEnd.isInvalid()
            || declBegin.isInvalid() || declEnd.isInvalid())
        return;
    SourceManager & SM = Rewrite.getSourceMgr();

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
    //OS << PLoc.getFilename() << ':' << PLoc.getLine()
    //<< ':' << PLoc.getColumn();
    
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
                            named_pos_t(declFilename, declBeginPos, name),
                            DECL_HT, declTargetType);
            hint_t declHint (range_t(declBeginPos, declEndPos),
                            named_pos_t(refFilename, refBeginPos, name),
                            REF_HT, refTargetType);
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
