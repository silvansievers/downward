#ifndef EVALUATORS_SUM_EVALUATOR_H
#define EVALUATORS_SUM_EVALUATOR_H

#include "combining_evaluator.h"

#include <memory>
#include <vector>

namespace sum_evaluator {
class SumEvaluator : public combining_evaluator::CombiningEvaluator {
protected:
    virtual int combine_values(const std::vector<int> &values) override;
public:
    SumEvaluator(
        const std::shared_ptr<AbstractTask> &task,
        const std::vector<std::shared_ptr<Evaluator>> &evals);
    virtual ~SumEvaluator() override;
};
}

#endif
