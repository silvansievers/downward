#include "heuristic_builder.h"

#include "abstract_task.h"
#include "option_parser.h"

using namespace std;

HeuristicBuilder::HeuristicBuilder(const options::Options &opts)
    : EvaluatorBuilder(opts) {
}

void add_heuristic_options_to_parser(OptionParser &parser) {
    parser.add_option<shared_ptr<AbstractTask>>(
        "transform",
        "Optional task transformation for the heuristic."
        " Currently, adapt_costs() and no_transform() are available.",
        "no_transform()");
    parser.add_option<bool>("cache_estimates", "cache heuristic estimates", "true");
}
