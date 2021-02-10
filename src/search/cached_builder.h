#ifndef CACHED_BUILDER_H
#define CACHED_BUILDER_H

#include "per_task_information.h"
#include "plugin_variables.h"

#include <memory>

class AbstractTask;

template<typename T>
class CachedBuilder {
    mutable PerTaskInformation<std::shared_ptr<T>> constructed_elements;
protected:
    virtual std::shared_ptr<T> build(
        PluginVariables &variable_context,
        const std::shared_ptr<AbstractTask> &task) const = 0;
public:
    CachedBuilder()
        : constructed_elements([](const TaskProxy&){ return nullptr; }){
    }
    virtual ~CachedBuilder() = default;

    /* Get the element from the cache if possible and construct it otherwise */
    std::shared_ptr<T> get_built_element(
        PluginVariables &variable_context,
        const std::shared_ptr<AbstractTask> &task) const {
        TaskProxy task_proxy(*task);
        if (!constructed_elements[task_proxy]) {
            constructed_elements[task_proxy] = build(variable_context, task);
        }
        return constructed_elements[task_proxy];
    }
};

#endif
