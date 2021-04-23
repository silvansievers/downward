#ifndef PLUGIN_LET_BUILDER_H
#define PLUGIN_LET_BUILDER_H

#include "plugin_builder.h"
#include "plugin_variable_assignment.h"

#include <memory>
#include <string>

template<typename VarType, typename ReturnType>
class PluginLetBuilder : public PluginBuilder<ReturnType> {
    std::string name;
    std::shared_ptr<PluginBuilder<VarType>> element_builder;
    std::shared_ptr<PluginBuilder<ReturnType>> nested_builder;
public:
    PluginLetBuilder(
        std::string name,
        const std::shared_ptr<PluginBuilder<VarType>> &element_builder,
        const std::shared_ptr<PluginBuilder<ReturnType>> &nested_builder);
    virtual std::shared_ptr<ReturnType> build(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
};

template<typename VarType, typename ReturnType>
inline PluginLetBuilder<VarType, ReturnType>::PluginLetBuilder(
    std::string name,
    const std::shared_ptr<PluginBuilder<VarType>> &element_builder,
    const std::shared_ptr<PluginBuilder<ReturnType>> &nested_builder)
    : name(name), element_builder(element_builder), nested_builder(nested_builder) {
}

template<typename VarType, typename ReturnType>
inline std::shared_ptr<ReturnType> PluginLetBuilder<VarType, ReturnType>::build(
    PluginVariableAssignment &variable_context,
    const std::shared_ptr<AbstractTask> &task) const {
    variable_context.push(name, element_builder);
    std::shared_ptr<ReturnType> result = nested_builder->build(variable_context, task);
    variable_context.pop();
    return result;
}

#endif
