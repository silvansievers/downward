#include "pattern_collection_generator_manual.h"

#include "validation.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../task_proxy.h"

#include "../utils/logging.h"

#include <iostream>

using namespace std;

namespace pdbs {
PatternCollectionGeneratorManual::PatternCollectionGeneratorManual(const Options &opts)
    : patterns(make_shared<PatternCollection>(opts.get_list<Pattern>("patterns"))) {
}

PatternCollectionInformation PatternCollectionGeneratorManual::generate(
    const shared_ptr<AbstractTask> &task) {
    TaskProxy task_proxy(*task);
    PatternCollectionInformation pci(task_proxy, patterns);
    validate_and_normalize_pattern_collection(task_proxy, pci);
    cout << "Manual pattern collection: " << pci.get_shared_patterns() << endl;
    return pci;
}

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.add_list_option<Pattern>(
        "patterns",
        "list of patterns (which are lists of variable numbers of the planning "
        "task).");

    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;

    return make_shared<PatternCollectionGeneratorManual>(opts);
}

static Plugin<PatternCollectionGenerator> _plugin("manual_patterns", _parse);
}
