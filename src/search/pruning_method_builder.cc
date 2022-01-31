#include "pruning_method.h"
#include "plugin.h"
#include "plugin_builder.h"

static PluginTypePlugin<PluginBuilder<PruningMethod>> _type_plugin(
    "PruningMethod",
    "Prune or reorder applicable operators.");
