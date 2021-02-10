#ifndef EVALUATORS_SUM_EVALUATOR_BUILDER_H
#define EVALUATORS_SUM_EVALUATOR_BUILDER_H

#include "combining_evaluator_builder.h"

namespace options {
class Options;
}

namespace sum_evaluator {
class SumEvaluatorBuilder : public combining_evaluator::CombiningEvaluatorBuilder {
protected:
    virtual std::shared_ptr<Evaluator> build(
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    explicit SumEvaluatorBuilder(const options::Options &opts);
    explicit SumEvaluatorBuilder(const std::vector<std::shared_ptr<EvaluatorBuilder>> &evals);
};
}

#endif
