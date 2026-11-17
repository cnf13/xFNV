#pragma once

#include "Reconstruction.hpp"

// IDA name; not in the PDB. Three rows of four floats, the layout of an hkMatrix3 of three hkVector4 (no extra
// alignment: the exe addresses it as plain floats).
class hkMatrix3x4 {
public:
	float cr[3][4];
};
