#pragma once

#include "BSShader/BSShader.h"
#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"

#include <stdint.h>

class BSFogProperty;
class NiDX9ShaderDeclaration;
class NiMaterialProperty;
class NiSkinInstance;

double fMin_(double a1, double a2);

using ShadowLightShaderMapEntries = NiShaderConstantMapEntry* [22];
using ShadowLightShaderPixelShaderArray = NiD3DPixelShader* [208];
using ShadowLightShaderVertexShaderArray = NiD3DVertexShader* [156];
using ShadowLightShaderMatrixRows = NiVector4[4];
// The pass table that ClearPassStages walks.
using ShadowLightShaderPassArray = NiPointer<NiD3DPass>[759];
// The per-pass constant flags, one entry per pass (759).
using ShadowLightShaderConstantFlags = uint32_t[759];

class ShadowLightShader : public BSShader {
public:
	// The previous frame's bone matrices (216 floats, the 0x360 bytes Lighting30Shader's PrevBoneMatrix3 entry maps)
	// and transforms.
	using PrevBonesArray = float[216];
	using LightDataArray = NiColorA[10];
	using uiPixelNonVolatileConstantFlags_t = int32_t[759];
	using uiVertexNonVolatileConstantFlags_t = int32_t[759];

	ShadowLightShader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apSkinnedShaderDecl,
		NiDX9ShaderDeclaration* apModelSpaceNormShaderDecl, NiDX9ShaderDeclaration* apSplatTexturingShaderDecl);
	~ShadowLightShader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	uint32_t SetupBoneMatrices(NiGeometry*, const NiSkinInstance*, const NiSkinPartition::Partition*,
		NiGeometryData::RendererData*, const NiPropertyState*, const NiDynamicEffectState*, const NiTransform&,
		const NiBound&) override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void SetupGeometryConstants(const NiPropertyState* apState) override;
	void PostGeometry(const NiPropertyState* apState) override;
	void RecreateRendererData() override;
	void InitPass(uint32_t auiCurrentPass) override;
	void SetupPassTextures(uint32_t auiCurrentPass) override;
	void SetupPassConstants(uint32_t auiCurrentPass) override;
	using BSShader::GetShaderDecl;
	NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;
	virtual void UpdateDiffuseNormalStages(uint32_t auiCurrentPass, BSShaderPPLightingProperty* apProp);
	virtual void UpdatePassTextureStages(uint32_t auiPassEnum, BSShaderPPLightingProperty* apL30Prop) {}

protected:
	virtual void LoadShaders();
	virtual void LoadVertexShaders();
	virtual void LoadPixelShaders();
	virtual bool LoadStagesAndPasses();
	virtual void UpdateTogglesConstant(NiGeometry*, BSFogProperty*, NiMaterialProperty*);

public:
	// The cube map of apShaderProp's texture slot aiTextureNumber. aiTextureNumber is only passed on as GetTexture's
	// unsigned index.
	NiSourceTexture* GetCubeMap(BSShaderPPLightingProperty* apShaderProp, uint32_t aiTextureNumber);
	// Returns spSplatTexturingShaderDecl.
	NiDX9ShaderDeclaration* GetLandShaderDecl();
	void SetupPassOpt_AmbientLight();
	// The scene graph's fog as vertex shader constants c14-c15.
	void SetupPassOpt_FogConstants();
	// Prepares the pixel shader constants.
	void SetupPassOpt_PreparePixelShaderConstants();
	// The pass's light colours.
	void SetupPassOpt_LightColor();
	void SetupPass_ADT4_Opt();
	void SetClampMode(NiD3DTextureStage* textureStage, BSShaderPPLightingProperty* shaderProp);
	// Returns the default shader declaration, spShaderDecl's object.
	NiDX9ShaderDeclaration* GetDefaultShaderDecl();
	void SetDiffuseMap(BSShaderPPLightingProperty* pprop, uint32_t auiStage, uint32_t auiIndex);
	void SetNormalMap(BSShaderPPLightingProperty* pprop, uint32_t auiStage, uint32_t auiIndex);
	void SetShadowMap(uint32_t auiStage);
	void SetProjectedShadowMap(uint32_t auiStage);
	void SetGlowMap(BSShaderPPLightingProperty* pprop, uint32_t auiStage, uint32_t auiIndex, bool abUseDefaultOnNULL);
	void SetCubeMap(BSShaderPPLightingProperty* pShaderProp, int32_t uiTexturePass, int32_t uiTextureNumber);
	void SetProjectedShadowMaskMap(uint32_t auiStage);
	void UpdatePipelineTexEffect(NiGeometry* apGeometry, BSShaderPPLightingProperty* apBSSProp,
		BSShaderProperty::RenderPass* apPass);
	// The texture effect's shader constants.
	void UpdateTexEffectConstants(NiGeometry* apGeometry, BSShaderPPLightingProperty* apProp);
	void SetupGeometryConstants_RefractionPower(BSShaderPPLightingProperty* pprop);
	void SetupGeometryConstants_WorldBoundCenter(NiGeometry* pgeometry);
	void SetEnvMask(BSShaderPPLightingProperty* pShaderProp, int32_t uiTexturePass, int32_t uiTextureNumber);
	void UpdatePipelineZOnlyTexEffect(NiGeometry* apGeometry, BSShaderPPLightingProperty* apBSSProp,
		BSShaderProperty::RenderPass* apPass);
	void DisableNonVolatileConstantsForPass(uint32_t auiPassEnum);
	void SetupGeometryConstants_ObjToCube(BSShaderProperty::RenderPass* apRenderPass, uint32_t epass,
		const NiSkinInstance* apSkin, const NiTransform& aWorld);
	void SetupGeometryConstants_SimpleShadowClipPlanes(uint32_t epass);
	void SetupGeometryOpt_WorldViewProj(NiGeometry* apGeometry, const NiTransform& aWorld);
	void Setup_ADT_Opt(NiPropertyState* a1);
	void Setup_ADT4_Opt(NiPropertyState* a1);

protected:
	void UpdateTogglesConstantLand(NiGeometry* apGeometry, BSFogProperty* apFog,
		BSShaderPPLightingProperty* apShaderProp, int32_t aiTextureIndex);

public:
	void SetupGeometryConstants_Lights(BSShaderPPLightingProperty* pprop, BSShaderProperty::RenderPass* apRenderPass,
		_D3DMATRIX d3dInvWorld, const NiTransform& aWorld, uint32_t epass, const NiSkinInstance* apSkin);
	void SetupGeometryConstants_EyePos(uint32_t epass, const NiTransform& aWorld, _D3DMATRIX d3dInvWorld);
	void SetupGeometryConstants_SkinToCube(const NiSkinInstance* apSkin);
	void SetupGeometryConstants_RefractionTime(BSShaderPPLightingProperty* pprop);
	void SetupGeometryConstants_HighDetailRange(NiGeometry* apGeometry);
	void SetupGeometryConstants_LODLandParams(uint32_t epass, BSShaderPPLightingProperty* pprop, NiGeometry* pgeometry);
	void SetupGeometryConstants_LandTextureMask(uint32_t epass, BSShaderProperty::RenderPass* apRenderPass);
	void SetupGeometryConstants_Toggles(uint32_t epass, NiGeometry* pgeometry, BSShaderPPLightingProperty* pprop,
		NiMaterialProperty* pmaterialProperty, BSShaderProperty::RenderPass* apRenderPass, NiAlphaProperty* palphaProp);
	void SetupGeometryConstants_LODTexParams(BSShaderPPLightingProperty* pprop);
	void SetupGeometryConstants_ShadowProj(const NiPropertyState* apState, const NiSkinInstance* apSkin,
		const NiTransform& aWorld);
	void SetupGeometryConstants_AmbientEmittance(BSShaderPPLightingProperty* pprop,
		BSShaderProperty::RenderPass* apRenderPass, NiGeometry* pgeometry, NiMaterialProperty* pmat);
	void SetLODNoiseMap(int32_t auiTexturePass);
	// a3 is only passed on as GetTexture's unsigned index.
	void SetParallaxTexture(BSShaderPPLightingProperty* a1, int32_t passStage, uint32_t a3, char a4);
	void SetupGeometryOpt_Lights(_D3DMATRIX& aInvworld, float afWorldScale);
	void SetupGeometryOpt_LightsSpecular(_D3DMATRIX& aInvworld, float afWorldScale, NiMaterialProperty* apMat);
	void SetupPass_ADT_Opt();
	void Setup_ADTS10_Opt(NiPropertyState* a2);
	void Setup_ADT_PxOpt(NiPropertyState* apProperties);
	void PresetAmbientPasses_1x();
	void PresetAmbDiffTexPasses_1x();
	void PresetAmbDiffDirAndPtPasses_1x();
	void PresetDiffuseLightPasses_1x();
	void PresetTexturePasses_1x();
	void PresetSpecularPasses_1x();
	void PresetEnvMapPasses_1x();
	void PresetFogPasses_1x();
	void PresetLandscape_1x();
	void PresetTextureEffectPasses_1x();
	void PresetZOnlyPasses_1x();
	void PresetConstAlphaPasses_1x();
	void PresetADTPasses_2x();
	void PresetADTSPasses_2x();
	void PresetADPasses_2x();
	void PresetDiffusePtPasses_2x();
	void PresetSpecularPasses_2x();
	void PresetEnvMapPasses_2x();
	void PresetLandscape_2x();
	void PresetTextureEffectPasses_2x();
	void PresetRenderNormalsPasses_2x();
	void PresetWaterFogPasses_2x();
	void AddEmitToAmbient(NiMaterialProperty* apMat);
	void SetEmitConstant(NiMaterialProperty* apMat);

	// The projected shadow's translation, into the shadow projection transform's x and y.
	static void SetProjectedShadowTranslate(NiPoint2 aTranslate);
	// Compares aiWhich unsigned and returns the table's pointer without adding a reference, or nullptr past the end of
	// the table.
	static NiD3DPixelShader* GetPixelShader(int32_t aiWhich);
	static void SetProjectedShadowScale(uint32_t aiScale);
	static uint32_t GetProjectedShadowScale();
	static void SetProjectedShadowMaskWidth(float afWidth);
	// Stores aLL, the lower-left corner of the projected shadow mask, in kShadowProjData's x and y.
	static void SetProjectedShadowMaskLL(NiPoint2 aLL);
	static void UpdateFarNear();
	// Sets up the three 2.x depth passes (487-489): their vertex and pixel shaders, the depth-only render states and
	// the passes' constant flags.
	static void PresetDepthPasses_2x();
	// Enables the shader constants pass auiPassEnum uses.
	static void EnableConstantsForPass(uint32_t auiPassEnum, bool abAlphaPass);
	// Set up the velocity passes (490, 491) and the local map passes (492 to 494): shaders, render states and
	// constant flags.
	static void PresetVelocityPasses_2x();
	static void PresetLocalMapPasses_2x();
	static void EnableConstantsForPass_1x(uint32_t auiPassEnum);
	static void EnableConstantsForPass_2x(uint32_t auiPassEnum);
	// Reads no member, so it is static.
	static void SetupGeometryConstants_WaterFog();
	static BSShader* CreateShader();

	NiPointer<NiDX9ShaderDeclaration> spShaderDecl;
	NiPointer<NiDX9ShaderDeclaration> spSkinnedShaderDecl;
	NiPointer<NiDX9ShaderDeclaration> spModelSpaceNormShaderDecl;
	NiPointer<NiDX9ShaderDeclaration> spSplatTexturingShaderDecl;
	NiPointer<NiD3DShaderConstantMap> spTexEffectPConstantMap;
	NiPointer<NiD3DShaderConstantMap> spTexEffectVConstantMap;
	NiPointer<NiD3DShaderConstantMap> spDefaultPixelConstantMap;
	NiPointer<NiD3DShaderConstantMap> spDefaultVertexConstantMap;

	static NiRTTI ms_RTTI;
	static inline NiVector4 kShadowProjTransform{};
	static inline float fConstantfRefractionTime;
	static inline ShadowLightShaderMapEntries spVertexConstantMapEntries;
	static inline ShadowLightShaderMapEntries spPixelConstantMapEntries;
	static inline NiVector4 kRefractionParams{};
	static inline NiVector4 kShadowProjData{};
	static inline NiVector4 kWorldBoundCenter{};
	static inline float fTextureUOffset;
	static inline float fTextureVOffset;
	static inline NiVector4 kFogParams{};
	static ShadowLightShaderPassArray spPass;
	static inline NiVector4 kFogPlane{};
	static inline NiVector4 kFillColor{};
	static inline NiVector4 kRimColor{};
	static inline NiVector4 fVars{};
	static inline NiVector4 kTexEffectFogColor{};
	static inline NiVector4 kTexEffectFogParam{};
	static inline NiVector4 kFarNear{};
	// The pixel and vertex shader tables (208 and 156 entries, the counts RecreateRendererData walks).
	static inline ShadowLightShaderPixelShaderArray spPixelShaders;
	static inline ShadowLightShaderVertexShaderArray spVertexShaders;
	static inline ShadowLightShaderMatrixRows kSkinWorldTranspose{};
	static inline ShadowLightShaderConstantFlags ulPixelConstantFlags{};
	static inline ShadowLightShaderConstantFlags ulVertexConstantFlags{};
	static inline PrevBonesArray kPrevBones{};
	static inline D3DXMATRIX kPrevModelViewProj{};
	static inline D3DXMATRIX kPrevModelView{};
	// The constants CreateConstantMaps maps: the shadow projection matrix, the object-to-world transform's rows,
	// the light data (ten registers: the direction, then the positions), the eye position and direction, the fog
	// colour and the texture effect's inverse bound diameter.
	static inline D3DXMATRIX ShadowProjMatrix{};
	static inline D3DXMATRIX kObjectToWorldTranspose{};
	static inline LightDataArray pLightData;
	static inline NiVector4 pEyePosition{};
	static inline NiVector4 pEyeVector{};
	static inline NiVector4 pFogColor{};
	static inline float fInvBoundDiameter;
	static inline bool bAlwaysWriteAlpha;
	static inline float fWaterDepthRange;
	static inline uiPixelNonVolatileConstantFlags_t uiPixelNonVolatileConstantFlags{};
	static inline uiVertexNonVolatileConstantFlags_t uiVertexNonVolatileConstantFlags{};
	static inline NiPoint4 kDefaultRefractionParams{1.0f};
};

using a0_0_t = const char[4];
inline a0_0_t a0_0{"0"};
