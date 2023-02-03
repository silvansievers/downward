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
static Projection compute_projection(const TaskProxy &task_proxy, const Pattern &pattern) {
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
            cerr << "Given pattern is too large! (Overflow occured): " << endl;
            cerr << pattern << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
    return Projection(Pattern(pattern), num_states, move(hash_multipliers));
}

static vector<int> compute_variable_to_index(
    const TaskProxy task_proxy, const Pattern &pattern) {
    VariablesProxy variables = task_proxy.get_variables();
    vector<int> variable_to_index(variables.size(), -1);
    for (size_t i = 0; i < pattern.size(); ++i) {
        variable_to_index[pattern[i]] = i;
    }
    return variable_to_index;
}

/*
  Abstract operators are built from concrete operators. The
  parameters follow the usual name convention of SAS+ operators,
  meaning prevail, preconditions and effects are all related to
  progression search.
*/
static AbstractOperator build_abstract_operator(
    const Projection &projection,
    const vector<FactPair> &prev_pairs,
    const vector<FactPair> &pre_pairs,
    const vector<FactPair> &eff_pairs,
    int concrete_op_id,
    int cost) {
    vector<FactPair> regression_preconditions(prev_pairs);
    regression_preconditions.insert(regression_preconditions.end(),
                                    eff_pairs.begin(),
                                    eff_pairs.end());
    // Sort preconditions for MatchTree construction.
    sort(regression_preconditions.begin(), regression_preconditions.end());
    for (size_t i = 1; i < regression_preconditions.size(); ++i) {
        assert(regression_preconditions[i].var !=
               regression_preconditions[i - 1].var);
    }
    int hash_effect = 0;
    assert(pre_pairs.size() == eff_pairs.size());
    for (size_t i = 0; i < pre_pairs.size(); ++i) {
        int var = pre_pairs[i].var;
        assert(var == eff_pairs[i].var);
        int old_val = eff_pairs[i].value;
        int new_val = pre_pairs[i].value;
        assert(new_val != -1);
        int effect = (new_val - old_val) * projection.get_multiplier(var);
        hash_effect += effect;
    }
    return AbstractOperator(concrete_op_id, cost, move(regression_preconditions), hash_effect);
}

/*
  Recursive method; called by build_abstract_operators. In the case
  of a precondition with value = -1 in the concrete operator, all
  multiplied out abstract operators are computed, i.e. for all
  possible values of the variable (with precondition = -1), one
  abstract operator with a concrete value (!= -1) is computed.
*/
static void multiply_out(
    const Projection &projection,
    const VariablesProxy &variables,
    int concrete_op_id,
    int cost,
    int pos,
    vector<FactPair> &prev_pairs,
    vector<FactPair> &pre_pairs,
    vector<FactPair> &eff_pairs,
    const vector<FactPair> &effects_without_pre,
    vector<AbstractOperator> &operators) {
    if (pos == static_cast<int>(effects_without_pre.size())) {
        // All effects without precondition have been checked: insert op.
        if (!eff_pairs.empty()) {
            operators.emplace_back(
                build_abstract_operator(
                    projection, prev_pairs, pre_pairs, eff_pairs,
                    concrete_op_id, cost));
        }
    } else {
        // For each possible value for the current variable, build an
        // abstract operator.
        int var_id = effects_without_pre[pos].var;
        int eff = effects_without_pre[pos].value;
        VariableProxy var = variables[projection.get_pattern()[var_id]];
        for (int i = 0; i < var.get_domain_size(); ++i) {
            if (i != eff) {
                pre_pairs.emplace_back(var_id, i);
                eff_pairs.emplace_back(var_id, eff);
            } else {
                prev_pairs.emplace_back(var_id, i);
            }
            multiply_out(projection, variables, concrete_op_id, cost,
                         pos + 1, prev_pairs, pre_pairs, eff_pairs,
                         effects_without_pre, operators);
            if (i != eff) {
                pre_pairs.pop_back();
                eff_pairs.pop_back();
            } else {
                prev_pairs.pop_back();
            }
        }
    }
}

/*
  Computes all abstract operators for a given concrete operator.
  Initializes data structures for initial call to recursive method
  multiply_out.
*/
static void build_abstract_operators(
    const Projection &projection,
    const OperatorProxy &op,
    int cost,
    const vector<int> &variable_to_index,
    const VariablesProxy &variables,
    vector<AbstractOperator> &operators) {
    // All variable value pairs that are a prevail condition
    vector<FactPair> prev_pairs;
    // All variable value pairs that are a precondition (value != -1)
    vector<FactPair> pre_pairs;
    // All variable value pairs that are an effect
    vector<FactPair> eff_pairs;
    // All variable value pairs that are a precondition (value = -1)
    vector<FactPair> effects_without_pre;

    size_t num_vars = variables.size();
    vector<bool> has_precond_and_effect_on_var(num_vars, false);
    vector<bool> has_precondition_on_var(num_vars, false);

    for (FactProxy pre : op.get_preconditions())
        has_precondition_on_var[pre.get_variable().get_id()] = true;

    for (EffectProxy eff : op.get_effects()) {
        int var_id = eff.get_fact().get_variable().get_id();
        int pattern_var_id = variable_to_index[var_id];
        int val = eff.get_fact().get_value();
        if (pattern_var_id != -1) {
            if (has_precondition_on_var[var_id]) {
                has_precond_and_effect_on_var[var_id] = true;
                eff_pairs.emplace_back(pattern_var_id, val);
            } else {
                effects_without_pre.emplace_back(pattern_var_id, val);
            }
        }
    }
    for (FactProxy pre : op.get_preconditions()) {
        int var_id = pre.get_variable().get_id();
        int pattern_var_id = variable_to_index[var_id];
        int val = pre.get_value();
        if (pattern_var_id != -1) { // variable occurs in pattern
            if (has_precond_and_effect_on_var[var_id]) {
                pre_pairs.emplace_back(pattern_var_id, val);
            } else {
                prev_pairs.emplace_back(pattern_var_id, val);
            }
        }
    }
    multiply_out(projection, variables, op.get_id(), cost, 0,
                 prev_pairs, pre_pairs, eff_pairs, effects_without_pre,
                 operators);
}

static vector<AbstractOperator> compute_abstract_operators(
    const TaskProxy &task_proxy,
    const Projection &projection,
    const vector<int> &variable_to_index,
    const vector<int> &operator_costs) {
    vector<AbstractOperator> operators;
    for (OperatorProxy op : task_proxy.get_operators()) {
        int op_cost;
        if (operator_costs.empty()) {
            op_cost = op.get_cost();
        } else {
            op_cost = operator_costs[op.get_id()];
        }
        build_abstract_operators(
            projection, op, op_cost, variable_to_index,
            task_proxy.get_variables(), operators);
    }
    return operators;
}

static MatchTree compute_match_tree(
    const TaskProxy &task_proxy,
    const Projection &projection,
    const vector<AbstractOperator> &abstract_ops) {
    MatchTree match_tree(task_proxy, projection);
    for (size_t op_id = 0; op_id < abstract_ops.size(); ++op_id) {
        const AbstractOperator &op = abstract_ops[op_id];
        match_tree.insert(op_id, op.get_regression_preconditions());
    }
    return match_tree;
}

static vector<FactPair> compute_abstract_goals(
    const TaskProxy &task_proxy,
    const vector<int> &variable_to_index) {
    vector<FactPair> abstract_goals;
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        int val = goal.get_value();
        if (variable_to_index[var_id] != -1) {
            abstract_goals.emplace_back(variable_to_index[var_id], val);
        }
    }
    return abstract_goals;
}

/*
  For a given abstract state (given as index), the according values
  for each variable in the state are computed and compared with the
  given pairs of goal variables and values. Returns true iff the
  state is a goal state.
*/
static bool is_goal_state(
    const Projection &projection,
    int state_index,
    const vector<FactPair> &abstract_goals,
    const VariablesProxy &variables) {
    for (const FactPair &abstract_goal : abstract_goals) {
        int pattern_var_id = abstract_goal.var;
        int var_id = projection.get_pattern()[pattern_var_id];
        VariableProxy var = variables[var_id];
        int val = projection.unrank(state_index, pattern_var_id, var.get_domain_size());
        if (val != abstract_goal.value) {
            return false;
        }
    }
    return true;
}

static vector<int> compute_distances(
    const VariablesProxy &variables,
    const Projection &projection,
    const vector<AbstractOperator> &abstract_ops,
    const MatchTree &match_tree,
    const vector<FactPair> &abstract_goals,
    bool compute_plan,
    vector<int> &generating_op_ids) {
    vector<int> distances;
    distances.reserve(projection.get_num_abstract_states());
    // first implicit entry: priority, second entry: index for an abstract state
    priority_queues::AdaptiveQueue<int> pq;

    // initialize queue
    for (int state_index = 0; state_index < projection.get_num_abstract_states(); ++state_index) {
        if (is_goal_state(projection, state_index, abstract_goals, variables)) {
            pq.push(0, state_index);
            distances.push_back(0);
        } else {
            distances.push_back(numeric_limits<int>::max());
        }
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
            const AbstractOperator &op = abstract_ops[op_id];
            int predecessor = state_index + op.get_hash_effect();
            int alternative_cost = distances[state_index] + op.get_cost();
            if (alternative_cost < distances[predecessor]) {
                distances[predecessor] = alternative_cost;
                pq.push(alternative_cost, predecessor);
                if (compute_plan) {
                    generating_op_ids[predecessor] = op_id;
                }
            }
        }
    }
    return distances;
}

static shared_ptr<PatternDatabase> generate_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const vector<int> &operator_costs,
    bool compute_plan,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan,
    vector<vector<OperatorID>> *wildcard_plan = nullptr) {
    task_properties::verify_no_axioms(task_proxy);
    task_properties::verify_no_conditional_effects(task_proxy);
    assert(utils::is_sorted_unique(pattern));
    assert(operator_costs.empty() ||
           operator_costs.size() == task_proxy.get_operators().size());

    Projection projection = compute_projection(task_proxy, pattern);
    vector<int> variable_to_index = compute_variable_to_index(
        task_proxy, projection.get_pattern());
    vector<AbstractOperator> abstract_ops = compute_abstract_operators(
        task_proxy, projection, variable_to_index, operator_costs);
    MatchTree match_tree = compute_match_tree(task_proxy, projection, abstract_ops);
    vector<FactPair> abstract_goals = compute_abstract_goals(task_proxy, variable_to_index);

    vector<int> generating_op_ids;
    if (compute_plan) {
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
        generating_op_ids.resize(projection.get_num_abstract_states());
    }
    vector<int> distances = compute_distances(
        task_proxy.get_variables(),
        projection,
        abstract_ops,
        match_tree,
        abstract_goals,
        compute_plan,
        generating_op_ids);

    // Compute abstract plan
    if (compute_plan) {
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
            projection.rank(initial_state.get_unpacked_values());
        if (distances[current_state] != numeric_limits<int>::max()) {
            while (!is_goal_state(projection, current_state, abstract_goals, task_proxy.get_variables())) {
                int op_id = generating_op_ids[current_state];
                assert(op_id != -1);
                const AbstractOperator &op = abstract_ops[op_id];
                int successor_state = current_state - op.get_hash_effect();

                // Compute equivalent ops
                vector<OperatorID> cheapest_operators;
                vector<int> applicable_operator_ids;
                match_tree.get_applicable_operator_ids(successor_state, applicable_operator_ids);
                for (int applicable_op_id : applicable_operator_ids) {
                    const AbstractOperator &applicable_op = abstract_ops[applicable_op_id];
                    int predecessor = successor_state + applicable_op.get_hash_effect();
                    if (predecessor == current_state && op.get_cost() == applicable_op.get_cost()) {
                        cheapest_operators.emplace_back(applicable_op.get_concrete_op_id());
                    }
                }
                if (compute_wildcard_plan) {
                    rng->shuffle(cheapest_operators);
                    wildcard_plan->push_back(move(cheapest_operators));
                } else {
                    OperatorID random_op_id = *rng->choose(cheapest_operators);
                    wildcard_plan->emplace_back();
                    wildcard_plan->back().push_back(random_op_id);
                }

                current_state = successor_state;
            }
        }
        utils::release_vector_memory(generating_op_ids);
    }
    return make_shared<PatternDatabase>(move(projection), move(distances));
}

shared_ptr<PatternDatabase> compute_pdb(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const vector<int> &operator_costs,
    const shared_ptr<utils::RandomNumberGenerator> &rng) {
    return generate_pdb(
        task_proxy, pattern, operator_costs, false, rng, false);
}

tuple<shared_ptr<PatternDatabase>, vector<vector<OperatorID>>>
compute_pdb_and_wildcard_plan(
    const TaskProxy &task_proxy,
    const Pattern &pattern,
    const vector<int> &operator_costs,
    const shared_ptr<utils::RandomNumberGenerator> &rng,
    bool compute_wildcard_plan) {
    vector<vector<OperatorID>> wildcard_plan;
    shared_ptr<PatternDatabase> pdb = generate_pdb(
        task_proxy, pattern, operator_costs, true, rng, compute_wildcard_plan, &wildcard_plan);
    return {
        pdb, wildcard_plan
    };
}
}
