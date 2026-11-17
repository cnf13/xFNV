#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"

// A renderer-side time controller. Only its RTTI record is declared; nothing of its layout is declared.
class NiD3DController : public NiTimeController {
public:
	static NiRTTI ms_RTTI;
};
