#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Reconstruction.hpp"

using SpeedTreeShader_pShaderData_t = float[4];
inline SpeedTreeShader_pShaderData_t SpeedTreeShader_pShaderData{};

using SpeedTreeShader_pWindMatrices_t = D3DXMATRIX[4];
inline SpeedTreeShader_pWindMatrices_t SpeedTreeShader_pWindMatrices{};
