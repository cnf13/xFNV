#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

// A node of Gamebryo's material graph. Only its RTTI record is declared; nothing of its layout is declared.
class NiMaterialNode : public NiRefObject {
public:
    static NiRTTI ms_RTTI;
};
