#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgramCache.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The Direct3D program cache: compiles a fragment material's programs for one shader profile.
// Only the members called so far; the rest is not reconstructed yet.
class NiD3DGPUProgramCache : public NiGPUProgramCache {
public:
	NiD3DGPUProgramCache(uint32_t auiVersion, const char* apcWorkingDir, NiGPUProgram::ProgramType aeType,
		const NiFixedString& akShaderProfile, const char* apcMaterialIdentifier, int32_t abAutoWriteToDisk,
		int32_t abWriteDebugFile, int32_t abLocked, int32_t abLoad);
	~NiD3DGPUProgramCache() override;

	NiGPUProgram* GenerateProgram(const char* pcName, const char* pcProgramText,
		NiTObjectPtrSet<NiPointer<NiMaterialResource>>& kUniforms) override;
	// "hlsl".
	const char* GetPlatformSpecificCodeID() const override;
	const char* GetPlatform() const override;

protected:
	bool SaveGPUProgram(NiBinaryStream& kStream, NiGPUProgram* pkProgram) override;
	bool LoadGPUProgram(NiBinaryStream& kStream, const NiFixedString& kName, NiGPUProgram*& pkProgram,
		bool bSkip) override;
	virtual bool SaveDebugHLSLFile(const char* pcFilename, const char* pcProgramText);

public:
	// The exe's fields of the derived class (Gamebryo's members of the same names; public here only so the layout
	// pins below can name them).
	NiGPUProgram::ProgramType m_eType;
	uint8_t m_bWriteDebugHLSLFile;
	NiFixedString m_kMaterialIdentifier;
};
inline constexpr fnvd::Vtable NiD3DGPUProgramCache_vftable{};

// The first pixel shader target (the exe's own string, text and NUL), whose address the constructor compares with the
// profile's handle. IDA name; not in the PDB.
using PixelTargetText = const char[7];
inline PixelTargetText aPs11{"ps_1_1"};
