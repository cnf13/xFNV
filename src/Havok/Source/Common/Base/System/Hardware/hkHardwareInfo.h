#pragma once

#include "Reconstruction.hpp"

// Reports the available hardware thread count.
// The PDB has no source path; placed with Havok's base system hardware utilities.
class hkHardwareInfo {
public:
	int32_t m_numThreads;
};
