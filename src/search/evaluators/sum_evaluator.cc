#include "sum_evaluator.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>

using namespace std;

namespace sum_evaluator {
SumEvaluator::SumEvaluator(const vector<shared_ptr<Evaluator>> &evals)
    : CombiningEvaluator(evals) {
}

SumEvaluator::~SumEvaluator() {
}

int SumEvaluator::combine_values(const vector<int> &values) {
    int result = 0;
    for (int value : values) {
        assert(value >= 0);
        result += value;
        assert(result >= 0); // Check against overflow.
    }
    return result;
}
}
