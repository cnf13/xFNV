#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"

#include <cstdint>

class NiD3DShaderConstantMap;

using SpeedTreeShader_pShaderData_t = float[4];
inline SpeedTreeShader_pShaderData_t SpeedTreeShader_pShaderData{};

using SpeedTreeShader_pWindMatrices_t = D3DXMATRIX[4];
inline SpeedTreeShader_pWindMatrices_t SpeedTreeShader_pWindMatrices{};

// Adds the tree data entry at register auiCount and the 16 wind matrix registers after it to apConstantMap.
void SpeedTreeShader_SetShaderConstants(NiD3DShaderConstantMap* apConstantMap, uint32_t auiCount);
