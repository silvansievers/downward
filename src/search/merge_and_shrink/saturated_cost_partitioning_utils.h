#ifndef MERGE_AND_SHRINK_SATURATED_COST_PARTITIONING_UTILS_H
#define MERGE_AND_SHRINK_SATURATED_COST_PARTITIONING_UTILS_H

#include <vector>

namespace utils {
class LogProxy;
}

namespace merge_and_shrink {
class Labels;
class TransitionSystem;

extern std::vector<int> compute_label_costs(const Labels &labels);
extern std::vector<int> compute_saturated_costs_for_transition_system(
    const TransitionSystem &transition_system,
    const std::vector<int> &goal_distances,
    int num_labels,
    utils::LogProxy &log);
extern void reduce_costs(std::vector<int> &label_costs, const std::vector<int> &saturated_label_costs);
}
#endif
