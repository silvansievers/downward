#ifndef HEURISTICS_BLIND_SEARCH_HEURISTIC_BUILDER_H
#define HEURISTICS_BLIND_SEARCH_HEURISTIC_BUILDER_H

#include "../heuristic_builder.h"
#include "../plugin_cache.h"

namespace blind_search_heuristic {
class BlindSearchHeuristicBuilder : public HeuristicBuilder {
    mutable PluginCache<Evaluator> cached_evaluators;
public:
    explicit BlindSearchHeuristicBuilder(const options::Options &opts);

    virtual std::shared_ptr<Evaluator> build(
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
