#include "hierarcy_tree.h"

#include <memory>
#include <functional>
#include <utility>
#include <map>
#include <string>

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"


using std::shared_ptr;
using std::string;
using std::make_shared;
using clang::SourceLocation;
using clang::SourceManager;
using namespace gj;

void gj::hierarcy_tree::add_edge(shared_ptr<hierarcy_tree_node> parent, shared_ptr<hierarcy_tree_node> child){
    parent->children.insert(make_pair(child->filename, child));
    child->parents.insert(make_pair(parent->filename, parent));
}
void gj::hierarcy_tree::add_edge(string parent_filename, shared_ptr<hierarcy_tree_node> child){
    add_edge(get_or_create(parent_filename), child);
}
void gj::hierarcy_tree::add_edge(string parent_filename, string child_filename){
    add_edge(parent_filename, get_or_create(child_filename));
}

void gj::SimpleHierarcyTreeCollector::FileChanged (clang::SourceLocation file_beginning,
        FileChangeReason reason, clang::SrcMgr::CharacteristicKind file_type, clang::FileID PrevFID){
    if(reason == EnterFile){
        const SourceLocation include_loc = sourceManager.getIncludeLoc(sourceManager.getFileID(file_beginning));
        const char * includee_filename = sourceManager.getPresumedLoc(file_beginning).getFilename();
        if(include_loc.isInvalid()) return;
        const char * includer_filename = sourceManager.getPresumedLoc(include_loc).getFilename();
        if(!file_filter.empty() && file_filter != includer_filename) return;
        parentHolder.push_back(includee_filename);
    }
}

void gj::HierarcyTreeCollector::FileChanged (clang::SourceLocation file_beginning,
        FileChangeReason reason, clang::SrcMgr::CharacteristicKind file_type, clang::FileID PrevFID){
    if(reason == EnterFile){
        const SourceLocation include_loc = sourceManager.getIncludeLoc(sourceManager.getFileID(file_beginning));
        const char * includee_filename = sourceManager.getPresumedLoc(file_beginning).getFilename();
        if(include_loc.isInvalid()) return;
        const char * includer_filename = sourceManager.getPresumedLoc(include_loc).getFilename();
        if(!file_filter.empty() && file_filter != includer_filename) return;
        tree.add_edge(includee_filename, includer_filename);
    }
}

Json::Value gj::hierarcy_tree::as_json() const {
    int id = 0;
    Json::Value result;
    vector<shared_ptr<hierarcy_tree_node> > toplevel_nodes;//no parents
    for(auto pr : *this){
        shared_ptr<hierarcy_tree_node> & node = pr.second;
        result[JSON_NAMES][id] = node->filename;
        node->id = id++;
        if(node->parents.empty()) toplevel_nodes.push_back(node);
    }
    std::function<void (shared_ptr<hierarcy_tree_node>)> dfs = [&dfs, &result](shared_ptr<hierarcy_tree_node> node){
        for(auto pr : node->children){
            shared_ptr<hierarcy_tree_node> & child = pr.second;
            result[JSON_EDGES].append(node->id);
            result[JSON_EDGES].append(child->id);
            dfs(child);
        }
    };
    for(auto node : toplevel_nodes){
        dfs(node);
    }
    return result;
}
hierarcy_tree gj::hierarcy_tree::parse_from_json(Json::Value json){
    hierarcy_tree result;
    vector<shared_ptr<hierarcy_tree_node> > nodes(json[JSON_NAMES].size());
    int id = 0;
    for(auto val : json[JSON_NAMES]){
        const string filename = val.asString();
        shared_ptr<hierarcy_tree_node> node = make_shared<hierarcy_tree_node>(filename);
        nodes[id++] = node;
        result.insert({filename, node});
    }
    int prev = -1;
    for(auto val : json[JSON_EDGES]){
        if(prev == -1) prev = val.asInt();
        else{
            result.add_edge(nodes[prev], nodes[val.asInt()]);
            prev = -1;
        }
    }
    return result;
}
void gj::hierarcy_tree::replace_parents(shared_ptr<hierarcy_tree_node> child, vector<string> parents){
    const string & filename = child->filename;
    for(std::pair<string, shared_ptr<hierarcy_tree_node> > pr : child->parents){
        shared_ptr<hierarcy_tree_node> parent = pr.second;
        parent->children.erase(filename);
    }
    child->parents.clear();
    for(const string & parent_filename : parents){
        add_edge(parent_filename, child);
    }
}
shared_ptr<hierarcy_tree_node> gj::hierarcy_tree::get_or_create(std::string filename){
    shared_ptr<hierarcy_tree_node> node;
    auto iter = find(filename);
    if(iter == end()){
        node = make_shared<hierarcy_tree_node>(filename);
        insert({filename, node});
    }else{
        node = iter->second;
    }
    return node;
}
void gj::hierarcy_tree::replace_parents(std::string filename, vector<string> parents){
    replace_parents(get_or_create(filename), parents);
}
