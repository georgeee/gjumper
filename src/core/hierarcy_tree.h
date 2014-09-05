#ifndef GJ_HIERARCY_TREE_H
#define GJ_HIERARCY_TREE_H

#include <map>
#include <unordered_map>
#include <set>
#include <vector>
#include <memory>
#include <string>

#include "jsoncpp/json/json.h"

#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Token.h"

using std::shared_ptr;
using std::string;
using std::set;
using std::map;
using std::unordered_map;
using std::vector;

using clang::SourceLocation;
using clang::SourceManager;

namespace gj{
    class hierarcy_tree_node{
        friend class hierarcy_tree;
        typedef unordered_map<string, shared_ptr<hierarcy_tree_node> > node_set;
        node_set parents;
        node_set children;
        int id; //is used as temporary value during serialization process
        public:
        const string filename;
        hierarcy_tree_node(string filename) : filename(filename) {}
        node_set::iterator parents_end(){
            return parents.end();
        }
        node_set::iterator children_end(){
            return children.end();
        }
        node_set::iterator parents_begin(){
            return parents.begin();
        }
        node_set::iterator children_begin(){
            return children.begin();
        }
    };
    class hierarcy_tree : public unordered_map<string, shared_ptr<hierarcy_tree_node> >{
        static constexpr const char * const JSON_NAMES = "names";
        static constexpr const char * const JSON_EDGES = "edges";
        public:
            hierarcy_tree(){}
            hierarcy_tree(const hierarcy_tree & tree);
            hierarcy_tree(hierarcy_tree && tree);
            void add_edge(shared_ptr<hierarcy_tree_node> parent, shared_ptr<hierarcy_tree_node> child);
            void add_edge(string parent, shared_ptr<hierarcy_tree_node> child);
            void add_edge(string parent, string child);

            Json::Value as_json() const;
            static hierarcy_tree parse_from_json(Json::Value json);
            void replace_parents(std::string filename, vector<string> parents);
            void replace_parents(shared_ptr<hierarcy_tree_node> child, vector<string> parents);
            shared_ptr<hierarcy_tree_node> get_or_create(std::string filename);
            virtual ~hierarcy_tree();
    };

    class SimpleHierarcyTreeCollector : public clang::PPCallbacks {
        private:
            SourceManager & sourceManager;
            vector<string> & parentHolder;
            const string file_filter;
        public:
            SimpleHierarcyTreeCollector(vector<string> & parentHolder, SourceManager & sourceManager, string file_filter) : parentHolder(parentHolder), sourceManager(sourceManager), file_filter(file_filter) {}
            virtual void FileChanged(clang::SourceLocation loc, FileChangeReason reason,
                    clang::SrcMgr::CharacteristicKind file_type,
                    clang::FileID PrevFID);
    };
    class HierarcyTreeCollector : public clang::PPCallbacks {
        private:
            SourceManager & sourceManager;
            hierarcy_tree & tree;
            const string file_filter;
        public:
            HierarcyTreeCollector(hierarcy_tree & tree, SourceManager & sourceManager, string file_filter = string()) : tree(tree), sourceManager(sourceManager), file_filter(file_filter) {}
            virtual void FileChanged(clang::SourceLocation loc, FileChangeReason reason,
                    clang::SrcMgr::CharacteristicKind file_type,
                    clang::FileID PrevFID);
    };

}
#endif
