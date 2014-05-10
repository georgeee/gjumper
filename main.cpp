#include <sys/stat.h>
#include <stdio.h>
#include <vector>
#include <iostream>

#include "astvisitor.h"
#include "datatypes.h"
#include "hintdb_exporter.h"
#include "processor.h"

using namespace clang;
using namespace gj;
using namespace std;

int main(int argc, char **argv)
{
    struct stat sb;

    if (argc < 2)
    {
        llvm::errs() << "Usage: ./gjumper <options> -files <filenames>\n";
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
        if (stat(fileName.c_str(), &sb) == -1)
        {
            perror(fileName.c_str());
            exit(EXIT_FAILURE);
        }
        proc.projectFiles.push_back(fileName);
    }
    proc.full_recache();
    return 0;
}
