#ifndef EVALUATOR_BUILDER_H
#define EVALUATOR_BUILDER_H

#include "cached_builder.h"

class Evaluator;

// TODO: we could just use CachedBuilder<Evaluator> directly.
class EvaluatorBuilder : public CachedBuilder<Evaluator> {
};


#endif
