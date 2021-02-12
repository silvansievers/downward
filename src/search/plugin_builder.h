#ifndef PLUGIN_BUILDER_H
#define PLUGIN_BUILDER_H

#include "abstract_task.h"

#include <memory>

class PluginVariableAssignment;

template<typename T>
class PluginBuilder {
public:
    virtual std::shared_ptr<T> build(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const = 0;
    virtual ~PluginBuilder() = default;
};

#endif
