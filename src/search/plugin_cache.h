#ifndef PLUGIN_CACHE_H
#define PLUGIN_CACHE_H

// TODO: this is adapted from PerStateInformation. Can we combine the two classes?
// TODO: caching heuristics doesn't work when using task transformations since
//       the builder sees different AbstractTask pointers.

#include "task_proxy.h"

#include "algorithms/subscriber.h"
#include "utils/hash.h"
#include "utils/memory.h"

#include <functional>

template<class Entry>
class PluginCache : public subscriber::Subscriber<AbstractTask> {
    using EntryConstructor = std::function<std::shared_ptr<Entry>(
                                               const std::shared_ptr<AbstractTask> &)>;
    EntryConstructor entry_constructor;
    utils::HashMap<TaskID, std::shared_ptr<Entry>> entries;
public:
    explicit PluginCache(EntryConstructor entry_constructor)
        : entry_constructor(entry_constructor) {
    }

    std::shared_ptr<Entry> &operator[](const std::shared_ptr<AbstractTask> &task) {
        TaskID id(task.get());
        const auto &it = entries.find(id);
        if (it == entries.end()) {
            entries[id] = entry_constructor(task);
            TaskProxy(*task).subscribe_to_task_destruction(this);
        }
        return entries[id];
    }

    virtual void notify_service_destroyed(const AbstractTask *task) override {
        TaskID id = TaskProxy(*task).get_id();
        entries.erase(id);
    }
};

#endif
