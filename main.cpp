#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <vector>
#include <iostream>

#include "astvisitor.h"
#include "datatypes.h"

#include "llvm/Support/Host.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FileSystem.h"

#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;
using namespace gj;
using namespace std;

class GASTConsumer : public ASTConsumer
{
    public:

        GASTConsumer(Rewriter &Rewrite, global_hint_base_t& hint_base) : rv(Rewrite, hint_base) { }
        virtual bool HandleTopLevelDecl(DeclGroupRef d);

        GJRecursiveASTVisitor rv;
};


bool GASTConsumer::HandleTopLevelDecl(DeclGroupRef d)
{
    typedef DeclGroupRef::iterator iter;

    for (iter b = d.begin(), e = d.end(); b != e; ++b)
    {
        //llvm::errs() << ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n";
        //(*b)->dump();
        //llvm::errs() << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
        rv.TraverseDecl(*b);
    }

    return true; // keep going
}


int main(int argc, char **argv)
{
    struct stat sb;

    if (argc < 2)
    {
        llvm::errs() << "Usage: CIrewriter <options> <filename>\n";
        return 1;
    }

    // Get filename
    std::string fileName(argv[argc - 1]);

    // Make sure it exists
    if (stat(fileName.c_str(), &sb) == -1)
    {
        perror(fileName.c_str());
        exit(EXIT_FAILURE);
    }

    CompilerInstance compiler;
    DiagnosticOptions diagnosticOptions;
    compiler.createDiagnostics();
    //compiler.createDiagnostics(argc, argv);

    // Create an invocation that passes any flags to preprocessor
    CompilerInvocation *Invocation = new CompilerInvocation;
    CompilerInvocation::CreateFromArgs(*Invocation, argv + 1, argv + argc,
            compiler.getDiagnostics());
    compiler.setInvocation(Invocation);

    // Set default target triple
    llvm::IntrusiveRefCntPtr<TargetOptions> pto( new TargetOptions());
    pto->Triple = llvm::sys::getDefaultTargetTriple();
    llvm::IntrusiveRefCntPtr<TargetInfo>
        pti(TargetInfo::CreateTargetInfo(compiler.getDiagnostics(),
                    pto.getPtr()));
    compiler.setTarget(pti.getPtr());

    compiler.createFileManager();
    compiler.createSourceManager(compiler.getFileManager());

    HeaderSearchOptions &headerSearchOptions = compiler.getHeaderSearchOpts();

    // <Warning!!> -- Platform Specific Code lives here
    // This depends on A) that you're running linux and
    // B) that you have the same GCC LIBs installed that
    // I do.
    // Search through Clang itself for something like this,
    // go on, you won't find it. The reason why is Clang
    // has its own versions of std* which are installed under
    // /usr/local/lib/clang/<version>/include/
    // See somewhere around Driver.cpp:77 to see Clang adding
    // its version of the headers to its include path.
    // To see what include paths need to be here, try
    // clang -v -c test.c
    // or clang++ for C++ paths as used below:
    headerSearchOptions.AddPath("/usr/include/c++/4.6",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath("/usr/include/c++/4.6/i686-linux-gnu",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath("/usr/include/c++/4.6/backward",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath("/usr/local/include",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath("/usr/include/clang/3.3/include",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath("/usr/include/c++/4.6/x86_64-linux-gnu",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath("/usr/include/x86_64-linux-gnu",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath("/usr/include",
            clang::frontend::Angled,
            false,
            false);
    headerSearchOptions.AddPath(".",
            clang::frontend::Angled,
            false,
            false);
    // </Warning!!> -- End of Platform Specific Code


    // Allow C++ code to get rewritten
    LangOptions langOpts;
    langOpts.GNUMode = 1; 
    langOpts.CXXExceptions = 0; 
    langOpts.RTTI = 1; 
    langOpts.Bool = 1; 
    langOpts.CPlusPlus = 1; 
    Invocation->setLangDefaults(langOpts,
            clang::IK_CXX,
            clang::LangStandard::lang_cxx0x);

    compiler.createPreprocessor();
    compiler.getPreprocessorOpts().UsePredefines = false;

    compiler.createASTContext();

    // Initialize rewriter
    Rewriter Rewrite;
    Rewrite.setSourceMgr(compiler.getSourceManager(), compiler.getLangOpts());

    const FileEntry *pFile = compiler.getFileManager().getFile(fileName);
    compiler.getSourceManager().createMainFileID(pFile);
    compiler.getDiagnosticClient().BeginSourceFile(compiler.getLangOpts(),
            &compiler.getPreprocessor());

    global_hint_base_t hint_base;
    GASTConsumer astConsumer(Rewrite, hint_base);

    ParseAST(compiler.getPreprocessor(), &astConsumer, compiler.getASTContext());
    compiler.getDiagnosticClient().EndSourceFile();

    cout << hint_base;

    return 0;
}

