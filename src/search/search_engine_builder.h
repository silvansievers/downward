#ifndef SEARCH_ENGINE_BUILDER_H
#define SEARCH_ENGINE_BUILDER_H

#include "cached_builder.h"
#include "operator_cost.h"

#include "options/options.h"
#include "utils/logging.h"

#include <memory>

class AbstractTask;
class SearchEngine;

namespace options {
class OptionParser;
}

class SearchEngineBuilder : public CachedBuilder<SearchEngine> {
protected:
    const int bound;
    const double max_time;
    const OperatorCost cost_type;
    const utils::Verbosity verbosity;
    virtual std::shared_ptr<SearchEngine> build(
        const std::shared_ptr<AbstractTask> &task) const = 0;
public:
    explicit SearchEngineBuilder(
        int bound, double max_time, OperatorCost cost_type, utils::Verbosity verbosity);
};

extern void add_search_pruning_option(options::OptionParser &parser);
extern void add_search_options_to_parser(options::OptionParser &parser);
extern void add_search_succ_order_options(options::OptionParser &parser);

#endif
