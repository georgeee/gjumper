#include "astvisitor.h"
#include "datatypes.h"
#include <typeinfo>
#include <tuple>
#include <vector>

#include "llvm/Support/raw_ostream.h"

bool GJRecursiveASTVisitor::StartTraversing(Decl *D){
    return TraverseDecl(D);
}

bool GJRecursiveASTVisitor::VisitClassTemplateSpecializationDecl(ClassTemplateSpecializationDecl * decl){
    ClassTemplateDecl * templateDecl = decl->getSpecializedTemplate();
    addHint(decl->getSourceRange(), templateDecl->getSourceRange(), 
                std::string("template ") + getTemplateDeclAsString(templateDecl),
                std::string("specialization ") + QualType::getAsString(decl->getTypeForDecl(), Qualifiers()));
    return true;
}

bool GJRecursiveASTVisitor::VisitVarDecl(VarDecl * decl){
    const Type * type = decl->getType().getTypePtr();
    auto addVarDeclHint = [type, decl, this] (SourceRange typeDeclRange, std::string typeDeclDesc, const Type * varType) {
        addHint(decl->getSourceRange(), typeDeclRange, typeDeclDesc,
                "{type " + QualType::getAsString(varType, Qualifiers()) + "} " + decl->getQualifiedNameAsString());
    };
    if(const AutoType* autoType = dyn_cast<AutoType>(type)){
        TypeLoc tl = decl->getTypeSourceInfo()->getTypeLoc();
        if(decl->hasInit()){
            QualType qType = decl->getInit()->getType();
            HintTypeDetails details = getDetailsForType(qType);
            addHint(tl.getSourceRange(), std::get<0>(details),
                   std::get<1>(details), std::string("<auto> ") + std::get<2>(details));
        }
    }
    const Type * desugaredType = type->getUnqualifiedDesugaredType();
    HintTypeDetails details = getDetailsForType(desugaredType, Qualifiers());
    addVarDeclHint(std::get<0>(details), std::get<1>(details), type);
    return true;
}

bool gj::GJRecursiveASTVisitor::VisitDeclRefExpr(DeclRefExpr* expr){
    ValueDecl * decl = expr->getDecl();
    std::string name = decl->getType().getAsString() + ' ' + decl->getQualifiedNameAsString();
    addHint(expr->getSourceRange(), decl->getSourceRange(), name, std::string("reference to ") + name);
    if(auto functionDecl = dyn_cast<FunctionDecl>(decl))
        if(FunctionTemplateDecl * templateDecl = functionDecl->getPrimaryTemplate()){
            addHint(expr->getSourceRange(), templateDecl->getSourceRange(),
                    "function template " + getTemplateDeclAsString(templateDecl),
                    std::string("reference to ") + name);
        }
    return true;
}

bool GJRecursiveASTVisitor::VisitFunctionDecl(FunctionDecl * decl){
    std::string name = decl->getType().getAsString() + ' ' + decl->getQualifiedNameAsString();
    if(!decl->isTemplateInstantiation())
        if(FunctionTemplateDecl * templateDecl = decl->getPrimaryTemplate()){
            addHint(decl->getSourceRange(), templateDecl->getSourceRange(),
                    "function template '" + getTemplateDeclAsString(templateDecl),
                    std::string("specialization ") + name);
        }
    return true;
}


GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::getDetailsForAutoType(AutoType const* type, Qualifiers qualifiers){
    QualType qt = type->getDeducedType();
    if(!qt.isNull()){
        HintTypeDetails typeDetails = getDetailsForType(qt);
        return make_tuple(std::get<0>(typeDetails),
                std::get<1>(typeDetails),
                std::string("<auto> ") + std::get<2>(typeDetails));
    }
    return emptyHintTypeDetails;
}

GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::getDetailsForTemplateTypeParmType(const TemplateTypeParmType* type, Qualifiers qualifiers){
    TemplateTypeParmDecl * decl = type->getDecl();
    std::string paramName = decl->getIdentifier()->getNameStart();
    return make_tuple(decl->getSourceRange(),
            std::string("template param ") + paramName,
            QualType::getAsString(type, qualifiers));
}

GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::getDetailsForTagType(const TagType* type, Qualifiers qualifiers){
    TagDecl * decl = type->getDecl();
    std::string kindName = decl->getKindName();
    std::string qualName = decl->getQualifiedNameAsString();
    return make_tuple(decl->getSourceRange(),
            kindName + ' ' + qualName,
            QualType::getAsString(type, qualifiers));
}

GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::getDetailsForTypedefType(TypedefType const* type, Qualifiers qualifiers){
    TypedefNameDecl * decl = type->getDecl();
    QualType subType = decl->getUnderlyingType();
    HintTypeDetails details = getDetailsForType(subType);
    if(std::get<0>(details).isInvalid()) return emptyHintTypeDetails;
    return make_tuple(std::get<0>(details),
            std::get<1>(details),
            std::string("typedef ") + (decl->getQualifiedNameAsString()) + " (" + std::get<2>(details) + ')');
}

GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::getDetailsForPointerType(PointerType const* type, Qualifiers qualifiers){
    QualType subType = type->getPointeeType();
    HintTypeDetails details = getDetailsForType(subType);
    if(std::get<0>(details).isInvalid()) return emptyHintTypeDetails;
    return make_tuple(std::get<0>(details),
            std::get<1>(details),
            std::string("(*) ") + std::get<2>(details));
}

GJRecursiveASTVisitor::HintTypeDetails GJRecursiveASTVisitor::getDetailsForTemplateSpecializationType(TemplateSpecializationType const* type, Qualifiers qualifiers){
    TemplateDecl * decl = type->getTemplateName().getAsTemplateDecl();
    if(decl){
        return make_tuple(decl->getSourceRange(),
                std::string("template ") + getTemplateDeclAsString(decl),
                QualType::getAsString(type, qualifiers));
    }
    return emptyHintTypeDetails;
}
