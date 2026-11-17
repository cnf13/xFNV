#include "BSMain/BSNiNode.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCollisionObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "SDK/MSVC/string.h"

#include <cmath>

NiRTTI BSNiNode::ms_RTTI("BSNiNode", &NiNode::ms_RTTI);

BSNiNode::BSNiNode() : NiNode(0) {}

// Takes apNode's place under its parent: with abKeepChildren and apNode animated (controllers, or a billboard),
// apNode is renamed "...Anim" and becomes the only child; otherwise its controllers, collision object, children,
// properties, transform and extra data move here and apNode is detached.
BSNiNode::BSNiNode(NiNode* apNode, bool abKeepChildren) : NiNode(0) {
	bool bCopy = true;
	NiNode* pParent = apNode->m_pkParent;
	SetName(apNode->m_kName);
	if (abKeepChildren && (apNode->m_spControllers || apNode->IsKindOfMember(&NiBillboardNode::ms_RTTI))) {
		bCopy = false;
		SetNewName(apNode, "Anim");
	}
	if (bCopy) {
		NiTimeController* pNext;
		for (NiTimeController* pController = apNode->m_spControllers; pController; pController = pNext) {
			pNext = pController->m_spNext.m_pObject;
			bool bManager = false;
			for (const NiRTTI* pRTTI = pController->GetRTTI(); pRTTI; pRTTI = pRTTI->GetBaseRTTI()) {
				if (pRTTI == &NiControllerManager::ms_RTTI) {
					bManager = true;
					break;
				}
			}
			if (bManager) {
				pController->NiTimeController::SetTarget(this);
			} else {
				pController->SetTarget(this);
			}
		}
		SetCollisionObject(apNode->m_spCollisionObject.m_pObject);
		apNode->SetCollisionObject(nullptr);
		for (uint32_t i = 0; i < apNode->m_kChildren.m_usSize; ++i) {
			NiAVObject* pChild = apNode->m_kChildren.m_pBase[i].m_pObject;
			if (pChild) {
				NiNode::AttachChild(pChild, true);
			}
		}
		while (apNode->m_kPropertyList.m_uiCount) {
			NiProperty* pProperty = apNode->m_kPropertyList.m_pkHead->m_element.m_pObject;
			AttachProperty(pProperty);
			apNode->DetachProperty(pProperty);
		}
		m_kLocal.m_Translate = apNode->m_kLocal.m_Translate;
		m_kLocal.m_Rotate = apNode->m_kLocal.m_Rotate;
		m_kLocal.m_fScale = std::fabs(static_cast<double>(apNode->m_kLocal.m_fScale));
		EnterCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
		SetExtraDataSize(apNode->m_usExtraDataSize);
		while (apNode->m_usExtraDataSize) {
			AddExtraData(static_cast<NiObjectNET*>(apNode)->m_ppkExtra[0]);
			apNode->RemoveExtraDataAt(0);
		}
		LeaveCriticalSection(&NiObjectNET::ExtraDataCrit.m_kCriticalSection);
		if (pParent) {
			NiPointer<NiAVObject> spDetachedChild;
			pParent->DetachChild(apNode, spDetachedChild);
		}
		BSNiNode::ReparentSkinInstances(this, this);
	} else {
		NiNode::AttachChild(apNode, true);
	}
	if (pParent) {
		pParent->AttachChild(this, true);
	}
	NiNode::UpdateWorldBound();
}

BSNiNode::~BSNiNode() {}

const NiRTTI* BSNiNode::GetRTTI() const {
	return &ms_RTTI;
}

NiObject* BSNiNode::CreateClone(NiCloningProcess& arCloneData) {
	void* pMemory = NiMemObject::operator new(sizeof(BSNiNode));
	BSNiNode* pkClone = pMemory ? ::new (pMemory) BSNiNode : nullptr;
	CopyMembers(pkClone, arCloneData);
	pkClone->UpdateWorldBound();
	return pkClone;
}

// Points every skinned geometry below apRoot at apNewParent as its skeleton root.
void BSNiNode::ReparentSkinInstances(NiNode* apRoot, NiNode* apNewParent) {
	if (!apRoot || !apNewParent) {
		return;
	}
	for (uint32_t i = 0; i < apRoot->m_kChildren.m_usSize; ++i) {
		NiAVObject* pkChild = apRoot->m_kChildren.m_pBase[i].m_pObject;
		if (!pkChild) {
			continue;
		}
		NiNode* pkNode = pkChild->IsNode();
		if (pkNode) {
			ReparentSkinInstances(pkNode, apNewParent);
		} else {
			NiGeometry* pkGeometry = pkChild->IsGeometry();
			if (pkGeometry && pkGeometry->m_spSkinInstance) {
				pkGeometry->m_spSkinInstance->m_pkRootParent = apNewParent;
			}
		}
	}
}

void BSNiNode::CopyMembers(BSNiNode* pDest, NiCloningProcess& arCloneData) {
	NiNode::CopyMembers(pDest, arCloneData);
}

void BSNiNode::SetNewName(NiNode* apNode, const char* apAppend) {
	size_t v5;
	const char* pcSrc = apNode->m_kName.GetHandle();
	if (pcSrc) {
		v5 = std::strlen(pcSrc);
	} else {
		v5 = 0;
	}
	size_t Size = v5 + std::strlen(apAppend) + 1;
	char* pcDest = static_cast<char*>(MemoryManager_Singleton.Allocate(Size));
	if (pcSrc) {
		strcpy_s(pcDest, Size, pcSrc);
	}
	strcat_s(pcDest, Size, apAppend);
}
