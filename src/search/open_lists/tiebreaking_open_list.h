#ifndef OPEN_LISTS_TIEBREAKING_OPEN_LIST_H
#define OPEN_LISTS_TIEBREAKING_OPEN_LIST_H

#include "../open_list_factory.h"
#include "../option_parser_util.h"

class EvaluatorBuilder;

namespace tiebreaking_open_list {
class TieBreakingOpenListFactory : public OpenListFactory {
    Options options;
    const std::vector<std::shared_ptr<EvaluatorBuilder>> evaluator_builders;
    const bool allow_unsafe_pruning;
public:
    explicit TieBreakingOpenListFactory(const Options &opts);
    virtual ~TieBreakingOpenListFactory() override = default;

    virtual std::unique_ptr<StateOpenList> create_state_open_list(
        const std::shared_ptr<AbstractTask> &task) override;
    //virtual std::unique_ptr<EdgeOpenList> create_edge_open_list() override;
};
}

#endif
