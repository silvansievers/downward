#include "g_evaluator_builder.h"

#include "g_evaluator.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace g_evaluator {
GEvaluatorBuilder::GEvaluatorBuilder() : EvaluatorBuilder(options::Options()) {
}

shared_ptr<Evaluator> GEvaluatorBuilder::build() const {
    return make_shared<GEvaluator>();
}

static shared_ptr<EvaluatorBuilder> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "g-value evaluator",
        "Returns the g-value (path cost) of the search node.");
    parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<GEvaluatorBuilder>();
}

static Plugin<EvaluatorBuilder> _plugin("g", _parse, "evaluators_basic");
}
