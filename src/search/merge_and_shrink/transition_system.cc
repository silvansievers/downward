#include "transition_system.h"

#include "distances.h"
#include "global_labels.h"

#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

using namespace std;
using utils::ExitCode;

namespace merge_and_shrink {
ostream &operator<<(ostream &os, const Transition &trans) {
    os << trans.src << "->" << trans.target;
    return os;
}

// Sorts the given set of transitions and removes duplicates.
static void normalize_given_transitions(vector<Transition> &transitions) {
    sort(transitions.begin(), transitions.end());
    transitions.erase(unique(transitions.begin(), transitions.end()), transitions.end());
}

void LocalLabelInfo::add_label(int label, int label_cost) {
    label_group.push_back(label);
    cost = min(cost, label_cost);
}

void LocalLabelInfo::merge_local_label_info(LocalLabelInfo &local_label_info) {
    assert(transitions == local_label_info.transitions);
    label_group.insert(
        label_group.end(),
        make_move_iterator(local_label_info.label_group.begin()),
        make_move_iterator(local_label_info.label_group.end()));
    cost = min(cost, local_label_info.cost);
    local_label_info.clear();
}

void LocalLabelInfo::clear() {
    utils::release_vector_memory(transitions);
    utils::release_vector_memory(label_group);
    cost = -1;
}


/*
  Implementation note: Transitions are grouped by their (local) labels,
  not by source state or any such thing. Such a grouping is beneficial
  for fast generation of products because we can iterate label by label
  (and the global labels they represent), and it also allows applying
  transition system mappings very efficiently.

  We rarely need to be able to efficiently query the successors of a
  given state; actually, only the distance computation requires that,
  and it simply generates such a graph representation of the
  transitions itself. Various experiments have shown that maintaining
  a graph representation permanently for the benefit of distance
  computation is not worth the overhead.
*/

TransitionSystem::TransitionSystem(
    int num_variables,
    vector<int> &&incorporated_variables,
    const GlobalLabels &global_labels,
    vector<int> &&global_label_to_local_label,
    vector<LocalLabelInfo> &&local_label_infos,
    int num_states,
    vector<bool> &&goal_states,
    int init_state)
    : num_variables(num_variables),
      incorporated_variables(move(incorporated_variables)),
      global_labels(move(global_labels)),
      global_label_to_local_label(move(global_label_to_local_label)),
      local_label_infos(move(local_label_infos)),
      num_states(num_states),
      goal_states(move(goal_states)),
      init_state(init_state) {
    assert(is_valid());
}

TransitionSystem::TransitionSystem(const TransitionSystem &other)
    : num_variables(other.num_variables),
      incorporated_variables(other.incorporated_variables),
      global_labels(other.global_labels),
      global_label_to_local_label(other.global_label_to_local_label),
      local_label_infos(other.local_label_infos),
      num_states(other.num_states),
      goal_states(other.goal_states),
      init_state(other.init_state) {
}

TransitionSystem::~TransitionSystem() {
}

unique_ptr<TransitionSystem> TransitionSystem::merge(
    const GlobalLabels &global_labels,
    const TransitionSystem &ts1,
    const TransitionSystem &ts2,
    utils::LogProxy &log) {
    if (log.is_at_least_verbose()) {
        log << "Merging " << ts1.get_description() << " and "
            << ts2.get_description() << endl;
    }

    assert(ts1.init_state != PRUNED_STATE && ts2.init_state != PRUNED_STATE);
    assert(ts1.is_valid() && ts2.is_valid());

    int num_variables = ts1.num_variables;
    vector<int> incorporated_variables;
    ::set_union(
        ts1.incorporated_variables.begin(), ts1.incorporated_variables.end(),
        ts2.incorporated_variables.begin(), ts2.incorporated_variables.end(),
        back_inserter(incorporated_variables));
    vector<int> global_label_to_local_label(global_labels.get_max_num_labels(), -1);
    vector<LocalLabelInfo> local_label_infos;
    local_label_infos.reserve(global_labels.get_max_num_labels());

    int ts1_size = ts1.get_size();
    int ts2_size = ts2.get_size();
    int num_states = ts1_size * ts2_size;
    vector<bool> goal_states(num_states, false);
    int init_state = -1;

    for (int s1 = 0; s1 < ts1_size; ++s1) {
        for (int s2 = 0; s2 < ts2_size; ++s2) {
            int state = s1 * ts2_size + s2;
            if (ts1.goal_states[s1] && ts2.goal_states[s2])
                goal_states[state] = true;
            if (s1 == ts1.init_state && s2 == ts2.init_state)
                init_state = state;
        }
    }
    assert(init_state != -1);

    /*
      We can compute the local equivalence relation of a composite T
      from the local equivalence relations of the two components T1 and T2:
      l and l' are locally equivalent in T iff:
      (A) they are locally equivalent in T1 and in T2, or
      (B) they are both dead in T (e.g., this includes the case where
          l is dead in T1 only and l' is dead in T2 only, so they are not
          locally equivalent in either of the components).
    */
    int multiplier = ts2_size;
    LabelGroup dead_labels;
    for (const LocalLabelInfo &local_label_info : ts1) {
        const LabelGroup &group1 = local_label_info.get_label_group();
        const vector<Transition> &transitions1 = local_label_info.get_transitions();

        // Distribute the labels of this group among the "buckets"
        // corresponding to the groups of ts2.
        unordered_map<int, LabelGroup> buckets;
        for (int label : group1) {
            int ts_local_label2 = ts2.global_label_to_local_label[label];
            buckets[ts_local_label2].push_back(label);
        }
        // Now buckets contains all equivalence classes that are
        // refinements of group1.

        // Now create the new groups together with their transitions.
        for (auto &bucket : buckets) {
            const vector<Transition> &transitions2 =
                ts2.local_label_infos[bucket.first].get_transitions();

            // Create the new transitions for this bucket
            vector<Transition> new_transitions;
            if (!transitions1.empty() && !transitions2.empty()
                && transitions1.size() > new_transitions.max_size() / transitions2.size())
                utils::exit_with(ExitCode::SEARCH_OUT_OF_MEMORY);
            new_transitions.reserve(transitions1.size() * transitions2.size());
            for (const Transition &transition1 : transitions1) {
                int src1 = transition1.src;
                int target1 = transition1.target;
                for (const Transition &transition2 : transitions2) {
                    int src2 = transition2.src;
                    int target2 = transition2.target;
                    int src = src1 * multiplier + src2;
                    int target = target1 * multiplier + target2;
                    new_transitions.emplace_back(src, target);
                }
            }

            // Create a new group if the transitions are not empty
            LabelGroup &new_labels = bucket.second;
            if (new_transitions.empty()) {
                dead_labels.insert(dead_labels.end(), new_labels.begin(), new_labels.end());
            } else {
                sort(new_transitions.begin(), new_transitions.end());
                int new_local_label = local_label_infos.size();
                int cost = INF;
                for (int label : new_labels) {
                    cost = min(ts1.global_labels.get_label_cost(label), cost);
                    global_label_to_local_label[label] = new_local_label;
                }
                local_label_infos.emplace_back(move(new_labels), move(new_transitions), cost);
            }
        }
    }

    /*
      We collect all dead labels separately, because the bucket refining
      does not work in cases where there are at least two dead labels l1
      and l2 in the composite, where l1 was only a dead label in the first
      component and l2 was only a dead label in the second component.
      All dead labels should form one single label group.
    */
    if (!dead_labels.empty()) {
        int new_local_label = local_label_infos.size();
        int cost = INF;
        for (int label : dead_labels) {
            cost = min(cost, ts1.global_labels.get_label_cost(label));
            global_label_to_local_label[label] = new_local_label;
        }
        // Dead labels have empty transitions
        local_label_infos.emplace_back(move(dead_labels), vector<Transition>(), cost);
    }

    return utils::make_unique_ptr<TransitionSystem>(
        num_variables,
        move(incorporated_variables),
        ts1.global_labels,
        move(global_label_to_local_label),
        move(local_label_infos),
        num_states,
        move(goal_states),
        init_state
        );
}

void TransitionSystem::compute_locally_equivalent_labels() {
    /*
      Compare every group of labels and their transitions to all others and
      merge two groups whenever the transitions are the same.

      Note that there can be empty local label groups after applying label
      reduction when combining labels which are combinable for this transition
      system.
    */
    int num_local_labels = local_label_infos.size();
    for (int local_label1 = 0; local_label1 < num_local_labels;
         ++local_label1) {
        if (!local_label_infos[local_label1].empty()) {
            const vector<Transition> &transitions1 = local_label_infos[local_label1].get_transitions();
            for (int local_label2 = local_label1 + 1;
                 local_label2 < num_local_labels; ++local_label2) {
                if (!local_label_infos[local_label2].empty()) {
                    const vector<Transition> &transitions2 = local_label_infos[local_label2].get_transitions();
                    // Comparing transitions directly works because they are sorted and unique.
                    if (transitions1 == transitions2) {
                        for (int global_label : local_label_infos[local_label2].get_label_group()) {
                            global_label_to_local_label[global_label] = local_label1;
                        }
                        local_label_infos[local_label1].merge_local_label_info(
                            local_label_infos[local_label2]);
                    }
                }
            }
        }
    }

    assert(is_valid());
}

void TransitionSystem::apply_abstraction(
    const StateEquivalenceRelation &state_equivalence_relation,
    const vector<int> &abstraction_mapping,
    utils::LogProxy &log) {
    assert(is_valid());

    int new_num_states = state_equivalence_relation.size();
    assert(new_num_states < num_states);
    if (log.is_at_least_verbose()) {
        log << tag() << "applying abstraction (" << get_size()
            << " to " << new_num_states << " states)" << endl;
    }

    vector<bool> new_goal_states(new_num_states, false);
    for (int new_state = 0; new_state < new_num_states; ++new_state) {
        const StateEquivalenceClass &state_equivalence_class =
            state_equivalence_relation[new_state];
        assert(!state_equivalence_class.empty());

        for (int old_state : state_equivalence_class) {
            if (goal_states[old_state]) {
                new_goal_states[new_state] = true;
                break;
            }
        }
    }
    goal_states = move(new_goal_states);

    // Update all transitions.
    for (LocalLabelInfo &local_label_info : local_label_infos) {
        vector<Transition> &transitions = local_label_info.transitions;
        if (!transitions.empty()) {
            vector<Transition> new_transitions;
            /*
            We reserve more memory than necessary here, but this is better
            than potentially resizing the vector several times when inserting
            transitions one after the other. See issue604-v6.

            An alternative could be to not use a new vector, but to modify
            the existing transitions inplace, and to remove all empty
            positions in the end. This would be more ugly, though.
            */
            new_transitions.reserve(transitions.size());
            for (size_t i = 0; i < transitions.size(); ++i) {
                const Transition &transition = transitions[i];
                int src = abstraction_mapping[transition.src];
                int target = abstraction_mapping[transition.target];
                if (src != PRUNED_STATE && target != PRUNED_STATE)
                    new_transitions.emplace_back(src, target);
            }
            normalize_given_transitions(new_transitions);
            transitions = move(new_transitions);
        }
    }

    compute_locally_equivalent_labels();

    num_states = new_num_states;
    init_state = abstraction_mapping[init_state];
    if (log.is_at_least_verbose() && init_state == PRUNED_STATE) {
        log << tag() << "initial state pruned; task unsolvable" << endl;
    }

    assert(is_valid());
}

void TransitionSystem::apply_label_reduction(
    const vector<pair<int, vector<int>>> &label_mapping,
    bool only_equivalent_labels) {
    /*
      We iterate over the given label mapping, treating every new label and
      the reduced old labels separately. We further distinguish the case
      where we know that the reduced labels are all from the same equivalence
      class from the case where we may combine arbitrary labels.

      The case where only equivalent labels are combined is simple: remove all
      old global labels from the local label they belong to (they all belong
      to the same local label because we know they are equivalent) and add the
      new global label to the local label instead. Thus the affected local
      label is "non-empty" in the sense that there are still global labels
      associated to it.

      The other case is more involved: again remove all old labels from their
      local labels, and the local labels themselves if they do not represent
      any global labels anymore. Also collect the transitions of all reduced
      labels. Add a new local label for every new global label and assign the
      collected transitions to it. Recompute the cost of all affected local
      labels and re-compute locally equivalent labels.
    */

    if (only_equivalent_labels) {
        // Update both label mappings.
        for (const pair<int, vector<int>> &mapping : label_mapping) {
            int new_label = mapping.first;
            const vector<int> &old_labels = mapping.second;
            assert(old_labels.size() >= 2);
            int local_label = global_label_to_local_label[old_labels.front()];
            LabelGroup &label_group = local_label_infos[local_label].label_group;
            label_group.push_back(new_label);
            global_label_to_local_label[new_label] = local_label;

            for (int old_label : old_labels) {
                assert(global_label_to_local_label[old_label] == local_label);
                label_group.erase(find(label_group.begin(), label_group.end(), old_label));
                // Reset (for consistency only, old labels are never accessed).
                global_label_to_local_label[old_label] = -1;
                // NOTE: if we were combining labels with different cost,
                // we would need to recompute the cost of the local label here.
            }
        }
    } else {
        /*
          Go over all label reductions (given by label_mapping). For each
          reduction, go over the reduced labels to combine their transitions
          into the transitions of the new label. Also remember the affected
          local labels. Furthermore, remove the reduced labels from their
          local labels. Add the new label together with its transitions as
          a new local label.
        */
        unordered_set<int> affected_local_labels;
        for (const pair<int, vector<int>> &mapping: label_mapping) {
            const vector<int> &old_labels = mapping.second;
            assert(old_labels.size() >= 2);
            unordered_set<int> seen_local_labels;
            // TODO: consider using vector and sort and make unique once at the end.
            set<Transition> new_label_transitions;
            for (int old_label : old_labels) {
                int old_local_label = global_label_to_local_label[old_label];
                if (seen_local_labels.insert(old_local_label).second) {
                    affected_local_labels.insert(old_local_label);
                    const vector<Transition> &transitions = local_label_infos[old_local_label].get_transitions();
                    new_label_transitions.insert(transitions.begin(), transitions.end());
                }

                LabelGroup &label_group = local_label_infos[old_local_label].label_group;
                label_group.erase(find(label_group.begin(), label_group.end(), old_label));
                // Reset (for consistency only, old labels are never accessed).
                global_label_to_local_label[old_label] = -1;
            }
            vector<Transition> new_transitions(
                new_label_transitions.begin(), new_label_transitions.end());

            int new_label = mapping.first;
            int new_local_label = local_label_infos.size();
            global_label_to_local_label[new_label] = new_local_label;
            int new_cost = global_labels.get_label_cost(new_label);

            LabelGroup new_label_group = {new_label};
            local_label_infos.emplace_back(move(new_label_group), move(new_transitions), new_cost);
        }

        // Update all affected local labels.
        for (int local_label : affected_local_labels) {
            LocalLabelInfo &local_label_info = local_label_infos[local_label];
            // If the local label does not represent any global label anymore,
            // invalidate the entry.
            if (local_label_infos[local_label].empty()) {
                local_label_infos[local_label].clear();
            }

            // Otherwise, recompute its cost.
            local_label_infos[local_label].cost = INF;
            for (int label : local_label_info.label_group) {
                int cost = global_labels.get_label_cost(label);
                local_label_infos[local_label].cost = min(
                    local_label_infos[local_label].cost, cost);
            }
        }

        compute_locally_equivalent_labels();
    }

    assert(is_valid());
}

string TransitionSystem::tag() const {
    string desc(get_description());
    desc[0] = toupper(desc[0]);
    return desc + ": ";
}

bool TransitionSystem::are_transitions_sorted_unique() const {
    for (const LocalLabelInfo &local_label_info : *this) {
        if (!utils::is_sorted_unique(local_label_info.get_transitions()))
            return false;
    }
    return true;
}

bool TransitionSystem::is_valid() const {
    return are_transitions_sorted_unique()
           && is_label_mapping_consistent();
}

bool TransitionSystem::is_label_mapping_consistent() const {
    for (int global_label : global_labels) {
        int local_label = global_label_to_local_label[global_label];
        const LabelGroup &label_group = local_label_infos[local_label].get_label_group();
        assert(!label_group.empty());

        if (find(label_group.begin(),
                 label_group.end(),
                 global_label)
            == label_group.end()) {
            dump_label_mapping();
            cerr << "global label " << global_label << " is not part of the "
                "local label it is mapped to" << endl;
            return false;
        }
    }

    for (size_t local_label = 0; local_label < local_label_infos.size(); ++local_label) {
        const LocalLabelInfo &local_label_info = local_label_infos[local_label];
        for (int global_label : local_label_info.label_group) {
            if (global_label_to_local_label[global_label] != static_cast<int>(local_label)) {
                dump_label_mapping();
                cerr << "global label " << global_label << " is not mapped "
                    "to the local label it is part of" << endl;
                return false;
            }
        }
    }
    return true;
}

void TransitionSystem::dump_label_mapping() const {
    utils::g_log << "global to local label mapping: ";
    for (int global_label : global_labels) {
        utils::g_log << global_label << " -> "
                     << global_label_to_local_label[global_label] << ", ";
    }
    utils::g_log << endl;
    utils::g_log << "local to global label mapping: ";
    for (size_t local_label = 0;
         local_label < local_label_infos.size(); ++local_label) {
        utils::g_log << local_label << ": "
                     << local_label_infos[local_label].label_group << ", ";
    }
    utils::g_log << endl;
}

bool TransitionSystem::is_solvable(const Distances &distances) const {
    if (init_state == PRUNED_STATE) {
        return false;
    }
    if (distances.are_goal_distances_computed() && distances.get_goal_distance(init_state) == INF) {
        return false;
    }
    return true;
}

int TransitionSystem::compute_total_transitions() const {
    int total = 0;
    for (const LocalLabelInfo &local_label_info : *this) {
        total += local_label_info.get_transitions().size();
    }
    return total;
}

string TransitionSystem::get_description() const {
    ostringstream s;
    if (incorporated_variables.size() == 1) {
        s << "atomic transition system #" << *incorporated_variables.begin();
    } else {
        s << "composite transition system with "
          << incorporated_variables.size() << "/" << num_variables << " vars";
    }
    return s.str();
}

void TransitionSystem::dump_dot_graph(utils::LogProxy &log) const {
    assert(is_valid());
    if (log.is_at_least_debug()) {
        log << "digraph transition_system";
        for (size_t i = 0; i < incorporated_variables.size(); ++i)
            log << "_" << incorporated_variables[i];
        log << " {" << endl;
        log << "    node [shape = none] start;" << endl;
        for (int i = 0; i < num_states; ++i) {
            bool is_init = (i == init_state);
            bool is_goal = goal_states[i];
            log << "    node [shape = " << (is_goal ? "doublecircle" : "circle")
                << "] node" << i << ";" << endl;
            if (is_init)
                log << "    start -> node" << i << ";" << endl;
        }
        for (const LocalLabelInfo &local_label_info : *this) {
            const LabelGroup &label_group = local_label_info.get_label_group();
            const vector<Transition> &transitions = local_label_info.get_transitions();
            for (const Transition &transition : transitions) {
                int src = transition.src;
                int target = transition.target;
                log << "    node" << src << " -> node" << target << " [label = ";
                for (size_t i = 0; i < label_group.size(); ++i) {
                    if (i != 0)
                        log << "_";
                    log << "x" << label_group[i];
                }
                log << "];" << endl;
            }
        }
        log << "}" << endl;
    }
}

void TransitionSystem::dump_labels_and_transitions(utils::LogProxy &log) const {
    if (log.is_at_least_debug()) {
        log << tag() << "transitions" << endl;
        for (const LocalLabelInfo &local_label_info : *this) {
            const LabelGroup &label_group = local_label_info.get_label_group();
            log << "labels: " << label_group << endl;
            log << "transitions: ";
            const vector<Transition> &transitions = local_label_info.get_transitions();
            for (size_t i = 0; i < transitions.size(); ++i) {
                int src = transitions[i].src;
                int target = transitions[i].target;
                if (i != 0)
                    log << ",";
                log << src << " -> " << target;
            }
            utils::g_log << "cost: " << local_label_info.cost << endl;
        }
    }
}

void TransitionSystem::statistics(utils::LogProxy &log) const {
    if (log.is_at_least_verbose()) {
        log << tag() << get_size() << " states, "
            << compute_total_transitions() << " arcs " << endl;
    }
}
}
