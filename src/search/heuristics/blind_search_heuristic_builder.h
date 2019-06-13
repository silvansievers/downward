#ifndef HEURISTICS_BLIND_SEARCH_HEURISTIC_BUILDER_H
#define HEURISTICS_BLIND_SEARCH_HEURISTIC_BUILDER_H

#include "../heuristic_builder.h"

namespace blind_search_heuristic {
class BlindSearchHeuristicBuilder : public HeuristicBuilder {
    int min_operator_cost;
public:
    explicit BlindSearchHeuristicBuilder(const options::Options &opts);

    virtual std::shared_ptr<Evaluator> build(
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
