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
    public:
    static Json::Value getJSONValue(const hint_base_t & hints);
    static Json::Value getJSONValue(const hint_t & hint);
    static Json::Value getJSONValue(const pos_t & pos);
    static Json::Value getJSONValue(const abs_pos_t & pos);
    static Json::Value getJSONValue(hint_type type);
    static Json::Value getJSONValue(const hint_target_type & target_type);
    static Json::Value getJSONValue(const range_t & range);
    hintdb_json_exporter(std::ostream & out) : hintdb_exporter(out) {}
    virtual void export_base(const hint_base_t & hints);
};

class hintdb_json_importer : public hintdb_importer{
    public:
    static hint_t retrieve_hint(const Json::Value & jsonValue);
    static pos_t retrieve_pos(const Json::Value & jsonValue);
    static abs_pos_t retrieve_abs_pos(const Json::Value & jsonValue);
    static hint_type retrieve_hint_type(const Json::Value & jsonValue);
    static hint_target_type retrieve_hint_target_type(const Json::Value & jsonValue);
    static range_t retrieve_range(const Json::Value & jsonValue);
    static hint_base_t retrieve_hint_base(const Json::Value & jsonValue);
    hintdb_json_importer(std::istream & in) : hintdb_importer(in) {}
    virtual hint_base_t import_base();
};

class import_exception : public exception{};


class double_converted_json_hint_base {
    hint_base_t * hint_base_ptr;
    Json::Value * json_ptr;
    void copy_from(const double_converted_json_hint_base & base);
    void copy_from(double_converted_json_hint_base && base);
    public:
        double_converted_json_hint_base() : hint_base_ptr(new hint_base_t()), json_ptr(NULL) {}

        double_converted_json_hint_base(const hint_base_t & hint_base)
            : hint_base_ptr(new hint_base_t(hint_base)), json_ptr(NULL) {}
        double_converted_json_hint_base(const Json::Value & json)
            : hint_base_ptr(NULL), json_ptr(new Json::Value(json)) {}

        double_converted_json_hint_base(double_converted_json_hint_base && base){
            copy_from(std::forward<double_converted_json_hint_base&&>(base));
        }
        double_converted_json_hint_base(const double_converted_json_hint_base & base){
            copy_from(base);
        }

        double_converted_json_hint_base& operator=(const double_converted_json_hint_base & base){
            drop_json();
            drop_hint_base();
            copy_from(base);
            return *this;
        }
        double_converted_json_hint_base& operator=(double_converted_json_hint_base && base){
            drop_json();
            drop_hint_base();
            copy_from(std::forward<double_converted_json_hint_base &&>(base));
            return *this;
        }

        Json::Value* get_json_ptr(){
            return json_ptr;
        }
        hint_base_t* get_hint_base_ptr(){
            return hint_base_ptr;
        }
        hint_base_t & get_hint_base(){
            ensure_hint_base_not_null();
            return *get_hint_base_ptr();
        }
        Json::Value & get_json(){
            ensure_json_not_null();
            return *get_json_ptr();
        }
        void drop_json();
        void drop_hint_base();
        void ensure_json_not_null();
        void ensure_hint_base_not_null();
        virtual ~double_converted_json_hint_base();
        class empty_base_exception : public std::exception {};
};

class splitted_json_hint_base{
    static constexpr const char * const REST_IDX = "rest";
    static constexpr const char * const FOREIGN_REFS_IDX = "foreign";
    map<std::string, double_converted_json_hint_base> foreign_refs;
    double_converted_json_hint_base rest;
    public:
    splitted_json_hint_base(hint_base_t hint_base, std::string filename);
    splitted_json_hint_base(hint_base_t hint_base);
    splitted_json_hint_base(Json::Value json);
    //splitted_json_hint_base(){}
    void add_foreign_refs(const std::string & filename, double_converted_json_hint_base & dc_hint_base);
    void add_foreign_refs(const std::string & filename, const hint_base_t & hint_base);
    void replace_foreign_refs(const std::string & filename, const double_converted_json_hint_base & dc_hint_base);
    void clear_foreign_refs(const std::string & filename);
    double_converted_json_hint_base & get_rest();
    template<typename Base>
    void set_rest(Base&& dc_hint_base){
        rest = std::forward<Base>(dc_hint_base);
    }
    Json::Value as_json();
    hint_base_t as_hint_base();
};

}
#endif
