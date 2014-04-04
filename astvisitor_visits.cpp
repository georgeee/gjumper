#include "astvisitor.h"
#include "datatypes.h"
#include <typeinfo>
#include <tuple>
#include <vector>

#include "llvm/Support/raw_ostream.h"

//bool gj::GJRecursiveASTVisitor::VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr* expr){
//return false;
//}
//

//bool gj::GJRecursiveASTVisitor::VisitEnumConstantDecl(EnumConstantDecl* decl){

//}

bool GJRecursiveASTVisitor::VisitVarDecl(VarDecl * decl){
    if(const AutoType* type = dyn_cast<AutoType>(decl->getType().getTypePtr())){
        //llvm::errs() << "Tik Tok! isDeduced: "
            //<< (type->isDeduced()?"true":"false") << " hasInit: "
            //<< (decl->hasInit()?"true":"false") << '\n';
        TypeLoc tl = decl->getTypeSourceInfo()->getTypeLoc();
        if(decl->hasInit()){
            const Type * initType = decl->getInit()->getType().getTypePtr();
            addHint(tl.getSourceRange(), getRangeForType(initType),
                    std::string("<auto> ") + getNameForType(initType));
        }
    }
    return true;
}

bool gj::GJRecursiveASTVisitor::VisitDeclRefExpr(DeclRefExpr* expr){
    NamedDecl * foundDecl = expr->getFoundDecl();
    std::string name;
    if(isa<ValueDecl>(foundDecl)){
        name += ((ValueDecl*) foundDecl)->getType().getAsString();
        name += " ";
    }
    name += foundDecl->getQualifiedNameAsString();
    addHint(expr->getSourceRange(), foundDecl->getSourceRange(), name);
    return true;
}

bool GJRecursiveASTVisitor::VisitStmt(Stmt * stmt){
    stmt->dumpColor();
    return true;
}

SourceRange GJRecursiveASTVisitor::getRangeForAutoType(AutoType const* type){
    QualType qt = type->getDeducedType();
    if(!qt.isNull()){
        SourceRange range = getRangeForType(qt.getTypePtr());
        return range;
    }
    return SourceRange();
}

std::string GJRecursiveASTVisitor::getNameForAutoType(AutoType const* type){
    llvm::errs() << "getNameForAutoType()\n";
    QualType qt = type->getDeducedType();
    if(!qt.isNull()){
        return std::string("auto ") + getNameForType(qt.getTypePtr());
    }
    return std::string();
}

SourceRange GJRecursiveASTVisitor::getRangeForTemplateTypeParmType(const TemplateTypeParmType* type){
    return type->getDecl()->getSourceRange();
}

std::string GJRecursiveASTVisitor::getNameForTemplateTypeParmType(const TemplateTypeParmType* type){
    TemplateTypeParmDecl * decl = type->getDecl();
    return std::string("template param type '") + decl->getIdentifier()->getNameStart() + "'";
}

SourceRange GJRecursiveASTVisitor::getRangeForTagType(const TagType* type){
    return type->getDecl()->getSourceRange();
}

std::string GJRecursiveASTVisitor::getNameForTagType(const TagType* type){
    TagDecl * decl = type->getDecl();
    return std::string("type '") + decl->getKindName() + " "
        + decl->getQualifiedNameAsString() + "'";
}

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

bool GJRecursiveASTVisitor::VisitTagTypeLoc(TagTypeLoc tl){
    return true;
}

bool GJRecursiveASTVisitor::VisitTypeLoc(TypeLoc tl){
    addHint(tl);

    tl.getType().dump((std::string("Some typeLoc's type:")
                + tl.getTypePtr()->getTypeClassName()).c_str());
    llvm::errs() << "   ";
    if(const AutoType* type = dyn_cast<AutoType>(tl.getType().getTypePtr())){
        llvm::errs() << (type->isDeduced()?"true":"false") << " ";
    }
    tl.getSourceRange().getBegin().dump(Rewrite.getSourceMgr());
    llvm::errs() << " ";
    tl.getSourceRange().getEnd().dump(Rewrite.getSourceMgr());
    llvm::errs() << "\n";
    return true;
}

bool GJRecursiveASTVisitor::VisitDeclaratorDecl(DeclaratorDecl * decl){
    TypeSourceInfo * typeSourceInfo = decl->getTypeSourceInfo();
    TypeLoc typeLoc = typeSourceInfo->getTypeLoc();
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
