#pragma once

#include "Reconstruction.hpp"

class hkMemoryRouter;

// Havok memory state for an engine thread.
// The PDB has no source path; placed with the engine's Havok integration.
struct BSHavokThreadData {
	hkMemoryRouter* pHavokMemoryRouter;
	void* pHavokAlloc;
	uint32_t uiOwnerID;
};
