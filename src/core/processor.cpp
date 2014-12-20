#include "processor.h"
#include "hint_db_cache_manager.h"
#include "datatypes.h"
#include "hintdb_exporter.h"
#include "hierarcy_tree.h"
#include "fs_utils.h"

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
#include "clang/Frontend/CompilerInvocation.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/Version.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "clang/Driver/Option.h"
#include "clang/Driver/ArgList.h"
#include "clang/Driver/Arg.h"
#include "clang/Driver/OptTable.h"
#include "clang/Driver/Util.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Option/Option.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"

#include <fstream>
#include <memory>
#include <iostream>
#include <functional>
#include <boost/filesystem.hpp>
#include <cstdio>

using namespace boost::filesystem;

using namespace clang;
using namespace std;

vector<std::string> gj::readStringVector(const Json::Value & json){
    vector<std::string> result;
    for(auto & val : json) result.push_back(val.asString());
    return result;
}


gj::compileConfig::compileConfig(Json::Value json) : compileConfig(readStringVector(json[PCCJ_OPTIONS]), json[PCCJ_ISCXX].asBool()) {}

unordered_map<std::string, compileConfig> gj::processor::loadPrjCompileConfig(const std::string & prjCompileConfigPath){
    unordered_map<std::string, compileConfig> result;
    if(!prjCompileConfigPath.empty() && exists(prjCompileConfigPath)){
        std::ifstream t;
        t.open(prjCompileConfigPath);
        Json::Value json;
        t >> json;
        for(auto it = json.begin(); it != json.end(); ++it){
            result[it.key().asString()] = compileConfig(*it);
        }
    }
    return result;
}

static std::string resolve_base_dir(){
    path cur = current_path();
    while (exists(cur)){
        if(exists(path(cur) /= PROJECT_COMPILE_CONFIG_NAME)) return cur.string();
        cur = cur.parent_path();
    }
    throw std::runtime_error("couldn't find base dir");
}

gj::processor::processor() : processor(resolve_base_dir()) {}
gj::processor::processor(const std::string & baseDir) : processor(baseDir, baseDir + '/' + CACHE_DIR_NAME, baseDir + '/' + PROJECT_COMPILE_CONFIG_NAME) {}
gj::processor::processor(const std::string & baseDir, const std::string & cacheDir, const std::string & prjCompileConfigPath): baseDir(baseDir), cacheDir(cacheDir), hintBaseCacher(baseDir, cacheDir), projectCompileConfig(loadPrjCompileConfig(prjCompileConfigPath)) {}

void gj::processor::initHeaderSearchOptions(CompilerInstance & compiler){
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

pair<global_hint_base_t, vector<string> >  gj::processor::collect(const std::string & fileName, const compileConfig & config){
    const vector<std::string> & compilerOptions = config.options;
    std::unique_ptr<const char * []> compilerOptionsArr (new const char*[compilerOptions.size()]);
    for(size_t i =0; i< compilerOptions.size(); ++i){
        compilerOptionsArr[i] = compilerOptions[i].c_str();
    }
    auto res = collect(fileName, compilerOptionsArr.get(), compilerOptionsArr.get() + compilerOptions.size(), config.isCxx);
    return res;
}
pair<global_hint_base_t, vector<string> >  gj::processor::collect(const std::string & fileName, const char ** compilerOptionsStart, const char ** compilerOptionsEnd, bool isCxx){
    global_hint_base_t hint_base(fileName);
    vector<string> parents;

    CompilerInstance compiler;
    CompilerInvocation * Invocation = new CompilerInvocation;
    compiler.createDiagnostics();

    // Create an invocation that passes any flags to preprocessor
    CompilerInvocation::CreateFromArgs(*Invocation, compilerOptionsStart, compilerOptionsEnd, compiler.getDiagnostics());
    compiler.setInvocation(Invocation);

    // Set default target triple
    llvm::IntrusiveRefCntPtr<TargetOptions> pto(new TargetOptions());
    pto->Triple = llvm::sys::getDefaultTargetTriple();
    llvm::IntrusiveRefCntPtr<TargetInfo>
        pti(TargetInfo::CreateTargetInfo(compiler.getDiagnostics(),
                    pto.getPtr()));
    compiler.setTarget(pti.getPtr());

    compiler.createFileManager();
    compiler.createSourceManager(compiler.getFileManager());
    initHeaderSearchOptions(compiler);

    LangOptions langOpts;
    langOpts.GNUMode = 1; 
    langOpts.CXXExceptions = 1; 
    langOpts.RTTI = 1; 
    langOpts.Bool = 1; 
    langOpts.CPlusPlus = isCxx; 
    Invocation->setLangDefaults(langOpts,
            clang::IK_CXX,
            clang::LangStandard::lang_cxx0x);

    compiler.createPreprocessor();
    compiler.getPreprocessor().addPPCallbacks(new SimpleHierarcyTreeCollector(parents, compiler.getSourceManager(), fileName));
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

void gj::processor::recache_dfs(hint_db_cache_manager & cacheManager, const shared_ptr<hierarcy_tree_node> & node, unordered_set<std::string> & visited, bool throw_on_cycle){
    const std::string & node_filename = node->filename;
    if(visited.find(node_filename) != visited.end()) {
        if(throw_on_cycle) throw recache_exception();
        else return;
    }
    visited.insert(node_filename);
    hierarcy_tree & hierarcy = cacheManager.retreive_hierarcy();
    //@TODO add exception when in prjCompileConfigPath there is no
    //node_filename
    auto collectResult = collect(node_filename, projectCompileConfig[node_filename]);
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
    hint_db_cache_manager cacheManager(baseDir, cacheDir);
    unordered_set<std::string> visited;
    hierarcy_tree & hierarcy = cacheManager.retreive_hierarcy();
    try{
        recache_dfs(cacheManager, hierarcy.get_or_create(filename), visited, true);
    }catch(recache_exception ex){
        full_recache();
    }
    hintBaseCacher.clear();
}

bool gj::GASTConsumer::HandleTopLevelDecl(DeclGroupRef d){
    typedef DeclGroupRef::iterator iter;
    for (iter b = d.begin(), e = d.end(); b != e; ++b)
    {
        rv.StartTraversing(*b);
    }
    return true;
}


void gj::processor::full_recache(){
    rmCacheDir();
    hint_db_cache_manager cacheManager(baseDir, cacheDir);
    unordered_set<std::string> visited;
    hierarcy_tree & hierarcy = cacheManager.retreive_hierarcy();
    for(auto iter = projectCompileConfig.begin(); iter != projectCompileConfig.end(); ++iter)
        recache_dfs(cacheManager, hierarcy.get_or_create(iter->first), visited);
    hintBaseCacher.clear();
}


void gj::processor::rmCacheDir() const{
    if(exists(cacheDir)) remove_all(cacheDir);
}

vector<shared_ptr<hint_t> > gj::processor::resolve_position(const std::string & filename, pos_t pos) {
    const hint_base_t & hint_base = hintBaseCacher.retreive(filename);
    return hint_base.resolve_position(pos);
}
vector<shared_ptr<hint_t> > gj::processor::resolve_position(const std::string & filename, int line, int index) {
    return resolve_position(filename, pos_t(line, index));
}
vector<shared_ptr<hint_t> > gj::processor::resolve_position(const std::string & filename, int line) {
    const hint_base_t & hint_base = hintBaseCacher.retreive(filename);
    return hint_base.resolve_position(line);
}

void gj::addToCompileConfig(bool isCxx, char ** argv, int argc, const std::string & baseDir){
    path prjCompileConfigPath = path(baseDir) /= PROJECT_COMPILE_CONFIG_NAME;
    vector<std::string> opts;
    for(int i=0; i<argc; ++i) opts.push_back(argv[i]);
    Json::Value json;
    if(exists(prjCompileConfigPath)){
        std::ifstream in;
        in.open(prjCompileConfigPath.string());
        in >> json;
    }
    vector<std::string> files = getFilesFromArgs(argv, argc);
    path baseDirPath = canonical(baseDir);
    for(std::string filename : files){
        try{
            filename = relativize(baseDirPath, filename).string();
            std::cerr << filename << "\n";
            json[filename] = compileConfig(opts, isCxx).asJson();
        }catch(filesystem_error er){
        }
    }
    std::ofstream out;
    out.open(prjCompileConfigPath.string());
    out << json;
}

using namespace clang::driver;
using namespace clang::driver::options;


vector<std::string> gj::getFilesFromArgs(char ** argv, int argc){
    std::unique_ptr<OptTable> Opts(createDriverOptTable());
    unsigned MissingArgIndex, MissingArgCount;
    std::unique_ptr<InputArgList> Args(Opts->ParseArgs(argv, argv + argc, MissingArgIndex, MissingArgCount));
    vector<std::string> result;
    for (arg_iterator it = Args->filtered_begin(OPT_INPUT), ie = Args->filtered_end(); it != ie; ++it) {
        result.push_back((*it)->getAsString(*Args));
    }
    return result;
}

Json::Value gj::compileConfig::asJson() const {
    Json::Value result;
    result[PCCJ_OPTIONS] = Json::Value();
    result[PCCJ_ISCXX] = isCxx;
    Json::Value & optsJson = result[PCCJ_OPTIONS];
    for(const std::string & opt : options) optsJson.append(opt);
    return result;
}
