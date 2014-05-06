#include "hintdb_exporter.h"
#include "datatypes.h"

using namespace gj;

const int hintdb_json_idxs::hint_range = 0;
const int hintdb_json_idxs::hint_pos = 1;
const int hintdb_json_idxs::hint_src_name = 2;
const int hintdb_json_idxs::hint_dest_name = 3;
const int hintdb_json_idxs::hint_type = 4;
const int hintdb_json_idxs::hint_target_type = 5;

const Json::Value hintdb_json_values::ref_ht = "ref";
const Json::Value hintdb_json_values::decl_ht = "decl";
const Json::Value hintdb_json_values::regular_htt = "reg";
const Json::Value hintdb_json_values::expansion_htt = "exp";
const Json::Value hintdb_json_values::spelling_htt = "spell";


void hintdb_json_exporter::export_base(const hint_base_t & hints){
    Json::FastWriter writer;
    out << writer.write(getJSONValue(hints));
}

Json::Value hintdb_json_exporter::getJSONValue(const hint_base_t & hints){
    Json::Value val;
    int i = 0;
    for(auto pair : hints)
        val[i++] = getJSONValue(*pair.second);
    return val;
}
hint_base_t hintdb_json_importer::retrieve_hint_base(const Json::Value & jsonValue){
    hint_base_t hints;
    for(auto val : jsonValue)
        hints.add(retrieve_hint(val));
    return hints;
}

Json::Value hintdb_json_exporter::getJSONValue(const hint_t & hint){
    Json::Value val;
    val[hintdb_json_idxs::hint_range] = getJSONValue(hint.range);
    val[hintdb_json_idxs::hint_pos] = getJSONValue(hint.pos);
    val[hintdb_json_idxs::hint_src_name] = hint.src_name;
    val[hintdb_json_idxs::hint_dest_name] = hint.dest_name;
    val[hintdb_json_idxs::hint_type] = getJSONValue(hint.type);
    val[hintdb_json_idxs::hint_target_type] = getJSONValue(hint.target_type);
    return val;
}

hint_t hintdb_json_importer::retrieve_hint(const Json::Value & val){
    return hint_t(retrieve_range(val[hintdb_json_idxs::hint_range]),
                  retrieve_abs_pos(val[hintdb_json_idxs::hint_pos]),
                  retrieve_hint_type(val[hintdb_json_idxs::hint_type]),
                  val[hintdb_json_idxs::hint_src_name].asString(),
                  val[hintdb_json_idxs::hint_dest_name].asString(),
                  retrieve_hint_target_type(val[hintdb_json_idxs::hint_target_type])); 
}

Json::Value hintdb_json_exporter::getJSONValue(const pos_t & pos){
    Json::Value val;
    val[0] = pos.line;
    val[1] = pos.index;
    return val;
}

pos_t hintdb_json_importer::retrieve_pos(const Json::Value & val){
    return pos_t(val[0].asInt(), val[1].asInt());
}

Json::Value hintdb_json_exporter::getJSONValue(const abs_pos_t & pos){
    Json::Value val;
    val[0] = pos.file;
    val[1] = pos.pos.line;
    val[2] = pos.pos.index;
    return val;
}

abs_pos_t hintdb_json_importer::retrieve_abs_pos(const Json::Value & val){
    return abs_pos_t(val[0].asString(), val[1].asInt(), val[2].asInt());
}

Json::Value hintdb_json_exporter::getJSONValue(hint_type type){
    switch(type){
        case REF_HT: return hintdb_json_values::ref_ht;
        case DECL_HT: return hintdb_json_values::decl_ht;
    }
    return Json::Value();
}

hint_type hintdb_json_importer::retrieve_hint_type(const Json::Value & val){
    if(val == hintdb_json_values::ref_ht) return REF_HT;
    if(val == hintdb_json_values::decl_ht) return DECL_HT;
    throw import_exception();
}

Json::Value hintdb_json_exporter::getJSONValue(const hint_target_type & target_type){
    switch(target_type){
        case REGULAR_HTT: return hintdb_json_values::regular_htt;
        case EXPANSION_HTT: return hintdb_json_values::expansion_htt;
        case SPELLING_HTT: return hintdb_json_values::spelling_htt;
    }
    return Json::Value();
}

hint_target_type hintdb_json_importer::retrieve_hint_target_type(const Json::Value & val){
    if(val == hintdb_json_values::regular_htt) return REGULAR_HTT;
    if(val == hintdb_json_values::expansion_htt) return EXPANSION_HTT;
    if(val == hintdb_json_values::spelling_htt) return SPELLING_HTT;
    throw import_exception();
}

Json::Value hintdb_json_exporter::getJSONValue(const range_t & range){
    Json::Value val;
    val[0] = getJSONValue(range.start);
    val[1] = getJSONValue(range.end);
    return val;
}

range_t hintdb_json_importer::retrieve_range(const Json::Value & val){
    return range_t(retrieve_pos(val[0]), retrieve_pos(val[1]));
}

hint_base_t hintdb_json_importer::import_base(){
    Json::Value val;
    in >> val;
    return retrieve_hint_base(val);
} 

double_converted_json_hint_base::double_converted_json_hint_base(hint_base_t hint_base) : json(NULL){
    this->hint_base = new hint_base_t(hint_base);
}

double_converted_json_hint_base::double_converted_json_hint_base(Json::Value json) : hint_base(NULL){
    json = new Json::Value(json);
}

double_converted_json_hint_base::~double_converted_json_hint_base(){
    if(json != NULL) delete json;
    if(hint_base != NULL) delete hint_base;
}

void double_converted_json_hint_base::drop_json(){
    if(json != NULL) delete json;
}
void double_converted_json_hint_base::drop_hint_base(){
    if(hint_base != NULL) delete hint_base;
}

void double_converted_json_hint_base::ensure_json_not_null(){
    if(json != NULL) return;
    json = new Json::Value(hintdb_json_exporter::getJSONValue(*hint_base));
}


void double_converted_json_hint_base::ensure_hint_base_not_null(){
    if(hint_base != NULL) return;
    hint_base = new hint_base_t(hintdb_json_importer::retrieve_hint_base(*json));
}

splitted_json_hint_base::splitted_json_hint_base(hint_base_t hint_base, std::string filename){
    hint_base_t rest_hdb;
    //for(pair<const std::string, const hint_t*> _pair : hint_base)
        //std::string & _filename = _pair.first;
    for(pair<pos_t, const hint_t *> _pair : hint_base){
        const hint_t & hint = *_pair.second;
        const std::string & _filename = hint.pos.file;
        if(_filename == filename) 
            rest_hdb.add(hint);
        else{
            auto iter = foreign_refs.find(_filename);
            if(iter == foreign_refs.end()){
                hint_base_t hdb;
                hdb.add(hint);
                foreign_refs.insert(make_pair(_filename, hdb));
            }else{
                iter->second.get_hint_base().add(hint);
            }
        }
    }
    rest = rest_hdb;
}

splitted_json_hint_base::splitted_json_hint_base(hint_base_t hint_base) : rest(hint_base) {}
splitted_json_hint_base::splitted_json_hint_base(Json::Value json) : rest(json[REST_IDX]) {
    Json::Value & foreign_json = json[FOREIGN_REFS_IDX];
    for(auto iter = foreign_json.begin(); iter != foreign_json.end(); ++iter){
        std::string key = iter.key().asString();
        foreign_refs[key] = *iter;
    }
}

hint_base_t splitted_json_hint_base::as_hint_base(){
    hint_base_t hint_base = rest.get_hint_base();
    for(pair<const std::string, double_converted_json_hint_base> _pair : foreign_refs)
        hint_base.add(_pair.second.get_hint_base());
    return hint_base;
}

Json::Value splitted_json_hint_base::as_json(){
    Json::Value json;
    json[REST_IDX] = rest.get_json();
    Json::Value foreign_json;
    for(pair<const std::string, double_converted_json_hint_base> _pair : foreign_refs)
        foreign_json[_pair.first] = _pair.second.get_json();
    json[FOREIGN_REFS_IDX] = foreign_json;
    return json;
}

void splitted_json_hint_base::replace_foreign_refs(std::string filename, double_converted_json_hint_base hint_base){
    foreign_refs[filename] = hint_base;
}
void splitted_json_hint_base::add_foreign_refs(std::string filename, double_converted_json_hint_base hint_base){
    auto iter = foreign_refs.find(filename);
    if(iter == foreign_refs.end()) foreign_refs[filename] = hint_base;
    else{
        foreign_refs[filename].get_hint_base().add(hint_base.get_hint_base());
        foreign_refs[filename].drop_json();
    }
}

void splitted_json_hint_base::clear_foreign_refs(std::string filename){
    foreign_refs.erase(filename);
}


double_converted_json_hint_base & splitted_json_hint_base::get_rest(){
    return rest;
}
void splitted_json_hint_base::set_rest(double_converted_json_hint_base hint_base){
    rest = hint_base;
}



