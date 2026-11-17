#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFragmentShaderInstanceDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"

class NiD3DTextureStage;
class NiDX9Renderer;
class NiDynamicEffectState;
class NiTexturingProperty;

// The shader the renderer builds for a fragment material's descriptor. Its members take the names of the Xbox twin,
// NiXenonFragmentShader. Only some of its members are declared.
class NiDX9FragmentShader : public NiD3DShader {
public:
	NiDX9FragmentShader(NiMaterialDescriptor* apMaterialDescriptor);

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	// Calls NiD3DShader::SetupRenderingPass.
	uint32_t SetupRenderingPass(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;
	uint32_t SetupShaderPrograms(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
		const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
		const NiBound& kWorldBound) override;
	uint32_t PostRender(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
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
	uint32_t UpdatePipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;
	uint32_t SetupTransformations(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
		const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
		const NiBound& kWorldBound) override;
	bool IsGenericallyConfigurable() override { return true; }
	NiShaderDeclaration* CreateShaderDeclaration(uint32_t uiMaxStreamEntryCount, uint32_t uiStreamCount) override;
	bool AppendRenderPass(uint32_t& uiPassId) override;
	bool SetAlphaOverride(uint32_t uiPassId, bool bAlphaBlend, bool bUsePreviousSrcBlendMode,
		NiAlphaProperty::AlphaFunction eSrcBlendMode, bool bUsePreviousDestBlendMode,
		NiAlphaProperty::AlphaFunction eDestBlendMode) override;
	// Binds pkProgram to the current pass when uiPassId is 0: false otherwise or without a current pass.
	bool SetGPUProgram(uint32_t uiPassId, NiGPUProgram* pkProgram, uint32_t& uiProgramId) override;
	// The current pass's vertex (uiProgramId 0) or pixel (1) constant map, created when it has none.
	NiShaderConstantMap* CreateShaderConstantMap(uint32_t uiPassId, uint32_t uiProgramId) override;
	// Appends a new texture stage to the current pass (uiPassId 0 only) and sets its map flags from kSemantic.
	bool AppendTextureSampler(uint32_t uiPassId, uint32_t& uiSamplerId, const NiFixedString& kSemantic,
		const NiFixedString& kVariableName, uint32_t uiInstance) override;
	NiShader::NiShaderInstanceDescriptor* GetShaderInstanceDesc() override { return &m_kDescriptor; }
	void DestroyRendererData() override;
	using NiD3DShader::Initialize;
	// Slot 60, new in this class: initialised already, or NiD3DShader::Initialize succeeds.
	virtual bool Initialize(NiDX9Renderer* pkRenderer);

	// Sets the stage's texture, clamp and filter mode from the texturing property's map or the dynamic effect its flags
	// select.
	void PrepareTextureStage(NiD3DTextureStage* pkStage, NiTexturingProperty* pkTexProp,
		const NiDynamicEffectState* pkEffects);

	NiFragmentShaderInstanceDescriptor m_kDescriptor;

	static NiRTTI ms_RTTI;
};
