#include "null_pruning_method_builder.h"

#include "null_pruning_method.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace null_pruning_method {
shared_ptr<PruningMethod> NullPruningMethodBuilder::create(
    PluginVariableAssignment &, const shared_ptr<AbstractTask> &) const {
    return make_shared<NullPruningMethod>();
}

static shared_ptr<PluginBuilder<PruningMethod>> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "No pruning",
        "This is a skeleton method that does not perform any pruning, i.e., "
        "all applicable operators are applied in all expanded states. ");

    if (parser.dry_run()) {
        return nullptr;
    }

    return make_shared<NullPruningMethodBuilder>();
}

static Plugin<PluginBuilder<PruningMethod>> _plugin("null", _parse);
}
