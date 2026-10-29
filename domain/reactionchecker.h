#pragma once
#include "domain/trainingvariant.h"

struct ReactionCheckResult {
    bool accepted = false;
    QString explanation;
    int step = -1; // Zero-based first mismatch; -1 for structural errors.
};

class ReactionChecker {
  public:
    static ReactionCheckResult check(const TrainingVariant& task, const Automaton& candidate);
    static QStringList stateNames(int count);
};
