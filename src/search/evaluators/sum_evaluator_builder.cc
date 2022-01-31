#include "sum_evaluator_builder.h"

#include "sum_evaluator.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <cassert>
#include <limits>

using namespace std;

namespace sum_evaluator {
SumEvaluatorBuilder::SumEvaluatorBuilder(const Options &opts)
    : CombiningEvaluatorBuilder(opts.get_list<shared_ptr<PluginBuilder<Evaluator>>>("evals")) {
}

SumEvaluatorBuilder::SumEvaluatorBuilder(const vector<shared_ptr<PluginBuilder<Evaluator>>> &evals)
    : CombiningEvaluatorBuilder(evals) {
}

shared_ptr<Evaluator> SumEvaluatorBuilder::create(
    PluginVariableAssignment &variable_context,
    const std::shared_ptr<AbstractTask> &task) const {
    vector<shared_ptr<Evaluator>> evaluators;
    for (auto &subeval : subevaluators) { // TODO: could directly get from options
        evaluators.push_back(subeval->build(variable_context, task));
    }
    return make_shared<SumEvaluator>(task, evaluators);
}

// TODO: should this be CombiningEvaluatorBuilder? Compiler doesn't like it.
static shared_ptr<SumEvaluatorBuilder> _parse(OptionParser &parser) {
    parser.document_synopsis("Sum evaluator",
                             "Calculates the sum of the sub-evaluators.");

    parser.add_list_option<shared_ptr<PluginBuilder<Evaluator>>>("evals", "at least one evaluator");
    Options opts = parser.parse();

    opts.verify_list_non_empty<shared_ptr<PluginBuilder<Evaluator>>>("evals");

    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<SumEvaluatorBuilder>(opts);
}

static Plugin<PluginBuilder<Evaluator>> _plugin("sum", _parse, "evaluators_basic");
}
