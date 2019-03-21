#include "pattern_information.h"

#include "pattern_database.h"
#include "validation.h"

#include <cassert>

using namespace std;

namespace pdbs {
PatternInformation::PatternInformation(
    const TaskProxy &task_proxy,
    Pattern pattern)
    : pattern(move(pattern)),
      pdb(nullptr) {
    validate_and_normalize_pattern(task_proxy, this->pattern);
}

void PatternInformation::create_pdb_if_missing(const TaskProxy &task_proxy) {
    if (!pdb) {
        pdb = make_shared<PatternDatabase>(task_proxy, pattern);
    }
}

bool PatternInformation::information_is_valid() const {
    return !pdb || pdb->get_pattern() == pattern;
}

void PatternInformation::set_pdb(const shared_ptr<PatternDatabase> &pdb_) {
    pdb = pdb_;
    assert(information_is_valid());
}

const Pattern &PatternInformation::get_pattern() const {
    return pattern;
}

shared_ptr<PatternDatabase> PatternInformation::get_pdb(
    const TaskProxy &task_proxy) {
    create_pdb_if_missing(task_proxy);
    return pdb;
}
}
