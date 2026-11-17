#pragma once

#include "BSShader/BSShaderProperty.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Lighting30ShaderProperty;
class NiMaterialProperty;
class NiSkinInstance;

// The PDB's spPixelShader and spVertexShader are NiPointer<NiD3DPixelShader>[50] and NiPointer<NiD3DVertexShader>[57];
// the exe's 0xBBA5D0 and 0xBB96F0 assign the elements through NiPointer's 0xA5F860.
using Lighting30ShaderPixelShaderArray = NiPointer<NiD3DPixelShader>[50];
using Lighting30ShaderVertexShaderArray = NiPointer<NiD3DVertexShader>[57];
using Lighting30ShaderVertexMapArray = NiShaderConstantMapEntry* [17];
using Lighting30ShaderPixelMapArray = NiShaderConstantMapEntry* [11];
// The exe's constant tables: kVSConstants runs 0x12010F0-0x1201B30 (164 entries) and kPSConstants 0x1201B30-0x1201F20
// (63 entries); Initialize (0xBBCE70) fills both whole.
using Lighting30ShaderConstantArray = NiVector4[63];
using Lighting30ShaderVSConstantArray = NiVector4[164];
// One flag per vertex map entry: EnableConstantsForPass (0xBBD880) fills 17, the constructor clears 0x1200854-0x1200864.
using Lighting30ShaderVSEnableArray = uint8_t[17];
// The exe's pass table: ClearPassStages (0xBBB7F0) walks 0x1200EC0 up to 0x1201080.
using Lighting30ShaderPassArray = NiPointer<NiD3DPass>[112];
// The per-pass constant flags, one dword per pass (bit j + 1 selects map entry j).
using Lighting30ShaderConstantFlags = uint32_t[112];

class Lighting30Shader : public BSShader {
public:
	using bPSConstantEnable_t = bool[10];

	Lighting30Shader(NiDX9ShaderDeclaration* apShaderDecl, NiDX9ShaderDeclaration* apSkinnedShaderDecl,
		NiDX9ShaderDeclaration* apShaderDeclVC, NiDX9ShaderDeclaration* apSkinnedShaderDeclVC);
	~Lighting30Shader() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	uint32_t SetupBoneMatrices(NiGeometry*, const NiSkinInstance*, const NiSkinPartition::Partition*,
		NiGeometryData::RendererData*, const NiPropertyState*, const NiDynamicEffectState*, const NiTransform&,
		const NiBound&) override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void SetupGeometryConstants(const NiPropertyState* apState) override;
	void SetupGeometryAlphaBlending(const NiPropertyState* apState) override;
	void PostGeometry(const NiPropertyState* apState) override;
	void RecreateRendererData() override;
	void InitPass(uint32_t auiCurrentPass) override;
	void SetupPassConstants(uint32_t auiCurrentPass) override;
	using BSShader::GetShaderDecl;
	NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;
	// Inferred type: a1: uint32_t: unsigned uses from 0xc0cd92
	virtual void UpdateDiffuseNormalStages(uint32_t, Lighting30ShaderProperty*);
	// Inferred type: a1: uint32_t: unsigned uses from 0xc0cf23
	virtual void UpdatePassTextureStages(uint32_t, Lighting30ShaderProperty*);
	virtual void SetRenderStatesForPass(uint32_t auiPass, Lighting30ShaderProperty* apL30Prop);

protected:
	virtual void LoadShaders();
	virtual void LoadVertexShaders();
	virtual void LoadPixelShaders();
	virtual bool LoadStagesAndPasses();

public:
	// Sets up the pass for aePassEnum and returns it.
	NiD3DPass* UpdatePass(BSShaderManager::eBSSMPassEnum aePassEnum, Lighting30ShaderProperty* apL30Prop);
	void SetTextureMode(NiD3DTextureStage* apStage, BSShaderPPLightingProperty* apProp);
	void DisableNonVolatileConstantsForPass(uint32_t auiPassEnum);
	void SetupGeometryConstants_WorldBoundCenter(const NiBound& worldBound);
	void SetupGeometryConstants_Envmap(Lighting30ShaderProperty* apProp);
	// Builds the shader's stages and passes up front.
	void PresetStages();
	void ClearUnusedStages(NiD3DPass* apPass, uint32_t auiFirstUnusedStage);
	void UpdatePipelineZOnlyTexEffect(BSShaderPPLightingProperty* apBSSProp, NiD3DPass* apPass);
	void SetHairMaps(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop);
	void SetNormalMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiStageNum);
	void SetHeightMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiStageNum);
	void SetEnvironmentMask(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop);
	void SetupGeometryConstants_RefractionPower(Lighting30ShaderProperty* pprop);
	// Gives every pass but the texture effect ones (102, 103) new vertex and pixel constant maps holding the entries
	// its constant flags select; passes 102 and 103 share the texture effect maps.
	void BuildPerPassConstantMaps();
	// Inferred type: apPass: NiD3DPass* from 0xc0d074 (NiD3DShader::m_spCurrentPass), its members used at 0xc0bb67
	void SetupStandardTextures(NiD3DPass* apPass, Lighting30ShaderProperty* apShaderProperty, bool abUseGlowMap);
	void SetupGeometryConstants_WaterFog();
	void SetupGeometryConstants_ShadowProjection(const NiSkinInstance* pskin, const NiTransform& worldTransform);
	void SetupGeometryConstants_HairTint(Lighting30ShaderProperty* pprop);
	void UpdatePipelineTexEffect(BSShaderPPLightingProperty* apBSSProp, NiD3DPass* apPass);
	void UpdateTexEffectConstants(NiGeometry* apGeometry, BSShaderPPLightingProperty* apBSSProp, NiD3DPass* apPass);
	void SetupGeometryConstants_ObjToCubemap(BSShaderProperty::RenderPass* apRenderPass, uint32_t auiCurrentPass,
		const NiSkinInstance* apSkin, const NiTransform& aWorldTransform);
	void SetupGeometryConstants_MaterialAlpha(const NiBound& aWorldBound, Lighting30ShaderProperty* apProp,
		const NiPropertyState* apState, uint32_t auiCurrentPass);
	void SetupGeometryConstants_Emittance(NiMaterialProperty* pmaterialProperty, Lighting30ShaderProperty* pprop);
	void SetupGeometryConstants_EyePosition(_D3DMATRIX d3dInvWorld, const NiTransform& worldTransform);
	void SetupGeometryConstants_Lights(Lighting30ShaderProperty* pprop, BSShaderProperty::RenderPass* apRenderPass,
		uint32_t auiCurrentPass, NiMaterialProperty* pmaterialProperty, _D3DMATRIX& d3dInvWorld,
		const NiSkinInstance* pskin, const NiTransform& worldTransform);
	void SetRenderNormalsMaps(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop);
	void SetProjShadowMaps(NiD3DPass* apPass);
	void SetEnvironmentMap(NiD3DPass* apPass, Lighting30ShaderProperty* apL30Prop, uint32_t auiPass);
	// Inferred type: a1: BSShaderManager::eBSSMPassEnum: unsigned uses from 0xc0abdc
	NiD3DPass* SelectPass(BSShaderManager::eBSSMPassEnum a1);

	// NiPointer's shared out-of-line
	// assignment from a raw pointer run on an element of pVertexConstantMapEntry, which
	// src keeps as a raw pointer: holds a reference on apEntry, releases the old one and returns appSlot.
	static NiShaderConstantMapEntry** SetVertexConstantMapEntry(NiShaderConstantMapEntry** appSlot,
		NiShaderConstantMapEntry* apEntry);
	// Uploads one light's constants: akData to kPSConstants[2 * aiOffset + 22], akColor to
	// kPSConstants[2 * aiOffset + 21].
	static void SetLight(int32_t aiOffset, NiColorA akData, NiColorA akColor);
	// IDA name; not in the PDB. Packs the low bits of eight flags into a pass index, acBit0 lowest; the flags are
	// sign-extended before the or, as the exe does.
	static int32_t C06DC0(int8_t acBit0, int8_t acBit1, int8_t acBit2, int8_t acBit3, int8_t acBit4, int8_t acBit5,
		int8_t acBit6, int8_t acBit7);
	// Enables the shader constants pass auiPassEnum uses.
	static void EnableConstantsForPass(uint32_t auiPassEnum, bool abAlphaPass);
	static void SetLightColor(int32_t aiOffset, NiColorA akColor);
	static void SetAmbientColor(const NiColorA& akColor, bool abModified);
	static void ResetPerFrameConstantData();
	static BSShader* CreateShader();
	static void CreateDefaultStages(NiD3DPass* a1, int32_t x0);

	NiPointer<NiD3DShaderConstantMap> spTexEffectPConstantMap;
	NiPointer<NiD3DShaderConstantMap> spTexEffectVConstantMap;
	NiPointer<NiD3DShaderConstantMap> spDefaultPixelConstantMap;
	NiPointer<NiD3DShaderConstantMap> spDefaultVertexConstantMap;
	NiPointer<NiDX9ShaderDeclaration> spShaderDecl;
	NiPointer<NiDX9ShaderDeclaration> spShaderDeclNC;
	NiPointer<NiDX9ShaderDeclaration> spShaderDeclVC;
	NiPointer<NiDX9ShaderDeclaration> spShaderDeclVCNC;
	NiPointer<NiDX9ShaderDeclaration> spSkinnedShaderDecl;
	NiPointer<NiDX9ShaderDeclaration> spSkinnedShaderDeclNC;
	NiPointer<NiDX9ShaderDeclaration> spSkinnedShaderDeclVC;
	NiPointer<NiDX9ShaderDeclaration> spSkinnedShaderDeclVCNC;
	// The geometry data last bound and whether z-writes are on: the constructor stores 0 here and 1 at 0xA0, the
	// destructor 0 here.
	NiGeometryData::RendererData* pLastGeometryBufferData;
	uint32_t iZWriteEnable;

	static Lighting30ShaderConstantArray kPSConstants;
	static Lighting30ShaderPixelShaderArray spPixelShaders;
	static Lighting30ShaderPixelShaderArray spPixelShadersLowLight;
	static Lighting30ShaderVertexShaderArray spVertexShaders;
	static Lighting30ShaderPassArray spPass;
	static inline Lighting30ShaderVSEnableArray bVSConstantEnable{};
	static inline Lighting30ShaderVertexMapArray pVertexConstantMapEntry;
	static inline int8_t bAmbientEnabled;
	static inline int8_t bMatAlphaEnabled;
	static inline Lighting30ShaderPixelMapArray pPixelConstantMapEntry;
	static Lighting30ShaderVSConstantArray kVSConstants;
	static NiRTTI ms_RTTI;
	// IDA lists these as Lighting30Shader::Constant::*; the PDB's statics are named below.
	static inline float fTextureUOffset;
	static inline float fTextureVOffset;
	static inline NiVector4 FillColor{};
	static inline NiVector4 RimColor{};
	static inline NiVector4 fVars{};
	static inline NiVector4 TexEffectFogColor{};
	static inline NiVector4 TexEffectFogParam{};
	static inline float fInvBoundDiameter;
	static inline Lighting30ShaderConstantFlags ulPixelConstantFlags{};
	static inline Lighting30ShaderConstantFlags ulVertexConstantFlags{};
	static inline int32_t iMaxLights = 20;
	static inline bPSConstantEnable_t bPSConstantEnable{};
	static inline bool bAlwaysWriteAlpha;
};

inline int32_t data_1200EBC;

// Not named in the PDB or IDA; name chosen here. An int (2) after iMaxLights that the exe never reads.
inline int32_t data_11AE610 = 2;
inline constexpr fnvd::Vtable Lighting30Shader_vftable{};
