#include "BSShader/BSOrderedNode.h"

#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSNiNode.h"
#include "BSShader/BSShaderAccumulator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI BSOrderedNode::ms_RTTI("BSOrderedNode", &BSNiNode::ms_RTTI);

// The static bound starts at the origin with radius 0, unused.
BSOrderedNode::BSOrderedNode() : bHasStaticBound(false) {
    StaticBound.m_kCenter = NiPoint3_ZERO;
    StaticBound.m_fRadius = 0.0f;
}

BSOrderedNode::~BSOrderedNode() {}

const NiRTTI* BSOrderedNode::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSOrderedNode::CreateClone(NiCloningProcess& kCloning) {
    void* pMemory = NiMemObject::operator new(sizeof(BSOrderedNode));
    BSOrderedNode* pkClone = pMemory ? ::new (pMemory) BSOrderedNode : nullptr;
    CopyMembers(pkClone, kCloning);
    return pkClone;
}

// Before Bethesda stream version 24 every ordered node had a static bound.
void BSOrderedNode::LoadBinary(NiStream& kStream) {
    NiNode::LoadBinary(kStream);
    StaticBound.LoadBinary(kStream);
    if (kStream.m_BSStreamHeader.uiVersion <= 0x17) {
        bHasStaticBound = true;
    }
    else {
        NiStreamLoadBinary(kStream, bHasStaticBound);
    }
}

void BSOrderedNode::LinkObject(NiStream& kStream) {
    NiNode::LinkObject(kStream);
}

bool BSOrderedNode::RegisterStreamables(NiStream& kStream) {
    return NiNode::RegisterStreamables(kStream);
}

void BSOrderedNode::SaveBinary(NiStream& kStream) {
    NiNode::SaveBinary(kStream);
    StaticBound.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, bHasStaticBound);
}

// The static bounds are compared only when both nodes have one.
bool BSOrderedNode::IsEqual(NiObject* pkObject) {
    if (!NiNode::IsEqual(pkObject)) {
        return false;
    }
    BSOrderedNode* pkOther = static_cast<BSOrderedNode*>(pkObject);
    if (bHasStaticBound != pkOther->bHasStaticBound) {
        return false;
    }
    return !bHasStaticBound || StaticBound == pkOther->StaticBound;
}

void BSOrderedNode::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiNode::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("bHasStaticBound", bHasStaticBound));
    NiBound kBound;
    kBound.m_kCenter = m_kWorld * StaticBound.m_kCenter;
    kBound.m_fRadius = StaticBound.m_fRadius * m_kWorld.m_fScale;
    pkStrings->Add(kBound.GetViewerString("Alpha Sort Bound"));
}

// The children's alphas are sorted as one group, around the static bound (moved into world space) or the world
// bound.
void BSOrderedNode::OnVisible(NiCullingProcess& arCuller) {
    BSShaderAccumulator* pkAccumulator =
        static_cast<BSShaderAccumulator*>(static_cast<BSCullingProcess&>(arCuller).spAccumulator.m_pObject);
    if (bHasStaticBound) {
        NiBound kBound;
        kBound.m_kCenter = m_kWorld * StaticBound.m_kCenter;
        kBound.m_fRadius = StaticBound.m_fRadius * m_kWorld.m_fScale;
        pkAccumulator->StartGroupingAlphas(kBound, false);
    }
    else {
        // EXE-INLINED 0x43D450
        const NiBound* pkBound = m_kWorldBound ? m_kWorldBound : &NiAVObject::NullBoundS;
        pkAccumulator->StartGroupingAlphas(*pkBound, false);
    }
    NiNode::OnVisible(arCuller);
    pkAccumulator->StopGroupingAlphas();
}

void BSOrderedNode::CopyMembers(BSOrderedNode* apDest, NiCloningProcess& arCloneData) {
    double m_fRadius;
    float z;
    BSNiNode::CopyMembers(apDest, arCloneData);
    m_fRadius = (double)StaticBound.m_fRadius;
    apDest->StaticBound.m_kCenter.x = StaticBound.m_kCenter.x;
    apDest->StaticBound.m_kCenter.y = StaticBound.m_kCenter.y;
    z = StaticBound.m_kCenter.z;
    apDest->StaticBound.m_fRadius = (float)m_fRadius;
    apDest->StaticBound.m_kCenter.z = z;
    apDest->bHasStaticBound = (bool)((uint8_t)bHasStaticBound);
}

// Stream factory for this class: a new node built by BSOrderedNode::BSOrderedNode, or null.
NiObject* BSOrderedNode::CreateObject() {
    void* pMemory = NiMemObject::operator new(sizeof(BSOrderedNode));
    if (!pMemory) {
        return nullptr;
    }
    return ::new (pMemory) BSOrderedNode;
}
