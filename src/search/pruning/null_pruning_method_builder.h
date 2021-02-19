#ifndef PRUNING_NULL_PRUNING_METHOD_BUILDER_H
#define PRUNING_NULL_PRUNING_METHOD_BUILDER_H

#include "../plugin_builder.h"

class PruningMethod;

namespace null_pruning_method {
class NullPruningMethodBuilder : public PluginBuilder<PruningMethod> {
protected:
    virtual std::shared_ptr<PruningMethod> build(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;

};
}

#endif
