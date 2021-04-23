#ifndef PLUGIN_VARIABLES_H
#define PLUGIN_VARIABLES_H

#include "utils/system.h"
#include "options/any.h"
#include "plugin_builder.h"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class AbstractTask;

class PluginVariableAssignment {
    // Map variable names to currently stored values.
    std::unordered_map<std::string, options::Any> storage;
    /*
      List of the pushed variable names, together with its shadowed value.
      Example:
        context.push("x", 42); // storage == {x: 42}; stack == [(x, Any())]
        context.push("x", 23); // storage == {x: 23}; stack == [(x, Any()), (x, 42)]
        context.push("x", 5);  // storage == {x: 5};  stack == [(x, Any()), (x, 42), (x, 23)]
        context.pop();         // storage == {x: 23}; stack == [(x, Any()), (x, 42)]
    */
    std::vector<std::pair<std::string, options::Any>> stack;
public:
    template<typename T>
    void push(const std::string &key, const std::shared_ptr<PluginBuilder<T>> &value) {
        options::Any previous_value;
        const auto it = storage.find(key);
        if (it != storage.end()) {
            previous_value = std::move(it->second);
        }
        stack.push_back(std::make_pair(key, std::move(previous_value)));
        storage[key] = value;
    }

    void pop() {
        const auto &removed = stack.back();
        std::string removed_key = removed.first;
        options::Any shadowed_value = std::move(removed.second);
        stack.pop_back();
        if (shadowed_value.type() == typeid(void)) {
            storage.erase(removed_key);
        } else {
            storage[removed_key] = std::move(shadowed_value);
        }
    }

    template<typename T>
    std::shared_ptr<T> get(const std::string &key,
                           const std::shared_ptr<AbstractTask> &task) {
        const auto it = storage.find(key);
        if (it == storage.end()) {
            std::cerr << "The command string uses a variable '" << key
                      << "' but that variable is not defined."
                      << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
        try {
            std::shared_ptr<PluginBuilder<T>> builder =
                options::any_cast<std::shared_ptr<PluginBuilder<T>>>(it->second);
            /*
              HACK: this should not rebuild if we already created an element
              for this variable and task. Also, passing *this here is wrong.
              We instead want to use the variable assignment at the point
              of time where the variable was defined, not where it is used.
              Also, this method should be const and passing *this prevents this.
            */
            return builder->build(*this, task);
        } catch (const options::BadAnyCast &) {
            std::cerr << "The command string contains a variable '" << key
                      << "' which which has the wrong type."
                      << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }
};

#endif
