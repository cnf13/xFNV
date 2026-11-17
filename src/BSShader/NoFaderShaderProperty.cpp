#include "BSShader/NoFaderShaderProperty.h"

#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/Lighting30ShaderProperty.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"

NoFaderShaderProperty::NoFaderShaderProperty() {
	pRenderDepthPass = nullptr;
	iShader = 29;
	iShaderPropertyType = 12;
	if ((ulFlags[1] & 0x800000u) == 0) {
		iLastRenderPassState = 0;
	}
	ulFlags[1] |= 0x800000u;
}

NoFaderShaderProperty::~NoFaderShaderProperty() {}

// The passes of the shader: the depth map, constant alpha, local map and silhouette modes have their own arrays;
// every other mode rebuilds the lit passes when the pass state changed or the camera crossed an end distance of the
// environment map, decal or specular LOD, and else only refreshes the lights of the passes it already has.
BSShaderProperty::RenderPassArray* NoFaderShaderProperty::GetRenderPasses(NiGeometry* apGeom, int32_t aiEnabledPasses,
	uint16_t& asCount, uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) {
	uint32_t uiPassState = aiEnabledPasses | auiRenderMode << 8;
	NiPropertyState* pState = &apGeom->m_PropertyState;
	bool bDecalVisible = true;
	bool bFading = false;
	if (auiRenderMode == 7) {
		uint32_t uiFlags = ulFlags[0];
		if ((uiFlags & BSS1_Refraction) || (uiFlags & BSS1_Fire_Refraction)) {
			return nullptr;
		}
		InitRenderPassArray_depthMap(1);
		RenderPassArray* pArray = pDepthMapRenderPassList;
		if (pArray->iSize) {
			RenderPass* pPass = pArray->pPassA.m_pBase[0];
			if (pPass->ucNumLights) {
				pPass->ppSceneLights[0] =
					static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
						->spShadowLight.m_pObject;
			}
		} else if (apGeom->m_spModelData.m_pObject->m_pkColor) {
			bool bSkinned = GetFlag(BSSP_SKINNED);
			pArray->Add(apGeom, bSkinned ? 459 : 457, true, 1,
				static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
					->spShadowLight.m_pObject,
				nullptr, nullptr, nullptr);
		} else {
			bool bSkinned = GetFlag(BSSP_SKINNED);
			pArray->Add(apGeom, bSkinned ? 458 : 456, true, 1,
				static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
					->spShadowLight.m_pObject,
				nullptr, nullptr, nullptr);
		}
		return pDepthMapRenderPassList;
	}
	if (auiRenderMode == 8) {
		uint32_t uiFlags = ulFlags[0];
		if ((uiFlags & BSS1_Refraction) || (uiFlags & BSS1_Fire_Refraction) || (ulFlags[1] & BSS2_Refraction_Tint)) {
			return pConstAlphaRenderPassList;
		}
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
			bool bColors = apGeom->m_spModelData.m_pObject->m_pkColor != nullptr;
			bool bSkinned = GetFlag(BSSP_SKINNED);
			uint16_t usPass;
			if (bSkinned) {
				usPass = bColors ? 470 : 469;
			} else {
				usPass = bColors ? 468 : 467;
			}
			pArray->Add(apGeom, usPass, true, 1,
				static_cast<BSShaderAccumulator*>(NiRenderer::ms_pkRenderer->m_spAccum.m_pObject)
					->spShadowLight.m_pObject,
				nullptr, nullptr, nullptr);
		}
		return pConstAlphaRenderPassList;
	}
	if (auiRenderMode == 9) {
		InitRenderPassArray_localMap(1);
		RenderPassArray* pArray = pLocalMapRenderPassList;
		if (!pArray->iSize) {
			if (ulFlags[0] & BSS1_Localmap_Hide_Secret) {
				pArray->Add(apGeom, 479, true, 0, nullptr, nullptr, nullptr, nullptr);
			} else {
				pArray->Add(apGeom, GetFlag(BSSP_SKINNED) ? 478 : 477, true, 0, nullptr, nullptr, nullptr, nullptr);
			}
		}
		return pLocalMapRenderPassList;
	}
	if (auiRenderMode == 15) {
		CheckSilhouetteRenderPassArray(1);
		RenderPassArray* pArray = pSilhouettePassList;
		if (!pArray->iSize && !apGeom->m_spSkinInstance.m_pObject) {
			pArray->Add(apGeom, GetFlag(BSSP_FLAG_VERTEXCOLORS) ? 444 : 443, true, 0, nullptr, nullptr, nullptr,
				nullptr);
		}
		return pSilhouettePassList;
	}

	// The LOD bands this shader uses; the passes are rebuilt when the camera crosses the end of one.
	float fOldDistance = fCameraDistance;
	uint32_t uiFlags = ulFlags[0];
	bool bEnvLOD = (uiFlags & BSS1_Environment_Mapping) && 0.0f < BSShaderManager::fEnvMapLODEnd;
	bool bEyeEnvLOD = (uiFlags & BSS1_Eye_Environment_Mapping) && 0.0f < BSShaderManager::fEyeEnvMapLODEnd;
	bool bDecalLOD = ((uiFlags & BSS1_Decal_Single_Pass) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) &&
		0.0f < BSShaderManager::fDecalLODEnd && !GetFlag(BSSP_FLAG_NOFADE);
	bool bSpecularLOD = (uiFlags & BSS1_Specular) && 0.0f < BSShaderManager::fSpecularLODEnd;
	if (bEyeEnvLOD || bEnvLOD || bDecalLOD || bSpecularLOD) {
		NiPoint3 kEye = apAccumulator->pActiveShadowSceneNode->kEyePosition;
		const NiBound* pBound = apGeom->m_kWorldBound;
		if (!pBound) {
			pBound = &NiAVObject::NullBoundS;
		}
		NiBound kBound = *pBound;
		NiPoint3 kDiff;
		kDiff.x = kBound.m_kCenter.x - kEye.x;
		kDiff.y = kBound.m_kCenter.y - kEye.y;
		kDiff.z = kBound.m_kCenter.z - kEye.z;
		fCameraDistance = kDiff.Length() - kBound.m_fRadius;
		if (BSShaderManager::GetAccumulator()->m_pkCamera) {
			fCameraDistance = BSShaderManager::GetAccumulator()->m_pkCamera->m_fLODAdjust * fCameraDistance;
		}
		if (fOldDistance != fCameraDistance) {
			if (bEyeEnvLOD) {
				float fEnd = BSShaderManager::fEyeEnvMapLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance)) {
					iLastRenderPassState = 0;
				}
			} else if (bEnvLOD) {
				float fEnd = BSShaderManager::fEnvMapLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance)) {
					iLastRenderPassState = 0;
				}
			}
			if (bDecalLOD) {
				float fEnd = BSShaderManager::fDecalLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance)) {
					iLastRenderPassState = 0;
				}
			}
			if (bSpecularLOD) {
				float fEnd = BSShaderManager::fSpecularLODEnd;
				if ((fEnd > fCameraDistance && fEnd <= fOldDistance) ||
					(fCameraDistance >= fEnd && fEnd > fOldDistance)) {
					iLastRenderPassState = 0;
				}
				// The exe stores the flag again here (it is already set on this path).
				if (fCameraDistance >= fEnd) {
					bSpecularLOD = true;
				}
			}
		}
	}

	// The alpha: the material's times the fade, faded out over the decal LOD band; past its end the decal is hidden.
	if (pState) {
		NiMaterialProperty* pMaterial =
			static_cast<NiMaterialProperty*>(pState->m_aspProps[NiProperty::MATERIAL].m_pObject);
		float fNewAlpha = 1.0f;
		if (!pMaterial) {
			fNewAlpha = fFadeAlpha;
		} else if (!bDecalLOD) {
			fNewAlpha = pMaterial->m_fAlpha * fFadeAlpha;
		} else if (0.0f != BSShaderManager::fDecalLODEnd && fCameraDistance >= BSShaderManager::fDecalLODEnd) {
			bDecalVisible = false;
		} else if (BSShaderManager::fDecalLODStartFade != 0.0f && 0.0f != BSShaderManager::fDecalLODEnd &&
			fCameraDistance >= BSShaderManager::fDecalLODStartFade && fCameraDistance < BSShaderManager::fDecalLODEnd) {
			fNewAlpha =
				FLerp(0.0f, 1.0f, BSShaderManager::fDecalLODEnd, BSShaderManager::fDecalLODStartFade, fCameraDistance);
		} else {
			fNewAlpha = pMaterial->m_fAlpha * fFadeAlpha;
		}
		SetAlpha(fNewAlpha);
		if (fNewAlpha < 1.0f) {
			bFading = true;
		}
	}
	if (bLightListChanged) {
		const NiBound* pBound = apGeom->m_kWorldBound;
		if (!pBound) {
			pBound = &NiAVObject::NullBoundS;
		}
		NiBound kBound = *pBound;
		ResortLights(&kBound);
	}

	// Same passes: only give them the current lights, unless a shadow pass lost its light.
	if (iLastRenderPassState == uiPassState) {
		if (bLightListChanged) {
			ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
			for (uint32_t i = 0; i < pRenderPassList->iSize; ++i) {
				RenderPass* pPass = pRenderPassList->pPassA.m_pBase[i];
				uint16_t usPass = pPass->usPassEnum;
				if (usPass >= 383 && usPass <= 442) {
					void* pIter;
					uint8_t ucNewLights = 1;
					if (GetFirstActiveNonShadowLight(pIter)) {
						do {
							++ucNewLights;
						} while (GetNextActiveNonShadowLight(pIter));
					}
					if (!ucNewLights) {
						pPass->ucNumLights = 0;
						continue;
					}
					if (pPass->ucMaxNumLights < ucNewLights) {
						pPass->ClearSceneLights();
						pPass->AllocateLightArray(ucNewLights);
						pPass->ucMaxNumLights = ucNewLights;
					}
					ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter);
					pPass->ppSceneLights[0] = pSunLight;
					int32_t iLight = 1;
					while (pLight && iLight < ucNewLights) {
						pPass->ppSceneLights[iLight] = pLight;
						++iLight;
						pLight = GetNextActiveNonShadowLight(pIter);
					}
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
		if (iLastRenderPassState == uiPassState) {
			return pRenderPassList;
		}
	}

	CheckCreateRenderPassArray(-1);
	pRenderPassList->iSize = 0;
	bLightListChanged = false;
	bool bShown = !(ulFlags[0] & BSS1_Localmap_Hide_Secret) || BSShaderManager::bLiteBrite;
	bool bDecalShown = bDecalVisible && (GetFlag(BSSP_FLAG_DECAL) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL));
	if (0.0f == fAlpha || !bShown || (bDecalLOD && !bDecalShown)) {
		return pRenderPassList;
	}
	bool bSkinned = GetFlag(BSSP_SKINNED);
	bool bEnvMap = GetFlag(BSSP_ENVMAP);
	bool bEyeReflect = GetFlag(BSSP_FLAG_EYEREFLECT);
	bool bWindowReflect = GetFlag(BSSP_FLAG_WINDOWREFLECT);
	bool bRefraction = GetFlag(BSSP_FLAG_REFRACTION);
	bool bFireRefraction = GetFlag(BSSP_FLAG_REFRACTION_FIRE);
	bool bParallax = GetFlag(BSSP_FLAG_PARALLAX);
	bool bParallaxOcclusion = GetFlag(BSSP_FLAG_PARALLAXOCCLUSION);
	bool bFaceGen = GetFlag(BSSP_FLAG_FACEGENBLEND);
	bool bGlowMap = spTextures[TEXTYPE_GLOW][0].m_pObject != nullptr;
	bool bVertexColors = false;
	// Bit 3 of the enabled passes is the specular pass.
	bool bSpecular = GetFlag(BSSP_SPECULAR) && (uiPassState & 8) && bSpecularLOD;
	bool bSorted = bFading ||
		(static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject)->m_uFlags &
			NiAlphaProperty::ALPHA_BLEND_MASK);
	bool bProjectedShadow = BSShaderManager::GetUseProjectedShadow() && !GetFlag(BSSP_FLAG_NOPROJSHADOW) &&
		ShadowLightShader::GetProjectedShadowScale() &&
		BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 1 &&
		BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 3;
	if (apGeom->m_spModelData.m_pObject->m_pkColor || GetFlag(BSSP_FLAG_VERTEXCOLORS)) {
		bVertexColors = true;
	}
	if (spTexEffectData.m_pObject) {
		bProjectedShadow = false;
	}
	if (bRefraction) {
		uint16_t usPass;
		if (bFireRefraction) {
			usPass = 464;
		} else if (bVertexColors) {
			usPass = 2 * bSkinned + 461;
		} else {
			usPass = 2 * bSkinned + 460;
		}
		pRenderPassList->Add(apGeom, usPass, true, 0, nullptr, nullptr, nullptr, nullptr);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_REFRACT;
		iLastRenderPassState = uiPassState;
		return pRenderPassList;
	}
	if (ulFlags[1] & BSS2_1st_person) {
		pRenderPassList->Add(apGeom, bSkinned + 465, true, 0, nullptr, nullptr, nullptr, nullptr);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint =
			BSSP_HINT_GROUP_1STPERSONNOREFRACT;
	}

	// The lit pass: the sun, then every active light that casts no shadow.
	ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
	pRenderPassList->Add(apGeom, 0, true, 0, nullptr, nullptr, nullptr, nullptr);
	RenderPass* pPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
	pPass->ucNumLights = 1;
	void* pIter;
	for (ShadowSceneLight* pLight = GetFirstActiveLight(pIter); pLight; pLight = GetNextActiveLight(pIter)) {
		if (!pLight->bCastShadow) {
			++pPass->ucNumLights;
		}
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
			if (!pLight->bCastShadow) {
				pPass->ppSceneLights[ucLight++] = pLight;
			}
		}
		pPass->ucNumLights = ucLight;
	} else {
		pPass->ppSceneLights = nullptr;
	}
	SetFlag(BSSP_FLAG_SHADOWMAP, false);
	if (GetFlag(BSSP_FLAG_HAIR)) {
		if (bSpecular) {
			pPass->usPassEnum = bProjectedShadow ? 430 : 400;
		} else {
			pPass->usPassEnum = bProjectedShadow ? 415 : 385;
		}
	} else if (bSpecular) {
		if (bSkinned) {
			// The skinned parallax pass keeps the enum Add gave it.
			if (bParallax) {
			} else if (bFaceGen) {
				pPass->usPassEnum = bProjectedShadow ? 434 : 404;
			} else if (bGlowMap) {
				if (bVertexColors) {
					pPass->usPassEnum = bProjectedShadow ? 442 : 412;
				} else {
					pPass->usPassEnum = bProjectedShadow ? 436 : 406;
				}
			} else if (bVertexColors) {
				pPass->usPassEnum = bProjectedShadow ? 438 : 408;
			} else {
				pPass->usPassEnum = bProjectedShadow ? 429 : 399;
			}
		} else if (bParallax) {
			if (bVertexColors) {
				if (bProjectedShadow) {
					pPass->usPassEnum = bParallaxOcclusion ? 440 : 439;
				} else {
					pPass->usPassEnum = bParallaxOcclusion ? 410 : 409;
				}
			} else if (bProjectedShadow) {
				pPass->usPassEnum = bParallaxOcclusion ? 432 : 431;
			} else {
				pPass->usPassEnum = bParallaxOcclusion ? 402 : 401;
			}
		} else if (bFaceGen) {
			pPass->usPassEnum = bProjectedShadow ? 433 : 403;
		} else if (bGlowMap) {
			if (bVertexColors) {
				pPass->usPassEnum = bProjectedShadow ? 441 : 411;
			} else {
				pPass->usPassEnum = bProjectedShadow ? 435 : 405;
			}
		} else if (bVertexColors) {
			pPass->usPassEnum = bProjectedShadow ? 437 : 407;
		} else {
			pPass->usPassEnum = bProjectedShadow ? 428 : 398;
		}
	} else if (bSkinned) {
		// The skinned parallax pass keeps the enum Add gave it.
		if (bParallax) {
		} else if (bFaceGen) {
			pPass->usPassEnum = bProjectedShadow ? 419 : 389;
		} else if (bGlowMap) {
			if (bVertexColors) {
				pPass->usPassEnum = bProjectedShadow ? 427 : 397;
			} else {
				pPass->usPassEnum = bProjectedShadow ? 421 : 391;
			}
		} else if (bVertexColors) {
			pPass->usPassEnum = bProjectedShadow ? 423 : 393;
		} else {
			pPass->usPassEnum = bProjectedShadow ? 414 : 384;
		}
	} else if (bParallax) {
		if (bVertexColors) {
			if (bProjectedShadow) {
				pPass->usPassEnum = bParallaxOcclusion ? 425 : 424;
			} else {
				pPass->usPassEnum = bParallaxOcclusion ? 395 : 394;
			}
		} else if (bProjectedShadow) {
			pPass->usPassEnum = bParallaxOcclusion ? 417 : 416;
		} else {
			pPass->usPassEnum = bParallaxOcclusion ? 387 : 386;
		}
	} else if (bFaceGen) {
		pPass->usPassEnum = bProjectedShadow ? 418 : 388;
	} else if (bGlowMap) {
		if (bVertexColors) {
			pPass->usPassEnum = bProjectedShadow ? 426 : 396;
		} else {
			pPass->usPassEnum = bProjectedShadow ? 420 : 390;
		}
	} else if (bVertexColors) {
		pPass->usPassEnum = bProjectedShadow ? 422 : 392;
	} else {
		pPass->usPassEnum = bProjectedShadow ? 413 : 383;
	}
	if (GetFlag(BSSP_FLAG_DECAL)) {
		pPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
	} else if (GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
		pPass->eAccumulationHint = GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DYNAMIC_DECAL;
	} else if (bSorted) {
		pPass->eAccumulationHint = fFadeAlpha < 1.0f ? BSSP_HINT_GROUP_FADING : BSSP_HINT_SORT;
	} else if (apAccumulator->eAccumulationMode == BSShaderAccumulator::BSSA_ACCUMLOD ||
		GetFlag(BSSP_FLAG_LODBUILDING) || GetFlag(BSSP_FLAG_LODLANDSCAPE)) {
		pPass->eAccumulationHint = BSSP_HINT_GROUP_LOD;
	}

	// A shadow pass per active light that casts one.
	if (BSShaderManager::GetShadows()) {
		for (ShadowSceneLight* pLight = GetFirstActiveLight(pIter); pLight; pLight = GetNextActiveLight(pIter)) {
			if (pLight->bCastShadow) {
				pRenderPassList->Add(apGeom, 0, false, 1, pLight, nullptr, nullptr, nullptr);
				RenderPass* pShadowPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
				if (bVertexColors) {
					pShadowPass->usPassEnum = 2 * bSkinned + 453;
				} else {
					pShadowPass->usPassEnum = 2 * bSkinned + 452;
				}
				if (GetFlag(BSSP_FLAG_DECAL)) {
					pShadowPass->eAccumulationHint =
						GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
				} else if (GetFlag(BSSP_FLAG_DYNAMIC_DECAL)) {
					pShadowPass->eAccumulationHint =
						GetFlag(BSSP_FLAG_ALPHA_DECAL) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DYNAMIC_DECAL;
				}
			}
		}
	}

	// The block-out texture's pass, else the fade pass of a geometry without alpha blending.
	BSShaderPPLightingProperty::TextureEffectData* pTexEffect = spTexEffectData.m_pObject;
	if (pTexEffect && pTexEffect->spBlockOutTexture.m_pObject) {
		pRenderPassList->Add(apGeom, bSkinned + 14, true, 0, nullptr, nullptr, nullptr, nullptr);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_FADING;
	} else if (GetFlag(BSSP_ZPREPASS) || fFadeAlpha < 1.0f) {
		NiAlphaProperty* pAlpha =
			static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject);
		if (!pAlpha || !(pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK)) {
			pRenderPassList->Add(apGeom, bVertexColors ? bSkinned + 12 : bSkinned + 10, true, 0, nullptr, nullptr,
				nullptr, nullptr);
			RenderPass* pFadePass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pFadePass->eAccumulationHint = GetFlag(BSSP_ZPREPASS) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_FADING;
			if (fFadeAlpha < 1.0f || fAlpha < 1.0f) {
				pFadePass->eAccumulationHint = BSSP_HINT_GROUP_FADING;
			}
		}
	}
	if (spTexEffectData.m_pObject) {
		pRenderPassList->Add(apGeom, bSkinned + 480, true, 0, nullptr, nullptr, nullptr, nullptr);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_SORT;
	}

	// The environment map pass: window or plain reflection, or the eye's, lit by the first active light.
	if (bEnvMap) {
		RenderPass* pEnvPass;
		if (bWindowReflect || !bEyeReflect) {
			uint16_t usPass;
			if (bWindowReflect) {
				usPass = 2 * bVertexColors + 446;
			} else if (bSkinned) {
				usPass = bVertexColors + 449;
			} else {
				usPass = 2 * bVertexColors + 445;
			}
			pRenderPassList->Add(apGeom, 0, false, 0, nullptr, nullptr, nullptr, nullptr);
			pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pEnvPass->usPassEnum = usPass;
		} else {
			ShadowSceneLight* pLight = GetFirstActiveLight(pIter);
			pRenderPassList->Add(apGeom, 0, false, 1, pLight ? pLight : pSunLight, nullptr, nullptr, nullptr);
			pEnvPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
			pEnvPass->usPassEnum = 451;
		}
		if (bSorted) {
			pEnvPass->eAccumulationHint = BSSP_HINT_SORT;
		}
	}
	iLastRenderPassState = uiPassState;
	return pRenderPassList;
}

float NoFaderShaderProperty::GetSpecularLODFade() {
	return 1.0;
}

float NoFaderShaderProperty::GetEnvMapLODFade() {
	return 1.0f;
}
