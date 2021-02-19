#include "lm_cut_heuristic_builder.h"

#include "lm_cut_heuristic.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace lm_cut_heuristic {
LandmarkCutHeuristicBuilder::LandmarkCutHeuristicBuilder(const options::Options &opts)
    : HeuristicBuilder(opts) {
}

shared_ptr<Evaluator> LandmarkCutHeuristicBuilder::create(
    PluginVariableAssignment &, const shared_ptr<AbstractTask> &task) const {
    return make_shared<LandmarkCutHeuristic>(task, cache_estimates, name);
}

static shared_ptr<PluginBuilder<Evaluator>> _parse(OptionParser &parser) {
    parser.document_synopsis("Landmark-cut heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    add_heuristic_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<LandmarkCutHeuristicBuilder>(opts);
}

static Plugin<PluginBuilder<Evaluator>> _plugin("lmcut", _parse);
}
