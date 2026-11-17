#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/SpeedTreeBranchShaderProperty.h"
#include "BSShader/SpeedTreeFrondShaderProperty.h"
#include "BSShader/SpeedTreeLeafShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// The pass helpers' bool flags are bytes the exe pushes as whole dwords (each helper reads the low byte); the shared
// pass flag the helpers clear is bFirst, the z-only pass has its own (bZOnlyFirst).
void BSShaderPPLightingProperty::GetRenderPasses_2x(NiGeometry* apGeom, int32_t aiEnabledPasses, uint16_t& asCount,
	uint32_t auiRenderMode, BSShaderAccumulator* apAccumulator, int32_t abUpdateList) {
	ShadowSceneLight* pLight;
	ShadowSceneLight* pLight2;
	ShadowSceneLight* pLight3;
	NiMaterialProperty* pMaterial;
	int32_t iExtraLight;
	int32_t iLightsLeft;

	uint32_t uiFlags = ulFlags[0];
	bool bSpecular = (uiFlags & BSS1_Specular) != 0;
	bool bEnvMapping = (uiFlags & BSS1_Environment_Mapping) != 0;
	const bool bEyeEnvMapping = (uiFlags & BSS1_Eye_Environment_Mapping) != 0;
	const bool bWindowEnvMapping = (uiFlags & BSS1_Window_Environment_Mapping) != 0;
	const bool bSkinned = (uiFlags & BSS1_Skinned) != 0;
	bool bSinglePass = (uiFlags & BSS1_Single_Pass) != 0 || auiRenderMode == BSShaderManager::BSSM_RENDER_SINGLEPASS;
	bool bAlphaTexture = (uiFlags & BSS1_Alpha_Texture) != 0;
	const bool bFaceGen = (uiFlags & BSS1_FaceGen) != 0;
	const bool bGlowMap = spTextures[2][0].m_pObject != nullptr && !bFaceGen;
	const bool bVertexColors =
		apGeom->m_spModelData.m_pObject->m_pkColor != nullptr || (ulFlags[1] & BSS2_Vertex_Colors) != 0;
	// The exe converts w to a 64-bit integer (fistp qword) and tests the low dword. Pass bit 0x20 of iEnabledPasses
	// is the projected shadow's (no src enum).
	const bool bProjShadow = !BSShaderManager::bInterior && BSShaderManager::pProjectedShadowTexture.m_pObject &&
		(BSShaderManager::iEnabledPasses & 0x20) != 0 && BSShaderManager::eShaderLevel >= 2 &&
		!(uiFlags & BSS1_Non_Projective_Shadows) &&
		static_cast<uint32_t>(static_cast<int64_t>(ShadowLightShader::kShadowProjTransform.w)) != 0 &&
		BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 1 &&
		BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->cSceneGraphIndex != 3;
	const bool bTreeBranch = DynamicCastMember(&SpeedTreeBranchShaderProperty::ms_RTTI) != nullptr;
	const bool bTreeFrond = DynamicCastMember(&SpeedTreeFrondShaderProperty::ms_RTTI) != nullptr;
	const bool bTreeLeaf = DynamicCastMember(&SpeedTreeLeafShaderProperty::ms_RTTI) != nullptr;
	uiFlags = ulFlags[0];
	const bool bTreeBillboard = (uiFlags & BSS1_Tree_Billboard) != 0;
	const bool bParallax = (uiFlags & BSS1_Parallax_Shader) != 0;
	const bool bModelSpaceNormals = (uiFlags & BSS1_Model_Space_Normals) != 0;
	const bool bLandscape = (uiFlags & BSS1_Landscape) != 0;
	const bool bRefraction = (uiFlags & BSS1_Refraction) != 0;
	const bool bFireRefraction = (uiFlags & BSS1_Fire_Refraction) != 0;
	const bool bZPrepass = (uiFlags & BSS1_Z_Prepass) != 0;
	const bool bShowInLocalMap = !(uiFlags & BSS1_Localmap_Hide_Secret) || BSShaderManager::bLiteBrite;
	const bool bHair = (uiFlags & BSS1_Hair) != 0;
	const bool bDecal = (uiFlags & BSS1_Decal_Single_Pass) || (uiFlags & BSS1_Dynamic_Decal_Single_Pass);
	// The exe reads the property state's alpha property here without a null test.
	const NiAlphaProperty* pStateAlpha =
		static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::ALPHA].m_pObject);
	const uint32_t uiFlags2 = ulFlags[1];
	const bool bLOD = !(pStateAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK) &&
		(apAccumulator->eAccumulationMode == BSShaderAccumulator::BSSA_ACCUMLOD || (uiFlags2 & BSS2_LOD_Building) ||
			(uiFlags2 & BSS2_LOD_Landscape));

	NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->GetProperty(NiProperty::ALPHA));
	if (!pAlpha) {
		// The exe tests the property state's address for null before reading it.
		NiPropertyState* pState = &apGeom->m_PropertyState;
		pAlpha = pState ? static_cast<NiAlphaProperty*>(pState->m_aspProps[NiProperty::ALPHA].m_pObject) : nullptr;
	}
	const bool bAlphaBlend =
		(pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK)) || fAlpha < 1.0f || fFadeAlpha < 1.0f;
	bool bAlphaTest = false;
	if (pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK)) {
		bAlphaTest = true;
		bAlphaTexture = false;
	}

	if (!(ulFlags[1] & BSS2_stinger_prop)) {
		if (bEyeEnvMapping) {
			bEnvMapping =
				BSShaderManager::fEyeEnvMapLODEnd == 0.0f || !(fCameraDistance >= BSShaderManager::fEyeEnvMapLODEnd);
			bSpecular = false;
		} else if (BSShaderManager::fEnvMapLODEnd != 0.0f && fCameraDistance >= BSShaderManager::fEnvMapLODEnd) {
			bEnvMapping = false;
		}
		if (BSShaderManager::fSpecularLODEnd != 0.0f && fCameraDistance >= BSShaderManager::fSpecularLODEnd) {
			bSpecular = false;
		}
	}
	ShadowSceneLight* pSunLight = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode->pSunLight;
	const bool bSunSpecular = bSpecular && pSunLight->spLight.m_pObject->m_kSpec != NiColor_BLACK;

	if (bAlphaBlend) {
		bool bZOnlyFirst = true;
		if ((bZPrepass || fFadeAlpha < 1.0f) && !(pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::ALPHA_BLEND_MASK))) {
			uiFlags = ulFlags[0];
			if (!(uiFlags & BSS1_Refraction) && !(uiFlags & BSS1_Fire_Refraction)) {
				AddZOnlyPass_1x(apGeom, asCount, abUpdateList, bZOnlyFirst, bSkinned, bAlphaTest);
				if (fFadeAlpha < 1.0f || fAlpha < 1.0f) {
					pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint =
						BSSP_HINT_GROUP_FADING;
				}
				bSpecular = false;
			}
		}
		bSinglePass = true;
	}

	bool bFirst = true;
	TextureEffectData* pTexEffect = spTexEffectData.m_pObject;
	if (pTexEffect && pTexEffect->spBlockOutTexture.m_pObject) {
		AddZOnlyTextureEffectPass_1x(apGeom, asCount, abUpdateList, bFirst, bSkinned);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_FADING;
	}

	const uint16_t usNumLights = GetNumberofActiveNonShadowLights();
	const bool bAmbientPass = (aiEnabledPasses & 1) != 0;
	const bool bDiffusePass = (aiEnabledPasses & 2) != 0;
	const bool bTexturePass = (aiEnabledPasses & 4) != 0;
	const bool bSpecularPass = (aiEnabledPasses & 8) != 0 && bSpecular;
	const bool bEnvMapPass = bEnvMapping && (aiEnabledPasses & 8) != 0;
	bool bAllPasses = (aiEnabledPasses & uiAllPasses) == uiAllPasses;
	bool bSunDiffuse = true;
	const int32_t iNumLights = usNumLights;
	uint32_t uiDiffuseLightsLeft = usNumLights;

	if (bRefraction || bFireRefraction) {
		AddRenderNormalsPass_1x(apGeom, asCount, abUpdateList, bFirst, bSkinned, bFireRefraction);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_GROUP_REFRACT;
		return;
	}
	if (ulFlags[1] & BSS2_1st_person) {
		AddRenderNormalsClearPass_1x(apGeom, asCount, abUpdateList, bFirst, bSkinned);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint =
			BSSP_HINT_GROUP_1STPERSONNOREFRACT;
	}
	if (bTreeFrond || bTreeLeaf) {
		// The PDB takes the render mode as uint32_t.
		GetRenderPasses_1x(apGeom, aiEnabledPasses, asCount, static_cast<uint32_t>(apAccumulator->eRenderMode),
			apAccumulator, abUpdateList);
		return;
	}

	void* pLightIter = nullptr;
	if (BSShaderManager::GetShadows() && !bLOD && !bModelSpaceNormals) {
		void* pShadowIter = nullptr;
		for (ShadowSceneLight* pShadowLight = GetFirstActiveLight(pShadowIter); pShadowLight;
			pShadowLight = GetNextActiveLight(pShadowIter)) {
			if (pShadowLight->bCastShadow) {
				pRenderPassList->Add(apGeom, 0, true, 1, pShadowLight, nullptr, nullptr, nullptr);
				BSShaderProperty::RenderPass* pPass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
				if (bSkinned) {
					pPass->usPassEnum = BSShaderManager::BSSM_2x_SIMPLESHADOW_S;
				} else if (bLandscape) {
					pPass->usPassEnum = BSShaderManager::BSSM_2x_SIMPLESHADOW_LAND;
				} else {
					pPass->usPassEnum =
						bTreeBranch ? BSShaderManager::BSSM_2x_SIMPLESHADOW_Sb : BSShaderManager::BSSM_2x_SIMPLESHADOW;
				}
				pPass->bLastPass = true;
			}
		}
	}

	if (bLandscape) {
		AddLandscapePasses_2x(apGeom, pSunLight, asCount, abUpdateList, bFirst, bSpecular, bProjShadow);
		goto LastPass;
	}
	if (!bShowInLocalMap) {
		goto LastPass;
	}
	if (!bAmbientPass) {
		goto DiffusePasses;
	}
	if (BSShaderManager::bBlurShader) {
		goto SunPasses;
	}
	if (!bSinglePass && !bAlphaBlend) {
		if (!bDiffusePass) {
			goto AmbientPass;
		}
		if (!bTexturePass || bAlphaTexture || bTreeBillboard) {
			goto SunPasses;
		}
	}

	// One pass for ambient, diffuse and textures, with the specular passes in it (ADTS*) or after it (ADT*).
	if (usNumLights <= 1 && bSunSpecular && (usNumLights == 0 || !bProjShadow) && bSpecularPass && !bFaceGen) {
		goto AmbDiffTexSpecular;
	}
	if (!bSinglePass || !(bParallax || bHair || bProjShadow || bModelSpaceNormals || bTreeBranch || bGlowMap)) {
		goto OptimizedPass;
	}
	if (bFaceGen) {
		goto AmbDiffTex;
	}
AmbDiffTexSpecular:
	if (!bSpecularPass) {
		goto AmbDiffTex;
	}
	if (!usNumLights) {
		ADTS(apGeom, pSunLight, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, 0, bProjShadow,
			bParallax, bTreeBranch, bHair);
	} else {
		pLight = GetFirstActiveNonShadowLight(pLightIter);
		ADTS2(apGeom, pSunLight, pLight, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, 0, bProjShadow,
			bParallax, bTreeBranch, bHair);
	}
	bAllPasses = false;
	goto EnvMapPass;
AmbDiffTex:
	if (!usNumLights) {
		ADT(apGeom, pSunLight, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, false, bProjShadow,
			bParallax, bModelSpaceNormals, bTreeBranch, bTreeBillboard, bHair, bAlphaTest);
	} else {
		pLight = GetFirstActiveNonShadowLight(pLightIter);
		ADT2(apGeom, pSunLight, pLight, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, 0, bProjShadow,
			bParallax, bTreeBranch, bHair);
	}
	bAllPasses = false;
	goto EnvMapPass;

	// The 1.0 pass for up to five (four with face gen, two with specular) lights, plus one extra light when the
	// optimized passes are on and nothing needs a pass of its own.
OptimizedPass:
	pMaterial = static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[NiProperty::MATERIAL].m_pObject);
	iExtraLight = 1;
	if (!BSShaderManager::bUseOptPasses) {
		goto NoExtraLight;
	}
	if (pMaterial) {
		// EXE-INLINED none: NiMaterialProperty::GetEmittance
		const NiColor* pEmit = pMaterial->m_external_emit;
		if (!pEmit) {
			pEmit = &pMaterial->m_emit;
		}
		if (!(*pEmit == NiColor_BLACK)) {
			goto NoExtraLight;
		}
	}
	if (pSunLight->spLight.m_pObject->m_kDiff != NiColor_BLACK || !GetFlag(BSSP_FLAG_VERTEXCOLORS) ||
		(pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK) && fFadeAlpha < 1.0f) ||
		GetFlag(BSSP_FLAG_DECAL) || GetFlag(BSSP_FLAG_DYNAMIC_DECAL) || GetFlag(BSSP_ENVMAP) ||
		GetFlag(BSSP_FLAG_WINDOWREFLECT) || GetFlag(BSSP_FLAG_EYEREFLECT) || apGeom->m_spSkinInstance.m_pObject) {
	NoExtraLight:
		iExtraLight = 0;
	}
	if (!bSinglePass) {
		if (!((iNumLights <= iExtraLight + 5 && !bSpecularPass && !bFaceGen) ||
				(iNumLights <= iExtraLight + 4 && !bSpecularPass && bFaceGen) ||
				(iNumLights <= iExtraLight + 2 && bSpecularPass && !bFaceGen))) {
			goto SunPasses;
		}
		if (usNumLights <= 1 && !bSpecular) {
			goto SunPasses;
		}
		if (bParallax || bHair || bProjShadow || bModelSpaceNormals || bTreeBranch || bGlowMap) {
			goto SunPasses;
		}
	}
	AddAmbDiffTexSpecular10Pass_2x(apGeom, pSunLight, asCount, abUpdateList, bFirst, bSkinned, bGlowMap, bSpecularPass,
		bFaceGen, iExtraLight != 0);
	bAllPasses = false;
	goto EnvMapPass;

	// Sunlight passes: ambient, diffuse and textures in one (ADT*), or ambient and diffuse with one or two point
	// lights (AD2, AD3), or the ambient pass alone.
SunPasses:
	if (!bDiffusePass) {
		goto AmbientPass;
	}
	if (bTexturePass && !(bFaceGen && BSShaderManager::bInterior) && (usNumLights <= 1 || bSinglePass)) {
		if (!usNumLights) {
			ADT(apGeom, pSunLight, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, false, bProjShadow,
				bParallax, bModelSpaceNormals, bTreeBranch, bTreeBillboard, bHair, bAlphaTest);
		} else {
			pLight = GetFirstActiveNonShadowLight(pLightIter);
			ADT2(apGeom, pSunLight, pLight, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, 0,
				bProjShadow, bParallax, bTreeBranch, bHair);
		}
		if (bSpecularPass) {
			goto SpecularPasses;
		}
		bAllPasses = false;
		goto EnvMapPass;
	}
	if (bFaceGen && BSShaderManager::bInterior) {
		goto AmbientPass;
	}
	pLight = GetFirstActiveNonShadowLight(pLightIter);
	if (usNumLights <= 1) {
		AD2(apGeom, pSunLight, pLight, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, false,
			bProjShadow, bParallax, bTreeBranch);
		uiDiffuseLightsLeft = 0;
	} else {
		pLight2 = GetNextActiveNonShadowLight(pLightIter);
		AD3(apGeom, pSunLight, pLight, pLight2, &asCount, abUpdateList, &bFirst, bSkinned, bGlowMap, bFaceGen, 0,
			bProjShadow, bParallax, bTreeBranch);
		uiDiffuseLightsLeft -= 2;
	}
	bSunDiffuse = false;
	goto DiffusePasses;
AmbientPass:
	AddAmbientPass_1x(apGeom, pSunLight, asCount, abUpdateList, bFirst, bSkinned, bAlphaTest, bGlowMap, bTreeBranch);
DiffusePasses:
	if (bDiffusePass) {
		if (bSunDiffuse) {
			AddDiffusePass_1x(apGeom, pSunLight, asCount, abUpdateList, bFirst, bSkinned, false, false, bTreeBranch);
			pLight = GetFirstActiveNonShadowLight(pLightIter);
		} else {
			pLight = GetNextActiveNonShadowLight(pLightIter);
		}
		for (; pLight; pLight = GetNextActiveNonShadowLight(pLightIter)) {
			if (uiDiffuseLightsLeft > 2) {
				pLight2 = GetNextActiveNonShadowLight(pLightIter);
				pLight3 = GetNextActiveNonShadowLight(pLightIter);
				DiffusePoint3(apGeom, pLight, pLight2, pLight3, &asCount, abUpdateList, &bFirst, bSkinned, bFaceGen, 0,
					bParallax, bTreeBranch);
				uiDiffuseLightsLeft -= 3;
			} else {
				pLight2 = GetNextActiveNonShadowLight(pLightIter);
				DiffusePoint2(apGeom, pLight, pLight2, &asCount, abUpdateList, &bFirst, bSkinned, bFaceGen, 0,
					bParallax, bTreeBranch);
				if (uiDiffuseLightsLeft > 1) {
					uiDiffuseLightsLeft -= 2;
				} else {
					uiDiffuseLightsLeft = 0;
				}
			}
		}
	}
	if (bTexturePass) {
		Textures(apGeom, pSunLight, &asCount, abUpdateList, &bFirst, bSkinned, 0, bFaceGen, bVertexColors, bTreeBranch,
			0, bParallax, bHair);
	}
	if (!bSpecularPass) {
		goto EnvMapPass;
	}
SpecularPasses:
	iLightsLeft = iNumLights;
	if (bSunSpecular) {
		Specular(apGeom, pSunLight, &asCount, abUpdateList, &bFirst, bSkinned, 0, bProjShadow, bParallax, bTreeBranch,
			bHair);
	}
	for (pLight = GetFirstActiveNonShadowLight(pLightIter); pLight; pLight = GetNextActiveNonShadowLight(pLightIter)) {
		if (iLightsLeft > 2) {
			pLight2 = GetNextActiveNonShadowLight(pLightIter);
			pLight3 = GetNextActiveNonShadowLight(pLightIter);
			SpecularPoint3(apGeom, pLight, pLight2, pLight3, &asCount, abUpdateList, &bFirst, bSkinned, bParallax,
				bTreeBranch, bHair);
			iLightsLeft -= 3;
		} else if (iLightsLeft > 1) {
			pLight2 = GetNextActiveNonShadowLight(pLightIter);
			SpecularPoint2(apGeom, pLight, pLight2, &asCount, abUpdateList, &bFirst, bSkinned, bParallax, bTreeBranch,
				bHair);
			iLightsLeft -= 2;
		} else if (iLightsLeft > 0) {
			Specular(apGeom, pLight, &asCount, abUpdateList, &bFirst, bSkinned, 0, bProjShadow, bParallax, bTreeBranch,
				bHair);
			--iLightsLeft;
		}
	}
EnvMapPass:
	if (bEnvMapPass) {
		pLight = GetFirstActiveNonShadowLight(pLightIter);
		AddPass_2X_EnvMap(apGeom, pSunLight, pLight, &asCount, abUpdateList, &bFirst, bSkinned, bEyeEnvMapping,
			bWindowEnvMapping);
		goto FogPass;
	}
LastPass:
	if (!bAllPasses) {
		BSShaderProperty::RenderPassArray* pArray = pRenderPassList;
		if (pArray->iSize && !bLandscape) {
			BSShaderProperty::RenderPass* pPass = pArray->pPassA.m_pBase[pArray->iSize - 1];
			if (pPass->usPassEnum != BSShaderManager::BSSM_FOG && pPass->usPassEnum != BSShaderManager::BSSM_FOG_S) {
				pPass->bLastPass = true;
			}
		}
		goto TextureEffectPass;
	}
	if (bLandscape) {
		goto TextureEffectPass;
	}
FogPass:
	AddFogPass_1x(apGeom, asCount, abUpdateList, bFirst, bSkinned, bTreeBranch, bAlphaBlend);
TextureEffectPass:
	if (spTexEffectData.m_pObject) {
		AddDepthPassOrCount(apGeom, &asCount, abUpdateList, &bFirst, bSkinned);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->eAccumulationHint = BSSP_HINT_SORT;
	}
	if (BSShaderManager::bBlurShader) {
		if (bAmbientPass) {
			AddSelfIllumAlphaPass(apGeom, asCount, abUpdateList, bSkinned, bGlowMap);
		}
		if (!bTreeBranch && !bTreeBillboard && !bParallax && !bModelSpaceNormals && !bLandscape && !bHair) {
			SelfIlluminationBlock(apGeom, &asCount, abUpdateList, bSkinned);
		}
	}

	{
		etAccumulationHint eHint = BSSP_HINT_NORMAL;
		if (bLOD) {
			eHint = BSSP_HINT_GROUP_LOD;
		} else if (bDecal) {
			eHint = BSSP_HINT_GROUP_DECAL;
		} else if (bAlphaBlend) {
			eHint = fFadeAlpha < 1.0f ? BSSP_HINT_GROUP_FADING : BSSP_HINT_SORT;
		}
		// Passes whose hint is still normal take the property's hint; the exe reloads the list after each pass.
		BSShaderProperty::RenderPassArray* pArray = pRenderPassList;
		if (pArray) {
			for (uint32_t i = 0; i < pArray->iSize; ++i) {
				BSShaderProperty::RenderPass* pPass = pArray->pPassA.m_pBase[i];
				if (pPass->eAccumulationHint == BSSP_HINT_NORMAL) {
					pPass->eAccumulationHint = eHint;
				}
				pArray = pRenderPassList;
			}
		}
	}
}

// The pass enums are IDA's BSShaderManager::RenderPassType names (not in the PDB): 0x22F BSSM_LANDO_Am, 0x61
// BSSM_AMBIENT_Mn, 0x62 BSSM_AMBIENT_F, 0x63 BSSM_AMBIENT_FAt.
void BSShaderPPLightingProperty::AddSilhouettePass_2x(NiGeometry* apGeom, bool abSkinned) {
	if (ulFlags[0] & 0x4000) {
		pSilhouettePassList->Add(apGeom, 0x22F, true, 0, nullptr, nullptr, nullptr, nullptr);
	} else if (ulFlags[1] & 2) {
		pSilhouettePassList->Add(apGeom, 0x61, true, 0, nullptr, nullptr, nullptr, nullptr);
	} else if (!abSkinned) {
		NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[0].m_pObject);
		if (pAlpha && (pAlpha->m_uFlags & NiAlphaProperty::TEST_ENABLE_MASK)) {
			pSilhouettePassList->Add(apGeom, 0x63, true, 0, nullptr, nullptr, nullptr, nullptr);
		} else {
			pSilhouettePassList->Add(apGeom, 0x62, true, 0, nullptr, nullptr, nullptr, nullptr);
		}
	}

	RenderPassArray* pArray = pSilhouettePassList;
	if (!pArray->iSize) {
		return;
	}
	RenderPass* pPass = pArray->pPassA.m_pBase[0];
	uint32_t uiFlags2 = ulFlags[1];
	if ((uiFlags2 & 2) || (uiFlags2 & 4)) {
		pPass->eAccumulationHint = BSSP_HINT_GROUP_LOD;
	} else if (ulFlags[0] & 0x4000000) {
		pPass->eAccumulationHint = (uiFlags2 & 0x200000) ? BSSP_HINT_SORT : BSSP_HINT_GROUP_DECAL;
	} else {
		pPass->eAccumulationHint = BSSP_HINT_NORMAL;
	}
}

// Adds the single ambient, diffuse, texture and specular pass for the sun and up to 10 lights (when abUpdateList)
// and picks its pass from the skinned, glow map, specular, face-gen and light-count flags; otherwise counts a pass.
// Returns the pass (eax on every path; null when abUpdateList is 0). The exe keeps the light iterator in apGeom's
// argument slot; src gives it its own local.
BSShaderProperty::RenderPass* BSShaderPPLightingProperty::AddAmbDiffTexSpecular10Pass_2x(NiGeometry* apGeom,
	ShadowSceneLight* apSunlight, uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abSkinned,
	bool abGlowMap, bool abSpecular, bool abFaceGenBlend, bool abExtraLight) {
	BSShaderProperty::RenderPass* ppass = nullptr;
	if (static_cast<uint8_t>(abUpdateList)) {
		int32_t iMaxLights;
		pRenderPassList->Add(apGeom, 0, 1, 0, 0, 0, 0, 0);
		ppass = pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1];
		uint8_t ucLights = GetNumberofActiveNonShadowLights() + 1;
		ppass->ucNumLights = ucLights;
		ppass->ucMaxNumLights = ucLights;
		ppass->ClearSceneLights();
		ppass->AllocateLightArray(ppass->ucNumLights);
		*ppass->ppSceneLights = apSunlight;
		int32_t iNumLights = ppass->ucNumLights;
		if (abSpecular) {
			if (abFaceGenBlend) {
			FillLights:;
				ppass->ucNumLights = static_cast<uint8_t>(iNumLights);
				void* pIterator = nullptr;
				ShadowSceneLight* pitlight = GetFirstActiveNonShadowLight(pIterator);
				for (int32_t ilight = 1; pitlight; pitlight = GetNextActiveNonShadowLight(pIterator)) {
					if (ilight >= ppass->ucNumLights) {
						break;
					}
					ppass->ppSceneLights[ilight++] = pitlight;
				}
				goto PickPass;
			}
			iMaxLights = abExtraLight + 3;
		} else {
			if (abFaceGenBlend) {
				iMaxLights = abExtraLight + 5;
			} else {
				iMaxLights = abExtraLight + 6;
			}
		}
		if (iNumLights >= iMaxLights) {
			iNumLights = iMaxLights;
		}
		goto FillLights;
	}
PickPass:;
	// The exe reads the light count once here (a null pass when !abUpdateList faults in the exe).
	const uint32_t uiNumLights = ppass->ucNumLights;
	if (abSkinned) {
		if (abGlowMap) {
			if (abSpecular) {
				if (static_cast<uint8_t>(abUpdateList) == 1) {
					ppass->usPassEnum = 213;
					abFirst = false;
					return ppass;
				}
			} else {
				if (uiNumLights >= 5u) {
					if (static_cast<uint8_t>(abUpdateList) == 1) {
						ppass->usPassEnum = 199;
						abFirst = false;
						return ppass;
					}
				} else {
					if (static_cast<uint8_t>(abUpdateList) == 1) {
						ppass->usPassEnum = 206;
						abFirst = false;
						return ppass;
					}
				}
			}
		} else {
			if (abSpecular) {
				if (abFaceGenBlend) {
					if (static_cast<uint8_t>(abUpdateList) == 1) {
						ppass->usPassEnum = 214;
						abFirst = false;
						return ppass;
					}
				} else {
					if (static_cast<uint8_t>(abUpdateList) == 1) {
						ppass->usPassEnum = 212;
						abFirst = false;
						return ppass;
					}
				}
			} else {
				if (abFaceGenBlend) {
					if (uiNumLights >= 5u) {
						if (static_cast<uint8_t>(abUpdateList) == 1) {
							ppass->usPassEnum = 200;
							abFirst = false;
							return ppass;
						}
					} else {
						if (static_cast<uint8_t>(abUpdateList) == 1) {
							ppass->usPassEnum = 207;
							abFirst = false;
							return ppass;
						}
					}
				} else {
					if (uiNumLights >= 5u) {
						if (static_cast<uint8_t>(abUpdateList) == 1) {
							ppass->usPassEnum = 198;
							abFirst = false;
							return ppass;
						}
					} else {
						if (static_cast<uint8_t>(abUpdateList) == 1) {
							ppass->usPassEnum = 205;
							abFirst = false;
							return ppass;
						}
					}
				}
			}
		}
		goto CountPass;
	}
	if (abGlowMap) {
		if (abSpecular) {
			if (static_cast<uint8_t>(abUpdateList) == 1) {
				ppass->usPassEnum = 210;
				abFirst = false;
				return ppass;
			}
		} else {
			if (uiNumLights >= 5u) {
				if (static_cast<uint8_t>(abUpdateList) == 1) {
					ppass->usPassEnum = 196;
					abFirst = false;
					return ppass;
				}
			} else {
				if (static_cast<uint8_t>(abUpdateList) == 1) {
					ppass->usPassEnum = 203;
					abFirst = false;
					return ppass;
				}
			}
		}
		goto CountPass;
	}
	if (abSpecular) {
		if (abFaceGenBlend) {
			if (static_cast<uint8_t>(abUpdateList) == 1) {
				ppass->usPassEnum = 211;
				abFirst = false;
				return ppass;
			}
			goto CountPass;
		}
		if (static_cast<uint8_t>(abUpdateList) == 1) {
			NiMaterialProperty* pMaterial =
				static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[2].m_pObject);
			BSShaderPPLightingProperty* pShaderProp =
				static_cast<BSShaderPPLightingProperty*>(apGeom->m_PropertyState.m_aspProps[3].m_pObject);
			NiAlphaProperty* pAlpha = static_cast<NiAlphaProperty*>(apGeom->m_PropertyState.m_aspProps[0].m_pObject);
			if (!BSShaderManager::bUseOptPasses) {
				goto SpecularPass;
			}
			if (pMaterial) {
				// EXE-INLINED none: NiMaterialProperty::GetEmittance
				const NiColor* pEmit = pMaterial->m_external_emit;
				if (!pEmit) {
					pEmit = &pMaterial->m_emit;
				}
				// NiColor::operator== is the exe's NiPoint3::operator== (0x4390C0, folded).
				if (!(*pEmit == NiColor_BLACK)) {
					goto SpecularPass;
				}
			}
			if (apSunlight->spLight.m_pObject->m_kDiff.operator!=(NiColor_BLACK) || !pShaderProp->GetFlag(37u) ||
				(pAlpha && (pAlpha->m_uFlags & 512) != 0 && pShaderProp->fFadeAlpha < 1.0f) || GetFlag(7u) ||
				GetFlag(21u) || GetFlag(17u) || pShaderProp->GetFlag(26u) || pShaderProp->GetFlag(27u)) {
			SpecularPass:;
				ppass->usPassEnum = 208;
				abFirst = false;
			} else {
				ppass->usPassEnum = 209;
				abFirst = false;
			}
			return ppass;
		}
	CountPass:;
		++asCount;
		abFirst = false;
		return ppass;
	}
	if (abFaceGenBlend) {
		if (uiNumLights >= 5u) {
			if (static_cast<uint8_t>(abUpdateList) == 1) {
				ppass->usPassEnum = 197;
				abFirst = false;
				return ppass;
			}
		} else {
			if (static_cast<uint8_t>(abUpdateList) == 1) {
				ppass->usPassEnum = 204;
				abFirst = false;
				return ppass;
			}
		}
		goto CountPass;
	}
	if (uiNumLights >= 5u) {
		if (static_cast<uint8_t>(abUpdateList) == 1) {
			ppass->usPassEnum = 195;
			abFirst = false;
			return ppass;
		}
		goto CountPass;
	}
	if (static_cast<uint8_t>(abUpdateList) != 1) {
		goto CountPass;
	}
	NiMaterialProperty* pMaterial = static_cast<NiMaterialProperty*>(apGeom->m_PropertyState.m_aspProps[2].m_pObject);
	BSShaderPPLightingProperty* pShaderProp =
		static_cast<BSShaderPPLightingProperty*>(apGeom->m_PropertyState.m_aspProps[3].m_pObject);
	if (!BSShaderManager::bUseOptPasses) {
		goto DiffusePass;
	}
	if (pMaterial) {
		// EXE-INLINED none: NiMaterialProperty::GetEmittance
		const NiColor* pEmit = pMaterial->m_external_emit;
		if (!pEmit) {
			pEmit = &pMaterial->m_emit;
		}
		// NiColor::operator== is the exe's NiPoint3::operator== (0x4390C0, folded).
		if (!(*pEmit == NiColor_BLACK)) {
			goto DiffusePass;
		}
	}
	if (apSunlight->spLight.m_pObject->m_kDiff.operator!=(NiColor_BLACK) || !pShaderProp->GetFlag(37u) ||
		pShaderProp->fFadeAlpha < 1.0f || GetFlag(7u) || GetFlag(21u) || GetFlag(17u) || pShaderProp->GetFlag(26u) ||
		pShaderProp->GetFlag(27u)) {
	DiffusePass:;
		ppass->usPassEnum = 201;
		abFirst = false;
	} else {
		ppass->usPassEnum = 202;
		abFirst = false;
	}
	return ppass;
}

void BSShaderPPLightingProperty::AddLandscapePasses_2x(NiGeometry* apGeom, ShadowSceneLight* apSunlight,
	uint16_t& asCount, int32_t abUpdateList, bool& abFirst, bool abSpecular, bool abProjShadow) {
	ShadowSceneLight* plight;
	ShadowSceneLight* plight1;
	ShadowSceneLight* plight2;
	uint16_t v23;
	void* kiter;
	kiter = 0;
	for (plight = GetFirstActiveNonShadowLight(kiter); plight; plight = GetNextActiveNonShadowLight(kiter)) {
		NiLight* m_pObject = plight->spLight.m_pObject;
		double r = m_pObject->m_kDiff.r;
		NiColor* p_m_kDiff = &m_pObject->m_kDiff;
		if (NiColor_BLACK.r != r) {
			break;
		}
		if (NiColor_BLACK.g != p_m_kDiff->g) {
			break;
		}
		if (NiColor_BLACK.b != p_m_kDiff->b) {
			break;
		}
	}
	for (plight1 = GetNextActiveNonShadowLight(kiter); plight1; plight1 = GetNextActiveNonShadowLight(kiter)) {
		NiLight* v14 = plight1->spLight.m_pObject;
		double v15 = v14->m_kDiff.r;
		NiColor* v16 = &v14->m_kDiff;
		if (NiColor_BLACK.r != v15) {
			break;
		}
		if (NiColor_BLACK.g != v16->g) {
			break;
		}
		if (NiColor_BLACK.b != v16->b) {
			break;
		}
	}
	for (plight2 = GetNextActiveNonShadowLight(kiter); plight2; plight2 = GetNextActiveNonShadowLight(kiter)) {
		NiLight* v18 = plight2->spLight.m_pObject;
		double v19 = v18->m_kDiff.r;
		NiColor* v20 = &v18->m_kDiff;
		if (NiColor_BLACK.r != v19) {
			break;
		}
		if (NiColor_BLACK.g != v20->g) {
			break;
		}
		if (NiColor_BLACK.b != v20->b) {
			break;
		}
	}
	uint16_t v21 = QLandscapeTextureCount();
	if (v21 >= 7u) {
		v21 = 7;
	}
	bool v22 = (ulFlags[1] & BSS2_Skip_Normal_Maps) != 0;
	if (plight) {
		uint16_t v24;
		v24 = 8 * v21 + 507;
		if (abProjShadow) {
			++v24;
		}
		if ((ulFlags[1] & BSS2_Skip_Normal_Maps) != 0) {
			v24 += 2;
		}
		if (static_cast<uint8_t>(abUpdateList) == 1) {
			pRenderPassList->Add(apGeom, v24, 1, 4, apSunlight, plight, plight1, plight2);
			goto LABEL_32;
		}
	LABEL_31:;
		++asCount;
		goto LABEL_32;
	}
	v23 = 8 * v21 + 503;
	if (abProjShadow) {
		++v23;
	}
	if ((ulFlags[1] & BSS2_Skip_Normal_Maps) != 0) {
		v23 += 2;
	}
	if (static_cast<uint8_t>(abUpdateList) != 1) {
		goto LABEL_31;
	}
	pRenderPassList->Add(apGeom, v23, 1, 1, apSunlight, 0, 0, 0);
LABEL_32:;
	abFirst = 0;
	if (static_cast<uint8_t>(abUpdateList) == 1 &&
		(BSShaderManager::bInLODWorld && (ulFlags[1] & BSS2_No_LOD_Land_Blend) == 0)) {
		pRenderPassList->Add(apGeom, 560, 0, 1, apSunlight, 0, 0, 0);
		pRenderPassList->pPassA.m_pBase[pRenderPassList->iSize - 1]->ucExtraParam = 9;
	} else {
		++asCount;
	}
}
