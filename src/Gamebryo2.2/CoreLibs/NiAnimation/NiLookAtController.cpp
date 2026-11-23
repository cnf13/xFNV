#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLookAtController.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiLookAtController::ms_RTTI("NiLookAtController", &NiTimeController::ms_RTTI);

NiLookAtController::NiLookAtController() : NiTimeController(), m_uFlags(0), m_pkLookAt(nullptr) {
    m_usFlags &= ~COMPUTESCALEDTIME_MASK;
}

NiLookAtController::~NiLookAtController() {}

NiObject* NiLookAtController::CreateClone(NiCloningProcess& kCloning) {
    NiLookAtController* pkClone = new NiLookAtController;
    CopyMembers(pkClone, kCloning);
    pkClone->m_uFlags = m_uFlags;
    if (m_uFlags & FLIP_MASK) {
        pkClone->m_uFlags |= FLIP_MASK;
    }
    else {
        pkClone->m_uFlags &= ~FLIP_MASK;
    }
    pkClone->m_uFlags = (m_uFlags & AXIS_MASK) | (pkClone->m_uFlags & ~AXIS_MASK);
    pkClone->m_pkLookAt = m_pkLookAt;
    return pkClone;
}

// Files before 10.0.1.2 keep the flags in the base controller's flag word, above its own five bits.
void NiLookAtController::LoadBinary(NiStream& kStream) {
    NiTimeController::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= 0x0A000102) {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
    else {
        m_uFlags = static_cast<uint16_t>((kStream.m_usNiTimeControllerFlags >> 5) & 0xFF);
    }

    kStream.ReadLinkID();
}

void NiLookAtController::LinkObject(NiStream& kStream) {
    NiTimeController::LinkObject(kStream);
    m_pkLookAt = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiLookAtController::RegisterStreamables(NiStream& kStream) {
    if (!NiTimeController::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_pkLookAt) {
        m_pkLookAt->RegisterStreamables(kStream);
    }
    return true;
}

void NiLookAtController::SaveBinary(NiStream& kStream) {
    NiTimeController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
    kStream.SaveLinkID(m_pkLookAt);
}

// The flags are not compared.
bool NiLookAtController::IsEqual(NiObject* pkObject) {
    if (!NiTimeController::IsEqual(pkObject)) {
        return false;
    }

    NiLookAtController* pkOther = static_cast<NiLookAtController*>(pkObject);
    if (m_pkLookAt) {
        if (!pkOther->m_pkLookAt) {
            return false;
        }
    }
    else if (pkOther->m_pkLookAt) {
        return false;
    }

    if (m_pkLookAt && !m_pkLookAt->IsEqual(pkOther->m_pkLookAt)) {
        return false;
    }
    return true;
}

void NiLookAtController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiTimeController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The clone looks at the clone of the look-at object, or at the original one when that was not cloned.
void NiLookAtController::ProcessClone(NiCloningProcess& kCloning) {
    NiTimeController::ProcessClone(kCloning);

    NiObject* pkCloneObject = nullptr;
    kCloning.m_pkCloneMap->GetAt(this, pkCloneObject);
    NiLookAtController* pkClone = static_cast<NiLookAtController*>(pkCloneObject);

    if (m_pkLookAt) {
        NiObject* pkLookAtClone;
        if (kCloning.m_pkCloneMap->GetAt(m_pkLookAt, pkLookAtClone)) {
            pkClone->m_pkLookAt = static_cast<NiAVObject*>(pkLookAtClone);
        }
        else {
            pkClone->m_pkLookAt = m_pkLookAt;
        }
    }
}

// Turns the target's local rotation so that the chosen axis points at the look-at object (away from it unless the flip
// bit is set). The axis field has no case for 3; this body then uses the identity matrix.
void NiLookAtController::Update(NiUpdateData& UpdateData) {
    if (!m_pkLookAt || !GetActive()) {
        return;
    }

    NiAVObject* pkTarget = static_cast<NiAVObject*>(m_pkTarget);
    NiNode* pkParent = pkTarget->m_pkParent;
    NiPoint3 kLookAtPos = m_pkLookAt->m_kWorld.m_Translate;

    NiMatrix3 kParentRot;
    NiPoint3 kTargetPos;
    if (pkParent) {
        kParentRot = pkParent->m_kWorld.m_Rotate;
        float fScale = pkParent->m_kWorld.m_fScale;
        NiPoint3 kOffset = kParentRot * pkTarget->m_kLocal.m_Translate;
        kTargetPos.x = pkParent->m_kWorld.m_Translate.x + kOffset.x * fScale;
        kTargetPos.y = pkParent->m_kWorld.m_Translate.y + kOffset.y * fScale;
        kTargetPos.z = pkParent->m_kWorld.m_Translate.z + fScale * kOffset.z;
    }
    else {
        kParentRot = NiMatrix3::IDENTITY;
        kTargetPos = pkTarget->m_kLocal.m_Translate;
    }

    NiPoint3 kDir = kLookAtPos - kTargetPos;
    float fSqrLength = kDir.x * kDir.x + kDir.y * kDir.y + kDir.z * kDir.z;
    if (fSqrLength < 0.001f) {
        NiMatrix3& kRot = pkTarget->m_kLocal.m_Rotate;
        kRot.m_pEntry[0][0] = 1.0f;
        kRot.m_pEntry[0][1] = 0.0f;
        kRot.m_pEntry[0][2] = 0.0f;
        kRot.m_pEntry[1][0] = 0.0f;
        kRot.m_pEntry[1][1] = 1.0f;
        kRot.m_pEntry[1][2] = 0.0f;
        kRot.m_pEntry[2][0] = 0.0f;
        kRot.m_pEntry[2][1] = 0.0f;
        kRot.m_pEntry[2][2] = 1.0f;
        return;
    }

    kDir.Unitize();

    // Up is the world axis least parallel to the direction, made perpendicular to it.
    NiPoint3 kUp;
    if (kDir.z < 1.001f && kDir.z > 0.999) {
        float fDot = kDir.y;
        kUp.x = NiPoint3::UNIT_Y.x - kDir.x * fDot;
        kUp.y = NiPoint3::UNIT_Y.y - kDir.y * fDot;
        kUp.z = NiPoint3::UNIT_Y.z - kDir.z * fDot;
    }
    else {
        float fDot = kDir.z;
        kUp.x = NiPoint3::UNIT_Z.x - kDir.x * fDot;
        kUp.y = NiPoint3::UNIT_Z.y - kDir.y * fDot;
        kUp.z = NiPoint3::UNIT_Z.z - kDir.z * fDot;
    }
    kUp.Unitize();

    if (!(m_uFlags & FLIP_MASK)) {
        kDir = -kDir;
    }

    // Axis field 3 has no case; initialising to identity keeps the result a rotation.
    NiMatrix3 kRot = NiMatrix3::IDENTITY;
    switch ((m_uFlags & AXIS_MASK) >> AXIS_POS) {
        case X:
            kRot.SetCol(0, kDir);
            kRot.SetCol(1, kUp);
            kRot.SetCol(2, -kUp.Cross(kDir));
            break;
        case Y:
            kRot.SetCol(0, kUp.Cross(kDir));
            kRot.SetCol(1, kDir);
            kRot.SetCol(2, -kUp);
            break;
        case Z:
            kRot.SetCol(0, kUp.Cross(kDir));
            kRot.SetCol(1, kUp);
            kRot.SetCol(2, kDir);
            break;
    }

    pkTarget->m_kLocal.m_Rotate = kParentRot.TransposeTimes(kRot);
}

bool NiLookAtController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

NiObject* NiLookAtController::CreateObject() {
    return new NiLookAtController;
}
