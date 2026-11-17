#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/DistantLODGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Reconstruction.hpp"

class NiD3DPixelShader;
class NiD3DVertexShader;

class DistantLODShader : public BSShader {
public:
	DistantLODShader(NiDX9ShaderDeclaration* apShaderDecl);
	~DistantLODShader() override;

	const NiRTTI* GetRTTI() const override;
	void RecreateRendererData() override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;
	void SetCurrentPass(uint32_t aPass, bool aFirstPass) override;

protected:
	virtual void LoadShaders();
	// The PC override returns true (mov al, 1), so the slot returns bool.
	virtual bool LoadStagesAndPasses();

	void PresetStages();

public:
	// The Xbox fNumVertices member (float, after pEyePosition) is absent on PC: the class ends at 0x100 here, where
	// the Xbox type is 0x104.
	static BSShader* CreateShader();

	NiPointer<NiD3DPass> spPass;
	float fWindTimer;
	BSShaderManager::eBSSMPassEnum ePass;
	bool bFirstPass;
	uint8_t padding79[3];
	NiPointer<NiD3DVertexShader> spVertexShaders[4];
	NiPointer<NiD3DPixelShader> spPixelShaders[2];
	// iInstanceDataCount instances, allocated by the constructor, handed to the "instance data" entry.
	DistantLODGroup::DistantLODInstanceData* pDistantLODInstanceData;
	NiShaderConstantMapEntry* pDistantLODInstanceDataEntry;
	// PC stores a 16-bit instance count, unlike the Xbox 32-bit member.
	uint16_t iInstanceDataCount;
	uint8_t padding9E[2];
	NiColorA pfAmbientColor;
	NiPoint4 pfDiffuseDir;
	NiColorA pfDiffuseColor;
	NiColorA pfAlphaTestRef;
	NiPoint4 pEyeVector;
	NiPoint4 pEyePosition;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable DistantLODShader_vftable{};

// IDA name TESTerrainLODManager::kShaderParams; not in the PDB. The terrain LOD shader's two parameters (20480 and
// 32768 in the exe's image). The distant LOD vertex map's "alpha param param" entry reads 16 bytes from it, so
// that register also receives BSShaderManager::iLastEnabledPasses and the flags that follow it.
using TESTerrainLODManager_kShaderParams_t = float[2];
inline TESTerrainLODManager_kShaderParams_t TESTerrainLODManager_kShaderParams{20480.0f, 32768.0f};
