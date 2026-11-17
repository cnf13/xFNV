#pragma once

#include "BSHavok/bhkConstraint.h"
#include "Reconstruction.hpp"

// A constraint along a path (PDB: bhkPointToPathConstraint : bhkConstraint). Only the RTTI record is declared: src has
// none of the class's code yet.
class bhkPointToPathConstraint : public bhkConstraint {
public:
	static NiRTTI ms_RTTI;
};
