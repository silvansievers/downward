#include "merge_scoring_function_cp.h"

#include "distances.h"
#include "factored_transition_system.h"
#include "labels.h"
#include "merge_and_shrink_algorithm.h"
#include "merge_and_shrink_representation.h"
#include "merge_scoring_function_miasm_utils.h"
#include "saturated_cost_partitioning_utils.h"
#include "shrink_strategy.h"
#include "transition_system.h"

#include "../task_proxy.h"

#include "../plugins/options.h"
#include "../plugins/plugin.h"

#include "../tasks/root_task.h"

#include "../utils/logging.h"
#include "../utils/markup.h"

using namespace std;

namespace merge_and_shrink {
MergeScoringFunctionCP::MergeScoringFunctionCP(
    const plugins::Options &options)
    : use_caching(options.get<bool>("use_caching")),
      shrink_strategy(options.get<shared_ptr<ShrinkStrategy>>("shrink_strategy")),
      max_states(options.get<int>("max_states")),
      max_states_before_merge(options.get<int>("max_states_before_merge")),
      shrink_threshold_before_merge(options.get<int>("threshold_before_merge")),
      ts_evaluation(options.get<TSEvaluation>("ts_evaluation")),
      component_aggregation(options.get<ComponentAggregation>("component_aggregation")),
      filter_trivial_factors(options.get<bool>("filter_trivial_factors")) {
}

static double compute_average(const vector<int> &values) {
    if (values.empty()) {
        return 0;
    }
    int sum = 0;
    for (int v : values) {
        sum += v;
    }
    return static_cast<double>(sum) / static_cast<double>(values.size());
}

static vector<vector<int>> compute_scp_h_values(
    const FactoredTransitionSystem &fts,
    const vector<int> &ts_indices,
    utils::LogProxy &log) {
    vector<int> label_costs = compute_label_costs(fts.get_labels());
    vector<vector<int>> result;
    result.reserve(ts_indices.size());
    for (size_t i = 0; i < ts_indices.size(); ++i) {
        int ts_index = ts_indices[i];
        assert(fts.is_active(ts_index));
        const TransitionSystem &transition_system = fts.get_transition_system(ts_index);

        if (log.is_at_least_debug()) {
            log << endl;
            log << "Abstraction index " << ts_index << endl;
//            transition_system.dump_labels_and_transitions();
            log << transition_system.tag() << endl;
            log << "Remaining label costs: " << label_costs << endl;
        }

        vector<int> goal_distances = compute_goal_distances_for_label_costs(
            transition_system, label_costs, log);
        if (log.is_at_least_debug()) {
            log << "Distances under remaining costs: " << goal_distances << endl;
        }

        // Only keep "useful" abstractions: abstractions which have non-zero
        // heuristic values or are non-total (map to infinite values). See
        // also comment at add_h_values in saturated_cost_partitionings.cc.
        if (!fts.get_mas_representation(ts_index).is_total() ||
            any_of(goal_distances.begin(), goal_distances.end(), [](int h) {
                assert(h != INF);
                return h > 0;
            })) {
            result.push_back(goal_distances);
        } else {
            result.emplace_back();
        }

        if (i == ts_indices.size() - 1) {
            return result;
        }

        vector<int> saturated_label_costs = compute_saturated_costs_for_transition_system(
            transition_system, goal_distances, fts.get_labels().get_num_total_labels(), log);

        reduce_costs(label_costs, saturated_label_costs);
    }
    // to silence compiler warning
    return result;
}

static int compute_value(
    const vector<vector<int>> &h_values,
    const vector<int> &abstract_states) {
    assert(h_values.size() == abstract_states.size());
    int h_val = 0;
    for (size_t i = 0; i < abstract_states.size(); ++i) {
        if (h_values[i].empty()) {
            // skip un-useful abstractions
            continue;
        }
        int abstract_state = abstract_states[i];
        if (abstract_state == PRUNED_STATE) {
            // If the state has been pruned, we encountered a dead end.
            return INF;
        }
        int cost = h_values[i][abstract_state];
        if (cost == INF) {
            // If the state is unreachable or irrelevant, we encountered a dead end.
            return INF;
        }
        h_val += cost;
    }
    return h_val;
}

double MergeScoringFunctionCP::compute_component_value_max(
    const FactoredTransitionSystem &fts,
    int index1,
    int index2) const {
    if (ts_evaluation == TSEvaluation::InitH) {
        int ts1_init = fts.get_transition_system(index1).get_init_state();
        int ts2_init = fts.get_transition_system(index2).get_init_state();
        if (ts1_init == PRUNED_STATE || ts2_init == PRUNED_STATE) {
            return INF;
        }
        int ts1_init_h = fts.get_distances(index1).get_goal_distance(ts1_init);
        int ts2_init_h = fts.get_distances(index2).get_goal_distance(ts2_init);
        return max(ts1_init_h, ts2_init_h);
    } else if (ts_evaluation == TSEvaluation::AvgH) {
        const vector<int> &h_values_ts1 = fts.get_distances(index1).get_goal_distances();
        const vector<int> &h_values_ts2 = fts.get_distances(index2).get_goal_distances();
        double average_ts1 = compute_average(h_values_ts1);
        double average_ts2 = compute_average(h_values_ts2);
        return max(average_ts1, average_ts2);
    } else {
        cerr << "unhandled value of enum TSEvaluation" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

double MergeScoringFunctionCP::compute_component_value_scp(
    const FactoredTransitionSystem &fts,
    int index1,
    int index2,
    vector<int> &trivial_factors,
    utils::LogProxy &log) const {
    // Compute the init h-value of the CP(s) over the product.
    vector<int> considered_factors;
    considered_factors.reserve(2);
    if (filter_trivial_factors) {
        if (trivial_factors.empty()) {
            trivial_factors.resize(fts.get_size(), -1);
        }

        if (trivial_factors[index1] == -1) {
            trivial_factors[index1] = fts.is_factor_trivial(index1);
        }
        if (trivial_factors[index2] == -1) {
            trivial_factors[index2] = fts.is_factor_trivial(index2);
        }

        if (trivial_factors[index1] == 0) {
            considered_factors.push_back(index1);
        }
        if (trivial_factors[index2] == 0) {
            considered_factors.push_back(index2);
        }
    } else {
        considered_factors.push_back(index1);
        considered_factors.push_back(index2);
    }

    if (considered_factors.empty()) {
        return 0;
    } else if (considered_factors.size() == 1) {
        int factor_index = considered_factors.front();
        if (ts_evaluation == TSEvaluation::InitH) {
            return fts.get_distances(factor_index).get_goal_distance(fts.get_transition_system(factor_index).get_init_state());
        } else if (ts_evaluation == TSEvaluation::AvgH) {
            const vector<int> &goal_distances = fts.get_distances(factor_index).get_goal_distances();
            return compute_average(goal_distances);
        } else {
            cerr << "unhandled value of enum TSEvaluation" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    } else {
        assert(considered_factors.size() == 2);
        assert(considered_factors[0] == index1);
        assert(considered_factors[1] == index2);

        // Compute MaxOverSCPs for both orders.
        vector<vector<int>> h_values_scp1 = compute_scp_h_values(fts, {index1, index2}, log);
        vector<vector<int>> h_values_scp2 = compute_scp_h_values(fts, {index2, index1}, log);

        if (ts_evaluation == TSEvaluation::InitH) {
            vector<int> abstract_states;
            abstract_states.reserve(2);
            int ts1_init = fts.get_transition_system(index1).get_init_state();
            int ts2_init = fts.get_transition_system(index2).get_init_state();
            abstract_states.push_back(ts1_init);
            abstract_states.push_back(ts2_init);
            int init_h_scp1 = compute_value(h_values_scp1, abstract_states);
            abstract_states.clear();
            abstract_states.push_back(ts2_init);
            abstract_states.push_back(ts1_init);
            int init_h_scp2 = compute_value(h_values_scp2, abstract_states);
            return max(init_h_scp1, init_h_scp2);
        } else if (ts_evaluation == TSEvaluation::AvgH) {
            double average_scp1 = compute_average(h_values_scp1[0]);
            average_scp1 += compute_average(h_values_scp1[1]);
            average_scp1 = average_scp1 / static_cast<double>(2);
            double average_scp2 = compute_average(h_values_scp2[0]);
            average_scp2 += compute_average(h_values_scp2[1]);
            average_scp2 = average_scp2 / static_cast<double>(2);
            return max(average_scp1, average_scp2);
        } else {
            cerr << "unhandled value of enum TSEvaluation" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }
}

vector<double> MergeScoringFunctionCP::compute_scores(
    const FactoredTransitionSystem &fts,
    const vector<pair<int, int>> &merge_candidates) {
    /*
      Score: component value - product value
      The component value is computed as follows:
      - TSEvaluation::InitH:
        - ComponentAggregation::MaxOverFactorHeuristics:
          maximum over initial h-values of both factor heuristics
        - ComponentAggregation::MaxOverSCPs:
          maximum over initial h-values of the 2 possible SCPs over the 2 factors
        - ComponentAggregation::OCP:
          initial h-value under the OCP over the 2 factors
      - TSEvaluation::AvgH:
        - ComponentAggregation::MaxOverFactorHeuristics:
          maximum over the averaged h-values of both factor heuristics
        - ComponentAggregation::MaxOverSCPs:
          maximum over the averaged h-values of both cost-partitioned h-values
          of each of the 2 factors (max over avg over avg)
      The product value is computed as follows:
      - TSEvaluation::InitH: initial h-value of the product factor
      - TSEvaluation::AvgH: average h-values of the product factor

      The lower the score, the better it is to compute the product instead of
      exploiting the information from the two factors.
    */
    vector<double> scores;
    scores.reserve(merge_candidates.size());
    vector<int> trivial_factors; // for SCP computation
    for (pair<int, int> merge_candidate : merge_candidates) {
        double score;
        int index1 = merge_candidate.first;
        int index2 = merge_candidate.second;
        if (use_caching && cached_scores_by_merge_candidate_indices[index1][index2]) {
            score = *cached_scores_by_merge_candidate_indices[index1][index2];
        } else {
            utils::LogProxy log = utils::get_silent_log();
            // Compute initial h value of the product.
            unique_ptr<TransitionSystem> product = shrink_before_merge_externally(
                fts,
                index1,
                index2,
                *shrink_strategy,
                max_states,
                max_states_before_merge,
                shrink_threshold_before_merge,
                log);
            unique_ptr<Distances> distances = utils::make_unique_ptr<Distances>(*product);
            const bool compute_init_distances = false;
            const bool compute_goal_distances = true;
            distances->compute_distances(compute_init_distances, compute_goal_distances, log);
            double product_value;
            if (ts_evaluation ==  TSEvaluation::InitH) {
                product_value = distances->get_goal_distance(product->get_init_state());
            } else if (ts_evaluation ==  TSEvaluation::AvgH) {
                const vector<int> &goal_distances = distances->get_goal_distances();
                product_value = compute_average(goal_distances);
            } else {
                cerr << "unhandled value of enum TSEvaluation" << endl;
                utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
            }

            double component_value;
            if (component_aggregation == ComponentAggregation::MaxOverFactorHeuristics) {
                component_value = compute_component_value_max(
                    fts, index1, index2);
            } else if (component_aggregation == ComponentAggregation::MaxOverSCPs) {
                component_value = compute_component_value_scp(
                    fts, index1, index2, trivial_factors, log);
            } else {
                cerr << "unhandled value of enum ComponentAggregation" << endl;
                utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
            }

            /*
              The score can be positive because CPs are computed over
              non-shrunk factors.
            */
            score = component_value - product_value;
            if (use_caching) {
                cached_scores_by_merge_candidate_indices[index1][index2] = score;
            }
        }
        scores.push_back(score);
    }
    return scores;
}

void MergeScoringFunctionCP::initialize(const TaskProxy &task_proxy) {
    initialized = true;
    int num_variables = task_proxy.get_variables().size();
    int max_factor_index = 2 * num_variables - 1;
    cached_scores_by_merge_candidate_indices.resize(
        max_factor_index,
        vector<optional<double>>(max_factor_index));
}

void MergeScoringFunctionCP::dump_function_specific_options(utils::LogProxy &log) const {
    if (log.is_at_least_normal()) {
        log << "Use caching: " << (use_caching ? "yes" : "no") << endl;
    }
}

string MergeScoringFunctionCP::name() const {
    return "sf_cp";
}

class MergeScoringFunctionCPFeature : public plugins::TypedFeature<MergeScoringFunction, MergeScoringFunctionCP> {
public:
    MergeScoringFunctionCPFeature() : TypedFeature("sf_cp") {
        document_title("CostPartitioning or Merge");
        document_synopsis(
            "This scoring function compares how merging two factors would do "
            "against computing a cost partitioning over the two factors "
            "instead. To this end, for each candidate, it computes the product "
            "(including shrinking according to the given options which should "
            "be chosen to match those of the merge-and-shrink algorithm) and "
            "the cost partitioning (OPC or SCP) and compares the initial h "
            "value of both, i.e., the score is the value of the product minus "
            "that of the CP. In the case of using SCPs, it computes two SCPs, "
            "one for each order, and takes the maximum. Note that because the "
            "CP is computed over the *unshrunk* factors, the difference can "
            "actually become positive.");
        add_option<bool>(
            "use_caching",
            "Cache scores for merge candidates. IMPORTANT! This only works "
            "under the assumption that the merge-and-shrink algorithm only "
            "uses exact label reduction and does not (non-exactly) shrink "
            "factors other than those being merged in the current iteration. "
            "In this setting, the MIASM score of a merge candidate is constant "
            "over merge-and-shrink iterations. If caching is enabled, only the "
            "scores for the new merge candidates need to be computed.",
            "true");

        // TODO: use shrink strategy and limit options from MergeAndShrinkHeuristic
        // instead of having the identical options here again.
        add_option<shared_ptr<ShrinkStrategy>>(
            "shrink_strategy",
            "We recommend setting this to match the shrink strategy configuration "
            "given to {{{merge_and_shrink}}}, see note below.");
        add_transition_system_size_limit_options_to_feature(*this);
        add_option<TSEvaluation>(
            "ts_evaluation",
            "");
        add_option<ComponentAggregation>(
            "component_aggregation",
            "");

        add_option<bool>(
            "filter_trivial_factors",
            "If true, do not consider trivial factors for computing CPs. Should "
            "be set to true when computing SCPs.");
    }

    virtual shared_ptr<MergeScoringFunctionCP> create_component(
        const plugins::Options &options, const utils::Context &context) const override {
        plugins::Options options_copy(options);
        handle_shrink_limit_options_defaults(options_copy, context);
        return make_shared<MergeScoringFunctionCP>(options_copy);
    }
};

static plugins::FeaturePlugin<MergeScoringFunctionCPFeature> _plugin;

static plugins::TypedEnumPlugin<TSEvaluation> _ts_evaluation_enum_plugin({
        {"init_h",
         "initial h-value"},
        {"avg_h",
         "average h-value"},
    });

static plugins::TypedEnumPlugin<ComponentAggregation> _component_aggregation_enum_plugin({
        {"max_factor",
         "maximum over the factor heuristics"},
        {"max_scp",
         "maximum over the scp heuristics"},
    });
}
