#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Reconstruction.hpp"

// A node of Gamebryo's material graph (the PDB's class, derived from NiRefObject). The exe holds only its RTTI
// record: no vtable and no code of the class, so nothing of its layout is declared.
class NiMaterialNode : public NiRefObject {
public:
	static NiRTTI ms_RTTI;
};
