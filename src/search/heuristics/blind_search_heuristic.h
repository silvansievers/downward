#ifndef HEURISTICS_BLIND_SEARCH_HEURISTIC_H
#define HEURISTICS_BLIND_SEARCH_HEURISTIC_H

#include "../heuristic.h"

namespace blind_search_heuristic {
class BlindSearchHeuristic : public Heuristic {
    int min_operator_cost;
protected:
    virtual int compute_heuristic(const GlobalState &global_state);
public:
    BlindSearchHeuristic(
        const std::shared_ptr<AbstractTask> &task,
        bool cache_estimates,
        const std::string &name);
    ~BlindSearchHeuristic();
};
}

#endif
