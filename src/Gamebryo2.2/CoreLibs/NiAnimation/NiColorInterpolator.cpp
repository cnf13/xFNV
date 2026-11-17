#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiColorInterpolator::ms_RTTI("NiColorInterpolator", &NiKeyBasedInterpolator::ms_RTTI);

NiColorInterpolator::NiColorInterpolator(NiColorA kPoseValue)
  : m_kColorValue(kPoseValue), m_spColorData(nullptr), m_uiLastIdx(0) {}

NiColorInterpolator::NiColorInterpolator(NiColorData* pkColorData)
  : m_kColorValue(NiInterpolator::INVALID_COLORA), m_spColorData(pkColorData), m_uiLastIdx(0) {}

NiColorInterpolator::~NiColorInterpolator() {}

const NiRTTI* NiColorInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiColorInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiColorInterpolator* pkObject =
        static_cast<NiColorInterpolator*>(NiMemObject::operator new(sizeof(NiColorInterpolator)));
    if (pkObject) {
        pkObject = ::new (pkObject) NiColorInterpolator(static_cast<NiColorData*>(nullptr));
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(nullptr, kCloning);
        return nullptr;
    }
}

// NiQuaternionInterpolator uses the same function: its quaternion is read as four floats, like the colour.
void NiColorInterpolator::LoadBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::LoadBinary(kStream);
    m_kColorValue.LoadBinary(kStream);
    m_spColorData = static_cast<NiColorData*>(kStream.ResolveLinkID());
}

void NiColorInterpolator::LinkObject(NiStream& kStream) {
    NiKeyBasedInterpolator::LinkObject(kStream);
}

// NiQuaternionInterpolator uses the same function.
bool NiColorInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiKeyBasedInterpolator::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_spColorData) {
        m_spColorData->RegisterStreamables(kStream);
    }
    return true;
}

void NiColorInterpolator::SaveBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::SaveBinary(kStream);
    m_kColorValue.SaveBinary(kStream);
    kStream.SaveLinkID(m_spColorData);
}

bool NiColorInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiKeyBasedInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiColorInterpolator* pkOther = static_cast<NiColorInterpolator*>(pkObject);
    if (m_kColorValue != pkOther->m_kColorValue) {
        return false;
    }
    if ((m_spColorData && !pkOther->m_spColorData) || (!m_spColorData && pkOther->m_spColorData)) {
        return false;
    }
    if (m_spColorData && !m_spColorData->IsEqual(pkOther->m_spColorData)) {
        return false;
    }
    return true;
}

void NiColorInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiKeyBasedInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kColorValue.GetViewerString("m_kColorValue"));
    pkStrings->Add(NiGetViewerString("m_spColorData", static_cast<const void*>(m_spColorData)));
}

// At an unchanged time the cached value is returned. Fails while the value is invalid.
bool NiColorInterpolator::Update(float fTime, NiObjectNET*, NiColorA& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kColorValue;
        return !(m_kColorValue == INVALID_COLORA);
    }

    if (m_spColorData) {
        uint32_t uiNumKeys = m_spColorData->m_uiNumKeys;
        NiColorKey* pkKeys = m_spColorData->m_pkKeys;
        NiColorKey::KeyType eType = m_spColorData->m_eType;
        uint8_t ucKeySize = m_spColorData->m_ucKeySize;
        if (uiNumKeys > 0) {
            m_kColorValue = NiColorKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, m_uiLastIdx, ucKeySize);
        }
    }

    if (m_kColorValue == INVALID_COLORA) {
        return false;
    }
    kValue = m_kColorValue;
    m_fLastTime = fTime;
    return true;
}

// Replaces keys that hold a single value by that value: no keys leave the value invalid.
void NiColorInterpolator::Collapse() {
    if (!m_spColorData) {
        return;
    }

    uint32_t uiNumKeys = m_spColorData->m_uiNumKeys;
    NiColorKey* pkKeys = m_spColorData->m_pkKeys;
    NiColorKey::KeyType eType = m_spColorData->m_eType;
    uint8_t ucKeySize = m_spColorData->m_ucKeySize;
    if (uiNumKeys == 0) {
        m_spColorData = nullptr;
        m_kColorValue = INVALID_COLORA;
        return;
    }

    NiColorA kValue = pkKeys->GetKeyAt(0, ucKeySize)->m_Color;
    bool bCollapse = true;
    if (uiNumKeys != 1) {
        if (eType != NiAnimationKey::LINKEY && eType != NiAnimationKey::STEPKEY) {
            return;
        }
        for (uint32_t ui = 1; ui < uiNumKeys && bCollapse; ui++) {
            if (pkKeys->GetKeyAt(ui, ucKeySize)->m_Color != kValue) {
                bCollapse = false;
            }
        }
    }

    if (bCollapse) {
        m_spColorData = nullptr;
        m_kColorValue = kValue;
    }
}

void NiColorInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    if (m_spColorData) {
        m_spColorData->GuaranteeKeysAtStartAndEnd(fStartTime, fEndTime);
    }
}

NiInterpolator* NiColorInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiColorInterpolator* pkSeqInterp =
        static_cast<NiColorInterpolator*>(NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime));
    if (m_spColorData) {
        NiPointer<NiColorData> spNewData;
        m_spColorData->GetSequenceData(fStartTime, fEndTime, spNewData);
        pkSeqInterp->m_spColorData = spNewData.m_pObject;
        pkSeqInterp->m_uiLastIdx = 0;
    }
    return pkSeqInterp;
}

// NiQuaternionInterpolator uses the same function.
uint32_t NiColorInterpolator::GetKeyCount(uint16_t) const {
    if (!m_spColorData) {
        return 0;
    }
    return m_spColorData->m_uiNumKeys;
}

// NiQuaternionInterpolator uses the same function.
NiAnimationKey::KeyType NiColorInterpolator::GetKeyType(uint16_t) const {
    if (!m_spColorData) {
        return NiAnimationKey::NOINTERP;
    }
    return m_spColorData->m_eType;
}

NiAnimationKey::KeyContent NiColorInterpolator::GetKeyContent(uint16_t) const {
    return NiAnimationKey::COLORKEY;
}

// NiQuaternionInterpolator uses the same function.
NiAnimationKey* NiColorInterpolator::GetKeyArray(uint16_t) const {
    if (!m_spColorData) {
        return nullptr;
    }
    return m_spColorData->m_pkKeys;
}

// NiQuaternionInterpolator uses the same function.
uint8_t NiColorInterpolator::GetKeyStride(uint16_t) const {
    if (!m_spColorData) {
        return 0;
    }
    return m_spColorData->m_ucKeySize;
}

// Posed: no keys, but a valid value.
bool NiColorInterpolator::GetChannelPosed(uint16_t) const {
    if (m_spColorData) {
        return false;
    }
    return !(m_kColorValue == INVALID_COLORA);
}

void NiColorInterpolator::SetPoseValue(NiColorA kPoseValue) {
    m_spColorData = nullptr;
    m_kColorValue = kPoseValue;
}

void NiColorInterpolator::SetColorData(NiColorData* pkColorData) {
    m_spColorData = pkColorData;
}

void NiColorInterpolator::CopyMembers(NiColorInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiKeyBasedInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_kColorValue = m_kColorValue;
    pkDest->m_spColorData = m_spColorData;
    pkDest->m_uiLastIdx = m_uiLastIdx;
}

NiObject* NiColorInterpolator::CreateObject() {
    NiColorInterpolator* pkObject =
        static_cast<NiColorInterpolator*>(NiMemObject::operator new(sizeof(NiColorInterpolator)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiColorInterpolator(static_cast<NiColorData*>(nullptr));
}
