#pragma once

#include "Reconstruction.hpp"

// The flag a loaded object's finish constructor reads: whether the object is being finished after a load. The PDB
// names its functions in hkpEntity.cpp.
class hkFinishLoadedObjectFlag {
public:
	int32_t m_finishing;
};
