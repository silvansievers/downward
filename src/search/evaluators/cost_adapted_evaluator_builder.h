#ifndef EVALUATORS_G_EVALUATOR_BUILDER_H
#define EVALUATORS_G_EVALUATOR_BUILDER_H

#include "../evaluator_builder.h"
#include "../operator_cost.h"

namespace options {
class Options;
}

namespace cost_adapted_evaluator {
class CostadaptedEvaluatorBuilder : public EvaluatorBuilder {
    std::shared_ptr<EvaluatorBuilder> child_evaluator_builder;
    OperatorCost cost_type;
protected:
    virtual std::shared_ptr<Evaluator> build(
        PluginVariables &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    explicit CostadaptedEvaluatorBuilder(const options::Options &opts);
};
}

#endif
