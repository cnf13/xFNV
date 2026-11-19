#include "Fallout Shared/BGSDecalNode.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderProperty.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TempEffects/BSTempEffectGeometryDecal.h"
#include "Fallout/AI/FixedStrings.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"

BSSimpleList<BGSDecalNode*> BGSDecalNode::kDecalNodes;
NiRTTI BGSDecalNode::ms_RTTI("BGSDecalNode", &NiNode::ms_RTTI);

BGSDecalNode::BGSDecalNode() : NiNode(0) {
    Init();
    BGSDecalNode::kDecalNodes.AddHead(this);
}

BGSDecalNode::BGSDecalNode(NiNode* apNode, bool abSkinnedNode) : NiNode(0), bSkinnedNode(abSkinnedNode) {
    Init();
    apNode->AttachChild(this, true);
    NiUpdateData kUpdateData(0.0f, false, false);
    apNode->Update(kUpdateData);
    BGSDecalNode::kDecalNodes.AddHead(this);
}

[[clang::disable_tail_calls]] void BGSDecalNode::AttachChild(NiAVObject* apChild, int32_t abFirstAvail) {
    NiNode::AttachChild(apChild, static_cast<uint8_t>(abFirstAvail));
}

NiObject* BGSDecalNode::CreateClone(NiCloningProcess& arCloneData) {
    return new BGSDecalNode;
}

void BGSDecalNode::OnVisible(NiCullingProcess& arCuller) {
    if (IsVisualObject()) {
        BSSimpleList<NiPointer<NiRefObject>>* plist = pDecalList;
        while (plist && !plist->IsEmpty()) {
            BSTempEffectGeometryDecal* v2 = static_cast<BSTempEffectGeometryDecal*>(plist->m_item.m_pObject);
            NiPointer<NiAVObject> pdecal(v2->Get3D());
            if (pdecal.m_pObject) {
                pdecal.m_pObject->Cull(arCuller);
            }
            plist = plist->m_pkNext;
        }
    }
}

void BGSDecalNode::AttachChild(BSTempEffect* apDecal, bool bFirstAvail) {
    NiAVObject* v3 = apDecal->Get3D();
    BSShaderProperty* pprop = static_cast<BSShaderProperty*>(v3->GetProperty(3));
    if (pprop) {
        pprop->SetDepthBias(1.0f);
    }
    NiAVObject* v4 = apDecal->Get3D();
    NiNode::AttachChild(v4, static_cast<uint8_t>(bFirstAvail));
}

BGSDecalNode::~BGSDecalNode() {
    BGSDecalNode::kDecalNodes.Remove(this);
    if (pDecalList) {
        while (!pDecalList->IsEmpty()) {
            NiRefObject* pDecal = pDecalList->GetItem().operator->();
            // The node's decal list holds plain references; every decal in it is a temp effect.
            BSTempEffect::RemoveDecalFromList(static_cast<BSTempEffect*>(pDecal));
            pDecalList->RemoveHead();
        }
        delete pDecalList;
        pDecalList = nullptr;
    }
}

void BGSDecalNode::RemoveFromDecalList(BSTempEffect* apDecal) {
    NiPointer<NiRefObject> apMemory(static_cast<NiRefObject*>(apDecal));
    pDecalList->Remove(apMemory);
}

void BGSDecalNode::DetachChild(BSTempEffect* apDecal) {
    NiPointer<NiAVObject> spobject(static_cast<NiAVObject*>(nullptr));
    DetachChild(apDecal, spobject);
}

void BGSDecalNode::DetachChild(BSTempEffect* apDecal, NiPointer<NiAVObject>& aspAVObject) {
    RemoveFromDecalList(apDecal);
    NiAVObject* v3 = apDecal->Get3D();
    NiNode::DetachChild(v3, aspAVObject);
}

// Gives the node an empty decal list and its node name.
void BGSDecalNode::Init() {
    void* pMemory = ourMalloc(sizeof(DecalList));
    DecalList* pList = pMemory ? DecalList_Construct(pMemory) : nullptr;
    pDecalList = pList;
    if (bSkinnedNode) {
        SetName(FixedStrings::QSkinnedDecalNode());
    }
    else {
        SetName(FixedStrings::QDecalNode());
    }
}

void BGSDecalNode::RemoveDecals() {
    if (pDecalList) {
        pDecalList->RemoveAll();
    }
    for (uint32_t i = 0; i < GetArrayCount(); ++i) {
        DetachChildAt(i);
    }
}

BGSDecalNode::DecalList* BGSDecalNode::FreeDecalList(BGSDecalNode::DecalList* apList, uint32_t auiFlags) {
    apList->~BSSimpleList<NiPointer<NiRefObject>>();
    if ((auiFlags & 1) != 0) {
        ourFree(apList);
    }
    return apList;
}

void BGSDecalNode::RemoveAllDecals() {
    for (BGSDecalNode* v0 : BGSDecalNode::kDecalNodes) {
        if (v0) {
            v0->RemoveDecals();
        }
    }
}

// The decal list's own copy of the list constructor: the item through NiPointer's constructor, then the link.
BGSDecalNode::DecalList* DecalList_Construct(void* apMemory) {
    BGSDecalNode::DecalList* pThis = static_cast<BGSDecalNode::DecalList*>(apMemory);
    ::new (&pThis->m_item) NiPointer<NiRefObject>(nullptr);
    pThis->m_pkNext = nullptr;
    return pThis;
}

// The decal is removed by value: a smart pointer is built for it, removed from the list and released, and the decal is
// counted out.
void BSTempEffect::RemoveDecalFromList(BSTempEffect* apDecal) {
    // The pointer is released before the decal is counted out: the block ends its scope there.
    {
        NiPointer<NiRefObject> spDecal(apDecal);
        BSTempEffect::kDecals.Remove(spDecal);
    }
    --BSTempEffect::iNumDecals;
}

NiFixedString& FixedStrings::QDecalNode() {
    return *Decal_Node;
}
