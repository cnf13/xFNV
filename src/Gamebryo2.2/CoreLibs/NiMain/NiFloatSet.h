#pragma once

#include "Reconstruction.hpp"

// A growable array of floats with an explicit used count. The PDB has no record of the class and no source path for
// it; placed with the other NiMain containers.
class NiFloatSet {
public:
	// Reallocates to auiSize entries, copying the used ones across; auiSize 0 frees the block.
	void RemoveAll(uint32_t auiSize);

	float* m_pBase;
	uint32_t m_uiAlloced;
	uint32_t m_uiUsed;
};
