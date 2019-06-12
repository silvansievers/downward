#ifndef HEURISTIC_BUILDER_H
#define HEURISTIC_BUILDER_H

#include "evaluator_builder.h"

class Heuristic;

class HeuristicBuilder : public EvaluatorBuilder{
public:
    explicit HeuristicBuilder(const options::Options &opts);
    virtual ~HeuristicBuilder() = default;
};

extern void add_heuristic_options_to_parser(options::OptionParser &parser);

#endif
