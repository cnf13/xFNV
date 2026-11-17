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
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Left unregistered (porting.md "Leave unregistered", reasons 1 and 3): src cannot reproduce the exe's frame bytes
// here.
// - The exe keeps its bools as single bytes packed in its frame (seta/mov byte) and pushes each to the pass helpers
//   as the whole dword (`mov ecx, [esp+0x1C]; push ecx`), so the upper three bytes of those arguments are the
//   neighbouring locals, some unwritten on some paths: an uninitialised read.
// - It writes bytes into its own argument slots: bzPrePass into auiRenderMode's, bfirst (ZOnly) into asCount's after
//   loading it, bsunspecular into apAccumulator's and the light iterator into abUpdateList's.
// - After AddRenderNormalsPass_1x it stores 6 to absolute address 6 (`mov byte ptr [6], 6`: the hint of a null
//   pass), which faults; src leaves that store out.
// abUpdateList is pushed on to every pass helper as this function's own argument dword (`mov ebp, [esp+0x60]`).
void BSShaderPPLightingProperty::GetRenderPasses_1x(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
	uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) {
	uint32_t uiFlags = ulFlags[0];
	bool bbasespecular = (uiFlags & 1) != 0;
	bool benvmap = (uiFlags & 0x80) != 0;
	bool bskinned = (uiFlags & 2) != 0;
	bool bspecular = bbasespecular;
	bool bsinglePass = (uiFlags & 0x20) != 0 || auiRenderMode == 3;
	bool bglowmap = spTextures[2][0].m_pObject != nullptr;
	bool bfacegenblend = (uiFlags & 0x400) != 0;
	bool bvertexcolors = apGeom->m_spModelData.m_pObject->m_pkColor != nullptr;
	bvertexcolors |= (ulFlags[1] & 0x20) != 0;
	bool btreebranch = DynamicCastMember(&SpeedTreeBranchShaderProperty::ms_RTTI) != nullptr;
	bool btreefrond = DynamicCastMember(&SpeedTreeFrondShaderProperty::ms_RTTI) != nullptr;
	bool btreeleaf = DynamicCastMember(&SpeedTreeLeafShaderProperty::ms_RTTI) != nullptr;
	uiFlags = ulFlags[0];
	bool blandsplat = (uiFlags & 0x4000) != 0;
	bool bRefraction = (uiFlags & 0x8000) != 0;
	bool bRefractionFire = (uiFlags & 0x10000) != 0;
	bool bzPrePass = (uiFlags & 0x200) != 0;
	bool bdrawHideSecret = (uiFlags & 0x100000) == 0 || BSShaderManager::bLiteBrite;
	bool bhair = (uiFlags & 0x40000) != 0;
	bool bdecal = (uiFlags & 0x4000000) != 0 || (uiFlags & 0x8000000) != 0;
	// The exe reads the alpha property's flags through the property state without a null test. Accumulation mode 2
	// and the two ulFlags[1] bits select the LOD hint.
	NiAlphaProperty* pStateAlpha =
		static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject);
	bool bLOD = (pStateAlpha->m_uFlags & 1) == 0 &&
		(apAccumulator->eAccumulationMode == 2 || (ulFlags[1] & 4) != 0 || (ulFlags[1] & 2) != 0);
	NiAlphaProperty* palphaprop = static_cast<NiAlphaProperty*>(apGeom->GetProperty(NiProperty::ALPHA));
	// EXE-INLINED none: NiAlphaProperty::GetAlphaTesting
	bool balphatest = palphaprop && ((palphaprop->m_uFlags >> 9) & 1) != 0;
	// EXE-INLINED none: NiAlphaProperty::GetAlphaBlending
	bool balphblend = (palphaprop && (palphaprop->m_uFlags & 1) != 0) || fAlpha < 1.0f;
	if ((ulFlags[0] & 0x20000) != 0) {
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
	bool bsunspecular = bspecular && psunlight->spLight.m_pObject->m_kSpec != NiColor_BLACK;
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
		return;
	}
	if ((ulFlags[1] & 0x40) != 0) {
		BSShaderProperty::RenderPass* ppass =
			AddRenderNormalsClearPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned);
		ppass->eAccumulationHint = BSSP_HINT_GROUP_1STPERSONNOREFRACT;
	}
	uint16_t inumlights = static_cast<uint16_t>(GetNumberofActiveNonShadowLights());
	void* pLightIter;
	ShadowSceneLight* plight;
	if (!bdrawHideSecret)
		goto ExplicitPasses;
	if (!bsinglePass) {
		if (aiEnabledPasses != uiAllPasses)
			goto ExplicitPasses;
		if (blandsplat)
			goto LandscapePasses;
	}
	if (inumlights >= 1 && !bsinglePass) {
		pLightIter = nullptr;
		plight = GetFirstActiveNonShadowLight(pLightIter);
		if (!balphatest && !bskinned && !bglowmap) {
			AmbientDirAndPoint(apGeom, psunlight, plight, &asCount, abUpdateList, &bfirst, 0, btreebranch);
		} else {
			if ((aiEnabledPasses & 1) != 0) {
				AddAmbientPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
					btreebranch);
			}
			if (psunlight->spLight.m_pObject->m_kDiff != NiColor_BLACK) {
				AddDiffusePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false,
					btreebranch);
			}
			if (plight->spLight.m_pObject->m_kDiff != NiColor_BLACK)
				AddDiffusePass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, btreebranch);
		}
		for (plight = GetNextActiveNonShadowLight(pLightIter); plight;
			plight = GetNextActiveNonShadowLight(pLightIter)) {
			if (plight->spLight.m_pObject->m_kDiff != NiColor_BLACK)
				AddDiffusePass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, btreebranch);
		}
		AddTexturePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, bfacegenblend,
			bvertexcolors, btreebranch, bhair);
		if (bspecular) {
			// The exe does not reset the iterator here.
			plight = GetFirstActiveNonShadowLight(pLightIter);
			if (bsunspecular && psunlight->spLight.m_pObject->m_kDiff != NiColor_BLACK) {
				AddSpecularPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false,
					bbasespecular, false, btreebranch);
			} else {
				// The exe adds the first light's pass without a null or colour test.
				AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, bbasespecular,
					false, btreebranch);
				plight = GetNextActiveNonShadowLight(pLightIter);
			}
			for (; plight; plight = GetNextActiveNonShadowLight(pLightIter)) {
				if (plight->spLight.m_pObject->m_kDiff != NiColor_BLACK) {
					AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false,
						bbasespecular, false, btreebranch);
				}
			}
		}
	} else {
		if ((inumlights < 1 && !bsinglePass && bsunspecular) || btreefrond || btreeleaf) {
			AddAmbDiffTexPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
				bfacegenblend, false, bvertexcolors, btreebranch);
		} else {
			AddAmbDiffTexPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
				bfacegenblend, true, bvertexcolors, btreebranch);
			bsinglePass = true;
		}
		if (bsunspecular && !bsinglePass) {
			AddSpecularPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false, bbasespecular,
				false, btreebranch);
		}
	}
	if (benvmap)
		AddEnvmapPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, bvertexcolors);
	if (!blandsplat)
		goto FogPass;
LandscapePasses:
	AddLandscapePasses_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, false);
FogPass:
	if (aiEnabledPasses == uiAllPasses && !bsinglePass)
		AddFogPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, btreebranch, balphblend);
	{
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
		if (BSShaderManager::bBlurShader && (aiEnabledPasses & 1) != 0)
			AddSelfIllumAlphaPass(apGeom, asCount, abUpdateList, bskinned, bglowmap);
		etAccumulationHint kaccumhint = BSSP_HINT_NORMAL;
		if (bLOD)
			kaccumhint = BSSP_HINT_GROUP_LOD;
		else if (bdecal)
			kaccumhint = (ulFlags[1] & 0x200000) != 0 ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
		else if (balphblend)
			kaccumhint = BSSP_HINT_SORT;
		// Passes whose hint is still normal take the property's hint.
		for (uint32_t i = 0; i < pRenderPassList->iSize; ++i) {
			BSShaderProperty::RenderPass* ppass = pRenderPassList->pPassA.GetAt(i);
			if (ppass->eAccumulationHint == BSSP_HINT_NORMAL)
				ppass->eAccumulationHint = kaccumhint;
		}
	}
	return;
ExplicitPasses:
	if (blandsplat)
		goto LandscapePasses;
	bfirst = true;
	if ((aiEnabledPasses & 1) != 0) {
		AddAmbientPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, balphatest, bglowmap,
			btreebranch);
	}
	if ((aiEnabledPasses & 2) != 0) {
		if (psunlight->spLight.m_pObject->m_kDiff != NiColor_BLACK)
			AddDiffusePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false, btreebranch);
		pLightIter = nullptr;
		for (plight = GetFirstActiveNonShadowLight(pLightIter); plight;
			plight = GetNextActiveNonShadowLight(pLightIter)) {
			if (plight->spLight.m_pObject->m_kDiff != NiColor_BLACK)
				AddDiffusePass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, btreebranch);
		}
	}
	if ((aiEnabledPasses & 4) != 0) {
		AddTexturePass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, bfacegenblend,
			bvertexcolors, btreebranch, bhair);
	}
	if (bspecular && (aiEnabledPasses & 8) != 0) {
		pLightIter = nullptr;
		plight = GetFirstActiveNonShadowLight(pLightIter);
		if (bsunspecular && psunlight->spLight.m_pObject->m_kDiff != NiColor_BLACK) {
			AddSpecularPass_1x(apGeom, psunlight, asCount, abUpdateList, bfirst, bskinned, false, false, bbasespecular,
				false, btreebranch);
			bfirst = false;
		} else if (plight) {
			if (plight->spLight.m_pObject->m_kDiff != NiColor_BLACK) {
				AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, bbasespecular,
					false, btreebranch);
				plight = GetNextActiveNonShadowLight(pLightIter);
			}
		}
		// A first light whose colour is black is tested again here, as the exe does.
		for (; plight; plight = GetNextActiveNonShadowLight(pLightIter)) {
			if (plight->spLight.m_pObject->m_kDiff != NiColor_BLACK) {
				AddSpecularPass_1x(apGeom, plight, asCount, abUpdateList, bfirst, bskinned, true, false, bbasespecular,
					false, btreebranch);
			}
		}
	}
	if (benvmap && (aiEnabledPasses & 8) != 0)
		AddEnvmapPass_1x(apGeom, asCount, abUpdateList, bfirst, bskinned, bvertexcolors);
	goto FogPass;
}

// The exe's cleanup-only EH frame (around the pass allocation) is left out.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddSelfIllumAlphaPass(NiGeometry* apGeom, uint16_t& asCount,
	int32_t abUpdateList, bool abSkinned, bool abGlow) {
	(void)abGlow;
	NiMaterialProperty* pMaterial =
		static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::MATERIAL].m_pObject);
	if (!pMaterial) {
		return nullptr;
	}
	// EXE-INLINED none: NiMaterialProperty::GetEmittance
	const NiColor* pEmit = pMaterial->m_external_emit;
	if (!pEmit) {
		pEmit = &pMaterial->m_emit;
	}
	NiColor kEmit = *pEmit;
	if (!(kEmit != NiColor_BLACK)) {
		return nullptr;
	}
	BSShaderProperty::RenderPass* pLastPass = pRenderPassList->pPassA.GetAt(pRenderPassList->iSize - 1);
	if (pLastPass) {
		pLastPass->bLastPass = true;
	}
	// The exe compares abUpdateList's low byte with 1 (`cmp byte ptr [esp+0x38], bl`, bl = 1): any other byte counts
	// the pass instead.
	const uint8_t ucUpdateList = static_cast<uint8_t>(abUpdateList);
	if (!abSkinned) {
		if (ucUpdateList == 1) {
			auto* pPass = static_cast<BSShaderProperty::RenderPass*>(MemoryManager_Singleton
					.Allocate(sizeof(BSShaderProperty::RenderPass)));
			if (!pPass) {
				return nullptr;
			}
			// Pass 0x2D6 (IDA: BSSM_SELFILLUMALPHA; no src enum value).
			return ::new (pPass) BSShaderProperty::RenderPass(apGeom, 0x2D6, false, 0, nullptr);
		}
	} else if (ucUpdateList == 1) {
		auto* pPass = static_cast<BSShaderProperty::RenderPass*>(MemoryManager_Singleton
				.Allocate(sizeof(BSShaderProperty::RenderPass)));
		if (!pPass) {
			return nullptr;
		}
		// Pass 0x2D7 (IDA: BSSM_SELFILLUMALPHA_S; no src enum value).
		return ::new (pPass) BSShaderProperty::RenderPass(apGeom, 0x2D7, false, 0, nullptr);
	}
	++asCount;
	return nullptr;
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddZOnlyPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abAlphaTest) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 3u, abFirst, 0, 0, 0, 0, 0);
			abFirst = 0;
			return 0;
		}
	} else {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, abAlphaTest + 1, abFirst, 0, 0, 0, 0, 0);
			abFirst = 0;
			return 0;
		}
	}
	++asCount;
	abFirst = 0;
	return 0;
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddZOnlyTextureEffectPass_1x(NiGeometry* apGeom,
	uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 5u, abFirst, 0, 0, 0, 0, 0);
		} else {
			++asCount;
		}
		abFirst = 0;
		return 0;
	} else {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 4u, abFirst, 0, 0, 0, 0, 0);
		} else {
			++asCount;
		}
		abFirst = 0;
		return 0;
	}
}

// (thiscall, ret 0x24, 0xBD9540). Same function as BSShaderProperty::Ambient, registered
// there.
// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`), and always queues
// the pass as a first pass with one light, the sunlight.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddAmbientPass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apSunlight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abAlphaTest, bool abGlowMap, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abAlphaTest) {
			if (abGlowMap) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 0x5Fu, 1, 1u, apSunlight, 0, 0, 0);
					abFirst = false;
					return nullptr;
				}
			} else {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 0x5Eu, 1, 1u, apSunlight, 0, 0, 0);
					abFirst = false;
					return nullptr;
				}
			}
		} else {
			if (abGlowMap) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 0x5Du, 1, 1u, apSunlight, 0, 0, 0);
					abFirst = false;
					return nullptr;
				}
			} else {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 0x5Cu, 1, 1u, apSunlight, 0, 0, 0);
					abFirst = false;
					return nullptr;
				}
			}
		}
	} else {
		if (abAlphaTest) {
			if (abGlowMap) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 0x5Bu, 1, 1u, apSunlight, 0, 0, 0);
					abFirst = false;
					return nullptr;
				}
			} else {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 0x5Au, 1, 1u, apSunlight, 0, 0, 0);
					abFirst = false;
					return nullptr;
				}
			}
		} else {
			if (abGlowMap) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 0x59u, 1, 1u, apSunlight, 0, 0, 0);
					abFirst = false;
					return nullptr;
				}
			} else {
				if (abTreeBranch) {
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 0x60u, 1, 1u, apSunlight, 0, 0, 0);
						abFirst = false;
						return nullptr;
					}
				} else {
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 0x58u, 1, 1u, apSunlight, 0, 0, 0);
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

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddAmbDiffTexPass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apSunlight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abAlphaTest, bool abGlowMap, bool abFacegenBlend, bool abFog, bool abVertexColors, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abAlphaTest) {
			if (!abGlowMap) {
				if (abFacegenBlend) {
					if (abFog || abVertexColors) {
						goto LABEL_147;
					}
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 120u, 1, 1u, apSunlight, 0, 0, 0);
						abFirst = 0;
						return 0;
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 142u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 141u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 117u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 116u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					}
				}
				goto LABEL_146;
			}
			if (!abFacegenBlend) {
				if (abFog) {
					if (abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 145u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					} else {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 144u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					}
				} else {
					if (abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 119u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					} else {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 118u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					}
				}
				goto LABEL_146;
			}
			if (abFog) {
				if (!abVertexColors) {
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 146u, 1, 1u, apSunlight, 0, 0, 0);
						abFirst = 0;
						return 0;
					}
					goto LABEL_146;
				}
			} else {
				if (!abVertexColors) {
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 121u, 1, 1u, apSunlight, 0, 0, 0);
						abFirst = 0;
						return 0;
					}
					goto LABEL_146;
				}
			}
		} else {
			if (abGlowMap) {
				if (!abFacegenBlend) {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 140u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 139u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 114u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 113u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					}
					goto LABEL_146;
				}
			} else {
				if (!abFacegenBlend) {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 138u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 137u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 112u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 111u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					}
					goto LABEL_146;
				}
				if (abFog) {
					if (!abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 143u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
						goto LABEL_146;
					}
				} else {
					if (!abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 115u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
						goto LABEL_146;
					}
				}
			}
		}
	} else {
		if (abAlphaTest) {
			if (!abGlowMap) {
				if (abFacegenBlend) {
					if (abFog || abVertexColors) {
						goto LABEL_147;
					}
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 109u, 1, 1u, apSunlight, 0, 0, 0);
						abFirst = 0;
						return 0;
					}
				} else {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 132u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 131u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 106u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 105u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					}
				}
				goto LABEL_146;
			}
			if (!abFacegenBlend) {
				if (abFog) {
					if (abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 135u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					} else {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 134u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					}
				} else {
					if (abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 108u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					} else {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 107u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
					}
				}
				goto LABEL_146;
			}
			if (abFog) {
				if (!abVertexColors) {
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 136u, 1, 1u, apSunlight, 0, 0, 0);
						abFirst = 0;
						return 0;
					}
				LABEL_146:;
					++asCount;
				}
			} else {
				if (!abVertexColors) {
					if (abUpdateList) {
						pRenderPassList->Add(apGeom, 110u, 1, 1u, apSunlight, 0, 0, 0);
						abFirst = 0;
						return 0;
					}
					goto LABEL_146;
				}
			}
		} else {
			if (abGlowMap) {
				if (!abFacegenBlend) {
					if (abFog) {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 130u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 129u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					} else {
						if (abVertexColors) {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 103u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 102u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					}
					goto LABEL_146;
				}
			} else {
				if (!abFacegenBlend) {
					if (abFog) {
						if (abVertexColors) {
							if (abTreeBranch) {
								if (abUpdateList) {
									pRenderPassList->Add(apGeom, 123u, 1, 1u, apSunlight, 0, 0, 0);
									abFirst = 0;
									return 0;
								}
							} else {
								if (abUpdateList) {
									pRenderPassList->Add(apGeom, 128u, 1, 1u, apSunlight, 0, 0, 0);
									abFirst = 0;
									return 0;
								}
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 127u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					} else {
						if (abVertexColors) {
							if (abTreeBranch) {
								if (abUpdateList) {
									pRenderPassList->Add(apGeom, 122u, 1, 1u, apSunlight, 0, 0, 0);
									abFirst = 0;
									return 0;
								}
							} else {
								if (abUpdateList) {
									pRenderPassList->Add(apGeom, 101u, 1, 1u, apSunlight, 0, 0, 0);
									abFirst = 0;
									return 0;
								}
							}
						} else {
							if (abUpdateList) {
								pRenderPassList->Add(apGeom, 100u, 1, 1u, apSunlight, 0, 0, 0);
								abFirst = 0;
								return 0;
							}
						}
					}
					goto LABEL_146;
				}
				if (abFog) {
					if (!abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 133u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
						goto LABEL_146;
					}
				} else {
					if (!abVertexColors) {
						if (abUpdateList) {
							pRenderPassList->Add(apGeom, 104u, 1, 1u, apSunlight, 0, 0, 0);
							abFirst = 0;
							return 0;
						}
						goto LABEL_146;
					}
				}
			}
		}
	}
LABEL_147:;
	abFirst = 0;
	return 0;
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x14], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddDiffusePass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apLight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abPointLight, bool abMultitexture, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abPointLight) {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 303u, abFirst, 1u, apLight, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		} else {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 302u, abFirst, 1u, apLight, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		}
	} else {
		if (abPointLight) {
			if (abTreeBranch) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 305u, abFirst, 1u, apLight, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			} else {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 301u, abFirst, 1u, apLight, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			}
		} else {
			if (abTreeBranch) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 304u, abFirst, 1u, apLight, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			} else {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 300u, abFirst, 1u, apLight, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			}
		}
	}
	++asCount;
	abFirst = 0;
	return 0;
}

BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddTexturePass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apLight, uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abSkinned,
	int32_t abMultitexture, bool abFacegenBlend, bool abVertexColors, bool abTreeBranch, bool abHair) {
	if (abSkinned) {
		NiNode* m_pkParent;
		const char* v15;
		char apDest[128];
		char apString1[256];
		if (!abFacegenBlend) {
			if (abVertexColors) {
				if (abHair) {
					if (static_cast<uint8_t>(abUpdateList) == 1) {
						pRenderPassList->Add(apGeom, 338, abFirst, 0, 0, 0, 0, 0);
						goto LABEL_39;
					}
				} else {
					if (static_cast<uint8_t>(abUpdateList) == 1) {
						pRenderPassList->Add(apGeom, 335, abFirst, 0, 0, 0, 0, 0);
						goto LABEL_39;
					}
				}
			} else {
				if (static_cast<uint8_t>(abUpdateList) == 1) {
					pRenderPassList->Add(apGeom, 333, abFirst, 0, 0, 0, 0, 0);
					goto LABEL_39;
				}
			}
			goto LABEL_28;
		}
		if (!abVertexColors) {
			if (static_cast<uint8_t>(abUpdateList) == 1) {
				pRenderPassList->Add(apGeom, 336, abFirst, 0, 0, 0, 0, 0);
				goto LABEL_39;
			}
			goto LABEL_28;
		}
		if (!apGeom) {
			goto LABEL_37;
		}
		m_pkParent = apGeom->m_pkParent;
		if (m_pkParent) {
			const char* m_kHandle = m_pkParent->m_kName.GetHandle();
			if (m_kHandle) {
				const char* v14 = apGeom->m_kName.GetHandle();
				if (v14) {
					const char* v17 = apGeom->m_kName.GetHandle();
					BSsprintf(apDest, 128, "Parent:%s,Child:%s", m_kHandle, v14);
				} else {
					BSsprintf(apDest, 128, "Parent:%s", m_kHandle);
				}
				goto LABEL_38;
			}
		}
		v15 = apGeom->m_kName.GetHandle();
		if (v15) {
			const char* v18 = apGeom->m_kName.GetHandle();
			BSsprintf(apDest, 128, "%s", v15);
		} else {
		LABEL_37:;
			BSsprintf(apDest, 128, "none");
		}
	LABEL_38:;
		BSsprintf(apString1, 255,
			"SHADER ERROR (%s)(%08X) : no shader to handle TEXTURE_SFgVc ( texture skinned facegen vertexcolors )",
			apDest, apGeom);
		goto LABEL_39;
	}
	if (abTreeBranch) {
		if (static_cast<uint8_t>(abUpdateList) == 1) {
			pRenderPassList->Add(apGeom, 337, abFirst, 0, 0, 0, 0, 0);
			goto LABEL_39;
		}
		goto LABEL_28;
	}
	if (abVertexColors) {
		if (abHair) {
			if (static_cast<uint8_t>(abUpdateList) == 1) {
				pRenderPassList->Add(apGeom, 332, abFirst, 0, 0, 0, 0, 0);
				goto LABEL_39;
			}
		} else {
			if (static_cast<uint8_t>(abUpdateList) == 1) {
				pRenderPassList->Add(apGeom, 334, abFirst, 0, 0, 0, 0, 0);
				goto LABEL_39;
			}
		}
		goto LABEL_28;
	}
	if (abFacegenBlend) {
		if (static_cast<uint8_t>(abUpdateList) == 1) {
			pRenderPassList->Add(apGeom, 331, abFirst, 0, 0, 0, 0, 0);
			goto LABEL_39;
		}
	LABEL_28:;
		++asCount;
		goto LABEL_39;
	}
	if (static_cast<uint8_t>(abUpdateList) != 1) {
		goto LABEL_28;
	}
	pRenderPassList->Add(apGeom, 330, abFirst, 0, 0, 0, 0, 0);
LABEL_39:;
	abFirst = 0;
	return 0;
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x14], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddSpecularPass_1x(NiGeometry* apGeom,
	ShadowSceneLight* apLight, uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned,
	bool abPointLight, bool abMultitexture, bool abBaseSpecular, bool abDecalSpecular, bool abTreeBranch) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abPointLight) {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 343u, abFirst, 1u, apLight, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		} else {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 342u, abFirst, 1u, apLight, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		}
	} else {
		if (abPointLight) {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 341u, abFirst, 1u, apLight, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		} else {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 340u, abFirst, 1u, apLight, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		}
	}
	++asCount;
	abFirst = 0;
	return 0;
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddEnvmapPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abVertexColors) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abVertexColors) {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 583u, abFirst, 0, 0, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		} else {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 582u, abFirst, 0, 0, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		}
	} else {
		if (abVertexColors) {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 581u, abFirst, 0, 0, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		} else {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 580u, abFirst, 0, 0, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		}
	}
	++asCount;
	abFirst = 0;
	return 0;
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddFogPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abTreeBranch, bool abAlphaBlend) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abTreeBranch) {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 598u, abFirst, 0, 0, 0, 0, 0);
			abFirst = 0;
			return 0;
		}
	} else {
		if (abSkinned) {
			if (abAlphaBlend) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 597u, abFirst, 0, 0, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			} else {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 596u, abFirst, 0, 0, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			}
		} else {
			if (abAlphaBlend) {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 595u, abFirst, 0, 0, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			} else {
				if (abUpdateList) {
					pRenderPassList->Add(apGeom, 594u, abFirst, 0, 0, 0, 0, 0);
					abFirst = 0;
					return 0;
				}
			}
		}
	}
	++asCount;
	abFirst = 0;
	return 0;
}

// The exe keeps the light iterators in the stack slots of abFirst and abProjShadow (both dead by then); src uses
// locals, which GetFirstActiveNonShadowLight writes before any read.
void BSShaderPPLightingProperty::AddLandscapePasses_1x(NiGeometry* apGeom, ShadowSceneLight* apSunlight,
	uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abProjShadow) {
	BSShaderPPLightingProperty* pShadeProp =
		static_cast<BSShaderPPLightingProperty*>(apGeom->GetProperty(NiProperty::SHADE));
	const bool bLowShader = BSShaderManager::eShaderLevel <= 1;
	// The exe compares the zero-extended count with iMaxLandTextures unsigned (jae).
	uint16_t usNumTextures;
	if (static_cast<uint32_t>(usLandscapeTextures) >= static_cast<uint32_t>(BSShaderManager::iMaxLandTextures)) {
		usNumTextures = static_cast<uint16_t>(BSShaderManager::iMaxLandTextures);
	} else {
		usNumTextures = usLandscapeTextures;
	}
	const uint8_t ucUpdateList = static_cast<uint8_t>(abUpdateList);
	if (BSShaderManager::bLiteBrite) {
		if (ucUpdateList == 1) {
			pRenderPassList->Add(apGeom, 147, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
		} else {
			++asCount;
		}
		abFirst = false;
		for (uint8_t i = 0; i < usNumTextures; ++i) {
			if (pShadeProp->spTextures[0][i + 1].m_pObject) {
				if (ucUpdateList == 1) {
					pRenderPassList->Add(apGeom, 498, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
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
			pRenderPassList->Add(apGeom, 147, abFirst, 1, apSunlight, nullptr, nullptr, nullptr);
		} else {
			++asCount;
		}
	} else if (ucUpdateList == 1) {
		pRenderPassList->Add(apGeom, 148, abFirst, 1, apSunlight, nullptr, nullptr, nullptr);
	} else {
		++asCount;
	}
	abFirst = false;
	if (pShadeProp->spTextures[2][0].m_pObject) {
		if (ucUpdateList == 1) {
			pRenderPassList->Add(apGeom, 497, false, 0, nullptr, nullptr, nullptr, nullptr);
		} else {
			++asCount;
		}
	}
	if (bLowShader) {
		void* pIter;
		for (ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter); pLight;
			pLight = GetNextActiveNonShadowLight(pIter)) {
			// EXE-INLINED none: NiColor::operator!= (the exe compares the three fields inline)
			const NiColor& kDiff = pLight->spLight.m_pObject->m_kDiff;
			if (NiColor_BLACK.r != kDiff.r || NiColor_BLACK.g != kDiff.g || NiColor_BLACK.b != kDiff.b) {
				if (ucUpdateList == 1) {
					pRenderPassList->Add(apGeom, 495, abFirst, 1, pLight, nullptr, nullptr, nullptr);
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
				pRenderPassList->Add(apGeom, 500, abFirst, 0, nullptr, nullptr, nullptr, nullptr);
				pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->ucExtraParam = i + 1;
			} else {
				++asCount;
			}
		}
		if (bLowShader) {
			void* pIter;
			for (ShadowSceneLight* pLight = GetFirstActiveNonShadowLight(pIter); pLight;
				pLight = GetNextActiveNonShadowLight(pIter)) {
				// EXE-INLINED none: NiColor::operator!= (the exe compares the three fields inline)
				const NiColor& kDiff = pLight->spLight.m_pObject->m_kDiff;
				if (NiColor_BLACK.r != kDiff.r || NiColor_BLACK.g != kDiff.g || NiColor_BLACK.b != kDiff.b) {
					if (ucUpdateList == 1) {
						pRenderPassList->Add(apGeom, 501, abFirst, 1, pLight, nullptr, nullptr, nullptr);
						pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->ucExtraParam = i + 1;
					} else {
						++asCount;
					}
				}
			}
		}
	}
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddTextureEffectPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 591u, abFirst, 0, 0, 0, 0, 0);
		} else {
			++asCount;
		}
		abFirst = 0;
		return 0;
	} else {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 590u, abFirst, 0, 0, 0, 0, 0);
		} else {
			++asCount;
		}
		abFirst = 0;
		return 0;
	}
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddRenderNormalsPass_1x(NiGeometry* apGeom, uint16_t& asCount,
	int32_t aiUpdateList, bool& abFirst, bool abSkinned, bool abRefractionFire) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abRefractionFire) {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 484u, abFirst, 0, 0, 0, 0, 0);
			abFirst = 0;
			return 0;
		}
	} else {
		if (abSkinned) {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 483u, abFirst, 0, 0, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		} else {
			if (abUpdateList) {
				pRenderPassList->Add(apGeom, 482u, abFirst, 0, 0, 0, 0, 0);
				abFirst = 0;
				return 0;
			}
		}
	}
	++asCount;
	abFirst = 0;
	return 0;
}

// The exe compares the update flag's low byte with 1 at each test (`cmp byte ptr [esp+0x10], 1`).
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddRenderNormalsClearPass_1x(NiGeometry* apGeom,
	uint16_t& asCount, int32_t aiUpdateList, bool& abFirst, bool abSkinned) {
	const bool abUpdateList = static_cast<uint8_t>(aiUpdateList) == 1;
	if (abSkinned) {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 486u, abFirst, 0, 0, 0, 0, 0);
		} else {
			++asCount;
		}
		abFirst = 0;
		return 0;
	} else {
		if (abUpdateList) {
			pRenderPassList->Add(apGeom, 485u, abFirst, 0, 0, 0, 0, 0);
		} else {
			++asCount;
		}
		abFirst = 0;
		return 0;
	}
}
