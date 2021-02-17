#ifndef HEURISTICS_LM_CUT_HEURISTIC_HEURISTIC_BUILDER_H
#define HEURISTICS_LM_CUT_HEURISTIC_HEURISTIC_BUILDER_H

#include "../heuristic_builder.h"

namespace lm_cut_heuristic {
class LandmarkCutHeuristicBuilder : public HeuristicBuilder {
protected:
    virtual std::shared_ptr<Evaluator> build(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    explicit LandmarkCutHeuristicBuilder(const options::Options &opts);

};
}

#endif
