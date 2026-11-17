#pragma once

#include "Reconstruction.hpp"

// The world's settings for the collision agents' iterative linear casts. The PDB names its functions in
// hkpWorld.cpp; it names only the first two members, IDA types the rest as bytes.
class hkpCollisionAgentConfig {
public:
	float m_iterativeLinearCastEarlyOutDistance;
	int32_t m_iterativeLinearCastMaxIterations;
	uint8_t padding08[8];
};
