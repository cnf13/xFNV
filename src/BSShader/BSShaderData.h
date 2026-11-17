#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSMaterialEmittanceMultController;
class BSRefractionFirePeriodController;
class BSRefractionStrengthController;
class BSShaderNoLightingProperty;
class BSShaderPPLightingProperty;
class BSShaderProperty;
class BSShaderTextureSet;
class NiAVObject;
class NiAlphaController;
class NiMaterialColorController;
class SkyShaderProperty;
class TallGrassShaderProperty;
class TileShaderProperty;

// The settings of a Bethesda shader, converted into the shader, material and alpha properties of a geometry tree.
class BSShaderData {
public:
	enum LightingShaderEnum : int32_t {
		LS_STANDARD = 0,
		LS_SKIN = 1,
		LS_HAIR = 2,
		LS_WATER = 3,
		LS_NOLIGHTING = 4,
		LS_VFOG = 5,
		LS_SKY = 6,
		LS_TALLGRASS = 7,
		LS_TILE = 8,
		LS_NOFADER = 9,
		LS_COUNT = 10,
	};

	enum PropertyFlagEnum : int32_t {
		PROP_SHADER = 1,
		PROP_MATERIAL = 2,
		PROP_ALPHA = 4,
		PROP_ALL = -1,
	};

	// The shader index of each LightingShaderEnum; AddShaderProperty indexes it
	// by eLightingShader and stores the entry in BSShaderProperty::uiShaderIndex.
	using ShaderPropertyToShaderTable = uint32_t[LS_COUNT];

	BSShaderData();

	// Adds the properties auiProperties selects (PropertyFlagEnum bits) to apTarget's tree.
	void Convert(NiAVObject* apTarget, uint32_t auiProperties);
	void AddShaderProperty(NiAVObject* apTarget);
	void AddAlphaProperty(NiAVObject* apTarget);
	void AddMaterialProperty(NiAVObject* apTarget);
	// The shader property eLightingShader selects, new and empty; null for an unknown shader.
	BSShaderProperty* GetShaderProperty();
	void SetupTexturesTileShader(TileShaderProperty* pTileShaderProp);
	// Called SetupTexturesGrassShader by IDA.
	void SetupTexturesTallGrassShader(TallGrassShaderProperty* pTGShaderProp);
	void SetupTexturesSkyShader(SkyShaderProperty* pSkyShaderProp);
	void SetupTexturesPPLightingShader(BSShaderPPLightingProperty* apShaderProp);
	void SetupDataLightingShader(BSShaderPPLightingProperty* a2);
	void CheckTextureAlphas();
	void SetupShaderFlags(BSShaderProperty* apShaderProp, NiAVObject* apObject);
	void SetupTexturesNoLightingShader(BSShaderNoLightingProperty* pNLShaderProp);

	LightingShaderEnum eLightingShader;
	NiColor SpecularColor;
	NiColor EmittanceColor;
	float fShininess;
	float fAlpha;
	float fEmittanceMult;
	NiMaterialColorController* pSpecularController;
	NiMaterialColorController* pEmittanceController;
	NiAlphaController* pAlphaController;
	BSMaterialEmittanceMultController* pEmittanceMultController;
	bool bAlphaBlend;
	NiAlphaProperty::AlphaFunction eAlphaSrcBlendMode;
	NiAlphaProperty::AlphaFunction eAlphaDestBlendMode;
	bool bAlphaTest;
	NiAlphaProperty::TestFunction eAlphaTestFunction;
	uint8_t ucAlphaTestRef;
	bool bVertexAlpha;
	bool bModelSpaceNormals;
	bool bRemappableTextures;
	bool bDecal;
	bool bExternalEmittance;
	bool bHideSecret;
	bool bNoFade;
	bool bLightFade;
	bool bZBufferWrite;
	bool bZBufferTest;
	bool bRefraction;
	float fRefractionPower;
	bool bRefractionFire;
	int32_t iRefractionFirePeriod;
	BSRefractionStrengthController* pRefractionStrengthController;
	BSRefractionFirePeriodController* pRefractionFirePeriodController;
	bool bParallax;
	bool bParallaxOcclusion;
	float fParallaxOcclusionHieghtScale;
	float fParallaxOcclusionMaxPasses;
	NiColor VolumetricFogColor;
	float fVolumetricFogDensity;
	bool bEnvironmentMapping;
	bool bEnvironmentMappingEye;
	bool bEnvironmentMappingWindow;
	float fEnvironmentMappingMaskScale;
	// The sky object a sky shader draws (SkyShaderProperty's SO_ values).
	int32_t iSkyTYpe;
	bool bFalloffEnabled;
	NiColorA FalloffData;
	BSShaderTextureSet* pTextureSet;
	bool pbTextureHasAlpha[7];
	NiTexturingProperty::ClampMode eClampMode;

	static inline ShaderPropertyToShaderTable uiShaderPropertyToShader{1, 14, 1, 17, 33, 34, 10, 0, 32, 1};
};
