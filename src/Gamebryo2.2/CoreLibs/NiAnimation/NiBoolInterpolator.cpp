#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBoolInterpolator::ms_RTTI("NiBoolInterpolator", &NiKeyBasedInterpolator::ms_RTTI);

NiBoolInterpolator::NiBoolInterpolator(bool abPoseValue)
  : NiKeyBasedInterpolator(), m_bBoolValue(abPoseValue), m_spBoolData(nullptr), m_uiLastIdx(0) {}

NiBoolInterpolator::NiBoolInterpolator(NiBoolData* pkBoolData)
  : NiKeyBasedInterpolator(), m_bBoolValue(NiInterpolator::INVALID_BOOL), m_spBoolData(pkBoolData), m_uiLastIdx(0) {}

// m_spBoolData is released by its destructor.
NiBoolInterpolator::~NiBoolInterpolator() {}

const NiRTTI* NiBoolInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBoolInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBoolInterpolator* pkObject = new NiBoolInterpolator(static_cast<NiBoolData*>(nullptr));
    CopyMembers(pkObject, kCloning);
    return pkObject;
}

void NiBoolInterpolator::LoadBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_bBoolValue);
    m_spBoolData = static_cast<NiBoolData*>(kStream.ResolveLinkID());
}

void NiBoolInterpolator::LinkObject(NiStream& kStream) {
    NiKeyBasedInterpolator::LinkObject(kStream);
}

bool NiBoolInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiKeyBasedInterpolator::RegisterStreamables(kStream)) {
        return false;
    }
    if (m_spBoolData) {
        m_spBoolData->RegisterStreamables(kStream);
    }
    return true;
}

void NiBoolInterpolator::SaveBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_bBoolValue);
    kStream.SaveLinkID(m_spBoolData);
}

bool NiBoolInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiKeyBasedInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBoolInterpolator* pkOther = static_cast<NiBoolInterpolator*>(pkObject);
    if (m_bBoolValue != pkOther->m_bBoolValue) {
        return false;
    }
    if ((m_spBoolData && !pkOther->m_spBoolData) || (!m_spBoolData && pkOther->m_spBoolData)) {
        return false;
    }
    if (m_spBoolData && !m_spBoolData->IsEqual(pkOther->m_spBoolData)) {
        return false;
    }
    return true;
}

void NiBoolInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiKeyBasedInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_bBoolValue", m_bBoolValue));
    pkStrings->Add(NiGetViewerString("m_spBoolData", static_cast<const void*>(m_spBoolData)));
}

// At an unchanged time the cached value is returned. Fails, reporting false, while the value is invalid.
bool NiBoolInterpolator::Update(float fTime, NiObjectNET*, bool& bValue) {
    if (m_fLastTime == fTime) {
        if (m_bBoolValue == INVALID_BOOL) {
            bValue = false;
            return false;
        }
        bValue = m_bBoolValue != 0;
        return true;
    }

    if (m_spBoolData) {
        uint32_t uiNumKeys = m_spBoolData->m_uiNumKeys;
        NiBoolKey* pkKeys = m_spBoolData->m_pkKeys;
        NiBoolKey::KeyType eType = m_spBoolData->m_eType;
        uint8_t ucKeySize = m_spBoolData->m_ucKeySize;
        if (uiNumKeys > 0) {
            // GenInterp's raw byte is stored as 0 or 1.
            m_bBoolValue = NiBoolKey::GenInterp(fTime, pkKeys, eType, uiNumKeys, m_uiLastIdx, ucKeySize) != 0;
        }
    }

    if (m_bBoolValue == INVALID_BOOL) {
        bValue = false;
        return false;
    }
    bValue = m_bBoolValue != 0;
    m_fLastTime = fTime;
    return true;
}

// Replaces keys that hold a single value by that value: no keys leave the value invalid.
void NiBoolInterpolator::Collapse() {
    if (!m_spBoolData) {
        return;
    }

    uint32_t uiNumKeys = m_spBoolData->m_uiNumKeys;
    NiBoolKey* pkKeys = m_spBoolData->m_pkKeys;
    NiBoolKey::KeyType eType = m_spBoolData->m_eType;
    uint8_t ucKeySize = m_spBoolData->m_ucKeySize;
    if (uiNumKeys == 0) {
        m_spBoolData = nullptr;
        m_bBoolValue = INVALID_BOOL;
        return;
    }

    bool bValue = pkKeys->GetKeyAt(0, ucKeySize)->GetBool();
    bool bCollapse = true;
    if (uiNumKeys != 1) {
        if (eType != NiAnimationKey::LINKEY && eType != NiAnimationKey::STEPKEY) {
            return;
        }
        for (uint32_t ui = 1; ui < uiNumKeys && bCollapse; ui++) {
            if (pkKeys->GetKeyAt(ui, ucKeySize)->GetBool() != bValue) {
                bCollapse = false;
            }
        }
    }

    if (bCollapse) {
        m_spBoolData = nullptr;
        m_bBoolValue = bValue;
    }
}

void NiBoolInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    if (m_spBoolData) {
        m_spBoolData->GuaranteeKeysAtStartAndEnd(fStartTime, fEndTime);
    }
}

NiInterpolator* NiBoolInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiBoolInterpolator* pkSeqInterp = static_cast<NiBoolInterpolator*>(
        NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime)
    );
    if (m_spBoolData) {
        NiPointer<NiBoolData> spNewData;
        m_spBoolData->GetSequenceData(fStartTime, fEndTime, spNewData);
        pkSeqInterp->m_spBoolData = spNewData.m_pObject;
        pkSeqInterp->m_uiLastIdx = 0;
    }
    return pkSeqInterp;
}

uint32_t NiBoolInterpolator::GetKeyCount(uint16_t) const {
    if (!m_spBoolData) {
        return 0;
    }
    return m_spBoolData->m_uiNumKeys;
}

NiAnimationKey::KeyType NiBoolInterpolator::GetKeyType(uint16_t) const {
    if (!m_spBoolData) {
        return NiAnimationKey::NOINTERP;
    }
    return m_spBoolData->m_eType;
}

NiAnimationKey::KeyContent NiBoolInterpolator::GetKeyContent(uint16_t) const {
    return NiAnimationKey::BOOLKEY;
}

NiAnimationKey* NiBoolInterpolator::GetKeyArray(uint16_t) const {
    if (!m_spBoolData) {
        return nullptr;
    }
    return m_spBoolData->m_pkKeys;
}

uint8_t NiBoolInterpolator::GetKeyStride(uint16_t) const {
    if (!m_spBoolData) {
        return 0;
    }
    return m_spBoolData->m_ucKeySize;
}

// Posed: no keys, but a valid value.
bool NiBoolInterpolator::GetChannelPosed(uint16_t) const {
    if (m_spBoolData) {
        return false;
    }
    return m_bBoolValue != INVALID_BOOL;
}

void NiBoolInterpolator::SetPoseValue(bool bPoseValue) {
    m_spBoolData = nullptr;
    m_bBoolValue = bPoseValue;
}

void NiBoolInterpolator::ReplaceKeys(NiBoolKey* pkKeys, NiAnimationKey::KeyType eType, uint32_t uiNumKeys) {
    if (uiNumKeys) {
        if (!m_spBoolData.m_pObject) {
            NiBoolData* pkData = new NiBoolData;
            m_spBoolData.SetNew(pkData);
        }
        m_spBoolData->ReplaceAnim(pkKeys, uiNumKeys, eType);
        m_uiLastIdx = 0;
    }
    else {
        NiBoolData* pkData = m_spBoolData.m_pObject;
        if (pkData) {
            pkData->ReplaceAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        }
    }
}

NiBoolKey* NiBoolInterpolator::GetKeys(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
    NiBoolData* m_pObject = m_spBoolData.m_pObject;
    if (m_pObject) {
        uiNumKeys = m_pObject->m_uiNumKeys;
        eType = m_pObject->m_eType;
        ucSize = m_pObject->m_ucKeySize;
        return m_pObject->m_pkKeys;
    }

    uiNumKeys = 0;
    eType = NiAnimationKey::NOINTERP;
    ucSize = 0;
    return nullptr;
}

void NiBoolInterpolator::CopyMembers(NiBoolInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiKeyBasedInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_bBoolValue = m_bBoolValue;
    NiBoolData* m_pObject = pkDest->m_spBoolData.m_pObject;
    if (m_pObject == m_spBoolData.m_pObject) {
        pkDest->m_uiLastIdx = m_uiLastIdx;
    }
    else {
        if (m_pObject) {
            NiDecRefCount(m_pObject);
        }
        NiBoolData* v5 = m_spBoolData.m_pObject;
        pkDest->m_spBoolData.m_pObject = v5;
        if (v5) {
            NiIncRefCount(v5);
        }
        pkDest->m_uiLastIdx = m_uiLastIdx;
    }
}

NiObject* NiBoolInterpolator::CreateObject() {
    return new NiBoolInterpolator(static_cast<NiBoolData*>(nullptr));
}
