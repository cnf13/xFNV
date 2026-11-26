#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

#include <cstdint>

class NiD3DShaderConstantMap;

class SpeedTreeShader {
public:
    // Adds the tree data entry at register auiCount and the 16 wind matrix registers after it to apConstantMap.
    static void SetShaderConstants(NiD3DShaderConstantMap* apConstantMap, uint32_t auiCount);

    static inline float pShaderData[4]{};
    static inline D3DXMATRIX pWindMatrices[4]{};
};
