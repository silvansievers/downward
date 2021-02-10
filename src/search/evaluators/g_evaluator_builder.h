#ifndef EVALUATORS_G_EVALUATOR_BUILDER_H
#define EVALUATORS_G_EVALUATOR_BUILDER_H

#include "../evaluator_builder.h"

namespace g_evaluator {
class GEvaluatorBuilder : public EvaluatorBuilder {
protected:
    virtual std::shared_ptr<Evaluator> build(
        PluginVariables &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
