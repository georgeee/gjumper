#ifndef GJ_PROCESSOR
#define GJ_PROCESSOR

#include <string>
#include <exception>
#include <unordered_set>
#include <unordered_map>
#include <memory>

#include "jsoncpp/json/json.h"

#include "datatypes.h"
#include "hintdb_exporter.h"
#include "hierarcy_tree.h"
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
#include "hint_db_cache_manager.h"

using namespace clang;

namespace gj{

    class processor{
        protected:
        ro_hint_base_cacher hintBaseCacher;
        const std::string cacheDir;
        CompilerInstance compiler;
        CompilerInvocation *Invocation;
        void initHeaderSearchOptions();
        void rmCacheDir() const;
        void recache_dfs(hint_db_cache_manager & cacheManager, const shared_ptr<hierarcy_tree_node> & node, unordered_set<std::string> & visited, bool throw_on_cycle = false);
        static vector<std::string> loadPrjFiles(const std::string & prjFilesPath);
        public:
        static constexpr const char * const DEFAULT_PROJECT_FILES_JSON_PATH = "gj_prj.json";
        vector<std::string> projectFiles;
        processor(char ** compilerOptionsStart, char ** compilerOptionsEnd, const std::string & cacheDir = DEFAULT_CACHE_DIR, const std::string & prjFilesPath = DEFAULT_PROJECT_FILES_JSON_PATH);
        virtual ~processor();
        pair<global_hint_base_t, vector<string> > collect(const std::string & filename);
        void recache(const std::string & filename);
        vector<shared_ptr<hint_t> > resolve_position(const std::string & filename, pos_t pos);
        vector<shared_ptr<hint_t> > resolve_position(const std::string & filename, int line, int index);
        void full_recache();
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
