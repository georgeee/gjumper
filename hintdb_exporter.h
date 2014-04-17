#ifndef GJ_HINTDB_EXPORTER
#define GJ_HINTDB_EXPORTER

#include <iostream>
#include "jsoncpp/json/json.h"
#include "datatypes.h"

using namespace gj;

namespace gj{

class hintdb_exporter{
    protected:
    std::ostream & out;
    public:
    hintdb_exporter(std::ostream & out) : out(out) {}
    virtual void export_base(const hint_base_t & hints) = 0;
};
class hintdb_importer{
    protected:
    std::istream & in;
    public:
    hintdb_importer(std::istream & in) : in(in) {}
    virtual hint_base_t import_base() = 0;
};

struct hintdb_json_idxs{
    static const int hint_range;
    static const int hint_pos;
    static const int hint_src_name;
    static const int hint_dest_name;
    static const int hint_type;
    static const int hint_target_type;
};

struct hintdb_json_values{
    static const Json::Value ref_ht;
    static const Json::Value decl_ht;
    static const Json::Value regular_htt;
    static const Json::Value expansion_htt;
    static const Json::Value spelling_htt;
};

class hintdb_json_exporter : public hintdb_exporter{
    Json::Value getJSONValue(const hint_base_t & hints);
    Json::Value getJSONValue(const hint_t & hint);
    Json::Value getJSONValue(const pos_t & pos);
    Json::Value getJSONValue(const abs_pos_t & pos);
    Json::Value getJSONValue(const hint_type & type);
    Json::Value getJSONValue(const hint_target_type & target_type);
    Json::Value getJSONValue(const range_t & range);

    public:
    hintdb_json_exporter(std::ostream & out) : hintdb_exporter(out) {}
    virtual void export_base(const hint_base_t & hints);
};

class hintdb_json_importer : public hintdb_importer{
    hint_base_t retrieve_hint_base(const Json::Value & jsonValue);
    hint_t retrieve_hint(const Json::Value & jsonValue);
    pos_t retrieve_pos(const Json::Value & jsonValue);
    abs_pos_t retrieve_abs_pos(const Json::Value & jsonValue);
    hint_type retrieve_hint_type(const Json::Value & jsonValue);
    hint_target_type retrieve_hint_target_type(const Json::Value & jsonValue);
    range_t retrieve_range(const Json::Value & jsonValue);

    public:
    virtual hint_base_t import_base();

};

class import_exception : public exception{};

}
#endif
