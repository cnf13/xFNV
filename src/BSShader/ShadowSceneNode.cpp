#include "BSShader/ShadowSceneNode.h"

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSCompoundFrustum.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSMultiBound.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/BSMultiBoundShape.h"
#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortal.h"
#include "BSMain/BSPortalGraph.h"
#include "BSMain/BSPortalSharedNode.h"
#include "BSShader/BSCubeMapCamera.h"
#include "BSShader/BSFogProperty.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderLightingProperty.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/MTRenderingSystem.h"
#include "BSShader/ShadowSceneLight.h"
#include "BSShader/TallGrassShader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSNiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDirectionalLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustumPlanes.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriBasedGeom.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// GetAverageLightPos's function-local statics: each gathered light's blend scale and attenuation (the same value),
// and its offset from the target. Only the NiPoint3 array has a guard; its constructors leave the points undefined,
// so initialising it sets guard bit 0 and nothing else.
using LightWeightArray = float[100];
static LightWeightArray pfscale{};
static LightWeightArray pfatten{};
using LightOffsetArray = NiPoint3[100];
static LightOffsetArray plightVecA{};
static uint32_t plightVecA_guard;
NiRTTI ShadowSceneNode::ms_RTTI("ShadowSceneNode", &NiNode::ms_RTTI);
BSSpinLock ShadowSceneNode::SSNLightQueueCrit;
BSSpinLock ShadowSceneNode::SSNObjectQueueCrit;

// The enabled passes while BSShaderManager::bLiteBrite is set.
constexpr uint32_t kLiteBriteEnabledPasses = 4;

// The rendered texture type AddQueuedLight borrows for a shadow map (IDA's TEX_SHADOWS_FIRST).
constexpr BSTextureManager::eTextureType kShadowMapTextureType = BSTextureManager::TEX_SHADOWS_SHADOWMAP;

ShadowSceneNode::ShadowSceneNode()
	: NiNode(0), kShadowLightActiveFence(nullptr), kShadowLightFadingInFence(nullptr),
	  kShadowLightActiveFenceObject(nullptr), kShadowLightFadingInFenceObject(nullptr), pSunLight(nullptr),
	  pWindowReflectionEnvMap(nullptr), spCubeMapCamera(nullptr), iCurrentCubeMapFace(0), bDisableLightUpdate(false),
	  bWireframe(false), spFog(nullptr), bVisibleUnboundSpace(true), pBSPortalGraph(nullptr),
	  kLightingOffset(NiPoint3_ZERO), kEyePosition(NiPoint3_ZERO), bAllowLightRemoveQueues(true) {
	SetName(NiFixedString("shadow scene node"));
	lLightList.RemoveAll();
	lShadowLightList.RemoveAll();
	kShadowLightActiveFenceObject = new ShadowSceneLight();
	kShadowLightFadingInFenceObject = new ShadowSceneLight();

	NiDirectionalLight* pLight = new NiDirectionalLight();
	pLight->m_kAmb = NiColor_BLACK;
	++pLight->m_uiRevID;
	pLight->m_kDiff = NiColor_BLACK;
	++pLight->m_uiRevID;
	pSunLight = new ShadowSceneLight();
	pSunLight->SetLight(pLight);

	m_uFlags = static_cast<NiAVObject::NiFlags>(m_uFlags | NiAVObject::ALWAYS_DRAW);
	if (!m_kWorldBound) {
		SetBound(static_cast<BSNiBound*>(MemoryManager_Singleton.Allocate(sizeof(NiBound))));
		m_kWorldBound->m_kCenter = NiPoint3_ZERO;
		m_kWorldBound->m_fRadius = 0.0f;
	}
	m_kWorldBound->m_fRadius = 1.0f;
	m_uFlags = static_cast<NiAVObject::NiFlags>(m_uFlags | NiAVObject::FIXED_BOUND);

	spFog = new BSFogProperty();
	spFog->IncRefCount();
	spFog->fStartDistance = 500000.0f;
	spFog->fEndDistance = 500000.0f;
	spFog->fStartWaterDistance = 500000.0f;
	spFog->fEndWaterDistance = 500000.0f;
	spFog->m_kColor = NiColor(0.5f, 0.5f, 0.5f);
	spFog->kWaterColor = NiColor(0.5f, 0.5f, 0.5f);
}

ShadowSceneNode::~ShadowSceneNode() {
	if (BSShaderManager::pShadowSceneNode[cSceneGraphIndex] == this) {
		BSShaderManager::SetShadowSceneNode(cSceneGraphIndex, nullptr);
	}
	if (BSShaderManager::GetAccumulator()) {
		if (BSShaderManager::GetAccumulator()->pActiveShadowSceneNode == this) {
			BSShaderManager::GetAccumulator()->pActiveShadowSceneNode = nullptr;
		}
	}
	RemoveAllLights();
	kShadowLightActiveFenceObject = nullptr;
	kShadowLightFadingInFenceObject = nullptr;
	delete pSunLight;
	if (pWindowReflectionEnvMap) {
		BSShaderManager::pTextureManager
			->ReturnRenderedTexture(static_cast<BSRenderedTexture*>(pWindowReflectionEnvMap));
		pWindowReflectionEnvMap->DecRefCount();
		pWindowReflectionEnvMap = nullptr;
	}
	lLightQueueAdd.RemoveAll();
	lLightQueueRemove.RemoveAll();
	lLightQueueUpdate.RemoveAll();
	pBSPortalGraph = nullptr;
	if (spCubeMapCamera) {
		spCubeMapCamera->DecRefCount();
		spCubeMapCamera = nullptr;
	}
	if (spFog) {
		spFog->DecRefCount();
	}
}

void ShadowSceneNode::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	NiNode::GetViewerStrings(apStrings);
	char* ViewerString = NiGetViewerString(ShadowSceneNode::ms_RTTI.GetName());
	uint32_t m_usSize = apStrings->m_usSize;
	uint32_t m_usMaxSize = apStrings->m_usMaxSize;
	char* v23 = ViewerString;
	if (m_usSize >= m_usMaxSize) {
		apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(m_usSize, v23);
	char* ViewerString_4 = NiGetViewerString("light count", static_cast<uint16_t>(lLightList.m_uiCount));
	uint32_t v8 = apStrings->m_usSize;
	uint32_t v9 = apStrings->m_usMaxSize;
	v23 = ViewerString_4;
	if (v8 >= v9) {
		apStrings->SetSize(v8 + apStrings->m_usGrowBy);
	}
	apStrings->SetAt(v8, v23);
	// The exe counts the shadow-casting lights in the stack slot that held the string above.
	uint32_t m_uiCount = lLightList.m_uiCount;
	int32_t v11 = 0;
	int32_t iShadowLightCount = 0;
	if (static_cast<uint16_t>(m_uiCount)) {
		do {
			ShadowSceneLight* m_pObject;
			int32_t v12 = static_cast<uint16_t>(v11);
			if (static_cast<uint16_t>(v11) < m_uiCount) {
				NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
				NiPointer<ShadowSceneLight>* p_m_element = &m_pkHead->m_element;
				NiTListItem<NiPointer<ShadowSceneLight>>* m_pkNext = m_pkHead->m_pkNext;
				m_pObject = p_m_element->m_pObject;
				if (static_cast<uint16_t>(v11)) {
					do {
						--v12;
						NiPointer<ShadowSceneLight>* v17 = &m_pkNext->m_element;
						m_pkNext = m_pkNext->m_pkNext;
						m_pObject = v17->m_pObject;
					} while (v12);
				}
			} else {
				m_pObject = 0;
			}
			if (m_pObject->bCastShadow) {
				++iShadowLightCount;
			}
			++v11;
		} while (v11 < static_cast<uint16_t>(m_uiCount));
	}
	char* ViewerString_5 = NiGetViewerString("shadow light count", iShadowLightCount);
	uint32_t v19 = apStrings->m_usSize;
	if (v19 >= apStrings->m_usMaxSize) {
		apStrings->SetSize(v19 + apStrings->m_usGrowBy);
	}
	if (v19 < apStrings->m_usSize) {
		if (ViewerString_5) {
			char** m_pBase = apStrings->m_pBase;
			if (!m_pBase[v19]) {
				++apStrings->m_usESize;
				m_pBase[v19] = ViewerString_5;
				return;
			}
		} else {
			if (apStrings->m_pBase[v19]) {
				--apStrings->m_usESize;
			}
		}
	} else {
		apStrings->m_usSize = v19 + 1;
		if (ViewerString_5) {
			char** v21 = apStrings->m_pBase;
			++apStrings->m_usESize;
			v21[v19] = ViewerString_5;
			return;
		}
	}
	apStrings->m_pBase[v19] = ViewerString_5;
}

// With a portal graph: the always-drawn objects and the portal node root are hidden from the normal cull and drawn
// afterwards; in normal culling only the rooms seen this frame and the children past the first four are culled, and
// each room's portals add their shared geometry.
void ShadowSceneNode::OnVisible(NiCullingProcess& arCuller) {
	BSCullingProcess& rCuller = static_cast<BSCullingProcess&>(arCuller);
	PreOnVisible(rCuller);

	// The exe holds a reference on the accumulator for the call, and a second one while it is the only holder.
	BSShaderAccumulator* pAccumulator = BSShaderManager::GetAccumulator();
	if (pAccumulator)
		NiIncRefCount(pAccumulator);
	if (pAccumulator->m_uiRefCount == 1)
		NiIncRefCount(pAccumulator);

	if (!pBSPortalGraph) {
		NiNode::OnVisible(arCuller);
		NiDecRefCount(pAccumulator);
		return;
	}

	ScrapHeap* pScrapHeap = MemoryManager_Singleton.GetThreadScrapHeap();
	uint8_t* pucWasCulled = nullptr;
	if (pBSPortalGraph->GetAlwaysRenderArrayCount()) {
		pucWasCulled = static_cast<uint8_t*>(pScrapHeap->Allocate(pBSPortalGraph->GetAlwaysRenderArrayCount(),
			ScrapHeap::ALIGNMENT));
		std::memset(pucWasCulled, 0, pBSPortalGraph->GetAlwaysRenderArrayCount());
	}
	for (uint32_t i = 0; i < pBSPortalGraph->GetAlwaysRenderArrayCount(); i++) {
		NiAVObject* pObject = pBSPortalGraph->GetAlwaysRenderAt(i);
		if (pObject) {
			pucWasCulled[i] = static_cast<uint8_t>(pObject->m_uFlags & NiAVObject::APP_CULLED);
			pObject->m_uFlags = static_cast<NiAVObject::NiFlags>(pObject->m_uFlags | NiAVObject::APP_CULLED);
		}
	}
	NiNode* pPortalNodeRoot = pBSPortalGraph->spPortalNodeRoot.m_pObject;
	if (pPortalNodeRoot)
		pPortalNodeRoot->m_uFlags =
			static_cast<NiAVObject::NiFlags>(pPortalNodeRoot->m_uFlags | NiAVObject::APP_CULLED);

	if (!bVisibleUnboundSpace && rCuller.kCullMode != BSCullingProcess::BSCP_CULL_ALLPASS &&
		rCuller.kCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS) {
		for (auto* pRoomItem = pBSPortalGraph->lMultiBoundRoomAccumList.m_pkHead; pRoomItem;) {
			BSMultiBoundRoom* pRoom = pRoomItem->m_element.m_pObject;
			pRoomItem = pRoomItem->m_pkNext;
			if (!pRoom)
				continue;
			pRoom->OnVisible(arCuller);
			if (pRoom->kPortalList.m_uiCount) {
				for (NiTListItem<BSPortal*>* pPortalItem = pRoom->kPortalList.m_pkHead; pPortalItem;) {
					BSPortal* pPortal = pPortalItem->m_element;
					pPortalItem = pPortalItem->m_pkNext;
					if (pPortal && rCuller.AddShared(pPortal->spSharedGeometry)) {
						BuildSharedCompoundFrustum(rCuller.m_pkCamera, pPortal);
						pPortal->spSharedGeometry->OnVisible(arCuller);
					}
				}
			}
			rCuller.pCompoundFrustum = nullptr;
		}
		for (uint32_t i = 4; i < m_kChildren.m_usSize; i++) {
			NiAVObject* pChild = m_kChildren.m_pBase[i].m_pObject;
			if (pChild)
				pChild->Cull(arCuller);
		}
	} else {
		NiNode::OnVisible(arCuller);
		// Every room in these modes, else the rooms seen this frame; a null room stands for the graph's own portals.
		BSCullingProcess::BSCPCullingType eCullMode = rCuller.kCullMode;
		NiTListItem<NiPointer<BSMultiBoundRoom>>* pRoomItem = nullptr;
		NiTListItem<NiPointer<BSMultiBoundRoom>>* pAccumItem = nullptr;
		if (eCullMode == BSCullingProcess::BSCP_CULL_ALLPASS ||
			eCullMode == BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS)
			pRoomItem = pBSPortalGraph->lMultiBoundRoomList.m_pkHead;
		else
			pAccumItem = pBSPortalGraph->lMultiBoundRoomAccumList.m_pkHead;
		alignas(BSCompoundFrustum) uint8_t aucFrustum[sizeof(BSCompoundFrustum)];
		BSCompoundFrustum* pFrustum = ::new (aucFrustum) BSCompoundFrustum;
		auto CullPortals = [&](BSMultiBoundRoom* pRoom) {
			NiTListBase<BSPortal*>* pPortals = pRoom ? &pRoom->kPortalList : &pBSPortalGraph->lPortalList;
			if (pPortals->m_uiCount) {
				for (NiTListItem<BSPortal*>* pPortalItem = pPortals->m_pkHead; pPortalItem;) {
					BSPortal* pPortal = pPortalItem->m_element;
					pPortalItem = pPortalItem->m_pkNext;
					if (pPortal && rCuller.AddShared(pPortal->spSharedGeometry))
						pPortal->spSharedGeometry->OnVisible(arCuller);
				}
			}
			rCuller.pCompoundFrustum = nullptr;
		};
		while (pRoomItem) {
			BSMultiBoundRoom* pRoom = pRoomItem->m_element.m_pObject;
			pRoomItem = pRoomItem->m_pkNext;
			CullPortals(pRoom);
		}
		while (pAccumItem) {
			BSMultiBoundRoom* pRoom = pAccumItem->m_element.m_pObject;
			pAccumItem = pAccumItem->m_pkNext;
			CullPortals(pRoom);
		}
		pFrustum->~BSCompoundFrustum();
	}

	for (uint32_t i = 0; i < pBSPortalGraph->GetAlwaysRenderArrayCount(); i++) {
		NiAVObject* pObject = pBSPortalGraph->GetAlwaysRenderAt(i);
		if (pObject && !pucWasCulled[i]) {
			pObject->m_uFlags = static_cast<NiAVObject::NiFlags>(pObject->m_uFlags & ~NiAVObject::APP_CULLED);
			pObject->Cull(arCuller);
		}
	}
	pPortalNodeRoot = pBSPortalGraph->spPortalNodeRoot.m_pObject;
	if (pPortalNodeRoot)
		pPortalNodeRoot->m_uFlags =
			static_cast<NiAVObject::NiFlags>(pPortalNodeRoot->m_uFlags & ~NiAVObject::APP_CULLED);
	pScrapHeap->Deallocate(pucWasCulled);
	NiDecRefCount(pAccumulator);
}

void ShadowSceneNode::RemoveObjectRecurse(NiNode* apNode) {
	for (uint32_t i = 0; i < apNode->m_kChildren.m_usSize; ++i) {
		NiPointer<NiAVObject>* m_pBase = apNode->m_kChildren.m_pBase;
		NiAVObject* child = m_pBase[i].m_pObject;
		if (child) {
			if (m_pBase[i].m_pObject->IsTriBasedGeom()) {
				ShadowSceneNode::RemoveLightsFromGeom(static_cast<NiTriBasedGeom*>(child));
			} else {
				NiNode* node = child->IsNode();
				if (node) {
					RemoveObjectRecurse(node);
				}
			}
		}
	}
}

// The exe jumps to the recursion with its own arguments.
void ShadowSceneNode::FindAndCullLight(NiNode* apNode, int32_t abCull) {
	if (apNode)
		FindAndCullLightRecurse(apNode, abCull);
}

void ShadowSceneNode::FindAndCullLightRecurse(NiNode* apNode, int32_t abCull) {
	uint32_t m_usSize = apNode->m_kChildren.m_usSize;
	for (uint32_t i = 0; i < m_usSize; ++i) {
		NiAVObject* pchlight__pchnode = apNode->m_kChildren.m_pBase[i].m_pObject;
		if (pchlight__pchnode) {
			const NiRTTI* v7 = pchlight__pchnode->GetRTTI();
			if (v7) {
				while (v7 != &NiLight::ms_RTTI) {
					v7 = v7->GetBaseRTTI();
					if (!v7) {
						goto LABEL_6;
					}
				}
				if (static_cast<uint8_t>(abCull)) {
					RemoveLight(static_cast<NiLight*>(pchlight__pchnode));
				} else {
					AddLight(static_cast<NiLight*>(pchlight__pchnode), 1);
				}
			} else {
			LABEL_6:;
				if (pchlight__pchnode->IsNode()) {
					FindAndCullLightRecurse(static_cast<NiNode*>(pchlight__pchnode), abCull);
				}
			}
		}
	}
}

ShadowSceneLight* ShadowSceneNode::GetLight(NiLight* apLight) {
	ShadowSceneLight* result;
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
	if (!m_pkHead) {
		return 0;
	}
	while (1) {
		result = m_pkHead->m_element.m_pObject;
		m_pkHead = m_pkHead->m_pkNext;
		if (result) {
			if (result->spLight.m_pObject == apLight) {
				break;
			}
		}
		if (!m_pkHead) {
			return 0;
		}
	}
	return result;
}

void ShadowSceneNode::UpdateLighting(bool abForceFull, bool abAddLights) {
	if (abForceFull) {
		NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
		while (m_pkHead) {
			ShadowSceneLight* m_pObject = m_pkHead->m_element.m_pObject;
			m_pkHead = m_pkHead->m_pkNext;
			if (m_pObject) {
				UpdateLight(m_pObject);
			}
		}
		if (!abAddLights) {
			RemoveAllLights();
		}
	}
}

void ShadowSceneNode::ProcessQueuedLights() {
	SSNLightQueueCrit.Lock(nullptr);

	for (ShadowSceneLightItem* pItem = lLightQueueAdd.m_pkHead; pItem;) {
		ShadowSceneLight* pLight = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (pLight) {
			if (pLight->bCastShadow)
				AddQueuedShadowLight(pLight);
			else
				AddQueuedLight(pLight);
		}
	}

	for (ShadowSceneLightItem* pItem = lLightQueueUpdate.m_pkHead; pItem;) {
		ShadowSceneLight* pLight = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (pLight) {
			if (pLight->bDynamicLight)
				UpdateQueuedDynamicLight(pLight);
			else
				UpdateQueuedLight(pLight);
		}
	}

	// A shadow-casting light gives its shadow map back before it goes.
	for (ShadowSceneLightItem* pItem = lLightQueueRemove.m_pkHead; pItem;) {
		ShadowSceneLight* pLight = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (!pLight)
			continue;
		if (pLight->bCastShadow) {
			NiPointer<ShadowSceneLight> spLight(pLight);
			if (spLight) {
				BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(spLight->spDynamicShadowMap);
				spLight->spDynamicShadowMap = nullptr;
				RemoveQueuedLight(spLight);
			}
		} else {
			NiPointer<ShadowSceneLight> spLight(pLight);
			RemoveQueuedLight(spLight);
		}
	}

	// Nodes smaller than 150 units take one light list for their whole subtree.
	SSNObjectQueueCrit.Lock(nullptr);
	for (NiTListItem<NiPointer<NiAVObject>>* pItem = lObjectQueueUpdateActiveOnly.m_pkHead; pItem;) {
		NiAVObject* pObject = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (!pObject)
			continue;
		bool bSingleLightList = false;
		if (pObject->IsNode()) {
			const NiBound* pWorldBound = pObject->m_kWorldBound;
			if (!pWorldBound)
				pWorldBound = &NiAVObject::NullBoundS;
			bSingleLightList = pWorldBound->m_fRadius < 150.0f;
		}
		UpdateQueuedObjectLightingStart(pObject, this, true, bSingleLightList);
	}
	for (NiTListItem<NiPointer<NiAVObject>>* pItem = lObjectQueueUpdate.m_pkHead; pItem;) {
		NiAVObject* pObject = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (!pObject)
			continue;
		bool bSingleLightList = false;
		if (pObject->IsNode()) {
			const NiBound* pWorldBound = pObject->m_kWorldBound;
			if (!pWorldBound)
				pWorldBound = &NiAVObject::NullBoundS;
			bSingleLightList = pWorldBound->m_fRadius < 150.0f;
		}
		UpdateQueuedObjectLightingStart(pObject, this, false, bSingleLightList);
	}

	if (pBSPortalGraph)
		pBSPortalGraph->ProcessQueue();
	lLightQueueAdd.RemoveAll();
	lLightQueueRemove.RemoveAll();
	lLightQueueUpdate.RemoveAll();
	lObjectQueueUpdate.RemoveAll();
	lObjectQueueUpdateActiveOnly.RemoveAll();
	bAllowLightRemoveQueues = true;
	if (--SSNObjectQueueCrit.uiLockCount == 0)
		SSNObjectQueueCrit.uiOwningThread = 0;
	if (--SSNLightQueueCrit.uiLockCount == 0)
		SSNLightQueueCrit.uiOwningThread = 0;
}

void ShadowSceneNode::UpdateObjectLighting(NiAVObject* apObject, bool abActiveLightsOnly) {
	if (!apObject)
		return;

	SSNObjectQueueCrit.Lock(nullptr);
	if (abActiveLightsOnly)
		lObjectQueueUpdateActiveOnly.AddTail(NiPointer<NiAVObject>(apObject));
	else
		lObjectQueueUpdate.AddTail(NiPointer<NiAVObject>(apObject));
	if (--SSNObjectQueueCrit.uiLockCount == 0)
		SSNObjectQueueCrit.uiOwningThread = 0;
}

void ShadowSceneNode::SetPortalGraph(BSPortalGraph* apBSPortalGraph) {
	if (apBSPortalGraph != pBSPortalGraph) {
		SSNLightQueueCrit.Lock(nullptr);
		SSNObjectQueueCrit.Lock(nullptr);
		bAllowLightRemoveQueues = false;
		lLightQueueAdd.RemoveAll();
		lLightQueueRemove.RemoveAll();
		lLightQueueUpdate.RemoveAll();
		lObjectQueueUpdate.RemoveAll();
		lObjectQueueUpdateActiveOnly.RemoveAll();

		// Each room of the old graph leaves its lights' room lists, and each of its portals its lights' portal lists.
		if (pBSPortalGraph) {
			for (NiTListItem<NiPointer<BSMultiBoundRoom>>* pRoomItem = pBSPortalGraph->lMultiBoundRoomList.m_pkHead;
				pRoomItem;) {
				BSMultiBoundRoom* pRoom = pRoomItem->m_element.m_pObject;
				pRoomItem = pRoomItem->m_pkNext;
				if (!pRoom)
					continue;

				for (uint32_t i = 0; i < pRoom->kLights.iSize; i++) {
					BSSimpleArray<BSMultiBoundNode*>& kRooms = pRoom->kLights.pBuffer[i]->kMultiboundRooms;
					uint32_t uiCount = kRooms.iSize;
					BSMultiBoundNode** ppRooms = kRooms.pBuffer;
					for (uint32_t j = 0; j < uiCount; j++) {
						if (ppRooms && ppRooms[j] == pRoom) {
							kRooms.Remove(j, true);
							break;
						}
					}
				}
				pRoom->kLights.Clear(true);

				for (NiTListItem<BSPortal*>* pPortalItem = pRoom->kPortalList.m_pkHead; pPortalItem;) {
					BSPortal* pPortal = pPortalItem->m_element;
					pPortalItem = pPortalItem->m_pkNext;
					if (!pPortal)
						continue;

					int32_t iLightCount = pPortal->spSharedGeometry->kLights.iSize;
					for (int32_t i = 0; i < iLightCount; i++) {
						BSSimpleArray<BSPortal*>& kPortals = pPortal->spSharedGeometry->kLights.pBuffer[i]->kPortals;
						uint32_t uiCount = kPortals.iSize;
						BSPortal** ppPortals = kPortals.pBuffer;
						for (uint32_t j = 0; j < uiCount; j++) {
							if (!ppPortals || ppPortals[j] != pPortal)
								continue;
							// The engine's inlined Remove(j, 1). Its shrinking copy moves iSize - 1 items after the
							// gap rather than iSize - j - 1.
							if (kPortals._ShouldShrink()) {
								uint32_t uiNewSize = kPortals._GetNewSizeForShrink();
								BSPortal** ppNewPortals = kPortals._Allocate(uiNewSize);
								kPortals._MoveItems(ppNewPortals, kPortals.pBuffer, j);
								kPortals._MoveItems(&ppNewPortals[j], &kPortals.pBuffer[j + 1], kPortals.iSize - 1);
								kPortals._Deallocate(kPortals.pBuffer);
								kPortals.pBuffer = ppNewPortals;
								kPortals.iReservedSize = uiNewSize;
							} else {
								kPortals._MoveItems(&kPortals.pBuffer[j], &kPortals.pBuffer[j + 1], uiCount - j - 1);
							}
							kPortals.iSize--;
							break;
						}
					}
					pPortal->spSharedGeometry->kLights.Clear(true);
				}
			}
		}

		bAllowLightRemoveQueues = true;
		if (--SSNLightQueueCrit.uiLockCount == 0)
			SSNLightQueueCrit.uiOwningThread = 0;
		if (--SSNObjectQueueCrit.uiLockCount == 0)
			SSNObjectQueueCrit.uiOwningThread = 0;
		RemoveAllLights();
	}
	pBSPortalGraph = apBSPortalGraph;
}

void ShadowSceneNode::PreOnVisible(NiCullingProcess& arCuller) {
	static_cast<BSCullingProcess&>(arCuller).ClearSharedMap();
	NiCamera* pCamera = arCuller.m_pkCamera;

	// Lights whose NiLight is gone (only the scene light still holds it) leave the scene; the rest are culled.
	if (!bDisableLightUpdate) {
		for (ShadowSceneLightItem* pItem = lLightList.m_pkHead; pItem;) {
			ShadowSceneLightItem* kIter = pItem;
			ShadowSceneLight* pLight = pItem->m_element;
			pItem = pItem->m_pkNext;
			if (!pLight)
				continue;
			NiLight* pNiLight = pLight->spLight.m_pObject;
			if (pNiLight && pNiLight->m_uiRefCount != 1) {
				if (pLight->bDynamicLight)
					UpdateLight(pLight);
				if (!pLight->bCastShadow || !BSShaderManager::GetShadows())
					pLight->TestFrustumCull(arCuller);
			} else if (pLight->pPortalGraph == pBSPortalGraph) {
				NiPointer<ShadowSceneLight> spLight(pLight);
				RemoveQueuedLight(spLight, kIter);
			} else if (pLight->bCastShadow) {
				NiPointer<ShadowSceneLight> spLight(pLight);
				lShadowLightList.Remove(spLight);
			} else {
				lLightList.RemovePos(kIter);
			}
		}
	}

	BSShaderManager::cSceneGraph = cSceneGraphIndex;
	BSShaderManager::spCamera = pCamera;
	if (!BSShaderManager::eShaderLevel) {
		NiNode::OnVisible(arCuller);
		return;
	}

	if (BSShaderManager::bLiteBrite == 1) {
		if (BSShaderManager::iLastEnabledPasses == -1)
			BSShaderManager::iLastEnabledPasses = BSShaderManager::iEnabledPasses;
		BSShaderManager::iEnabledPasses = kLiteBriteEnabledPasses;
	} else if (BSShaderManager::iLastEnabledPasses != -1) {
		BSShaderManager::iEnabledPasses = BSShaderManager::iLastEnabledPasses;
		BSShaderManager::iLastEnabledPasses = -1;
	}

	NiPoint4* pConstants = BSShaderLightingProperty::pShaderLightConstants;
	const NiTransform& kWorld = pCamera->m_kWorld;
	NiPoint4& kEyePosition = pConstants[BSShaderLightingProperty::SLP_SC_EYEPOSITION];
	kEyePosition.x = kWorld.m_Translate.x;
	kEyePosition.y = kWorld.m_Translate.y;
	kEyePosition.z = kWorld.m_Translate.z;
	kEyePosition.w = 0.0f;
	NiPoint4& kEyeDirection = pConstants[BSShaderLightingProperty::SLP_SC_EYEDIRECTION];
	kEyeDirection.x = kWorld.m_Rotate.m_pEntry[0][0];
	kEyeDirection.y = kWorld.m_Rotate.m_pEntry[1][0];
	kEyeDirection.z = kWorld.m_Rotate.m_pEntry[2][0];
	kEyeDirection.w = 0.0f;
	NiPoint4& kEyeRight = pConstants[BSShaderLightingProperty::SLP_SC_EYERIGHT];
	kEyeRight.x = kWorld.m_Rotate.m_pEntry[0][2];
	kEyeRight.y = kWorld.m_Rotate.m_pEntry[1][2];
	kEyeRight.z = kWorld.m_Rotate.m_pEntry[2][2];
	kEyeRight.w = 0.0f;

	BSFogProperty* pFog = spFog;
	if (pFog) {
		float fEndDistance = pFog->fEndDistance;
		float fRange = fEndDistance - pFog->fStartDistance;
		NiPoint4& kFogParams = pConstants[BSShaderLightingProperty::SLP_SC_FOGPARAM];
		kFogParams.x = fEndDistance;
		kFogParams.y = fRange;
		kFogParams.z = pFog->fPower;
		kFogParams.w = 0.0f;
		NiPoint4& kFogColor = pConstants[BSShaderLightingProperty::SLP_SC_FOGCOLOR];
		kFogColor.x = pFog->m_kColor.r;
		kFogColor.y = pFog->m_kColor.g;
		kFogColor.z = pFog->m_kColor.b;
		kFogColor.w = 0.0f;
	}

	TallGrassShader::pGroupedVertexConstants.kGrassWindMagnitude.x =
		(TallGrassShader::fWindMagMax - TallGrassShader::fWindMagMin) * BSShaderManager::fWindMagnitude +
		TallGrassShader::fWindMagMin;
}

ShadowSceneLight* ShadowSceneNode::AddQueuedLight(ShadowSceneLight* apLight) {
	NiLight* pLight = apLight->spLight.m_pObject;
	for (ShadowSceneLightItem* pItem = lLightList.m_pkHead; pItem;) {
		ShadowSceneLight* pSSLight = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (pSSLight && pSSLight->spLight.m_pObject == pLight) {
			pSSLight->bDynamicLight = apLight->bDynamicLight;
			UpdateQueuedLight(pSSLight);
			return pSSLight;
		}
	}

	// The exe builds the NiPointer in apLight's own argument slot, without its constructor's and destructor's null
	// tests (apLight was dereferenced above).
	FNVD_INLINED_LOCAL(NiPointer<ShadowSceneLight>, spLight, std::nullopt, std::nullopt);
	spLight.m_pObject = apLight;
	NiIncRefCount(apLight);
	lLightList.AddTail(spLight);
	NiDecRefCount(apLight);
	if (apLight->bCastShadow) {
		BSRenderedTexture* pShadowMap;
		// The exe tests bCubeShadowMap through a null ShadowSceneLight pointer (it reads the absolute address 0xED),
		// so a shadow-casting light faults here.
		ShadowSceneLight* pNoLight = nullptr;
		if (pNoLight->bCubeShadowMap) {
			pShadowMap = BSShaderManager::pTextureManager->BorrowRenderedTexture(BSShaderManager::pRenderer.m_pObject,
				kShadowMapTextureType, 0, nullptr, 0);
			NiRenderTargetGroup* pGroup = pShadowMap->GetGroup();
			pGroup->AttachDepthStencilBuffer(BSShaderManager::pTextureManager->GetShadowDepthBuffer());
		} else {
			pShadowMap = BSShaderManager::pTextureManager->BorrowFrustumShadowTexture();
		}
		apLight->SetDynamicShadowMap(pShadowMap);
	}
	UpdateQueuedLight(apLight);
	return nullptr;
}

void ShadowSceneNode::RemoveLight(NiLight* apLight) {
	if (!apLight || !bAllowLightRemoveQueues)
		return;

	for (ShadowSceneLightItem* pItem = lLightList.m_pkHead; pItem;) {
		ShadowSceneLight* pSSLight = pItem->m_element;
		pItem = pItem->m_pkNext;
		if (pSSLight && pSSLight->spLight.m_pObject == apLight) {
			NiPointer<ShadowSceneLight> spSSLight(pSSLight);
			RemoveLight(spSSLight);
			return;
		}
	}
}

void ShadowSceneNode::RemoveLight(const NiPointer<ShadowSceneLight>& apSSLight) {
	if ((&apSSLight)->m_pObject) {
		if (bAllowLightRemoveQueues) {
			(&ShadowSceneNode::SSNLightQueueCrit)->Lock(nullptr);
			(&lLightQueueRemove)->AddTail(apSSLight);
			if (!(--ShadowSceneNode::SSNLightQueueCrit.uiLockCount)) {
				ShadowSceneNode::SSNLightQueueCrit.uiOwningThread = (uint32_t)0;
			}
		}
	}
}

void ShadowSceneNode::RemoveQueuedLight(const NiPointer<ShadowSceneLight>& apSSLight) {
	ShadowSceneLight* pSSLight = apSSLight.m_pObject;
	if (pSSLight->pPortalGraph != pBSPortalGraph) {
		if (pSSLight->bCastShadow) {
			NiPointer<ShadowSceneLight> spRemoved = lShadowLightList.Remove(apSSLight);
		} else {
			NiPointer<ShadowSceneLight> spRemoved = lLightList.Remove(apSSLight);
		}
		return;
	}

	// The light's node in its list (the PDB's NiTListIterator).
	ShadowSceneLightItem* pIter = nullptr;
	ShadowSceneLightItem* pItem = pSSLight->bCastShadow ? lShadowLightList.m_pkHead : lLightList.m_pkHead;
	while (pItem) {
		ShadowSceneLightItem* pCurrent = pItem;
		pItem = pItem->m_pkNext;
		if (pSSLight == pCurrent->m_element.m_pObject) {
			pIter = pCurrent;
			break;
		}
	}
	if (pIter)
		RemoveQueuedLight(apSSLight, pIter);
}

void ShadowSceneNode::RemoveQueuedLight(const NiPointer<ShadowSceneLight>& apSSLight, ShadowSceneLightItem*& arIter) {
	if (pBSPortalGraph && pBSPortalGraph->lMultiBoundRoomList.m_uiCount) {
		ShadowSceneLight* pLight = apSSLight.m_pObject;
		for (uint32_t i = 0; i < pLight->kMultiboundRooms.iSize; ++i) {
			BSMultiBoundRoom* pRoom = static_cast<BSMultiBoundRoom*>(pLight->kMultiboundRooms.pBuffer[i]);
			if (pRoom && pRoom->kLights.iSize) {
				int32_t iIndex = pRoom->kLights.Find(apSSLight.m_pObject, 0,
					&BSSimpleArray<ShadowSceneLight*>::DefaultEqualFunctionByAddress);
				if (iIndex != -1)
					pRoom->kLights.Remove(iIndex, true);
			}
			pLight = apSSLight.m_pObject;
		}

		pLight = apSSLight.m_pObject;
		for (uint32_t i = 0; i < pLight->kPortals.iSize; ++i) {
			BSPortal* pPortal = pLight->kPortals.pBuffer[i];
			if (pPortal) {
				BSPortalSharedNode* pNode = pPortal->spSharedGeometry.m_pObject;
				if (pNode->kLights.iSize) {
					// EXE-INLINED 0xB5A930
					uint32_t iSize = pNode->kLights.iSize;
					for (uint32_t j = 0; j < iSize; ++j) {
						ShadowSceneLight** pBuffer = pNode->kLights.pBuffer;
						if (pBuffer && pBuffer[j] == pLight) {
							if (static_cast<int32_t>(j) != -1)
								pNode->kLights.Remove(j, true);
							break;
						}
					}
				}
			}
			pLight = apSSLight.m_pObject;
		}
	}

	ShadowSceneLightList& rList = apSSLight.m_pObject->bCastShadow ? lShadowLightList : lLightList;
	NiPointer<ShadowSceneLight> spRemoved = rList.RemovePos(arIter);
}

void ShadowSceneNode::BuildSharedCompoundFrustum(const NiCamera* apCamera, BSPortal* apPortal) {
	BSCompoundFrustum* pCompoundFrustum;
	BSCompoundFrustum* p_kCompoundFrustum;
	BSCompoundFrustum* v8;
	uint8_t bVisibleUnboundSpace;
	uint8_t v10;
	uint8_t apPortala;
	pCompoundFrustum = apPortal->spSharedGeometry.m_pObject->pCompoundFrustum;
	BSMultiBoundRoom* v4 = apPortal->pMultiBoundRoom[0];
	BSMultiBoundRoom* v5 = apPortal->pMultiBoundRoom[1];
	uint8_t v6 = 1;
	if (v4) {
		apPortala = (uint8_t)(((int32_t)v4->spMultiBound.m_pObject->spShape.m_pObject->kCullResult) == 1);
	} else {
		apPortala = (uint8_t)this->bVisibleUnboundSpace;
	}
	if (v5) {
		bVisibleUnboundSpace = (uint8_t)(((int32_t)v5->spMultiBound.m_pObject->spShape.m_pObject->kCullResult) == 1);
	} else {
		bVisibleUnboundSpace = (uint8_t)this->bVisibleUnboundSpace;
	}
	if (v4) {
		v10 = (uint8_t)(apPortala && v4->pCompoundFrustum);
	} else {
		v10 = (uint8_t)(kCompoundFrustum.iFreeOp == 0);
	}
	if (v5) {
		if ((!bVisibleUnboundSpace) || (!v5->pCompoundFrustum)) {
			v6 = 0;
		}
	} else {
		v6 = (uint8_t)(kCompoundFrustum.iFreeOp == 0);
	}
	if (v4) {
		p_kCompoundFrustum = v4->pCompoundFrustum;
	} else {
		p_kCompoundFrustum = &kCompoundFrustum;
	}
	if (v5) {
		v8 = v5->pCompoundFrustum;
	} else {
		v8 = &kCompoundFrustum;
	}
	pCompoundFrustum->SetCamera(apCamera);
	if (((!apPortala) || p_kCompoundFrustum->iFreeOp) && ((!bVisibleUnboundSpace) || v8->iFreeOp)) {
		if (v10) {
			if (v6) {
				pCompoundFrustum->StartGroupUnion();
				pCompoundFrustum->AddCompoundFrustum(p_kCompoundFrustum);
				pCompoundFrustum->AddCompoundFrustum(v8);
				pCompoundFrustum->EndGroup();
			} else {
				pCompoundFrustum->AddCompoundFrustum(p_kCompoundFrustum);
			}
		} else {
			if (v6) {
				pCompoundFrustum->AddCompoundFrustum(v8);
			}
		}
		pCompoundFrustum->RemoveEmptyGroups();
		pCompoundFrustum->PrethreadOpList();
	}
}

// Culls the multibound's shape once per frame against the culler's planes (not when the culler passes everything or
// ignores multibounds), then tests it against the portal graph's visible occluders until one hides it.
void ShadowSceneNode::UpdateMultiboundVisibility(BSMultiBoundNode* apMultibound, NiPoint3 akViewPoint,
	NiCullingProcess& akCuller) {
	BSMultiBoundShape* pShape = apMultibound->spMultiBound.m_pObject->spShape.m_pObject;
	// The exe reads a BSCullingProcess's cull mode (+0x90), but both callers pass a plain NiCullingProcess local, so
	// it reads their frame past it: TES::UpdateMultiBoundVisibility's camera x (a float's bits),
	// BGSTerrainManager::UpdateMultiBoundVisibility's SEH link (a stack address), neither ALLPASS nor
	// IGNOREMULTIBOUNDS. Here a culler that is not a BSCullingProcess culls as normal.
	BSCullingProcess::BSCPCullingType eCullMode = BSCullingProcess::BSCP_CULL_NORMAL;
	if (akCuller.GetRTTI() == &BSCullingProcess::ms_RTTI) {
		eCullMode = static_cast<BSCullingProcess&>(akCuller).kCullMode;
	}
	if (eCullMode != BSCullingProcess::BSCP_CULL_ALLPASS &&
		eCullMode != BSCullingProcess::BSCP_CULL_IGNOREMULTIBOUNDS &&
		pShape->kCullResult == BSMultiBoundShape::BS_CULL_UNTESTED) {
		if (apMultibound->m_kChildren.m_usESize) {
			bool bshapewithinfrustum = pShape->WithinFrustum(akCuller.m_kPlanes);
			// The exe reloads the shape for the store.
			apMultibound->spMultiBound.m_pObject->spShape.m_pObject->kCullResult =
				static_cast<BSMultiBoundShape::BSMBCullResult>(!bshapewithinfrustum + 1);
			if (bshapewithinfrustum) {
				BSPortalGraph* pBSPortalGraph = this->pBSPortalGraph;
				if (pBSPortalGraph) {
					NiTListItem<BSOcclusionPlane*>* pItem = pBSPortalGraph->lOccluderList.m_pkHead;
					while (pItem) {
						BSOcclusionPlane* pOccluder = pItem->m_element;
						pItem = pItem->m_pkNext;
						if (pOccluder && pOccluder->kCullResult == BSMultiBoundShape::BS_CULL_VISIBLE &&
							!apMultibound->VisibleAgainstActiveOccluders(akViewPoint, akCuller, pOccluder))
							break;
					}
				}
			}
		}
	}
}

// Walks the rooms seen from apRoom (null: the space outside every room) breadth first through the portals that pass
// each room's compound frustum, building the frustum of every room reached; portals into the unbound space build the
// scene node's own frustum. Every portal walked through is marked visible.
void ShadowSceneNode::UpdateRoomAndPortalVisibility(BSMultiBoundRoom* apRoom, const NiCamera* apCamera,
	const NiFrustumPlanes& apPlanes, bool abFirstRoom) {
	using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
	using RoomItem = NiTListItem<BSMultiBoundRoom*>;

	// Never filled; only emptied on the way out.
	NiTPointerList<BSMultiBoundRoom*> kUnusedList{};
	NiTPointerList<BSMultiBoundRoom*> kRoomQueue{};
	if (abFirstRoom)
		bVisibleUnboundSpace = false;
	kCompoundFrustum.SetCamera(apCamera);
	NiPoint3 kCameraPos = apCamera->m_kWorld.m_Translate;

	RoomItem* pkNode;
	if (!apRoom) {
		BSPortalGraph* pGraph = pBSPortalGraph;
		bVisibleUnboundSpace = true;
		if (pGraph->lOccluderList.m_uiCount) {
			NiTListItem<BSOcclusionPlane*>* pItem = pGraph->lOccluderList.m_pkHead;
			bool bGroupOpen = false;
			if (pItem) {
				do {
					BSOcclusionPlane* pPlane = pItem->m_element;
					pItem = pItem->m_pkNext;
					if (!pPlane)
						continue;
					BSMultiBoundShape::BSMBCullResult eResult = pPlane->kCullResult;
					if (eResult == BSMultiBoundShape::BS_CULL_VISIBLE ||
						(eResult == BSMultiBoundShape::BS_CULL_UNTESTED &&
							pPlane->WithinFrustumDistFirst(apPlanes, kCameraPos))) {
						if (!bGroupOpen) {
							kCompoundFrustum.StartGroupIntersection();
							bGroupOpen = true;
						}
						kCompoundFrustum.AddOcclusionPlane(pPlane);
					}
				} while (pItem);
				if (bGroupOpen)
					kCompoundFrustum.EndGroup();
			}
		}
		pkNode =
			static_cast<RoomItem*>(reinterpret_cast<NiTPointerAllocator<uint32_t>&>(kRoomQueue.m_uiCount).Allocate());
		pkNode->m_element = nullptr;
	} else {
		void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSCompoundFrustum));
		BSCompoundFrustum* pFrustum = pMemory ? ::new (pMemory) BSCompoundFrustum : nullptr;
		pFrustum->SetCamera(apCamera);
		NiTListItem<BSOcclusionPlane*>* pItem = apRoom->lOccluderList.m_pkHead;
		bool bGroupOpen = false;
		if (pItem) {
			do {
				BSOcclusionPlane* pPlane = pItem->m_element;
				pItem = pItem->m_pkNext;
				if (!pPlane)
					continue;
				BSMultiBoundShape::BSMBCullResult eResult = pPlane->kCullResult;
				if (eResult == BSMultiBoundShape::BS_CULL_VISIBLE ||
					(eResult == BSMultiBoundShape::BS_CULL_UNTESTED &&
						pPlane->WithinFrustumDistFirst(apPlanes, kCameraPos))) {
					if (!bGroupOpen) {
						pFrustum->StartGroupIntersection();
						bGroupOpen = true;
					}
					pFrustum->AddOcclusionPlane(pPlane);
				}
			} while (pItem);
			if (bGroupOpen)
				pFrustum->EndGroup();
		}
		apRoom->SetCompoundFrustum(pFrustum);
		pkNode =
			static_cast<RoomItem*>(reinterpret_cast<NiTPointerAllocator<uint32_t>&>(kRoomQueue.m_uiCount).Allocate());
		pkNode->m_element = apRoom;
	}
	kRoomQueue.AddNodeTail(pkNode);

	do {
		// NiTPointerListBase::RemoveHead, with the node's return to the pool inlined.
		pkNode = kRoomQueue.m_pkHead;
		kRoomQueue.m_pkHead = pkNode->m_pkNext;
		if (kRoomQueue.m_pkHead)
			kRoomQueue.m_pkHead->m_pkPrev = nullptr;
		else
			kRoomQueue.m_pkTail = nullptr;
		BSMultiBoundRoom* pRoom = pkNode->m_element;
		pkNode->m_element = nullptr;
		PoolAllocator::Lock();
		PoolAllocator::AllocNode* pkFreed = reinterpret_cast<PoolAllocator::AllocNode*>(pkNode);
		pkFreed->m_pkData = nullptr;
		pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
		PoolAllocator::ms_pkFreeMem = pkFreed;
		PoolAllocator::Unlock();
		kRoomQueue.m_uiCount--;

		pBSPortalGraph->AddRoomToAccumList(pRoom);
		BSCompoundFrustum* pFrustum;
		NiTListBase<BSPortal*>* pPortals;
		if (pRoom) {
			pRoom->SetCullResult(BSMultiBoundShape::BS_CULL_VISIBLE);
			pFrustum = pRoom->pCompoundFrustum;
			pPortals = &pRoom->kPortalList;
		} else {
			pPortals = &pBSPortalGraph->lPortalList;
			bVisibleUnboundSpace = true;
			pFrustum = &kCompoundFrustum;
		}
		pFrustum->PrethreadOpList();

		for (NiTListItem<BSPortal*>* pPortalItem = pPortals->m_pkHead; pPortalItem;) {
			BSPortal* pPortal = pPortalItem->m_element;
			pPortalItem = pPortalItem->m_pkNext;
			if (!pPortal || pPortal->kCullResult != BSMultiBoundShape::BS_CULL_UNTESTED ||
				!pFrustum->ProcessPlane(pPortal))
				continue;
			BSMultiBoundRoom* pNextRoom = pPortal->pMultiBoundRoom[0];
			if (pNextRoom == pRoom) {
				pNextRoom = pPortal->pMultiBoundRoom[1];
				if (pNextRoom == pRoom)
					continue;
			}

			if (pNextRoom != apRoom) {
				// NiTPointerListBase::AddTail, with the node's allocation from the pool inlined.
				PoolAllocator::Lock();
				PoolAllocator::AllocNode* pkNew = PoolAllocator::ms_pkFreeMem;
				if (!pkNew) {
					PoolAllocator::CreateFreeMem();
					pkNew = PoolAllocator::ms_pkFreeMem;
				}
				PoolAllocator::ms_pkFreeMem = pkNew->m_pkNext;
				pkNew->m_element = 0;
				pkNew->m_pkNext = nullptr;
				pkNew->m_pkData = nullptr;
				PoolAllocator::Unlock();
				RoomItem* pkItem = reinterpret_cast<RoomItem*>(pkNew);
				pkItem->m_element = pNextRoom;
				kRoomQueue.AddNodeTail(pkItem);

				if (pNextRoom) {
					BSCompoundFrustum* pRoomFrustum = pNextRoom->pCompoundFrustum;
					if (pRoomFrustum && pRoomFrustum->iFreeOp) {
						// The room was reached before: add this portal's view as one more union member.
						pRoomFrustum->MakeIntoUnion();
						if ((pFrustum && pFrustum->iFreeOp) || pNextRoom->lOccluderList.m_uiCount) {
							pRoomFrustum->StartGroupIntersection();
							if (pFrustum && pFrustum->iFreeOp)
								pRoomFrustum->AddCompoundFrustum(pFrustum);
						}
						pRoomFrustum->AddPortal(pPortal, false);
						for (NiTListItem<BSOcclusionPlane*>* pItem = pNextRoom->lOccluderList.m_pkHead; pItem;) {
							BSOcclusionPlane* pPlane = pItem->m_element;
							pItem = pItem->m_pkNext;
							if (pPlane && pFrustum->ProcessPlane(pPlane))
								pRoomFrustum->AddOcclusionPlane(pPlane);
						}
						pRoomFrustum->EndGroup();
						if ((pFrustum && pFrustum->iFreeOp) || pNextRoom->lOccluderList.m_uiCount)
							pRoomFrustum->EndGroup();
					} else {
						if (!pRoomFrustum) {
							void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSCompoundFrustum));
							pRoomFrustum = pMemory ? ::new (pMemory) BSCompoundFrustum : nullptr;
						}
						pRoomFrustum->SetCamera(apCamera);
						// A frustum of more than 25 operations is not carried through the portal.
						BSCompoundFrustum* pSource = pFrustum;
						if (pFrustum->iFreeOp > 25)
							pSource = nullptr;
						if ((pSource && pSource->iFreeOp) || pNextRoom->lOccluderList.m_uiCount) {
							pRoomFrustum->StartGroupIntersection();
							if (pSource && pSource->iFreeOp) {
								alignas(BSCompoundFrustum) uint8_t aucFrustum[sizeof(BSCompoundFrustum)];
								BSCompoundFrustum* pPortalFrustum = ::new (aucFrustum) BSCompoundFrustum;
								pPortalFrustum->BuildForPortal(pSource, pPortal);
								pRoomFrustum->AddCompoundFrustum(pPortalFrustum);
								pPortalFrustum->~BSCompoundFrustum();
							}
						}
						pRoomFrustum->AddPortal(pPortal, false);
						for (NiTListItem<BSOcclusionPlane*>* pItem = pNextRoom->lOccluderList.m_pkHead; pItem;) {
							BSOcclusionPlane* pPlane = pItem->m_element;
							pItem = pItem->m_pkNext;
							if (pPlane && (!pSource || pSource->ProcessPlane(pPlane)))
								pRoomFrustum->AddOcclusionPlane(pPlane);
						}
						if ((pSource && pSource->iFreeOp) || pNextRoom->lOccluderList.m_uiCount) {
							if (pRoomFrustum->iFreeOp == 1)
								pRoomFrustum->SetCamera(apCamera);
							else
								pRoomFrustum->EndGroup();
						}
						if (!pSource || pSource->bSkipViewFrustum ||
							(!pSource->iFreeOp && pPortal->QEdgeVisibleState() == 0xF))
							pRoomFrustum->bSkipViewFrustum = true;
						if (!pNextRoom->pCompoundFrustum)
							pNextRoom->SetCompoundFrustum(pRoomFrustum);
					}
				} else {
					// The portal leads out into the unbound space: build the scene node's frustum.
					NiTListBase<BSOcclusionPlane*>* pOccluders = &pBSPortalGraph->lOccluderList;
					if (!kCompoundFrustum.iFreeOp) {
						if ((pFrustum && pFrustum->iFreeOp) || pOccluders->m_uiCount) {
							kCompoundFrustum.StartGroupIntersection();
							if (pFrustum && pFrustum->iFreeOp) {
								alignas(BSCompoundFrustum) uint8_t aucFrustum[sizeof(BSCompoundFrustum)];
								BSCompoundFrustum* pPortalFrustum = ::new (aucFrustum) BSCompoundFrustum;
								pPortalFrustum->BuildForPortal(pFrustum, pPortal);
								kCompoundFrustum.AddCompoundFrustum(pPortalFrustum);
								pPortalFrustum->~BSCompoundFrustum();
							}
						}
						kCompoundFrustum.AddPortal(pPortal, false);
						for (NiTListItem<BSOcclusionPlane*>* pItem = pOccluders->m_pkHead; pItem;) {
							BSOcclusionPlane* pPlane = pItem->m_element;
							pItem = pItem->m_pkNext;
							if (pPlane && pFrustum->ProcessPlane(pPlane))
								kCompoundFrustum.AddOcclusionPlane(pPlane);
						}
						if ((pFrustum && pFrustum->iFreeOp) || pOccluders->m_uiCount)
							kCompoundFrustum.EndGroup();
						if (!pFrustum || (!pFrustum->iFreeOp && pPortal->QEdgeVisibleState() == 0xF))
							kCompoundFrustum.bSkipViewFrustum = true;
					} else {
						kCompoundFrustum.MakeIntoUnion();
						if ((pFrustum && pFrustum->iFreeOp) || pOccluders->m_uiCount) {
							kCompoundFrustum.StartGroupIntersection();
							if (pFrustum && pFrustum->iFreeOp)
								kCompoundFrustum.AddCompoundFrustum(pFrustum);
						}
						kCompoundFrustum.AddPortal(pPortal, false);
						for (NiTListItem<BSOcclusionPlane*>* pItem = pOccluders->m_pkHead; pItem;) {
							BSOcclusionPlane* pPlane = pItem->m_element;
							pItem = pItem->m_pkNext;
							if (pPlane && pFrustum->ProcessPlane(pPlane))
								kCompoundFrustum.AddOcclusionPlane(pPlane);
						}
						kCompoundFrustum.EndGroup();
						if ((pFrustum && pFrustum->iFreeOp) || pOccluders->m_uiCount)
							kCompoundFrustum.EndGroup();
					}
				}
			}
			pPortal->kCullResult = BSMultiBoundShape::BS_CULL_VISIBLE;
		}
	} while (kRoomQueue.m_uiCount);

	kRoomQueue.RemoveAll();
	kUnusedList.RemoveAll();
}

// Takes apFog; without one, makes a fog that starts and ends at 500000 units, in mid grey above and below water.
void ShadowSceneNode::SetFogProperty(BSFogProperty* apFog) {
	if (apFog) {
		BSFogProperty* pOldFog = spFog;
		if (pOldFog != apFog) {
			if (pOldFog)
				NiDecRefCount(pOldFog);
			spFog = apFog;
			NiIncRefCount(apFog);
		}
		return;
	}

	void* pMemory = NiMemObject::operator new(sizeof(BSFogProperty));
	BSFogProperty* pNewFog = pMemory ? ::new (pMemory) BSFogProperty : nullptr;
	BSFogProperty* pOldFog = spFog;
	if (pOldFog != pNewFog) {
		if (pOldFog)
			NiDecRefCount(pOldFog);
		spFog = pNewFog;
		if (pNewFog)
			NiIncRefCount(pNewFog);
	}

	BSFogProperty* pFog = spFog;
	pFog->fStartDistance = 500000.0f;
	pFog->fEndDistance = 500000.0f;
	pFog->fPower = 1.0f;
	pFog->fStartWaterDistance = 500000.0f;
	pFog->fEndWaterDistance = 500000.0f;
	pFog->m_kColor.r = 0.5f;
	pFog->m_kColor.g = 0.5f;
	pFog->m_kColor.b = 0.5f;
	NiColor& kWaterColor = spFog->kWaterColor;
	kWaterColor.r = 0.5f;
	kWaterColor.g = 0.5f;
	kWaterColor.b = 0.5f;
}

// Replaces the sun's scene light with one for apLight; without one, for a new black directional light.
void ShadowSceneNode::SetSunLight(NiDirectionalLight* apLight) {
	if (pSunLight) {
		delete pSunLight;
		pSunLight = nullptr;
	}

	NiDirectionalLight* pLight = apLight;
	if (!pLight) {
		void* pLightMemory = NiMemObject::operator new(sizeof(NiDirectionalLight));
		pLight = pLightMemory ? ::new (pLightMemory) NiDirectionalLight : nullptr;
		// SetAmbientColor and SetDiffuseColor, inlined: each copies the colour and bumps the revision.
		pLight->m_kAmb = NiColor_BLACK;
		pLight->m_uiRevID++;
		pLight->m_kDiff = NiColor_BLACK;
		pLight->m_uiRevID++;
	}

	void* pMemory = NiMemObject::operator new(sizeof(ShadowSceneLight));
	ShadowSceneLight* pSceneLight = pMemory ? ::new (pMemory) ShadowSceneLight : nullptr;
	pSunLight = pSceneLight;
	pSceneLight->SetLight(pLight);
}

void ShadowSceneNode::UpdateOcclusionPlaneVisibility(NiCamera* apCamera) {
	if (!pBSPortalGraph)
		return;

	alignas(NiCullingProcess) uint8_t aCullerStorage[sizeof(NiCullingProcess)];
	NiCullingProcess* pCuller = ::new (aCullerStorage) NiCullingProcess(nullptr);
	pCuller->m_pkCamera = apCamera;
	pCuller->SetFrustum(apCamera->m_kViewFrustum);
	NiTListItem<BSOcclusionPlane*>* pIter = pBSPortalGraph->lOccluderList.m_pkHead;
	// A frustum-planes local the exe builds (six NiPlane constructors, every plane active) and never reads.
	NiFrustumPlanes kUnusedPlanes;
	kUnusedPlanes.SetActivePlaneState((1u << NiFrustumPlanes::MAX_PLANES) - 1);
	while (pIter) {
		BSOcclusionPlane* pPlane = pIter->m_element;
		pIter = pIter->m_pkNext;
		if (pPlane) {
			bool bWithinFrustum = pPlane->WithinFrustum(pCuller->m_kPlanes, false, NiPoint3_ZERO);
			bool bCorrectSide = pPlane->ViewerOnCorrectSide(apCamera->m_kWorld.m_Translate);
			pPlane->kCullResult = (bWithinFrustum & bCorrectSide) ? BSMultiBoundShape::BS_CULL_VISIBLE
																  : BSMultiBoundShape::BS_CULL_CULLED;
		}
	}
	pCuller->~NiCullingProcess();
}

void ShadowSceneNode::RemoveAllShadowLights() {
	ShadowSceneLightItem* pItem = lShadowLightList.m_pkHead;
	while (pItem) {
		ShadowSceneLight* pLight = pItem->m_element.m_pObject;
		pItem = pItem->m_pkNext;
		if (pLight) {
			BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(pLight->spDynamicShadowMap.m_pObject);
			pLight->spDynamicShadowMap = nullptr;
		}
	}
	lShadowLightList.RemoveAll();
	kShadowLightActiveFence = nullptr;
	kShadowLightFadingInFence = nullptr;
}

void ShadowSceneNode::RemoveObject(NiAVObject* apObject) {
	NiNode* pNode;
	if (apObject) {
		pNode = apObject->IsNode();
		if (pNode) {
			RemoveObjectRecurse(pNode);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void ShadowSceneNode::RemoveShadowLight(NiAVObject* apObject) {
	if (!apObject)
		return;

	for (ShadowSceneLightItem* pItem = lShadowLightList.m_pkHead; pItem;) {
		ShadowSceneLight* pLight = pItem->m_element.m_pObject;
		pItem = pItem->m_pkNext;
		if (pLight && pLight->spShadowCaster.m_pObject == apObject) {
			// The exe builds the NiPointer in apObject's argument slot, without its constructor's and destructor's
			// null tests (pLight was tested above).
			FNVD_INLINED_LOCAL(NiPointer<ShadowSceneLight>, spLight, std::nullopt, std::nullopt);
			spLight.m_pObject = pLight;
			NiIncRefCount(pLight);
			RemoveLight(spLight);
			NiDecRefCount(pLight);
			return;
		}
	}
}

ShadowSceneLight* ShadowSceneNode::AddLight(NiLight* apLight, bool abDynamic) {
	if (!apLight ||
		(apLight->GetRTTI() != &NiDirectionalLight::ms_RTTI && apLight->GetRTTI() != &NiPointLight::ms_RTTI))
		return nullptr;

	SSNLightQueueCrit.Lock(nullptr);
	void* pMemory = NiMemObject::operator new(sizeof(ShadowSceneLight));
	ShadowSceneLight* pSceneLight = pMemory ? ::new (pMemory) ShadowSceneLight : nullptr;
	pSceneLight->SetLight(apLight);
	pSceneLight->bDynamicLight = abDynamic;
	{
		NiPointer<ShadowSceneLight> spSceneLight(pSceneLight);
		lLightQueueAdd.AddTail(spSceneLight);
	}
	pSceneLight->pPortalGraph = pBSPortalGraph;
	// BSSpinLock::Unlock, inlined.
	if (--SSNLightQueueCrit.uiLockCount == 0)
		SSNLightQueueCrit.uiOwningThread = 0;
	return pSceneLight;
}

uint32_t ShadowSceneNode::AddAllLightsFromNode_Recurse(NiNode* apNode, float afDefaultRadius) {
	uint32_t uiCount = 0;
	for (uint32_t i = 0; i < apNode->m_kChildren.m_usSize; ++i) {
		NiAVObject* pChild = apNode->m_kChildren.m_pBase[i].m_pObject;
		if (!pChild)
			continue;

		NiNode* pChildNode = pChild->IsNode();
		if (pChildNode) {
			uiCount += AddAllLightsFromNode_Recurse(pChildNode, afDefaultRadius);
			continue;
		}

		NiLight* pLight = pChild->IsKindOf(&NiLight::ms_RTTI) ? static_cast<NiLight*>(pChild) : nullptr;
		if (!pLight)
			continue;

		AddLight(pLight, true);
		NiUpdateData kUpdateData;
		kUpdateData.fTime = 0.0f;
		kUpdateData.bUpdateControllers = false;
		kUpdateData.bParallelUpdate = false;
		kUpdateData.bFoundParticles = false;
		kUpdateData.bFoundMorphController = false;
		kUpdateData.bSceneGraphChange = false;
		pLight->Update(kUpdateData);
		NiPointLight* pPointLight = static_cast<NiPointLight*>(pLight->DynamicCast(&NiPointLight::ms_RTTI));
		if (pPointLight) {
			// SetSpecularColor, inlined: the radius is kept in the specular colour's red channel.
			pPointLight->m_uiRevID++;
			pPointLight->m_kSpec.r = afDefaultRadius;
			pPointLight->m_kSpec.g = 0.0f;
			pPointLight->m_kSpec.b = 0.0f;
		}
		++uiCount;
	}
	return uiCount;
}

ShadowSceneLight* ShadowSceneNode::AddShadowLight(NiAVObject* apObject) {
	ShadowSceneLightItem* pItem = lShadowLightList.m_pkHead;
	while (pItem) {
		ShadowSceneLight* pLight = pItem->m_element.m_pObject;
		pItem = pItem->m_pkNext;
		if (pLight && pLight->spShadowCaster.m_pObject == apObject) {
			NiLight* pNiLight = pLight->spLight.m_pObject;
			pNiLight->m_uFlags = static_cast<NiAVObject::NiFlags>(pNiLight->m_uFlags & ~NiAVObject::APP_CULLED);
			pLight->SetFadeTarget(1.0f, false);
			return pLight;
		}
	}

	SSNLightQueueCrit.Lock(nullptr);
	void* pMemory = NiMemObject::operator new(sizeof(ShadowSceneLight));
	ShadowSceneLight* pSceneLight = pMemory ? ::new (pMemory) ShadowSceneLight : nullptr;
	pSceneLight->SetShadowCaster(static_cast<NiNode*>(apObject));
	pSceneLight->SetCastShadow(true);
	{
		NiPointer<ShadowSceneLight> spSceneLight(pSceneLight);
		lLightQueueAdd.AddTail(spSceneLight);
	}
	// BSSpinLock::Unlock, inlined.
	if (--SSNLightQueueCrit.uiLockCount == 0)
		SSNLightQueueCrit.uiOwningThread = 0;
	return pSceneLight;
}

// Turns apLight into a dynamic shadow-casting point light (a new, visible NiPointLight) and lists it at once.
ShadowSceneLight* ShadowSceneNode::AddQueuedShadowLight(ShadowSceneLight* apLight) {
	apLight->bDynamicLight = true;
	void* pMemory = NiMemObject::operator new(sizeof(NiPointLight));
	NiPointLight* pPointLight = pMemory ? ::new (pMemory) NiPointLight : nullptr;
	pPointLight->m_uFlags = static_cast<NiAVObject::NiFlags>(pPointLight->m_uFlags & ~NiAVObject::APP_CULLED);
	apLight->SetLight(pPointLight);
	apLight->SetCastShadow(true);
	apLight->bCubeShadowMap = false;
	NiPointer<ShadowSceneLight> spLight(apLight);
	lShadowLightList.AddTail(spLight);
	return apLight;
}

// Queues every light for removal (when the remove queues are allowed) and drops every shadow light.
void ShadowSceneNode::RemoveAllLights() {
	ShadowSceneLightItem* pItem = lLightList.m_pkHead;
	while (pItem) {
		ShadowSceneLight* pLight = pItem->m_element.m_pObject;
		pItem = pItem->m_pkNext;
		if (pLight) {
			NiPointer<ShadowSceneLight> spLight(pLight);
			if (bAllowLightRemoveQueues) {
				SSNLightQueueCrit.Lock(nullptr);
				lLightQueueRemove.AddTail(spLight);
				// BSSpinLock::Unlock, inlined.
				if (--SSNLightQueueCrit.uiLockCount == 0)
					SSNLightQueueCrit.uiOwningThread = 0;
			}
		}
	}
	RemoveAllShadowLights();
	lShadowLightList.RemoveAll();
	kShadowLightActiveFence = nullptr;
	kShadowLightFadingInFence = nullptr;
	RemoveAllShadowLights();
}

// Queues apSSLight for an update; no lock is taken.
void ShadowSceneNode::UpdateLight(ShadowSceneLight* apSSLight) {
	if (apSSLight) {
		NiPointer<ShadowSceneLight> spLight(apSSLight);
		lLightQueueUpdate.AddTail(spLight);
	}
}

void ShadowSceneNode::UpdateLight_0(NiLight* apLight) {
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
	if (m_pkHead) {
		ShadowSceneLight* apSSLight;
		while (1) {
			apSSLight = m_pkHead->m_element.m_pObject;
			m_pkHead = m_pkHead->m_pkNext;
			if (apSSLight) {
				if (apSSLight->spLight.m_pObject == apLight) {
					break;
				}
			}
			if (!m_pkHead) {
				return;
			}
		}
		UpdateLight(apSSLight);
	}
}

// Relights apSSLight. A culled or nearly dark light only drops its geometry. With rooms, the light joins each room
// that holds or touches it (with the room's portals and their shared nodes) and leaves the rooms it no longer
// reaches, then takes the unbound nodes and every portal; without rooms it takes the scene's child nodes (of the world
// scene node only children 3 and 6). Always-render objects come last.
// The engine inlines several finds as plain loops (the buffer is tested per item), written out here.
void ShadowSceneNode::UpdateQueuedLight(ShadowSceneLight* apSSLight) {
	NiLight* pLight = apSSLight->spLight.m_pObject;
	if ((pLight->m_uFlags & NiAVObject::APP_CULLED) || pLight->m_fDimmer < 0.05000000074505806) {
		apSSLight->RemoveAllTriShapes();
		return;
	}

	apSSLight->pPortalGraph = pBSPortalGraph;
	apSSLight->InsertUnusedFence();
	BSPortalGraph* pGraph = pBSPortalGraph;
	if (pGraph && pGraph->lMultiBoundRoomList.m_uiCount) {
		apSSLight->kMultiboundRooms.Clear(true);
		apSSLight->kPortals.Clear(true);
		apSSLight->kProcessedNodes.Clear(true);

		NiPoint3 kLightPos = apSSLight->spLight->m_kWorld.m_Translate;
		NiBound kLightBound;
		kLightBound.m_kCenter = kLightPos;
		kLightBound.m_fRadius = apSSLight->spLight->m_kSpec.r;

		NiTListItem<NiPointer<BSMultiBoundRoom>>* pRoomIter = pBSPortalGraph->lMultiBoundRoomList.m_pkHead;
		while (pRoomIter) {
			BSMultiBoundRoom* pRoom = pRoomIter->m_element.m_pObject;
			pRoomIter = pRoomIter->m_pkNext;
			if (!pRoom)
				continue;

			int32_t iLightIndex =
				pRoom->kLights.Find(apSSLight, 0, &BSSimpleArray<ShadowSceneLight*>::DefaultEqualFunctionByAddress);
			if (!pRoom->spMultiBound->spShape->QPointWithin(kLightPos) &&
				pRoom->CheckBound(kLightBound) == BSMultiBoundShape::BS_INTERSECT_NONE) {
				// The light left the room: drop it there and from the portals it no longer passes through.
				if (iLightIndex == -1)
					continue;
				pRoom->kLights.RemoveFast(iLightIndex, 1);
				NiTListItem<BSPortal*>* pPortalIter = pRoom->kPortalList.m_pkHead;
				while (pPortalIter) {
					BSPortal* pPortal = pPortalIter->m_element;
					pPortalIter = pPortalIter->m_pkNext;
					if (!pPortal)
						continue;
					if (apSSLight->kPortals.Find(pPortal, 0,
							&BSSimpleArray<BSPortal*>::DefaultEqualFunctionByAddress) != -1)
						continue;
					int32_t iPortalIndex = pPortal->spSharedGeometry->kLights.Find(apSSLight, 0,
						&BSSimpleArray<ShadowSceneLight*>::DefaultEqualFunctionByAddress);
					if (iPortalIndex != -1)
						pPortal->spSharedGeometry->kLights.RemoveFast(iPortalIndex, 1);
				}
				continue;
			}

			if (iLightIndex == -1) {
				uint32_t uiIndex = pRoom->kLights.AddUninitialized();
				pRoom->kLights.pBuffer[uiIndex] = apSSLight;
			}
			NiTListItem<BSPortal*>* pPortalIter = pRoom->kPortalList.m_pkHead;
			while (pPortalIter) {
				BSPortal* pPortal = pPortalIter->m_element;
				pPortalIter = pPortalIter->m_pkNext;
				if (!pPortal)
					continue;

				BSSimpleArray<ShadowSceneLight*>& kPortalLights = pPortal->spSharedGeometry->kLights;
				int32_t iIndex = -1;
				for (uint32_t i = 0; i < kPortalLights.iSize; ++i) {
					if (kPortalLights.pBuffer && kPortalLights.pBuffer[i] == apSSLight) {
						iIndex = static_cast<int32_t>(i);
						break;
					}
				}
				if (iIndex == -1) {
					uint32_t uiIndex = kPortalLights.AddUninitialized();
					kPortalLights.pBuffer[uiIndex] = apSSLight;
				}

				BSPortalSharedNode* pSharedNode = pPortal->spSharedGeometry.m_pObject;
				iIndex = -1;
				for (uint32_t i = 0; i < apSSLight->kProcessedNodes.iSize; ++i) {
					if (apSSLight->kProcessedNodes.pBuffer && apSSLight->kProcessedNodes.pBuffer[i] == pSharedNode) {
						iIndex = static_cast<int32_t>(i);
						break;
					}
				}
				if (iIndex == -1) {
					apSSLight->AddObject(pSharedNode);
					BSPortalSharedNode* pProcessedNode = pPortal->spSharedGeometry.m_pObject;
					uint32_t uiIndex = apSSLight->kProcessedNodes.AddUninitialized();
					apSSLight->kProcessedNodes.pBuffer[uiIndex] = pProcessedNode;
				}

				iIndex = -1;
				for (uint32_t i = 0; i < apSSLight->kPortals.iSize; ++i) {
					if (apSSLight->kPortals.pBuffer && apSSLight->kPortals.pBuffer[i] == pPortal) {
						iIndex = static_cast<int32_t>(i);
						break;
					}
				}
				if (iIndex == -1) {
					uint32_t uiIndex = apSSLight->kPortals.AddUninitialized();
					apSSLight->kPortals.pBuffer[uiIndex] = pPortal;
				}
			}
			apSSLight->AddObject(pRoom);
			uint32_t uiRoomIndex = apSSLight->kMultiboundRooms.AddUninitialized();
			apSSLight->kMultiboundRooms.pBuffer[uiRoomIndex] = pRoom;
		}

		for (uint32_t i = 0; i < pBSPortalGraph->kUnboundNodes.iSize; ++i)
			apSSLight->AddObject(pBSPortalGraph->kUnboundNodes.pBuffer[i].m_pObject);

		NiTListItem<BSPortal*>* pPortalIter = pBSPortalGraph->lPortalList.m_pkHead;
		while (pPortalIter) {
			BSPortal* pPortal = pPortalIter->m_element;
			pPortalIter = pPortalIter->m_pkNext;
			if (!pPortal)
				continue;

			BSSimpleArray<ShadowSceneLight*>& kPortalLights = pPortal->spSharedGeometry->kLights;
			int32_t iIndex = -1;
			for (uint32_t i = 0; i < kPortalLights.iSize; ++i) {
				if (kPortalLights.pBuffer && kPortalLights.pBuffer[i] == apSSLight) {
					iIndex = static_cast<int32_t>(i);
					break;
				}
			}
			if (iIndex == -1) {
				uint32_t uiIndex = kPortalLights.AddUninitialized();
				kPortalLights.pBuffer[uiIndex] = apSSLight;
			}

			BSPortalSharedNode* pSharedNode = pPortal->spSharedGeometry.m_pObject;
			iIndex = -1;
			for (uint32_t i = 0; i < apSSLight->kProcessedNodes.iSize; ++i) {
				if (apSSLight->kProcessedNodes.pBuffer && apSSLight->kProcessedNodes.pBuffer[i] == pSharedNode) {
					iIndex = static_cast<int32_t>(i);
					break;
				}
			}
			if (iIndex == -1) {
				apSSLight->AddObject(pSharedNode);
				BSPortalSharedNode* pProcessedNode = pPortal->spSharedGeometry.m_pObject;
				uint32_t uiIndex = apSSLight->kProcessedNodes.AddUninitialized();
				apSSLight->kProcessedNodes.pBuffer[uiIndex] = pProcessedNode;
			}

			iIndex = -1;
			for (uint32_t i = 0; i < apSSLight->kPortals.iSize; ++i) {
				if (apSSLight->kPortals.pBuffer && apSSLight->kPortals.pBuffer[i] == pPortal) {
					iIndex = static_cast<int32_t>(i);
					break;
				}
			}
			if (iIndex == -1) {
				uint32_t uiIndex = apSSLight->kPortals.AddUninitialized();
				apSSLight->kPortals.pBuffer[uiIndex] = pPortal;
			}
		}

		for (uint32_t i = 0; i < pBSPortalGraph->GetAlwaysRenderArrayCount(); ++i) {
			NiAVObject* pObject = pBSPortalGraph->GetAlwaysRenderAt(i);
			if (pObject)
				apSSLight->AddObject(pObject);
		}
		apSSLight->RemoveUnusedGeom();
		return;
	}

	if (this != BSShaderManager::pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD]) {
		for (uint32_t i = 0; i < m_kChildren.m_usSize; ++i) {
			NiAVObject* pChild = m_kChildren.m_pBase[i].m_pObject;
			if (pChild) {
				NiNode* pChildNode = pChild->IsNode();
				if (pChildNode)
					apSSLight->AddObject(pChildNode);
			}
		}
	} else {
		if (m_kChildren.m_usSize > 3) {
			NiAVObject* pChild = m_kChildren.m_pBase[3].m_pObject;
			if (pChild) {
				NiNode* pChildNode = pChild->IsNode();
				if (pChildNode)
					apSSLight->AddObject(pChildNode);
			}
		}
		if (m_kChildren.m_usSize > 6) {
			NiAVObject* pChild = m_kChildren.m_pBase[6].m_pObject;
			if (pChild) {
				NiNode* pChildNode = pChild->IsNode();
				if (pChildNode)
					apSSLight->AddObject(pChildNode);
			}
		}
	}

	if (pBSPortalGraph) {
		for (uint32_t i = 0; i < pBSPortalGraph->GetAlwaysRenderArrayCount(); ++i) {
			NiAVObject* pObject = pBSPortalGraph->GetAlwaysRenderAt(i);
			if (pObject && !pObject->m_pkParent)
				apSSLight->AddObject(pObject);
		}
	}
	apSSLight->RemoveUnusedGeom();
}

// Follows a moving dynamic point light that casts no shadow: a culled light loses its geometry, a moved one that is
// not culled from every frustum takes the new position and is relit.
void ShadowSceneNode::UpdateQueuedDynamicLight(ShadowSceneLight* apSSLight) {
	if (!apSSLight->bPointLight || !apSSLight->bDynamicLight || apSSLight->bCastShadow)
		return;

	NiLight* pLight = apSSLight->spLight.m_pObject;
	if (pLight->m_uFlags & NiAVObject::APP_CULLED) {
		apSSLight->RemoveAllTriShapes();
		return;
	}

	NiPoint3 kPointPosition = apSSLight->bPointPosition;
	const NiPoint3& kTranslate = pLight->m_kWorld.m_Translate;
	if (kPointPosition != kTranslate && apSSLight->usFrustumCull != ShadowSceneLight::SSL_ALL) {
		apSSLight->SetDynamicPosition(kTranslate);
		UpdateQueuedLight(apSSLight);
	}
}

void ShadowSceneNode::SetSceneGraphIndex(uint8_t acSceneGraphIndex) {
	cSceneGraphIndex = acSceneGraphIndex;
	BSShaderManager::SetShadowSceneNode(static_cast<BSShaderManager::eSceneGraphEnum>(acSceneGraphIndex), this);
}

BSPortalGraph* ShadowSceneNode::GetPortalGraph() const {
	return pBSPortalGraph;
}

bool ShadowSceneNode::GetDisableLightUpdate() const {
	return bDisableLightUpdate;
}

void ShadowSceneNode::SetDisableLightUpdate(bool abDisableLightUpdate) {
	bDisableLightUpdate = abDisableLightUpdate;
}

bool ShadowSceneNode::QWireframe() const {
	return bWireframe;
}

void ShadowSceneNode::SetEyePosition(NiPoint3 aPosition) {
	kEyePosition = aPosition;
}

void ShadowSceneNode::SetLightingOffset(NiPoint3 kOffset) {
	kLightingOffset = kOffset;
}

NiPoint3 ShadowSceneNode::GetCameraPos() const {
	return kEyePosition;
}

void ShadowSceneNode::AddObject(NiAVObject* apObject) {
	if (apObject && !(apObject->m_uFlags & NiAVObject::APP_CULLED))
		UpdateObjectLighting(apObject, 0);
}

ShadowSceneLight* ShadowSceneNode::GetLightAt(uint32_t auiIndex) const {
	if (auiIndex >= lLightList.m_uiCount)
		return nullptr;

	ShadowSceneLightItem* pItem = lLightList.m_pkHead;
	for (uint32_t i = auiIndex; i; --i)
		pItem = pItem->m_pkNext;
	return pItem->m_element;
}

// Under a sun brighter than 0.4 the light sits 1000 units from aTargetCenter towards the sun. Otherwise it sits at
// aTargetCenter plus the offsets of up to 100 unculled scene lights, each weighted by its falloff over its radius,
// how squarely it faces aTargetDir, and its luminance; with no light weighing over 0.01 it falls back to the sun.
bool ShadowSceneNode::GetAverageLightPos(const NiPoint3& aTargetCenter, const NiPoint3& aTargetDir, NiPoint3& aLoc) {
	NiDirectionalLight* pdirlight = static_cast<NiDirectionalLight*>(pSunLight->spLight.m_pObject);
	float fDimmer = pdirlight->m_fDimmer;
	NiColor dirlightcolor;
	dirlightcolor.r = fDimmer * pdirlight->m_kDiff.r;
	dirlightcolor.g = fDimmer * pdirlight->m_kDiff.g;
	dirlightcolor.b = fDimmer * pdirlight->m_kDiff.b;
	NiPoint3 blendedVect = NiPoint3_ZERO;
	uint32_t uinumlights = 0;
	float fdirlum = (dirlightcolor.g + dirlightcolor.r + dirlightcolor.b) / 3.0;
	if (fdirlum > 0.4) {
		// EXE-INLINED 0x4A0BD0
		NiPoint3 lightdir;
		lightdir.x = -pdirlight->m_kWorldDir.x;
		lightdir.y = -pdirlight->m_kWorldDir.y;
		lightdir.z = -pdirlight->m_kWorldDir.z;
		lightdir.Unitize();
		// EXE-INLINED 0x45BB20
		blendedVect.x = lightdir.x * 1000.0;
		blendedVect.y = lightdir.y * 1000.0;
		blendedVect.z = lightdir.z * 1000.0;
		uinumlights = 1;
	} else {
		// The exe's guarded static: plightVecA (0x11F99C8) with guard bit 0 of 0x11F9E78.
		if (!(plightVecA_guard & 1))
			plightVecA_guard |= 1;

		NiPoint3 kextents = NiPoint3_ZERO;
		float fbradius = kextents.x > kextents.y ? kextents.x : kextents.y;
		fbradius = fbradius > kextents.z ? fbradius : kextents.z;
		int32_t fgtzero = 0;
		float ftotalscale = 0.0f;
		uint16_t ilightindex = 0;
		ShadowSceneLight* plight = GetLightAt(ilightindex);
		while (plight && uinumlights < 100) {
			NiLight* pLight = plight->spLight.m_pObject;
			if (!(pLight->m_uFlags & NiAVObject::APP_CULLED)) {
				// EXE-INLINED 0x439EF0
				NiPoint3 kvect;
				kvect.x = pLight->m_kWorld.m_Translate.x - aTargetCenter.x;
				kvect.y = pLight->m_kWorld.m_Translate.y - aTargetCenter.y;
				kvect.z = pLight->m_kWorld.m_Translate.z - aTargetCenter.z;
				NiPoint3 knormvect = kvect;
				knormvect.Unitize();
				// EXE-INLINED 0x4B6190
				float fdot = aTargetDir.x * knormvect.x + aTargetDir.y * knormvect.y + aTargetDir.z * knormvect.z;
				if (fdot < 0.0f)
					fdot = 0.0f;

				// The exe reloads the light for its colour, but reads the radius through the pointer loaded above.
				NiLight* pColorLight = plight->spLight.m_pObject;
				float fLightDimmer = pColorLight->m_fDimmer;
				NiColor plightColor;
				plightColor.r = fLightDimmer * pColorLight->m_kDiff.r;
				plightColor.g = fLightDimmer * pColorLight->m_kDiff.g;
				plightColor.b = fLightDimmer * pColorLight->m_kDiff.b;
				float flum = (plightColor.g + plightColor.r + plightColor.b) / 3.0;

				// EXE-INLINED 0x457990: the sum of squares is stored as a float before the CRT's sqrt.
				float fSqrLength = kvect.x * kvect.x + kvect.y * kvect.y + kvect.z * kvect.z;
				float fLength = std::sqrt(fSqrLength);
				float fatten = 1.0f - (fLength - fbradius) / pLight->m_kSpec.r;
				if (fatten < 0.0)
					fatten = 0.0f;
				if (fatten > 1.0)
					fatten = 1.0f;
				fatten = fatten * fdot * flum;
				pfatten[uinumlights] = fatten;
				pfscale[uinumlights] = fatten;
				if (fatten > 0.01f)
					++fgtzero;
				ftotalscale += fatten;
				plightVecA[uinumlights] = kvect;
				++uinumlights;
			}

			// The exe inlines GetLightAt here, with the index widened from 16 bits.
			++ilightindex;
			if (ilightindex >= lLightList.m_uiCount)
				break;
			ShadowSceneLightItem* pItem = lLightList.m_pkHead;
			for (uint32_t i = ilightindex; i; --i)
				pItem = pItem->m_pkNext;
			plight = pItem->m_element.m_pObject;
		}

		if (fgtzero > 0) {
			for (uint32_t i = 0; i < uinumlights; ++i) {
				float fblend = pfscale[i] / ftotalscale;
				if (fblend > 0.0f) {
					// EXE-INLINED 0x45BB20
					NiPoint3 kvector;
					kvector.x = plightVecA[i].x * fblend;
					kvector.y = plightVecA[i].y * fblend;
					kvector.z = plightVecA[i].z * fblend;
					// EXE-INLINED 0x63C8A0
					blendedVect.x = kvector.x + blendedVect.x;
					blendedVect.y = blendedVect.y + kvector.y;
					blendedVect.z = blendedVect.z + kvector.z;
				}
			}
		} else {
			NiDirectionalLight* pSun = static_cast<NiDirectionalLight*>(pSunLight->spLight.m_pObject);
			// EXE-INLINED 0x4A0BD0
			NiPoint3 lightdir;
			lightdir.x = -pSun->m_kWorldDir.x;
			lightdir.y = -pSun->m_kWorldDir.y;
			lightdir.z = -pSun->m_kWorldDir.z;
			lightdir.Unitize();
			// EXE-INLINED 0x45BB20
			blendedVect.x = lightdir.x * 1000.0;
			blendedVect.y = lightdir.y * 1000.0;
			blendedVect.z = lightdir.z * 1000.0;
			uinumlights = 1;
		}
	}

	// EXE-INLINED 0x439E90: the sum is built on the stack, then copied to aLoc.
	NiPoint3 kLoc;
	kLoc.x = aTargetCenter.x + blendedVect.x;
	kLoc.y = aTargetCenter.y + blendedVect.y;
	kLoc.z = aTargetCenter.z + blendedVect.z;
	aLoc = kLoc;
	return uinumlights > 0;
}

BSFogProperty* ShadowSceneNode::GetFogProperty() {
	return spFog;
}

bool ShadowSceneNode::HasVisibleUnboundSpace() {
	return (bool)((uint8_t)bVisibleUnboundSpace);
}

void* ShadowSceneNode::GetLightingOffset(NiPoint3* pOut) {
	float y;
	float z;
	pOut->x = kLightingOffset.x;
	y = kLightingOffset.y;
	z = kLightingOffset.z;
	pOut->y = y;
	pOut->z = z;
	return pOut;
}

void ShadowSceneNode::ReleaseAllShadowMaps() {
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lShadowLightList.m_pkHead;
	while (m_pkHead) {
		ShadowSceneLight* m_pObject = m_pkHead->m_element.m_pObject;
		m_pkHead = m_pkHead->m_pkNext;
		if (m_pObject) {
			BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(m_pObject->spDynamicShadowMap.m_pObject);
			m_pObject->spDynamicShadowMap = nullptr;
		}
	}
	BSShaderManager::pTextureManager->ClearShadowDepthBuffer();
}

uint16_t ShadowSceneNode::GetActiveLightCount() {
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
	uint16_t result = 0;
	while (m_pkHead) {
		ShadowSceneLight* m_pObject = m_pkHead->m_element.m_pObject;
		m_pkHead = m_pkHead->m_pkNext;
		if (m_pObject) {
			if (m_pObject->usFrustumCull != 255) {
				++result;
			}
		}
	}
	return result;
}

// Starts the shadow light iterator at the first shadow light with a light, or at the last one.
ShadowSceneLight* ShadowSceneNode::GetFirstShadowLight() {
	ShadowSceneLightItem* pItem = lShadowLightList.m_pkHead;
	kShadowLightListIterator = pItem;
	if (!pItem)
		return nullptr;
	kShadowLightListIterator = pItem->m_pkNext;
	ShadowSceneLight* psslight = pItem->m_element.m_pObject;
	while (kShadowLightListIterator) {
		if (psslight && psslight->spLight.m_pObject)
			break;
		pItem = kShadowLightListIterator;
		kShadowLightListIterator = pItem->m_pkNext;
		psslight = pItem->m_element.m_pObject;
	}
	return psslight;
}

// Steps the shadow light iterator to the next shadow light with a light, or to the last one.
ShadowSceneLight* ShadowSceneNode::GetNextShadowLight() {
	ShadowSceneLight* psslight = nullptr;
	if (kShadowLightListIterator) {
		ShadowSceneLightItem* pItem = kShadowLightListIterator;
		kShadowLightListIterator = pItem->m_pkNext;
		psslight = pItem->m_element.m_pObject;
		while (kShadowLightListIterator) {
			if (psslight && psslight->spLight.m_pObject)
				break;
			pItem = kShadowLightListIterator;
			kShadowLightListIterator = pItem->m_pkNext;
			psslight = pItem->m_element.m_pObject;
		}
	}
	return psslight;
}

void ShadowSceneNode::DisableAllShadowLights() {
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lShadowLightList.m_pkHead;
	while (m_pkHead) {
		ShadowSceneLight* pscenelight = m_pkHead->m_element.m_pObject;
		m_pkHead = m_pkHead->m_pkNext;
		if (pscenelight) {
			if (pscenelight->spLight.m_pObject) {
				BSShaderManager::pTextureManager->ReturnFrustumShadowTexture(pscenelight->spDynamicShadowMap.m_pObject);
				pscenelight->spDynamicShadowMap = nullptr;
				pscenelight->spLight.m_pObject->m_uFlags |= 1u;
				pscenelight->ClearRenderPasses();
			}
		}
	}
}

float ShadowSceneNode::GetLuminanceAtPoint(NiPoint3 apPoint, int32_t& inumberlights, float& afAmbientLum,
	float& afDirectionalLum, NiLight* apIgnore) {
	float fLuminance = 0.0;
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
	while (m_pkHead) {
		ShadowSceneLight* m_pObject = m_pkHead->m_element.m_pObject;
		m_pkHead = m_pkHead->m_pkNext;
		if (m_pObject) {
			fLuminance = m_pObject->GetLuminanceAtPoint(apPoint, apIgnore) + fLuminance;
			if (fLuminance > 0.0) {
				++inumberlights;
			}
		}
	}
	NiLight* v9 = pSunLight->spLight.m_pObject;
	float fAmbR = v9->m_kAmb.r;
	float b = v9->m_kAmb.b;
	double g = v9->m_kAmb.g;
	if (!(b < g)) {
		g = b;
	}
	float apIgnorea = g;
	double v12 = fAmbR;
	if (!(apIgnorea < static_cast<double>(fAmbR))) {
		v12 = apIgnorea;
	}
	float apIgnorec = v12;
	afAmbientLum = apIgnorec;
	float v13 = v9->m_kDiff.g;
	float r = v9->m_kDiff.r;
	float v15 = v9->m_kDiff.b;
	double v16 = v13;
	if (!(v15 < static_cast<double>(v13))) {
		v16 = v15;
	}
	float apIgnoreb = v16;
	double v17 = r;
	if (!(apIgnoreb < static_cast<double>(r))) {
		v17 = apIgnoreb;
	}
	float apIgnored = v17;
	afDirectionalLum = apIgnored;
	return fLuminance;
}

float ShadowSceneNode::GetViewLuminanceAtPoint(NiPoint3 apPoint, int32_t& inumberlights, float& afAmbientLum,
	float& afDirectionalLum, NiPoint3 kViewPoint, NiLight* apIgnore) {
	double v23;
	double v24;
	NiPoint3 v31;
	NiPoint3 v32;
	float v28 = 0.0;
	float v30 = 0.0;
	double v7 = static_cast<float>(0.0);
	inumberlights = 0;
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
	while (m_pkHead) {
		ShadowSceneLight* m_pObject = m_pkHead->m_element.m_pObject;
		m_pkHead = m_pkHead->m_pkNext;
		if (m_pObject) {
			v28 = m_pObject->GetViewLuminanceAtPoint(apPoint, kViewPoint, apIgnore) + v28;
			v7 = v28;
			if (v30 < static_cast<double>(v28)) {
				++inumberlights;
				v30 = v28;
			}
		}
	}
	NiDirectionalLight* v11 = static_cast<NiDirectionalLight*>(pSunLight->spLight.m_pObject);
	float g = v11->m_kAmb.g;
	v31.x = v11->m_kAmb.r;
	float b = v11->m_kAmb.b;
	v31.y = g;
	double v14 = g;
	v31.z = b;
	if (!(b < static_cast<double>(g))) {
		v14 = b;
	}
	float inumberlightsa = v14;
	double x = v31.x;
	if (!(inumberlightsa < static_cast<double>(v31.x))) {
		x = inumberlightsa;
	}
	float inumberlightsd = x;
	afAmbientLum = inumberlightsd;
	float y = v11->m_kWorldDir.y;
	v32.x = v11->m_kWorldDir.x;
	float z = v11->m_kWorldDir.z;
	v32.y = y;
	float r = v11->m_kDiff.r;
	v32.z = z;
	float v19 = v11->m_kDiff.g;
	float v29 = v7 + inumberlightsd * 0.25;
	v31.x = r;
	float v20 = v11->m_kDiff.b;
	v31.y = v19;
	double v21 = v19;
	v31.z = v20;
	if (!(v20 < static_cast<double>(v19))) {
		v21 = v20;
	}
	float inumberlightsb = v21;
	double v22 = v31.x;
	if (!(inumberlightsb < static_cast<double>(v31.x))) {
		v22 = inumberlightsb;
	}
	float inumberlightsc = v22;
	afDirectionalLum = inumberlightsc;
	v31.x = kViewPoint.x - apPoint.x;
	v31.y = kViewPoint.y - apPoint.y;
	v31.z = kViewPoint.z - apPoint.z;
	v31.Unitize();
	v32.Unitize();
	float kViewPointa = v31.x * 0.75;
	float kViewPoint_4 = v31.y * 0.75;
	float kViewPoint_8 = 0.75 * v31.z;
	float a8b = v32.y * kViewPoint_4 + kViewPointa * v32.x + v32.z * kViewPoint_8;
	float a8a = a8b + 0.25;
	if (!(a8a > 1.0)) {
		v24 = a8a;
		v23 = 1.0;
	} else {
		v23 = 1.0;
		v24 = 1.0;
	}
	float a8c = v24;
	double v25 = a8c;
	if (a8c < 0.0) {
		v25 = 0.0;
	}
	float a8d = v25;
	float a8e = v23 - a8d;
	return a8e * inumberlightsc + v29;
}

// The exe's cleanup-only C++ EH frame is left out.
void ShadowSceneNode::UpdateDynamicShadows(BSCullingProcess& arCuller) {
	ShadowSceneNode* v2 = this;
	TLSData* v17 = GetThreadTLSData();
	MEM_CONTEXT eMemContext = v17->eMemContext;
	v17->eMemContext = MEM_CONTEXT_SHADOWS;
	BSCubeMapCamera* spCubeMapCamera = this->spCubeMapCamera;
	if (!spCubeMapCamera) {
		BSCubeMapCamera* v5;
		BSCubeMapCamera* v4 = static_cast<BSCubeMapCamera*>(NiMemObject::operator new(sizeof(BSCubeMapCamera)));
		if (v4) {
			v5 = ::new (v4) BSCubeMapCamera(0);
		} else {
			v5 = 0;
		}
		BSCubeMapCamera* v6 = v2->spCubeMapCamera;
		if (v6 == v5) {
			goto LABEL_11;
		}
		if (v6) {
			NiDecRefCount(v6);
		}
		this->spCubeMapCamera = v5;
		if (v5) {
			NiIncRefCount(v5);
		LABEL_11:;
			v2 = this;
			goto LABEL_12;
		}
		v2 = this;
	}
LABEL_12:;
	v2->spCubeMapCamera->eCubeMapType = BSCubeMapCamera::BSCMC_CUBEMAP;
	if (v2->lShadowLightList.m_uiCount) {
		NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = v2->lShadowLightList.m_pkHead;
		int32_t v8 = 0;
		while (m_pkHead) {
			ShadowSceneLight* plight = m_pkHead->m_element.m_pObject;
			m_pkHead = m_pkHead->m_pkNext;
			if (plight) {
				NiLight* m_pObject = plight->spLight.m_pObject;
				if (m_pObject) {
					if ((m_pObject->m_uFlags & 1) != 0) {
						plight->RemoveAllTriShapes();
					} else {
						NiUpdateData UpdateData;
						UpdateData.fTime = 0.0f;
						UpdateData.bUpdateControllers = 0;
						UpdateData.bParallelUpdate = 0;
						UpdateData.bFoundParticles = 0;
						UpdateData.bFoundMorphController = 0;
						UpdateData.bSceneGraphChange = 0;
						m_pObject->Update(UpdateData);
						if (plight->usFrustumCull != 255) {
							BSShaderManager::pShadowMapLight = plight;
							plight->MTRenderFrustumDepthMap(arCuller, v8++);
						}
					}
				}
			}
		}
		for (int32_t i = v8 + 17; i < 21; ++i) {
			MTRenderingSystem_Instance.WaitOnThreadStage(MTRenderingSystem::THREAD_2, i);
		}
		v2 = this;
	}
	BSCubeMapCamera* v12 = v2->spCubeMapCamera;
	BSRenderedTexture* spRenderedCubeMap = v12->spRenderedCubeMap;
	BSRenderedTexture** p_spRenderedCubeMap = &v12->spRenderedCubeMap;
	if (spRenderedCubeMap) {
		NiDecRefCount(spRenderedCubeMap);
		*p_spRenderedCubeMap = 0;
		v17->eMemContext = eMemContext;
	} else {
		v17->eMemContext = eMemContext;
	}
}

void ShadowSceneNode::SetLightCastShadows(NiLight* apLight, int32_t abCastShadows) {
	NiTListItem<NiPointer<ShadowSceneLight>>* m_pkHead = lLightList.m_pkHead;
	while (m_pkHead) {
		ShadowSceneLight* m_pObject = m_pkHead->m_element.m_pObject;
		m_pkHead = m_pkHead->m_pkNext;
		if (m_pObject && m_pObject->spLight.m_pObject == apLight) {
			m_pObject->SetCastShadow(abCastShadows);
			return;
		}
	}
}

// Moves the fading-in fence, then the active fence, to the head of the shadow lights (adding each fence the first
// time), then sorts the lights after the fading-in fence: fading-out lights before the active fence, lights faded in
// past 0.1 after it, and other visible lights next to the fading-in fence (the player's shadow before it).
void ShadowSceneNode::SortShadowLights(ShadowSceneLight* apPlayerShadow) {
	if (kShadowLightFadingInFence) {
		lShadowLightList.MoveToBefore(kShadowLightFadingInFence, lShadowLightList.m_pkHead);
	} else {
		lShadowLightList.AddHead(kShadowLightFadingInFenceObject);
		kShadowLightFadingInFence = lShadowLightList.m_pkHead;
	}
	if (kShadowLightActiveFence) {
		lShadowLightList.MoveToBefore(kShadowLightActiveFence, lShadowLightList.m_pkHead);
	} else {
		lShadowLightList.AddHead(kShadowLightActiveFenceObject);
		kShadowLightActiveFence = lShadowLightList.m_pkHead;
	}

	ShadowSceneLightItem* pFadingInFence = kShadowLightFadingInFence;
	if (!pFadingInFence)
		return;
	ShadowSceneLightItem* pItem = pFadingInFence->m_pkNext;
	if (!pItem)
		return;

	// The exe keeps the fade time on the x87 stack and reloads it only after it moves a light.
	double fFadeTime = BSShaderManager::fShadowFadeTime;
	do {
		ShadowSceneLightItem* pCurrent = pItem;
		ShadowSceneLight* pLight = pCurrent->m_element.m_pObject;
		pItem = pItem->m_pkNext;
		if (!pLight)
			continue;

		if (pLight->fShadowLODDimmerTarget <= 0.10000000149011612 && pLight->fShadowLODDimmerElapsed < fFadeTime) {
			ShadowSceneLightItem* pActiveFence = kShadowLightActiveFence;
			if (pCurrent != pActiveFence) {
				// EXE-INLINED 0xB702E0
				if (lShadowLightList.m_pkHead == pCurrent)
					lShadowLightList.m_pkHead = pCurrent->m_pkNext;
				if (lShadowLightList.m_pkHead == pActiveFence)
					lShadowLightList.m_pkHead = pCurrent;
				if (lShadowLightList.m_pkTail == pCurrent)
					lShadowLightList.m_pkTail = pCurrent->m_pkPrev;
				if (pCurrent->m_pkNext)
					pCurrent->m_pkNext->m_pkPrev = pCurrent->m_pkPrev;
				if (pCurrent->m_pkPrev)
					pCurrent->m_pkPrev->m_pkNext = pCurrent->m_pkNext;
				ShadowSceneLightItem* pPrev = pActiveFence->m_pkPrev;
				pCurrent->m_pkPrev = pPrev;
				pCurrent->m_pkNext = pActiveFence;
				if (pPrev)
					pPrev->m_pkNext = pCurrent;
				pActiveFence->m_pkPrev = pCurrent;
				fFadeTime = BSShaderManager::fShadowFadeTime;
			}
			continue;
		}

		if (pLight->fShadowLODDimmerTarget >= 0.8999999761581421 && pLight->fShadowLODDimmerElapsed < fFadeTime) {
			double fRatio = pLight->fShadowLODDimmerElapsed / fFadeTime;
			if (!(pLight->fShadowLODDimmerTarget > 0.0f))
				fRatio = 1.0 - fRatio;
			float fFade = fRatio;
			if (fFade > 0.10000000149011612) {
				lShadowLightList.MoveToAfter(pCurrent, kShadowLightActiveFence);
				fFadeTime = BSShaderManager::fShadowFadeTime;
				continue;
			}
		}

		if (!(pLight->spLight.m_pObject->m_uFlags & NiAVObject::APP_CULLED) &&
			(!(pLight->fShadowLODDimmerTarget >= 0.8999999761581421) ||
				!(pLight->fShadowLODDimmerElapsed < fFadeTime))) {
			if (pLight == apPlayerShadow)
				lShadowLightList.MoveToBefore(pCurrent, kShadowLightFadingInFence);
			else
				lShadowLightList.MoveToAfter(pCurrent, kShadowLightFadingInFence);
			fFadeTime = BSShaderManager::fShadowFadeTime;
		}
	} while (pItem);
}

bool ShadowSceneNode::UpdateQueuedObjectLightingStart(NiAVObject* apObject, ShadowSceneNode* apSceneNode,
	int32_t abActiveLightsOnly, bool abSingleLightList) {
	if (!apObject)
		return true;

	NiNode* pParent = apObject->m_pkParent;
	BSMultiBoundRoom* pRoom = nullptr;
	BSPortalSharedNode* pPortalNode = nullptr;
	if (pParent) {
		if (pParent->GetRTTI() == &BSMultiBoundRoom::ms_RTTI)
			pRoom = static_cast<BSMultiBoundRoom*>(pParent);
		else if (pParent->GetRTTI() == &BSPortalSharedNode::ms_RTTI)
			pPortalNode = static_cast<BSPortalSharedNode*>(pParent);
	}

	if (!abSingleLightList) {
		if (apObject->IsTriBasedGeom())
			UpdateQueuedObjectLighting(static_cast<NiTriBasedGeom*>(apObject), apSceneNode, abActiveLightsOnly, pRoom,
				pPortalNode);
		else
			UpdateQueuedObjectLighting(apObject, apSceneNode, abActiveLightsOnly, pRoom, pPortalNode);
		return true;
	}

	// One null-terminated list of the lights that reach the object's bound, brightest first.
	ScrapHeap* pScrapHeap = MemoryManager_Singleton.GetThreadScrapHeap();
	int32_t iCount = 0;
	const NiBound* pWorldBound = apObject->m_kWorldBound;
	if (!pWorldBound)
		pWorldBound = &NiAVObject::NullBoundS;
	NiBound kBound = *pWorldBound;

	ShadowSceneLight** ppLights;
	if (pRoom) {
		ppLights =
			static_cast<ShadowSceneLight**>(pScrapHeap->Allocate(pRoom->kLights.iSize * sizeof(ShadowSceneLight*) + 4,
				ScrapHeap::ALIGNMENT));
		for (uint32_t i = 0; i < pRoom->kLights.iSize; i++) {
			ShadowSceneLight* pLight = pRoom->kLights.pBuffer[i];
			if (pLight &&
				(!static_cast<uint8_t>(abActiveLightsOnly) || pLight->usFrustumCull != ShadowSceneLight::SSL_ALL) &&
				pLight->BoundWithinLightRadius(kBound, pLight->spLight, 1.0f)) {
				ppLights[iCount++] = pLight;
				pLight->GetLuminanceToBound(&kBound);
			}
		}
	} else if (pPortalNode) {
		ppLights = static_cast<ShadowSceneLight**>(pScrapHeap
				->Allocate(pPortalNode->kLights.iSize * sizeof(ShadowSceneLight*) + 4, ScrapHeap::ALIGNMENT));
		for (uint32_t i = 0; i < pPortalNode->kLights.iSize; i++) {
			ShadowSceneLight* pLight = pPortalNode->kLights.pBuffer[i];
			if (pLight &&
				(!static_cast<uint8_t>(abActiveLightsOnly) || pLight->usFrustumCull != ShadowSceneLight::SSL_ALL) &&
				pLight->BoundWithinLightRadius(kBound, pLight->spLight, 1.0f)) {
				ppLights[iCount++] = pLight;
				pLight->GetLuminanceToBound(&kBound);
			}
		}
	} else {
		ppLights = static_cast<ShadowSceneLight**>(pScrapHeap
				->Allocate(apSceneNode->lLightList.m_uiCount * sizeof(ShadowSceneLight*) + 4, ScrapHeap::ALIGNMENT));
		for (ShadowSceneLightItem* pItem = apSceneNode->lLightList.m_pkHead; pItem;) {
			ShadowSceneLight* pLight = pItem->m_element;
			pItem = pItem->m_pkNext;
			if (pLight &&
				(!static_cast<uint8_t>(abActiveLightsOnly) || pLight->usFrustumCull != ShadowSceneLight::SSL_ALL) &&
				pLight->BoundWithinLightRadius(kBound, pLight->spLight, 1.0f)) {
				ppLights[iCount++] = pLight;
				pLight->GetLuminanceToBound(&kBound);
			}
		}
	}

	if (iCount > 0)
		std::qsort(ppLights, iCount, sizeof(ShadowSceneLight*), &ShadowSceneLight::SortByLuminance);
	ppLights[iCount] = nullptr;
	UpdateQueuedObjectLightingList(apObject, apSceneNode, ppLights, abActiveLightsOnly);
	pScrapHeap->Deallocate(ppLights);
	return true;
}

bool ShadowSceneNode::UpdateQueuedObjectLighting(NiTriBasedGeom* apGeom, ShadowSceneNode* apSceneNode,
	int32_t abActiveLightsOnly, BSMultiBoundRoom* apRoom, BSPortalSharedNode* apPortal) {
	NiProperty* pShade = apGeom->m_PropertyState.m_aspProps[NiProperty::SHADE].m_pObject;
	if (!pShade)
		return true;

	// The engine's inline casts to BSShaderProperty and BSShaderLightingProperty, by the shader property type.
	NiShadeProperty* pShadeProperty = static_cast<NiShadeProperty*>(pShade);
	if (pShadeProperty->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_UNKNOWN)
		return false;
	if (static_cast<uint32_t>(pShadeProperty->iShaderPropertyType - BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_FIRST) >
		BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_FIRST)
		return false;
	BSShaderLightingProperty* pShaderProperty = static_cast<BSShaderLightingProperty*>(pShadeProperty);

	ShadowSceneNode* pSceneNode = apSceneNode;
	if (!pSceneNode) {
		pSceneNode = BSShaderManager::GetAccumulator()->pActiveShadowSceneNode;
		if (!pSceneNode)
			pSceneNode = BSShaderManager::pShadowSceneNode[BSShaderManager::BSSM_SSN_WORLD];
	}

	pShaderProperty->FenceAllLights();
	const NiBound* pWorldBound = apGeom->m_kWorldBound;
	if (!pWorldBound)
		pWorldBound = &NiAVObject::NullBoundS;
	NiBound kBound = *pWorldBound;

	if (apRoom) {
		for (uint32_t i = 0; i < apRoom->kLights.iSize; ++i) {
			ShadowSceneLight* pLight = apRoom->kLights.pBuffer[i];
			if (pLight && pLight->BoundWithinLightRadius(kBound, pLight->spLight.m_pObject, 1.0f))
				pLight->AddTriGeom(apGeom);
		}
	} else if (apPortal) {
		for (uint32_t i = 0; i < apPortal->kLights.iSize; ++i) {
			ShadowSceneLight* pLight = apPortal->kLights.pBuffer[i];
			if (pLight && pLight->BoundWithinLightRadius(kBound, pLight->spLight.m_pObject, 1.0f))
				pLight->AddTriGeom(apGeom);
		}
	} else {
		ShadowSceneLightItem* pItem = pSceneNode->lLightList.m_pkHead;
		while (pItem) {
			ShadowSceneLight* pLight = pItem->m_element.m_pObject;
			pItem = pItem->m_pkNext;
			if (pLight && pLight->BoundWithinLightRadius(kBound, pLight->spLight.m_pObject, 1.0f))
				pLight->AddTriGeom(apGeom);
		}
	}

	pShaderProperty->RemoveFencedLights(apGeom);
	apGeom->m_uFlags = static_cast<NiAVObject::NiFlags>(apGeom->m_uFlags | NiAVObject::WORLD_BOUND_CHANGE);
	return true;
}

bool ShadowSceneNode::UpdateQueuedObjectLighting(NiAVObject* apObject, ShadowSceneNode* apSceneNode,
	int32_t abActiveLightsOnly, BSMultiBoundRoom* apRoom, BSPortalSharedNode* apPortal) {
	if (!apObject)
		return true;

	const NiBound* pWorldBound = apObject->m_kWorldBound;
	if (!pWorldBound || pWorldBound->m_fRadius == 0.0f || (apObject->m_uFlags & NiAVObject::APP_CULLED) ||
		!apObject->IsNode())
		return false;

	NiNode* pNode = apObject->IsNode();
	bool bResult = true;
	// The exe inlines the static DynamicCast's null test and calls the member DynamicCast out of line.
	NiSwitchNode* pSwitch =
		pNode ? static_cast<NiSwitchNode*>(pNode->DynamicCastMember(&NiSwitchNode::ms_RTTI)) : nullptr;
	if (pSwitch) {
		NiAVObject* pChild = pSwitch->GetActiveChild();
		if (pChild)
			return UpdateQueuedObjectLighting(pChild, apSceneNode, abActiveLightsOnly, apRoom, apPortal);
		return bResult;
	}

	for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
		NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
		if (pChild) {
			if (pChild->IsTriBasedGeom())
				bResult &= UpdateQueuedObjectLighting(static_cast<NiTriBasedGeom*>(pChild), apSceneNode,
					abActiveLightsOnly, apRoom, apPortal);
			else
				bResult &= UpdateQueuedObjectLighting(pChild, apSceneNode, abActiveLightsOnly, apRoom, apPortal);
		}
	}
	return bResult;
}

// The exe also tests the address of apGeom's property state for null, which a real geometry never fails.
bool ShadowSceneNode::UpdateQueuedObjectLightingList(NiTriBasedGeom* apGeom, ShadowSceneNode* apSceneNode,
	ShadowSceneLight** apLightList, int32_t abActiveLightsOnly) {
	NiProperty* pShade = apGeom->m_PropertyState.m_aspProps[NiProperty::SHADE].m_pObject;
	if (!pShade)
		return true;

	// The engine's inline casts to BSShaderProperty and BSShaderLightingProperty, by the shader property type.
	NiShadeProperty* pShadeProperty = static_cast<NiShadeProperty*>(pShade);
	if (pShadeProperty->iShaderPropertyType == BSShaderManager::BSSM_PROPERTYTYPE_UNKNOWN)
		return false;
	if (static_cast<uint32_t>(pShadeProperty->iShaderPropertyType - BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_FIRST) >
		BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_FIRST)
		return false;
	BSShaderLightingProperty* pShaderProperty = static_cast<BSShaderLightingProperty*>(pShadeProperty);

	pShaderProperty->FenceAllLights();
	for (uint32_t i = 0; apLightList[i]; ++i) {
		ShadowSceneLight* pLight = apLightList[i];
		if (pLight)
			pLight->AddTriGeom(apGeom);
	}
	pShaderProperty->RemoveFencedLights(apGeom);
	apGeom->m_uFlags = static_cast<NiAVObject::NiFlags>(apGeom->m_uFlags | NiAVObject::WORLD_BOUND_CHANGE);
	return true;
}

bool ShadowSceneNode::UpdateQueuedObjectLightingList(NiAVObject* apObject, ShadowSceneNode* apSceneNode,
	ShadowSceneLight** apLightList, int32_t abActiveLightsOnly) {
	if (!apObject)
		return true;

	const NiBound* pWorldBound = apObject->m_kWorldBound;
	if (!pWorldBound || pWorldBound->m_fRadius == 0.0f || (apObject->m_uFlags & NiAVObject::APP_CULLED) ||
		!apObject->IsNode())
		return false;

	NiNode* pNode = apObject->IsNode();
	bool bResult = true;
	// The exe inlines the static DynamicCast's null test and calls the member DynamicCast out of line.
	NiSwitchNode* pSwitch =
		pNode ? static_cast<NiSwitchNode*>(pNode->DynamicCastMember(&NiSwitchNode::ms_RTTI)) : nullptr;
	if (pSwitch) {
		NiAVObject* pChild = pSwitch->GetActiveChild();
		if (pChild)
			return UpdateQueuedObjectLightingList(pChild, apSceneNode, apLightList, abActiveLightsOnly);
		return bResult;
	}

	for (uint32_t i = 0; i < pNode->m_kChildren.m_usSize; ++i) {
		NiAVObject* pChild = pNode->m_kChildren.m_pBase[i].m_pObject;
		if (pChild) {
			if (pChild->IsTriBasedGeom())
				bResult &= UpdateQueuedObjectLightingList(static_cast<NiTriBasedGeom*>(pChild), apSceneNode,
					apLightList, abActiveLightsOnly);
			else
				bResult &= UpdateQueuedObjectLightingList(pChild, apSceneNode, apLightList, abActiveLightsOnly);
		}
	}
	return bResult;
}

// Drops the geometry from every light of its lighting property: GetNextLight walks the fence one light per call. The
// exe passes the address of its own parameter as the iterator, so the slot still holds the geometry pointer when the
// first light is looked up; the geometry handed to RemoveTriGeom is a copy the exe keeps in a register.
void ShadowSceneNode::RemoveLightsFromGeom(NiTriBasedGeom* apGeom) {
	NiTriBasedGeom* pGeom = apGeom;
	if (!apGeom)
		return;

	NiProperty* pShade = apGeom->m_PropertyState.m_aspProps[NiProperty::SHADE].m_pObject;
	if (!pShade)
		return;

	// The engine's inline cast to BSShaderLightingProperty, by the shader property type.
	NiShadeProperty* pShadeProperty = static_cast<NiShadeProperty*>(pShade);
	if (static_cast<uint32_t>(pShadeProperty->iShaderPropertyType - BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_FIRST) >
		BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_LAST - BSShaderManager::BSSM_PROPERTYTYPE_LIGHTING_FIRST)
		return;
	BSShaderLightingProperty* pShaderProperty = static_cast<BSShaderLightingProperty*>(pShadeProperty);

	ShadowSceneLight* pLight = pShaderProperty->GetNextLight(reinterpret_cast<void*&>(apGeom));
	while (pLight) {
		pLight->RemoveTriGeom(pGeom, nullptr);
		pLight = pShaderProperty->GetNextLight(reinterpret_cast<void*&>(apGeom));
	}
}

void BSPortalGraph::AddRoomToAccumList(BSMultiBoundRoom* apRoom) {
	if (apRoom)
		apRoom->IncRefCount();

	auto* pItem = lMultiBoundRoomAccumList.m_pkHead;
	bool bAdd = true;
	while (pItem) {
		auto* pElement = &pItem->m_element;
		pItem = pItem->m_pkNext;
		if (apRoom == pElement->m_pObject) {
			bAdd = false;
			break;
		}
	}
	NiRefObject* pRoom = apRoom;
	if (pRoom) {
		pRoom->DecRefCount();
	}

	if (bAdd) {
		NiPointer<BSMultiBoundRoom> spRoom;
		spRoom.m_pObject = apRoom;
		if (apRoom)
			apRoom->IncRefCount();
		lMultiBoundRoomAccumList.AddTail(spRoom);
		spRoom.m_pObject = nullptr;
		if (pRoom) {
			pRoom->DecRefCount();
		}
	}
}
