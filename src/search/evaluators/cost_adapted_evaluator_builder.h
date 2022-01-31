#ifndef EVALUATORS_G_EVALUATOR_BUILDER_H
#define EVALUATORS_G_EVALUATOR_BUILDER_H

#include "../operator_cost.h"
#include "../plugin_builder.h"

class Evaluator;

namespace options {
class Options;
}

namespace cost_adapted_evaluator {
class CostadaptedEvaluatorBuilder : public PluginBuilder<Evaluator> {
    std::shared_ptr<PluginBuilder<Evaluator>> child_evaluator_builder;
    OperatorCost cost_type;
protected:
    virtual std::shared_ptr<Evaluator> create(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    explicit CostadaptedEvaluatorBuilder(const options::Options &opts);
};
}

#endif
