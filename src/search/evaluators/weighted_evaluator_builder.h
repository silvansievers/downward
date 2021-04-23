#ifndef EVALUATORS_WEIGHTED_EVALUATOR_BUILDER_H
#define EVALUATORS_WEIGHTED_EVALUATOR_BUILDER_H

#include "../evaluator_builder.h"

namespace weighted_evaluator {
class WeightedEvaluatorBuilder : public EvaluatorBuilder {
    std::shared_ptr<EvaluatorBuilder> evaluator;
    int w;
public:
    explicit WeightedEvaluatorBuilder(const options::Options &opts);
    WeightedEvaluatorBuilder(const std::shared_ptr<EvaluatorBuilder> &eval, int weight);

    virtual std::shared_ptr<Evaluator> build() const override;
};
}

#endif
