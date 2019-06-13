#include "cost_adapted_evaluator_builder.h"

#include "../plugin.h"

#include "../tasks/cost_adapted_task.h"

using namespace std;

namespace cost_adapted_evaluator {
CostadaptedEvaluatorBuilder::CostadaptedEvaluatorBuilder(const options::Options &opts)
    : child_evaluator_builder(opts.get<shared_ptr<EvaluatorBuilder>>("evaluator")),
      cost_type(static_cast<OperatorCost>(opts.get_enum("cost_type"))) {
}

shared_ptr<Evaluator> CostadaptedEvaluatorBuilder::build(
    const shared_ptr<AbstractTask> &task) const {
    shared_ptr<AbstractTask> cost_adapted_task =
        make_shared<tasks::CostAdaptedTask>(task, cost_type);
    return child_evaluator_builder->build(cost_adapted_task);
}

static shared_ptr<EvaluatorBuilder> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Cost-adapted evaluator",
        "Apply a cost-transformation to the root task.");
    add_cost_type_option_to_parser(parser);
    options::Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
        return make_shared<CostadaptedEvaluatorBuilder>(opts);
    }
}

static Plugin<EvaluatorBuilder> _plugin("adapt_costs_new", _parse, "evaluators_basic");
}
