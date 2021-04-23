#ifndef SEARCH_ENGINES_EAGER_SEARCH_BUILDER_H
#define SEARCH_ENGINES_EAGER_SEARCH_BUILDER_H

#include "../plugin_builder.h"
#include "../search_engine_builder.h"

class Evaluator;
class OpenListFactory;

namespace eager_search {
class EagerSearchBuilder : public SearchEngineBuilder {
    const bool reopen_closed_nodes;

    std::shared_ptr<OpenListFactory> open_list_builder;
    std::shared_ptr<PluginBuilder<Evaluator>> f_evaluator_builder;
    std::vector<std::shared_ptr<PluginBuilder<Evaluator>>> preferred_operator_evaluator_builders;
    std::shared_ptr<PluginBuilder<Evaluator>> lazy_evaluator_builder;
protected:
    virtual std::shared_ptr<SearchEngine> build(
        PluginVariableAssignment &variable_context,
        const std::shared_ptr<AbstractTask> &task) const override;
public:
    explicit EagerSearchBuilder(const options::Options &opts);
};

extern void add_options_to_parser(options::OptionParser &parser);
}

#endif
