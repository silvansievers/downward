#include "eager_search_builder.h"

#include "eager_search.h"

#include "../evaluator_builder.h"
#include "../open_list_factory.h"

using namespace std;

namespace eager_search {
EagerSearchBuilder::EagerSearchBuilder(const options::Options &opts)
    : SearchEngineBuilder(
          opts.get<int>("bound"),
          opts.get<double>("max_time"),
          opts.get<OperatorCost>("cost_type"),
          opts.get<utils::Verbosity>("verbosity")),
      reopen_closed_nodes(opts.get<bool>("reopen_closed")),
      open_list_builder(opts.get<shared_ptr<OpenListFactory>>("open")),
      f_evaluator_builder(opts.get<shared_ptr<EvaluatorBuilder>>("f_eval", nullptr)),
      preferred_operator_evaluator_builders(opts.get_list<shared_ptr<EvaluatorBuilder>>("preferred")),
      lazy_evaluator_builder(opts.get<shared_ptr<EvaluatorBuilder>>("lazy_evaluator", nullptr)) {
}

shared_ptr<SearchEngine> EagerSearchBuilder::build(
    PluginVariables &variable_context, const shared_ptr<AbstractTask> &task) const {
    vector<shared_ptr<Evaluator>> preferred_operator_evaluators;
    preferred_operator_evaluators.reserve(preferred_operator_evaluator_builders.size());
    for (auto &builder : preferred_operator_evaluator_builders) {
        preferred_operator_evaluators.push_back(builder->get_built_element(variable_context, task));
    }
    return make_shared<EagerSearch>(
        open_list_builder->create_state_open_list(variable_context, task),
        f_evaluator_builder->get_built_element(variable_context, task),
        preferred_operator_evaluators,
        lazy_evaluator_builder ?
            lazy_evaluator_builder->get_built_element(variable_context, task) : nullptr,
        bound, max_time, cost_type, reopen_closed_nodes, verbosity);
}

void add_options_to_parser(options::OptionParser &parser) {
    add_search_pruning_option(parser);
    add_search_options_to_parser(parser);
}
}
