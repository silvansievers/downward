#ifndef PLUGIN_VARIABLE_BUILDER_H
#define PLUGIN_VARIABLE_BUILDER_H

#include "plugin_builder.h"
#include "plugin_variable_assignment.h"

#include <string>

template<typename T>
class PluginVariableBuilder : public PluginBuilder<T> {
    std::string name;
public:
    explicit PluginVariableBuilder(const std::string &name);

    virtual std::shared_ptr<T> create(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
};

template<typename T>
inline PluginVariableBuilder<T>::PluginVariableBuilder(const std::string &name)
    : name(name) {
}

template<typename T>
inline std::shared_ptr<T> PluginVariableBuilder<T>::create(
    PluginVariableAssignment &variable_context,
    const std::shared_ptr<AbstractTask> &task) const {
    return variable_context.get<T>(name, task);
}

#endif
