#ifndef PDBS_VALIDATION_H
#define PDBS_VALIDATION_H

#include "types.h"

class TaskProxy;

namespace pdbs {
/*
  Validation and normalization of patterns and pattern collections is
  intended for user input. The given pattern or pattern collection has to be
  non-empty (otherwise exit with user input error). It will be sorted to
  detect and remove duplicates. If a pattern contains a variable out of the
  range of the task, a user input error is thrown.
*/
extern void validate_and_normalize_pattern(
    const TaskProxy &task_proxy, Pattern &pattern);
extern void validate_and_normalize_pattern_collection(
    const TaskProxy &task_proxy, PatternCollection &patterns);

/*
  A pattern collection is valid if:
  - it is non-empty,
  - all of its patterns are valid, i.e., they are non-empty and do not contain
    duplicate variables, and
  - it does not contain duplicate patterns.
*/
extern bool pattern_collection_is_valid(const PatternCollection &patterns);
}

#endif
