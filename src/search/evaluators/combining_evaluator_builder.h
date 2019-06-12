#ifndef EVALUATORS_COMBINING_EVALUATOR_BUILDER_H
#define EVALUATORS_COMBINING_EVALUATOR_BUILDER_H

#include "../evaluator_builder.h"

#include <vector>

namespace combining_evaluator {
class CombiningEvaluatorBuilder : public EvaluatorBuilder {
protected:
    std::vector<std::shared_ptr<EvaluatorBuilder>> subevaluators;
public:
    CombiningEvaluatorBuilder(
        const options::Options &opts,
        const std::vector<std::shared_ptr<EvaluatorBuilder>> &subevaluators_);
};
}

#endif
