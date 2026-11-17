#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

// Havok's log sink (the PDB's hkLogger : hkReferencedObject). Only what the reconstructed code uses is declared: its
// owners delete it through its virtual destructor (slot 0).
class hkLogger : public hkReferencedObject {
public:
	~hkLogger() override;
};
