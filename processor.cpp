#include "processor.h"
#include "datatypes.h"
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

gj::processor::processor(char ** compilerOptionsStart, char ** compilerOptionsEnd) {
    Invocation = new CompilerInvocation();
    compiler.createDiagnostics();
    // Create an invocation that passes any flags to preprocessor
    CompilerInvocation::CreateFromArgs(*Invocation, compilerOptionsStart,
            compilerOptionsEnd, compiler.getDiagnostics());
    compiler.setInvocation(Invocation);

    // Set default target triple
    llvm::IntrusiveRefCntPtr<TargetOptions> pto(new TargetOptions());
    pto->Triple = llvm::sys::getDefaultTargetTriple();
    llvm::IntrusiveRefCntPtr<TargetInfo>
        pti(TargetInfo::CreateTargetInfo(compiler.getDiagnostics(),
                    pto.getPtr()));
    compiler.setTarget(pti.getPtr());

    compiler.createFileManager();
    
    initHeaderSearchOptions();
    
    LangOptions langOpts;
    langOpts.GNUMode = 1; 
    langOpts.CXXExceptions = 0; 
    langOpts.RTTI = 1; 
    langOpts.Bool = 1; 
    langOpts.CPlusPlus = 1; 
    Invocation->setLangDefaults(langOpts,
            clang::IK_CXX,
            clang::LangStandard::lang_cxx0x);
}

gj::processor::~processor(){
    delete Invocation;
}

void gj::processor::initHeaderSearchOptions(){
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
}

global_hint_base_t gj::processor::process(std::string fileName){
    global_hint_base_t hint_base;

    compiler.createSourceManager(compiler.getFileManager());
    compiler.createPreprocessor();
    compiler.getPreprocessorOpts().UsePredefines = false;
    compiler.createASTContext();

    const FileEntry *pFile = compiler.getFileManager().getFile(fileName);
    compiler.getSourceManager().createMainFileID(pFile);
    compiler.getDiagnosticClient().BeginSourceFile(compiler.getLangOpts(),
            &compiler.getPreprocessor());
    GASTConsumer astConsumer(compiler.getSourceManager(), hint_base);
    ParseAST(compiler.getPreprocessor(), &astConsumer, compiler.getASTContext());
    compiler.getDiagnosticClient().EndSourceFile();
    return hint_base;
}

bool gj::GASTConsumer::HandleTopLevelDecl(DeclGroupRef d){
    typedef DeclGroupRef::iterator iter;
    for (iter b = d.begin(), e = d.end(); b != e; ++b)
    {
        rv.StartTraversing(*b);
    }
    return true;
}
