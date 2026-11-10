#ifndef EQUIVALENCECHECKER_H
#define EQUIVALENCECHECKER_H

#include "domain/automaton.h"

struct EquivalenceResult {
    bool equivalent = false;
    QString explanation;
};

class EquivalenceChecker {
public:
    // Moore outputs are observed after consuming an input, on the destination
    // state. The initial Moore output is therefore not part of the compared
    // reaction. State counts and unreachable states may differ.
    static EquivalenceResult compare(const Automaton& first,
                                     const Automaton& second);

private:
    static Automaton reactionModel(const Automaton& source);
    static Automaton minimizedReachable(const Automaton& source);
};

#endif
