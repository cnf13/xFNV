#pragma once

#include "Reconstruction.hpp"

// Four floats; IDA-only (the PDB has no such class).
class NiVector4 {
public:
	float x;
	float y;
	float z;
	float w;
};
