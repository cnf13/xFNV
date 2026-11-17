#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Reconstruction.hpp"

class NiD3DPass;
class NiD3DRenderStateGroup;
class NiD3DTextureStage;
class NiTextureEffect;

// The D3D shader: runs its passes per draw with its declaration, render states
// and constant maps.
class NiD3DShader : public NiD3DShaderInterface {
public:
	// Public: the table's definition (FNVD_DEFINE_DATA) names it at namespace scope.
	using DynEffectPacker = void (*)(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
		NiD3DTextureStage* pkStage, int32_t bSave, NiDX9Renderer* pkD3DRenderer);
	using DynEffectPackers = DynEffectPacker[5];

	~NiD3DShader() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bool Initialize() override;
	bool SetupGeometry(NiGeometry* pkGeometry) override;
	uint32_t PreProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;
	uint32_t SetupRenderingPass(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;
	uint32_t SetupTransformations(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
		const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
		const NiBound& kWorldBound) override;
	uint32_t SetupShaderPrograms(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
		const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
		const NiBound& kWorldBound) override;
	NiGeometryData::RendererData* PrepareGeometryForRendering(NiGeometry* pkGeometry,
		const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
		const NiPropertyState* pkState) override;
	uint32_t PostProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;
	uint32_t FirstPass() override;
	uint32_t NextPass() override;

	bool SetUsesNiRenderState(bool bUses) override {
		m_bUsesNiRenderState = bUses;
		return true;
	}

	NiD3DShaderDeclaration* GetShaderDecl() override { return m_spShaderDecl; }
	void SetShaderDecl(NiD3DShaderDeclaration* pkShaderDecl) override;
	void DestroyRendererData() override;
	void RecreateRendererData() override;
	bool GetVSPresentAllPasses() override;
	bool GetVSPresentAnyPass() override;
	bool GetPSPresentAllPasses() override;
	bool GetPSPresentAnyPass() override;
	virtual void ResetPasses();

	static void InitializePools();
	static void ShutdownPools();
	// cdecl, ret 0, 0xE81100: points the dynamic effect packers at the exe's packers.
	static void _SDMInit();
	// cdecl, ret 0, 0xE80690: releases ms_kNDLShaderMapName's handle.
	static void _SDMShutdown();
	static void SetupSCMExtraData(NiD3DShader* pkD3DShader, NiGeometry* pkGeometry);
	// Packs the texture effect's world projection into the stage with the packer
	// for its coordinate mode.
	static void PackDynamicEffect(const NiTextureEffect* apTextureEffect, NiD3DTextureStage* apStage, int32_t abSave,
		NiDX9Renderer* apD3DRenderer);

	// The packers are public: NiD3DShader.cpp's table entries (exePack*) name them at namespace scope.
	static void PackWorldParallelEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
		NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer);
	static void PackWorldPerspectiveEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
		NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer);
	static void PackWorldSphereEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
		NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer);
	static void PackSpecularCubeEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
		NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer);
	static void PackDiffuseCubeEffect(const NiMatrix3& kWorldMat, const NiPoint3& kWorldTrans,
		NiD3DTextureStage* pkStage, bool bSave, NiDX9Renderer* pkD3DRenderer);

	bool m_bUsesNiRenderState;
	bool m_bUsesNiLightState;
	NiPointer<NiD3DShaderDeclaration> m_spShaderDecl;
	NiD3DRenderStateGroup* m_pkRenderStateGroup;
	NiPointer<NiD3DShaderConstantMap> m_spPixelConstantMap;
	NiPointer<NiD3DShaderConstantMap> m_spVertexConstantMap;
	uint32_t m_uiCurrentPass;

	static NiRTTI ms_RTTI;
	// the pass being rendered (a plain pointer on PC)
	static inline NiD3DPass* m_spCurrentPass;
	// the name of the NiSCMExtraData a geometry carries for this shader's
	// constant maps
	static NiFixedString ms_kNDLShaderMapName;
	// the Pack*Effect function of each NiTextureEffect::CoordGenType
	static inline DynEffectPackers ms_apfnDynEffectPackers{};
};

NiSmartPointer(NiD3DShader);

inline constexpr fnvd::Vtable NiD3DShader_vftable{};
