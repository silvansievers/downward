#ifndef EVALUATORS_G_EVALUATOR_BUILDER_H
#define EVALUATORS_G_EVALUATOR_BUILDER_H

#include "../evaluator_builder.h"

namespace cost_adapted_evaluator {
class CostadaptedEvaluatorBuilder : public EvaluatorBuilder {
public:
    explicit CostadaptedEvaluatorBuilder(const options::Options &opts);

    virtual std::shared_ptr<Evaluator> build(
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
