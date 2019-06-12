#include "g_evaluator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"

using namespace std;

namespace g_evaluator {
EvaluationResult GEvaluator::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;
    result.set_evaluator_value(eval_context.get_g_value());
    return result;
}
}
