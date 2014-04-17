#ifndef GJ_PROCESSOR
#define GJ_PROCESSOR

#include "datatypes.h"
#include "astvisitor.h"

#include "llvm/Support/Host.h"
#include "llvm/Support/FileSystem.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Parse/ParseAST.h"

using namespace clang;
using namespace std;

namespace gj{
    class processor{
        CompilerInstance compiler;
        CompilerInvocation *Invocation;
        void initHeaderSearchOptions();
        public:
            processor(char ** compilerOptionsStart, char ** compilerOptionsEnd);
            virtual ~processor();
            global_hint_base_t process(std::string filename);
    };

class GASTConsumer : public ASTConsumer
{
    public:
        GASTConsumer(SourceManager & SM, global_hint_base_t& hint_base) : rv(SM, hint_base) { }
        virtual bool HandleTopLevelDecl(DeclGroupRef d);
        GJRecursiveASTVisitor rv;
};

}

#endif
