#ifndef EVALUATORS_COMBINING_EVALUATOR_BUILDER_H
#define EVALUATORS_COMBINING_EVALUATOR_BUILDER_H

#include "../plugin_builder.h"

#include <vector>

class Evaluator;

namespace combining_evaluator {
class CombiningEvaluatorBuilder : public PluginBuilder<Evaluator> {
protected:
    std::vector<std::shared_ptr<PluginBuilder<Evaluator>>> subevaluators;
public:
    CombiningEvaluatorBuilder(
        const std::vector<std::shared_ptr<PluginBuilder<Evaluator>>> &subevaluators_);
};
}

#endif
