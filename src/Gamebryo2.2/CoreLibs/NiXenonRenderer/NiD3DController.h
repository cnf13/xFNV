#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

// A renderer-side time controller (the PDB's class, derived from NiTimeController). The exe holds only its RTTI
// record: no vtable and no code of the class, so nothing of its layout is declared.
class NiD3DController : public NiTimeController {
public:
	static NiRTTI ms_RTTI;
};
