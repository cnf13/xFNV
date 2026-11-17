#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DDefaultShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9LightManager.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

NiRTTI NiD3DDefaultShader::ms_RTTI("NiD3DDefaultShader", &NiD3DShader::ms_RTTI);

// NiD3DShader's fields are set here.
NiD3DDefaultShader::NiD3DDefaultShader() : m_peFogPassArray(nullptr), m_uiFogPassArraySize(0), m_pkLastState(nullptr) {
	m_bUsesNiRenderState = false;
	m_bUsesNiLightState = false;
	m_spShaderDecl = nullptr;
	m_pkRenderStateGroup = nullptr;
	m_spPixelConstantMap = nullptr;
	m_spVertexConstantMap = nullptr;
	m_uiCurrentPass = 0;
	m_kName = "Default Shader From Gamebryo 2.1";
	m_bIsBestImplementation = true;
}

NiD3DDefaultShader::~NiD3DDefaultShader() {
	_NiFree(m_peFogPassArray);
}

bool NiD3DDefaultShader::Initialize() {
	if (!m_bInitialized) {
		if (!NiD3DShader::Initialize()) {
			return false;
		}
		InitializeDeviceCaps();
	}
	return true;
}

// Applies the property state and the lights (with the texturing property) before NiD3DShader's preprocessing, and
// saves the lighting state that results for PostProcessPipeline.
uint32_t NiD3DDefaultShader::PreProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	m_pkD3DRenderState->UpdateRenderState(pkState);
	m_pkD3DRenderer->m_pkLightManager->SetState(pkEffects,
		static_cast<const NiTexturingProperty*>(pkState->m_aspProps[NiProperty::TEXTURING].m_pObject), nullptr);
	const uint32_t uiRet =
		NiD3DShader::PreProcessPipeline(pkGeometry, pkSkin, pkRendererData, pkState, pkEffects, kWorld, kWorldBound);
	m_uiOldLightingValue = m_pkD3DRenderState->GetRenderState(D3DRS_LIGHTING);
	return uiRet;
}

// Rebuilds the pass list for this geometry.
uint32_t NiD3DDefaultShader::UpdatePipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	ResetPasses();
	return ConstructPipeline(pkGeometry, pkSkin, static_cast<NiGeometryBufferData*>(pkRendererData), pkState, pkEffects,
		kWorld, kWorldBound);
}

// Puts back the lighting state PreProcessPipeline saved, then finishes as NiD3DShader.
uint32_t NiD3DDefaultShader::PostProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	m_pkD3DRenderState->SetRenderState(D3DRS_LIGHTING, m_uiOldLightingValue, 0, false);
	return NiD3DShader::PostProcessPipeline(pkGeometry, pkSkin, pkRendererData, pkState, pkEffects, kWorld,
		kWorldBound);
}

// One pass: the base map modulated by the diffuse color under a non-replace apply mode, the base map alone
// otherwise, or the diffuse color alone without a base texture. Nothing when there is neither a base texture nor
// an apply mode. The top byte of the buffer data's flags counts its texture coordinate sets.
uint32_t NiD3DDefaultShader::ConstructPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryBufferData* pkBuffData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	if (!pkGeometry) {
		if (!pkBuffData || !pkBuffData->IsVBChipValid() || !pkState) {
			return 0xFFFFFFFF;
		}
	} else if (!pkBuffData) {
		return 0xFFFFFFFF;
	}

	NiTexturingProperty* pkTexProp =
		static_cast<NiTexturingProperty*>(static_cast<NiProperty*>(pkState->m_aspProps[NiProperty::TEXTURING]));
	if (!pkBuffData) {
		pkBuffData = pkGeometry->m_spModelData->m_pkBuffData;
	}
	const uint16_t usTextureSets = static_cast<uint8_t>(pkBuffData->m_uiFlags >> 24);

	NiTexturingProperty::Map* pkBaseMap = nullptr;
	bool bBaseTexture = false;
	m_uiMaxTextureIndex = 0;
	if (usTextureSets) {
		pkBaseMap = pkTexProp->m_kMaps[NiTexturingProperty::BASE_INDEX];
		m_uiMaxTextureIndex = usTextureSets - 1;
		bBaseTexture = pkBaseMap && pkBaseMap->m_spTexture;
	}

	const bool bApplyMode = ((pkTexProp->m_uFlags & NiTexturingProperty::APPLYMODE_MASK) >>
								NiTexturingProperty::APPLYMODE_POS) != NiTexturingProperty::APPLY_REPLACE;
	if (!bApplyMode && !bBaseTexture) {
		return 0;
	}

	OpenNewPass(D3DBLEND_ZERO, D3DBLEND_SRCCOLOR, false, false);
	NiD3DTextureStage* pkStage;
	if (bBaseTexture) {
		uint16_t usIndex = pkBaseMap->m_uFlags & NiTexturingProperty::Map::TEXCOORD_MASK;
		if (usIndex > m_uiMaxTextureIndex) {
			usIndex = static_cast<uint16_t>(m_uiMaxTextureIndex);
		}
		pkStage = AddTextureStage(pkBaseMap, usIndex);
		if (bApplyMode) {
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLOROP, D3DTOP_MODULATE, false);
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLORARG2, D3DTA_DIFFUSE, false);
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_MODULATE, false);
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG2, D3DTA_DIFFUSE, false);
			ClosePass(pkBaseMap);
			return 0;
		}
	} else {
		pkStage = AddTextureStage(nullptr, 0);
	}
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLOROP, D3DTOP_SELECTARG1, false);
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_SELECTARG1, false);
	ClosePass(pkBaseMap);
	return 0;
}

// The new pass only writes red, green and blue (D3DRS_COLORWRITEENABLE 7). A pass returning the base texture's
// alpha keeps one stage and one texture for it.
void NiD3DDefaultShader::OpenNewPass(D3DBLEND eSrcBlend, D3DBLEND eDestBlend, bool bReturnBaseTextureAlpha,
	bool bReturnVertexColorAlpha) {
	NiPointer<NiD3DPass> spPass;
	m_spCurrentPass = nullptr;
	NiD3DPass::CreateNewPass(spPass);
	m_spCurrentPass = spPass;
	spPass->SetRenderState(D3DRS_COLORWRITEENABLE, 7);

	m_bBaseTextureAlphaNeeded = bReturnBaseTextureAlpha;
	m_bVertexColorAlphaNeeded = bReturnVertexColorAlpha;
	m_uiRemainingStages = NiD3DPass::ms_uiMaxTextureBlendStages;
	m_uiRemainingTextures = NiD3DPass::ms_uiMaxSimultaneousTextures;
	if (bReturnBaseTextureAlpha) {
		m_uiRemainingStages--;
		m_uiRemainingTextures--;
	}
}

// When the pass returns the base texture's alpha, a last stage keeps the color and takes the alpha from the base
// texture (modulated by the diffuse alpha).
void NiD3DDefaultShader::ClosePass(const NiTexturingProperty::Map* pkBaseMap) {
	if (!m_spCurrentPass) {
		return;
	}

	if (m_bBaseTextureAlphaNeeded && pkBaseMap) {
		uint16_t usIndex = pkBaseMap->m_uFlags & NiTexturingProperty::Map::TEXCOORD_MASK;
		if (usIndex > m_uiMaxTextureIndex) {
			usIndex = static_cast<uint16_t>(m_uiMaxTextureIndex);
		}
		NiD3DTextureStage* pkStage = AddTextureStage(pkBaseMap, usIndex);
		if (pkStage) {
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLOROP, D3DTOP_SELECTARG2, false);
			if (m_bBaseTextureAlphaNeeded) {
				pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_MODULATE, false);
				pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG2, D3DTA_DIFFUSE, false);
			} else {
				pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_SELECTARG1, false);
			}
		}
	}
	m_uiRemainingStages = 0;
	m_uiRemainingTextures = 0;
}

// A textured stage modulates nothing yet: color is the texture, alpha the current one (or the vertex alpha, once,
// when the pass still needs it). A stage without a texture passes the diffuse color through the same way.
NiD3DTextureStage* NiD3DDefaultShader::AddTextureStage(const NiTexturingProperty::Map* pkMap, uint32_t uiIndex) {
	if (!m_spCurrentPass || !m_spCurrentPass->CheckFreeStages(1, pkMap != nullptr)) {
		return nullptr;
	}

	NiPointer<NiD3DTextureStage> spStage;
	NiD3DTextureStage::CreateNewStage(spStage);
	NiD3DTextureStage* pkStage = spStage;
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_RESULTARG, D3DTA_CURRENT, false);

	NiTexture* pkTexture;
	if (pkMap && (pkTexture = pkMap->m_spTexture) != nullptr) {
		pkStage->SetTexture(pkTexture);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, uiIndex, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLOROP, D3DTOP_SELECTARG1, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLORARG1, D3DTA_TEXTURE, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLORARG2, D3DTA_CURRENT, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_SELECTARG2, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG1, D3DTA_TEXTURE, false);
		if (m_bVertexColorAlphaNeeded) {
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG2, D3DTA_DIFFUSE, false);
			m_bVertexColorAlphaNeeded = false;
		} else {
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG2, D3DTA_CURRENT, false);
		}

		pkStage->SetFilterMode(static_cast<
			NiTexturingProperty::FilterMode>((pkMap->m_uFlags & NiTexturingProperty::Map::FILTERMODE_MASK) >>
			NiTexturingProperty::Map::FILTERMODE_POS));
		pkStage
			->SetClampMode(static_cast<NiTexturingProperty::ClampMode>((pkMap->m_uFlags &
																		   NiTexturingProperty::Map::CLAMPMODE_MASK) >>
				NiTexturingProperty::Map::CLAMPMODE_POS));

		NiTextureTransform* pkTransform = pkMap->m_pkTextureTransform;
		if (!pkTransform) {
			pkStage->m_bTextureTransform = false;
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE, false);
		} else {
			PackTextureTransform(*pkTransform->GetMatrix(), pkStage);
		}
		m_uiRemainingTextures--;
	} else {
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLOROP, D3DTOP_SELECTARG1, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLORARG1, D3DTA_DIFFUSE, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_COLORARG2, D3DTA_CURRENT, false);
		if (m_bVertexColorAlphaNeeded) {
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_SELECTARG1, false);
			m_bVertexColorAlphaNeeded = false;
		} else {
			pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAOP, D3DTOP_SELECTARG2, false);
		}
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG1, D3DTA_DIFFUSE, false);
		pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_ALPHAARG2, D3DTA_CURRENT, false);
		if (m_uiRemainingTextures) {
			m_uiRemainingTextures--;
		}
	}
	m_uiRemainingStages--;

	m_spCurrentPass->SetStage(m_spCurrentPass->m_uiCurrentStage, pkStage);
	return pkStage;
}

// The stage's transform takes the matrix's first two rows as its first two columns; every other entry is zero.
void NiD3DDefaultShader::PackTextureTransform(const NiMatrix3& kTexMatrix, NiD3DTextureStage* pkStage) {
	pkStage->m_bTextureTransform = true;
	pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT2, false);

	D3DMATRIX& kD3DMatrix = pkStage->m_kTextureTransformation;
	kD3DMatrix._11 = kTexMatrix.m_pEntry[0][0];
	kD3DMatrix._21 = kTexMatrix.m_pEntry[0][1];
	kD3DMatrix._31 = kTexMatrix.m_pEntry[0][2];
	kD3DMatrix._41 = 0.0f;
	kD3DMatrix._12 = kTexMatrix.m_pEntry[1][0];
	kD3DMatrix._22 = kTexMatrix.m_pEntry[1][1];
	kD3DMatrix._32 = kTexMatrix.m_pEntry[1][2];
	kD3DMatrix._42 = 0.0f;
	kD3DMatrix._43 = 0.0f;
	kD3DMatrix._33 = 0.0f;
	kD3DMatrix._23 = 0.0f;
	kD3DMatrix._13 = 0.0f;
	kD3DMatrix._44 = 0.0f;
	kD3DMatrix._34 = 0.0f;
	kD3DMatrix._24 = 0.0f;
	kD3DMatrix._14 = 0.0f;
}
