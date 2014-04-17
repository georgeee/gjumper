#ifndef GJ_ASTVISITOR
#define GJ_ASTVISITOR

#define GJ_HINT_TYPE_LIST(FUNC) \
    FUNC(Pointer) \
    FUNC(Typedef) \
    FUNC(TemplateTypeParm) \
    FUNC(TemplateSpecialization) \
    FUNC(Auto) \
    FUNC(Tag)

#include <iostream>
#include <string>
#include <tuple>
#include "datatypes.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"

using namespace clang;
using namespace gj;
using namespace std;

namespace gj{

    class GJRecursiveASTVisitor
        : public clang::RecursiveASTVisitor<GJRecursiveASTVisitor>
    {


        public:
            GJRecursiveASTVisitor(SourceManager & SM, global_hint_base_t& hint_base)
                : sourceManager(SM), hint_base(hint_base) { }
            bool StartTraversing(Decl *D);
            bool VisitDeclRefExpr(DeclRefExpr* expr);
            bool VisitType(Type* type);
            bool VisitStmt(Stmt* stmt);
            bool VisitDecl(Decl * decl);
            bool VisitVarDecl(VarDecl * decl);
            bool VisitTypeLoc(TypeLoc TL);
            bool VisitFunctionDecl(FunctionDecl * decl);
            bool VisitClassTemplateSpecializationDecl(ClassTemplateSpecializationDecl * decl);

            SourceManager & sourceManager; 
            global_hint_base_t& hint_base;
            
        private:

            void addHint(TypeLoc tl);
            void addHint(SourceRange refRange, const Type * t);
            void addHint(SourceRange refRange, const Type * type, Qualifiers qualifiers);
            void addHint(SourceRange refRange, QualType qType);
            void addHint(SourceRange refRange, SourceRange declRange, std::string declName, std::string refName);
            //@TODO two different names!
            void addHint(SourceLocation refBegin, SourceLocation refEnd, SourceLocation declBegin, SourceLocation declEnd, std::string declName, std::string refName);

            typedef std::tuple<SourceRange, std::string, std::string> HintTypeDetails;
            const static HintTypeDetails emptyHintTypeDetails;

            #define GEN_TYPE_DECL(NAME) \
            HintTypeDetails getDetailsFor##NAME##Type(const NAME##Type* type, Qualifiers qualifiers);

            GJ_HINT_TYPE_LIST(GEN_TYPE_DECL)
            GEN_TYPE_DECL()
            #undef GEN_TYPE_DECL

            HintTypeDetails getDetailsForType(QualType qType){
                return getDetailsForType(qType.getTypePtr(), qType.getQualifiers());
            }

            std::string getTemplateParametersAsString(TemplateParameterList * tpl) const;
            std::string getTemplateDeclAsString(TemplateDecl * decl) const;
            unsigned getLocationOffsetAndFileID(SourceLocation Loc, FileID &FID) const;
    };
}

#endif
