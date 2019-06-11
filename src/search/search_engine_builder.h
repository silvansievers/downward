#ifndef SEARCH_ENGINE_BUILDER_H
#define SEARCH_ENGINE_BUILDER_H

#include "search_engine.h"

#include "options/options.h"

#include <memory>

namespace options {
class OptionParser;
}

class SearchEngineBuilder {
    const options::Options opts;
public:
    explicit SearchEngineBuilder(const options::Options &opts);
    virtual ~SearchEngineBuilder() = default;

    virtual std::unique_ptr<SearchEngine> build() const = 0;
};

extern void add_search_pruning_option(options::OptionParser &parser);
extern void add_search_options_to_parser(options::OptionParser &parser);
extern void add_search_succ_order_options(options::OptionParser &parser);

#endif
