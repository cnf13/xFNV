#pragma once

#include "BSShader/BSShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiVector4.h"
#include "Reconstruction.hpp"

class BSShaderNoLightingProperty;
class NiD3DPixelShader;
class NiD3DVertexShader;
class NiDX9ShaderDeclaration;
class NiMaterialProperty;

class BSShaderNoLighting : public BSShader {
public:
	// The PDB's names; the PC types and sizes are those CreateConstantMaps (0xBC9CC0) registers: three texture
	// transform rows, the fog's two registers (properties, then colour), the water fog's two and 16 sub-texture
	// offsets.
	using TextureTransformArray = NiVector4[3];
	// The PDB's NiColorA[2] (type 0x2BAF1) for both fog arrays; the exe stores NiColorA::BLACK into them (0xBC6DE0).
	using FogArray = NiColorA[2];
	using SubTexOffsetArray = NiVector4[16];

	BSShaderNoLighting(NiDX9ShaderDeclaration* apDecl, NiDX9ShaderDeclaration* apDeclVC,
		NiDX9ShaderDeclaration* apDeclTexVC, NiDX9ShaderDeclaration* apDeclN, NiDX9ShaderDeclaration* apDeclVCN,
		NiDX9ShaderDeclaration* apDeclTexVCN, NiDX9ShaderDeclaration* apSkinDecl, NiDX9ShaderDeclaration* apSkinDeclVC,
		NiDX9ShaderDeclaration* apSkinDeclTexVC, NiDX9ShaderDeclaration* apSkinDeclN,
		NiDX9ShaderDeclaration* apSkinDeclVCN, NiDX9ShaderDeclaration* apSkinDeclTexVCN);
	~BSShaderNoLighting() override;

	const NiRTTI* GetRTTI() const override;
	bool Initialize() override;
	uint32_t SetupBoneMatrices(NiGeometry*, const NiSkinInstance*, const NiSkinPartition::Partition*,
		NiGeometryData::RendererData*, const NiPropertyState*, const NiDynamicEffectState*, const NiTransform&,
		const NiBound&) override;
	void SetupGeometryTextures(const NiPropertyState* apState) override;
	void SetupGeometryConstants(const NiPropertyState* apState) override;
	void SetupGeometryAlphaBlending(const NiPropertyState* apState) override;
	void PostGeometry(const NiPropertyState* apState) override;
	void InitPass(uint32_t auiCurrentPass) override;
	void SetupPassConstants(uint32_t auiCurrentPass) override;
	using BSShader::GetShaderDecl;
	NiD3DShaderDeclaration* GetShaderDecl(NiGeometry* apGeometry, BSShaderProperty* apProperty) override;
	void CreateConstantMaps() override;
	void ReloadShaders() override;
	void ClearPassStages() override;

	bool LoadStagesAndPasses();
	void LoadShaders();
	void SetupGeometryConstants_MaterialColor(BSShaderNoLightingProperty* pprop, NiMaterialProperty* pmaterialProperty);
	// PC ends after the declarations; Xbox aModelCameraUpRight is not present.
	void SetupGeometryConstants_Particles(NiGeometry* pgeometry, const NiTransform& worldTransform);
	void SetupGeometryConstants_WaterFog();
	void SetupGeometryConstants_TextureTransform(const NiPropertyState* apState);

	static BSShader* CreateShader();

	// The constructor builds these with eh_vector_constructor_iterator and the NiPointer constructor (0xBC9CC0 calls
	// it with 4, 30), and LoadStagesAndPasses passes each to CreateNewPass(NiPointer<NiD3DPass>&).
	NiPointer<NiD3DPass> spPass[30];
	NiD3DVertexShader* spVertexShader[28];
	NiD3DPixelShader* spPixelShader[7];
	NiShaderConstantMapEntry* pParticleSubTextureOffsetEntry;
	// PC omits Xbox pCameraUpRightEntry: these entries and declarations are four bytes earlier.
	NiShaderConstantMapEntry* pWorldEntry;
	NiShaderConstantMapEntry* pWorldViewEntry;
	NiShaderConstantMapEntry* pWorldEyeDirEntry;
	NiShaderConstantMapEntry* pBonesEntry;
	NiDX9ShaderDeclaration* spDecl;
	NiDX9ShaderDeclaration* spDeclVc;
	NiDX9ShaderDeclaration* spDeclTexVc;
	NiDX9ShaderDeclaration* spDeclN;
	NiDX9ShaderDeclaration* spDeclVcN;
	NiDX9ShaderDeclaration* spDeclTexVcN;
	NiDX9ShaderDeclaration* spSkinDecl;
	NiDX9ShaderDeclaration* spSkinDeclVc;
	NiDX9ShaderDeclaration* spSkinDeclTexVc;
	NiDX9ShaderDeclaration* spSkinDeclN;
	NiDX9ShaderDeclaration* spSkinDeclVcN;
	NiDX9ShaderDeclaration* spSkinDeclTexVcN;

	static inline NiColorA kMaterialColor{};
	static NiRTTI ms_RTTI;
	static inline TextureTransformArray pTextureTransform{};
	static inline NiVector4 Toggles{};
	static inline NiVector4 FalloffData{};
	static SubTexOffsetArray aSubTexOffsets;
	static inline FogArray pFogPropA;
	static inline FogArray pWaterFogA;
};
inline constexpr fnvd::Vtable BSShaderNoLighting_vftable{};
