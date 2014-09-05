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
    constexpr const char * const DEFAULT_PROJECT_COMPILE_CONFIG_JSON_PATH = "gj_compile.json";

    vector<std::string> readStringVector(const Json::Value & json);
    void addToCompileConfig(bool isCxx, char ** argv, int argc, const std::string & prjCompileConfigPath = DEFAULT_PROJECT_COMPILE_CONFIG_JSON_PATH);

    struct compileConfig{
        static constexpr const char * const PCCJ_OPTIONS = "opts";
        static constexpr const char * const PCCJ_ISCXX = "cxx";
        vector<std::string> options;
        bool isCxx;
        compileConfig(){}
        compileConfig(vector<std::string> options, bool isCxx) : options(options), isCxx(isCxx) {}
        compileConfig(Json::Value json);
        Json::Value asJson();
    };
    class processor {
        protected:
        ro_hint_base_cacher hintBaseCacher;
        const std::string cacheDir;
        void initHeaderSearchOptions(CompilerInstance & compiler);
        void rmCacheDir() const;
        void recache_dfs(hint_db_cache_manager & cacheManager, const shared_ptr<hierarcy_tree_node> & node, unordered_set<std::string> & visited, bool throw_on_cycle = false);
        static unordered_map<std::string, compileConfig> loadPrjCompileConfig(const std::string & prjCompileConfigPath);
        public:
        unordered_map<std::string, compileConfig> projectCompileConfig;
        processor(const std::string & cacheDir = DEFAULT_CACHE_DIR, const std::string & prjCompileConfigPath = DEFAULT_PROJECT_COMPILE_CONFIG_JSON_PATH);
        pair<global_hint_base_t, vector<string> > collect(const std::string & filename, const compileConfig & config);
        pair<global_hint_base_t, vector<string> > collect(const std::string & filename, const char ** compilerOptionsStart, const char ** compilerOptionsEnd, bool isCxx = true);
        void recache(const std::string & filename);
        vector<shared_ptr<hint_t> > resolve_position(const std::string & filename, pos_t pos);
        vector<shared_ptr<hint_t> > resolve_position(const std::string & filename, int line, int index);
        vector<shared_ptr<hint_t> > resolve_position(const std::string & filename, int line);
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
