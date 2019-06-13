#include "blind_search_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"

#include "../task_utils/task_properties.h"

#include <cstddef>
#include <limits>
#include <utility>

using namespace std;

namespace blind_search_heuristic {
BlindSearchHeuristic::BlindSearchHeuristic(
    const shared_ptr<AbstractTask> &task, bool cache_estimates, const string &name)
    : Heuristic(task, cache_estimates, name),
      min_operator_cost(task_properties::get_min_operator_cost(task_proxy)) {
    cout << "Initializing blind search heuristic..." << endl;
}

BlindSearchHeuristic::~BlindSearchHeuristic() {
}

int BlindSearchHeuristic::compute_heuristic(const GlobalState &global_state) {
    State state = convert_global_state(global_state);
    if (task_properties::is_goal_state(task_proxy, state))
        return 0;
    else
        return min_operator_cost;
}
}
