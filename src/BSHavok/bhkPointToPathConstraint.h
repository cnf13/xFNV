#pragma once

#include "BSHavok/bhkConstraint.h"

// A constraint along a path. Only the RTTI record is declared.
class bhkPointToPathConstraint : public bhkConstraint {
public:
    static NiRTTI ms_RTTI;
};
