#pragma once

#include "BSShader/ShadowLightShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Reconstruction.hpp"

// Does something to the map with a count;
// SpeedTreeBranchShader::CreateConstantMaps passes its vertex map and 0x2A.
// Inferred type: apConstantMap: NiD3DShaderConstantMap* from 0xbb2f01 (NiPointer<NiD3DShaderConstantMap>::m_pObject
// (this of SpeedTreeLeafShader::CreateConstantMaps)), its members used at 0xbb2f55
void sub_BB2F50(NiD3DShaderConstantMap* apConstantMap, uint32_t auiCount);

class SpeedTreeBranchShader : public ShadowLightShader {
public:
	// IDA name; not in the PDB. NiPointers: the exe passes each entry to NiD3DPass::CreateNewPass(NiPointer&)
	// (0xC16350) and releases them with the inlined NiPointer code (0xBD46A0).
	using m_kPasses_t = NiPointer<NiD3DPass>[30];

	SpeedTreeBranchShader(NiDX9ShaderDeclaration* apShaderDecl);
	~SpeedTreeBranchShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void SetupGeometryConstants(const NiPropertyState* apState) override;
	void PostGeometry(const NiPropertyState* apState) override {}
	void InitPass(uint32_t auiCurrentPass) override;
	void SetupPassTextures(uint32_t auiCurrentPass) override {}
	void SetupPassConstants(uint32_t auiCurrentPass) override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;

protected:
	void LoadVertexShaders() override;
	void LoadPixelShaders() override;
	bool LoadStagesAndPasses() override;
	void UpdateTogglesConstant(NiGeometry*, BSFogProperty*, NiMaterialProperty*) override;

public:
	virtual void EnableConstantsForPass(uint16_t);

protected:
	virtual void PresetPasses_1x();
	virtual void PresetPasses_2x();

public:
	static BSShader* CreateShader();

	NiPointer<NiD3DVertexShader> pVertexShader[30];
	NiPointer<NiD3DPixelShader> pPixelShader[10];

	static NiRTTI ms_RTTI;
	static m_kPasses_t m_kPasses;
};
inline constexpr fnvd::Vtable SpeedTreeBranchShader_vftable{};
