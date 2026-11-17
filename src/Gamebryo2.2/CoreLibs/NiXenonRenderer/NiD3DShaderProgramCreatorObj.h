#pragma once

#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderProgramCreatorAsm.h"
#include "Reconstruction.hpp"

// The shader program creator for precompiled shader objects (.vso and .pso files); registered with
// NiD3DShaderProgramFactory by _SDMInit. It adds no members to NiD3DShaderProgramCreatorAsm and overrides the two
// LoadShaderCode virtuals (IDA names them Func0009 and Func000A).
class NiD3DShaderProgramCreatorObj : public NiD3DShaderProgramCreatorAsm {
public:
	NiD3DShaderProgramCreatorObj();
	~NiD3DShaderProgramCreatorObj() override;

	// The exe passes the caller's whole dword on; bool would be zero-extended. PDB: bool bRecoverable.
	bool LoadShaderCodeFromFile(const char* pcFileName, void*& pvCode, uint32_t& uiCodeSize, void*& pvConstantData,
		uint32_t& uiConstantSize, int32_t bRecoverable) override;
	bool LoadShaderCodeFromBuffer(const void* pvBuffer, uint32_t uiBufferSize, void*& pvCode, uint32_t& uiCodeSize,
		void*& pvConstantData, uint32_t& uiConstantSize, int32_t bRecoverable) override;

	// Creates the creator instance and registers it for "vso" and "pso".
	static void _SDMInit();
};
inline constexpr fnvd::Vtable NiD3DShaderProgramCreatorObj_vftable{};
