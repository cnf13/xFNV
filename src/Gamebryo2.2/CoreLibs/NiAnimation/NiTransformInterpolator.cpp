#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiEulerRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiKeyBasedInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiTransformInterpolator::ms_RTTI("NiTransformInterpolator", &NiKeyBasedInterpolator::ms_RTTI);

namespace {

    // Widens [arfBegin, arfEnd] to cover the first and last of auiNumKeys keys.
    void
    GrowTimeRange(const NiAnimationKey* apkKeys, uint32_t auiNumKeys, uint8_t aucSize, float& arfBegin, float& arfEnd) {
        float fTime = apkKeys->m_fTime;
        if (arfBegin > fTime) {
            arfBegin = fTime;
        }
        fTime = apkKeys->GetKeyAt(auiNumKeys - 1, aucSize)->m_fTime;
        if (arfEnd < fTime) {
            arfEnd = fTime;
        }
    }

} // namespace

// m_kTransformValue's own constructor leaves every part invalid.
NiTransformInterpolator::NiTransformInterpolator(NiTransformData* a1)
  : m_spData(a1), m_usLastTransIdx(0), m_usLastRotIdx(0), m_usLastScaleIdx(0), bPose(false) {}

NiTransformInterpolator::NiTransformInterpolator(NiQuatTransform kPoseValue)
  : m_kTransformValue(kPoseValue), m_spData(nullptr), m_usLastTransIdx(0), m_usLastRotIdx(0), m_usLastScaleIdx(0),
    bPose(false) {}

NiTransformInterpolator::~NiTransformInterpolator() {}

NiObject* NiTransformInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiTransformInterpolator* v4 = new NiTransformInterpolator(nullptr);
    CopyMembers(v4, kCloning);
    return v4;
}

// The data link is resolved at once (nif binds it in LinkObject). Without data, a transform with any valid part is a
// fixed pose.
void NiTransformInterpolator::LoadBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::LoadBinary(kStream);

    m_kTransformValue.LoadBinary(kStream);
    m_origTranslate = m_kTransformValue.m_kTranslate;
    m_spData = static_cast<NiTransformData*>(kStream.ResolveLinkID());
    bPose = !m_spData && !m_kTransformValue.IsTransformInvalid();
}

void NiTransformInterpolator::LinkObject(NiStream& kStream) {
    NiKeyBasedInterpolator::LinkObject(kStream);
}

bool NiTransformInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiKeyBasedInterpolator::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spData) {
        m_spData->RegisterStreamables(kStream);
    }
    return true;
}

void NiTransformInterpolator::SaveBinary(NiStream& kStream) {
    NiKeyBasedInterpolator::SaveBinary(kStream);

    m_kTransformValue.SaveBinary(kStream);
    kStream.SaveLinkID(m_spData);
}

bool NiTransformInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiKeyBasedInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiTransformInterpolator* pkOther = static_cast<NiTransformInterpolator*>(pkObject);
    if (!(m_kTransformValue == pkOther->m_kTransformValue)) {
        return false;
    }

    if ((m_spData && !pkOther->m_spData) || (!m_spData && pkOther->m_spData) ||
        (m_spData && !m_spData->IsEqual(pkOther->m_spData))) {
        return false;
    }
    return true;
}

void NiTransformInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiKeyBasedInterpolator::GetViewerStrings(pkStrings);

    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    m_kTransformValue.GetViewerStrings(pkStrings);
    if (m_spData) {
        m_spData->GetViewerStrings(pkStrings);
    }
}

// Samples each channel that has keys into the stored transform. A fixed pose and an unchanged time return the
// stored transform as is; the time is only recorded when the result is valid.
bool NiTransformInterpolator::Update(float fTime, NiObjectNET*, NiQuatTransform& kValue) {
    if (bPose) {
        m_fLastTime = fTime;
        kValue = m_kTransformValue;
        return true;
    }

    if (fTime == m_fLastTime) {
        kValue = m_kTransformValue;
        return !kValue.IsTransformInvalid();
    }

    uint32_t uiNumKeys;
    NiAnimationKey::KeyType eType;
    uint8_t ucSize;

    NiPosKey* pkPosKeys = GetPosData(uiNumKeys, eType, ucSize);
    if (uiNumKeys > 0) {
        uint32_t uiLastIdx = m_usLastTransIdx;
        m_kTransformValue.m_kTranslate = NiPosKey::GenInterp(fTime, pkPosKeys, eType, uiNumKeys, uiLastIdx, ucSize);
        m_usLastTransIdx = static_cast<uint16_t>(uiLastIdx);
    }

    NiRotKey* pkRotKeys = GetRotData(uiNumKeys, eType, ucSize);
    if (uiNumKeys > 0) {
        uint32_t uiLastIdx = m_usLastRotIdx;
        m_kTransformValue.m_kRotate = NiRotKey::GenInterp(fTime, pkRotKeys, eType, uiNumKeys, uiLastIdx, ucSize);
        m_usLastRotIdx = static_cast<uint16_t>(uiLastIdx);
    }

    NiFloatKey* pkScaleKeys = GetScaleData(uiNumKeys, eType, ucSize);
    if (uiNumKeys > 0) {
        uint32_t uiLastIdx = m_usLastScaleIdx;
        m_kTransformValue.m_fScale = NiFloatKey::GenInterp(fTime, pkScaleKeys, eType, uiNumKeys, uiLastIdx, ucSize);
        m_usLastScaleIdx = static_cast<uint16_t>(uiLastIdx);
    }

    kValue = m_kTransformValue;
    if (kValue.IsTransformInvalid()) {
        return false;
    }

    m_fLastTime = fTime;
    return true;
}

// KEEP: the exe pushes the whole dword of its key-size slot (mov edx,[esp+0x14] at 0xA3FE6F, 0xA3FED5, 0xA3FF41)
// to each GenInterp, but GetPosData, GetRotData and GetScaleData write only its low byte: the upper three bytes are
// an uninitialised read src cannot reproduce.
// A channel whose keys all hold one value (a single key, or linear or step keys that never change) becomes that
// constant value and loses its keys. Data left without keys is dropped.
void NiTransformInterpolator::Collapse() {
    if (!m_spData) {
        return;
    }

    NiQuatTransform kTransform;
    kTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    kTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    kTransform.m_fScale = -FLT_MAX;

    NiTransformData* pkData = m_spData;
    uint32_t uiNumKeys = pkData->m_usNumPosKeys;
    NiAnimationKey::KeyType eType = pkData->m_ePosType;
    NiPosKey* pkPosKeys = pkData->m_pkPosKeys;
    if (uiNumKeys == 0) {
        pkData->ReplacePosAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        if (m_kTransformValue.IsTranslateValid()) {
            kTransform.m_kTranslate = m_kTransformValue.m_kTranslate;
        }
    }
    else {
        NiPoint3 kPos = pkPosKeys->m_Pos;
        bool bCollapse = uiNumKeys == 1;
        if (!bCollapse && (eType == NiAnimationKey::LINKEY || eType == NiAnimationKey::STEPKEY)) {
            bCollapse = true;
            for (uint32_t ui = 1; ui < uiNumKeys && bCollapse; ui++) {
                if (pkPosKeys->GetKeyAt(ui, pkData->m_ucPosSize)->m_Pos != kPos) {
                    bCollapse = false;
                }
            }
        }

        if (bCollapse) {
            pkData->ReplacePosAnim(nullptr, 0, NiAnimationKey::NOINTERP);
            kTransform.m_kTranslate = kPos;
        }
        else if (m_kTransformValue.IsTranslateValid()) {
            kTransform.m_kTranslate = m_kTransformValue.m_kTranslate;
        }
    }

    pkData = m_spData;
    uiNumKeys = pkData->m_usNumRotKeys;
    eType = pkData->m_eRotType;
    NiRotKey* pkRotKeys = pkData->m_pkRotKeys;
    if (uiNumKeys == 0) {
        pkData->ReplaceRotAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        if (m_kTransformValue.IsRotateValid()) {
            kTransform.m_kRotate = m_kTransformValue.m_kRotate;
        }
    }
    else {
        NiQuaternion kRot = pkRotKeys->m_quat;
        bool bCollapse = uiNumKeys == 1 && eType != NiAnimationKey::EULERKEY;
        if (!bCollapse && (eType == NiAnimationKey::LINKEY || eType == NiAnimationKey::STEPKEY)) {
            bCollapse = true;
            for (uint32_t ui = 1; ui < pkData->m_usNumRotKeys && bCollapse; ui++) {
                const NiQuaternion& kKey = pkRotKeys->GetKeyAt(ui, pkData->m_ucRotSize)->m_quat;
                if (!(kKey == kRot)) {
                    bCollapse = false;
                }
            }
        }

        if (bCollapse) {
            pkData->ReplaceRotAnim(nullptr, 0, NiAnimationKey::NOINTERP);
            kTransform.m_kRotate = kRot;
        }
        else if (m_kTransformValue.IsRotateValid()) {
            kTransform.m_kRotate = m_kTransformValue.m_kRotate;
        }
    }

    pkData = m_spData;
    uiNumKeys = pkData->m_usNumScaleKeys;
    eType = pkData->m_eScaleType;
    NiFloatKey* pkScaleKeys = pkData->m_pkScaleKeys;
    if (uiNumKeys == 0) {
        pkData->ReplaceScaleAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        if (m_kTransformValue.IsScaleValid()) {
            kTransform.m_fScale = m_kTransformValue.m_fScale;
        }
    }
    else {
        float fScale = pkScaleKeys->m_fValue;
        bool bCollapse = uiNumKeys == 1;
        if (!bCollapse && (eType == NiAnimationKey::LINKEY || eType == NiAnimationKey::STEPKEY)) {
            bCollapse = true;
            for (uint32_t ui = 1; ui < uiNumKeys && bCollapse; ui++) {
                if (pkScaleKeys->GetKeyAt(ui, pkData->m_ucScaleSize)->m_fValue != fScale) {
                    bCollapse = false;
                }
            }
        }

        if (bCollapse) {
            pkData->ReplaceScaleAnim(nullptr, 0, NiAnimationKey::NOINTERP);
            kTransform.m_fScale = fScale;
        }
        else if (m_kTransformValue.IsScaleValid()) {
            kTransform.m_fScale = m_kTransformValue.m_fScale;
        }
    }

    pkData = m_spData;
    if (pkData->m_usNumPosKeys == 0 && pkData->m_usNumRotKeys == 0 && pkData->m_usNumScaleKeys == 0) {
        m_spData = nullptr;
    }

    m_kTransformValue = kTransform;
}

// Euler rotations take the range of their three float key channels. Without keys the range is [0, 0].
void NiTransformInterpolator::GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const {
    fBeginKeyTime = FLT_MAX;
    fEndKeyTime = -FLT_MAX;
    bool bKeys = false;

    NiTransformData* pkData = m_spData;
    if (pkData) {
        uint32_t uiNumKeys = pkData->m_usNumPosKeys;
        if (uiNumKeys > 0) {
            GrowTimeRange(pkData->m_pkPosKeys, uiNumKeys, pkData->m_ucPosSize, fBeginKeyTime, fEndKeyTime);
            bKeys = true;
        }

        uiNumKeys = pkData->m_usNumRotKeys;
        if (uiNumKeys > 0) {
            if (pkData->m_eRotType == NiAnimationKey::EULERKEY) {
                NiEulerRotKey* pkEuler = static_cast<NiEulerRotKey*>(pkData->m_pkRotKeys);
                for (uint32_t ui = 0; ui < 3; ui++) {
                    if (pkEuler->m_uiNumKeys[ui] > 0) {
                        GrowTimeRange(
                            pkEuler->m_apkKeys[ui], pkEuler->m_uiNumKeys[ui], pkEuler->m_ucSizes[ui], fBeginKeyTime,
                            fEndKeyTime
                        );
                        bKeys = true;
                    }
                }
            }
            else {
                GrowTimeRange(pkData->m_pkRotKeys, uiNumKeys, pkData->m_ucRotSize, fBeginKeyTime, fEndKeyTime);
                bKeys = true;
            }
        }

        uiNumKeys = pkData->m_usNumScaleKeys;
        if (uiNumKeys > 0) {
            GrowTimeRange(pkData->m_pkScaleKeys, uiNumKeys, pkData->m_ucScaleSize, fBeginKeyTime, fEndKeyTime);
            bKeys = true;
        }
    }

    if (!bKeys) {
        fBeginKeyTime = 0.0f;
        fEndKeyTime = 0.0f;
    }
}

void NiTransformInterpolator::GuaranteeTimeRange(float fStartTime, float fEndTime) {
    if (m_spData) {
        m_spData->GuaranteeKeysAtStartAndEnd(fStartTime, fEndTime);
    }
}

// The sequence copy gets the data cut to [afStartTime, afEndTime].
NiInterpolator* NiTransformInterpolator::GetSequenceInterpolator(float fStartTime, float fEndTime) {
    NiTransformInterpolator* pkSeqInterp =
        static_cast<NiTransformInterpolator*>(NiInterpolator::GetSequenceInterpolator(fStartTime, fEndTime));
    if (pkSeqInterp && m_spData) {
        NiPointer<NiTransformData> spNewData;
        m_spData->GetSequenceData(fStartTime, fEndTime, spNewData);
        pkSeqInterp->SetTransformData(spNewData);
        pkSeqInterp->m_usLastTransIdx = 0;
        pkSeqInterp->m_usLastRotIdx = 0;
        pkSeqInterp->m_usLastScaleIdx = 0;
    }
    return pkSeqInterp;
}

uint32_t NiTransformInterpolator::GetKeyCount(uint16_t usChannel) const {
    if (!m_spData) {
        return 0;
    }

    switch (usChannel) {
        case POSITION: return m_spData->m_usNumPosKeys;
        case ROTATION: return m_spData->m_usNumRotKeys;
        case SCALE: return m_spData->m_usNumScaleKeys;
        default: return 0;
    }
}

NiAnimationKey::KeyType NiTransformInterpolator::GetKeyType(uint16_t usChannel) const {
    if (!m_spData) {
        return NiAnimationKey::NOINTERP;
    }

    switch (usChannel) {
        case POSITION: return m_spData->m_ePosType;
        case ROTATION: return m_spData->m_eRotType;
        case SCALE: return m_spData->m_eScaleType;
        default: return NiAnimationKey::NOINTERP;
    }
}

NiAnimationKey::KeyContent NiTransformInterpolator::GetKeyContent(uint16_t usChannel) const {
    switch (usChannel) {
        case POSITION: return NiAnimationKey::POSKEY;
        case ROTATION: return NiAnimationKey::ROTKEY;
        case SCALE: return NiAnimationKey::FLOATKEY;
        default: return NiAnimationKey::NUMKEYCONTENTS;
    }
}

NiAnimationKey* NiTransformInterpolator::GetKeyArray(uint16_t usChannel) const {
    if (!m_spData) {
        return nullptr;
    }

    switch (usChannel) {
        case POSITION: return m_spData->m_pkPosKeys;
        case ROTATION: return m_spData->m_pkRotKeys;
        case SCALE: return m_spData->m_pkScaleKeys;
        default: return nullptr;
    }
}

uint8_t NiTransformInterpolator::GetKeyStride(uint16_t usChannel) const {
    if (!m_spData) {
        return 0;
    }

    switch (usChannel) {
        case POSITION: return m_spData->m_ucPosSize;
        case ROTATION: return m_spData->m_ucRotSize;
        case SCALE: return m_spData->m_ucScaleSize;
        default: return 0;
    }
}

// A channel is posed when it has no keys and its part of the stored transform is valid.
bool NiTransformInterpolator::GetChannelPosed(uint16_t usChannel) const {
    switch (usChannel) {
        case POSITION:
            if (m_spData && GetKeyCount(usChannel) > 0) {
                return false;
            }
            return m_kTransformValue.IsTranslateValid();
        case ROTATION:
            if (m_spData && GetKeyCount(usChannel) > 0) {
                return false;
            }
            return m_kTransformValue.IsRotateValid();
        case SCALE:
            if (m_spData && GetKeyCount(usChannel) > 0) {
                return false;
            }
            return m_kTransformValue.IsScaleValid();
        default: return false;
    }
}

void NiTransformInterpolator::SetPoseValue(NiQuatTransform kPoseValue) {
    if (kPoseValue.IsRotateValid()) {
        m_kTransformValue.m_kRotate = kPoseValue.m_kRotate;
        if (m_spData) {
            m_spData->ReplaceRotAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        }
    }
    if (kPoseValue.IsScaleValid()) {
        m_kTransformValue.m_fScale = kPoseValue.m_fScale;
        if (m_spData) {
            m_spData->ReplaceScaleAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        }
    }
    if (kPoseValue.IsTranslateValid()) {
        m_kTransformValue.m_kTranslate = kPoseValue.m_kTranslate;
        if (m_spData) {
            m_spData->ReplacePosAnim(nullptr, 0, NiAnimationKey::NOINTERP);
        }
    }
}

void NiTransformInterpolator::SetTransformData(NiTransformData* pkData) {
    m_spData = pkData;
    m_usLastRotIdx = 0;
    m_usLastScaleIdx = 0;
    m_usLastTransIdx = 0;
}

NiPosKey*
NiTransformInterpolator::GetPosData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
    if (!m_spData) {
        uiNumKeys = 0;
        eType = NiAnimationKey::NOINTERP;
        ucSize = 0;
        return nullptr;
    }

    uiNumKeys = m_spData->m_usNumPosKeys;
    eType = m_spData->m_ePosType;
    ucSize = m_spData->m_ucPosSize;
    return m_spData->m_pkPosKeys;
}

NiRotKey*
NiTransformInterpolator::GetRotData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
    if (!m_spData) {
        uiNumKeys = 0;
        eType = NiAnimationKey::NOINTERP;
        ucSize = 0;
        return nullptr;
    }

    return m_spData->GetRotAnim(uiNumKeys, eType, ucSize);
}

NiFloatKey*
NiTransformInterpolator::GetScaleData(uint32_t& uiNumKeys, NiAnimationKey::KeyType& eType, uint8_t& ucSize) const {
    if (!m_spData) {
        uiNumKeys = 0;
        eType = NiAnimationKey::NOINTERP;
        ucSize = 0;
        return nullptr;
    }

    uiNumKeys = m_spData->m_usNumScaleKeys;
    eType = m_spData->m_eScaleType;
    ucSize = m_spData->m_ucScaleSize;
    return m_spData->m_pkScaleKeys;
}

void NiTransformInterpolator::CopyMembers(NiTransformInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);

    pkDest->m_kTransformValue = m_kTransformValue;
    pkDest->m_origTranslate = m_origTranslate;
    pkDest->m_spData = m_spData;
    pkDest->m_usLastTransIdx = m_usLastTransIdx;
    pkDest->m_usLastRotIdx = m_usLastRotIdx;
    pkDest->m_usLastScaleIdx = m_usLastScaleIdx;
}

NiObject* NiTransformInterpolator::CreateObject() {
    return new NiTransformInterpolator(nullptr);
}
