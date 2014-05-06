#include "processor.h"
#include "datatypes.h"
#include "hintdb_exporter.h"
#include "hierarcy_tree.h"

#include "jsoncpp/json/json.h"

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

#include <fstream>
#include <memory>
#include <iostream>
#include <functional>
#include <boost/filesystem.hpp>
using namespace boost::filesystem;

using namespace clang;
using namespace std;

gj::processor::processor(char ** compilerOptionsStart, char ** compilerOptionsEnd, const std::string & cacheDir) : cacheDir(cacheDir), hintBaseCacher(cacheDir) {
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

pair<global_hint_base_t, vector<string> >  gj::processor::collect(const std::string & fileName){
    global_hint_base_t hint_base(fileName);
    vector<string> parents;
    SimpleHierarcyTreeCollector treeCollector(parents, compiler.getSourceManager(), fileName);
    compiler.createSourceManager(compiler.getFileManager());
    compiler.createPreprocessor();
    compiler.getPreprocessor().addPPCallbacks(&treeCollector);
    compiler.getPreprocessorOpts().UsePredefines = false;
    compiler.createASTContext();

    const FileEntry *pFile = compiler.getFileManager().getFile(fileName);
    compiler.getSourceManager().createMainFileID(pFile);
    compiler.getDiagnosticClient().BeginSourceFile(compiler.getLangOpts(),
            &compiler.getPreprocessor());
    GASTConsumer astConsumer(compiler.getSourceManager(), hint_base);
    ParseAST(compiler.getPreprocessor(), &astConsumer, compiler.getASTContext());
    compiler.getDiagnosticClient().EndSourceFile();

    return make_pair(hint_base, parents);
}

void gj::hint_db_cache_manager::saveCached(const std::string & filename, const Json::Value & jsonValue) const {
    const std::string cachePath = getCachePath(filename);
    std::ofstream t;
    t.open(cachePath);
    t << jsonValue;
}


Json::Value gj::ro_hint_db_cache_manager_base::loadCached(const std::string & filename) const {
    const std::string cachePath = getCachePath(filename);
    if(exists(cachePath)){
        std::ifstream t;
        t.open(cachePath);
        Json::Value json;
        t >> json;
        return json;
    }
    return Json::Value();
}


std::string gj::ro_hint_db_cache_manager_base::getCachePath(const std::string & filename) const {
    mkCacheDir();
    return cacheDir + PATH_SEPARATOR + std::to_string(std::hash<std::string>()(filename)) + CACHE_EXT;
}

void gj::hint_db_cache_manager::saveHierarcyTree(const hierarcy_tree & tree) const {
    saveCached(HIERARCY_TREE_CACHE_FILENAME, tree.as_json());
}

bool gj::ro_hint_db_cache_manager::is_cached(const std::string & filename) const {
    return find(filename) != end();
}
splitted_json_hint_base & gj::ro_hint_db_cache_manager::retreive_impl(const std::string & filename){
    auto iter = find(filename);
    if(iter == end()) {
        insert({filename, loadCached(filename)});
        return find(filename)->second;
    }
    return iter->second;
}

foreign_ref_holders_t & gj::ro_hint_db_cache_manager::retreive_FRH_impl(){
    if(cachedFRH) return *cachedFRH;
    return *(cachedFRH = new foreign_ref_holders_t(foreign_ref_holders_t::parse_from_json(loadCached(FRH_CACHE_FILENAME))));
}
hierarcy_tree & gj::ro_hint_db_cache_manager::retreive_hierarcy_impl(){
    if(cachedTree) return *cachedTree;
    return *(cachedTree = new hierarcy_tree(hierarcy_tree::parse_from_json(loadCached(HIERARCY_TREE_CACHE_FILENAME))));
}

void gj::hint_db_cache_manager::saveFRH(const foreign_ref_holders_t & frh) const {
    saveCached(FRH_CACHE_FILENAME, frh.as_json());
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
    return retreive_hierarcy_impl();
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

void gj::processor::recache_dfs(hint_db_cache_manager & cacheManager, const shared_ptr<hierarcy_tree_node> & node, unordered_set<std::string> & visited, bool throw_on_cycle){
    const std::string & node_filename = node->filename;
    if(visited.find(node_filename) != visited.end()) {
        if(throw_on_cycle) throw recache_exception();
        else return;
    }
    visited.insert(node_filename);
    hierarcy_tree hierarcy = cacheManager.retreive_hierarcy();
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
        recache_dfs(cacheManager, child_iter->second, visited, throw_on_cycle);
    }
}

void gj::processor::recache(const std::string & filename){
    hint_db_cache_manager cacheManager;
    unordered_set<std::string> visited;
    hierarcy_tree hierarcy = cacheManager.retreive_hierarcy();
    try{
        recache_dfs(cacheManager, hierarcy.get_or_create(filename), visited, true);
    }catch(recache_exception ex){
        full_recache();
    }
    hintBaseCacher.clear();
}

void gj::ro_hint_db_cache_manager_base::mkCacheDir() const {
    //@TODO proper exception handling
    if(!exists(cacheDir)) create_directory(cacheDir);
}

bool gj::GASTConsumer::HandleTopLevelDecl(DeclGroupRef d){
    typedef DeclGroupRef::iterator iter;
    for (iter b = d.begin(), e = d.end(); b != e; ++b)
    {
        rv.StartTraversing(*b);
    }
    return true;
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

void gj::processor::full_recache(){
    rmCacheDir();
    hint_db_cache_manager cacheManager;
    unordered_set<std::string> visited;
    hierarcy_tree hierarcy = cacheManager.retreive_hierarcy();
    for(const std::string & filename : project_files)
        recache_dfs(cacheManager, hierarcy.get_or_create(filename), visited);
    hintBaseCacher.clear();
}


void gj::processor::rmCacheDir() const{
    if(exists(cacheDir)) remove_all(cacheDir);
}

const hint_base_t & gj::ro_hint_base_cacher::retreive(const std::string & filename) {
    auto iter = find(filename);
    if(iter != end()) return iter->second;
    hint_base_t hint_base = splitted_json_hint_base(loadCached(filename)).as_hint_base();
    return insert(std::pair<std::string, hint_base_t>(filename, hint_base)).first->second;
}

vector<hint_t> gj::processor::resolve_position(const std::string & filename, pos_t pos) {
    const hint_base_t & hint_base = hintBaseCacher.retreive(filename);
    vector<const hint_t *> hint_ptrs = hint_base.resolve_position(pos);
    vector<hint_t> result;
    for(const hint_t* hint_ptr : hint_ptrs)
        result.push_back(*hint_ptr);
    return result;
}
vector<hint_t> gj::processor::resolve_position(const std::string & filename, int line, int index) {
    return resolve_position(filename, pos_t(line, index));
}
