#ifndef HINT_DB_CACHE_MANAGER_H
#define HINT_DB_CACHE_MANAGER_H


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

using namespace clang;

namespace gj{

    const std::string PATH_SEPARATOR = "/";
    static constexpr const char * const DEFAULT_CACHE_DIR = ".gjumper-cache";

    class recache_exception : public std::exception {};

    class foreign_ref_holders_t : public std::unordered_map<std::string, std::unordered_set<std::string> >{
        public:
            static foreign_ref_holders_t parse_from_json(const Json::Value & json);
            Json::Value as_json() const;
    };

    class ro_hint_db_cache_manager_base {
        public:
        static constexpr const char * const CACHE_EXT = ".json";
        const std::string cacheDir;
        ro_hint_db_cache_manager_base(const std::string & cacheDir = DEFAULT_CACHE_DIR) : cacheDir(cacheDir){}
        void mkCacheDir() const;
        std::string getCachePath(const std::string & filename) const;
        Json::Value loadCached(const std::string & filename) const;
    };
    class ro_hint_db_cache_manager : public map<std::string, splitted_json_hint_base>, public ro_hint_db_cache_manager_base {
        protected:
        static constexpr const char * const HIERARCY_TREE_CACHE_FILENAME = "hierarcy_tree.json";
        static constexpr const char * const FRH_CACHE_FILENAME = "frh.json";
        hierarcy_tree * cachedTree;
        foreign_ref_holders_t * cachedFRH;
        splitted_json_hint_base & retreive_impl(const std::string & filename);
        hierarcy_tree & retreive_hierarcy_impl();
        foreign_ref_holders_t & retreive_FRH_impl();
        public:
        ro_hint_db_cache_manager(const std::string & cacheDir = DEFAULT_CACHE_DIR) : ro_hint_db_cache_manager_base(cacheDir), cachedTree(NULL), cachedFRH(NULL) {}
        bool is_cached(const std::string & filename) const;
        const splitted_json_hint_base & retreive_ro(const std::string & filename);
        const hierarcy_tree & retreive_hierarcy_ro();
        const foreign_ref_holders_t & retreive_FRH_ro();
        void reset_ro();
        ~ro_hint_db_cache_manager();
    };
    class hint_db_cache_manager : public ro_hint_db_cache_manager {
        void saveCached(const std::string & filename, const Json::Value & jsonValue) const;
        void saveHierarcyTree(const hierarcy_tree & tree) const;
        void saveFRH(const foreign_ref_holders_t & FRH) const;
        public:
        hint_db_cache_manager(const std::string & cacheDir = DEFAULT_CACHE_DIR) : ro_hint_db_cache_manager(cacheDir){}
        splitted_json_hint_base & retreive(const std::string & filename);
        hierarcy_tree & retreive_hierarcy();
        foreign_ref_holders_t & retreive_FRH();
        ~hint_db_cache_manager();
    };
    class ro_hint_base_cacher : public unordered_map<std::string, hint_base_t>, private ro_hint_db_cache_manager_base{
        public:
        ro_hint_base_cacher(const std::string & cacheDir = DEFAULT_CACHE_DIR) : ro_hint_db_cache_manager_base(cacheDir){}
        const hint_base_t & retreive(const std::string & filename);
    };

}

#endif
