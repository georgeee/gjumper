#ifndef GJ_ASTVISITOR
#define GJ_ASTVISITOR

#define GJ_HINT_TYPE_LIST(FUNC) \
    FUNC(TemplateTypeParm) \
    FUNC(Auto) \
    FUNC(Tag)

#include <iostream>
#include <string>
#include "datatypes.h"

#include "clang/Basic/SourceManager.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;
using namespace gj;
using namespace std;

namespace gj{

    class GJRecursiveASTVisitor
        : public clang::RecursiveASTVisitor<GJRecursiveASTVisitor>
    {


        public:
            GJRecursiveASTVisitor(Rewriter &R, global_hint_base_t& hint_base)
                : Rewrite(R), hint_base(hint_base) { }
            //void InstrumentStmt(Stmt *s);

            //bool VisitMaterializeTemporaryExpr(MaterializeTemporaryExpr* expr);
            bool VisitDeclRefExpr(DeclRefExpr* expr);
            bool VisitType(Type* type);
            bool VisitStmt(Stmt* stmt);
            bool VisitDecl(Decl * decl);
            bool VisitVarDecl(VarDecl * decl);
            bool VisitDeclaratorDecl(DeclaratorDecl * decl);
            bool VisitTypeLoc(TypeLoc TL);
            bool VisitTagTypeLoc(TagTypeLoc tagTypeLoc);
            
            void addHint(TypeLoc tl);
            void addHint(SourceRange refRange, const Type * t);
            void addHint(SourceRange refRange, SourceRange declRange, string name);
            //@TODO two different names!
            void addHint(SourceLocation refBegin, SourceLocation refEnd, SourceLocation declBegin, SourceLocation declEnd, string name);

            #define GEN_TYPE_DECL(NAME) \
            static SourceRange getRangeFor##NAME##Type(const NAME##Type* type); \
            static std::string getNameFor##NAME##Type(const NAME##Type* type);

            GJ_HINT_TYPE_LIST(GEN_TYPE_DECL)
            GEN_TYPE_DECL()
            #undef GEN_TYPE_DECL

            Rewriter &Rewrite;
            global_hint_base_t& hint_base;
    };
}

#endif
