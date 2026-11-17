#pragma once

#include "Reconstruction.hpp"

// The fog a shader is given: its far and near distances and its power. IDA's names; the PDB has no such class.
class FogParameters {
public:
	float fDistFar;
	float fDistNear;
	float fPower;
	// IDA: float unk. The shaders store 0 here.
	float unk0C;
};
