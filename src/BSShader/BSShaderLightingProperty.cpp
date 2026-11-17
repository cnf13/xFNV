#include "BSShader/BSShaderLightingProperty.h"

#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderPPLightingProperty.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/Lighting30Shader.h"
#include "BSShader/ShadowSceneLight.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPropertyState.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTAbstractPoolAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiMain/Xenon/NiPoint4.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/System.h"

BSShaderLightingProperty::ShaderLightConstantTable BSShaderLightingProperty::pShaderLightConstants;

ShadowSceneLight BSShaderLightingProperty::kEmptyLight;
NiRTTI BSShaderLightingProperty::ms_RTTI("BSShaderLightingProperty", &BSShaderProperty::ms_RTTI);

// A light is active when it is not culled and its NiLight is not app-culled.
static bool IsActiveLight(const ShadowSceneLight* apLight) {
	return apLight->usFrustumCull != ShadowSceneLight::SSL_ALL &&
		!(apLight->spLight->m_uFlags & NiAVObject::APP_CULLED);
}

// An active light that also does not cast shadows (the engine compares the flag with 1).
static bool IsActiveNonShadowLight(const ShadowSceneLight* apLight) {
	return IsActiveLight(apLight) && apLight->bCastShadow != 1;
}

using LightItem = BSShaderLightingProperty::ShadowSceneLightItem;

// Steps *apIterator along the list from pNext/pLight until a light passes pfnActive; returns it or null.
template <bool (*Active)(const ShadowSceneLight*)>
static ShadowSceneLight* SkipToActiveLight(LightItem** apIterator, LightItem* pNext, ShadowSceneLight* pLight) {
	while (pNext) {
		if (!pLight)
			return nullptr;
		if (Active(pLight))
			break;

		LightItem* pItem = *apIterator;
		pNext = pItem->m_pkNext;
		*apIterator = pNext;
		pLight = pItem->m_element;
	}

	if (!pLight)
		return nullptr;
	return Active(pLight) ? pLight : nullptr;
}

template <bool (*Active)(const ShadowSceneLight*)>
static ShadowSceneLight* FirstActiveLight(const BSShaderLightingProperty* apProperty, LightItem** apIterator) {
	LightItem* pHead = apProperty->lLightList.m_pkHead;
	*apIterator = pHead;
	if (!pHead)
		return nullptr;

	LightItem* pNext = pHead->m_pkNext;
	*apIterator = pNext;
	return SkipToActiveLight<Active>(apIterator, pNext, pHead->m_element);
}

template <bool (*Active)(const ShadowSceneLight*)> static ShadowSceneLight* NextActiveLight(LightItem** apIterator) {
	LightItem* pItem = *apIterator;
	if (!pItem)
		return nullptr;

	LightItem* pNext = pItem->m_pkNext;
	*apIterator = pNext;
	ShadowSceneLight* pLight = pItem->m_element;
	if (!pLight)
		return nullptr;
	return SkipToActiveLight<Active>(apIterator, pNext, pLight);
}

// Property type 1, an empty light list, no forced darkness and no fence.
BSShaderLightingProperty::BSShaderLightingProperty()
	: fForcedDarkness(1.0f), uiReferenceID(0), bLightListChanged(false), kLightListFence(nullptr) {
	lLightList.RemoveAll();
	iShaderPropertyType = 1;
}

// Empties the light list; the list's own destructor runs after.
BSShaderLightingProperty::~BSShaderLightingProperty() {
	lLightList.RemoveAll();
}

NiObject* BSShaderLightingProperty::CreateClone(NiCloningProcess& arCloneData) {
	BSShaderLightingProperty* pClone =
		static_cast<BSShaderLightingProperty*>(NiMemObject::operator new(sizeof(BSShaderLightingProperty)));
	if (pClone) {
		pClone = ::new (pClone) BSShaderLightingProperty;
	} else {
		pClone = nullptr;
	}
	BSShaderProperty::CopyMembers(pClone, arCloneData);
	CopyToMembers(pClone);
	return pClone;
}

void BSShaderLightingProperty::LoadBinary(NiStream& kStream) {
	BSShaderProperty::LoadBinary(kStream);
}

void BSShaderLightingProperty::LinkObject(NiStream& kStream) {
	BSShaderProperty::LinkObject(kStream);
}

bool BSShaderLightingProperty::RegisterStreamables(NiStream& kStream) {
	return BSShaderProperty::RegisterStreamables(kStream);
}

void BSShaderLightingProperty::SaveBinary(NiStream& kStream) {
	BSShaderProperty::SaveBinary(kStream);
}

bool BSShaderLightingProperty::IsEqual(NiObject* pkObject) {
	return BSShaderProperty::IsEqual(pkObject);
}

void BSShaderLightingProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSShaderProperty::GetViewerStrings(apStrings);
	apStrings->Add(NiGetViewerString(BSShaderLightingProperty::ms_RTTI.GetName()));
	apStrings->Add(NiGetViewerString("number of lights", static_cast<uint16_t>(lLightList.m_uiCount)));
	apStrings->Add(NiGetViewerString("active lights", static_cast<uint16_t>(GetNumberofActiveLights())));
	apStrings->Add(NiGetViewerString("Reference ID", uiReferenceID));
}

void BSShaderLightingProperty::CopyTo(BSShaderLightingProperty* pDest) {
	BSShaderProperty::CopyTo(pDest);
	CopyToMembers(pDest);
}

// The base members first, then the lighting copy hook on pDest.
void BSShaderLightingProperty::CopyMembers(BSShaderLightingProperty* pDest, NiCloningProcess& arCloneData) {
	BSShaderProperty::CopyMembers(pDest, arCloneData);
	CopyToMembers(pDest);
}

bool BSShaderLightingProperty::IsAttachedToLight(ShadowSceneLight* apSSLight, NiBound* apObjBound,
	void*& apInsertPosition) const {
	apInsertPosition = nullptr;
	if (lLightList.m_uiCount < 1)
		return false;

	LightItem* pItem = lLightList.m_pkHead;
	ShadowSceneLight* pLight = pItem->m_element;
	LightItem* pNext = pItem->m_pkNext;
	while (pLight) {
		if (pLight == apSSLight) {
			apInsertPosition = pItem;
			return true;
		}
		if (!pNext)
			return false;
		pLight = pNext->m_element;
		pItem = pNext;
		pNext = pNext->m_pkNext;
	}
	return false;
}

// The iterator is a NiTListIterator (void*) holding the list node.
ShadowSceneLight* BSShaderLightingProperty::GetFirstActiveLight(void*& aitIter) const {
	return FirstActiveLight<IsActiveLight>(this, reinterpret_cast<LightItem**>(&aitIter));
}

ShadowSceneLight* BSShaderLightingProperty::GetNextActiveLight(void*& aitIter) const {
	return NextActiveLight<IsActiveLight>(reinterpret_cast<LightItem**>(&aitIter));
}

ShadowSceneLight* BSShaderLightingProperty::GetNextLight(void*& aitIter) {
	ShadowSceneLightItem* pItem = lLightList.m_pkHead;
	aitIter = pItem;
	if (!pItem) {
		return nullptr;
	}
	aitIter = pItem->m_pkNext;
	return pItem->m_element;
}

ShadowSceneLight* BSShaderLightingProperty::GetFirstActiveNonShadowLight(void*& aitIter) const {
	return FirstActiveLight<IsActiveNonShadowLight>(this, reinterpret_cast<LightItem**>(&aitIter));
}

ShadowSceneLight* BSShaderLightingProperty::GetNextActiveNonShadowLight(void*& aitIter) const {
	return NextActiveLight<IsActiveNonShadowLight>(reinterpret_cast<LightItem**>(&aitIter));
}

uint16_t BSShaderLightingProperty::GetNumberofActiveLights() const {
	uint32_t uiCount = 0;
	for (LightItem* pItem = lLightList.m_pkHead; pItem;) {
		ShadowSceneLight* pLight = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (pLight && IsActiveLight(pLight))
			++uiCount;
	}
	return uiCount;
}

uint16_t BSShaderLightingProperty::GetNumberofActiveNonShadowLights() const {
	uint32_t uiCount = 0;
	for (LightItem* pItem = lLightList.m_pkHead; pItem;) {
		ShadowSceneLight* pLight = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (pLight && pLight->usFrustumCull != ShadowSceneLight::SSL_ALL &&
			!(pLight->spLight->m_uFlags & NiAVObject::APP_CULLED) && !pLight->bCastShadow)
			++uiCount;
	}
	return uiCount;
}

void BSShaderLightingProperty::FenceAllLights() {
	ShadowSceneLight* pEmptyLight = &kEmptyLight;
	lLightList.Remove(pEmptyLight);
	ShadowSceneLightItem* pkNode =
		static_cast<ShadowSceneLightItem*>(reinterpret_cast<NiTPointerAllocator<uint32_t>&>(lLightList.m_uiCount)
				.Allocate());
	pkNode->m_element = &kEmptyLight;
	pkNode->m_pkPrev = nullptr;
	pkNode->m_pkNext = lLightList.m_pkHead;
	if (lLightList.m_pkHead) {
		lLightList.m_pkHead->m_pkPrev = pkNode;
	} else {
		lLightList.m_pkTail = pkNode;
	}
	++lLightList.m_uiCount;
	lLightList.m_pkHead = pkNode;
	kLightListFence = pkNode;
}

// Each node after the fence is removed. Gamebryo's NiTPointerAllocator::Deallocate casts the item to the pool's node.
void BSShaderLightingProperty::RemoveFencedLights(NiTriBasedGeom* apGeometry) {
	if (kLightListFence != lLightList.m_pkTail) {
		bLightListChanged = true;
		ShadowSceneLightItem* pkNext = kLightListFence->m_pkNext;
		while (pkNext) {
			ShadowSceneLightItem* pkNode = pkNext;
			ShadowSceneLight* pLight = pkNode->m_element;
			pkNext = pkNode->m_pkNext;
			if (pkNode == lLightList.m_pkHead) {
				lLightList.m_pkHead = pkNode->m_pkNext;
				if (lLightList.m_pkHead) {
					lLightList.m_pkHead->m_pkPrev = nullptr;
				} else {
					lLightList.m_pkTail = nullptr;
				}
				auto* pkFree = reinterpret_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(pkNode);
				pkNode->m_element = nullptr;
				NiTAbstractPoolAllocator<uint32_t>::Lock();
				pkFree->m_pkData = nullptr;
				pkFree->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
				NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pkFree;
				NiTAbstractPoolAllocator<uint32_t>::Unlock();
				--lLightList.m_uiCount;
			} else if (pkNode == lLightList.m_pkTail) {
				lLightList.RemoveTail();
			} else {
				ShadowSceneLightItem* pkPrev = pkNode->m_pkPrev;
				ShadowSceneLightItem* pkAfter = pkNode->m_pkNext;
				if (pkPrev) {
					pkPrev->m_pkNext = pkAfter;
				}
				if (pkAfter) {
					pkAfter->m_pkPrev = pkPrev;
				}
				auto* pkFree = reinterpret_cast<NiTAbstractPoolAllocator<uint32_t>::AllocNode*>(pkNode);
				pkNode->m_element = nullptr;
				NiTAbstractPoolAllocator<uint32_t>::Lock();
				pkFree->m_pkData = nullptr;
				pkFree->m_pkNext = NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem;
				NiTAbstractPoolAllocator<uint32_t>::ms_pkFreeMem = pkFree;
				NiTAbstractPoolAllocator<uint32_t>::Unlock();
				--lLightList.m_uiCount;
			}
			pLight->RemoveTriGeom(apGeometry, nullptr);
			if (pLight->bCastShadow) {
				iLastRenderPassState = 0;
			}
		}
	}
	lLightList.RemovePos(kLightListFence);
	kLightListFence = nullptr;
}

void BSShaderLightingProperty::AttachLight(ShadowSceneLight* apSSLight, NiBound* apObjBound) {
	// Deliberately reuses apObjBound as IsAttachedToLight's node out-parameter, as the game does.
	void*& rpItem = *reinterpret_cast<void**>(&apObjBound);
	ShadowSceneLightItem* pFence = kLightListFence;
	if (pFence) {
		if (IsAttachedToLight(apSSLight, apObjBound, rpItem)) {
			ShadowSceneLightItem* pItem = static_cast<ShadowSceneLightItem*>(rpItem);
			if (!pItem) {
				pItem = lLightList.m_pkTail;
			}
			lLightList.MoveToBefore(pItem, pFence);
		} else {
			lLightList.InsertBefore(pFence, apSSLight);
			bLightListChanged = true;
		}
	} else {
		if (!IsAttachedToLight(apSSLight, apObjBound, rpItem)) {
			lLightList.AddTail(apSSLight);
		}
		bLightListChanged = true;
		if (apSSLight->bCastShadow) {
			iLastRenderPassState = 0;
		}
	}
}

void BSShaderLightingProperty::RemoveLight(ShadowSceneLight* apSSLight) {
	if (kLightListFence) {
		return;
	}
	lLightList.Remove(apSSLight);
	bLightListChanged = true;
	if (apSSLight->bCastShadow) {
		iLastRenderPassState = 0;
	}
	// The light must be gone from the list now.
	if (!kLightListFence) {
		for (ShadowSceneLightItem* pItem = lLightList.m_pkHead; pItem;) {
			ShadowSceneLight* pLight = pItem->m_element;
			pItem = pItem->m_pkNext;
			if (pLight == apSSLight) {
				platform::DebuggerBreak();
			}
		}
	}
}

void BSShaderLightingProperty::ResortLights(NiBound* apObjBound) {
	ShadowSceneLightItem* kIter = lLightList.m_pkHead;
	char bChanged = 0;
	if (kIter) {
		while (1) {
			ShadowSceneLightItem* kDest = lLightList.m_pkHead;
			ShadowSceneLightItem* kNext = kIter->m_pkNext;
			float fLuminance = kIter->m_element->GetLuminanceToBound(apObjBound);
			while (kDest != kIter) {
				if (!kDest) {
					break;
				}
				ShadowSceneLight* m_pObject = kDest->m_element;
				ShadowSceneLightItem* v5 = kDest;
				kDest = kDest->m_pkNext;
				if (m_pObject->GetLuminanceToBound(apObjBound) > static_cast<double>(fLuminance)) {
					if (kIter != v5) {
						if (lLightList.m_pkHead == kIter) {
							lLightList.m_pkHead = kIter->m_pkNext;
						}
						if (lLightList.m_pkHead == v5) {
							lLightList.m_pkHead = kIter;
						}
						if (lLightList.m_pkTail == kIter) {
							lLightList.m_pkTail = kIter->m_pkPrev;
						}
						if (kIter->m_pkNext) {
							kIter->m_pkNext->m_pkPrev = kIter->m_pkPrev;
						}
						ShadowSceneLightItem* m_pkPrev = kIter->m_pkPrev;
						if (m_pkPrev) {
							m_pkPrev->m_pkNext = kIter->m_pkNext;
						}
						ShadowSceneLightItem* v7 = v5->m_pkPrev;
						kIter->m_pkPrev = v7;
						kIter->m_pkNext = v5;
						if (v7) {
							v7->m_pkNext = kIter;
						}
						v5->m_pkPrev = kIter;
					}
					kDest = 0;
					bChanged = 1;
				}
			}
			if (!kNext) {
				break;
			}
			kIter = kNext;
		}
		if (bChanged) {
			iLastRenderPassState = 0;
		}
	}
}

void BSShaderLightingProperty::SetDirectionalLight3x(uint32_t auiLightIndex, ShadowSceneLight* apLight,
	float afForcedDarkness) {
	if (apLight) {
		int32_t iShaderPropertyType;
		double v9;
		FNVD_INLINED_LOCAL(NiColorA, pambientColor, std::nullopt, std::nullopt);
		FNVD_INLINED_LOCAL(NiColorA, kdata, std::nullopt, std::nullopt);
		FNVD_INLINED_LOCAL(NiColorA, kcolor, std::nullopt, std::nullopt);
		BSShaderPPLightingProperty* m_pObject =
			static_cast<BSShaderPPLightingProperty*>(BSShaderManager::pCurrentRenderPass->pGeometry->m_PropertyState
					.m_aspProps[3]
					.m_pObject);
		NiDirectionalLight* v5 = static_cast<NiDirectionalLight*>(apLight->spLight.m_pObject);
		if (!m_pObject ||
			((iShaderPropertyType = m_pObject->iShaderPropertyType, iShaderPropertyType < 8) ||
				iShaderPropertyType > 12)) {
			m_pObject = 0;
		}
		float y = v5->m_kWorldDir.y;
		float z = v5->m_kWorldDir.z;
		// MAYBE-INLINED 0xB70E69: a write opening NiColorA's uses: may be its inlined constructor
		kdata.r = v5->m_kWorldDir.x;
		// MAYBE-INLINED 0xB70E75: a write opening NiColorA's uses: may be its inlined constructor
		kdata.g = y;
		// MAYBE-INLINED 0xB70E81: a write opening NiColorA's uses: may be its inlined constructor
		kdata.b = z;
		if (m_pObject) {
			v9 = m_pObject->GetSpecularLODFade();
		} else {
			v9 = 1.0;
		}
		kdata.a = v9;
		pambientColor = NiColorA_BLACK;
		float pkLight = v5->m_fDimmer;
		bool bmodambient = 0;
		double v10 = pkLight;
		if (!BSShaderManager::bHDR && v10 > 1.0) {
			pkLight = 1.0f;
			v10 = 1.0f;
		}
		float v17 = v5->m_kAmb.r * v10;
		float v19 = v5->m_kAmb.g * v10;
		float v22 = v10 * v5->m_kAmb.b;
		if (BSShaderManager::fMinAmbient > 0.0f) {
			float v15 = v17 * 0.33000001311302185 + 0.3400000035762787 * v19 + v22 + 0.33000001311302185;
			double v11 = v15;
			if (!(v15 <= 0.0f)) {
				double v12;
				double v13;
				if (!(BSShaderManager::fMinAmbient < v11)) {
					v13 = BSShaderManager::fMinAmbient / v11;
					v12 = v22;
				} else {
					v12 = v22;
					v13 = 1.0;
				}
				float v16 = v13;
				v17 = (v17 + 0.10000000149011612) * v16;
				v19 = (v19 + 0.10000000149011612) * v16;
				v22 = (v12 + 0.10000000149011612) * v16;
			} else {
				v17 = BSShaderManager::fMinAmbient;
				v19 = BSShaderManager::fMinAmbient;
				v22 = BSShaderManager::fMinAmbient;
			}
		}
		float v18 = v17 * afForcedDarkness;
		float v20 = v19 * afForcedDarkness;
		float v23 = v22 * afForcedDarkness;
		if (afForcedDarkness < 1.0) {
			bmodambient = 1;
		}
		pambientColor.r = v18;
		pambientColor.g = v20;
		pambientColor.b = v23;
		pambientColor.a = 1.0;
		Lighting30Shader::SetAmbientColor(pambientColor, bmodambient);
		float pkLightb = pkLight * afForcedDarkness;
		float pkLighta = pkLightb * apLight->fLODDimmer;
		if (BSShaderManager::bHDR && !BSShaderManager::bInterior) {
			pkLighta = BSShaderManager::fSunlightDimmer * pkLighta;
		}
		float g = v5->m_kDiff.g;
		float b = v5->m_kDiff.b;
		// MAYBE-INLINED 0xB710C4: a write opening NiColorA's uses: may be its inlined constructor
		kcolor.r = v5->m_kDiff.r * pkLighta;
		// MAYBE-INLINED 0xB710D4: a write opening NiColorA's uses: may be its inlined constructor
		kcolor.g = g * pkLighta;
		// MAYBE-INLINED 0xB710E3: a write opening NiColorA's uses: may be its inlined constructor
		kcolor.b = pkLighta * b;
		// MAYBE-INLINED 0xB710F8: a write opening NiColorA's uses: may be its inlined constructor
		kcolor.a = apLight->fShadowLODDimmer;
		Lighting30Shader::SetLight(auiLightIndex, kdata, kcolor);
	} else {
		Lighting30Shader::SetLightColor(auiLightIndex, NiColorA_BLACK);
	}
}

void BSShaderLightingProperty::SetLight1x2x(uint32_t auiLightIndex, ShadowSceneLight* apLight, float afForcedDarkness) {
	if (auiLightIndex >= 8) {
		return;
	}
	if (!apLight) {
		pShaderLightConstants[auiLightIndex + 1].x = NiColorA_BLACK.r;
		pShaderLightConstants[auiLightIndex + 1].y = NiColorA_BLACK.g;
		pShaderLightConstants[auiLightIndex + 1].z = NiColorA_BLACK.b;
		pShaderLightConstants[auiLightIndex + 1].w = NiColorA_BLACK.a;
		return;
	}

	NiLight* pLight = apLight->spLight.m_pObject;
	bool bPointLight = apLight->bPointLight;
	bool bAmbientLight = apLight->bAmbientLight;
	// The geometry's shade property, when it is a per-pixel lighting property (property types 8 to 12).
	BSShaderPPLightingProperty* pShaderProp =
		static_cast<BSShaderPPLightingProperty*>(BSShaderManager::pCurrentRenderPass->pGeometry->m_PropertyState
				.m_aspProps[3]
				.m_pObject);
	if (pShaderProp && (pShaderProp->iShaderPropertyType < 8 || pShaderProp->iShaderPropertyType > 12)) {
		pShaderProp = nullptr;
	}

	NiColor kColor;
	if (bPointLight) {
		if (pLight) {
			// The light's position.
			FNVD_INLINED_LOCAL(NiColorA, kPosition, std::nullopt, std::nullopt);
			kPosition.r = pLight->m_kWorld.m_Translate.x;
			kPosition.g = pLight->m_kWorld.m_Translate.y;
			kPosition.b = pLight->m_kWorld.m_Translate.z;
			kPosition.a = 1.0f;
			SetShaderConstant(static_cast<int32_t>(auiLightIndex + 11), kPosition);
		}
	} else {
		if (pLight) {
			// The light's direction, its w the specular fade.
			NiPoint3 kDir = static_cast<NiDirectionalLight*>(pLight)->m_kWorldDir;
			kDir.Unitize();
			float fSpecularFade;
			if (pShaderProp) {
				fSpecularFade = pShaderProp->GetSpecularLODFade();
			} else {
				fSpecularFade = 1.0f;
			}
			FNVD_INLINED_LOCAL(NiColorA, kDirection, std::nullopt, std::nullopt);
			kDirection.r = kDir.x;
			kDirection.g = kDir.y;
			kDirection.b = kDir.z;
			kDirection.a = fSpecularFade;
			SetShaderConstant(static_cast<int32_t>(auiLightIndex + 19), kDirection);
		}
		if (!BSShaderManager::bSeparateAmbientLight || bAmbientLight) {
			float fDimmer = pLight->m_fDimmer;
			if (!BSShaderManager::bHDR) {
				fDimmer = BSMIN(1.0f, fDimmer);
			}
			kColor.r = pLight->m_kAmb.r * fDimmer;
			kColor.g = pLight->m_kAmb.g * fDimmer;
			kColor.b = fDimmer * pLight->m_kAmb.b;
			// Lifts a too dark ambient colour to the minimum luminance.
			float fMinAmbient = BSShaderManager::fMinAmbient;
			if (fMinAmbient > 0.0f) {
				float fLuminance =
					kColor.r * 0.33000001311302185 + 0.3400000035762787 * kColor.g + kColor.b + 0.33000001311302185;
				if (!(fLuminance <= 0.0f)) {
					float fScale;
					if (fLuminance > fMinAmbient) {
						fScale = 1.0f;
					} else {
						fScale = fMinAmbient / fLuminance;
					}
					kColor.r = (kColor.r + 0.10000000149011612) * fScale;
					kColor.g = (kColor.g + 0.10000000149011612) * fScale;
					kColor.b = (kColor.b + 0.10000000149011612) * fScale;
				} else {
					kColor.r = fMinAmbient;
					kColor.g = fMinAmbient;
					kColor.b = fMinAmbient;
				}
			}
			kColor.r = kColor.r * afForcedDarkness;
			kColor.g = kColor.g * afForcedDarkness;
			kColor.b = kColor.b * afForcedDarkness;
			pShaderLightConstants[0].x = kColor.r;
			pShaderLightConstants[0].y = kColor.g;
			pShaderLightConstants[0].z = kColor.b;
			pShaderLightConstants[0].w = 1.0f;
		}
	}

	// The light's diffuse colour.
	float fDimmer = pLight->m_fDimmer;
	if (!BSShaderManager::bHDR && fDimmer > 1.0) {
		fDimmer = 1.0f;
	}
	float fLODDimmer = apLight->fLODDimmer;
	kColor.r = pLight->m_kDiff.r * fDimmer;
	kColor.g = pLight->m_kDiff.g * fDimmer;
	kColor.b = fDimmer * pLight->m_kDiff.b;
	kColor.r = kColor.r * afForcedDarkness;
	kColor.g = kColor.g * afForcedDarkness;
	kColor.b = kColor.b * afForcedDarkness;
	kColor.r = kColor.r * fLODDimmer;
	kColor.g = fLODDimmer * kColor.g;
	kColor.b = fLODDimmer * kColor.b;
	if (bPointLight) {
		if (afForcedDarkness < 1.0) {
			kColor = NiColor_BLACK;
		}
	} else if (BSShaderManager::bHDR && !BSShaderManager::bInterior) {
		kColor *= BSShaderManager::fSunlightDimmer;
	}
	float fShadowLODDimmer = apLight->fShadowLODDimmer;
	if (static_cast<uint16_t>(BSShaderManager::eCurrentPass - 340) <= 35u && pShaderProp) {
		kColor *= pShaderProp->GetSpecularLODFade();
	}
	pShaderLightConstants[auiLightIndex + 1].x = kColor.r;
	pShaderLightConstants[auiLightIndex + 1].y = kColor.g;
	pShaderLightConstants[auiLightIndex + 1].z = kColor.b;
	pShaderLightConstants[auiLightIndex + 1].w = fShadowLODDimmer;
	if (bPointLight) {
		pShaderLightConstants[auiLightIndex + 19].x = pLight->m_kSpec.r;
		pShaderLightConstants[auiLightIndex + 19].y = 0.0f;
		pShaderLightConstants[auiLightIndex + 19].z = 0.0f;
		pShaderLightConstants[auiLightIndex + 19].w = 1.0f;
	}
	bLightIsPoint = bPointLight;
	eLightType[auiLightIndex] = bPointLight ? SLP_LT_POINT : SLP_LT_DIR;
}

void BSShaderLightingProperty::SetLight(uint32_t auiLightIndex, ShadowSceneLight* apLight, float afForcedDarkness) {
	if (BSShaderManager::pCurrentRenderPass->pGeometry->GetShader()->iShaderType == 17) {
		if (apLight->bPointLight) {
			BSShaderLightingProperty::SetPointLight3x(auiLightIndex, apLight, afForcedDarkness);
		} else {
			BSShaderLightingProperty::SetDirectionalLight3x(auiLightIndex, apLight, afForcedDarkness);
		}
	} else {
		BSShaderLightingProperty::SetLight1x2x(auiLightIndex, apLight, afForcedDarkness);
	}
}

void BSShaderLightingProperty::SetPointLight3x(uint32_t auiLightIndex, ShadowSceneLight* apLight,
	float afForcedDarkness) {
	if (auiLightIndex < 20) {
		if (apLight) {
			float r;
			float v6;
			float v7;
			FNVD_INLINED_LOCAL(NiColorA, v8, std::nullopt, std::nullopt);
			FNVD_INLINED_LOCAL(NiColorA, kdata, std::nullopt, std::nullopt);
			NiLight* m_pObject = apLight->spLight.m_pObject;
			// MAYBE-INLINED 0xB711A9: a write opening NiColorA's uses: may be its inlined constructor
			kdata.r = m_pObject->m_kWorld.m_Translate.x;
			// MAYBE-INLINED 0xB711BB: a write opening NiColorA's uses: may be its inlined constructor
			kdata.g = m_pObject->m_kWorld.m_Translate.y;
			// MAYBE-INLINED 0xB711CD: a write opening NiColorA's uses: may be its inlined constructor
			kdata.b = m_pObject->m_kWorld.m_Translate.z;
			// MAYBE-INLINED 0xB711D9: a write opening NiColorA's uses: may be its inlined constructor
			kdata.a = m_pObject->m_kSpec.r;
			float g = m_pObject->m_kSpec.g;
			float b = m_pObject->m_kSpec.b;
			if (!(afForcedDarkness < 1.0f)) {
				float fdimmer = m_pObject->m_fDimmer;
				if (!BSShaderManager::bHDR) {
					fdimmer = BSMIN(1.0f, fdimmer);
				}
				float iNumLights = fdimmer * afForcedDarkness;
				float v9 = m_pObject->m_kDiff.g;
				float v10 = m_pObject->m_kDiff.b;
				float iNumLightsa = iNumLights * apLight->fLODDimmer;
				float v11 = m_pObject->m_kDiff.r * iNumLightsa;
				r = v11;
				float v13 = v9 * iNumLightsa;
				v6 = v13;
				float v15 = iNumLightsa * v10;
				v7 = v15;
			} else {
				r = NiColorA_BLACK.r;
				v6 = NiColorA_BLACK.g;
				v7 = NiColorA_BLACK.b;
			}
			v8.r = r;
			v8.g = v6;
			v8.b = v7;
			v8.a = apLight->fShadowLODDimmer;
			Lighting30Shader::SetLight(static_cast<int32_t>(auiLightIndex), kdata, v8);
		} else {
			Lighting30Shader::SetLightColor(static_cast<int32_t>(auiLightIndex), NiColorA_BLACK);
		}
	}
}

NiObject* BSShaderLightingProperty::CreateObject() {
	void* pMemory = NiMemObject::operator new(sizeof(BSShaderLightingProperty));
	if (!pMemory)
		return nullptr;
	return ::new (pMemory) BSShaderLightingProperty;
}

// Clears the light types and walks the light colours (register 1 on) and their reset colours (register 19 on) to the
// ninth register.
void BSShaderLightingProperty::ResetLights(int32_t aiStartIndex) {
	if (!(resetColor_guard & 1)) {
		resetColor_guard |= 1;
		resetColor.r = 0.001f;
		resetColor.g = 0.0f;
		resetColor.b = 0.0f;
		resetColor.a = 0.0f;
	}
	if (aiStartIndex < 8) {
		for (int32_t i = aiStartIndex; i < 8; ++i) {
			eLightType[i] = SLP_LT_NONE;
		}
		for (int32_t i = aiStartIndex + 1; i < 9; ++i) {
			pShaderLightConstants[i].x = NiColorA_BLACK.r;
			pShaderLightConstants[i].y = NiColorA_BLACK.g;
			pShaderLightConstants[i].z = NiColorA_BLACK.b;
			pShaderLightConstants[i].w = NiColorA_BLACK.a;
			pShaderLightConstants[i + 18].x = resetColor.r;
			pShaderLightConstants[i + 18].y = resetColor.g;
			pShaderLightConstants[i + 18].z = resetColor.b;
			pShaderLightConstants[i + 18].w = resetColor.a;
		}
	}
}
