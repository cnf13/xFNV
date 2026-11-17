#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Reconstruction.hpp"

// The key a fragment material builds its shaders from: an identifier and a bit array of render options.
class NiMaterialDescriptor {
public:
	NiFixedString m_kIdentifier;
	uint32_t m_auiBitArray[4];
};
