#ifndef PLUGIN_BUILDER_H
#define PLUGIN_BUILDER_H

#include "abstract_task.h"

#include <memory>
#include <unordered_map>

class PluginVariableAssignment;

template<typename T>
class PluginBuilder {
protected:
    std::unordered_map<std::shared_ptr<AbstractTask>, std::shared_ptr<T>> cache;
    virtual std::shared_ptr<T> create(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const = 0;
public:
    std::shared_ptr<T> build(
            PluginVariableAssignment &variable_context,
            const std::shared_ptr<AbstractTask> &task) {
        if (cache.count(task) > 0) {
            return cache[task];
        } else {
            std::shared_ptr<T> ret = create(variable_context, task);
            cache.insert(std::make_pair(task,ret));
            return ret;
        }
    }
    virtual ~PluginBuilder() = default;
};

#endif
