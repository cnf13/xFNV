#include "Gamebryo2.2/CoreLibs/NiMain/NiDynamicEffect.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

NiRTTI NiDynamicEffect::ms_RTTI("NiDynamicEffect", &NiAVObject::ms_RTTI);

NiDynamicEffect::NiDynamicEffect()
  : m_bOn(true), m_ucEffectType(0xFF), m_iIndex(InterlockedIncrementInt32(&ms_iNextIndex)), m_uiPushCount(0),
    m_uiRevID(1) {}

NiDynamicEffect::~NiDynamicEffect() {
    DetachAllUnaffectedNodes();
}

void NiDynamicEffect::LoadBinary(NiStream& kStream) {
    NiAVObject::LoadBinary(kStream);
    // The asm reads into the incoming stream-pointer stack slot, preserving unread bytes on a short read.
    uint32_t uiReadValue = reinterpret_cast<uintptr_t>(&kStream);
    if (kStream.m_uiNifFileVersion >= 0x0A010066) {
        NiBinaryStream* pkIstr = kStream.m_pkIstr;
        uint32_t uiSize = 1;
        pkIstr->BinaryRead(&uiReadValue, 1, &uiSize, 1);
        m_bOn = static_cast<uint8_t>(uiReadValue) != 0;
    }
    if (kStream.m_uiNifFileVersion < 0x04010000) {
        NiBinaryStream* pkIstr = kStream.m_pkIstr;
        uint32_t uiSize = 4;
        pkIstr->BinaryRead(&uiReadValue, 4, &uiSize, 1);
        for (int32_t i = 0; i < static_cast<int32_t>(uiReadValue); ++i) {
            pkIstr = kStream.m_pkIstr;
            uint32_t uiLinkID;
            uiSize = 4;
            pkIstr->BinaryRead(&uiLinkID, 4, &uiSize, 1);
        }
    }
    if (kStream.m_uiNifFileVersion >= 0x0A00010E) {
        kStream.ReadMultipleLinkIDs();
    }
}

// From 10.0.1.14 on, the file lists the unaffected nodes; each non-null one is added at the head. A node is taken from
// the shared pointer pool under its spin lock (refilled when empty), which hands out untyped memory the list uses as
// its item.
void NiDynamicEffect::LinkObject(NiStream& kStream) {
    using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
    NiAVObject::LinkObject(kStream);
    if (kStream.m_uiNifFileVersion < 0x0A00010E) {
        return;
    }

    uint32_t uiSize = kStream.GetNumberOfLinkIDs();
    while (uiSize != 0) {
        uiSize--;
        NiNode* pkNode = static_cast<NiNode*>(kStream.GetObjectFromLinkID());
        if (!pkNode) {
            continue;
        }
        // EXE-INLINED 0x43A010
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
        // EXE-INLINED none: NiTPointerListBase<NiNode*>::AddHead
        NiTListItem<NiNode*>* pkItem = reinterpret_cast<NiTListItem<NiNode*>*>(pkNew);
        pkItem->m_element = pkNode;
        pkItem->m_pkPrev = nullptr;
        pkItem->m_pkNext = m_kUnaffectedNodeList.m_pkHead;
        if (m_kUnaffectedNodeList.m_pkHead) {
            m_kUnaffectedNodeList.m_pkHead->m_pkPrev = pkItem;
        }
        else {
            m_kUnaffectedNodeList.m_pkTail = pkItem;
        }
        m_kUnaffectedNodeList.m_uiCount++;
        m_kUnaffectedNodeList.m_pkHead = pkItem;
    }
}

bool NiDynamicEffect::RegisterStreamables(NiStream& kStream) {
    return NiAVObject::RegisterStreamables(kStream);
}

// Writes the on flag, the unaffected-node count and each unaffected node's link ID, newest node first. The count is
// compared signed.
void NiDynamicEffect::SaveBinary(NiStream& kStream) {
    NiAVObject::SaveBinary(kStream);
    uint8_t ucOn = m_bOn;
    uint32_t uiSize = 1;
    kStream.m_pkOstr->BinaryWrite(&ucOn, 1, &uiSize, 1);
    int32_t iListSize = static_cast<int32_t>(m_kUnaffectedNodeList.m_uiCount);
    NiBinaryStream* pkOstr = kStream.m_pkOstr;
    uiSize = 4;
    pkOstr->BinaryWrite(&iListSize, 4, &uiSize, 1);
    if (iListSize > 0) {
        // Walks the nodes directly: element first, then the previous link, then the call.
        NiTListItem<NiNode*>* pkPos = m_kUnaffectedNodeList.m_pkTail;
        while (pkPos) {
            NiNode* pkNode = pkPos->m_element;
            pkPos = pkPos->m_pkPrev;
            kStream.SaveLinkID(pkNode);
        }
    }
}

// The second list position is advanced without a null test, as the game does.
bool NiDynamicEffect::IsEqual(NiObject* pkObject) {
    if (!NiAVObject::IsEqual(pkObject)) {
        return false;
    }

    NiDynamicEffect* pkEffect = static_cast<NiDynamicEffect*>(pkObject);
    if (m_kAffectedNodeList.m_uiCount != pkEffect->m_kAffectedNodeList.m_uiCount) {
        return false;
    }
    if (m_kAffectedNodeList.m_uiCount > 0) {
        NiTListItem<NiNode*>* pkPos0 = m_kAffectedNodeList.m_pkHead;
        NiTListItem<NiNode*>* pkPos1 = pkEffect->m_kAffectedNodeList.m_pkHead;
        while (pkPos0) {
            NiNode* pkNode0 = pkPos0->m_element;
            pkPos0 = pkPos0->m_pkNext;
            NiNode* pkNode1 = pkPos1->m_element;
            pkPos1 = pkPos1->m_pkNext;
            if ((pkNode0 && !pkNode1) || (!pkNode0 && pkNode1)) {
                return false;
            }
        }
    }

    if (m_kUnaffectedNodeList.m_uiCount != pkEffect->m_kUnaffectedNodeList.m_uiCount) {
        return false;
    }
    if (m_kUnaffectedNodeList.m_uiCount > 0) {
        NiTListItem<NiNode*>* pkPos0 = m_kUnaffectedNodeList.m_pkHead;
        NiTListItem<NiNode*>* pkPos1 = pkEffect->m_kUnaffectedNodeList.m_pkHead;
        while (pkPos0) {
            NiNode* pkNode0 = pkPos0->m_element;
            pkPos0 = pkPos0->m_pkNext;
            NiNode* pkNode1 = pkPos1->m_element;
            pkPos1 = pkPos1->m_pkNext;
            if ((pkNode0 && !pkNode1) || (!pkNode0 && pkNode1)) {
                return false;
            }
        }
    }

    return true;
}

void NiDynamicEffect::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiDynamicEffect* v2 = this;
    NiAVObject::GetViewerStrings(pkStrings);
    char* ViewerString = NiGetViewerString(NiDynamicEffect::ms_RTTI.GetName());
    uint32_t m_usSize = pkStrings->m_usSize;
    uint32_t m_usMaxSize = pkStrings->m_usMaxSize;
    char* _i = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        pkStrings->SetSize(m_usSize + pkStrings->m_usGrowBy);
    }
    pkStrings->SetAt(m_usSize, _i);
    if (v2->m_kAffectedNodeList.m_uiCount) {
        char* v13;
        uint32_t v14;
        char** v15;
        char acPrefix[64];
        NiTListItem<NiNode*>* m_pkHead = v2->m_kAffectedNodeList.m_pkHead;
        int32_t i_2 = 0;
        if (m_pkHead) {
            do {
                NiNode* pkNode = m_pkHead->m_element;
                m_pkHead = m_pkHead->m_pkNext;
                NiSprintf(acPrefix, 64u, "affected node[%d]", i_2);
                char* ViewerString_8 =
                    NiGetViewerString(static_cast<const char*>(acPrefix), static_cast<const void*>(pkNode));
                uint32_t v9 = pkStrings->m_usSize;
                if (v9 >= pkStrings->m_usMaxSize) {
                    pkStrings->SetSize(v9 + pkStrings->m_usGrowBy);
                }
                if (v9 < pkStrings->m_usSize) {
                    if (ViewerString_8) {
                        if (!pkStrings->m_pBase[v9]) {
                            ++pkStrings->m_usESize;
                        }
                    }
                    else {
                        if (pkStrings->m_pBase[v9]) {
                            --pkStrings->m_usESize;
                        }
                    }
                }
                else {
                    pkStrings->m_usSize = v9 + 1;
                    if (ViewerString_8) {
                        ++pkStrings->m_usESize;
                    }
                }
                char** m_pBase = pkStrings->m_pBase;
                ++i_2;
                m_pBase[v9] = ViewerString_8;
            }
            while (m_pkHead);
            v2 = this;
        }
        for (NiTListItem<NiNode*>* i = v2->m_kUnaffectedNodeList.m_pkHead; i; v15[v14] = v13) {
            NiNode* pkNode_2 = i->m_element;
            i = i->m_pkNext;
            NiSprintf(acPrefix, 64u, "unaffected node[%d]", i_2);
            v13 = NiGetViewerString(static_cast<const char*>(acPrefix), static_cast<const void*>(pkNode_2));
            v14 = pkStrings->m_usSize;
            if (v14 >= pkStrings->m_usMaxSize) {
                pkStrings->SetSize(v14 + pkStrings->m_usGrowBy);
            }
            if (v14 < pkStrings->m_usSize) {
                if (v13) {
                    if (!pkStrings->m_pBase[v14]) {
                        ++pkStrings->m_usESize;
                    }
                }
                else {
                    if (pkStrings->m_pBase[v14]) {
                        --pkStrings->m_usESize;
                    }
                }
            }
            else {
                pkStrings->m_usSize = v14 + 1;
                if (v13) {
                    ++pkStrings->m_usESize;
                }
            }
            v15 = pkStrings->m_pBase;
            ++i_2;
        }
    }
}

// Empties the unaffected-node list from its head, returning each node to the shared pointer pool under the pool's spin
// lock; the pool treats a freed list node as untyped memory, an AllocNode.
void NiDynamicEffect::DetachAllUnaffectedNodes() {
    using PoolAllocator = NiTAbstractPoolAllocator<uint32_t>;
    while (m_kUnaffectedNodeList.m_uiCount != 0) {
        // EXE-INLINED none: NiTPointerListBase<NiNode*>::RemoveHead
        NiTListItem<NiNode*>* pkNode = m_kUnaffectedNodeList.m_pkHead;
        m_kUnaffectedNodeList.m_pkHead = pkNode->m_pkNext;
        if (m_kUnaffectedNodeList.m_pkHead) {
            m_kUnaffectedNodeList.m_pkHead->m_pkPrev = nullptr;
        }
        else {
            m_kUnaffectedNodeList.m_pkTail = nullptr;
        }
        // EXE-INLINED none: NiTPointerListBase<NiNode*>::DeleteItem
        pkNode->m_element = nullptr;
        PoolAllocator::Lock();
        PoolAllocator::AllocNode* pkFreed = reinterpret_cast<PoolAllocator::AllocNode*>(pkNode);
        pkFreed->m_pkData = nullptr;
        pkFreed->m_pkNext = PoolAllocator::ms_pkFreeMem;
        PoolAllocator::ms_pkFreeMem = pkFreed;
        PoolAllocator::Unlock();
        m_kUnaffectedNodeList.m_uiCount--;
    }
}

void NiDynamicEffect::CopyMembers(NiDynamicEffect* pkDest, NiCloningProcess& kCloning) {
    NiAVObject::CopyMembers(pkDest, kCloning);
    pkDest->m_uiPushCount = m_uiPushCount;
    pkDest->m_uiRevID = m_uiRevID;
    pkDest->m_bOn = m_bOn;
}

void NiDynamicEffect::AttachAffectedNode(NiNode* pkAffectedNode) {}
