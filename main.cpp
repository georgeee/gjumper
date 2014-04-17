#include <sys/stat.h>
#include <stdio.h>
#include <vector>
#include <iostream>

#include "astvisitor.h"
#include "datatypes.h"
#include "hintdb_exporter.h"
#include "processor.h"

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
using namespace gj;
using namespace std;



int main(int argc, char **argv)
{
    struct stat sb;

    if (argc < 2)
    {
        llvm::errs() << "Usage: CIrewriter <options> -files <filenames>\n";
        return 1;
    }

    vector<std::string> files;

    char ** compilerOptionsStart = argv + 1;
    char ** compilerOptionsEnd = NULL;
    for(int i = 1; i < argc; ++i){
        if(strcmp(argv[i], "-files") == 0){
            compilerOptionsEnd = argv + i;
            for(++i; i < argc; ++i)
                files.push_back(argv[i]);
        }
    }

    if(compilerOptionsEnd == NULL){
        compilerOptionsEnd = argv + argc - 1;
        files.push_back(argv[argc - 1]);
    }

    processor proc(compilerOptionsStart, compilerOptionsEnd);

    bool firstLine = true;
    for(std::string & fileName : files){
        if(firstLine) {
            firstLine = false;
        }else{
            cout << "\n\n===========================\n\n";
        }

        // Make sure it exists
        if (stat(fileName.c_str(), &sb) == -1)
        {
            perror(fileName.c_str());
            exit(EXIT_FAILURE);
        }

        global_hint_base_t hint_base = proc.process(fileName);

        hintdb_json_exporter exporter(cout);

        for(pair<const string, hint_base_t> & el : hint_base){
            cout << "File " << el.first << "\n";
            cout << el.second << "\n\n";
            exporter.export_base(el.second);
        }
    }

    return 0;
}

