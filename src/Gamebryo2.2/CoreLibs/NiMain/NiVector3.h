#pragma once

#include "Reconstruction.hpp"

// Three floats, IDA's name for a plain vector; the PDB has no such class.
class NiVector3 {
public:
	float x;
	float y;
	float z;
};
