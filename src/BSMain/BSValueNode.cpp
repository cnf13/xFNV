#include "BSMain/BSValueNode.h"

#include "BSMain/BSNiNode.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBillboardNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

NiRTTI BSValueNode::ms_RTTI("BSValueNode", &BSNiNode::ms_RTTI);

BSValueNode::BSValueNode() : iValue(0), spReservedObject(nullptr) {}

// The node built from a2 (abKeepChildren false); a billboard whose mode bits are 5 sets bit 0 of the flags.
BSValueNode::BSValueNode(NiBillboardNode* a2) : BSNiNode(a2, false) {
    if (a2 && a2->IsKindOf(&NiBillboardNode::ms_RTTI) && (a2->NiBillboardNode::m_uFlags & 7) == 5) {
        m_uFlags |= 1;
    }
}

BSValueNode::~BSValueNode() {}

const NiRTTI* BSValueNode::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* BSValueNode::CreateClone(NiCloningProcess& arCloneData) {
    void* pMemory = NiMemObject::operator new(sizeof(BSValueNode));
    BSValueNode* pkClone = pMemory ? ::new (pMemory) BSValueNode : nullptr;
    CopyMembers(pkClone, arCloneData);
    return pkClone;
}

// The flags were added in Bethesda stream version 25. The value goes through a uint32_t local.
void BSValueNode::LoadBinary(NiStream& arStream) {
    NiNode::LoadBinary(arStream);
    uint32_t uiLoaded;
    NiStreamLoadBinary(arStream, uiLoaded);
    iValue = static_cast<int32_t>(uiLoaded);
    if (arStream.m_BSStreamHeader.uiVersion > 0x18) {
        NiStreamLoadBinary(arStream, reinterpret_cast<bool&>(m_uFlags));
    }
}

void BSValueNode::LinkObject(NiStream& arStream) {
    NiNode::LinkObject(arStream);
}

bool BSValueNode::RegisterStreamables(NiStream& arStream) {
    return NiNode::RegisterStreamables(arStream);
}

void BSValueNode::SaveBinary(NiStream& arStream) {
    NiNode::SaveBinary(arStream);
    int32_t iSaved = iValue;
    NiAnimationKey::SaveBinary(arStream, reinterpret_cast<NiAnimationKey*>(&iSaved));
    NiStreamSaveByte(&arStream, &m_uFlags);
}

bool BSValueNode::IsEqual(NiObject* apObject) {
    if (!NiNode::IsEqual(apObject)) {
        return false;
    }
    BSValueNode* pkOther = static_cast<BSValueNode*>(apObject);
    return iValue == pkOther->iValue && m_uFlags == pkOther->m_uFlags;
}

void BSValueNode::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    NiNode::GetViewerStrings(apStrings);
    apStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    apStrings->Add(NiGetViewerString("Value", iValue));
    if (m_uFlags & 1) {
        apStrings->Add(NiGetViewerString("Billboard", "true"));
    }
}

// A billboard value node ignores its parents' rotation.
void BSValueNode::UpdateWorldData(NiUpdateData& UpdateData) {
    NiAVObject::UpdateWorldData(UpdateData);
    if (m_uFlags & 1) {
        m_kWorld.m_Rotate = NiMatrix3_IDENTITY;
    }
}

// The value is the number at the end of the name; '#' and '@' may sit among its digits. Returns whether the name has a
// value suffix: whether the number's first character is not the name's terminator, so a name ending in a non-digit
// returns false.
bool BSValueNode::ParseNameForValue() {
    const char* pcName = m_kName;
    size_t uiLength = std::strlen(pcName);
    size_t i = 1;
    for (; (pcName[uiLength - i] >= '0' && pcName[uiLength - i] <= '9') || pcName[uiLength - i] == '#' ||
         pcName[uiLength - i] == '@';
         ++i) {
        if (i > uiLength) {
            break;
        }
    }
    const char* pcValue = &pcName[uiLength - i + 1];
    bool bHasValue = *pcValue != 0;
    iValue = std::atoi(pcValue);
    return bHasValue;
}

void BSValueNode::CopyMembers(BSValueNode* pDest, NiCloningProcess& arCloneData) {
    BSNiNode::CopyMembers(pDest, arCloneData);
    pDest->iValue = iValue;
    pDest->m_uFlags = m_uFlags;
}

int32_t BSValueNode::QValue() {
    return iValue;
}

NiObject* BSValueNode::CreateObject() {
    void* apMemory = NiMemObject::operator new(sizeof(BSValueNode));
    if (apMemory) {
        return ::new (apMemory) BSValueNode;
    }

    return nullptr;
}
