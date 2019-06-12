#include "weighted_evaluator_builder.h"

#include "weighted_evaluator.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cstdlib>
#include <sstream>

using namespace std;

namespace weighted_evaluator {
WeightedEvaluatorBuilder::WeightedEvaluatorBuilder(const Options &opts)
    : EvaluatorBuilder(opts),
      evaluator(opts.get<shared_ptr<EvaluatorBuilder>>("eval")),
      w(opts.get<int>("weight")) {
}

WeightedEvaluatorBuilder::WeightedEvaluatorBuilder(const shared_ptr<EvaluatorBuilder> &eval, int weight)
    : EvaluatorBuilder(options::Options()), // TODO: re-think if having options in the base class makes sense.
      evaluator(eval), w(weight) {
}

shared_ptr<Evaluator> WeightedEvaluatorBuilder::build() const {
    return make_shared<WeightedEvaluator>(evaluator->build(), w);
}

static shared_ptr<EvaluatorBuilder> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Weighted evaluator",
        "Multiplies the value of the evaluator with the given weight.");
    parser.add_option<shared_ptr<EvaluatorBuilder>>("eval", "evaluator");
    parser.add_option<int>("weight", "weight");
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<WeightedEvaluatorBuilder>(opts);
}

static Plugin<EvaluatorBuilder> _plugin("weight", _parse, "evaluators_basic");
}
