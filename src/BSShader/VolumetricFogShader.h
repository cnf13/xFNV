#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiD3DPass;
class NiD3DPixelShader;
class NiD3DVertexShader;

// The volumetric fog shader: its passes, vertex and pixel shaders, and the fog constants it hands the shaders.
class VolumetricFogShader : public BSShader {
public:
	enum etPasses : int32_t {
		VFOG_PASS_FRONT = 0x0,
		VFOG_PASS_BACK = 0x1,
		VFOG_PASS_SINGLE = 0x2,
		VFOG_PASS_COUNT = 0x3,
	};

	enum etVertexShaderList : int32_t {
		VFOG_VS_FOG = 0x0,
		VFOG_VS_FOG_30 = 0x1,
		VFOG_VS_COUNT = 0x2,
	};

	enum etPixelShaderList : int32_t {
		VFOG_PS_FRONT_BACK = 0x0,
		VFOG_PS_FRONT_BACK_LINEARIZE = 0x1,
		VFOG_PS_SINGLE = 0x2,
		VFOG_PS_SINGLE_LINEARIZE = 0x3,
		VFOG_PS_COUNT = 0x4,
	};

	VolumetricFogShader();
	~VolumetricFogShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;
	virtual void LoadShaders();
	virtual void LoadVertexShaders();
	virtual void LoadPixelShaders();
	virtual bool LoadStagesAndPasses();

	static BSShader* CreateShader();

	NiPointer<NiD3DPass> spPass[3];
	NiPointer<NiD3DVertexShader> spVertexShader[2];
	// The PC build keeps only two pixel shaders, not VFOG_PS_COUNT (4); the destructor releases 2.
	NiPointer<NiD3DPixelShader> spPixelShader[2];
	// The "fvars0" shader constant.
	NiColorA fVars;
	// The "FogProps" shader constant.
	NiColorA fFogProps;

	static NiRTTI ms_RTTI;
	// Public so they can be defined. No code reads either: the initialisers only release the pointer at exit and copy
	// BSShaderManager::b30VFog into the flag.
	static NiPointer<BSRenderedTexture> pFogThicknessTexture;
	static bool bUseSinglePass;
};
