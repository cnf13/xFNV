#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiPoint3Interpolator::ms_RTTI("NiPoint3Interpolator", &NiKeyBasedInterpolator::ms_RTTI);

NiPoint3Interpolator::NiPoint3Interpolator(NiPoint3 kPoseValue)
  : m_kPoint3Value(kPoseValue), m_spPoint3Data(nullptr), m_uiLastIdx(0) {}

NiPoint3Interpolator::NiPoint3Interpolator(NiPosData* pkPoint3Data)
  : m_kPoint3Value(INVALID_POINT3), m_spPoint3Data(pkPoint3Data), m_uiLastIdx(0) {}

NiPoint3Interpolator::~NiPoint3Interpolator() {}

const NiRTTI* NiPoint3Interpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiPoint3Interpolator::CreateClone(NiCloningProcess& kCloning) {
    NiPoint3Interpolator* pkObject =
        static_cast<NiPoint3Interpolator*>(NiMemObject::operator new(sizeof(NiPoint3Interpolator)));
    if (pkObject) {
        ::new (pkObject) NiPoint3Interpolator(static_cast<NiPosData*>(nullptr));
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

void NiPoint3Interpolator::LoadBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::LoadBinary(kStream);
    m_kPoint3Value.LoadBinary(kStream);
    m_spPoint3Data = static_cast<NiPosData*>(kStream.ResolveLinkID());
}

void NiPoint3Interpolator::LinkObject(NiStream& kStream) {
    NiKeyBasedInterpolator::LinkObject(kStream);
}

bool NiPoint3Interpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiKeyBasedInterpolator::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_spPoint3Data) {
        m_spPoint3Data->RegisterStreamables(kStream);
    }
    return true;
}

void NiPoint3Interpolator::SaveBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::SaveBinary(kStream);
    m_kPoint3Value.SaveBinary(kStream);
    kStream.SaveLinkID(m_spPoint3Data);
}

bool NiPoint3Interpolator::IsEqual(NiObject* pkObject) {
    if (!NiKeyBasedInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiPoint3Interpolator* pkOther = static_cast<NiPoint3Interpolator*>(pkObject);
    if (m_kPoint3Value != pkOther->m_kPoint3Value) {
        return false;
    }
    if ((m_spPoint3Data && !pkOther->m_spPoint3Data) || (!m_spPoint3Data && pkOther->m_spPoint3Data)) {
        return false;
    }
    if (m_spPoint3Data && !m_spPoint3Data->IsEqual(pkOther->m_spPoint3Data)) {
        return false;
    }
    return true;
}

void NiPoint3Interpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiKeyBasedInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kPoint3Value.GetViewerString("m_kPoint3Value"));
    pkStrings->Add(NiGetViewerString("m_spPoint3Data", static_cast<const void*>(m_spPoint3Data)));
}

// At an unchanged time the cached value is returned. Fails while the value is invalid.
bool NiPoint3Interpolator::Update(float fTime, NiObjectNET*, NiPoint3& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kPoint3Value;
        // EXE-INLINED 0x4390C0
        return !(
            m_kPoint3Value.x == INVALID_POINT3.x && m_kPoint3Value.y == INVALID_POINT3.y &&
            m_kPoint3Value.z == INVALID_POINT3.z
        );
    }

    if (m_spPoint3Data) {
        uint32_t uiNumKeys = m_spPoint3Data->m_uiNumKeys;
        NiPosKey* pkKeys = m_spPoint3Data->m_pkKeys;
        NiPosKey::KeyType eType = m_spPoint3Data->m_eType;
        uint8_t ucKeySize = m_spPoint3Data->m_ucKeySize;
        if (uiNumKeys > 0) {
            m_kPoint3Value = NiPosKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, m_uiLastIdx, ucKeySize);
        }
    }

    // EXE-INLINED 0x4390C0
    if (m_kPoint3Value.x == INVALID_POINT3.x && m_kPoint3Value.y == INVALID_POINT3.y &&
        m_kPoint3Value.z == INVALID_POINT3.z) {
        return false;
    }
    kValue = m_kPoint3Value;
    m_fLastTime = fTime;
    return true;
}

// Replaces keys that hold a single value by that value: no keys leave the value invalid.
void NiPoint3Interpolator::Collapse() {
    if (!m_spPoint3Data) {
        return;
    }

    uint32_t uiNumKeys = m_spPoint3Data->m_uiNumKeys;
    NiPosKey* pkKeys = m_spPoint3Data->m_pkKeys;
    NiPosKey::KeyType eType = m_spPoint3Data->m_eType;
    uint8_t ucKeySize = m_spPoint3Data->m_ucKeySize;
    if (uiNumKeys == 0) {
        m_spPoint3Data = nullptr;
        m_kPoint3Value = INVALID_POINT3;
        return;
    }

    NiPoint3 kValue = pkKeys->GetKeyAt(0, ucKeySize)->m_Pos;
    bool bCollapse = true;
    if (uiNumKeys != 1) {
        if (eType != NiAnimationKey::LINKEY && eType != NiAnimationKey::STEPKEY) {
            return;
        }
        for (uint32_t ui = 1; ui < uiNumKeys && bCollapse; ui++) {
            if (pkKeys->GetKeyAt(ui, ucKeySize)->m_Pos != kValue) {
                bCollapse = false;
            }
        }
    }

    if (bCollapse) {
        m_spPoint3Data = nullptr;
        m_kPoint3Value = kValue;
    }
}

void NiPoint3Interpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    if (m_spPoint3Data) {
        m_spPoint3Data->GuaranteeKeysAtStartAndEnd(fStartTime, fEndTime);
    }
}

NiInterpolator* NiPoint3Interpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiPoint3Interpolator* pkSeqInterp =
        static_cast<NiPoint3Interpolator*>(NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime));
    if (m_spPoint3Data) {
        NiPointer<NiPosData> spNewData;
        m_spPoint3Data->GetSequenceData(fStartTime, fEndTime, spNewData);
        pkSeqInterp->m_spPoint3Data = spNewData.m_pObject;
        pkSeqInterp->m_uiLastIdx = 0;
    }
    return pkSeqInterp;
}

uint32_t NiPoint3Interpolator::GetKeyCount(uint16_t) const {
    if (!m_spPoint3Data) {
        return 0;
    }
    return m_spPoint3Data->m_uiNumKeys;
}

NiAnimationKey::KeyType NiPoint3Interpolator::GetKeyType(uint16_t) const {
    if (!m_spPoint3Data) {
        return NiAnimationKey::NOINTERP;
    }
    return m_spPoint3Data->m_eType;
}

NiAnimationKey* NiPoint3Interpolator::GetKeyArray(uint16_t) const {
    if (!m_spPoint3Data) {
        return nullptr;
    }
    return m_spPoint3Data->m_pkKeys;
}

uint8_t NiPoint3Interpolator::GetKeyStride(uint16_t) const {
    if (!m_spPoint3Data) {
        return 0;
    }
    return m_spPoint3Data->m_ucKeySize;
}

// Posed: no keys, but a valid value.
bool NiPoint3Interpolator::GetChannelPosed(uint16_t) const {
    if (m_spPoint3Data) {
        return false;
    }
    // EXE-INLINED 0x4390C0
    return !(
        m_kPoint3Value.x == INVALID_POINT3.x && m_kPoint3Value.y == INVALID_POINT3.y &&
        m_kPoint3Value.z == INVALID_POINT3.z
    );
}

void NiPoint3Interpolator::SetPoseValue(NiPoint3 kPoseValue) {
    m_spPoint3Data = nullptr;
    m_kPoint3Value = kPoseValue;
}

void NiPoint3Interpolator::CopyMembers(NiPoint3Interpolator* pkDest, NiCloningProcess& kCloning) {
    NiKeyBasedInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_kPoint3Value = m_kPoint3Value;
    pkDest->m_spPoint3Data = m_spPoint3Data;
    pkDest->m_uiLastIdx = m_uiLastIdx;
}

NiObject* NiPoint3Interpolator::CreateObject() {
    NiPoint3Interpolator* pkObject =
        static_cast<NiPoint3Interpolator*>(NiMemObject::operator new(sizeof(NiPoint3Interpolator)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiPoint3Interpolator(static_cast<NiPosData*>(nullptr));
}
