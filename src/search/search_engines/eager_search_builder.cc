#include "eager_search_builder.h"

#include "eager_search.h"

using namespace std;

namespace eager_search {
EagerSearchBuilder::EagerSearchBuilder(const options::Options &opts)
    : SearchEngineBuilder(opts) {
}

shared_ptr<SearchEngine> EagerSearchBuilder::build() const {
    return make_shared<EagerSearch>(opts);
}

void add_options_to_parser(options::OptionParser &parser) {
    add_search_pruning_option(parser);
    add_search_options_to_parser(parser);
}
}
