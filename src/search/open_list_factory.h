#ifndef OPEN_LIST_FACTORY_H
#define OPEN_LIST_FACTORY_H

#include "open_list.h"
#include "plugin_variable_assignment.h"

#include <memory>

class AbstractTask;

class OpenListFactory {
protected:
    bool preferred_only;
public:
    explicit OpenListFactory(bool preferred_only)
        : preferred_only(preferred_only) {
    }
    virtual ~OpenListFactory() = default;

    OpenListFactory(const OpenListFactory &) = delete;

    virtual std::unique_ptr<StateOpenList> create_state_open_list(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) = 0;
    //virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() = 0;

    /*
      The following template receives manual specializations (in the
      cc file) for the open list types we want to support. It is
      intended for templatized callers, e.g. the constructor of
      AlternationOpenList.
    */
    template<typename T>
    std::unique_ptr<OpenList<T>> create_open_list(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task);
};

#endif
