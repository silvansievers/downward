#include "validation.h"

#include "pattern_collection_information.h"

#include "../task_proxy.h"

#include "../utils/system.h"

#include <algorithm>
#include <iostream>

using namespace std;
using utils::ExitCode;

namespace pdbs {
void validate_and_normalize_pattern(
    const TaskProxy &task_proxy, Pattern &pattern) {
    /*
      - Sort by variable number and remove duplicate variables.
      - Warn if duplicate variables exist and remove them.
      - Error if the pattern contains out-of-range variable numbers.
    */
    sort(pattern.begin(), pattern.end());
    auto it = unique(pattern.begin(), pattern.end());
    if (it != pattern.end()) {
        pattern.erase(it, pattern.end());
        cout << "Warning: duplicate variables in pattern have been removed"
             << endl;
    }
    if (pattern.front() < 0) {
        cerr << "Variable number too low in pattern" << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    int num_variables = task_proxy.get_variables().size();
    if (pattern.back() >= num_variables) {
        cerr << "Variable number too high in pattern" << endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

void validate_and_normalize_pattern_collection(
    const TaskProxy &task_proxy, PatternCollectionInformation &pci) {
    /*
      TODO: this breaks validity of pattern collection information if pdbs
      have been set.
    */
    PatternCollection &patterns = pci.get_patterns();
    /*
      - Remove empty patterns.
      - Validate and normalize each pattern (see there).
      - Warn if duplicate patterns exist and remove them.
    */
    if (patterns.empty()) {
        cerr << "Pattern collection is empty" <<endl;
        utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
    }
    PatternCollection::iterator it = patterns.begin();
    while (it != patterns.end()) {
        Pattern &pattern = *it;
        if (pattern.empty()) {
            it = patterns.erase(it);
        } else {
            validate_and_normalize_pattern(task_proxy, pattern);
            ++it;
        }
    }
    sort(patterns.begin(), patterns.end());
    it = unique(patterns.begin(), patterns.end());
    if (it != patterns.end()) {
        patterns.erase(it, patterns.end());
        cout << "Warning: duplicate patterns have been removed" << endl;
    }
}

bool pattern_is_valid(const Pattern &pattern) {
    if (pattern.empty()) {
        return false;
    }
    Pattern sorted_pattern(pattern);
    sort(sorted_pattern.begin(), sorted_pattern.end());
    auto it = unique(sorted_pattern.begin(), sorted_pattern.end());
    if (it != sorted_pattern.end()) {
        return false;
    }
    return true;
}

bool pattern_collection_is_valid(const PatternCollection &patterns) {
    if (patterns.empty()) {
        return false;
    }
    PatternCollection patterns_copy(patterns);
    for (Pattern &pattern : patterns_copy) {
        if (!pattern_is_valid(pattern)) {
            return false;
        }
        sort(pattern.begin(), pattern.end());
    }
    sort(patterns_copy.begin(), patterns_copy.end());
    auto it = unique(patterns_copy.begin(), patterns_copy.end());
    if (it != patterns_copy.end()) {
        return false;
    }
    return true;
}
}
