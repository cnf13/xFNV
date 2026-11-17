#pragma once

#include "BSShader/ShadowLightShader.h"

class HairShader : public ShadowLightShader {
public:
	HairShader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apSkinnedShaderDecl);
	~HairShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	void RecreateRendererData() override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;

protected:
	void LoadVertexShaders() override;
	void LoadPixelShaders() override {}
	bool LoadStagesAndPasses() override;

public:
	virtual int32_t GetShaderType();

	static BSShader* CreateShader();

	NiPointer<NiD3DPass> spHairPass[2];
	NiPointer<NiD3DVertexShader> spHairVertexShader[7];
	NiPointer<NiD3DPixelShader> spHairPixelShader[3];
	NiPointer<NiD3DVertexShader> spHairVertexShader2[7];
	NiPointer<NiD3DPixelShader> spHairPixelShader2[3];
	NiPointer<NiDX9ShaderDeclaration> spShaderDecl;
	NiPointer<NiDX9ShaderDeclaration> spSkinnedShaderDecl;

	static NiRTTI ms_RTTI;
};
