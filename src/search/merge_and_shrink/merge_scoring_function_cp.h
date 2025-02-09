#ifndef MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_CP_H
#define MERGE_AND_SHRINK_MERGE_SCORING_FUNCTION_CP_H

#include "merge_scoring_function.h"

#include <memory>
#include <optional>

namespace plugins {
class Options;
}

namespace merge_and_shrink {
class ShrinkStrategy;

enum class TSEvaluation {
    InitH,
    AvgH
};

enum class ComponentAggregation {
    MaxOverFactorHeuristics,
    MaxOverSCPs
};

class MergeScoringFunctionCP : public MergeScoringFunction {
    const bool use_caching;
    std::shared_ptr<ShrinkStrategy> shrink_strategy;
    const int max_states;
    const int max_states_before_merge;
    const int shrink_threshold_before_merge;
    TSEvaluation ts_evaluation;
    ComponentAggregation component_aggregation;
    const bool filter_trivial_factors;
    std::vector<std::vector<std::optional<double>>> cached_scores_by_merge_candidate_indices;

    double compute_component_value_max(
        const FactoredTransitionSystem &fts,
        int index1,
        int index2) const;
    double compute_component_value_scp(
        const FactoredTransitionSystem &fts,
        int index1,
        int index2,
        std::vector<int> &trivial_factors,
        utils::LogProxy &log) const;

    virtual std::string name() const override;
    virtual void dump_function_specific_options(utils::LogProxy &log) const override;
public:
    explicit MergeScoringFunctionCP(const plugins::Options &options);
    virtual ~MergeScoringFunctionCP() override = default;
    virtual std::vector<double> compute_scores(
        const FactoredTransitionSystem &fts,
        const std::vector<std::pair<int, int>> &merge_candidates) override;
    virtual void initialize(const TaskProxy &task_proxy) override;

    virtual bool requires_init_distances() const override {
        return true;
    }

    virtual bool requires_goal_distances() const override {
        return true;
    }
};
}

#endif
