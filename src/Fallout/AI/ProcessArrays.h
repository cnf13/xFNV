#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class ProcessArrays {
public:
    // The mobile objects being processed this frame.
    NiTPrimitiveArray<void*> kMobileObjects;
    uint32_t uiBeginOffsets[4];
    uint32_t uiEndOffsets[4];
    uint32_t uiStoredBeginOffsets[4];
};
