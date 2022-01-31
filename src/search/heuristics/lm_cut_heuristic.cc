#include "lm_cut_heuristic.h"

#include "lm_cut_landmarks.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include "../utils/memory.h"

#include <iostream>

using namespace std;

namespace lm_cut_heuristic {
LandmarkCutHeuristic::LandmarkCutHeuristic(
        const std::shared_ptr<AbstractTask> &task,
        bool cache_estimates,
        const std::string &name)
    : Heuristic(task, cache_estimates, name),
      landmark_generator(utils::make_unique_ptr<LandmarkCutLandmarks>(task_proxy)) {
    utils::g_log << "Initializing landmark cut heuristic..." << endl;
}

LandmarkCutHeuristic::~LandmarkCutHeuristic() {
}

int LandmarkCutHeuristic::compute_heuristic(const State &ancestor_state) {
    State state = convert_ancestor_state(ancestor_state);
    int total_cost = 0;
    bool dead_end = landmark_generator->compute_landmarks(
        state,
        [&total_cost](int cut_cost) {total_cost += cut_cost;},
        nullptr);

    if (dead_end)
        return DEAD_END;
    return total_cost;
}
}
