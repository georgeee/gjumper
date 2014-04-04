#include <set>
#include <vector>
#include <utility>
#include <iostream>
#include <map>
#include "datatypes.h"

using namespace std;
using namespace gj;

ostream& gj::operator<<(ostream& os, const hint_target_type target_type) {
    switch (target_type) {
        case REGULAR_HTT:
            cout << "regular";
            break;
        case SPELLING_HTT:
            cout << "spelling";
            break;
        case EXPANSION_HTT:
            cout << "expansion";
            break;
    }
    return os;
}

ostream& gj::operator<<(ostream& os, const hint_type type) {
    switch (type) {
        case DECL_HT:
            cout << "declaration";
            break;
        case REF_HT:
            cout << "reference";
            break;
    }
    return os;
}

ostream& gj::operator<<(ostream& os, const printable & obj) {
    return obj.printTo(os);
}

ostream& gj::pos_t::printTo (ostream& os) const{
    return os << '(' << line << ";" << index << ')';
}
const bool gj::pos_t::operator< (const pos_t& pos) const{
    if(line != pos.line)
        return line < pos.line;
    return index < pos.index;
}
const bool gj::pos_t::operator== (const pos_t& pos) const{
    return index == pos.index && line == pos.line;
}
const bool gj::pos_t::operator!= (const pos_t& pos) const{
    return !(*this==pos);
}
const bool gj::pos_t::operator<= (const pos_t& pos) const{
    return *this==pos || *this < pos;
}

ostream& gj::abs_pos_t::printTo (ostream& os) const{
    return os << '(' << file << ": " << pos.line << ";" << pos.index << ')';
}
const bool gj::abs_pos_t::operator< (const abs_pos_t& pos) const{
    int cmp = file.compare(pos.file);
    if(cmp != 0) return cmp < 0;
    return this->pos < pos.pos;
}
const bool gj::abs_pos_t::operator== (const abs_pos_t& pos) const{
    return file.compare(pos.file) == 0 && this->pos == pos.pos;
}
const bool gj::abs_pos_t::operator!= (const abs_pos_t& pos) const{
    return !(*this==pos);
}

ostream& gj::named_pos_t::printTo (ostream& os) const{
    os << '<' << name << ", ";
    abs_pos_t::printTo(os);
    os << '>';
    return os;
}
const bool gj::named_pos_t::operator< (const named_pos_t & npos) const{
    if(abs_pos_t::operator==(npos)) return name.compare(npos.name) < 0;
    return abs_pos_t::operator<(npos);
}
ostream& gj::range_t::printTo (ostream& os) const{
    return os << start << ".." << end;
}
const bool gj::range_t::operator< (const range_t & range) const{
    return (start != range.start) ? start < range.start : end < range.end;
}
ostream& gj::hint_t::printTo (ostream& os) const{
    return os << '{' << range << ' ' << pos << ' ' << type << ' ' << target_type << '}';
}
const bool gj::hint_t::operator< (const hint_t & hint) const{
    if(range.start != hint.range.start) return range.start < hint.range.start;
    if(type != hint.type) return type < hint.type;
    int cmp = pos.name.compare(hint.pos.name);
    if(cmp != 0) return cmp < 0;
    if(range.end != hint.range.end) return range.end < hint.range.end;
    return pos < hint.pos;
}
void gj::hint_base_t::add(const hint_t & _hint){
    hint_t * hint = new hint_t(_hint);
    insert(make_pair(hint->range.start, hint));
}

vector<hint_t*> gj::hint_base_t::resolve_position(pos_t pos) const{
    vector<hint_t*> res;
    for(const_iterator it = lower_bound(make_pair(pos,(hint_t*) NULL)); it != end(); ++it){
        if(it->second->range.start <= pos && pos <= it->second->range.end){
            res.push_back(it->second);
        }
    }
    return res;
}

vector<hint_t*> gj::hint_base_t::resolve_position(int line, int index) const{
    return resolve_position(pos_t(line, index));
}

gj::hint_base_t::~hint_base_t(){
    for(iterator it = set::begin(); it != set::end(); ++it){
        delete it->second;
    }
}

ostream& gj::hint_base_t::printTo (ostream& os) const{
    os << "hint base [\n";
    for(const_iterator it = begin(); it != end(); ++it){
        os << "   " << *it->second << ",\n";
    }
    os << "]";
    return os;
}

ostream& gj::global_hint_base_t::printTo (ostream & os) const{
    os << "global hint base [\n";
    for(const_iterator it = begin(); it != end(); ++it){
        os << it->first << " : " << it->second << ",\n";
    }
    os << "]\n";
    return os;
}

void gj::global_hint_base_t::add(const string & file, const hint_t & hint){
    iterator iter = find(file);
    if(iter == end()){
        insert(make_pair(file, hint_base_t()));
    }
    hint_base_t & base = find(file)->second;
    base.add(hint);
}
