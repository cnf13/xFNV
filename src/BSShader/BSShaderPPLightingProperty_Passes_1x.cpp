#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeBranchShaderProperty.h"
#include "BSShader/SpeedTreeFrondShaderProperty.h"
#include "BSShader/SpeedTreeLeafShaderProperty.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"

#include <cstdint>

// After AddRenderNormalsPass_1x the game stores 6 to absolute address 6 (the hint of a null pass), which faults; this
// code deliberately leaves that store out. abUpdateList is passed on to every pass helper as this function's own
// argument.
void BSShaderPPLightingProperty::GetRenderPasses_1x(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
	uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) {
	uint32_t uiFlags = ulFlags[0];
	bool bbasespecular = (uiFlags & BSS1_Specular) != 0;
	bool benvmap = (uiFlags & BSS1_Environment_Mapping) != 0;
	bool bskinned = (uiFlags & BSS1_Skinned) != 0;
	bool bspecular = bbasespecular;
	bool bsinglePass = (uiFlags & BSS1_Single_Pass) != 0 || auiRenderMode == 3;
	bool bglowmap = spTextures[2][0].m_pObject != nullptr;
	bool bfacegenblend = (uiFlags & BSS1_FaceGen) != 0;
	bool bvertexcolors = apGeom->m_spModelData->m_pkColor != nullptr;
	bvertexcolors |= (ulFlags[1] & BSS2_Vertex_Colors) != 0;
	bool btreebranch = DynamicCastMember(&SpeedTreeBranchShaderProperty::ms_RTTI) != nullptr;
	bool btreefrond = DynamicCastMember(&SpeedTreeFrondShaderProperty::ms_RTTI) != nullptr;
	bool btreeleaf = DynamicCastMember(&SpeedTreeLeafShaderProperty::ms_RTTI) != nullptr;
	uiFlags = ulFlags[0];
	bool blandsplat = (uiFlags & BSS1_Landscape) != 0;
	bool bRefraction = (uiFlags & BSS1_Refraction) != 0;
	bool bRefractionFire = (uiFlags & BSS1_Fire_Refraction) != 0;
	bool bzPrePass = (uiFlags & BSS1_Z_Prepass) != 0;
	bool bdrawHideSecret = (uiFlags & BSS1_Localmap_Hide_Secret) == 0 || BSShaderManager::bLiteBrite;
	bool bhair = (uiFlags & BSS1_Hair) != 0;
	bool bdecal = (uiFlags & BSS1_Decal_Single_Pass) != 0 || (uiFlags & BSS1_Dynamic_Decal_Single_Pass) != 0;
	// The alpha property's flags are read through the property state without a null test.
	// Accumulation mode 2 and the two ulFlags[1] bits select the LOD hint.
	NiAlphaProperty* pStateAlpha =
		static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject);
	bool bLOD = (pStateAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) == 0 &&
		(apAccumulator->eAccumulationMode == 2 || (ulFlags[1] & BSS2_LOD_Building) != 0 ||
		 (ulFlags[1] & BSS2_LOD_Landscape) != 0);
	NiAlphaProperty* palphaprop = static_cast<NiAlphaProperty*>(apGeom->GetProperty(NiProperty::ALPHA));
	bool balphatest = palphaprop && palphaprop->GetAlphaTesting();
	bool balphblend = (palphaprop && palphaprop->GetAlphaBlending()) || fAlpha < 1.0f;
	if ((ulFlags[0] & BSS1_Eye_Environment_Mapping) != 0) {
		float fLODEnd = BSShaderManager::fEyeEnvMapLODEnd;
		benvmap = fLODEnd == 0.0f || !(fCameraDistance >= fLODEnd);
		bspecular = false;
	} else {
		float fLODEnd = BSShaderManager::fEnvMapLODEnd;
		if (fLODEnd != 0.0f && fCameraDistance >= fLODEnd)
			benvmap = false;
	}
	float fSpecularEnd = BSShaderManager::fSpecularLODEnd;
	if (fSpecularEnd != 0.0f && fCameraDistance >= fSpecularEnd)
		bspecular = false;
	ShadowSceneNode* pscenenode = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode;
	ShadowSceneLight* psunlight = pscenenode->pSunLight;
	bool bsunspecular = bspecular && psunlight->spLight->m_kSpec != NiColor::BLACK;
	if (balphblend) {
		bool bfirst = true;
		if (bzPrePass) {
			AddZOnlyPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, balphatest);
			bspecular = false;
		}
		bsinglePass = true;
	}
	bool bfirst = true;
	if (bRefraction || bRefractionFire) {
		AddRenderNormalsPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, bRefractionFire);
	} else {
		if ((ulFlags[1] & BSS2_1st_person) != 0) {
			BSShaderProperty::RenderPass* ppass =
				AddRenderNormalsClearPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned);
			ppass->eAccumulationHint = BSSP_HINT_GROUP_1STPERSONNOREFRACT;
		}
		uint16_t inumlights = GetNumberofActiveNonShadowLights();
		void* pLightIter;
		ShadowSceneLight* plight;
		if (bdrawHideSecret &&
			(bsinglePass || (aiEnabledPasses == BSShaderManager::BSSM_ENABLED_LIGHTING_PASSES && !blandsplat))) {
			if (inumlights >= 1 && !bsinglePass) {
				pLightIter = nullptr;
				plight = GetFirstActiveNonShadowLight(pLightIter);
				if (balphatest || bskinned || bglowmap) {
					if ((aiEnabledPasses & BSShaderManager::BSSM_ENABLED_AMBIENT) != 0) {
						AddAmbientPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
							btreebranch);
					}
					if (psunlight->spLight->m_kDiff != NiColor::BLACK) {
						AddDiffusePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false,
							btreebranch);
					}
					if (plight->spLight->m_kDiff != NiColor::BLACK)
						AddDiffusePass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, btreebranch);
				} else {
					AmbientDirAndPoint(apGeom, psunlight, plight, &asCount, abUpdateList, &bfirst, 0, btreebranch);
				}
				for (plight = GetNextActiveNonShadowLight(pLightIter); plight;
					plight = GetNextActiveNonShadowLight(pLightIter)) {
					if (plight->spLight->m_kDiff != NiColor::BLACK)
						AddDiffusePass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, btreebranch);
				}
				AddTexturePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, bfacegenblend,
					bvertexcolors, btreebranch, bhair);
				if (bspecular) {
					// The iterator is deliberately not reset here.
					plight = GetFirstActiveNonShadowLight(pLightIter);
					if (bsunspecular && psunlight->spLight->m_kDiff != NiColor::BLACK) {
						AddSpecularPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false,
							bbasespecular, false, btreebranch);
					} else {
						// Adds the first light's pass without a null or colour test.
						AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false,
							bbasespecular, false, btreebranch);
						plight = GetNextActiveNonShadowLight(pLightIter);
					}
					while (plight) {
						if (plight->spLight->m_kDiff != NiColor::BLACK) {
							AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false,
								bbasespecular, false, btreebranch);
						}
						plight = GetNextActiveNonShadowLight(pLightIter);
					}
				}
				if (benvmap)
					AddEnvmapPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, bvertexcolors);
			} else {
				if ((!bsinglePass && bsunspecular) || btreefrond || btreeleaf) {
					AddAmbDiffTexPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
						bfacegenblend, false, bvertexcolors, btreebranch);
				} else {
					AddAmbDiffTexPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
						bfacegenblend, true, bvertexcolors, btreebranch);
					bsinglePass = true;
				}
				if (bsunspecular && !bsinglePass) {
					AddSpecularPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false,
						bbasespecular, false, btreebranch);
				}
				if (benvmap)
					AddEnvmapPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, bvertexcolors);
			}
		} else if (!blandsplat) {
			bfirst = true;
			if ((aiEnabledPasses & BSShaderManager::BSSM_ENABLED_AMBIENT) != 0) {
				AddAmbientPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
					btreebranch);
			}
			if ((aiEnabledPasses & BSShaderManager::BSSM_ENABLED_DIFFUSE) != 0) {
				if (psunlight->spLight->m_kDiff != NiColor::BLACK)
					AddDiffusePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false, btreebranch);
				pLightIter = nullptr;
				for (plight = GetFirstActiveNonShadowLight(pLightIter); plight;
					plight = GetNextActiveNonShadowLight(pLightIter)) {
					if (plight->spLight->m_kDiff != NiColor::BLACK)
						AddDiffusePass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, btreebranch);
				}
			}
			if ((aiEnabledPasses & BSShaderManager::BSSM_ENABLED_TEXTURE) != 0) {
				AddTexturePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, bfacegenblend,
					bvertexcolors, btreebranch, bhair);
			}
			if (bspecular && (aiEnabledPasses & BSShaderManager::BSSM_ENABLED_SPECULAR) != 0) {
				pLightIter = nullptr;
				plight = GetFirstActiveNonShadowLight(pLightIter);
				if (bsunspecular && psunlight->spLight->m_kDiff != NiColor::BLACK) {
					AddSpecularPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false,
						bbasespecular, false, btreebranch);
					bfirst = false;
				} else if (plight) {
					if (plight->spLight->m_kDiff != NiColor::BLACK) {
						AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false,
							bbasespecular, false, btreebranch);
						plight = GetNextActiveNonShadowLight(pLightIter);
					}
				}
				// A first light whose colour is black is tested again here.
				while (plight) {
					if (plight->spLight->m_kDiff != NiColor::BLACK) {
						AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false,
							bbasespecular, false, btreebranch);
					}
					plight = GetNextActiveNonShadowLight(pLightIter);
				}
			}
			if (benvmap && (aiEnabledPasses & BSShaderManager::BSSM_ENABLED_SPECULAR) != 0)
				AddEnvmapPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, bvertexcolors);
		}
		if (blandsplat)
			AddLandscapePasses_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, false);
		if (aiEnabledPasses == BSShaderManager::BSSM_ENABLED_LIGHTING_PASSES && !bsinglePass)
			AddFogPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, btreebranch, balphblend);
		TextureEffectData* pTexEffect = spTexEffectData.m_pObject;
		if (pTexEffect) {
			if (pTexEffect->spBlockOutTexture.m_pObject) {
				BSShaderProperty::RenderPass* ppass =
					AddZOnlyTextureEffectPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned);
				ppass->eAccumulationHint = BSSP_HINT_SORT;
			}
			BSShaderProperty::RenderPass* ppass =
				AddTextureEffectPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned);
			ppass->eAccumulationHint = BSSP_HINT_SORT;
		}
		if (BSShaderManager::bBlurShader && (aiEnabledPasses & BSShaderManager::BSSM_ENABLED_AMBIENT) != 0)
			AddSelfIllumAlphaPass(apGeom, asCount, abUpdateList, bskinned, bglowmap);
		etAccumulationHint kaccumhint = BSSP_HINT_NORMAL;
		if (bLOD)
			kaccumhint = BSSP_HINT_GROUP_LOD;
		else if (bdecal)
			kaccumhint = (ulFlags[1] & BSS2_Alpha_Decal) != 0 ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
		else if (balphblend)
			kaccumhint = BSSP_HINT_SORT;
		// Passes whose hint is still normal take the property's hint.
		for (uint32_t i = 0; i < pRenderPassList->iSize; ++i) {
			BSShaderProperty::RenderPass* ppass = pRenderPassList->pPassA.GetAt(i);
			if (ppass->eAccumulationHint == BSSP_HINT_NORMAL)
				ppass->eAccumulationHint = kaccumhint;
		}
	}
}

BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddSelfIllumAlphaPass(NiGeometry* apGeom, uint16_t& asCount,
	int32_t abUpdateList, bool abSkinned, [[maybe_unused]] bool abGlow) {
	NiMaterialProperty* pMaterial =
		static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::MATERIAL].m_pObject);
	if (!pMaterial) {
		return nullptr;
	}
	NiColor kEmit = pMaterial->GetEmittance();
	if (!(kEmit != NiColor::BLACK)) {
		return nullptr;
	}
	BSShaderProperty::RenderPass* pLastPass = pRenderPassList->pPassA.GetAt(pRenderPassList->iSize - 1);
	if (pLastPass) {
		pLastPass->bLastPass = true;
	}
	// Compares abUpdateList's low byte with 1: any other byte counts the pass instead.
	const uint8_t ucUpdateList = static_cast<uint8_t>(abUpdateList);
	if (!abSkinned) {
		if (ucUpdateList == 1) {
			auto* pPass = static_cast<BSShaderProperty::RenderPass*>(MemoryManager::Instance()
					.Allocate(sizeof(BSShaderProperty::RenderPass)));
			if (!pPass) {
				return nullptr;
			}
			// Pass 0x2D6 is BSSM_SELFILLUMALPHA, which has no enum value.
			return ::new (pPass) BSShaderProperty::RenderPass(apGeom, 0x2D6, false, 0, nullptr);
		}
	} else if (ucUpdateList == 1) {
		auto* pPass = static_cast<BSShaderProperty::RenderPass*>(MemoryManager::Instance()
				.Allocate(sizeof(BSShaderProperty::RenderPass)));
		if (!pPass) {
			return nullptr;
		}
		// Pass 0x2D7 is BSSM_SELFILLUMALPHA_S, which has no enum value.
		return ::new (pPass) BSShaderProperty::RenderPass(apGeom, 0x2D7, false, 0, nullptr);
	}
	++asCount;
	return nullptr;
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddZOnlyPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abAlphaTest) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_ZONLY_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
            abFirst = false;
            return nullptr;
        }
	} else {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, abAlphaTest + 1, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
            abFirst = false;
            return nullptr;
        }
	}
	++asCount;
    abFirst = false;
    return nullptr;
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddZOnlyTextureEffectPass_1x(NiGeometry* apGeom,
	uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_ZONLY_TEXEFFECT_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
        } else {
			++asCount;
		}
        abFirst = false;
        return nullptr;
    } else {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_ZONLY_TEXEFFECT, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
        } else {
			++asCount;
		}
        abFirst = false;
        return nullptr;
    }
}

// Compares the update flag's low byte with 1 at each test, and always queues the pass as a first pass with one light,
// the sunlight.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddAmbientPass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apSunlight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abAlphaTest, bool abGlowMap, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abAlphaTest) {
			if (abGlowMap) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_SGAt, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                    abFirst = false;
					return nullptr;
				}
			} else {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_SAt, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                    abFirst = false;
					return nullptr;
				}
			}
		} else {
			if (abGlowMap) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_SG, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                    abFirst = false;
					return nullptr;
				}
			} else {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_S, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                    abFirst = false;
					return nullptr;
				}
			}
		}
	} else {
		if (abAlphaTest) {
			if (abGlowMap) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_GAt, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                    abFirst = false;
					return nullptr;
				}
			} else {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_At, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                    abFirst = false;
					return nullptr;
				}
			}
		} else {
			if (abGlowMap) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_G, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                    abFirst = false;
					return nullptr;
				}
			} else {
				if (abTreeBranch) {
					if (abUpdateList) {
                        pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT_Sb, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                        abFirst = false;
						return nullptr;
					}
				} else {
					if (abUpdateList) {
                        pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBIENT, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
                        abFirst = false;
						return nullptr;
					}
				}
			}
		}
	}
	++asCount;
	abFirst = false;
	return nullptr;
}

// Each test compares the update flag's low byte with 1. Combinations that no shader handles add no pass and count none.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddAmbDiffTexPass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apSunlight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abAlphaTest, bool abGlowMap, bool abFacegenBlend, bool abFog, bool abVertexColors, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abAlphaTest) {
			if (abGlowMap) {
				if (abFacegenBlend) {
					if (abFog) {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFGFgA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SGFgA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFGAVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFGA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SGAVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SGA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			} else {
				if (abFacegenBlend) {
					if (!abFog) {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFgA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFAVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SAVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			}
		} else {
			if (abGlowMap) {
				if (!abFacegenBlend) {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFGVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFG, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SGVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SG, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			} else {
				if (abFacegenBlend) {
					if (abFog) {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFFg, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFg, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SFVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SF, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_S, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			}
		}
	} else {
		if (abAlphaTest) {
			if (abGlowMap) {
				if (abFacegenBlend) {
					if (abFog) {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FGFgA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_GFgA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FGAVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FGA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_GAVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_GA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			} else {
				if (abFacegenBlend) {
					if (!abFog) {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FgA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FAVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FA, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_AVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_A, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			}
		} else {
			if (abGlowMap) {
				if (!abFacegenBlend) {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FGVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FG, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_GVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_G, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			} else {
				if (abFacegenBlend) {
					if (abFog) {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FFg, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (!abVertexColors) {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_Fg, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abTreeBranch) {
								if (abUpdateList)
									pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_SbF, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
								else
									++asCount;
							} else {
								if (abUpdateList)
									pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_FVc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
								else
									++asCount;
							}
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_F, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					} else {
						if (abVertexColors) {
							if (abTreeBranch) {
								if (abUpdateList)
									pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_Sb, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
								else
									++asCount;
							} else {
								if (abUpdateList)
									pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX_Vc, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
								else
									++asCount;
							}
						} else {
							if (abUpdateList)
								pRenderPassList->Add(apGeom, BSShaderManager::BSSM_AMBDIFFTEX, 1, 1u, apSunlight, nullptr, nullptr, nullptr);
							else
								++asCount;
						}
					}
				}
			}
		}
	}
	abFirst = false;
	return nullptr;
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddDiffusePass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apLight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abPointLight, bool abMultitexture, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abPointLight) {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_DIFFUSEPT_S, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		} else {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_DIFFUSEDIR_S, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		}
	} else {
		if (abPointLight) {
			if (abTreeBranch) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_DIFFUSEPT_Sb, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			} else {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_DIFFUSEPT, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			}
		} else {
			if (abTreeBranch) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_DIFFUSEDIR_Sb, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			} else {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_DIFFUSEDIR, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			}
		}
	}
	++asCount;
    abFirst = false;
    return nullptr;
}

BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddTexturePass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apLight, uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abSkinned,
	int32_t abMultitexture, bool abFacegenBlend, bool abVertexColors, bool abTreeBranch, bool abHair) {
	if (abSkinned) {
		if (abFacegenBlend) {
			if (abVertexColors) {
				char acName[128];
				if (apGeom) {
					NiNode* pParent = apGeom->m_pkParent;
					const char* pcParentName = pParent ? pParent->m_kName.GetHandle() : nullptr;
					if (pcParentName) {
						const char* pcName = apGeom->m_kName.GetHandle();
						if (pcName)
							BSsprintf(acName, 128, "Parent:%s,Child:%s", pcParentName, pcName);
						else
							BSsprintf(acName, 128, "Parent:%s", pcParentName);
					} else {
						const char* pcName = apGeom->m_kName.GetHandle();
						if (pcName)
							BSsprintf(acName, 128, "%s", pcName);
						else
							BSsprintf(acName, 128, "none");
					}
				} else {
					BSsprintf(acName, 128, "none");
				}
				char acMessage[256];
				BSsprintf(acMessage, 255,
					"SHADER ERROR (%s)(%08X) : no shader to handle TEXTURE_SFgVc ( texture skinned facegen vertexcolors )",
					acName, apGeom);
			} else {
				if (static_cast<uint8_t>(abUpdateList) == 1)
					pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_SFg, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
				else
					++asCount;
			}
		} else if (abVertexColors) {
			if (abHair) {
				if (static_cast<uint8_t>(abUpdateList) == 1)
					pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_SH, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
				else
					++asCount;
			} else {
				if (static_cast<uint8_t>(abUpdateList) == 1)
					pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_SVc, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
				else
					++asCount;
			}
		} else {
			if (static_cast<uint8_t>(abUpdateList) == 1)
				pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
			else
				++asCount;
		}
	} else if (abTreeBranch) {
		if (static_cast<uint8_t>(abUpdateList) == 1)
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_Sb, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
		else
			++asCount;
	} else if (abVertexColors) {
		if (abHair) {
			if (static_cast<uint8_t>(abUpdateList) == 1)
				pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_H, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
			else
				++asCount;
		} else {
			if (static_cast<uint8_t>(abUpdateList) == 1)
				pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_Vc, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
			else
				++asCount;
		}
	} else if (abFacegenBlend) {
		if (static_cast<uint8_t>(abUpdateList) == 1)
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE_Fg, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
		else
			++asCount;
	} else {
		if (static_cast<uint8_t>(abUpdateList) == 1)
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXTURE, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
		else
			++asCount;
	}
	abFirst = false;
	return nullptr;
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddSpecularPass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apLight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abPointLight, bool abMultitexture, bool abBaseSpecular, bool abDecalSpecular, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abPointLight) {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_SPECULARPT_S, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		} else {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_SPECULARDIR_S, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		}
	} else {
		if (abPointLight) {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_SPECULARPT, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		} else {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_SPECULARDIR, abFirst, 1u, apLight, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		}
	}
	++asCount;
    abFirst = false;
    return nullptr;
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddEnvmapPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abVertexColors) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abVertexColors) {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_ENVMAP_SVc, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		} else {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_ENVMAP_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		}
	} else {
		if (abVertexColors) {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_ENVMAP_Vc, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		} else {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_ENVMAP, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		}
	}
	++asCount;
    abFirst = false;
    return nullptr;
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddFogPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abTreeBranch, bool abAlphaBlend) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abTreeBranch) {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_FOG_Sb, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
            abFirst = false;
            return nullptr;
        }
	} else {
		if (abSkinned) {
			if (abAlphaBlend) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_FOG_SA, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			} else {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_FOG_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			}
		} else {
			if (abAlphaBlend) {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_FOG_A, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			} else {
				if (abUpdateList) {
                    pRenderPassList->Add(apGeom, BSShaderManager::BSSM_FOG, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                    abFirst = false;
                    return nullptr;
                }
			}
		}
	}
	++asCount;
    abFirst = false;
    return nullptr;
}

// The light iterators are locals, which GetFirstActiveNonShadowLight writes before any read.
void BSShaderPPLightingProperty::AddLandscapePasses_1x(NiGeometry* apGeom, ShadowSceneLight* apSunlight,
	uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abProjShadow) {
	BSShaderPPLightingProperty* pShadeProp =
		static_cast<BSShaderPPLightingProperty*>(apGeom->GetProperty(NiProperty::SHADE));
	const bool bLowShader = BSShaderManager::eShaderLevel <= 1;
	// The zero-extended count is compared with iMaxLandTextures unsigned.
	uint16_t usNumTextures;
	if (static_cast<uint32_t>(usLandscapeTextures) >= static_cast<uint32_t>(BSShaderManager::iMaxLandTextures)) {
		usNumTextures = static_cast<uint16_t>(BSShaderManager::iMaxLandTextures);
	} else {
		usNumTextures = usLandscapeTextures;
	}
	const uint8_t ucUpdateList = static_cast<uint8_t>(abUpdateList);
	if (BSShaderManager::bLiteBrite) {
		if (ucUpdateList == 1) {
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LANDAD, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
		} else {
			++asCount;
		}
		abFirst = false;
		for (uint8_t i = 0; i < usNumTextures; ++i) {
			if (pShadeProp->spTextures[0][i + 1].m_pObject) {
				if (ucUpdateList == 1) {
					pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LANDAD_A, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
					pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->ucExtraParam = i + 1;
				} else {
					++asCount;
				}
			}
		}
		return;
	}
	if (!abProjShadow) {
		if (ucUpdateList == 1) {
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LANDAD, abFirst, 1, apSunlight, nullptr, nullptr, nullptr);
		} else {
			++asCount;
		}
	} else if (ucUpdateList == 1) {
		pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LANDAD_Shp, abFirst, 1, apSunlight, nullptr, nullptr, nullptr);
	} else {
		++asCount;
	}
	abFirst = false;
	if (pShadeProp->spTextures[2][0].m_pObject) {
		if (ucUpdateList == 1) {
			pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LAND_G, false, 0, nullptr, nullptr, nullptr, nullptr);
		} else {
			++asCount;
		}
	}
	if (bLowShader) {
		void* pIter;
		for (ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter); pLight;
			pLight = GetNextActiveNonShadowLight(pIter)) {
			const NiColor& kDiff = pLight->spLight->m_kDiff;
			if (NiColor::BLACK != kDiff) {
				if (ucUpdateList == 1) {
					pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LANDDIFF, abFirst, 1, pLight, nullptr, nullptr, nullptr);
				} else {
					++asCount;
				}
			}
		}
	}
	for (uint8_t i = 0; i < usNumTextures; ++i) {
		if (pShadeProp->spTextures[0][i + 1].m_pObject) {
			if (ucUpdateList == 1) {
				pRenderPassList->Add(apGeom, abProjShadow ? 499 : 498, abFirst, 1, apSunlight, nullptr, nullptr,
					nullptr);
				pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->ucExtraParam = i + 1;
			} else {
				++asCount;
			}
		}
	}
	for (uint8_t i = 0; i < usNumTextures; ++i) {
		if (!pShadeProp->spTextures[0][i + 1].m_pObject) {
			continue;
		}
		if (pShadeProp->spTextures[2][i + 1].m_pObject) {
			if (ucUpdateList == 1) {
				pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LAND_GA, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
				pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->ucExtraParam = i + 1;
			} else {
				++asCount;
			}
		}
		if (bLowShader) {
			void* pIter;
			for (ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter); pLight;
				pLight = GetNextActiveNonShadowLight(pIter)) {
				const NiColor& kDiff = pLight->spLight->m_kDiff;
				if (NiColor::BLACK != kDiff) {
					if (ucUpdateList == 1) {
						pRenderPassList->Add(apGeom, BSShaderManager::BSSM_LANDDIFF_A, abFirst, 1, pLight, nullptr, nullptr, nullptr);
						pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->ucExtraParam = i + 1;
					} else {
						++asCount;
					}
				}
			}
		}
	}
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddTextureEffectPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXEFFECT_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
        } else {
			++asCount;
		}
        abFirst = false;
        return nullptr;
    } else {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_TEXEFFECT, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
        } else {
			++asCount;
		}
        abFirst = false;
        return nullptr;
    }
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddRenderNormalsPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abRefractionFire) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abRefractionFire) {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_RENDERNORMALS_FIRE, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
            abFirst = false;
            return nullptr;
        }
	} else {
		if (abSkinned) {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_RENDERNORMALS_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		} else {
			if (abUpdateList) {
                pRenderPassList->Add(apGeom, BSShaderManager::BSSM_RENDERNORMALS, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
                abFirst = false;
                return nullptr;
            }
		}
	}
	++asCount;
    abFirst = false;
    return nullptr;
}

// Each test compares the update flag's low byte with 1.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddRenderNormalsClearPass_1x(NiGeometry* apGeom,
	uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_RENDERNORMALS_CLEAR_S, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
        } else {
			++asCount;
		}
        abFirst = false;
        return nullptr;
    } else {
		if (abUpdateList) {
            pRenderPassList->Add(apGeom, BSShaderManager::BSSM_RENDERNORMALS_CLEAR, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
        } else {
			++asCount;
		}
        abFirst = false;
        return nullptr;
    }
}
