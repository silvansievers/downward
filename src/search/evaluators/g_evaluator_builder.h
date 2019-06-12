#ifndef EVALUATORS_G_EVALUATOR_BUILDER_H
#define EVALUATORS_G_EVALUATOR_BUILDER_H

#include "../evaluator_builder.h"

namespace g_evaluator {
class GEvaluatorBuilder : public EvaluatorBuilder {
public:
    GEvaluatorBuilder();

    virtual std::shared_ptr<Evaluator> build() const override;
};
}

#endif
