#include "plugin_let_builder.h"

#include "evaluator.h"
#include "plugin.h"
#include "search_engine.h"

#include "options/option_parser.h"

using namespace std;

template<typename VarType, typename ReturnType>
static shared_ptr<PluginBuilder<ReturnType>> _parse(options::OptionParser &parser) {
    parser.document_synopsis(
        "Variable Definition",
        "Use in combination with \"var\" to reuse objects");
    parser.add_option<string>("name", "Variable name");
    parser.add_option<shared_ptr<PluginBuilder<VarType>>>(
        "element_builder",
        "definition of the object stored in the variable");
    parser.add_option<shared_ptr<PluginBuilder<ReturnType>>>(
        "nested_builder",
        "expression that may use var(name) to refer to the defined variable");
    options::Options opts = parser.parse();
    if (parser.dry_run()) {
        return nullptr;
    } else {
        string name = opts.get<string>("name");
        if (name.empty()) {
            cerr << "Variable names must not be empty." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
        return make_shared<PluginLetBuilder<VarType, ReturnType>>(
            name,
            opts.get<shared_ptr<PluginBuilder<VarType>>>("element_builder"),
            opts.get<shared_ptr<PluginBuilder<ReturnType>>>("nested_builder"));
    }
}

static Plugin<PluginBuilder<SearchEngine>> _plugin("let", _parse<Evaluator, SearchEngine>);
