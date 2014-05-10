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

class processor_test : public processor{
    public:
        processor_test(char ** compilerOptionsStart, char ** compilerOptionsEnd, const std::string & cacheDir = DEFAULT_CACHE_DIR, const std::string & prjFilesPath = DEFAULT_PROJECT_FILES_JSON_PATH) : processor(compilerOptionsStart, compilerOptionsEnd, cacheDir, prjFilesPath) {}
        void full_recache_test(){
            rmCacheDir();
            hint_db_cache_manager cacheManager;
            unordered_set<std::string> visited;
            hierarcy_tree & hierarcy = cacheManager.retreive_hierarcy();
            for(const std::string & filename : projectFiles){
                fprintf(stderr, "full_recache_test: launching recache_dfs_test with filename=%s\n", filename.c_str());
                recache_dfs_test(cacheManager, hierarcy.get_or_create(filename), visited);
            }
            hintBaseCacher.clear();
        }
        void recache_dfs_test(hint_db_cache_manager & cacheManager, const shared_ptr<hierarcy_tree_node> & node, unordered_set<std::string> & visited, bool throw_on_cycle = false){
            const std::string & node_filename = node->filename;
            if(visited.find(node_filename) != visited.end()) {
                if(throw_on_cycle) throw recache_exception();
                else return;
            }
            visited.insert(node_filename);
            hierarcy_tree & hierarcy = cacheManager.retreive_hierarcy();
            auto collectResult = collect(node_filename);
            hierarcy.replace_parents(node, collectResult.second);
            const global_hint_base_t & ghint_base = collectResult.first;
            splitted_json_hint_base & node_base = cacheManager.retreive(node_filename);
            foreign_ref_holders_t & frh = cacheManager.retreive_FRH();
            auto frh_iter = frh.find(node_filename);
            if(frh_iter != frh.end()){
                for(const std::string & ref_holder_filename : frh_iter->second){
                    cacheManager.retreive(ref_holder_filename).clear_foreign_refs(node_filename);
                }
            }else{
                frh_iter = frh.insert(make_pair(node_filename, std::unordered_set<std::string> ())).first;
            }
            std::unordered_set<std::string> & node_frh = frh_iter->second;
            node_frh.clear();
            if(ghint_base.empty()){
                node_base.set_rest(hint_base_t());
            }else{
                for(auto hdb_pr : ghint_base){
                    const std::string & filename = hdb_pr.first;
                    const hint_base_t & base = hdb_pr.second;
                    if(filename == node_filename){
                        node_base.set_rest(base);
                    }else{
                        splitted_json_hint_base & forein_base = cacheManager.retreive(filename);
                        forein_base.add_foreign_refs(node_filename, base);
                        node_frh.insert(filename);
                    }
                }
            }
            for(auto child_iter = node->children_begin(); child_iter != node->children_end(); ++child_iter){
                recache_dfs_test(cacheManager, child_iter->second, visited, throw_on_cycle);
            }
        }
};

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

    processor_test proc(compilerOptionsStart, compilerOptionsEnd);

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

        proc.projectFiles.push_back(fileName);

        //auto collectResult = proc.collect(fileName);
        //global_hint_base_t & hint_base = collectResult.first;


        //hintdb_json_exporter exporter(cout);

        //for(pair<const string, hint_base_t> & el : hint_base){
            //cout << "File " << el.first << "\n";
            //cout << el.second << "\n\n";
            //exporter.export_base(el.second);
        //}
        //cout << " -- parents:\n";
        //for(const string parent : collectResult.second)
            //cout << " ---- " << parent << "\n";
    }
    proc.full_recache_test();
    return 0;
}
