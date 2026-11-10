#pragma once
#include "domain/trainingvariant.h"

struct ReactionCheckResult {
    bool accepted = false;
    Result result;
    int step = -1;
    ReactionCheckResult(bool accepted, const Message& message, int step = -1)
        : accepted(accepted), result(accepted ? Result::success(message, ResultCategory::Validation)
                                             : Result::error(message, ResultCategory::Validation)), step(step) {}
};

class ReactionChecker {
  public:
    static ReactionCheckResult check(const TrainingVariant& task, const Automaton& candidate,
                                     bool graph = false);
    static QStringList stateNames(int count);
};
