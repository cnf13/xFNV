#pragma once

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Reconstruction.hpp"

class NiD3DTextureStage;

// The fixed-function style shader the renderer falls back to: builds its passes from the geometry's properties.
class NiD3DDefaultShader : public NiD3DShader {
public:
	enum PassFogStatus : uint32_t {
		NO_FOG = 0,
		BLACK_FOG = 1,
		STANDARD_FOG = 2,
	};

	NiD3DDefaultShader();
	~NiD3DDefaultShader() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	bool Initialize() override;
	uint32_t PreProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;
	uint32_t UpdatePipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;
	uint32_t PostProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
		NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState,
		const NiDynamicEffectState* pkEffects, const NiTransform& kWorld, const NiBound& kWorldBound) override;

protected:
	void InitializeDeviceCaps() {}
	// Builds the single pass from the base map and the texturing apply mode; 0xFFFFFFFF without the geometry data.
	uint32_t ConstructPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin, NiGeometryBufferData* pkBuffData,
		const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
		const NiBound& kWorldBound);
	// Starts a pass as the current pass; the blend arguments are unused on PC.
	void OpenNewPass(D3DBLEND eSrcBlend, D3DBLEND eDestBlend, bool bReturnBaseTextureAlpha,
		bool bReturnVertexColorAlpha);
	// Adds the base texture's alpha stage when the pass needs it and clears the remaining counts.
	void ClosePass(const NiTexturingProperty::Map* pkBaseMap);
	// Appends a stage for pkMap (or a diffuse stage without a texture) to the current pass; null when it is full.
	NiD3DTextureStage* AddTextureStage(const NiTexturingProperty::Map* pkMap, uint32_t uiIndex);
	// Turns on a two-coordinate texture transform on the stage from the 3x3 texture matrix.
	void PackTextureTransform(const NiMatrix3& kTexMatrix, NiD3DTextureStage* pkStage);

public:
	bool m_bBaseTextureAlphaNeeded;
	bool m_bVertexColorAlphaNeeded;
	uint32_t m_uiMaxTextureIndex;
	uint32_t m_uiRemainingStages;
	uint32_t m_uiRemainingTextures;
	uint32_t m_uiOldLightingValue;
	PassFogStatus* m_peFogPassArray;
	uint32_t m_uiFogPassArraySize;
	const NiDynamicEffectState* m_pkLastState;

	static NiRTTI ms_RTTI;
};

// The shader's name (the exe's own string, text and NUL), whose address the constructor compares with the name's
// handle. IDA name; not in the PDB.
using DefaultShaderNameText = const char[33];
inline DefaultShaderNameText aDefaultShaderF{"Default Shader From Gamebryo 2.1"};
inline constexpr fnvd::Vtable NiD3DDefaultShader_vftable{};
