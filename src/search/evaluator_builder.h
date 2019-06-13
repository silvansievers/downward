#ifndef EVALUATOR_BUILDER_H
#define EVALUATOR_BUILDER_H

#include "../options/options.h"

#include <memory>

class AbstractTask;
class Evaluator;

class EvaluatorBuilder {
protected:
    const options::Options opts;
public:
    explicit EvaluatorBuilder(const options::Options &opts);
    virtual ~EvaluatorBuilder() = default;

    virtual std::shared_ptr<Evaluator> build(const std::shared_ptr<AbstractTask> &task) const = 0;
};

#endif
