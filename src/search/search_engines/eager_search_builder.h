#ifndef SEARCH_ENGINES_EAGER_SEARCH_BUILDER_H
#define SEARCH_ENGINES_EAGER_SEARCH_BUILDER_H

#include "../search_engine_builder.h"

namespace eager_search {
class EagerSearchBuilder : public SearchEngineBuilder {
public:
    explicit EagerSearchBuilder(const options::Options &opts);

    virtual std::shared_ptr<SearchEngine> build() const override;
};

extern void add_options_to_parser(options::OptionParser &parser);
}

#endif
