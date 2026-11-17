#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9FragmentShader.h"

#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DRenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9RenderState.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9ShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffectState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFragmentShaderInstanceDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGPUProgram.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialDescriptor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinPartition.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTextureEffect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderDeclaration.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderInterface.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStageGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiGeometryBufferData.h"

NiRTTI NiDX9FragmentShader::ms_RTTI("NiDX9FragmentShader", &NiD3DShader::ms_RTTI);

// The base shader's fields are cleared, the descriptor's material descriptor starts empty, then is assigned
// *apMaterialDescriptor. The descriptor's identifier is released by its own destructor.
NiDX9FragmentShader::NiDX9FragmentShader(NiMaterialDescriptor* apMaterialDescriptor) {
	m_kDescriptor.m_kMatDesc = *apMaterialDescriptor;
}

uint32_t NiDX9FragmentShader::SetupRenderingPass(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	return NiD3DShader::SetupRenderingPass(pkGeometry, pkSkin, pkRendererData, pkState, pkEffects, kWorld, kWorldBound);
}

uint32_t NiDX9FragmentShader::SetupShaderPrograms(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
	const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
	const NiBound& kWorldBound) {
	return NiD3DShader::SetupShaderPrograms(pkGeometry, pkSkin, pkPartition, pkRendererData, pkState, pkEffects, kWorld,
		kWorldBound);
}

uint32_t NiDX9FragmentShader::PostRender(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
	const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
	const NiBound& kWorldBound) {
	return NiShader::SetupTransformations(pkGeometry, pkSkin, pkPartition, pkRendererData, pkState, pkEffects, kWorld,
		kWorldBound);
}

NiGeometryData::RendererData* NiDX9FragmentShader::PrepareGeometryForRendering(NiGeometry* pkGeometry,
	const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
	const NiPropertyState* pkState) {
	return NiD3DShader::PrepareGeometryForRendering(pkGeometry, pkPartition, pkRendererData, pkState);
}

uint32_t NiDX9FragmentShader::PostProcessPipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	return NiD3DShader::PostProcessPipeline(pkGeometry, pkSkin, pkRendererData, pkState, pkEffects, kWorld,
		kWorldBound);
}

uint32_t NiDX9FragmentShader::FirstPass() {
	return NiD3DShader::FirstPass();
}

uint32_t NiDX9FragmentShader::NextPass() {
	return NiD3DShader::NextPass();
}

uint32_t NiDX9FragmentShader::UpdatePipeline(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	NiGeometryData::RendererData* pkRendererData, const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects,
	const NiTransform& kWorld, const NiBound& kWorldBound) {
	NiD3DPass* v8 = NiD3DShader::m_spCurrentPass;
	if (NiD3DShader::m_spCurrentPass) {
		uint32_t v10 = 0;
		if (NiD3DShader::m_spCurrentPass->m_uiStageCount) {
			do {
				NiD3DTextureStage* m_pObject = v8->m_kStages.m_pBase[v10].m_pObject;
				if (m_pObject) {
					PrepareTextureStage(m_pObject, static_cast<NiTexturingProperty*>(pkState->m_aspProps[5].m_pObject),
						pkEffects);
				}
				++v10;
			} while (v10 < v8->m_uiStageCount);
		}
	}
	return NiShader::UpdatePipeline(pkGeometry, pkSkin, pkRendererData, pkState, pkEffects, kWorld, kWorldBound);
}

uint32_t NiDX9FragmentShader::SetupTransformations(NiGeometry* pkGeometry, const NiSkinInstance* pkSkin,
	const NiSkinPartition::Partition* pkPartition, NiGeometryData::RendererData* pkRendererData,
	const NiPropertyState* pkState, const NiDynamicEffectState* pkEffects, const NiTransform& kWorld,
	const NiBound& kWorldBound) {
	if (!pkPartition) {
		return NiD3DShader::SetupTransformations(pkGeometry, pkSkin, 0, pkRendererData, pkState, pkEffects, kWorld,
			kWorldBound);
	}
	if (!m_uiCurrentPass) {
		m_pkD3DRenderer->CalculateBoneMatrices(pkSkin, kWorld, 1, 3u, 1);
	}
	m_pkD3DRenderState->SetBoneCount(pkPartition->m_usBonesPerVertex);
	m_pkD3DRenderer->SetModelTransform(kWorld, 0);
	return 0;
}

NiShaderDeclaration* NiDX9FragmentShader::CreateShaderDeclaration(uint32_t uiMaxStreamEntryCount,
	uint32_t uiStreamCount) {
	NiDX9ShaderDeclaration* v4 = NiDX9ShaderDeclaration::Create(static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer),
		uiMaxStreamEntryCount, uiStreamCount);
	SetShaderDecl(v4);
	return v4;
}

bool NiDX9FragmentShader::AppendRenderPass(uint32_t& uiPassId) {
	NiPointer<NiD3DPass> spPass;
	if (NiD3DShader::m_spCurrentPass) {
		return false;
	}
	NiD3DPass::CreateNewPass(spPass);
	NiD3DShader::m_spCurrentPass = spPass.m_pObject;
	uiPassId = 0;
	return true;
}

bool NiDX9FragmentShader::SetAlphaOverride(uint32_t uiPassId, bool bAlphaBlend, bool bUsePreviousSrcBlendMode,
	NiAlphaProperty::AlphaFunction eSrcBlendMode, bool bUsePreviousDestBlendMode,
	NiAlphaProperty::AlphaFunction eDestBlendMode) {
	if (uiPassId) {
		return 0;
	}
	NiD3DPass* v8 = NiD3DShader::m_spCurrentPass;
	if (!NiD3DShader::m_spCurrentPass) {
		return 0;
	}
	if (bAlphaBlend) {
		if (!NiD3DShader::m_spCurrentPass->m_pkRenderStateGroup) {
			v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v8->m_pkRenderStateGroup->SetRenderState(27u, 1u, 1);
		NiDX9RenderState* D3DRenderState = v8->GetD3DRenderState();
		if (bUsePreviousSrcBlendMode) {
			NiD3DRenderStateGroup* m_pkRenderStateGroup = v8->m_pkRenderStateGroup;
			if (m_pkRenderStateGroup) {
				m_pkRenderStateGroup->RemoveRenderState(19u);
			}
		} else {
			v8->SetRenderState(19u, D3DRenderState->m_auiAlphaMapping[eSrcBlendMode], 0);
		}
		if (!bUsePreviousDestBlendMode) {
			v8->SetRenderState(20u, D3DRenderState->m_auiAlphaMapping[eDestBlendMode], 0);
			return 1;
		}
		NiD3DRenderStateGroup* v11 = v8->m_pkRenderStateGroup;
		if (v11) {
			v11->RemoveRenderState(20u);
			return 1;
		}
	} else {
		if (!NiD3DShader::m_spCurrentPass->m_pkRenderStateGroup) {
			v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
		}
		v8->m_pkRenderStateGroup->SetRenderState(27u, 0, 1);
	}
	return 1;
}

// uiProgramId is not written.
bool NiDX9FragmentShader::SetGPUProgram(uint32_t uiPassId, NiGPUProgram* pkProgram, uint32_t& uiProgramId) {
	if (uiPassId != 0 || !NiD3DShader::m_spCurrentPass) {
		return false;
	}
	switch (pkProgram->m_eProgramType) {
	case NiGPUProgram::PROGRAM_VERTEX:
		NiD3DShader::m_spCurrentPass->SetVertexShader(static_cast<NiD3DVertexShader*>(pkProgram));
		break;
	case NiGPUProgram::PROGRAM_PIXEL:
		NiD3DShader::m_spCurrentPass->SetPixelShader(static_cast<NiD3DPixelShader*>(pkProgram));
		break;
	}
	return true;
}

NiShaderConstantMap* NiDX9FragmentShader::CreateShaderConstantMap(uint32_t uiPassId, uint32_t uiProgramId) {
	if (uiPassId != 0) {
		return nullptr;
	}
	NiD3DPass* pkPass = NiD3DShader::m_spCurrentPass;
	if (!pkPass) {
		return nullptr;
	}
	NiD3DShaderConstantMap* pkSCM = nullptr;
	if (uiProgramId == 0) {
		pkSCM = pkPass->m_spVertexConstantMap.m_pObject;
		if (pkSCM) {
			return pkSCM;
		}
		void* pvMem = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
		pkSCM = pvMem ? ::new (pvMem) NiD3DShaderConstantMap(static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer),
							NiGPUProgram::PROGRAM_VERTEX)
					  : nullptr;
		pkPass->SetVertexConstantMap(pkSCM);
		return pkSCM;
	}
	if (uiProgramId == 1) {
		pkSCM = pkPass->m_spPixelConstantMap.m_pObject;
		if (!pkSCM) {
			void* pvMem = NiMemObject::operator new(sizeof(NiD3DShaderConstantMap));
			pkSCM = pvMem ? ::new (pvMem) NiD3DShaderConstantMap(static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer),
								NiGPUProgram::PROGRAM_PIXEL)
						  : nullptr;
			pkPass->SetPixelConstantMap(pkSCM);
		}
	}
	return pkSCM;
}

// Unlike the reference source, takes the pass from NiD3DShader::m_spCurrentPass, accepts only uiPassId 0 and puts the
// stage at the pass's current stage through SetStage.
bool NiDX9FragmentShader::AppendTextureSampler(uint32_t uiPassId, uint32_t& uiSamplerId, const NiFixedString& kSemantic,
	const NiFixedString& kVariableName, uint32_t uiInstance) {
	if (uiPassId >= 1) {
		return false;
	}
	NiD3DPass* pkPass = NiD3DShader::m_spCurrentPass;
	if (!pkPass) {
		return false;
	}
	// EXE-INLINED none: NiD3DTextureStage::DecRefCount
	NiPointer<NiD3DTextureStage> spStage;
	NiD3DTextureStage::CreateNewStage(spStage);
	if (pkPass->SetStage(pkPass->m_uiCurrentStage, spStage.m_pObject) != 0) {
		return false;
	}
	uiSamplerId = pkPass->m_uiStageCount - 1;
	uint32_t uiTextureID = 0;
	NiD3DTextureStage* pkStage = spStage.m_pObject;
	if (NiTexturingProperty::GetMapIDFromName(kSemantic, uiTextureID)) {
		// The bodies of SetNDLMap / SetDecalMap / SetShaderMap.
		switch (uiTextureID) {
		case NiTexturingProperty::BASE_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x200000;
			break;
		case NiTexturingProperty::DARK_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x300000;
			break;
		case NiTexturingProperty::DETAIL_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x400000;
			break;
		case NiTexturingProperty::GLOSS_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x500000;
			break;
		case NiTexturingProperty::GLOW_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x600000;
			break;
		case NiTexturingProperty::BUMP_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x700000;
			break;
		case NiTexturingProperty::NORMAL_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x800000;
			break;
		case NiTexturingProperty::PARALLAX_INDEX:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF0FFFFFF) | 0x900000;
			break;
		case NiTexturingProperty::DECAL_BASE:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF00FFFFF) | (uiInstance | 0x1000000);
			break;
		case NiTexturingProperty::SHADER_BASE:
			pkStage->m_uiTextureFlags = (pkStage->m_uiTextureFlags & 0xF00FFFFF) | (uiInstance | 0x2000000);
			break;
		default:
			return false;
		}
	} else if (NiTextureEffect::GetTypeIDFromName(kSemantic, uiTextureID)) {
		// The object texture type (NiShaderAttributeDesc::OT_EFFECT_*) in bits 8-15.
		uint32_t uiObjTextureFlags = 0;
		switch (uiTextureID) {
		case NiTextureEffect::PROJECTED_LIGHT:
			uiObjTextureFlags = 0x700;
			break;
		case NiTextureEffect::PROJECTED_SHADOW:
			uiObjTextureFlags = 0x600;
			break;
		case NiTextureEffect::ENVIRONMENT_MAP:
			uiObjTextureFlags = 0x500;
			break;
		case NiTextureEffect::FOG_MAP:
			uiObjTextureFlags = 0x800;
			break;
		}
		pkStage->m_usObjTextureFlags = static_cast<uint16_t>((uiObjTextureFlags & 0xFF00) | uiInstance);
	}
	return true;
}

void NiDX9FragmentShader::DestroyRendererData() {
	NiD3DShader::DestroyRendererData();
}

bool NiDX9FragmentShader::Initialize(NiDX9Renderer* pkRenderer) {
	return m_bInitialized || NiD3DShader::Initialize();
}

void NiDX9FragmentShader::PrepareTextureStage(NiD3DTextureStage* pkStage, NiTexturingProperty* pkTexProp,
	const NiDynamicEffectState* pkEffects) {
	uint16_t usObjTextureFlags = pkStage->m_usObjTextureFlags;
	uint32_t uiTextureFlags = pkStage->m_uiTextureFlags;
	NiTexture* pkTexture = nullptr;
	NiTexturingProperty::ClampMode eClampMode = NiTexturingProperty::CLAMP_S_CLAMP_T;
	NiTexturingProperty::FilterMode eFilterMode = NiTexturingProperty::FILTER_NEAREST;
	if (usObjTextureFlags == 0) {
		if (uiTextureFlags != 0) {
			NiTexturingProperty::Map* pkMap = nullptr;
			bool bFound = true;
			uint32_t uiIndex = uiTextureFlags & 0xFFFF;
			switch (uiTextureFlags & 0x0F000000) {
			case 0:
				// The NDL map: 0x200000 (base) to 0x900000 (parallax).
				switch (uiTextureFlags & 0x00F00000) {
				case 0x200000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::BASE_INDEX];
					break;
				case 0x300000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::DARK_INDEX];
					break;
				case 0x400000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::DETAIL_INDEX];
					break;
				case 0x500000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::GLOSS_INDEX];
					break;
				case 0x600000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::GLOW_INDEX];
					break;
				case 0x700000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::BUMP_INDEX];
					break;
				case 0x800000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::NORMAL_INDEX];
					break;
				case 0x900000:
					pkMap = pkTexProp->m_kMaps.m_pBase[NiTexturingProperty::PARALLAX_INDEX];
					break;
				default:
					bFound = false;
					break;
				}
				break;
			case 0x1000000:
				// A decal map: the decals follow the eight NDL maps. The compare is unsigned.
				if (uiIndex < static_cast<uint32_t>(pkTexProp->m_kMaps.m_usSize - 8)) {
					pkMap = pkTexProp->m_kMaps.m_pBase[uiIndex + NiTexturingProperty::DECAL_BASE];
				} else {
					bFound = false;
				}
				break;
			case 0x2000000:
				if (pkTexProp->m_pkShaderMaps && uiIndex < pkTexProp->m_pkShaderMaps->m_usSize) {
					pkMap = pkTexProp->GetShaderMap(uiIndex);
				} else {
					bFound = false;
				}
				break;
			default:
				bFound = false;
				break;
			}
			if (bFound && pkMap) {
				// EXE-INLINED 0x5585E0
				pkTexture = pkMap->m_spTexture.m_pObject;
				eClampMode = static_cast<NiTexturingProperty::ClampMode>((pkMap->m_uFlags >> 12) & 3);
				eFilterMode = static_cast<NiTexturingProperty::FilterMode>((pkMap->m_uFlags >> 8) & 0xF);
				// TSTF_MAP_USE_INDEX
				if ((uiTextureFlags & 0xF0000000) == 0x10000000) {
					uint32_t uiTexCoordIndex = pkMap->m_uFlags & 0xFF;
					uiTexCoordIndex |= pkStage->GetTexCoordIndex() & 0xFFFF0000;
					bool bSave = pkStage->GetSaveTexCoordIndex();
					pkStage->m_pkTextureStageGroup->SetStageState(D3DTSS_TEXCOORDINDEX, uiTexCoordIndex, bSave);
				}
			}
		}
	} else if (pkEffects) {
		NiTextureEffect* pkTextureEffect = nullptr;
		uint16_t usObjectIndex = usObjTextureFlags & 0xFF;
		// NiShaderAttributeDesc::OT_EFFECT_* in bits 8-15.
		switch (usObjTextureFlags >> 8) {
		case 5:
			pkTextureEffect = pkEffects->m_pEnvMap;
			if (pkTextureEffect && !pkTextureEffect->m_bOn)
				pkTextureEffect = nullptr;
			break;
		case 6: {
			uint16_t usIndex = 0;
			for (NiDynamicEffectState::ListItem* pkItem = pkEffects->m_pProjShadowList; pkItem;) {
				NiTextureEffect* pkProjShadow = static_cast<NiTextureEffect*>(pkItem->m_pItem);
				pkItem = pkItem->m_pNext;
				if (pkProjShadow && pkProjShadow->m_bOn == true) {
					if (usIndex++ == usObjectIndex) {
						pkTextureEffect = pkProjShadow;
						break;
					}
				}
			}
			break;
		}
		case 7: {
			uint16_t usIndex = 0;
			for (NiDynamicEffectState::ListItem* pkItem = pkEffects->m_pProjLightList; pkItem;) {
				NiTextureEffect* pkProjLight = static_cast<NiTextureEffect*>(pkItem->m_pItem);
				pkItem = pkItem->m_pNext;
				if (pkProjLight && pkProjLight->m_bOn == true) {
					if (usIndex++ == usObjectIndex) {
						pkTextureEffect = pkProjLight;
						break;
					}
				}
			}
			break;
		}
		case 8:
			pkTextureEffect = pkEffects->m_pFogMap;
			if (pkTextureEffect && !pkTextureEffect->m_bOn)
				pkTextureEffect = nullptr;
			break;
		}
		if (pkTextureEffect) {
			pkTexture = pkTextureEffect->m_spTexture.m_pObject;
			eClampMode = pkTextureEffect->m_eClamp;
			eFilterMode = pkTextureEffect->m_eFilter;
			if ((uiTextureFlags & 0xF0000000) == 0x10000000) {
				NiDX9Renderer* pkD3DRenderer = m_pkD3DRenderer;
				NiD3DShader::PackDynamicEffect(pkTextureEffect, pkStage, pkStage->GetSaveTexCoordIndex(),
					pkD3DRenderer);
			}
		}
	}
	pkStage->m_spTexture = pkTexture;
	pkStage->SetClampMode(eClampMode);
	pkStage->SetFilterMode(eFilterMode);
}
