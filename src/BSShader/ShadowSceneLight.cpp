#include "BSShader/ShadowSceneLight.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSBound.h"
#include "BSMain/BSCullingProcess.h"
#include "BSShader/BSCubeMapCamera.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/DebugShader.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceManager.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ShadowLightShader.h"
#include "BSShader/ShadowSceneNode.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAmbientLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedCubeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShaderConstantMap.h"

#include <cmath>

ShadowSceneLight::ShadowSceneLight()
	: fLuminance(0.0f), fLODDimmer(0.0f), fShadowLODDimmer(0.0f), fShadowLODDimmerTarget(1.0f),
	  fShadowLODDimmerElapsed(0.0f), bCastShadow(0), bCubeShadowMap(false), spSpecularAttenuationTexture(nullptr),
	  bPointLight(false), bAmbientLight(false), spLight(nullptr), bDynamicLight(false), bPointPosition(NiPoint3_ZERO),
	  spDynamicShadowMap(nullptr), usFrustumCull(0), spFrustumVis(nullptr), bShadowSpotlight(false),
	  fSpotlightFOV(90.0f), fSpotlightFalloff(1.0f), bShowDebugTexture(0), spShadowCaster(nullptr),
	  kGeomListFence(nullptr), kGeomListFenceObject(nullptr), spShadowCamera(nullptr), bClipSpacePlanesReady(false),
	  spShadowAccumulator(nullptr) {
	std::memset(&kViewProjection, 0, sizeof(kViewProjection));
	kViewProjection._11 = 1.0f;
	kViewProjection._22 = 1.0f;
	kViewProjection._33 = 1.0f;
	kViewProjection._44 = 1.0f;
	std::memset(pfClipSpacePlanes, 0, sizeof(pfClipSpacePlanes));
}

ShadowSceneLight::~ShadowSceneLight() {
	RemoveAllTriShapes();
	if (spSpecularAttenuationTexture) {
		spSpecularAttenuationTexture->DecRefCount();
		spSpecularAttenuationTexture = nullptr;
	}
	spLight = nullptr;
	if (spDynamicShadowMap.m_pObject) {
		BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spDynamicShadowMap.m_pObject);
	}
	spDynamicShadowMap = nullptr;
	usFrustumCull = 0;
	if (spFrustumVis) {
		spFrustumVis->DecRefCount();
		spFrustumVis = nullptr;
	}
	spShadowCamera = nullptr;
	spShadowCaster = nullptr;
	spShadowAccumulator = nullptr;
	kGeomListFence = nullptr;
	kGeomListFenceObject = nullptr;
}

float ShadowSceneLight::GetLuminanceAtPoint(NiPoint3 apPoint, NiLight* apIgnore) {
	NiLight* m_pObject = spLight.m_pObject;
	// Compares the byte with 1, as IsShadowLight does.
	if (m_pObject == apIgnore || bCastShadow == 1) {
		return 0.0;
	} else {
		bool v4 = !bPointLight;
		float apIgnorec = m_pObject->m_fDimmer;
		float y = m_pObject->m_kWorld.m_Translate.y;
		float kLightPos = m_pObject->m_kWorld.m_Translate.x;
		float kDimmedColor = apIgnorec * m_pObject->m_kDiff.r;
		float z = m_pObject->m_kWorld.m_Translate.z;
		float kDimmedColor_4 = apIgnorec * m_pObject->m_kDiff.g;
		float kDimmedColor_8 = apIgnorec * m_pObject->m_kDiff.b;
		float fRadius = m_pObject->m_kSpec.r;
		fLuminance = 1.0;
		if (!v4) {
			double v6;
			double v7;
			float kPosDiff = apPoint.x - kLightPos;
			float kPosDiff_4 = apPoint.y - y;
			float kPosDiff_8 = apPoint.z - z;
			float apIgnored = kPosDiff_4 * kPosDiff_4 + kPosDiff * kPosDiff + kPosDiff_8 * kPosDiff_8;
			float apIgnoree = std::sqrt((double)apIgnored);
			float apIgnoref = apIgnoree / fRadius;
			double v5 = apIgnoref;
			if (apIgnoref < 0.0) {
				v5 = 0.0;
			}
			float apIgnorea = v5;
			if (!(apIgnorea > 1.0)) {
				v7 = apIgnorea;
				v6 = 1.0;
			} else {
				v6 = 1.0;
				v7 = 1.0;
			}
			float apIgnoreg = v7;
			fLuminance = v6 - apIgnoreg * apIgnoreg;
		}
		double v8 = kDimmedColor_4;
		if (!(kDimmedColor_8 < static_cast<double>(kDimmedColor_4))) {
			v8 = kDimmedColor_8;
		}
		float apIgnoreb = v8;
		double v9 = kDimmedColor;
		if (!(apIgnoreb < static_cast<double>(kDimmedColor))) {
			v9 = apIgnoreb;
		}
		float apIgnoreh = v9;
		float apIgnorei = apIgnoreh * fLuminance;
		fLuminance = apIgnorei;
		return apIgnorei;
	}
}

void ShadowSceneLight::SetShadowCaster(NiNode* apNode) {
	NiNode* pOldCaster = spShadowCaster.m_pObject;
	if (pOldCaster == apNode)
		return;

	if (pOldCaster)
		NiDecRefCount(pOldCaster);
	spShadowCaster.m_pObject = apNode;
	if (apNode)
		NiIncRefCount(apNode);
}

// The object-type callback's result is unused: bit 0 of the shadow mask is tested whatever it returns. The shader type
// compares are signed. The passes are BSSM_AMBIENT (0x58), BSSM_AMBIENT_S (0x5C), BSSM_3XDEPTHMAP (0x1C8) and
// BSSM_3XDEPTHMAP_S (0x1CA); eBSSMPassEnum declares no enumerators yet, so they are cast.
bool ShadowSceneLight::Display(NiCullingProcess& arCuller) {
	if (spLight.m_pObject->m_uFlags & 1) {
		return true;
	}
	NiTListItem<NiPointer<NiGeometry>>* pkIter = lGeomList.m_pkHead;
	while (pkIter) {
		NiGeometry* pGeometry = pkIter->m_element.m_pObject;
		pkIter = pkIter->m_pkNext;
		if (!pGeometry) {
			continue;
		}
		BSShader* pShader = pGeometry->m_spShader;
		if (!pShader) {
			continue;
		}
		if (BSShaderManager::pObjectTypeCallback) {
			BSShaderManager::pObjectTypeCallback(pGeometry);
		}
		if (!(BSShaderManager::uiObjectTypeShadowMask & 1)) {
			continue;
		}
		bool bIsAccumulating = NiRenderer::ms_pkRenderer->m_spAccum.m_pObject->IsAccumulating();
		int32_t iShaderType = static_cast<int32_t>(pShader->iShaderType);
		if (iShaderType >= 1 && iShaderType <= 5) {
			if (!bIsAccumulating) {
				BSShaderManager::pCurrentRenderPass = nullptr;
				if (pGeometry->m_spSkinInstance.m_pObject) {
					BSShaderManager::eCurrentPass = static_cast<BSShaderManager::eBSSMPassEnum>(0x5C);
					ShadowLightShader::EnableConstantsForPass(0x5C, false);
				} else {
					BSShaderManager::eCurrentPass = static_cast<BSShaderManager::eBSSMPassEnum>(0x58);
					ShadowLightShader::EnableConstantsForPass(0x58, false);
				}
				pShader->m_spVertexConstantMap.m_pObject->SortByEnabled();
				pShader->m_spPixelConstantMap.m_pObject->SortByEnabled();
			}
		} else if (iShaderType == 17) {
			if (!bIsAccumulating) {
				BSShaderManager::pCurrentRenderPass = nullptr;
				if (pGeometry->m_spSkinInstance.m_pObject) {
					BSShaderManager::eCurrentPass = static_cast<BSShaderManager::eBSSMPassEnum>(0x1CA);
					Lighting30Shader::EnableConstantsForPass(0x1CA, false);
				} else {
					BSShaderManager::eCurrentPass = static_cast<BSShaderManager::eBSSMPassEnum>(0x1C8);
					Lighting30Shader::EnableConstantsForPass(0x1C8, false);
				}
				pShader->m_spVertexConstantMap.m_pObject->SortByEnabled();
				pShader->m_spPixelConstantMap.m_pObject->SortByEnabled();
			}
		} else {
			continue;
		}
		pGeometry->Cull(arCuller);
	}
	return true;
}

void ShadowSceneLight::SetLight(NiLight* apLight) {
	spLight = apLight;
	bPointLight = NiObject::IsKindOf(&NiPointLight::ms_RTTI, apLight);
	if (!bPointLight) {
		fLODDimmer = 1.0f;
	} else if (bDynamicLight) {
		bPointPosition = apLight->m_kWorld.m_Translate;
	}
	bAmbientLight = NiObject::IsKindOf(&NiAmbientLight::ms_RTTI, apLight);
}

void ShadowSceneLight::SetCastShadow(int32_t abCastShadow) {
	AutoMemContext kMemContext(MEM_CONTEXT_SHADOWS);
	// Tests and stores only the low byte, unconverted.
	uint8_t ucCastShadow = static_cast<uint8_t>(abCastShadow);
	// Bit 4 of the enabled passes is the shadow pass.
	if ((bCastShadow && !ucCastShadow) || BSShaderManager::eShaderLevel < 3 ||
		!(BSShaderManager::iEnabledPasses & 0x10)) {
		if (spDynamicShadowMap.m_pObject) {
			BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spDynamicShadowMap.m_pObject);
			spDynamicShadowMap = nullptr;
		}
	}
	bCastShadow = ucCastShadow;
}

void ShadowSceneLight::SetFadeTarget(float afTarget, bool abForce) {
	double v3;
	double v4;
	double v5;
	float abNoFadea;
	float abNoFade;
	float abNoFadeb;
	if (afTarget != fShadowLODDimmerTarget) {
		if (abForce) {
			fShadowLODDimmerElapsed = (float)1.0;
			fShadowLODDimmerTarget = afTarget;
		} else {
			abNoFadea = fShadowLODDimmerElapsed / BSShaderManager::fShadowFadeTime;
			v3 = (double)abNoFadea;
			if (abNoFadea < 0.0) {
				v3 = 0.0;
			}
			abNoFade = (float)v3;
			if (!(abNoFade > 1.0)) {
				v5 = (double)abNoFade;
				v4 = 1.0;
			} else {
				v4 = 1.0;
				v5 = 1.0;
			}
			abNoFadeb = (float)v5;
			fShadowLODDimmerElapsed = (float)(BSShaderManager::fShadowFadeTime * (v4 - abNoFadeb));
			fShadowLODDimmerTarget = afTarget;
		}
	}
}

bool ShadowSceneLight::BoundWithinLightRadius(const NiBound& apBound, NiLight* apLight, float afRadiusScale) {
	NiLight* pLight = apLight;
	if (!pLight) {
		pLight = spLight.m_pObject;
	}
	if (pLight->GetRTTI() == &NiDirectionalLight::ms_RTTI) {
		return true;
	}
	NiPoint3 kOffset = BSShaderManager::pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD]->kLightingOffset;
	float fDeltaX = pLight->m_kWorld.m_Translate.x - apBound.m_kCenter.x;
	float fDeltaY = pLight->m_kWorld.m_Translate.y - apBound.m_kCenter.y;
	float fDeltaZ = pLight->m_kWorld.m_Translate.z - apBound.m_kCenter.z;
	float fX = kOffset.x + fDeltaX;
	float fY = kOffset.y + fDeltaY;
	float fZ = kOffset.z + fDeltaZ;
	float fDistance = fY * fY + fX * fX + fZ * fZ;
	fDistance = std::sqrt(fDistance);
	float fGap = fDistance - apBound.m_fRadius * afRadiusScale;
	return pLight->m_kSpec.r > fGap;
}

// Attaches the light to apTriGeom's lighting property and adds the geometry to the light's list once (moving it in
// front of the fence when found past it). The list entry is kept in the function static sptriGeom.
void ShadowSceneLight::AddTriGeom(NiTriBasedGeom* apTriGeom) {
	static NiPointer<NiGeometry> sptriGeom;
	BSShader* m_spShader = apTriGeom->GetShader();
	if (m_spShader) {
		if (NiD3DShaderInterface::DynamicCast(m_spShader, &NiD3DDefaultShader::ms_RTTI)) {
			// The address &apTriGeom->m_PropertyState is never null.
			BSShaderLightingProperty* pshprop =
				static_cast<BSShaderLightingProperty*>(apTriGeom->m_PropertyState.m_aspProps[3].m_pObject);
			int32_t iType = pshprop ? pshprop->iShaderPropertyType : -1;
			// Tests 1 <= iType <= 12 as one unsigned compare of iType - 1 against 11.
			if (static_cast<uint32_t>(iType - 1) <= 11) {
				NiTListItem<NiPointer<NiGeometry>>* pFence = kGeomListFence;
				NiTListItem<NiPointer<NiGeometry>>* pPos = pFence;
				if (!pFence) {
					pPos = lGeomList.m_pkHead;
				}
				if (pPos) {
					NiTListItem<NiPointer<NiGeometry>>* pItem;
					while (1) {
						pItem = pPos;
						pPos = pPos->m_pkNext;
						if (pItem->m_element.m_pObject == apTriGeom) {
							break;
						}
						if (!pPos) {
							goto LABEL_15;
						}
					}
					if (pFence) {
						lGeomList.MoveToBefore(pItem, pFence);
					}
					BSNiBound* m_kWorldBound = apTriGeom->m_kWorldBound;
					if (!m_kWorldBound) {
						m_kWorldBound = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
					}
					pshprop->AttachLight(this, m_kWorldBound);
				} else {
				LABEL_15:;
					if (!pshprop->GetFlag(12u) || iType == 4) {
						sptriGeom.m_pObject = apTriGeom;
						BSNiBound* v10 = apTriGeom->m_kWorldBound;
						if (!v10) {
							v10 = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
						}
						pshprop->AttachLight(this, v10);
						lGeomList.AddHead(sptriGeom);
						sptriGeom.m_pObject = 0;
					}
				}
			}
		}
	}
}

void ShadowSceneLight::RemoveAllTriShapes() {
	NiTListItem<NiPointer<NiGeometry>>* pPos = lGeomList.m_pkHead;
	while (pPos) {
		NiGeometry* pGeometry = pPos->m_element.m_pObject;
		pPos = pPos->m_pkNext;
		if (pGeometry) {
			// The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare).
			BSShaderProperty* pProperty =
				static_cast<BSShaderProperty*>(pGeometry->m_PropertyState.m_aspProps[3].m_pObject);
			if (pProperty &&
				static_cast<uint32_t>(pProperty->iShaderPropertyType - 1) <=
					BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1) {
				static_cast<BSShaderLightingProperty*>(pProperty)->RemoveLight(this);
			}
		}
	}
	lGeomList.RemoveAll();
	lRootList.RemoveAll();
}

void ShadowSceneLight::SetDynamicPosition(NiPoint3 abDynamicPosition) {
	bPointPosition = abDynamicPosition;
}

// Puts the fence shape (made on first use, named "fence") at the head of the geometry list and marks it as the fence.
void ShadowSceneLight::InsertUnusedFence() {
	if (!kGeomListFenceObject.m_pObject) {
		NiTriShape* pShape;
		void* pMemory = NiMemObject::operator new(sizeof(NiTriShape));
		if (pMemory) {
			pShape = ::new (pMemory) NiTriShape(static_cast<NiTriShapeData*>(nullptr));
		} else {
			pShape = 0;
		}
		kGeomListFenceObject = pShape;
		// kName's destructor releases the string handle.
		NiFixedString kName;
		kName.SetHandle(NiGlobalStringTable::AddString("fence"));
		kGeomListFenceObject.m_pObject->SetName(kName);
	}
	lGeomList.AddHead(kGeomListFenceObject);
	kGeomListFence = lGeomList.m_pkHead;
}

void ShadowSceneLight::AddObject(NiAVObject* apAVObject) {
	if (!apAVObject) {
		return;
	}
	NiNode* pNode = apAVObject->IsNode();
	if (pNode && !(apAVObject->m_uFlags & NiAVObject::APP_CULLED) && pNode->m_kWorldBound &&
		0.0f != pNode->m_kWorldBound->m_fRadius) {
		if (!bCastShadow) {
			ParseObjectAddRadius(pNode);
			return;
		}
		if (apAVObject != spShadowCaster.m_pObject || (apAVObject->m_uFlags & NiAVObject::NO_SHADOWS)) {
			ParseObjectAddFrustum(pNode);
			return;
		}
	}
	if (apAVObject->IsTriBasedGeom()) {
		AddTriGeom(static_cast<NiTriBasedGeom*>(apAVObject));
	}
}

void ShadowSceneLight::RemoveTriGeom(NiTriBasedGeom* apTriGeom, void* akIterator) {
	// The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare).
	BSShaderProperty* pProperty = static_cast<BSShaderProperty*>(apTriGeom->m_PropertyState.m_aspProps[3].m_pObject);
	if (pProperty &&
		static_cast<uint32_t>(pProperty->iShaderPropertyType - 1) <=
			BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1) {
		static_cast<BSShaderLightingProperty*>(pProperty)->RemoveLight(this);
	}
	if (akIterator) {
		NiTListItem<NiPointer<NiGeometry>>* pItem = static_cast<NiTListItem<NiPointer<NiGeometry>>*>(akIterator);
		lGeomList.RemovePos(pItem);
	} else {
		NiPointer<NiGeometry> spGeometry(apTriGeom);
		lGeomList.Remove(spGeometry);
	}
}

void ShadowSceneLight::RemoveUnusedGeom() {
	NiTListItem<NiPointer<NiGeometry>>* pPos = kGeomListFence;
	while (pPos) {
		NiGeometry* pGeometry = pPos->m_element.m_pObject;
		NiTListItem<NiPointer<NiGeometry>>* pItem = pPos;
		pPos = pPos->m_pkNext;
		if (pGeometry) {
			RemoveTriGeom(static_cast<NiTriBasedGeom*>(pGeometry), pItem);
		}
	}
	kGeomListFence = nullptr;
}

float ShadowSceneLight::GetLuminanceToBound(NiBound* apBound) {
	NiPoint3 kOffset = BSShaderManager::pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD]->kLightingOffset;
	NiLight* pLight = spLight.m_pObject;
	float fX = kOffset.x + pLight->m_kWorld.m_Translate.x;
	float fY = pLight->m_kWorld.m_Translate.y + kOffset.y;
	float fZ = pLight->m_kWorld.m_Translate.z + kOffset.z;
	float fDeltaX = fX - apBound->m_kCenter.x;
	float fDeltaY = fY - apBound->m_kCenter.y;
	float fDeltaZ = fZ - apBound->m_kCenter.z;
	float fDY = fDeltaY;
	float fDX = fDeltaX;
	float fDZ = fDeltaZ;
	float fSquared = fDY * fDY + fDX * fDX + fDZ * fDZ;
	float fDistance = fSquared;
	fDistance = std::sqrt(fDistance);
	float fRoot = fDistance;
	float fResult = (fRoot - apBound->m_fRadius) / pLight->m_kSpec.r;
	float fRounded = fResult;
	fLuminance = fRounded;
	return fRounded;
}

void ShadowSceneLight::TestFrustumCull(NiCullingProcess& arCuller) {
	uint32_t v5;
	const NiPlane* pPlane;
	double v9;
	NiCamera* m_pkCamera;
	double v14;
	double v15;
	double v16;
	double v17;
	double fnewShadowLODdim;
	double v19;
	NiNode* v22;
	double v24;
	double fnewShadowLODdim_2;
	double v27;
	double v32;
	double v34;
	float v46;
	float v47;
	float v48;
	float isEnableda;
	float isEnabledb;
	float isEnabled;
	NiBound kbound;
	float v56;
	float v57;
	float v58;
	FNVD_INLINED_LOCAL(NiFrustumPlanes, frustumPlanes, std::nullopt, std::nullopt);
	NiLight* m_pObject = spLight.m_pObject;
	kbound.m_fRadius = m_pObject->m_kSpec.r;
	bool bCastShadow = this->bCastShadow;
	kbound.m_kCenter = m_pObject->m_kWorld.m_Translate;
	int32_t newcull = 0;
	bool bpointlightinfrustum = 1;
	if (bCastShadow && spShadowCamera.m_pObject) {
		newcull = FrustumFrustumIntersection(arCuller.m_pkCamera, spShadowCamera.m_pObject) ? 0 : 255;
	LABEL_19:;
		v9 = 0.0;
		goto LABEL_20;
	}
	frustumPlanes = arCuller.m_kPlanes;
	v5 = 0;
	pPlane = &frustumPlanes.m_akCullingPlanes[0];
	do {
		if ((1 << v5 & frustumPlanes.GetActivePlaneState()) != 0) {
			double v7 = pPlane->m_kNormal.y * kbound.m_kCenter.y + kbound.m_kCenter.x * pPlane->m_kNormal.x +
				pPlane->m_kNormal.z * kbound.m_kCenter.z;
			float v42 = v7;
			double m_fConstant = pPlane->m_fConstant;
			float v43 = v42 - m_fConstant;
			if (-kbound.m_fRadius >= v43) {
				newcull = static_cast<int32_t>((static_cast<uint32_t>(newcull) & 0xFFFF0000u) |
					(static_cast<uint32_t>(static_cast<uint16_t>(255))));
				v9 = 0.0;
				goto LABEL_20;
			}
			float v44 = v7 - m_fConstant;
			if (v44 < 0.0f) {
				bpointlightinfrustum = 0;
			}
		}
		++v5;
		++pPlane;
	} while (v5 < 6);
	v9 = 0.0;
	if (!bpointlightinfrustum &&
		(bCastShadow &&
			(BSShaderManager::eShaderLevel >= 3 && ((BSShaderManager::iEnabledPasses & 16) != 0 && bCubeShadowMap)))) {
		uint16_t v10 = 0;
		char v11 = 0;
		do {
			if (!CubeFaceFrustumFrustumIntersection(kbound, arCuller, v10)) {
				newcull |= 1 << v11;
			}
			++v10;
			++v11;
		} while (v10 < 6u);
		goto LABEL_19;
	}
LABEL_20:;
	uint16_t newcull_2 = newcull;
	float fnewLODdim = fLODDimmer;
	float fnewShadowLODdim_3 = fShadowLODDimmer;
	if (static_cast<uint16_t>(newcull) == 255) {
		goto LABEL_51;
	}
	m_pkCamera = arCuller.m_pkCamera;
	v56 = kbound.m_kCenter.x - m_pkCamera->m_kWorld.m_Translate.x;
	v57 = kbound.m_kCenter.y - m_pkCamera->m_kWorld.m_Translate.y;
	v58 = kbound.m_kCenter.z - m_pkCamera->m_kWorld.m_Translate.z;
	v46 = v57 * v57 + v56 * v56 + v58 * v58;
	v47 = std::sqrt((double)v46);
	v48 = v47 - kbound.m_fRadius;
	if (BSShaderManager::GetAccumulator()->m_pkCamera) {
		v48 = BSShaderManager::GetAccumulator()->m_pkCamera->m_fLODAdjust * v48;
	}
	v14 = v48;
	if (BSShaderManager::fLightLODStartFade > static_cast<double>(v48)) {
		double v20;
		v27 = 0.0;
	LABEL_40:;
		fnewLODdim = 1.0f;
		fnewShadowLODdim = v27;
		v19 = 1.0f;
	LABEL_41:;
		if (this->bCastShadow) {
			double v28;
			if (BSShaderManager::fShadowLODStartFade > v14 ||
				(v28 = BSShaderManager::fShadowLODEnd, BSShaderManager::fShadowLODEnd == fnewShadowLODdim)) {
				v20 = fnewShadowLODdim;
				fnewShadowLODdim_3 = 1.0f;
			} else {
				if (v14 > v28) {
					double v30 = v19;
					v16 = fnewShadowLODdim;
					v17 = v30;
					fnewShadowLODdim_3 = fnewShadowLODdim;
					goto LABEL_31;
				}
				v20 = fnewShadowLODdim;
				fnewShadowLODdim_3 =
					v19 - (v14 - BSShaderManager::fShadowLODStartFade) / (v28 - BSShaderManager::fShadowLODStartFade);
			}
			goto LABEL_30;
		}
	LABEL_29:;
		v20 = fnewShadowLODdim;
	LABEL_30:;
		double v21 = v19;
		v16 = v20;
		v17 = v21;
		goto LABEL_31;
	}
	v15 = BSShaderManager::fLightLODEnd;
	if (BSShaderManager::fLightLODEnd == 0.0f) {
		v27 = 0.0;
		goto LABEL_40;
	}
	if (!(v15 < v14)) {
		fnewLODdim = 1.0f - (v14 - BSShaderManager::fLightLODStartFade) / (v15 - BSShaderManager::fLightLODStartFade);
		fnewShadowLODdim = 0.0;
		v19 = 1.0f;
		if (!(fnewLODdim > 0.0f)) {
			goto LABEL_29;
		}
		goto LABEL_41;
	}
	newcull_2 = 255;
	v16 = 0.0;
	v17 = 1.0f;
	fnewLODdim = 0.0f;
	fnewShadowLODdim_3 = 0.0f;
LABEL_31:;
	v22 = spShadowCaster.m_pObject;
	if (v22) {
		NiMaterialProperty* pmat = static_cast<NiMaterialProperty*>(v22->GetProperty(NiProperty::MATERIAL));
		if (pmat) {
			fnewShadowLODdim_3 = pmat->m_fAlpha * fnewShadowLODdim_3;
		}
		v17 = 1.0;
		v16 = 0.0;
	}
	isEnableda = fShadowLODDimmerElapsed + BSShaderManager::fTimerValues[1];
	fShadowLODDimmerElapsed = isEnableda;
	isEnabledb = isEnableda / BSShaderManager::fShadowFadeTime;
	v24 = isEnabledb;
	if (isEnabledb > v17) {
		v24 = 1.0;
	}
	isEnabled = v24;
	if (v16 == fShadowLODDimmerTarget) {
		fnewShadowLODdim_2 = (v17 - isEnabled) * fnewShadowLODdim_3;
		v9 = v16;
	} else {
		v9 = v16;
		fnewShadowLODdim_2 = isEnabled * fnewShadowLODdim_3;
	}
	fnewShadowLODdim_3 = fnewShadowLODdim_2;
LABEL_51:;
	double v31 = fnewShadowLODdim_3;
	if (usFrustumCull == 255) {
		if (newcull_2 != 255) {
			goto LABEL_67;
		}
	} else {
		if (newcull_2 == 255) {
			goto LABEL_67;
		}
	}
	if (v9 == fLODDimmer) {
		v32 = v9;
		if (v9 < fnewLODdim) {
		LABEL_66:;
			v9 = v32;
		LABEL_67:;
			NiTListItem<NiPointer<NiGeometry>>* m_pkHead = lGeomList.m_pkHead;
			while (m_pkHead) {
				NiGeometry* v37 = m_pkHead->m_element.m_pObject;
				m_pkHead = m_pkHead->m_pkNext;
				if (v37) {
					int32_t v39;
					BSShaderProperty* v38 =
						static_cast<BSShaderProperty*>(v37->m_PropertyState.m_aspProps[3].m_pObject);
					// The shade property is a lighting property when its shader type is 1 to 12 (one unsigned
					// compare).
					if (v38) {
						v39 = static_cast<uint32_t>(v38->iShaderPropertyType - 1) <=
							BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1;
					} else {
						v39 = 0;
					}
					BSShaderLightingProperty* v40 = v39 ? static_cast<BSShaderLightingProperty*>(v38) : nullptr;
					if (v40) {
						v40->bLightListChanged = 1;
					}
				}
			}
			if (v9 == fShadowLODDimmer) {
				if (spDynamicShadowMap.m_pObject) {
					BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spDynamicShadowMap.m_pObject);
					spDynamicShadowMap = nullptr;
				}
			}
			goto LABEL_81;
		}
		v31 = fnewShadowLODdim_3;
	}
	v32 = v9;
	v34 = v31;
	if ((v32 < fLODDimmer && v32 == fnewLODdim) || (v32 == fShadowLODDimmer && v32 < v34)) {
		goto LABEL_66;
	}
	if (v32 < fShadowLODDimmer) {
		bool v35 = v32 == v34;
		v9 = v32;
		if (v35) {
			goto LABEL_67;
		}
	}
LABEL_81:;
	usFrustumCull = newcull_2;
	fLODDimmer = fnewLODdim;
	fShadowLODDimmer = fnewShadowLODdim_3;
}

void ShadowSceneLight::SetDynamicShadowMap(BSRenderedTexture* apDynamicShadowMap) {
	BSRenderedTexture* m_pObject;
	m_pObject = spDynamicShadowMap.m_pObject;
	if (m_pObject != apDynamicShadowMap) {
		if (m_pObject) {
			if (!InterlockedDecrementUInt32(&m_pObject->m_uiRefCount)) {
				m_pObject->DeleteThis();
			}
		}
		spDynamicShadowMap.m_pObject = apDynamicShadowMap;
		if (apDynamicShadowMap) {
			InterlockedIncrementUInt32(&apDynamicShadowMap->m_uiRefCount);
		}
	}
}

uint16_t ShadowSceneLight::GetFrustumCull() {
	return usFrustumCull;
}

bool ShadowSceneLight::IsFadedOut() {
	return (bool)((uint8_t)((BSShaderManager::fShadowFadeTime <= ((double)fShadowLODDimmerElapsed)) &&
		(fShadowLODDimmerTarget <= 0.10000000149011612)));
}

void ShadowSceneLight::ShowDebugTexture(bool abShow) {
	bShowDebugTexture = (uint8_t)abShow;
}

NiLight* ShadowSceneLight::GetLight() {
	return (&spLight)->operator->();
}

NiAVObject* ShadowSceneLight::GetShadowScene() {
	return (&spShadowCaster)->operator->();
}

void ShadowSceneLight::ParseObjectAddFrustum(NiAVObject* apObject) {
	NiNode* v5;
	uint32_t v7;
	uint32_t m_uiActivePlanes = kShadowCameraPlanes.GetActivePlaneState();
	if (apObject == spShadowCaster.m_pObject) {
		goto LABEL_20;
	}
	if ((apObject->m_uFlags & NiAVObject::NO_SHADOWS) != 0) {
		goto LABEL_20;
	}
	if (m_uiActivePlanes) {
		if ((apObject->m_uFlags & NiAVObject::ALWAYS_DRAW) == 0) {
			NiBound* m_kWorldBound = apObject->m_kWorldBound;
			if (!m_kWorldBound) {
				m_kWorldBound = &NiAVObject::NullBoundS;
			}
			if (!BoundWithinLightFrustum(*m_kWorldBound)) {
				goto LABEL_20;
			}
		}
	}
	v5 = apObject->IsNode();
	if (!v5) {
		if (apObject->IsTriBasedGeom()) {
			AddTriGeom(static_cast<NiTriBasedGeom*>(apObject));
		}
		goto LABEL_20;
	}
	v7 = 0;
	if (!v5->m_kChildren.m_usSize) {
	LABEL_20:;
		kShadowCameraPlanes.m_uiActivePlanes = m_uiActivePlanes;
		return;
	}
	do {
		NiAVObject* m_pObject = v5->m_kChildren.m_pBase[v7].m_pObject;
		if (m_pObject) {
			NiBound* v10;
			uint32_t m_uFlags = m_pObject->m_uFlags;
			if (((m_uFlags & NiAVObject::ALWAYS_DRAW) != 0 ||
					((v10 = m_pObject->m_kWorldBound) != 0 && 0.0 != v10->m_fRadius)) &&
				(m_uFlags & 1) == 0) {
				ParseObjectAddFrustum(m_pObject);
			}
		}
		++v7;
	} while (v7 < v5->m_kChildren.m_usSize);
	kShadowCameraPlanes.m_uiActivePlanes = m_uiActivePlanes;
}

// Adds the geometry under apObject that lies within the light's radius; the camera planes' active state is restored
// on return.
void ShadowSceneLight::ParseObjectAddRadius(NiAVObject* apObject) {
	uint32_t uiActivePlanes = kShadowCameraPlanes.m_uiActivePlanes;
	if (!(apObject->m_uFlags & NiAVObject::ALWAYS_DRAW)) {
		NiBound* pBound = apObject->m_kWorldBound;
		if (!pBound) {
			pBound = &NiAVObject::NullBoundS;
		}
		if (!BoundWithinLightRadius(*pBound, spLight.m_pObject, 1.0f)) {
			kShadowCameraPlanes.m_uiActivePlanes = uiActivePlanes;
			return;
		}
	}
	NiNode* pNode = apObject->IsNode();
	if (pNode) {
		for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
			NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
			if (!pChild) {
				continue;
			}
			uint32_t uiFlags = pChild->m_uFlags;
			if (!(uiFlags & NiAVObject::ALWAYS_DRAW)) {
				if (!pChild->m_kWorldBound || 0.0f == pChild->m_kWorldBound->m_fRadius) {
					continue;
				}
			}
			if (!(uiFlags & NiAVObject::APP_CULLED)) {
				ParseObjectAddRadius(pChild);
			}
		}
	} else if (apObject->IsTriBasedGeom()) {
		AddTriGeom(apObject->IsTriBasedGeom());
	}
	kShadowCameraPlanes.m_uiActivePlanes = uiActivePlanes;
}

float ShadowSceneLight::GetViewLuminanceAtPoint(NiPoint3 apPoint, NiPoint3 apView, NiLight* apIgnore) {
	NiLight* m_pObject = spLight.m_pObject;
	// Compares the byte with 1, as IsShadowLight does.
	if (m_pObject == apIgnore || (bCastShadow == 1 || (m_pObject->m_uFlags & 1) != 0)) {
		return 0.0;
	} else {
		NiPoint3 v20;
		NiPoint3 v21;
		bool v6 = !bPointLight;
		float a8c = m_pObject->m_fDimmer;
		float x = m_pObject->m_kWorld.m_Translate.x;
		v21.y = m_pObject->m_kWorld.m_Translate.y;
		float r = m_pObject->m_kDiff.r;
		float b = m_pObject->m_kDiff.b;
		v21.x = x;
		float v17 = a8c * r;
		v21.z = m_pObject->m_kWorld.m_Translate.z;
		float v18 = a8c * m_pObject->m_kDiff.g;
		float v19 = a8c * b;
		float a8a = m_pObject->m_kSpec.r;
		fLuminance = 1.0;
		v20.x = apPoint.x - v21.x;
		v20.y = apPoint.y - v21.y;
		v20.z = apPoint.z - v21.z;
		if (!v6) {
			float a8d = v20.Length() / a8a;
			float a2 = BSMAX(static_cast<float>(0.0), a8d);
			float a8e = BSMIN(static_cast<float>(1.0), a2);
			fLuminance = 1.0 - a8e * a8e;
		}
		v21.x = apView.x - apPoint.x;
		v21.y = apView.y - apPoint.y;
		v21.z = apView.z - apPoint.z;
		v21.Unitize();
		v20.Unitize();
		float apPointb = -v21.x;
		float apPoint_4 = -v21.y;
		float apPoint_8 = -v21.z;
		float a8f = apPoint_4 * v20.y + apPointb * v20.x + apPoint_8 * v20.z;
		double v9 = a8f;
		if (a8f > 1.0) {
			v9 = 1.0;
		}
		float a8g = v9;
		double v10 = a8g;
		if (a8g < 0.0) {
			v10 = 0.0;
		}
		float apPointa = v10;
		double v11 = v18;
		if (!(v19 < static_cast<double>(v18))) {
			v11 = v19;
		}
		float a8b = v11;
		double v12 = v17;
		if (!(a8b < static_cast<double>(v17))) {
			v12 = a8b;
		}
		float a8h = v12;
		float a8i = fLuminance * apPointa * a8h;
		fLuminance = a8i;
		return a8i;
	}
}

// The value of aTargetCenter is never read.
void ShadowSceneLight::ComputeWeightedShadowLight(BSShaderLightingProperty* apLightingProperty,
	NiPoint3 aTargetCenter) {
	NiPoint3* v8;
	ShadowSceneLight* plight;
	float v66;
	float v67;
	float v68;
	NiPoint3 kExtents;
	NiPoint3 kvector;
	NiPoint3 blendedVect;
	void* kiter;
	float v82;
	float v83;
	float v84;
	NiPoint3 kdirlightvect;
	NiUpdateData UpdateData;
	NiNode* m_pObject = spShadowCaster.m_pObject;
	BSBound* pbound = static_cast<BSBound*>(m_pObject->GetExtraData(*BSBound::pTag));
	NiNode* v6 = spShadowCaster.m_pObject;
	aTargetCenter = v6->m_kWorld.m_Translate;
	if (pbound) {
		float m_fScale = v6->m_kWorld.m_fScale;
		kvector.x = pbound->Center.x * m_fScale;
		kvector.y = pbound->Center.y * m_fScale;
		kvector.z = m_fScale * pbound->Center.z;
		aTargetCenter.x = kvector.x + aTargetCenter.x;
		aTargetCenter.y = aTargetCenter.y + kvector.y;
		aTargetCenter.z = aTargetCenter.z + kvector.z;
		float v52 = v6->m_kWorld.m_fScale;
		kExtents.x = pbound->Extents.x * v52;
		kExtents.y = pbound->Extents.y * v52;
		kExtents.z = v52 * pbound->Extents.z;
		v8 = &kExtents;
	} else {
		v8 = &NiPoint3_ZERO;
	}
	float x = v8->x;
	float y = v8->y;
	float z = v8->z;
	kvector.x = x;
	double v12 = x;
	kvector.y = y;
	kvector.z = z;
	if (!(y < static_cast<double>(x))) {
		v12 = y;
	}
	float v61 = v12;
	double v13 = v61;
	if (!(kvector.z < static_cast<double>(v61))) {
		v13 = kvector.z;
	}
	float v72 = v13;
	float v77 = 0.0f;
	uint32_t v14 = 0;
	uint16_t v15 = 0;
	uint32_t v76 = 0;
	if (apLightingProperty) {
		plight = apLightingProperty->GetFirstActiveNonShadowLight(kiter);
	} else {
		plight = BSShaderManager::pShadowSceneNode[0]->GetLightAt(0);
	}
	if (plight) {
		do {
			if (v14 >= 40) {
				break;
			}
			if (!plight->bCastShadow) {
				NiLight* v17 = plight->spLight.m_pObject;
				if ((v17->m_uFlags & 1) == 0 && v17->m_fDimmer > 0.0f) {
					kvector.x = v17->m_kWorld.m_Translate.x - aTargetCenter.x;
					kvector.y = v17->m_kWorld.m_Translate.y - aTargetCenter.y;
					kvector.z = v17->m_kWorld.m_Translate.z - aTargetCenter.z;
					float v53 = kvector.x * kvector.x + kvector.y * kvector.y + kvector.z * kvector.z;
					float v54 = std::sqrt((double)v53);
					float v55 = 1.0 - (v54 - v72) / v17->m_kSpec.r;
					double v18 = v55;
					if (v55 < 0.0) {
						v18 = 0.0;
					}
					float v62 = v18;
					double v19 = v62;
					if (v62 > 1.0) {
						v19 = 1.0;
					}
					float v63 = v19;
					++v14;
					fLightDistances[v14 - 1] = v63;
					fLightWeights[v14 - 1] = v63;
					v77 = v63 + v77;
				}
			}
			plight = apLightingProperty ? apLightingProperty->GetNextActiveNonShadowLight(kiter)
										: BSShaderManager::pShadowSceneNode[0]->GetLightAt(++v15);
		} while (plight);
		v76 = v14;
	}
	ShadowSceneNode* v20 = BSShaderManager::pShadowSceneNode[0];
	float v64 = 0.0f;
	blendedVect.x = NiPoint3_ZERO.x;
	float r = NiColor_BLACK.r;
	// The sun's light is a directional light.
	NiDirectionalLight* v21 =
		static_cast<NiDirectionalLight*>(BSShaderManager::pShadowSceneNode[0]->pSunLight->spLight.m_pObject);
	double v22 = v21->m_kWorldDir.x;
	blendedVect.y = NiPoint3_ZERO.y;
	kdirlightvect.x = -v22;
	blendedVect.z = NiPoint3_ZERO.z;
	kdirlightvect.y = -v21->m_kWorldDir.y;
	float g = NiColor_BLACK.g;
	double v23 = v21->m_kWorldDir.z;
	float b = NiColor_BLACK.b;
	kdirlightvect.z = -v23 * (v72 * 0.0625);
	kdirlightvect.Unitize();
	if (0.0f == v77) {
		blendedVect.z = kdirlightvect.z;
		float v24 = v21->m_kDiff.b;
		blendedVect.x = kdirlightvect.x;
		float v25 = v21->m_kDiff.r;
		blendedVect.y = kdirlightvect.y;
		float v26 = v21->m_kDiff.g;
		b = v24;
		r = v25;
		g = v26;
		blendedVect.Unitize();
		float v56 = v72 / 75.0;
		if (blendedVect.z < static_cast<double>(v56)) {
			blendedVect.z = v72 / 75.0;
		}
	} else {
		ShadowSceneLight* plight_2;
		ShadowSceneLight* plight_3;
		BSShaderLightingProperty* v27 = apLightingProperty;
		int32_t v28 = 0;
		if (apLightingProperty) {
			plight_2 = apLightingProperty->GetFirstActiveNonShadowLight(kiter);
		} else {
			plight_2 = v20->GetLightAt(0);
		}
		uint32_t v30 = 0;
		for (ShadowSceneLight* i = plight_2; v30 < v76; i = plight_3) {
			if (i) {
				if (!i->bCastShadow) {
					NiLight* v32 = i->spLight.m_pObject;
					if ((v32->m_uFlags & 1) == 0) {
						float v57 = fLightWeights[v30] / v77;
						if (v57 > 0.0f) {
							NiPoint3 kvector_2;
							double dInvWeight = 1.0 - fLightWeights[v30];
							v64 = v57 * (v32->m_kSpec.r * fLightWeights[v30] + dInvWeight * 1000.0) + v64;
							kvector_2.x = v32->m_kWorld.m_Translate.x - aTargetCenter.x;
							kvector_2.y = v32->m_kWorld.m_Translate.y - aTargetCenter.y;
							kvector_2.z = v32->m_kWorld.m_Translate.z - aTargetCenter.z;
							kvector_2.Unitize();
							float fInvWeight = dInvWeight;
							double v33 = fInvWeight;
							v82 = fInvWeight * kdirlightvect.x;
							v83 = kdirlightvect.y * fInvWeight;
							v84 = fInvWeight * kdirlightvect.z;
							float fWeight = fLightWeights[v30];
							float v99 = kvector_2.x * fWeight;
							float v100 = kvector_2.y * fWeight;
							float v101 = fWeight * kvector_2.z;
							kvector.x = v99 + v82;
							kvector.y = v100 + v83;
							kvector.z = v101 + v84;
							kvector_2 = kvector;
							v66 = kvector.x * v57;
							v67 = kvector.y * v57;
							v68 = kvector.z * v57;
							blendedVect.x = v66 + blendedVect.x;
							float v34 = v21->m_kDiff.r;
							float v35 = v21->m_kDiff.g;
							float v36 = v21->m_kDiff.b;
							blendedVect.y = blendedVect.y + v67;
							float v85 = v34;
							float v37 = v32->m_kDiff.r;
							float v86 = v35;
							float v38 = v32->m_kDiff.g;
							float v87 = v36;
							blendedVect.z = v68 + blendedVect.z;
							float v39 = v32->m_kDiff.b;
							float v78 = v37;
							float fInvWeight2 = v33;
							float v79 = v38;
							float v80 = v39;
							v85 = v85 * fInvWeight2;
							v86 = v86 * fInvWeight2;
							v87 = fInvWeight2 * v87;
							float fWeight2 = fLightWeights[v30];
							v78 = v37 * fWeight2;
							v79 = v38 * fWeight2;
							v80 = fWeight2 * v39;
							float v88 = v78 + v85;
							float v89 = v79 + v86;
							float v90 = v80 + v87;
							float v93 = v88 * v57;
							float v94 = v89 * v57;
							float v95 = v57 * v90;
							r = v93 + r;
							g = v94 + g;
							b = v95 + b;
						}
					}
					v27 = apLightingProperty;
				}
			}
			if (v27) {
				plight_3 = v27->GetNextActiveNonShadowLight(kiter);
			} else {
				plight_3 = BSShaderManager::pShadowSceneNode[0]->GetLightAt(++v28);
			}
			++v30;
		}
		blendedVect.Unitize();
		if (blendedVect.z < 0.75) {
			blendedVect.z = 0.75f;
		}
	}
	blendedVect.Unitize();
	double v41 = v72;
	if (v72 > 256.0) {
		v41 = 256.0f;
	}
	float v58 = v41;
	NiPoint3* pLightPos = &spLight.m_pObject->m_kLocal.m_Translate;
	v66 = blendedVect.x * v58;
	v67 = blendedVect.y * v58;
	v68 = v58 * blendedVect.z;
	v82 = v66 * 1.399999976158142;
	v83 = v67 * 1.399999976158142;
	v84 = 1.399999976158142 * v68;
	v66 = v82 + aTargetCenter.x;
	v67 = v83 + aTargetCenter.y;
	float v44 = v67;
	double v45 = v84 + aTargetCenter.z;
	pLightPos->x = v66;
	pLightPos->y = v44;
	v68 = v45;
	pLightPos->z = v68;
	NiAVObject* v46 = spLight.m_pObject;
	UpdateData.bUpdateControllers = 0;
	float v65 = v58 * 8.0;
	UpdateData.bParallelUpdate = 0;
	UpdateData.bFoundParticles = 0;
	UpdateData.fTime = 0.0f;
	UpdateData.bFoundMorphController = 0;
	UpdateData.bSceneGraphChange = 0;
	v46->Update(UpdateData);
	float v92 = g * 0.4000000059604645 + r * 0.4000000059604645 + 0.4000000059604645 * b;
	double v47 = v92;
	if (v92 < 0.800000011920929) {
		v47 = 0.8f;
	}
	float v59 = v47;
	double v48 = v59;
	if (v59 > 1.0) {
		v48 = 1.0;
	}
	// The diffuse and specular setters: each stores its color and bumps the revision.
	NiLight* v49 = spLight.m_pObject;
	float v60 = v48;
	v49->m_kDiff.r = v60;
	v49->m_kDiff.g = v60;
	v66 = v65;
	v49->m_kDiff.b = v60;
	v67 = 0.0f;
	v68 = 0.0f;
	++v49->m_uiRevID;
	NiLight* v50 = spLight.m_pObject;
	++v50->m_uiRevID;
	v50->m_kSpec.r = v65;
	v50->m_kSpec.g = v67;
	v50->m_kSpec.b = v68;
}

void ShadowSceneLight::AccumulateGeometry(BSCullingProcess* a1, int32_t a2) {
	alignas(16) NiPoint3 v55;
	alignas(16) NiPoint3 v62;
	NiPoint3 arData;
	alignas(16) NiTransform kLightPos;
	bool bRender = 1;
	if ((!bCastShadow || (BSShaderManager::eShaderLevel < 3 || (BSShaderManager::iEnabledPasses & 16) == 0)) &&
		!bShadowSpotlight) {
		bRender = 0;
	}
	if (!spDynamicShadowMap.m_pObject) {
		BSRenderedTexture* v4 = BSShaderManager::pTextureManager->BorrowFrustumShadowTexture();
		if (!v4) {
			spLight.m_pObject->m_uFlags |= 1u;
		}
		SetDynamicShadowMap(v4);
	}
	if (bRender && (usFrustumCull != 255 && (fShadowLODDimmer > 0.05 && spDynamicShadowMap.m_pObject))) {
		NiPoint3* v15;
		NiPoint3 kWorldUp;
		alignas(16) NiRect<float> kViewport;
		NiNode* m_pObject = spShadowCaster.m_pObject;
		double v6 = m_pObject->m_kWorld.m_Translate.x;
		kLightPos = spLight.m_pObject->m_kWorld;
		float x = v6 - kLightPos.m_Translate.x;
		float y = m_pObject->m_kWorld.m_Translate.y - kLightPos.m_Translate.y;
		float za = m_pObject->m_kWorld.m_Translate.z - kLightPos.m_Translate.z;
		v55.x = x;
		v55.y = y;
		v55.z = za;
		v55.Unitize();
		float zb = -NiPoint3_UNIT_Z.x;
		y = -NiPoint3_UNIT_Z.y;
		x = -NiPoint3_UNIT_Z.z;
		kWorldUp.x = zb;
		kWorldUp.y = y;
		kWorldUp.z = x;
		NiPoint3 kWorldPt = v55.UnitCross(kWorldUp);
		if (kWorldPt.x < 0.01 && kWorldPt.x > -0.01 && kWorldPt.y < 0.01 && kWorldPt.y > -0.01 && kWorldPt.z < 0.01 &&
			kWorldPt.z > -0.01) {
			kWorldUp = NiPoint3_UNIT_X;
			NiPoint3* v7 = ::new (&v62) NiPoint3(v55.UnitCross(kWorldUp));
			float v8 = v7->y;
			kWorldPt.x = v7->x;
			float v9 = v7->z;
			kWorldPt.y = v8;
			kWorldPt.z = v9;
		}
		kWorldUp = *::new (&v62) NiPoint3(v55.UnitCross(kWorldPt));
		NiFrustum kFrustum(0);
		NiNode* v10 = spShadowCaster.m_pObject;
		kFrustum.m_bOrtho = 1;
		NiExtraData* ExtraData = v10->GetExtraData(*BSBound::pTag);
		NiNode* v12 = spShadowCaster.m_pObject;
		float v13 = v12->m_kWorld.m_Translate.y;
		v55.x = v12->m_kWorld.m_Translate.x;
		float v14 = v12->m_kWorld.m_Translate.z;
		v55.y = v13;
		v55.z = v14;
		if (ExtraData) {
			// The extra data is the caster's BSBound: its half extents, scaled.
			v15 = ::new (&v62) NiPoint3(static_cast<BSBound*>(ExtraData)->Extents * v12->m_kWorld.m_fScale);
		} else {
			v15 = &NiPoint3_ZERO;
		}
		float v16 = v15->x;
		float v17 = v15->y;
		NiBound* m_kWorldBound = (NiBound*)v12->m_kWorldBound;
		float v19 = v15->z;
		arData.x = v16;
		arData.y = v17;
		arData.z = v19;
		if (!m_kWorldBound) {
			m_kWorldBound = &NiAVObject::NullBoundS;
		}
		float zc = arData.z * 1.5;
		float afRight = BSMAX(zc, m_kWorldBound->m_fRadius);
		float zd = arData.y * 1.5;
		float afRighta = BSMAX(zd, afRight);
		float ze = arData.x * 1.5;
		float afRightb = BSMAX(ze, afRighta);
		double v20 = BSMIN(256.0f, afRightb);
		NiLight* v21 = spLight.m_pObject;
		float v52 = v20;
		float zf = v21->m_kWorld.m_Translate.x - v55.x;
		y = v21->m_kWorld.m_Translate.y - v55.y;
		x = v21->m_kWorld.m_Translate.z - v55.z;
		v62.x = zf;
		v62.y = y;
		v62.z = x;
		double v22 = v62.Length();
		bool v23 = !bCastShadow;
		float zg = v22;
		double v24 = v52;
		fSpotlightFOV = v52 / zg * 150.0;
		float v53 = -v52;
		kFrustum.m_fLeft = v53;
		kFrustum.m_fBottom = v53;
		kFrustum.m_fRight = v24;
		kFrustum.m_fTop = v24;
		kFrustum.m_fNear = 1.0f;
		kFrustum.m_fFar = v21->m_kSpec.r;
		kViewport.m_left = 0.0f;
		kViewport.m_bottom = 0.0f;
		kViewport.m_right = 1.0f;
		kViewport.m_top = 1.0f;
		if (!v23 && BSShaderManager::GetShadows()) {
			NiPoint3 result;
			NiUpdateData kUpdateData;
			spShadowAccumulator.operator=(BSShaderManager::GetNextAvailableShadowMapAccumulator());
			spShadowAccumulator.m_pObject->eRenderMode = BSShaderManager::BSSM_RENDER_DEPTHMAP;
			NiPointer<NiCamera>* p_spShadowCamera = &spShadowCamera;
			if (!spShadowCamera.m_pObject) {
				NiCamera* v27;
				NiCamera* v26 = static_cast<NiCamera*>(NiMemObject::operator new(sizeof(NiCamera)));
				if (v26) {
					v27 = ::new (v26) NiCamera;
				} else {
					v27 = 0;
				}
				p_spShadowCamera->operator=(v27);
			}
			NiCamera* v28 = p_spShadowCamera->m_pObject;
			float v29 = kLightPos.m_Translate.y;
			v28->m_kLocal.m_Translate.x = kLightPos.m_Translate.x;
			float v30 = kLightPos.m_Translate.z;
			v28->m_kLocal.m_Translate.y = v29;
			v28->m_kLocal.m_Translate.z = v30;
			NiAVObject* v31 = (NiAVObject*)p_spShadowCamera->m_pObject;
			kUpdateData.fTime = 0.0f;
			kUpdateData.bUpdateControllers = false;
			kUpdateData.bParallelUpdate = false;
			kUpdateData.bFoundParticles = false;
			kUpdateData.bFoundMorphController = false;
			kUpdateData.bSceneGraphChange = false;
			v31->Update(kUpdateData);
			NiNode* v32 = spShadowCaster.m_pObject;
			kWorldPt = v32->m_kWorld.m_Translate;
			BSBound* v33 = static_cast<BSBound*>(v32->GetExtraData(*BSBound::pTag));
			kWorldPt.operator+=(*::new (&result) NiPoint3(v33->Center * spShadowCaster.m_pObject->m_kWorld.m_fScale));
			::new (&v62) NiPoint3(v33->Extents * spShadowCaster.m_pObject->m_kWorld.m_fScale);
			if (v62.y + v62.x < v62.z) {
				v62.x = NiPoint3_UNIT_Z.x * 10.0;
				v62.y = NiPoint3_UNIT_Z.y * 10.0;
				v62.z = 10.0 * NiPoint3_UNIT_Z.z;
				kWorldPt.x = v62.x + kWorldPt.x;
				kWorldPt.y = kWorldPt.y + v62.y;
				kWorldPt.z = kWorldPt.z + v62.z;
			}
			p_spShadowCamera->m_pObject->LookAtWorldPoint(kWorldPt, kWorldUp);
			p_spShadowCamera->m_pObject->SetViewFrustum(kFrustum);
			p_spShadowCamera->m_pObject->SetViewPort(kViewport);
			NiAVObject* v34 = (NiAVObject*)p_spShadowCamera->m_pObject;
			kUpdateData.fTime = 0.0f;
			kUpdateData.bUpdateControllers = false;
			kUpdateData.bParallelUpdate = false;
			kUpdateData.bFoundParticles = false;
			kUpdateData.bFoundMorphController = false;
			kUpdateData.bSceneGraphChange = false;
			v34->Update(kUpdateData);
			float m_fNear = kFrustum.m_fNear;
			BSNiBound* v35 = spShadowCaster.m_pObject->m_kWorldBound;
			float z = kFrustum.m_fFar;
			if (!v35) {
				v35 = static_cast<BSNiBound*>(&NiAVObject::NullBoundS);
			}
			p_spShadowCamera->m_pObject->FitNearAndFarToBound(*v35);
			NiCamera* v36 = p_spShadowCamera->m_pObject;
			kFrustum = p_spShadowCamera->m_pObject->m_kViewFrustum;
			kFrustum.m_fFar = kFrustum.m_fFar + 400.0;
			if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_ISFILTER) {
				kFrustum.m_fNear = kFrustum.m_fFar * 0.20000000298023224;
			}
			kShadowCameraPlanes.Set(kFrustum, v36->m_kWorld);
			NiCamera* v37 = spShadowCamera.m_pObject;
			kFrustum.m_fNear = m_fNear;
			kFrustum.m_fFar = z;
			v37->SetViewFrustum(kFrustum);
			spShadowAccumulator.m_pObject->SetShadowLight(this);
			MTRenderingSystem_Instance.AddAccumTask(spShadowCamera.m_pObject, 0, spShadowCaster.m_pObject, 0, 0,
				spShadowAccumulator.m_pObject, BSCullingProcess::BSCP_CULL_NORMAL, a2 + 17, 0);
		}
	} else {
		if (spDynamicShadowMap.m_pObject) {
			BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spDynamicShadowMap.m_pObject);
			spDynamicShadowMap = nullptr;
			spShadowAccumulator = nullptr;
		}
	}
	MTRenderingSystem_Instance.SetThreadStage(MTRenderingSystem::THREAD_1, a2 + 17);
}

// Renders the shadow caster into the light's depth map from the shadow camera, builds the light's view, projection and
// view-projection matrices from the camera, then blurs the map when shadows are image-space filtered.
void ShadowSceneLight::MTRenderFrustumDepthMap(BSCullingProcess& arCuller, int32_t aiIndex) {
	MTRenderingSystem_Instance.WaitOnThreadStage(MTRenderingSystem::THREAD_2, aiIndex + 17);
	NiColorA kOrgColor = NiColorA_BLACK;
	NiRenderer::ms_pkRenderer->GetBackgroundColor(kOrgColor);
	NiColorA kWhite(1.0f, 1.0f, 1.0f, 1.0f);
	NiRenderer::ms_pkRenderer->SetBackgroundColor(kWhite);
	BSRenderedTexture::StartOffscreen(7u, spDynamicShadowMap.m_pObject->GetGroup());
	NiRenderer::ms_pkRenderer->SetBackgroundColor(kOrgColor);
	arCuller.spAccumulator = spShadowAccumulator.m_pObject;
	BSShaderUtil::RenderScene(spShadowCamera.m_pObject, spShadowAccumulator.m_pObject, 0);
	spShadowAccumulator.m_pObject->spShadowLight = nullptr;
	arCuller.spAccumulator = nullptr;
	NiCamera* pCamera = spShadowCamera.m_pObject;
	NiDX9Renderer* pRenderer = BSShaderManager::pRenderer.m_pObject;
	NiPoint3 kCol2;
	kCol2.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][2];
	kCol2.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][2];
	kCol2.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][2];
	NiPoint3 kCol1;
	kCol1.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][1];
	kCol1.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][1];
	kCol1.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][1];
	NiPoint3 kCol0;
	kCol0.x = pCamera->m_kWorld.m_Rotate.m_pEntry[0][0];
	kCol0.y = pCamera->m_kWorld.m_Rotate.m_pEntry[1][0];
	kCol0.z = pCamera->m_kWorld.m_Rotate.m_pEntry[2][0];
	pRenderer->SetCameraData(pCamera);
	NiLight* pLight = spLight.m_pObject;
	alignas(16) D3DXMATRIX viewMatrix = pRenderer->m_kD3DView;
	alignas(16) D3DXMATRIX projMatrix = pRenderer->m_kD3DProj;
	NiTransform kLightTransform;
	kLightTransform = pLight->m_kWorld;
	double y = kLightTransform.m_Translate.y;
	double x = kLightTransform.m_Translate.x;
	double z = kLightTransform.m_Translate.z;
	kShadowView = viewMatrix;
	// Each dot product is stored as a float before it is negated.
	float fDot = y * kCol2.y + x * kCol2.x + z * kCol2.z;
	kShadowView._41 = -fDot;
	fDot = y * kCol1.y + x * kCol1.x + z * kCol1.z;
	kShadowView._42 = -fDot;
	fDot = y * kCol0.y + x * kCol0.x + z * kCol0.z;
	kShadowView._43 = -fDot;
	kShadowProj = projMatrix;
	D3DXMatrixMultiply_Thunk(&projMatrix, &kShadowView, &kShadowProj);
	kViewProjection = projMatrix;
	BSRenderedTexture::StopOffscreen();
	if (BSShaderManager::eShadowMode == BSShaderManager::BSSM_SHADOW_ISFILTER) {
		BSRenderedTexture* pBlur = BSShaderManager::pTextureManager->BorrowFrustumShadowTexture();
		ImageSpaceTexture kBlurTexture;
		kBlurTexture.bForceAniso = 0;
		kBlurTexture.bIsRenderedTexture = 0;
		kBlurTexture.bIsTextureBorrowed = 0;
		kBlurTexture.spTexture = nullptr;
		kBlurTexture.eFilterMode = 0;
		kBlurTexture.eClampMode = 0;
		kBlurTexture.SetTexture(pBlur);
		{
			ImageSpaceEffectParam kParam;
			ImageSpaceEffectBlur::fBlurRadius = 2.0f;
			BSShaderManager::pImageSpaceManager->pEffectList.m_pBase[ImageSpaceManager::EFFECT_BLUR5]->SetTexture(2,
				&kBlurTexture, NiTexturingProperty::FILTER_NEAREST);
			BSShaderManager::pImageSpaceManager->RenderEffect(ImageSpaceManager::EFFECT_BLUR5,
				BSShaderManager::pRenderer.m_pObject, spDynamicShadowMap.m_pObject, spDynamicShadowMap.m_pObject,
				&kParam, 1);
			BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(pBlur);
		}
	}
	if (bShowDebugTexture) {
		NiRenderedTexture* Texture = spDynamicShadowMap.m_pObject->GetTexture(0);
		DebugShader* Shader = static_cast<DebugShader*>(BSShaderManager::GetShader(9));
		Shader->SetTextureToRender(Texture);
	}
	bClipSpacePlanesReady = 0;
}

bool ShadowSceneLight::BoundWithinLightFrustum(const NiBound& arBound) {
	uint32_t v4 = 0;
	float* p_z = &kShadowCameraPlanes.m_akCullingPlanes[0].m_kNormal.z;
	while ((1 << v4 & kShadowCameraPlanes.GetActivePlaneState()) == 0) {
	LABEL_6:;
		++v4;
		p_z += 4;
		if (v4 >= 6) {
			return 1;
		}
	}
	float arBoundb = *(p_z - 1) * arBound.m_kCenter.y + *(p_z - 2) * arBound.m_kCenter.x + arBound.m_kCenter.z * *p_z;
	float arBounda = arBoundb - p_z[1];
	if (!(-arBound.m_fRadius >= arBounda)) {
		if (arBound.m_fRadius <= static_cast<double>(arBounda)) {
			kShadowCameraPlanes.m_uiActivePlanes &= ~(1 << v4);
		}
		goto LABEL_6;
	}
	return 0;
}

bool ShadowSceneLight::FrustumFrustumIntersection(NiCamera* apCamera1, NiCamera* apCamera2) {
	NiFrustumPlanes* v34;
	// Each camera's frustum points: the eye, then the four far corners (x, y, z each).
	alignas(8) float v258[15];
	float v259[15];
	alignas(8) FNVD_INLINED_LOCAL(NiFrustumPlanes, v260, std::nullopt, std::nullopt);
	alignas(8) FNVD_INLINED_LOCAL(NiFrustumPlanes, v262, std::nullopt, std::nullopt);
	alignas(8) FNVD_INLINED_LOCAL(NiFrustumPlanes, v263, std::nullopt, std::nullopt);
	// The frustum's eight edges as pairs of point indices, stored byte by byte.
	uint8_t aucEdges[8][2];
	aucEdges[0][0] = 0;
	aucEdges[0][1] = 1;
	aucEdges[1][0] = 0;
	aucEdges[1][1] = 2;
	aucEdges[2][0] = 0;
	aucEdges[2][1] = 3;
	aucEdges[3][0] = 0;
	aucEdges[3][1] = 4;
	aucEdges[4][0] = 1;
	aucEdges[4][1] = 2;
	aucEdges[5][0] = 1;
	aucEdges[5][1] = 3;
	aucEdges[6][0] = 2;
	aucEdges[6][1] = 4;
	aucEdges[7][0] = 3;
	aucEdges[7][1] = 4;
	NiPlane* v3 = v262.m_akCullingPlanes;
	for (int32_t i = 5; i >= 0; --i) {
		::new (v3) NiPlane;
		++v3;
	}
	v262.m_uiActivePlanes = 63;
	NiPlane* v4 = v263.m_akCullingPlanes;
	for (int32_t j = 5; j >= 0; --j) {
		::new (v4) NiPlane;
		++v4;
	}
	float x = apCamera1->m_kWorld.m_Translate.x;
	float v213 = apCamera1->m_kWorld.m_Rotate.m_pEntry[0][0];
	float y = apCamera1->m_kWorld.m_Translate.y;
	float z = apCamera1->m_kWorld.m_Translate.z;
	float v216 = apCamera1->m_kWorld.m_Rotate.m_pEntry[1][0];
	v263.m_uiActivePlanes = 63;
	float v218 = apCamera1->m_kWorld.m_Rotate.m_pEntry[2][0];
	float v252 = apCamera1->m_kWorld.m_Rotate.m_pEntry[0][1];
	v258[0] = x;
	double v8 = apCamera1->m_kWorld.m_Rotate.m_pEntry[1][1];
	v258[1] = y;
	float v255 = v8;
	v258[2] = z;
	float m_fTop = apCamera1->m_kViewFrustum.m_fTop;
	float v240 = v252 * m_fTop;
	float v203 = v255 * m_fTop;
	float v190 = m_fTop * apCamera1->m_kWorld.m_Rotate.m_pEntry[2][1];
	double m_fRight = apCamera1->m_kViewFrustum.m_fRight;
	float v177 = apCamera1->m_kWorld.m_Rotate.m_pEntry[0][2] * m_fRight;
	float v231 = apCamera1->m_kWorld.m_Rotate.m_pEntry[1][2] * m_fRight;
	float v244 = m_fRight * apCamera1->m_kWorld.m_Rotate.m_pEntry[2][2];
	float m_fFar = apCamera1->m_kViewFrustum.m_fFar;
	float v178 = v177 * m_fFar;
	double v10 = v178;
	float v179 = v231 * m_fFar;
	double v11 = v179;
	float v180 = v244 * m_fFar;
	double v12 = v180;
	float v181 = v240 * m_fFar;
	double v13 = v181;
	float v204 = v203 * m_fFar;
	double v14 = v204;
	float v241 = v190 * m_fFar;
	float v245 = v213 * m_fFar;
	float v191 = v216 * m_fFar;
	double v15 = v191;
	float v222 = m_fFar * v218;
	float v205 = v245 + x;
	float v192 = y + v191;
	float v182 = z + v222;
	float v206 = v205 + v181;
	float v226 = v192 + v204;
	float v194 = v182 + v241;
	float v151 = v206 + v178;
	float v116 = v226 + v179;
	float v75 = v194 + v180;
	v258[3] = v151;
	v258[4] = v116;
	v258[5] = v75;
	float v207 = v10;
	float v183 = v11;
	float v233 = v12;
	float v195 = v13;
	float v227 = v14;
	float v117 = v15;
	float v76 = v245 + x;
	float v118 = y + v117;
	float v152 = z + v222;
	float v77 = v76 + v195;
	float v119 = v118 + v227;
	float v153 = v152 + v241;
	float v78 = v77 - v207;
	float v120 = v119 - v183;
	float v154 = v153 - v233;
	v258[6] = v78;
	v258[7] = v120;
	v258[8] = v154;
	float v208 = v10;
	float v184 = v11;
	float v234 = v12;
	float v196 = v13;
	float v121 = v15;
	float v79 = v245 + x;
	float v122 = y + v121;
	float v155 = z + v222;
	float v80 = v79 - v196;
	float v123 = v122 - v227;
	float v156 = v155 - v241;
	float v81 = v80 + v208;
	float v124 = v123 + v184;
	float v157 = v156 + v234;
	v258[9] = v81;
	v258[10] = v124;
	v258[11] = v157;
	float v125 = v15;
	float v82 = v245 + x;
	float v126 = y + v125;
	float v158 = z + v222;
	float v83 = v82 - v196;
	float v127 = v126 - v227;
	float v159 = v158 - v241;
	float v84 = v83 - v208;
	float v128 = v127 - v184;
	float v160 = v159 - v234;
	v258[12] = v84;
	float v16 = apCamera2->m_kWorld.m_Translate.x;
	double v17 = apCamera2->m_kWorld.m_Rotate.m_pEntry[0][0];
	v258[13] = v128;
	float v18 = apCamera2->m_kWorld.m_Translate.y;
	float v253 = v17;
	double v19 = apCamera2->m_kWorld.m_Rotate.m_pEntry[1][0];
	v258[14] = v160;
	float v256 = v19;
	float v250 = apCamera2->m_kWorld.m_Translate.z;
	float v257 = apCamera2->m_kWorld.m_Rotate.m_pEntry[2][0];
	v259[0] = v16;
	double v20 = apCamera2->m_kWorld.m_Rotate.m_pEntry[0][1];
	v259[1] = v18;
	float v214 = v20;
	v259[2] = v250;
	float v85 = apCamera2->m_kViewFrustum.m_fTop;
	float v197 = v214 * v85;
	float v228 = apCamera2->m_kWorld.m_Rotate.m_pEntry[1][1] * v85;
	float v193 = v85 * apCamera2->m_kWorld.m_Rotate.m_pEntry[2][1];
	double v21 = apCamera2->m_kViewFrustum.m_fRight;
	float v86 = apCamera2->m_kWorld.m_Rotate.m_pEntry[0][2] * v21;
	float v129 = apCamera2->m_kWorld.m_Rotate.m_pEntry[1][2] * v21;
	float v161 = v21 * apCamera2->m_kWorld.m_Rotate.m_pEntry[2][2];
	double v22 = apCamera2->m_kViewFrustum.m_fFar;
	float v87 = v86 * v22;
	double v23 = v87;
	float v88 = v129 * v22;
	double v24 = v88;
	float v89 = v161 * v22;
	double v25 = v89;
	float v90 = v197 * v22;
	double v26 = v90;
	float v91 = v228 * v22;
	double v27 = v91;
	float v223 = v193 * v22;
	float v246 = v253 * v22;
	float v92 = v256 * v22;
	double v28 = v22 * v257;
	double v29 = v92;
	float v242 = v28;
	float v130 = v246 + v16;
	float v93 = v18 + v92;
	float v162 = v250 + v242;
	float v131 = v130 + v90;
	float v94 = v93 + v91;
	float v163 = v162 + v223;
	float v132 = v131 + v87;
	float v95 = v94 + v88;
	float v164 = v163 + v89;
	v259[3] = v132;
	v259[4] = v95;
	v259[5] = v164;
	float v210 = v23;
	float v186 = v24;
	float v236 = v25;
	float v199 = v26;
	float v230 = v27;
	float v133 = v29;
	float v96 = v246 + v16;
	float v134 = v18 + v133;
	float v165 = v250 + v242;
	float v97 = v96 + v199;
	float v135 = v134 + v230;
	float v166 = v165 + v223;
	float v98 = v97 - v210;
	float v136 = v135 - v186;
	float v167 = v166 - v236;
	v259[6] = v98;
	v259[7] = v136;
	v259[8] = v167;
	float v211 = v23;
	float v187 = v24;
	float v237 = v25;
	float v200 = v26;
	float v137 = v29;
	float v99 = v246 + v16;
	float v138 = v18 + v137;
	float v168 = v250 + v242;
	float v100 = v99 - v200;
	float v139 = v138 - v230;
	float v169 = v168 - v223;
	float v101 = v100 + v211;
	float v140 = v139 + v187;
	float v170 = v169 + v237;
	v259[9] = v101;
	float v212 = v23;
	v259[10] = v140;
	v259[11] = v170;
	float v188 = v24;
	float v238 = v25;
	float v141 = v29;
	float v102 = v246 + v16;
	float v142 = v18 + v141;
	float v171 = v250 + v242;
	float v103 = v102 - v200;
	float v143 = v142 - v230;
	float v172 = v171 - v223;
	float v104 = v103 - v212;
	float v144 = v143 - v188;
	float v173 = v172 - v238;
	v259[12] = v104;
	v259[13] = v144;
	v259[14] = v173;
	alignas(8) NiCullingProcess kcullproc(nullptr);
	kcullproc.m_pkCamera = apCamera1;
	kcullproc.SetFrustum(apCamera1->m_kViewFrustum);
	v262 = kcullproc.m_kPlanes;
	kcullproc.m_pkCamera = apCamera2;
	kcullproc.SetFrustum(apCamera2->m_kViewFrustum);
	v263 = kcullproc.m_kPlanes;
	NiPlane* v30 = v260.m_akCullingPlanes;
	for (int32_t k = 5; k >= 0; --k) {
		::new (v30) NiPlane;
		++v30;
	}
	double v32 = 0.0;
	int32_t v33 = 0;
	v260.m_uiActivePlanes = 63;
	float* v243 = 0;
LABEL_8:;
	if (v33) {
		if (v33 != 1) {
			goto LABEL_13;
		}
		v243 = v259;
		v34 = &v262;
	} else {
		v243 = v258;
		v34 = &v263;
	}
	v260 = *v34;
LABEL_13:;
	int32_t v35 = 0;
	float* v36 = v243 + 2;
LABEL_14:;
	int32_t v37 = 0;
	float* p_z = &v260.m_akCullingPlanes[0].m_kNormal.z;
	do {
		float v105 = *(p_z - 2) * *(v36 - 2) + *(p_z - 1) * *(v36 - 1) + *v36 * *p_z - p_z[1];
		if (!(v105 > 0.0f)) {
			++v35;
			v36 += 3;
			if (v35 < 5) {
				goto LABEL_14;
			}
			if (++v33 >= 2) {
				NiFrustumPlanes* v40;
				int32_t v39 = 0;
				int32_t v189 = 0;
			LABEL_21:;
				if (v39) {
					if (v39 == 1) {
						v243 = v259;
						v40 = &v262;
						goto LABEL_25;
					}
				} else {
					v243 = v258;
					v40 = &v263;
				LABEL_25:;
					v260 = *v40;
				}
				int32_t v41 = 0;
			LABEL_27:;
				float* v42 = &v243[3 * aucEdges[v41][0]];
				float v254 = *v42;
				double v43 = v42[2];
				double v44 = v42[1];
				float* v45 = &v243[3 * aucEdges[v41][1]];
				double v46 = v254;
				float v248 = *v45;
				float v249 = v45[1];
				float v251 = v45[2];
				int32_t v47 = 0;
				float* v48 = &v260.m_akCullingPlanes[0].m_kNormal.z;
				while (1) {
					int32_t v52;
					int32_t v53;
					double v58;
					double v60;
					double v63;
					float v106 = v46 * *(v48 - 2) + *(v48 - 1) * v44 + v43 * *v48 - v48[1];
					if (!(v106 < v32)) {
						v52 = v106 > v32;
					} else {
						v52 = 2;
					}
					float v107 = v248 * *(v48 - 2) + *(v48 - 1) * v249 + v251 * *v48 - v48[1];
					if (!(v107 < v32)) {
						v53 = v107 > v32;
					} else {
						v53 = 2;
					}
					if (v52 == v53) {
						if (!v52 && !v53) {
							goto LABEL_52;
						}
						double v70 = v46;
						double v71 = v32;
						v58 = v44;
						double v72 = v71;
						v60 = v70;
						v63 = v72;
					} else {
						float v215 = *(v48 - 2);
						float v217 = *(v48 - 1);
						double v54 = v48[1];
						float v108 = v215 * v54;
						float v145 = v54 * v217;
						float v174 = v54 * *v48;
						float v201 = v108 - v46;
						float v109 = v145 - v44;
						float v146 = v174 - v43;
						float v224 = v248 - v46;
						float v247 = v249 - v44;
						float v239 = v251 - v43;
						float v110 = v109 * v217 + v201 * v215 + v146 * *v48;
						double v55 = v110;
						float v111 = v215 * v224 + v217 * v247 + *v48 * v239;
						double v56 = v44;
						double v57 = v55 / v111;
						v58 = v56;
						float v147 = v57;
						double v59 = v147;
						float v112 = v224 * v147;
						float v148 = v247 * v147;
						float v175 = v239 * v59;
						v60 = v254;
						float v202 = v112 + v254;
						float v149 = v148 + v56;
						double v61 = 0.0;
						if (!(v59 >= 0.0)) {
						LABEL_51:;
							v63 = v61;
						} else {
							bool v62 = !(v59 <= 1.0);
							v63 = 0.0;
							if (!v62) {
								float v113 = v175 + v43;
								double v64 = v113;
								int32_t v65 = 0;
								float* v66 = &v260.m_akCullingPlanes[0].m_kNormal.z;
								do {
									if (v65 != v47) {
										float v114 = *(v66 - 2) * v202 + *(v66 - 1) * v149 + v64 * *v66 - v66[1];
										double v67 = v64;
										v61 = v63;
										double v68 = v67;
										if (v61 > v114) {
											goto LABEL_51;
										}
										double v69 = v61;
										v64 = v68;
										v63 = v69;
									}
									++v65;
									v66 += 4;
								} while (v65 < 6);
								goto LABEL_52;
							}
						}
					}
					++v47;
					v48 += 4;
					if (v47 >= 6) {
						++v41;
						v32 = v63;
						if (v41 < 8) {
							goto LABEL_27;
						}
						v39 = ++v189;
						if (v189 >= 2) {
							return 0;
						}
						goto LABEL_21;
					}
					double v49 = v60;
					double v50 = v58;
					v32 = v63;
					double v51 = v50;
					v46 = v49;
					v44 = v51;
				}
			}
			goto LABEL_8;
		}
		++v37;
		p_z += 4;
	} while (v37 < 6);
LABEL_52:;
	return 1;
}

bool ShadowSceneLight::CubeFaceFrustumFrustumIntersection(NiBound aBound, NiCullingProcess& arCuller,
	uint16_t ausCubeFace) {
	NiUpdateData updateData;
	FNVD_INLINED_LOCAL(BSCubeMapCamera, kCamera, std::nullopt, std::nullopt);
	TLSData* v5 = GetThreadTLSData();
	NiCamera* m_pkCamera = arCuller.m_pkCamera;
	MEM_CONTEXT eMemContext = v5->eMemContext;
	v5->eMemContext = MEM_CONTEXT_SHADOWS;
	::new (&kCamera) BSCubeMapCamera(0);
	kCamera.m_kLocal.m_Translate = aBound.m_kCenter;
	kCamera.SetupTransformation(static_cast<NiRenderedCubeMap::FaceID>(ausCubeFace));
	updateData.fTime = 0.0;
	updateData.bUpdateControllers = 0;
	updateData.bParallelUpdate = 0;
	updateData.bFoundParticles = 0;
	updateData.bFoundMorphController = 0;
	updateData.bSceneGraphChange = 0;
	kCamera.Update(updateData);
	bool v8 = FrustumFrustumIntersection(m_pkCamera, &kCamera);
	kCamera.~BSCubeMapCamera();
	v5->eMemContext = eMemContext;
	return v8;
}

void ShadowSceneLight::ClearRenderPasses() {
	NiTListItem<NiPointer<NiGeometry>>* pPos = kGeomListFence;
	while (pPos) {
		NiGeometry* pGeometry = pPos->m_element.m_pObject;
		pPos = pPos->m_pkNext;
		if (pGeometry) {
			BSShaderProperty* pProperty =
				static_cast<BSShaderProperty*>(pGeometry->m_PropertyState.m_aspProps[3].m_pObject);
			if (pProperty) {
				// The shade property is a lighting property when its shader type is 1 to 12 (one unsigned compare); the
				// pointer or null is selected, then tested.
				BSShaderLightingProperty* pLighting = static_cast<uint32_t>(pProperty->iShaderPropertyType - 1) <=
						BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - 1
					? static_cast<BSShaderLightingProperty*>(pProperty)
					: nullptr;
				if (pLighting) {
					pLighting->iLastRenderPassState = 0;
				}
			}
		}
	}
	kGeomListFence = nullptr;
}

int32_t ShadowSceneLight::SortByLuminance(const void* appLight1, const void* appLight2) {
	const float fFirst = (*static_cast<ShadowSceneLight* const*>(appLight1))->fLuminance;
	const float fSecond = (*static_cast<ShadowSceneLight* const*>(appLight2))->fLuminance;
	if (fSecond < fFirst)
		return -1;
	return fSecond > fFirst;
}

bool ShadowSceneLight::IsEqual(ShadowSceneLight** a1, ShadowSceneLight** a2) {
	return *a1 == *a2;
}
