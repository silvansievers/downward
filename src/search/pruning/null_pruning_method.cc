#include "null_pruning_method.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../utils/logging.h"

using namespace std;

namespace null_pruning_method {
void NullPruningMethod::initialize(const shared_ptr<AbstractTask> &task) {
    PruningMethod::initialize(task);
    utils::g_log << "pruning method: none" << endl;
}
}
