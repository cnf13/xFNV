#include "BSShader/Lighting30ShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI Lighting30ShaderProperty::ms_RTTI("Lighting30ShaderProperty", &BSShaderPPLightingProperty::ms_RTTI);

Lighting30ShaderProperty::Lighting30ShaderProperty() {
	pRenderDepthPass = nullptr;
}

Lighting30ShaderProperty::Lighting30ShaderProperty(BSShaderPPLightingProperty* aPPLightingProp) {
	aPPLightingProp->CopyTo(this);
	iShader = 29;
	iShaderPropertyType = 12;
}

Lighting30ShaderProperty::~Lighting30ShaderProperty() {}

// The clone is built while the thread's heap context is MEM_CONTEXT_HAIRSHADER: the exe inlines the context guard's
// save and restore around the allocation and the copy body. NoFaderShaderProperty's slot holds the same address
// (ICF); registered here.
NiObject* Lighting30ShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
	AutoMemContext memContext(MEM_CONTEXT_HAIRSHADER);
	Lighting30ShaderProperty* pClone =
		static_cast<Lighting30ShaderProperty*>(NiMemObject::operator new(sizeof(Lighting30ShaderProperty)));
	if (pClone) {
		// EXE-INLINED 0xBB4520
		pClone = ::new (pClone) Lighting30ShaderProperty;
	}
	CopyMembers(pClone, arCloneData);
	return pClone;
}

void Lighting30ShaderProperty::LoadBinary(NiStream& kStream) {
	BSShaderPPLightingProperty::LoadBinary(kStream);
}

void Lighting30ShaderProperty::LinkObject(NiStream& kStream) {
	BSShaderPPLightingProperty::LinkObject(kStream);
}

bool Lighting30ShaderProperty::RegisterStreamables(NiStream& kStream) {
	return BSShaderPPLightingProperty::RegisterStreamables(kStream);
}

void Lighting30ShaderProperty::SaveBinary(NiStream& kStream) {
	BSShaderPPLightingProperty::SaveBinary(kStream);
}

void Lighting30ShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSShaderPPLightingProperty::GetViewerStrings(apStrings);
	apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The exe's slot is a bare jmp to BSShaderPPLightingProperty::PostLinkObject.
void Lighting30ShaderProperty::PostLinkObject(NiStream& kStream) {
	BSShaderPPLightingProperty::PostLinkObject(kStream);
}

bool Lighting30ShaderProperty::SetupGeometry(NiGeometry* apGeometry) {
	bool bResult = BSShaderPPLightingProperty::SetupGeometry(apGeometry);
	// BSSP_FLAG_DYNAMICALPHA
	if (ulFlags[0] & 0x80000)
		fDepthBias = 1.0f;
	NiProperty* pProperty = apGeometry->GetProperty(NiProperty::VERTEX_COLOR);
	if (pProperty)
		apGeometry->DetachProperty(pProperty);
	return bResult;
}

// The passes of a geometry for the accumulator's render mode. The depth-map, constant-alpha, local-map and silhouette
// modes build their lists once. The normal mode measures the camera distance, drops the cached list when the distance
// crosses an LOD end (eye or plain env map, decal, specular), fades decals out, refreshes the lights of the cached
// passes, and rebuilds the list when the state (enabled passes and mode) differs from the last one built. The exe
// keeps several flags in reused stack and argument slots; here each is its own local.
BSShaderProperty::RenderPassArray* Lighting30ShaderProperty::GetRenderPasses(NiGeometry* apGeom,
	int32_t aiEnabledPasses, uint16_t& asCount, uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator,
	int32_t abUpdateList) {
	const uint32_t uiState = static_cast<uint32_t>(aiEnabledPasses) | (auiRenderMode << 8);
	const uint32_t uiFlags1 = ulFlags[1];
	const bool bStinger = (uiFlags1 & BSS2_stinger_prop) != 0;
	bool bEnvMapInRange = true;
	bool bDecalInRange = true;
	bool bFaded = false;

	switch (auiRenderMode) {
	case 7: {
		const uint32_t uiFlags0 = ulFlags[0];
		if ((uiFlags0 & BSS1_Refraction) || (uiFlags0 & BSS1_Fire_Refraction))
			return nullptr;
		InitRenderPassArray_depthMap(1);
		RenderPassArray* pArray = pDepthMapRenderPassList;
		if (pArray->iSize) {
			RenderPass* pPass = pArray->pPassA.m_pBase[0];
			if (pPass->ucNumLights) {
				pPass->ppSceneLights[0] =
					static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
						->spShadowLight.m_pObject;
			}
		} else if (apGeom->m_spModelData->m_pkColor) {
			const bool bSkinned = GetFlag(BSSP_SKINNED);
			ShadowSceneLight* pLight = static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
										   ->spShadowLight.m_pObject;
			pArray->Add(apGeom, bSkinned ? 459 : 457, true, 1, pLight, nullptr, nullptr, nullptr);
		} else {
			const bool bSkinned = GetFlag(BSSP_SKINNED);
			ShadowSceneLight* pLight = static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
										   ->spShadowLight.m_pObject;
			pArray->Add(apGeom, bSkinned ? 458 : 456, true, 1, pLight, nullptr, nullptr, nullptr);
		}
		return pDepthMapRenderPassList;
	}
	case 8: {
		const uint32_t uiFlags0 = ulFlags[0];
		// Refraction, fire refraction or refraction tint: the list as it is.
		if ((uiFlags0 & BSS1_Refraction) || (uiFlags0 & BSS1_Fire_Refraction) || (uiFlags1 & BSS2_Refraction_Tint))
			return pConstAlphaRenderPassList;
		InitRenderPassArray_constAlpha(1);
		RenderPassArray* pArray = pConstAlphaRenderPassList;
		if (pArray->iSize) {
			RenderPass* pPass = pArray->pPassA.m_pBase[0];
			if (pPass->ucNumLights) {
				pPass->ppSceneLights[0] =
					static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
						->spShadowLight.m_pObject;
			}
		} else {
			const bool bVertexColors = apGeom->m_spModelData->m_pkColor != nullptr;
			const bool bSkinned = GetFlag(BSSP_SKINNED);
			ShadowSceneLight* pLight = static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
										   ->spShadowLight.m_pObject;
			uint16_t usPass;
			if (bSkinned)
				usPass = bVertexColors ? 470 : 469;
			else
				usPass = bVertexColors ? 468 : 467;
			pArray->Add(apGeom, usPass, true, 1, pLight, nullptr, nullptr, nullptr);
		}
		return pConstAlphaRenderPassList;
	}
	case 9: {
		InitRenderPassArray_localMap(1);
		RenderPassArray* pArray = pLocalMapRenderPassList;
		if (!pArray->iSize) {
			if (ulFlags[0] & BSS1_Localmap_Hide_Secret)
				pArray->Add(apGeom, 479, true, 0, nullptr, nullptr, nullptr, nullptr);
			else
				pArray->Add(apGeom, GetFlag(BSSP_SKINNED) ? 478 : 477, true, 0, nullptr, nullptr, nullptr, nullptr);
		}
		return pLocalMapRenderPassList;
	}
	case 15: {
		CheckSilhouetteRenderPassArray(1);
		RenderPassArray* pArray = pSilhouettePassList;
		if (!pArray->iSize && !apGeom->m_spSkinInstance.m_pObject) {
			pArray->Add(apGeom, GetFlag(BSSP_FLAG_VERTEXCOLORS) ? 444 : 443, true, 0, nullptr, nullptr, nullptr,
				nullptr);
		}
		return pSilhouettePassList;
	}
	default:
		break;
	}

	// The LOD fades that apply, from the flags and the LOD ends.
	const float fOldDistance = fCameraDistance;
	const uint32_t uiFlags0 = ulFlags[0];
	const bool bEnvMap = (uiFlags0 & BSS1_Environment_Mapping) && BSShaderManager::fEnvMapLODEnd > 0.0f;
	const bool bEyeEnvMap = (uiFlags0 & BSS1_Eye_Environment_Mapping) && BSShaderManager::fEyeEnvMapLODEnd > 0.0f;
	const bool bDecal = ((uiFlags0 & BSS1_Decal_Single_Pass) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) &&
		BSShaderManager::fDecalLODEnd > 0.0f && !GetFlag(BSSP_FLAG_NOFADE);
	bool bSpecular = (uiFlags0 & BSS1_Specular) && BSShaderManager::fSpecularLODEnd > 0.0f;

	if (bEyeEnvMap || bEnvMap || bDecal || bSpecular) {
		const NiPoint3 kEyePos = apAccumulator->pActiveShadowSceneNode->kEyePosition;
		const NiBound* pWorldBound = apGeom->m_kWorldBound;
		if (!pWorldBound)
			pWorldBound = &NiAVObject::NullBoundS;
		const NiBound kBound = *pWorldBound;
		// EXE-INLINED 0x439EF0
		NiPoint3 kDiff;
		kDiff.x = kBound.m_kCenter.x - kEyePos.x;
		kDiff.y = kBound.m_kCenter.y - kEyePos.y;
		kDiff.z = kBound.m_kCenter.z - kEyePos.z;
		fCameraDistance = kDiff.Length() - kBound.m_fRadius;
		if (BSShaderManager::GetAccumulator()->m_pkCamera)
			fCameraDistance = BSShaderManager::GetAccumulator()->m_pkCamera->m_fLODAdjust * fCameraDistance;

		// A distance that crossed an LOD end since the last call, either way, drops the cached list.
		if (fOldDistance != fCameraDistance) {
			if (bEyeEnvMap) {
				const float fEnd = BSShaderManager::fEyeEnvMapLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance))
					iLastRenderPassState = 0;
			} else if (bEnvMap) {
				const float fEnd = BSShaderManager::fEnvMapLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance))
					iLastRenderPassState = 0;
			}
			if (bDecal) {
				const float fEnd = BSShaderManager::fDecalLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance))
					iLastRenderPassState = 0;
			}
			if (bSpecular && !bStinger) {
				const float fEnd = BSShaderManager::fSpecularLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance))
					iLastRenderPassState = 0;
				if (fCameraDistance >= BSShaderManager::fSpecularLODEnd)
					bSpecular = false;
			}
		}
	}
	if (!bStinger && BSShaderManager::fEnvMapLODEnd != 0.0f && fCameraDistance >= BSShaderManager::fEnvMapLODEnd)
		bEnvMapInRange = false;

	// The alpha: decals fade out between fDecalLODStartFade and fDecalLODEnd and are dropped past it.
	NiPropertyState* pState = &apGeom->m_PropertyState;
	if (pState) {
		float fNewAlpha = 1.0f;
		NiMaterialProperty* pMaterial =
			static_cast<NiMaterialProperty*>(pState->m_aspProps[NiProperty::MATERIAL].m_pObject);
		if (!pMaterial) {
			fNewAlpha = fFadeAlpha;
		} else if (!bDecal) {
			fNewAlpha = pMaterial->m_fAlpha * fFadeAlpha;
		} else if (BSShaderManager::fDecalLODEnd != 0.0f && fCameraDistance >= BSShaderManager::fDecalLODEnd) {
			bDecalInRange = false;
		} else if (BSShaderManager::fDecalLODStartFade != 0.0f && BSShaderManager::fDecalLODEnd != 0.0f &&
			fCameraDistance >= BSShaderManager::fDecalLODStartFade && fCameraDistance < BSShaderManager::fDecalLODEnd) {
			fNewAlpha =
				FLerp(0.0f, 1.0f, BSShaderManager::fDecalLODEnd, BSShaderManager::fDecalLODStartFade, fCameraDistance);
		} else {
			fNewAlpha = pMaterial->m_fAlpha * fFadeAlpha;
		}
		SetAlpha(fNewAlpha);
		if (fNewAlpha < 1.0f)
			bFaded = true;
	}

	if (bLightListChanged) {
		const NiBound* pWorldBound = apGeom->m_kWorldBound;
		if (!pWorldBound)
			pWorldBound = &NiAVObject::NullBoundS;
		NiBound kObjBound = *pWorldBound;
		ResortLights(&kObjBound);
	}

	// The same state with new lights: put the lights into the cached passes.
	if (iLastRenderPassState == uiState && bLightListChanged) {
		ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
		for (uint32_t i = 0; i < pRenderPassList->iSize; ++i) {
			RenderPass* pPass = pRenderPassList->pPassA.m_pBase[i];
			const uint16_t usPass = pPass->usPassEnum;
			if (usPass >= 383 && usPass <= 442) {
				void* pIter;
				uint8_t ucNewLights = 1;
				if (GetFirstActiveNonShadowLight(pIter)) {
					do {
						++ucNewLights;
					} while (GetNextActiveNonShadowLight(pIter));
					if (!ucNewLights) {
						pPass->ucNumLights = 0;
						continue;
					}
				}
				if (pPass->ucMaxNumLights < ucNewLights) {
					pPass->ClearSceneLights();
					pPass->AllocateLightArray(ucNewLights);
					pPass->ucMaxNumLights = ucNewLights;
				}
				ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter);
				pPass->ppSceneLights[0] = pSunLight;
				int32_t iLight = 1;
				for (; pLight && iLight < ucNewLights; pLight = GetNextActiveNonShadowLight(pIter))
					pPass->ppSceneLights[iLight++] = pLight;
				pPass->ucNumLights = static_cast<uint8_t>(iLight);
			} else if (usPass >= 445 && usPass <= 451) {
				if (pPass->ucNumLights == 1) {
					void* pIter;
					ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter);
					pPass->ppSceneLights[0] = pLight ? pLight : pSunLight;
				}
			} else if (usPass >= 452 && usPass <= 455) {
				if (pPass->ucNumLights == 1) {
					void* pIter;
					ShadowSceneLight* pLight = GetFirstActiveLight(pIter);
					// No shadow-casting light left, or its light is culled: rebuild the list.
					if (!pLight || !pLight->bCastShadow ||
						(pLight->spLight.m_pObject->m_uFlags & NiAVObject::APP_CULLED)) {
						iLastRenderPassState = 0;
						break;
					}
					pPass->ppSceneLights[0] = pLight;
				}
			}
		}
		bLightListChanged = false;
	}

	if (iLastRenderPassState == uiState)
		return pRenderPassList;

	CheckCreateRenderPassArray(-1);
	pRenderPassList->iSize = 0;
	bLightListChanged = false;
	const bool bLit = !(ulFlags[0] & BSS1_Localmap_Hide_Secret) || BSShaderManager::bLiteBrite;
	const bool bDecalDrawn = bDecalInRange && (GetFlag(BSSP_FLAG_DECAL) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL));
	if (fAlpha == 0.0f || !bLit || (bDecal && !bDecalDrawn))
		return pRenderPassList;

	const bool bSkinned = GetFlag(BSSP_SKINNED);
	const bool bEnvMapPass = bEnvMapInRange && GetFlag(BSSP_ENVMAP);
	const bool bEyeReflect = GetFlag(BSSP_FLAG_EYEREFLECT);
	const bool bWindowReflect = GetFlag(BSSP_FLAG_WINDOWREFLECT);
	const bool bRefraction = GetFlag(BSSP_FLAG_REFRACTION);
	const bool bRefractionFire = GetFlag(BSSP_FLAG_REFRACTION_FIRE);
	const bool bParallax = GetFlag(BSSP_FLAG_PARALLAX);
	const bool bParallaxOcc = GetFlag(BSSP_FLAG_PARALLAXOCCLUSION);
	const bool bFaceGen = GetFlag(BSSP_FLAG_FACEGENBLEND);
	const bool bGlowMap = spTextures[TEXTYPE_GLOW][0].m_pObject != nullptr;
	const bool bSpecularPass = GetFlag(BSSP_SPECULAR) && (uiState & 8) && bSpecular;
	// The exe reads the alpha property without a null test.
	const bool bAlphaBlend = bFaded ||
		(static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject)->m_uFlags &
			NiAlphaProperty::ALPHA_BLEND_MASK);
	bool bProjShadow = BSShaderManager::GetUseProjectedShadow() && !GetFlag(BSSP_FLAG_NOPROJSHADOW) &&
		ShadowLightShader::GetProjectedShadowScale() &&
		BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 1 &&
		BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 3;
	const bool bVertexColors = apGeom->m_spModelData->m_pkColor || GetFlag(BSSP_FLAG_VERTEXCOLORS);
	if (spTexEffectData.m_pObject)
		bProjShadow = false;

	if (bRefraction) {
		if (bRefractionFire) {
			pRenderPassList->Add(apGeom, 464, true, 0, nullptr, nullptr, nullptr, nullptr);
		} else {
			pRenderPassList->Add(apGeom, bVertexColors ? 2 * bSkinned + 461 : 2 * bSkinned + 460, true, 0, nullptr,
				nullptr, nullptr, nullptr);
		}
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_REFRACT;
		iLastRenderPassState = uiState;
		return pRenderPassList;
	}

	if (ulFlags[1] & BSS2_1st_person) {
		pRenderPassList->Add(apGeom, bSkinned + 465, true, 0, nullptr, nullptr, nullptr, nullptr);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint =
			BSSP_HINT_GROUP_1STPERSONNOREFRACT;
	}

	// The main pass: the sun first, then every active light that casts no shadow.
	ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
	pRenderPassList->Add(apGeom, 0, true, 0, nullptr, nullptr, nullptr, nullptr);
	RenderPass* pPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
	void* pIter;
	pPass->ucNumLights = 1;
	for (ShadowSceneLight* pLight = GetFirstActiveLight(pIter); pLight; pLight = GetNextActiveLight(pIter)) {
		if (!pLight->bCastShadow)
			++pPass->ucNumLights;
	}
	if (pPass->ucNumLights) {
		if (pPass->ucMaxNumLights < pPass->ucNumLights) {
			pPass->ClearSceneLights();
			pPass->AllocateLightArray(pPass->ucNumLights);
			pPass->ucMaxNumLights = pPass->ucNumLights;
		}
		ShadowSceneLight* pLight = GetFirstActiveLight(pIter);
		pPass->ppSceneLights[0] = pSunLight;
		uint8_t ucLight = 1;
		for (; pLight; pLight = GetNextActiveLight(pIter)) {
			if (!pLight->bCastShadow)
				pPass->ppSceneLights[ucLight++] = pLight;
		}
		pPass->ucNumLights = ucLight;
	} else {
		pPass->ppSceneLights = nullptr;
	}
	SetFlag(BSSP_FLAG_SHADOWMAP, false);

	// The main pass's shader. Skinned parallax has none of its own and keeps pass 0.
	if (GetFlag(BSSP_FLAG_HAIR)) {
		if (bSpecularPass)
			pPass->usPassEnum = bProjShadow ? 430 : 400;
		else
			pPass->usPassEnum = bProjShadow ? 415 : 385;
	} else if (bSpecularPass) {
		if (bSkinned) {
			if (bParallax) {
			} else if (bFaceGen) {
				pPass->usPassEnum = bProjShadow ? 434 : 404;
			} else if (bGlowMap) {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 442 : 412;
				else
					pPass->usPassEnum = bProjShadow ? 436 : 406;
			} else {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 438 : 408;
				else
					pPass->usPassEnum = bProjShadow ? 429 : 399;
			}
		} else {
			if (bParallax) {
				if (bVertexColors) {
					if (bProjShadow)
						pPass->usPassEnum = bParallaxOcc ? 440 : 439;
					else
						pPass->usPassEnum = bParallaxOcc ? 410 : 409;
				} else {
					if (bProjShadow)
						pPass->usPassEnum = bParallaxOcc ? 432 : 431;
					else
						pPass->usPassEnum = bParallaxOcc ? 402 : 401;
				}
			} else if (bFaceGen) {
				pPass->usPassEnum = bProjShadow ? 433 : 403;
			} else if (bGlowMap) {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 441 : 411;
				else
					pPass->usPassEnum = bProjShadow ? 435 : 405;
			} else {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 437 : 407;
				else
					pPass->usPassEnum = bProjShadow ? 428 : 398;
			}
		}
	} else {
		if (bSkinned) {
			if (bParallax) {
			} else if (bFaceGen) {
				pPass->usPassEnum = bProjShadow ? 419 : 389;
			} else if (bGlowMap) {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 427 : 397;
				else
					pPass->usPassEnum = bProjShadow ? 421 : 391;
			} else {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 423 : 393;
				else
					pPass->usPassEnum = bProjShadow ? 414 : 384;
			}
		} else {
			if (bParallax) {
				if (bVertexColors) {
					if (bProjShadow)
						pPass->usPassEnum = bParallaxOcc ? 425 : 424;
					else
						pPass->usPassEnum = bParallaxOcc ? 395 : 394;
				} else {
					if (bProjShadow)
						pPass->usPassEnum = bParallaxOcc ? 417 : 416;
					else
						pPass->usPassEnum = bParallaxOcc ? 387 : 386;
				}
			} else if (bFaceGen) {
				pPass->usPassEnum = bProjShadow ? 418 : 388;
			} else if (bGlowMap) {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 426 : 396;
				else
					pPass->usPassEnum = bProjShadow ? 420 : 390;
			} else {
				if (bVertexColors)
					pPass->usPassEnum = bProjShadow ? 422 : 392;
				else
					pPass->usPassEnum = bProjShadow ? 413 : 383;
			}
		}
	}

	if (GetFlag(BSSP_FLAG_DECAL)) {
		pPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
	} else if (GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
		pPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DYNAMIC_DECAL;
	} else if (bAlphaBlend) {
		// NaN sorts.
		pPass->eAccumulationHint = fFadeAlpha < 1.0f ? BSSP_HINT_GROUP_FADING : BSSP_HINT_SORT;
	} else if (apAccumulator->eAccumulationMode == BSShaderAccumulator::BSSA_ACCUMLOD ||
		GetFlag(BSSP_FLAG_LODBUILDING) || GetFlag(BSSP_FLAG_LODLANDSCAPE)) {
		pPass->eAccumulationHint = BSSP_HINT_GROUP_LOD;
	}

	// One shadow pass per active shadow-casting light.
	if (BSShaderManager::GetShadows()) {
		for (ShadowSceneLight* pLight = GetFirstActiveLight(pIter); pLight; pLight = GetNextActiveLight(pIter)) {
			if (!pLight->bCastShadow)
				continue;
			pRenderPassList->Add(apGeom, 0, false, 1, pLight, nullptr, nullptr, nullptr);
			RenderPass* pShadowPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pShadowPass->usPassEnum = bVertexColors ? 2 * bSkinned + 453 : 2 * bSkinned + 452;
			if (GetFlag(BSSP_FLAG_DECAL)) {
				pShadowPass->eAccumulationHint =
					GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
			} else if (GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
				pShadowPass->eAccumulationHint =
					GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DYNAMIC_DECAL;
			}
		}
	}

	BSShaderPPLightingProperty::TextureEffectData* pTexEffectData = spTexEffectData.m_pObject;
	if (pTexEffectData && pTexEffectData->spBlockOutTexture.m_pObject) {
		pRenderPassList->Add(apGeom, bSkinned + 14, true, 0, nullptr, nullptr, nullptr, nullptr);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_FADING;
	} else if (GetFlag(BSSP_ZPREPASS) || fFadeAlpha < 1.0f) {
		NiAlphaProperty* pAlpha =
			static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject);
		if (!pAlpha || !(pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK)) {
			pRenderPassList->Add(apGeom, bVertexColors ? bSkinned + 12 : bSkinned + 10, true, 0, nullptr, nullptr,
				nullptr, nullptr);
			RenderPass* pDepthPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pDepthPass->eAccumulationHint = GetFlag(BSSP_ZPREPASS) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_FADING;
			if (fFadeAlpha < 1.0f || fAlpha < 1.0f)
				pDepthPass->eAccumulationHint = BSSP_HINT_GROUP_FADING;
		}
	}

	if (spTexEffectData.m_pObject) {
		pRenderPassList->Add(apGeom, bSkinned + 480, true, 0, nullptr, nullptr, nullptr, nullptr);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_SORT;
	}

	if (bEnvMapPass) {
		RenderPass* pEnvPass;
		if (bWindowReflect) {
			const uint16_t usPass = 2 * bVertexColors + 446;
			pRenderPassList->Add(apGeom, 0, false, 0, nullptr, nullptr, nullptr, nullptr);
			pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pEnvPass->usPassEnum = usPass;
		} else if (bEyeReflect) {
			ShadowSceneLight* pLight = GetFirstActiveLight(pIter);
			pRenderPassList->Add(apGeom, 0, false, 1, pLight ? pLight : pSunLight, nullptr, nullptr, nullptr);
			pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pEnvPass->usPassEnum = 451;
		} else {
			const uint16_t usPass = bSkinned ? bVertexColors + 449 : 2 * bVertexColors + 445;
			pRenderPassList->Add(apGeom, 0, false, 0, nullptr, nullptr, nullptr, nullptr);
			pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pEnvPass->usPassEnum = usPass;
		}
		if (bAlphaBlend)
			pEnvPass->eAccumulationHint = BSSP_HINT_SORT;
	}

	iLastRenderPassState = uiState;
	return pRenderPassList;
}

BSShaderProperty::RenderPass* Lighting30ShaderProperty::GetRenderDepthPass(NiGeometry* apGeom) {
	// BSSP_FLAG_DECAL, BSSP_FLAG_DYNAMIC_DECAL
	if (ulFlags[0] & 0x4000000)
		return nullptr;
	if (ulFlags[0] & 0x8000000)
		return nullptr;
	const bool bVertexColors = apGeom->m_spModelData->m_pkColor != nullptr;
	if (!pRenderDepthPass) {
		uint32_t uiPass;
		// BSSP_SKINNED
		if (ulFlags[0] & 2)
			uiPass = bVertexColors ? 0x1DA : 0x1D9;
		else
			uiPass = bVertexColors ? 0x1D8 : 0x1D7;
		void* pMemory = MemoryManager_Singleton.Allocate(sizeof(RenderPass));
		pRenderDepthPass =
			pMemory ? ::new (static_cast<RenderPass*>(pMemory)) RenderPass(apGeom, uiPass, true, 0, nullptr) : nullptr;
	}
	return pRenderDepthPass;
}

BSShaderProperty::RenderPassArray* Lighting30ShaderProperty::GetWaterFogPassList(NiGeometry* apGeom) {
	// NaN builds the list.
	if (fRefractionPower > 0.0f)
		return nullptr;
	const NiAlphaProperty* pAlpha =
		static_cast<const NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject);
	const bool bAlphaTest = pAlpha && ((pAlpha->m_uFlags >> 9) & 1);
	const bool bVertexColors = apGeom->m_spModelData->m_pkColor != nullptr;
	CheckCreateWaterFogRenderPassArray(1);
	if (pWaterFogRenderPassList->iSize == 0) {
		uint32_t uiPass;
		// BSSP_SKINNED
		if (ulFlags[0] & 2) {
			if (bVertexColors)
				uiPass = bAlphaTest ? 0x2ED : 0x2E7;
			else
				uiPass = bAlphaTest ? 0x2E1 : 0x2DB;
		} else {
			if (bVertexColors)
				uiPass = bAlphaTest ? 0x2EC : 0x2E6;
			else
				uiPass = bAlphaTest ? 0x2E0 : 0x2DA;
		}
		pWaterFogRenderPassList->Add(apGeom, uiPass, true, 0, nullptr, nullptr, nullptr, nullptr);
	}
	return pWaterFogRenderPassList;
}

void Lighting30ShaderProperty::CopyMembers(Lighting30ShaderProperty* pDest, NiCloningProcess& arCloneData) {
	BSShaderLightingProperty::CopyMembers(pDest, arCloneData);
	CopyToMembers(pDest);
}

// The stream's factory: the class's default constructor (0xBB4520, inlined by the exe) in an engine block. The
// exe's C++ EH (unwind) frame is left out.
NiObject* Lighting30ShaderProperty::CreateObject() {
	Lighting30ShaderProperty* pProperty =
		static_cast<Lighting30ShaderProperty*>(NiMemObject::operator new(sizeof(Lighting30ShaderProperty)));
	if (pProperty) {
		pProperty = ::new (pProperty) Lighting30ShaderProperty;
	}
	return pProperty;
}
