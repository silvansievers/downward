#ifndef EVALUATORS_G_EVALUATOR_BUILDER_H
#define EVALUATORS_G_EVALUATOR_BUILDER_H

#include "../plugin_builder.h"

class Evaluator;

namespace g_evaluator {
class GEvaluatorBuilder : public PluginBuilder<Evaluator> {
protected:
    virtual std::shared_ptr<Evaluator> create(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
};
}

#endif
