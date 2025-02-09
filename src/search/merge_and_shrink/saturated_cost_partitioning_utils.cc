#include "saturated_cost_partitioning_utils.h"

#include "distances.h"
#include "labels.h"
#include "transition_system.h"

#include "../utils/logging.h"

using namespace std;

namespace merge_and_shrink {
vector<int> compute_label_costs(
    const Labels &labels) {
    int num_labels = labels.get_num_total_labels();
    vector<int> label_costs(num_labels, -1);
    for (int label_no : labels) {
        label_costs[label_no] = labels.get_label_cost(label_no);
    }
    return label_costs;
}

vector<int> compute_saturated_costs_for_transition_system(
    const TransitionSystem &transition_system,
    const vector<int> &goal_distances,
    int num_labels,
    utils::LogProxy &log) {
    static bool dump_if_empty_transitions = true;
    static bool dump_if_infinite_transitions = true;
    vector<int> saturated_label_costs(num_labels, -1);
    for (const LocalLabelInfo &local_label_info : transition_system) {
        const LabelGroup &label_group = local_label_info.get_label_group();
        const vector<Transition> &transitions = local_label_info.get_transitions();
        int group_saturated_cost = -INF;
        if (log.is_at_least_verbose() && dump_if_empty_transitions && transitions.empty()) {
            dump_if_empty_transitions = false;
            log << "found dead label group" << endl;
        } else {
            for (const Transition &transition : transitions) {
                int src = transition.src;
                int target = transition.target;
                int h_src = goal_distances[src];
                int h_target = goal_distances[target];
                int diff;
                if (h_src == -INF || h_target == INF) {
                    diff = -INF;
                } else if (h_src == INF || h_target == -INF) {
                    diff = INF;
                } else {
                    assert(h_src != INF && h_src != -INF &&
                        h_target != INF && h_target != -INF);
                    diff = h_src - h_target;
                }
                group_saturated_cost = max(group_saturated_cost, diff);
            }
            if (log.is_at_least_verbose()
                && dump_if_infinite_transitions
                && group_saturated_cost == -INF) {
                dump_if_infinite_transitions = false;
                log << "label group does not lead to any state with finite heuristic value" << endl;
            }
        }
        for (int label_no : label_group) {
            saturated_label_costs[label_no] = group_saturated_cost;
        }
    }
//    log << "num original labels in abs: " << mapped_labels.size() << endl;
//    assert(static_cast<int>(mapped_labels.size()) == num_original_labels);
//    log << "original labels from abs: "
//         << vector<int>(mapped_labels.begin(), mapped_labels.end()) << endl;
//    assert(original_labels == vector<int>(mapped_labels.begin(), mapped_labels.end()));
    if (log.is_at_least_debug()) {
        log << "Saturated label costs: " << saturated_label_costs << endl;
    }
    return saturated_label_costs;
}

void reduce_costs(vector<int> &label_costs, const vector<int> &saturated_label_costs) {
    for (size_t label_no = 0; label_no < label_costs.size(); ++label_no) {
        int remaining_cost = label_costs[label_no];
        int saturated_cost = saturated_label_costs[label_no];
        if (remaining_cost == -1) {
            /*
              Skip reduced labels -- they have cost -1 "from the first
              abstraction on" and therefore always keep cost of -1, under the
              assumption that (remaining) cost functions are always positive.
            */
            assert(saturated_cost == -1);
        } else {
            assert(remaining_cost >= 0);
            assert(saturated_cost <= remaining_cost);
            if (remaining_cost == INF) {
                /*
                  Skip labels with infinite cost -- according to left addition,
                  these values remain infinite. See journal paper. Note that
                  this case covers saturated_cost == INF because that implies
                  remaining_cost == INF.
                */
            } else if (saturated_cost == -INF) {
                label_costs[label_no] = INF;
            } else {
                label_costs[label_no] = remaining_cost - saturated_cost;
            }
            assert(label_costs[label_no] >= 0);
        }
    }
}
}
