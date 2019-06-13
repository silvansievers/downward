#ifndef EVALUATOR_BUILDER_H
#define EVALUATOR_BUILDER_H

#include <memory>

class AbstractTask;
class Evaluator;

class EvaluatorBuilder {
public:
    virtual ~EvaluatorBuilder() = default;

    virtual std::shared_ptr<Evaluator> build(const std::shared_ptr<AbstractTask> &task) const = 0;
};

#endif
