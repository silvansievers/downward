#ifndef SEARCH_ENGINES_EAGER_SEARCH_BUILDER_H
#define SEARCH_ENGINES_EAGER_SEARCH_BUILDER_H

#include "../search_engine_builder.h"

class EvaluatorBuilder;
class OpenListFactory;

namespace eager_search {
class EagerSearchBuilder : public SearchEngineBuilder {
    const bool reopen_closed_nodes;

    std::shared_ptr<OpenListFactory> open_list_builder;
    std::shared_ptr<EvaluatorBuilder> f_evaluator_builder;
    std::vector<std::shared_ptr<EvaluatorBuilder>> preferred_operator_evaluator_builders;
    std::shared_ptr<EvaluatorBuilder> lazy_evaluator_builder;
public:
    explicit EagerSearchBuilder(const options::Options &opts);

    virtual std::shared_ptr<SearchEngine> build(
        const std::shared_ptr<AbstractTask> &task) const override;
};

extern void add_options_to_parser(options::OptionParser &parser);
}

#endif
