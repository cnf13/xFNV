#pragma once

#include "BSShader/BSShaderLightingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"

class BSShaderTextureSet;
class BSTextureSet;
class Lighting30ShaderProperty;

int32_t IsADTOptPass(int32_t a1);

// The per-pixel lit shader property: texture sets (diffuse, normal, glow, height, environment, mask), landscape
// blending, refraction and parallax parameters.
class BSShaderPPLightingProperty : public BSShaderLightingProperty {
public:
	class TextureEffectData;
	class TangentSpaceData;

	BSShaderPPLightingProperty();
	~BSShaderPPLightingProperty() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void PostLinkObject(NiStream& kStream) override;
	bool SetupGeometry(NiGeometry* apGeometry) override;
	BSShaderProperty::RenderPassArray* GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) override;
	uint16_t GetNumberofPasses(NiGeometry* apGeometry) override;
	BSShaderProperty::RenderPass* GetRenderDepthPass(NiGeometry* apGeometry) override;
	BSShaderProperty* ClarifyShader(NiGeometry* apGeometry, bool abForceShader, bool abAllow30Shaders) override;
	NiTexture* GetBaseTexture() override { return spTextures[0][0]; }
	BSShaderProperty::RenderPassArray* GetWaterFogPassList(NiGeometry* apGeom) override;
	void PrecacheTextures() override;
	// BSShaderLightingProperty::CopyTo, then CopyToMembers
	virtual void CopyTo(BSShaderPPLightingProperty* pDest);
	virtual void CopyToMembers(BSShaderPPLightingProperty* pDest);
	virtual NiAdditionalGeometryData* CreateRendererSpecificProperty(NiGeometry* apGeometry);
	// The D3DTEXTUREADDRESS of the textures.
	virtual D3DTEXTUREADDRESS GetClampD3D() const { return D3DTADDRESS_WRAP; }
	virtual void SetClampD3D(D3DTEXTUREADDRESS aClampMode);
	virtual NiTexturingProperty::ClampMode GetClampNI() const { return TextureClampMode; }
	virtual void SetClampNI(NiTexturingProperty::ClampMode aClampMode) { TextureClampMode = aClampMode; }
	virtual uint16_t GetTextureCount() const { return iTextureCount; }
	virtual uint16_t QLandscapeTextureCount() const { return usLandscapeTextures; }
	// aeType: the texture kind (diffuse, normal, glow, height, environment, mask).
	virtual void SetTexture(BSShaderProperty::TextureTypeEnum aeType, uint32_t aiIndex, NiTexture* apTexture);

	virtual NiTexture* GetTexture(BSShaderProperty::TextureTypeEnum aeType, uint32_t aiIndex) {
		return spTextures[aeType][aiIndex];
	}

	virtual void SetTextures(BSShaderTextureSet* apTextureSet);
	virtual void SetDiffuseTexture(uint32_t aiIndex, NiTexture* apTexture);
	virtual void SetNormalTexture(uint32_t aiIndex, NiTexture* apTexture);
	virtual void SetGlowTexture(uint32_t aiIndex, NiTexture* apTexture);
	virtual void SetFlagsFromTextures();
	virtual float GetSpecularLODFade();
	virtual float GetEnvMapLODFade();
	// abUpdateList, here and in the pass helpers below, is an int32_t rather than a bool because the caller's whole
	// value is passed on unchanged: GetRenderPasses_1x and _2x pass their own abUpdateList as is to every helper, which
	// compares its low byte with 1.
	virtual void GetRenderPasses_1x(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList);
	virtual void GetRenderPasses_2x(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
		uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList);
	using BSShaderLightingProperty::CopyTo;
	using BSShaderLightingProperty::CopyToMembers;

	void CopyMembers(BSShaderPPLightingProperty* pDest, NiCloningProcess& arCloneData);
	// Stores the strength clamped to [0, 1] in fRefractionPower.
	void SetRefractionStrength(float fStrength);
	// Releases the first texture of every kind, then refills them from apTextureSet (spTextureSet if null).
	// apcfilename is unused.
	void LoadTextureSet(const char* apcfilename, BSTextureSet* apTextureSet);
	void SetTerrainTexParams(float afOffsetX, float afOffsetY, float afFade);
	void SetTexEffectData(TextureEffectData* apData);
	// Returns spTexEffectData.
	TextureEffectData* GetTextureEffectData();
	void SetLandscapeSpecularExponents(uint8_t aucDefSpec, uint8_t aucSpec0, uint8_t aucSpec1, uint8_t aucSpec2,
		uint8_t aucSpec3, uint8_t aucSpec4, uint8_t aucSpec5, uint8_t aucSpec6, uint8_t aucSpec7, uint8_t aucSpec8);
	Lighting30ShaderProperty* SetMaxParallaxPasses(float a2);
	void SetParallaxOccScale(float afParallaxOccScale);
	// Queues the silhouette pass of apGeom (landscape, LOD, or alpha-tested or plain unskinned geometry) and sets
	// the first pass's accumulation hint.
	void AddSilhouettePass_2x(NiGeometry* apGeom, bool abSkinned);
	NiTexture* GetDiffuseTextures(int32_t a2);
	NiTexture* GetNormalTexture(uint32_t aiIndex);
	void SetTextureSet(BSTextureSet* apTextureSet);
	void StoreExtraDataPointers(BSShaderPPLightingProperty::TangentSpaceData* aspTangentSpaceData);
	// spTangentSpaceData's object.
	BSShaderPPLightingProperty::TangentSpaceData* GetExtraDataPointers();
	// When apGeom's material emits light: marks the last queued pass as the last one, then creates the
	// self-illuminated alpha pass (abUpdateList) or counts it in asCount. abGlow is unused.
	BSShaderProperty::RenderPass* AddSelfIllumAlphaPass(NiGeometry* apGeom, uint16_t& asCount, int32_t abUpdateList,
		bool abSkinned, bool abGlow);
	// The 1.x pass helpers GetRenderPasses_1x calls: each adds its pass for apGeom to the pass list (abUpdateList) or
	// counts it in asCount.
	BSShaderProperty::RenderPass* AddZOnlyPass_1x(NiGeometry* apGeom, uint16_t& asCount, int32_t abUpdateList,
		bool& abFirst, bool abSkinned, bool abAlphaTest);
	BSShaderProperty::RenderPass* AddZOnlyTextureEffectPass_1x(NiGeometry* apGeom, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abSkinned);
	BSShaderProperty::RenderPass* AddAmbientPass_1x(NiGeometry* apGeom, ShadowSceneLight* apSunlight, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abSkinned, bool abAlphaTest, bool abGlowMap, bool abTreeBranch);
	BSShaderProperty::RenderPass* AddAmbDiffTexPass_1x(NiGeometry* apGeom, ShadowSceneLight* apSunlight,
		uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abSkinned, bool abAlphaTest, bool abGlowMap,
		bool abFacegenBlend, bool abFog, bool abVertexColors, bool abTreeBranch);
	BSShaderProperty::RenderPass* AddDiffusePass_1x(NiGeometry* apGeom, ShadowSceneLight* apLight, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abSkinned, bool abPointLight, bool abMultitexture, bool abTreeBranch);
	// abMultitexture is an int32_t, not a bool, because the caller's whole value is passed on and a bool would be
	// zero-extended (BSShaderProperty::Textures passes its int32_t `empty` as abMultitexture).
	BSShaderProperty::RenderPass* AddTexturePass_1x(NiGeometry* apGeom, ShadowSceneLight* apLight, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abSkinned, int32_t abMultitexture, bool abFacegenBlend,
		bool abVertexColors, bool abTreeBranch, bool abHair);
	BSShaderProperty::RenderPass* AddSpecularPass_1x(NiGeometry* apGeom, ShadowSceneLight* apLight, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abSkinned, bool abPointLight, bool abMultitexture,
		bool abBaseSpecular, bool abDecalSpecular, bool abTreeBranch);
	BSShaderProperty::RenderPass* AddEnvmapPass_1x(NiGeometry* apGeom, uint16_t& asCount, int32_t abUpdateList,
		bool& abFirst, bool abSkinned, bool abVertexColors);
	BSShaderProperty::RenderPass* AddFogPass_1x(NiGeometry* apGeom, uint16_t& asCount, int32_t abUpdateList,
		bool& abFirst, bool abSkinned, bool abTreeBranch, bool abAlphaBlend);
	// Returns void; its only caller, GetRenderPasses_1x, ignores any result. abUpdateList is int32_t as in the other
	// 1.x helpers, not bool.
	void AddLandscapePasses_1x(NiGeometry* apGeom, ShadowSceneLight* apSunlight, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abProjShadow);
	BSShaderProperty::RenderPass* AddTextureEffectPass_1x(NiGeometry* apGeom, uint16_t& asCount, int32_t abUpdateList,
		bool& abFirst, bool abSkinned);
	BSShaderProperty::RenderPass* AddRenderNormalsPass_1x(NiGeometry* apGeom, uint16_t& asCount, int32_t abUpdateList,
		bool& abFirst, bool abSkinned, bool abRefractionFire);
	BSShaderProperty::RenderPass* AddRenderNormalsClearPass_1x(NiGeometry* apGeom, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abSkinned);
	void UseDefaultNormalMap();
	int16_t GetUnk68();
	// Returns the pass it adds, on every path.
	BSShaderProperty::RenderPass* AddAmbDiffTexSpecular10Pass_2x(NiGeometry* apGeom, ShadowSceneLight* apSunlight,
		uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abSkinned, bool abGlowMap, bool abSpecular,
		bool abFaceGenBlend, bool abExtraLight);
	void AddLandscapePasses_2x(NiGeometry* apGeom, ShadowSceneLight* apSunlight, uint16_t& asCount,
		int32_t abUpdateList, bool& abFirst, bool abSpecular, bool abProjShadow);
	void FillTextureArray();
	void SetLandscapeTextureSet(uint32_t aiIndex, BSTextureSet* apTexSet);

	// The normal map texture of apShaderProperty at aiIndex.
	static NiSourceTexture* GetNormalMap(BSShaderPPLightingProperty* apShaderProperty, int32_t aiIndex);
	// Sets the refraction of the shader properties under apObject: the refraction flag, the power, the fire flag, the
	// fire period and the tint flag. The three flags are int32_t rather than bool because the caller's whole 32-bit
	// value is passed on; bool would be zero-extended.
	static void SetRefractionRecurse(NiAVObject* apObject, int32_t abEnable, float afRefractionPower, int32_t abFire,
		float afRefractionFirePeriod, int32_t abTinted);
	static bool GetForceNoProjectedShadowTexture();
	static void SetForceNoProjectedShadowTexture(bool abForce);
	// The class loader's factory: a new property, or null when the allocation fails.
	static NiObject* CreateObject();

	float fWaterDepthCameraOffset;
	float fGeomorphParam;
	NiColorA pHairTintColor;
	NiPoint4 LandBlendParamsA;
	NiPointer<BSShaderTextureSet> spTextureSet;
	uint16_t iTextureCount;
	uint8_t paddingAA[2];
	// per texture kind, an array of iTextureCount textures
	NiPointer<NiTexture>* spTextures[6];
	uint8_t* pSpecularExponent;
	uint16_t usLandscapeTextures;
	uint8_t paddingCA[2];
	bool* pbGlossMap;
	NiPointer<TangentSpaceData> spTangentSpaceData;
	float* pTextureSplatVertexData;
	NiTexturingProperty::ClampMode TextureClampMode;
	NiPointer<TextureEffectData> spTexEffectData;
	float fRefractionPower;
	int32_t iRefractionFirePeriod;
	float fParallaxOccMaxPasses;
	float fParallaxOccScale;
	float fTerrainTexOffsetX;
	float fTerrainTexOffsetY;
	float fTerrainTexFade;
	float fTerrainDetailTexScale;
	BSShaderProperty::RenderPass* pRenderDepthPass;

	static NiRTTI ms_RTTI;
	static inline bool bForceNoProjectedShadow;
};

// A rim/fill lighting texture effect applied to a subtree.
class BSShaderPPLightingProperty::TextureEffectData : public NiRefObject {
public:
	TextureEffectData();
	~TextureEffectData() override;

	NiPointer<NiTexture> spTexture;
	NiColorA CurrentFillColor;
	NiColorA CurrentRimColor;
	NiColorA FillColor;
	NiColorA RimColor;
	float fTextureUOffset;
	float fTextureVOffset;
	float fEdgeExponent;
	float fBoundDiameter;
	D3DBLEND eSrcBlendMode;
	D3DBLEND eDestBlendMode;
	D3DBLENDOP eBlendOperation;
	D3DCMPFUNC eZTestFunction;
	NiPointer<NiTexture> spBlockOutTexture;
	uint32_t iAlphaTest;
};

// The geometry's tangent and binormal arrays.
class BSShaderPPLightingProperty::TangentSpaceData : public NiRefObject {
public:
	TangentSpaceData(bool abOwnsMemory);
	~TangentSpaceData() override;

	bool bOwnsMemory;
	uint8_t padding09[3];
	NiPoint3* pTangentExtraData;
	NiPoint3* pBinormalExtraData;
};
