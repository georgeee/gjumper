#ifndef GJ_DATATYPES
#define GJ_DATATYPES

#include <set>
#include <vector>
#include <utility>
#include <iostream>
#include <map>

using namespace std;

namespace gj{
    enum hint_type{
        REF_HT, DECL_HT
    };
    enum hint_target_type{
        REGULAR_HTT, EXPANSION_HTT, SPELLING_HTT
    };

    class printable {
        public:
            virtual ostream& printTo (ostream& os) const = 0;
    };

    ostream& operator<<(ostream& os, const hint_target_type target_type);
    ostream& operator<<(ostream& os, const hint_type type);
    ostream& operator<<(ostream& os, const printable & obj);

    struct pos_t : public printable{
        pos_t (int line, int index)
            : line(line), index(index){}
        const int line;
        const int index;
        ostream& printTo (ostream& os) const;
        const bool operator< (const pos_t& pos) const;
        const bool operator== (const pos_t& pos) const;
        const bool operator!= (const pos_t& pos) const;
        const bool operator<= (const pos_t& pos) const;
    };

    struct abs_pos_t : public printable{
        abs_pos_t (string file, pos_t pos)
            : file(file), pos(pos) {}
        abs_pos_t (string file, int line, int index)
            : file(file), pos(pos_t(line, index)) {}
        const string file;
        const pos_t pos;
        ostream& printTo (ostream& os) const;
        const bool operator< (const abs_pos_t& pos) const;
        const bool operator== (const abs_pos_t& pos) const;
        const bool operator!= (const abs_pos_t& pos) const;
    };

    struct named_pos_t : public abs_pos_t{
        named_pos_t (string file, pos_t pos, string name)
            : abs_pos_t(file, pos), name(name) {}
        named_pos_t (string file, int line, int index, string name)
            : abs_pos_t(file, pos_t(line, index)), name(name) {}
        const string name;
        ostream& printTo (ostream& os) const;
        const bool operator< (const named_pos_t & npos) const;
    };

    struct range_t : public printable{
        range_t (pos_t start, pos_t end)
            : start(start), end(end) {}
        range_t (int sline, int sindex, int eline, int eindex)
            : start(pos_t(sline, sindex)), end(pos_t(eline, eindex)) {}
        const pos_t start;
        const pos_t end;
        ostream& printTo (ostream& os) const;
        const bool operator< (const range_t & range) const;
    };

    struct hint_t : public printable{
        hint_t (range_t range, named_pos_t pos, hint_type type, hint_target_type target_type = REGULAR_HTT)
            : range(range), pos(pos), type(type), target_type(target_type) {}
        hint_t (const hint_t& hint)
            : range(hint.range), pos(hint.pos), type(hint.type), target_type(hint.target_type) {}
        const range_t range;
        const named_pos_t pos;
        const hint_type type;
        const hint_target_type target_type;
        ostream& printTo (ostream& os) const;
        const bool operator< (const hint_t & hint) const;
    };

    //@TODO At the moment hint_base implementation is very-very simple, in the future
    //it's better to implement interval tree
    class hint_base_t : private set<pair<pos_t, hint_t*> >, public printable{
        public:
            typedef pair<pos_t, hint_t*> element_t;
            void add(const hint_t & _hint);
            vector<hint_t*> resolve_position(pos_t pos) const;
            vector<hint_t*> resolve_position(int line, int index) const;
            ~hint_base_t();
            ostream& printTo (ostream& os) const;
    };

    class global_hint_base_t : private map<string, hint_base_t>, public printable{
        map<string, hint_base_t> hints;
        public:
            ostream& printTo (ostream& os) const;
            void add(const string & file, const hint_t & hint);

    };
}

#endif
