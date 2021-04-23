#include "g_evaluator_builder.h"

#include "g_evaluator.h"

#include "../evaluator.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace g_evaluator {
shared_ptr<Evaluator> GEvaluatorBuilder::build(
    PluginVariableAssignment &, const shared_ptr<AbstractTask> &task) const {
    return make_shared<GEvaluator>(task);
}

static shared_ptr<PluginBuilder<Evaluator>> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "g-value evaluator",
        "Returns the g-value (path cost) of the search node.");
    parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GEvaluatorBuilder>();
}

static Plugin<PluginBuilder<Evaluator>> _plugin("g", _parse, "evaluators_basic");
}
