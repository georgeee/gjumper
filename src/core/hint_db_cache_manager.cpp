#include "hint_db_cache_manager.h"
#include "datatypes.h"
#include "hintdb_exporter.h"
#include "hierarcy_tree.h"

#include "jsoncpp/json/json.h"

#include <fstream>
#include <memory>
#include <iostream>
#include <functional>
#include <boost/filesystem.hpp>
#include "fs_utils.h"
using namespace boost::filesystem;

using namespace clang;
using namespace std;



void gj::hint_db_cache_manager::saveCached(const std::string & filename, const Json::Value & jsonValue, bool hashFilename) const {
    const std::string cachePath = getCachePath(filename, hashFilename);
    std::ofstream t;
    t.open(cachePath);
    t << jsonValue;
}


Json::Value gj::ro_hint_db_cache_manager_base::loadCached(const std::string & filename, bool hashFilename) const {
    std::string cachePath = getCachePath(filename, hashFilename);
    if(!exists(cachePath))
        cachePath = getCachePath(canonical(filename).string(), hashFilename);
    if(!exists(cachePath))
        cachePath = getCachePath(relativize(baseDir, filename).string(), hashFilename);
    if(!exists(cachePath)) return Json::Value();
    std::ifstream t;
    t.open(cachePath);
    Json::Value json;
    t >> json;
    return json;
}

std::string gj::ro_hint_db_cache_manager_base::getCachePath(const std::string & filename, bool hashFilename) const {
    mkCacheDir();
    return (path(cacheDir) /= (hashFilename ? std::to_string(std::hash<std::string>()(filename)) + CACHE_EXT : filename)).string();
}

void gj::hint_db_cache_manager::saveHierarcyTree(const hierarcy_tree & tree) const {
    saveCached(HIERARCY_TREE_CACHE_FILENAME, tree.as_json(), false);
}

bool gj::ro_hint_db_cache_manager::is_cached(const std::string & filename) const {
    return find(filename) != end();
}
splitted_json_hint_base & gj::ro_hint_db_cache_manager::retreive_impl(const std::string & filename){
    auto iter = find(filename);
    if(iter != end())
        return iter->second;
    return insert(pair<std::string, splitted_json_hint_base>(filename, loadCached(filename))).first->second;
}

foreign_ref_holders_t & gj::ro_hint_db_cache_manager::retreive_FRH_impl(){
    if(cachedFRH) return *cachedFRH;
    return *(cachedFRH = new foreign_ref_holders_t(foreign_ref_holders_t::parse_from_json(loadCached(FRH_CACHE_FILENAME, false))));
}
hierarcy_tree & gj::ro_hint_db_cache_manager::retreive_hierarcy_impl(){
    if(cachedTree) return *cachedTree;
    return *(cachedTree = new hierarcy_tree(hierarcy_tree::parse_from_json(loadCached(HIERARCY_TREE_CACHE_FILENAME, false))));
}

void gj::hint_db_cache_manager::saveFRH(const foreign_ref_holders_t & frh) const {
    saveCached(FRH_CACHE_FILENAME, frh.as_json(), false);
}

const splitted_json_hint_base & gj::ro_hint_db_cache_manager::retreive_ro(const std::string & filename){
    return retreive_impl(filename);
}
const hierarcy_tree & gj::ro_hint_db_cache_manager::retreive_hierarcy_ro(){
    return retreive_hierarcy_impl();
}
const foreign_ref_holders_t & gj::ro_hint_db_cache_manager::retreive_FRH_ro(){
    return retreive_FRH_impl();
}

splitted_json_hint_base & gj::hint_db_cache_manager::retreive(const std::string & filename){
    return retreive_impl(filename);
}
hierarcy_tree & gj::hint_db_cache_manager::retreive_hierarcy(){
    hierarcy_tree & tree = retreive_hierarcy_impl();
    return tree;
}
foreign_ref_holders_t & gj::hint_db_cache_manager::retreive_FRH(){
    return retreive_FRH_impl();
}

gj::hint_db_cache_manager::~hint_db_cache_manager(){
    if(cachedTree)
        saveHierarcyTree(*cachedTree);
    if(cachedFRH)
        saveFRH(*cachedFRH);
    for(auto pr : *this)
        saveCached(pr.first, pr.second.as_json());
}
void gj::ro_hint_db_cache_manager::reset_ro(){
    if(cachedTree) delete cachedTree;
    if(cachedFRH) delete cachedFRH;
    clear();
}

gj::ro_hint_db_cache_manager::~ro_hint_db_cache_manager(){
    reset_ro();
}

void gj::ro_hint_db_cache_manager_base::mkCacheDir() const {
    //@TODO proper exception handling
    if(!exists(cacheDir)) create_directory(cacheDir);
}


const hint_base_t & gj::ro_hint_base_cacher::retreive(const std::string & filename) {
    auto iter = find(filename);
    if(iter != end()) return iter->second;
    hint_base_t hint_base = splitted_json_hint_base(loadCached(filename)).as_hint_base();
    return insert(std::pair<std::string, hint_base_t>(filename, hint_base)).first->second;
}

foreign_ref_holders_t gj::foreign_ref_holders_t::parse_from_json(const Json::Value & json){
    foreign_ref_holders_t result;
    for(auto iter = json.begin(); iter != json.end(); ++iter){
        unordered_set<std::string> set;
        for(auto val : *iter){
            set.insert(val.asString());
        }
        result[iter.key().asString()] = set;
    }
    return result;
}


Json::Value gj::foreign_ref_holders_t::as_json() const{
    Json::Value result;
    for(auto pr : *this){
        Json::Value setJson = Json::Value();
        for(const std::string & filename : pr.second)
            setJson.append(filename);
        result[pr.first] = setJson;
    }
    return result;
}
