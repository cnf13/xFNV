#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRollController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiRollController::ms_RTTI("NiRollController", &NiFloatController::ms_RTTI);

NiRollController::NiRollController() : NiFloatController() {}

NiRollController::~NiRollController() {}

NiObject* NiRollController::CreateClone(NiCloningProcess& kCloning) {
    NiRollController* pkObject = static_cast<NiRollController*>(NiMemObject::operator new(sizeof(NiRollController)));
    if (pkObject) {
        ::new (pkObject) NiRollController;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(0, kCloning);
        return 0;
    }
}

void NiRollController::LoadBinary(NiStream& stream) {
    NiFloatController::LoadBinary(stream);
}

void NiRollController::LinkObject(NiStream& stream) {
    NiFloatController::LinkObject(stream);
}

bool NiRollController::RegisterStreamables(NiStream& stream) {
    return NiFloatController::RegisterStreamables(stream);
}

void NiRollController::SaveBinary(NiStream& stream) {
    NiFloatController::SaveBinary(stream);
}

bool NiRollController::IsEqual(NiObject* pObject) {
    return NiFloatController::IsEqual(pObject);
}

// The lines follow NiTimeController's directly: NiFloatController's heading is not added.
void NiRollController::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiTimeController::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(NiRollController::ms_RTTI.GetName()));
}

// Rolls the target about its local z axis: the keyed angle at the scaled time, applied after its rotation.
void NiRollController::Update(NiUpdateData& UpdateData) {
    if (!m_pkTarget || DontDoUpdate(UpdateData.fTime)) {
        return;
    }

    uint32_t uiNumKeys;
    NiFloatKey::KeyType eType;
    uint8_t ucSize;
    NiFloatKey* pkKeys = GetAnim(uiNumKeys, eType, ucSize);
    float fRoll = NiFloatKey::GenInterp(m_fScaledTime, pkKeys, eType, uiNumKeys, m_uiLastIdx, ucSize);

    NiMatrix3 kRot = static_cast<NiAVObject*>(m_pkTarget)->m_kLocal.m_Rotate;
    NiMatrix3 kRoll;
    float fSine;
    float fCosine;
    // EXE-INLINED 0x4A0C90
    float fAngle = -fRoll;
    // EXE-INLINED 0x4169A0
    fSine = std::sin(fAngle);
    fCosine = std::cos(fAngle);
    kRoll.m_pEntry[0][0] = fCosine;
    kRoll.m_pEntry[0][1] = fSine;
    kRoll.m_pEntry[0][2] = 0.0f;
    kRoll.m_pEntry[1][0] = -fSine;
    kRoll.m_pEntry[1][1] = fCosine;
    kRoll.m_pEntry[1][2] = 0.0f;
    kRoll.m_pEntry[2][0] = 0.0f;
    kRoll.m_pEntry[2][1] = 0.0f;
    kRoll.m_pEntry[2][2] = 1.0f;
    kRot = kRot * kRoll;
    static_cast<NiAVObject*>(m_pkTarget)->m_kLocal.m_Rotate = kRot;
}

bool NiRollController::TargetIsRequiredType() const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, m_pkTarget);
}

void NiRollController::CopyMembers(NiRollController* pDest, NiCloningProcess& kCloning) {
    NiFloatController::CopyMembers(pDest, kCloning);
}

NiObject* NiRollController::CreateObject() {
    NiRollController* pkObject = static_cast<NiRollController*>(NiMemObject::operator new(sizeof(NiRollController)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiRollController;
}
