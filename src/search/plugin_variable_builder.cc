#include "plugin_variable_builder.h"

#include "evaluator.h"
#include "plugin.h"

#include "options/option_parser.h"

// HACK: probably not needed, see below
#include "search_engine_builder.h"
#include "pruning_method.h"
#include "open_list_factory.h"



using namespace std;

template<typename T>
static shared_ptr<PluginBuilder<T>> _parse(options::OptionParser &parser) {
    parser.document_synopsis("Variable",
                             "Use in combination with \"let\" to reuse objects");
    parser.add_option<string>("name", "Variable name");
    options::Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
        string name = opts.get<string>("name");
        if (name.empty()) {
            cerr << "Variable names must not be empty." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
        return make_shared<PluginVariableBuilder<T>>(name);
    }
}

static Plugin<PluginBuilder<Evaluator>> _plugin("var", _parse<Evaluator>);
