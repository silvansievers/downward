#include "combining_evaluator_builder.h"

#include "combining_evaluator.h"

using namespace std;

namespace combining_evaluator {
CombiningEvaluatorBuilder::CombiningEvaluatorBuilder(
    const options::Options &opts,
    const vector<shared_ptr<EvaluatorBuilder>> &subevaluators_) // TODO: could directly get from options
    : EvaluatorBuilder(opts),
      subevaluators(subevaluators_) {
}
}
