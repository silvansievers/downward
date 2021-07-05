#include "pattern_database_factory.h"

#include "abstract_operator.h"
#include "match_tree.h"
#include "pattern_database.h"

#include "../algorithms/priority_queues.h"
#include "../task_utils/task_properties.h"
#include "../utils/collections.h"
#include "../utils/logging.h"
#include "../utils/math.h"
#include "../utils/rng.h"
#include "../utils/timer.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using namespace std;

namespace pdbs {
Projection::Projection(
    const TaskProxy &task_proxy, const Pattern &pattern)
    : task_proxy(task_proxy), pattern(pattern) {
}

void Projection::compute_variable_to_index() const {
    variable_to_index.resize(task_proxy.get_variables().size(), -1);
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_to_index[pattern[i]] = i;
    }
}

void Projection::compute_abstract_goals() const {
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        int val = goal.get_value();
        if (variable_to_index[var_id] != -1) {
            abstract_goals.emplace_back(variable_to_index[var_id], val);
        }
    }
}

PerfectHashFunction compute_hash_function(
    const TaskProxy &task_proxy, const Pattern &pattern) {
    vector<int> hash_multipliers;
    hash_multipliers.reserve(pattern.size());
    int num_states = 1;
    for (int pattern_var_id : pattern) {
        hash_multipliers.push_back(num_states);
        VariableProxy var = task_proxy.get_variables()[pattern_var_id];
        if (utils::is_product_within_limit(num_states, var.get_domain_size(),
                                           numeric_limits<int>::max())) {
            num_states *= var.get_domain_size();
        } else {
            cerr << "Given pattern is too large! (Overflow occurred): " << endl;
            cerr << pattern << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
    return PerfectHashFunction(Pattern(pattern), num_states, move(hash_multipliers));
}

unique_ptr<MatchTree> build_match_tree(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<AbstractOperator> &abstract_operators) {
    unique_ptr<MatchTree> match_tree = utils::make_unique_ptr<MatchTree>(projection, hash_function);
    for (size_t op_id = 0; op_id < abstract_operators.size(); ++op_id) {
        const AbstractOperator &op = abstract_operators[op_id];
        match_tree->insert(op_id, op.get_preconditions());
    }
    return match_tree;
}

bool is_goal_state(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    int state_index) {
    for (const FactPair &abstract_goal : projection.get_abstract_goals()) {
        int pattern_var_id = abstract_goal.var;
        int var_id = projection.get_pattern()[pattern_var_id];
        VariableProxy var = projection.get_task_proxy().get_variables()[var_id];
        int val = hash_function.unrank(state_index, pattern_var_id, var.get_domain_size());
        if (val != abstract_goal.value) {
            return false;
        }
    }
    return true;
}

vector<int> compute_distances(
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<AbstractOperator> &abstract_operators,
    const MatchTree &match_tree,
    const unique_ptr<vector<int>> &generating_op_ids) {
    vector<int> distances;
    distances.reserve(hash_function.get_num_states());
    // first implicit entry: priority, second entry: index for an abstract state
    priority_queues::AdaptiveQueue<int> pq;

    // initialize queue
    for (int state_index = 0; state_index < hash_function.get_num_states(); ++state_index) {
        if (is_goal_state(projection, hash_function, state_index)) {
            pq.push(0, state_index);
            distances.push_back(0);
        } else {
            distances.push_back(numeric_limits<int>::max());
        }
    }

    if (generating_op_ids) {
        /*
          If computing a plan during Dijkstra, we store, for each state,
          an operator leading from that state to another state on a
          strongly optimal plan of the PDB. We store the first operator
          encountered during Dijkstra and only update it if the goal distance
          of the state was updated. Note that in the presence of zero-cost
          operators, this does not guarantee that we compute a strongly
          optimal plan because we do not minimize the number of used zero-cost
          operators.
         */
        generating_op_ids->resize(hash_function.get_num_states());
    }

    // Dijkstra loop
    while (!pq.empty()) {
        pair<int, int> node = pq.pop();
        int distance = node.first;
        int state_index = node.second;
        if (distance > distances[state_index]) {
            continue;
        }

        // regress abstract_state
        vector<int> applicable_operator_ids;
        match_tree.get_applicable_operator_ids(state_index, applicable_operator_ids);
        for (int op_id : applicable_operator_ids) {
            const AbstractOperator &op = abstract_operators[op_id];
            int predecessor = state_index + op.get_hash_effect();
            int alternative_cost = distances[state_index] + op.get_cost();
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
                if (generating_op_ids) {
                    generating_op_ids->at(predecessor) = op_id;
                }
            }
        }
    }
    return distances;
}

shared_ptr<PatternDatabase> generate_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    bool dump,
    const vector<int> &operator_costs) {
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);
    assert(operator_costs.empty() ||
           operator_costs.size() == task_proxy.get_operators().size());
    assert(utils::is_sorted_unique(pattern));

    utils::Timer timer;
    Projection projection(task_proxy, pattern);
    PerfectHashFunction hash_function = compute_hash_function(
        task_proxy, pattern);
    vector<AbstractOperator> abstract_operators = compute_abstract_operators(
        projection,
        hash_function,
        operator_costs);
    unique_ptr<MatchTree> match_tree = build_match_tree(
        projection,
        hash_function,
        abstract_operators);
    vector<int> distances = compute_distances(
        projection,
        hash_function,
        abstract_operators,
        *match_tree);

    if (dump)
        utils::g_log << "PDB construction time: " << timer << endl;

    return make_shared<PatternDatabase>(move(hash_function), move(distances));
}

static void compute_plan(
    const TaskProxy &task_proxy,
    const Projection &projection,
    const PerfectHashFunction &hash_function,
    const vector<AbstractOperator> &abstract_operators,
    const MatchTree &match_tree,
    const vector<int> &generating_op_ids,
    const vector<int> &distances,
    bool compute_wildcard_plan,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    vector<vector<OperatorID>> &wildcard_plan) {
    /*
      Using the generating operators computed during Dijkstra, we start
      from the initial state and follow the generating operator to the
      next state. Then we compute all operators of the same cost inducing
      the same abstract transition and randomly pick one of them to
      set for the next state. We iterate until reaching a goal state.
      Note that this kind of plan extraction does not uniformly at random
      consider all successor of a state but rather uses the arbitrarily
      chosen generating operator to settle on one successor state, which
      is biased by the number of operators leading to the same successor
      from the given state.
    */
    State initial_state = task_proxy.get_initial_state();
    initial_state.unpack();
    int current_state =
        hash_function.rank(initial_state.get_unpacked_values());
    if (distances[current_state] != numeric_limits<int>::max()) {
        while (!is_goal_state(projection, hash_function, current_state)) {
            int op_id = generating_op_ids[current_state];
            assert(op_id != -1);
            const AbstractOperator &op = abstract_operators[op_id];
            int successor_state = current_state - op.get_hash_effect();

            // Compute equivalent ops
            vector<OperatorID> cheapest_operators;
            vector<int> applicable_operator_ids;
            match_tree.get_applicable_operator_ids(successor_state, applicable_operator_ids);
            for (int applicable_op_id : applicable_operator_ids) {
                const AbstractOperator &applicable_op = abstract_operators[applicable_op_id];
                int predecessor = successor_state + applicable_op.get_hash_effect();
                if (predecessor == current_state && op.get_cost() == applicable_op.get_cost()) {
                    cheapest_operators.emplace_back(applicable_op.get_concrete_op_id());
                }
            }
            if (compute_wildcard_plan) {
                rng->shuffle(cheapest_operators);
                wildcard_plan.push_back(move(cheapest_operators));
            } else {
                OperatorID random_op_id = *rng->choose(cheapest_operators);
                wildcard_plan.emplace_back();
                wildcard_plan.back().push_back(random_op_id);
            }

            current_state = successor_state;
        }
    }
}

shared_ptr<PatternDatabase> generate_pdb_and_plan(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    vector<vector<OperatorID>> &wildcard_plan,
    bool compute_wildcard_plan,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool dump,
    const vector<int> &operator_costs) {
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);
    assert(operator_costs.empty() ||
           operator_costs.size() == task_proxy.get_operators().size());
    assert(utils::is_sorted_unique(pattern));

    utils::Timer timer;
    Projection projection(task_proxy, pattern);
    PerfectHashFunction hash_function = compute_hash_function(
        task_proxy, pattern);
    vector<AbstractOperator> abstract_operators = compute_abstract_operators(
        projection,
        hash_function,
        operator_costs);
    unique_ptr<MatchTree> match_tree = build_match_tree(
        projection,
        hash_function,
        abstract_operators);
    unique_ptr<vector<int>> generating_op_ids = utils::make_unique_ptr<vector<int>>();
    vector<int> distances = compute_distances(
        projection,
        hash_function,
        abstract_operators,
        *match_tree,
        generating_op_ids);
    compute_plan(
        task_proxy, projection, hash_function, abstract_operators, *match_tree,
        *generating_op_ids, distances, compute_wildcard_plan, rng,
        wildcard_plan);

    if (dump)
        utils::g_log << "PDB construction time: " << timer << endl;

    return make_shared<PatternDatabase>(move(hash_function), move(distances));
}
}
