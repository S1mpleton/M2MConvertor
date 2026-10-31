#ifndef CONVERSIONSERVICE_H
#define CONVERSIONSERVICE_H

#include "domain/automaton.h"

class ConversionService {
public:
    static bool convert(const Automaton& source, Automaton* target,
                        QString* error = nullptr);

private:
    static bool mealyToMoore(const Automaton& source, Automaton* target,
                             QString* error);
    static bool mooreToMealy(const Automaton& source, Automaton* target,
                             QString* error);
};

#endif
