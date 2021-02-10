#ifndef PLUGIN_VARIABLES_H
#define PLUGIN_VARIABLES_H

#include "utils/system.h"
#include "options/any.h"

#include <iostream>
#include <string>
#include <unordered_map>

// TODO: reduce the overlap with options::Options?
class PluginVariables {
    std::unordered_map<std::string, options::Any> storage;
public:
    template<typename T>
    void set(const std::string &key, T value) {
        storage[key] = value;
    }

    template<typename T>
    T get(const std::string &key) const {
        const auto it = storage.find(key);
        if (it == storage.end()) {
            std::cerr << "The command string contains a variable '" << key
                      << "' but there was no predifition for this key."
                      << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
        try {
            T result = options::any_cast<T>(it->second);
            return result;
        } catch (const options::BadAnyCast &) {
            std::cerr << "The command string contains a variable '" << key
                      << "' which has a predifition with the wrong type."
                      << std::endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }
};

#endif
