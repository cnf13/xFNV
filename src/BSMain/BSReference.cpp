#include "BSMain/BSReference.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

NiRTTI BSReference::ms_RTTI("BSReference", &NiObject::ms_RTTI);
NiRTTI BSNodeReferences::ms_RTTI("BSNodeReferences", &NiObject::ms_RTTI);

// The reference's name, then the object the stream's reference map holds for it; the stream switches to that object
// when the map has one.
void BSReference::LoadBinary(NiStream& arStream) {
    NiObject::LoadBinary(arStream);
    arStream.LoadCString(pName);
    BSStream* pkStream = static_cast<BSStream*>(&arStream);
    NiObjectNET* pObject = nullptr;
    if (pkStream->pObjectRefMap && pName) {
        pkStream->pObjectRefMap->GetAt(pName, pObject);
    }
    if (pObject) {
        arStream.ChangeObject(pObject);
    }
}

void BSReference::SaveBinary(NiStream& arStream) {
    NiObject::SaveBinary(arStream);
    arStream.SaveCString(pName);
}

// Equal when both names exist and match ignoring case.
bool BSReference::IsEqual(NiObject* apObject) {
    if (!NiObject::IsEqual(apObject)) {
        return false;
    }

    BSReference* pOther = static_cast<BSReference*>(apObject);
    if (!pName || !pOther->pName) {
        return false;
    }

    return NiStricmp(pName, pOther->pName) == 0;
}

void BSReference::SetName(const char* apName) {
    MemoryManager_Singleton.Deallocate(pName);
    pName = nullptr;
    if (apName) {
        size_t Size = std::strlen(apName) + 1;
        pName = static_cast<char*>(MemoryManager_Singleton.Allocate(Size));
        if (pName) {
            strcpy_s(pName, Size, apName);
        }
    }
}

NiObject* BSReference::CreateObject() {
    return new BSReference;
}

// Empties the list; the array frees its block itself.
BSNodeReferences::~BSNodeReferences() {
    RefNodeA.RemoveAll();
}

void BSNodeReferences::LoadBinary(NiStream& arStream) {
    NiObject::LoadBinary(arStream);
    arStream.ReadMultipleLinkIDs();
}

// The links come in (parent, child) pairs; each child is reattached to its parent. The objects are used as nodes
// without a type check.
void BSNodeReferences::LinkObject(NiStream& arStream) {
    NiObject::LinkObject(arStream);
    uint32_t uiCount = arStream.GetNumberOfLinkIDs();
    if (!uiCount) {
        return;
    }

    RefNodeA.SetSize(uiCount);
    uiCount >>= 1;
    for (uint32_t i = 0; i < uiCount; ++i) {
        NiNode* pParent = static_cast<NiNode*>(arStream.GetObjectFromLinkID());
        NiAVObject* pChild = static_cast<NiAVObject*>(arStream.GetObjectFromLinkID());
        if (pParent && pChild) {
            pParent->AttachChild(pChild, false);
        }
    }
}

bool BSNodeReferences::RegisterStreamables(NiStream& arStream) {
    return NiObject::RegisterStreamables(arStream);
}

// Each node is saved as a (parent, node) link pair.
void BSNodeReferences::SaveBinary(NiStream& arStream) {
    NiObject::SaveBinary(arStream);
    const uint32_t uiCount = RefNodeA.m_usESize;
    const uint32_t uiLinks = uiCount * 2;
    NiStreamSaveBinary(arStream, uiLinks);
    for (uint32_t i = 0; i < uiCount; ++i) {
        NiAVObject* pNode = RefNodeA.m_pBase[i];
        if (pNode) {
            arStream.SaveLinkID(pNode->m_pkParent);
            arStream.SaveLinkID(pNode);
        }
    }
}

// Adds the parent of a scene graph object to the list: the object must be a NiAVObject and its parent's name must be
// one the stream's reference map holds. The map is keyed by the name's handle.
bool BSNodeReferences::AddParentRef(BSStream& arStream, NiObject* apObject) {
    NiAVObject* pObject = nullptr;
    if (apObject) {
        // Walks the type's RTTI chain looking for NiAVObject's.
        const NiRTTI* pRTTI = apObject->GetRTTI();
        bool bIsAVObject = false;
        while (pRTTI) {
            if (pRTTI == &NiAVObject::ms_RTTI) {
                bIsAVObject = true;
                break;
            }
            pRTTI = pRTTI->m_pkBaseRTTI;
        }
        if (bIsAVObject) {
            pObject = static_cast<NiAVObject*>(apObject);
        }
    }
    if (!pObject) {
        return false;
    }

    NiNode* pParent = pObject->m_pkParent;
    if (!pParent) {
        return false;
    }

    const char* pParentName = pParent->m_kName.GetHandle();
    NiObjectNET* pRefObject = nullptr;
    if (arStream.pObjectRefMap && pParentName) {
        arStream.pObjectRefMap->GetAt(pParentName, pRefObject);
    }
    if (!pRefObject) {
        return false;
    }

    // SWEEP-INLINE 0xC4C8D0
    RefNodeA.Add(pObject);
    return true;
}

NiObject* BSNodeReferences::CreateObject() {
    return new BSNodeReferences;
}
