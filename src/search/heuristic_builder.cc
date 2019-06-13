#include "heuristic_builder.h"

#include "abstract_task.h"
#include "option_parser.h"

using namespace std;

HeuristicBuilder::HeuristicBuilder(const options::Options &opts)
    : EvaluatorBuilder(opts),
      cache_estimates(opts.get<bool>("cache_estimates")) {
}

void add_heuristic_options_to_parser(OptionParser &parser) {
    parser.add_option<bool>("cache_estimates", "cache heuristic estimates", "true");
}
