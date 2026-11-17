#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderRequirementDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

// A shader library's description: its requirements and the descriptions of the shaders it holds (Gamebryo's
// NiShaderLibraryDesc.h; the PDB's m_kShaderDescs at 0x38). The exe reads the array's base at +0x3C and its
// effective size at +0x44 (0xA9FFE0, 0xA9FFD0).
class NiShaderLibraryDesc : public NiShaderRequirementDesc {
public:
	// The description of shader uiShader.
	NiShaderDesc* GetShaderDesc(uint32_t uiShader) const;
	// The number of shader descriptions: the array's effective size.
	uint32_t GetShaderCount() const;

	NiTObjectArray<NiPointer<NiShaderDesc>> m_kShaderDescs;
};
