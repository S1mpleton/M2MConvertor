#ifndef ISOMORPHISMCHECKER_H
#define ISOMORPHISMCHECKER_H

#include "domain/automaton.h"

struct IsomorphismResult {
    bool equivalent = false;
    QString explanation;
};

class IsomorphismChecker {
public:
    static IsomorphismResult compare(const Automaton& expected,
                                     const Automaton& actual);
};

#endif
