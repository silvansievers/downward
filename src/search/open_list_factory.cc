#include "open_list_factory.h"

#include "plugin.h"

using namespace std;


template<>
unique_ptr<StateOpenList> OpenListFactory::create_open_list(
    PluginVariableAssignment &variable_context, const shared_ptr<AbstractTask> &task) {
    return create_state_open_list(variable_context, task);
}

/*
template<>
unique_ptr<EdgeOpenList> OpenListFactory::create_open_list() {
    return create_edge_open_list();
}
*/

static PluginTypePlugin<OpenListFactory> _type_plugin(
    "OpenList",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
