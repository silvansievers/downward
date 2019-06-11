#include "eager_search_builder.h"

using namespace std;

namespace eager_search {
EagerSearchBuilder::EagerSearchBuilder(const options::Options &opts)
    : SearchEngineBuilder(opts) {
}

unique_ptr<SearchEngine> EagerSearchBuilder::build() const {
    return utils::make_unique_ptr<EagerSearchBuilder>(opts);
}

void add_options_to_parser(options::OptionParser &parser) {
    add_search_pruning_option(parser);
    add_search_options_to_parser(parser);
}
}
