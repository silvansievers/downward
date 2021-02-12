#include "blind_search_heuristic_builder.h"

#include "blind_search_heuristic.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

#include <cstddef>
#include <limits>
#include <utility>

using namespace std;

namespace blind_search_heuristic {
BlindSearchHeuristicBuilder::BlindSearchHeuristicBuilder(const options::Options &opts)
    : HeuristicBuilder(opts) {
}

shared_ptr<Evaluator> BlindSearchHeuristicBuilder::build(
    PluginVariableAssignment &, const shared_ptr<AbstractTask> &task) const {
    return make_shared<BlindSearchHeuristic>(task, cache_estimates, name);
}

static shared_ptr<PluginBuilder<Evaluator>> _parse(OptionParser &parser) {
    parser.document_synopsis("Blind heuristic",
                             "Returns cost of cheapest action for "
                             "non-goal states, "
                             "0 for goal states");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support("axioms", "supported");
    parser.document_property("admissible", "yes");
    parser.document_property("consistent", "yes");
    parser.document_property("safe", "yes");
    parser.document_property("preferred operators", "no");

    add_heuristic_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<BlindSearchHeuristicBuilder>(opts);
}

static Plugin<PluginBuilder<Evaluator>> _plugin("blind", _parse);
}
