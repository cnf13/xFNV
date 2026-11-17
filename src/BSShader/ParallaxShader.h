#pragma once

#include "BSShader/ShadowLightShader.h"
#include "Reconstruction.hpp"

// The exe's pass table: ClearPassStages (0xBCCE10) walks 0x1202230 up to 0x12022B4.
using ParallaxShaderPassArray = NiPointer<NiD3DPass>[33];

class ParallaxShader : public ShadowLightShader {
public:
	ParallaxShader(NiDX9ShaderDeclaration* apShaderDecl);
	~ParallaxShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void RecreateRendererData() override;
	void InitPass(uint32_t auiCurrentPass) override;
	void SetupPassTextures(uint32_t auiCurrentPass) override;
	using ShadowLightShader::GetShaderDecl;
	NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;

protected:
	void LoadVertexShaders() override;
	void LoadPixelShaders() override;
	bool LoadStagesAndPasses() override;
	virtual void PresetPasses();
	virtual void PresetADPasses_2x();
	virtual void PresetADTPasses_2x();
	virtual void PresetADTSPasses_2x();
	virtual void PresetDiffusePtPasses_2x();
	virtual void PresetTexturePasses_2x();
	virtual void PresetSpecularPasses_2x();

public:
	static BSShader* CreateShader();

	NiD3DVertexShader* pVertexShader[20];
	NiD3DPixelShader* pPixelShader[33];

	static NiRTTI ms_RTTI;
	static ParallaxShaderPassArray spPass;
};
inline constexpr fnvd::Vtable ParallaxShader_vftable{};
