#pragma once

#include "Reconstruction.hpp"

// The settings of the multithreaded simulation. The PDB places its functions in hkpWorld.cpp.
struct hkpMultithreadConfig {
	hkpMultithreadConfig();

	int32_t m_maxNumConstraintsSolvedSingleThreaded;
};
