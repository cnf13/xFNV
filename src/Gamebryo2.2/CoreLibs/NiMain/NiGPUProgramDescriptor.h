#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"

// The key a fragment material builds one GPU program from: an identifier and a bit array of program options (Gamebryo's
// BIT_ARRAY_SIZE is 4).
class NiGPUProgramDescriptor {
public:
    NiFixedString m_kIdentifier;
    uint32_t m_auiBitArray[4];
};
