#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuaternionInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiEulerRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiQuaternionInterpolator::ms_RTTI("NiQuaternionInterpolator", &NiKeyBasedInterpolator::ms_RTTI);

NiQuaternionInterpolator::NiQuaternionInterpolator()
  : NiKeyBasedInterpolator(), m_kQuaternionValue(INVALID_QUATERNION), m_spQuaternionData(nullptr), m_uiLastIdx(0) {}

NiQuaternionInterpolator::~NiQuaternionInterpolator() {}

const NiRTTI* NiQuaternionInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiQuaternionInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiQuaternionInterpolator* pkObject =
        static_cast<NiQuaternionInterpolator*>(NiMemObject::operator new(sizeof(NiQuaternionInterpolator)));
    if (pkObject) {
        ::new (pkObject) NiQuaternionInterpolator;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

void NiQuaternionInterpolator::LoadBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::LoadBinary(kStream);
    m_kQuaternionValue.LoadBinary(kStream);
    m_spQuaternionData = static_cast<NiRotData*>(kStream.ResolveLinkID());
}

void NiQuaternionInterpolator::LinkObject(NiStream& kStream) {
    NiKeyBasedInterpolator::LinkObject(kStream);
}

bool NiQuaternionInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiKeyBasedInterpolator::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_spQuaternionData) {
        m_spQuaternionData->RegisterStreamables(kStream);
    }
    return true;
}

void NiQuaternionInterpolator::SaveBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::SaveBinary(kStream);
    m_kQuaternionValue.SaveBinary(kStream);
    kStream.SaveLinkID(m_spQuaternionData);
}

bool NiQuaternionInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiKeyBasedInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiQuaternionInterpolator* pkOther = static_cast<NiQuaternionInterpolator*>(pkObject);
    if (m_kQuaternionValue != pkOther->m_kQuaternionValue) {
        return false;
    }
    if ((m_spQuaternionData && !pkOther->m_spQuaternionData) || (!m_spQuaternionData && pkOther->m_spQuaternionData)) {
        return false;
    }
    if (m_spQuaternionData && !m_spQuaternionData->IsEqual(pkOther->m_spQuaternionData)) {
        return false;
    }
    return true;
}

void NiQuaternionInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiKeyBasedInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kQuaternionValue.GetViewerString("m_kQuaternionValue"));
    pkStrings->Add(NiGetViewerString("m_spQuaternionData", static_cast<const void*>(m_spQuaternionData)));
}

// At an unchanged time the cached value is returned. Fails while the value is invalid.
bool NiQuaternionInterpolator::Update(float fTime, NiObjectNET*, NiQuaternion& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kQuaternionValue;
        return !(m_kQuaternionValue == INVALID_QUATERNION);
    }

    if (m_spQuaternionData) {
        uint32_t uiNumKeys = m_spQuaternionData->m_uiNumKeys;
        NiRotKey* pkKeys = m_spQuaternionData->m_pkKeys;
        NiRotKey::KeyType eType = m_spQuaternionData->m_eType;
        uint8_t ucKeySize = m_spQuaternionData->m_ucKeySize;
        if (uiNumKeys > 0) {
            m_kQuaternionValue = NiRotKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, m_uiLastIdx, ucKeySize);
        }
    }

    if (m_kQuaternionValue == INVALID_QUATERNION) {
        return false;
    }
    kValue = m_kQuaternionValue;
    m_fLastTime = fTime;
    return true;
}

// KEEP: the exe stores the key size as one byte (mov [esp+8],dl at 0xA48CE4) and pushes that slot's whole dword to
// GenInterp (0xA48CEC): the upper three bytes are an uninitialised read src cannot reproduce.
// Replaces keys that hold a single rotation by that rotation: no keys leave the value invalid. A single Euler key
// stays, since its angles are themselves animated.
void NiQuaternionInterpolator::Collapse() {
    if (!m_spQuaternionData) {
        return;
    }

    uint32_t uiNumKeys = m_spQuaternionData->m_uiNumKeys;
    NiRotKey* pkKeys = m_spQuaternionData->m_pkKeys;
    NiRotKey::KeyType eType = m_spQuaternionData->m_eType;
    uint8_t ucKeySize = m_spQuaternionData->m_ucKeySize;
    if (uiNumKeys == 0) {
        m_spQuaternionData = nullptr;
        m_kQuaternionValue = INVALID_QUATERNION;
        return;
    }

    NiQuaternion kValue = pkKeys->GetKeyAt(0, ucKeySize)->m_quat;
    bool bCollapse = true;
    if (uiNumKeys != 1 || eType == NiAnimationKey::EULERKEY) {
        if (eType != NiAnimationKey::LINKEY && eType != NiAnimationKey::STEPKEY) {
            return;
        }
        // Leaves the loop at the first key that differs.
        for (uint32_t ui = 1; ui < uiNumKeys && bCollapse; ui++) {
            if (pkKeys->GetKeyAt(ui, ucKeySize)->m_quat != kValue) {
                bCollapse = false;
            }
        }
    }

    if (bCollapse) {
        m_spQuaternionData = nullptr;
        m_kQuaternionValue = kValue;
    }
}

// An Euler key spans from the earliest first to the latest last key of its three angle arrays.
void NiQuaternionInterpolator::GetActiveTimeRange(float& afBeginKeyTime, float& afEndKeyTime) const {
    if (!m_spQuaternionData || m_spQuaternionData->m_uiNumKeys == 0) {
        afBeginKeyTime = 0.0f;
        afEndKeyTime = 0.0f;
        return;
    }

    uint32_t uiNumKeys = m_spQuaternionData->m_uiNumKeys;
    NiRotKey* pkKeys = m_spQuaternionData->m_pkKeys;
    uint8_t ucKeySize = m_spQuaternionData->m_ucKeySize;
    if (m_spQuaternionData->m_eType != NiAnimationKey::EULERKEY) {
        afBeginKeyTime = pkKeys->GetTime();
        afEndKeyTime = pkKeys->GetKeyAt(uiNumKeys - 1, ucKeySize)->GetTime();
        return;
    }

    NiEulerRotKey* pkEulerKey = static_cast<NiEulerRotKey*>(pkKeys);
    float fBeginKeyTime = FLT_MAX;
    float fEndKeyTime = -FLT_MAX;
    for (uint32_t ui = 0; ui < 3; ui++) {
        NiFloatKey* pkAngleKeys = pkEulerKey->m_apkKeys[ui];
        uint32_t uiNumAngleKeys = pkEulerKey->m_uiNumKeys[ui];
        uint8_t ucAngleKeySize = pkEulerKey->m_ucSizes[ui];
        float fTime = pkAngleKeys->GetTime();
        if (fTime < fBeginKeyTime) {
            fBeginKeyTime = fTime;
        }
        fTime = pkAngleKeys->GetKeyAt(uiNumAngleKeys - 1, ucAngleKeySize)->GetTime();
        if (fTime > fEndKeyTime) {
            fEndKeyTime = fTime;
        }
    }
    afBeginKeyTime = fBeginKeyTime;
    afEndKeyTime = fEndKeyTime;
}

void NiQuaternionInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    if (m_spQuaternionData) {
        m_spQuaternionData->GuaranteeKeysAtStartAndEnd(fStartTime, fEndTime);
    }
}

NiInterpolator* NiQuaternionInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiQuaternionInterpolator* pkSeqInterp =
        static_cast<NiQuaternionInterpolator*>(NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime));
    if (m_spQuaternionData) {
        NiPointer<NiRotData> spNewData;
        m_spQuaternionData->GetSequenceData(fStartTime, fEndTime, spNewData);
        pkSeqInterp->m_spQuaternionData = spNewData.m_pObject;
        pkSeqInterp->m_uiLastIdx = 0;
    }
    return pkSeqInterp;
}

uint32_t NiQuaternionInterpolator::GetKeyCount(uint16_t) const {
    if (!m_spQuaternionData) {
        return 0;
    }
    return m_spQuaternionData->m_uiNumKeys;
}

NiAnimationKey::KeyType NiQuaternionInterpolator::GetKeyType(uint16_t) const {
    if (!m_spQuaternionData) {
        return NiAnimationKey::NOINTERP;
    }
    return m_spQuaternionData->m_eType;
}

NiAnimationKey::KeyContent NiQuaternionInterpolator::GetKeyContent(uint16_t) const {
    return NiAnimationKey::ROTKEY;
}

NiAnimationKey* NiQuaternionInterpolator::GetKeyArray(uint16_t) const {
    if (!m_spQuaternionData) {
        return nullptr;
    }
    return m_spQuaternionData->m_pkKeys;
}

uint8_t NiQuaternionInterpolator::GetKeyStride(uint16_t) const {
    if (!m_spQuaternionData) {
        return 0;
    }
    return m_spQuaternionData->m_ucKeySize;
}

// Posed: no keys, but a valid value.
bool NiQuaternionInterpolator::GetChannelPosed(uint16_t) const {
    if (m_spQuaternionData) {
        return false;
    }
    return !(m_kQuaternionValue == INVALID_QUATERNION);
}

void NiQuaternionInterpolator::CopyMembers(NiQuaternionInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiKeyBasedInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_kQuaternionValue = m_kQuaternionValue;
    pkDest->m_spQuaternionData = m_spQuaternionData;
    pkDest->m_uiLastIdx = m_uiLastIdx;
}

NiObject* NiQuaternionInterpolator::CreateObject() {
    NiQuaternionInterpolator* pkObject =
        static_cast<NiQuaternionInterpolator*>(NiMemObject::operator new(sizeof(NiQuaternionInterpolator)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiQuaternionInterpolator;
}
