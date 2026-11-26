#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiEulerRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiTransformData::ms_RTTI("NiTransformData", &NiObject::ms_RTTI);

NiTransformData::NiTransformData()
  : m_usNumRotKeys(0), m_usNumPosKeys(0), m_usNumScaleKeys(0), m_eRotType(NiAnimationKey::NOINTERP),
    m_ePosType(NiAnimationKey::NOINTERP), m_eScaleType(NiAnimationKey::NOINTERP), m_ucRotSize(0), m_ucPosSize(0),
    m_ucScaleSize(0), m_pkRotKeys(nullptr), m_pkPosKeys(nullptr), m_pkScaleKeys(nullptr) {}

NiTransformData::~NiTransformData() {
    if (m_pkRotKeys) {
        if (m_eRotType == NiAnimationKey::EULERKEY) {
            static_cast<NiEulerRotKey*>(m_pkRotKeys)->CleanUp();
        }
        NiAnimationKey::ms_deletes[m_eRotType + 12](m_pkRotKeys);
    }
    if (m_pkPosKeys) {
        NiAnimationKey::ms_deletes[m_ePosType + 6](m_pkPosKeys);
    }
    if (m_pkScaleKeys) {
        NiAnimationKey::ms_deletes[m_eScaleType](m_pkScaleKeys);
    }
}

const NiRTTI* NiTransformData::GetRTTI() const {
    return &ms_RTTI;
}

// Each channel loads its count, then its type and keys; counts past the u16 fields are cut to 65535 keys.
void NiTransformData::LoadBinary(NiStream& stream) {
    // The u16 key counts cap each channel at this many keys.
    constexpr uint32_t MAX_TRANSFORM_KEYS = 0xFFFF;

    NiObject::LoadBinary(stream);

    uint32_t uiNumRotKeys;
    NiStreamLoadBinary(stream, uiNumRotKeys);
    if (uiNumRotKeys) {
        NiRotKey::KeyType eType;
        NiStreamLoadBinary(stream, eType);
        uint8_t ucSize = NiRotKey::GetKeySize(eType);
        NiRotKey* pkKeys = static_cast<NiRotKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::ROTKEY, eType)(stream, uiNumRotKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::ROTKEY, eType)(pkKeys, uiNumRotKeys, ucSize);
        if (uiNumRotKeys > MAX_TRANSFORM_KEYS) {
            uiNumRotKeys = MAX_TRANSFORM_KEYS;
        }
        ReplaceRotAnim(pkKeys, uiNumRotKeys, eType);
    }

    uint32_t uiNumPosKeys;
    NiStreamLoadBinary(stream, uiNumPosKeys);
    if (uiNumPosKeys) {
        NiPosKey::KeyType eType;
        NiStreamLoadBinary(stream, eType);
        uint8_t ucSize = NiPosKey::GetKeySize(eType);
        NiPosKey* pkKeys = static_cast<NiPosKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::POSKEY, eType)(stream, uiNumPosKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::POSKEY, eType)(pkKeys, uiNumPosKeys, ucSize);
        if (uiNumPosKeys > MAX_TRANSFORM_KEYS) {
            uiNumPosKeys = MAX_TRANSFORM_KEYS;
        }
        ReplacePosAnim(pkKeys, uiNumPosKeys, eType);
    }

    uint32_t uiNumScaleKeys;
    NiStreamLoadBinary(stream, uiNumScaleKeys);
    if (uiNumScaleKeys) {
        NiFloatKey::KeyType eType;
        NiStreamLoadBinary(stream, eType);
        uint8_t ucSize = NiFloatKey::GetKeySize(eType);
        NiFloatKey* pkKeys = static_cast<NiFloatKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY, eType)(stream, uiNumScaleKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(pkKeys, uiNumScaleKeys, ucSize);
        if (uiNumScaleKeys > MAX_TRANSFORM_KEYS) {
            uiNumScaleKeys = MAX_TRANSFORM_KEYS;
        }
        ReplaceScaleAnim(pkKeys, uiNumScaleKeys, eType);
    }
}

// KEEP: each key count is read into the stream argument's own stack slot and each key type into a slot nothing wrote,
// so a short read (malformed data) leaves the stream's address or stack garbage there, which src cannot express;
// porting.md's unwritten-slot exception covers GetChunkData only.
void NiTransformData::LinkObject(NiStream& stream) {
    NiObject::LinkObject(stream);
}

bool NiTransformData::RegisterStreamables(NiStream& stream) {
    return NiObject::RegisterStreamables(stream);
}

// Each channel saves its count as 32 bits, then its type and keys.
void NiTransformData::SaveBinary(NiStream& stream) {
    NiObject::SaveBinary(stream);

    uint32_t uiNumRotKeys = m_usNumRotKeys;
    NiStreamSaveBinary(stream, uiNumRotKeys);
    if (uiNumRotKeys) {
        NiStreamSaveBinary(stream, m_eRotType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::ROTKEY, m_eRotType)(stream, m_pkRotKeys, uiNumRotKeys);
    }

    uint32_t uiNumPosKeys = m_usNumPosKeys;
    NiStreamSaveBinary(stream, uiNumPosKeys);
    if (uiNumPosKeys) {
        NiStreamSaveBinary(stream, m_ePosType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::POSKEY, m_ePosType)(stream, m_pkPosKeys, uiNumPosKeys);
    }

    uint32_t uiNumScaleKeys = m_usNumScaleKeys;
    NiStreamSaveBinary(stream, uiNumScaleKeys);
    if (uiNumScaleKeys) {
        NiStreamSaveBinary(stream, m_eScaleType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::FLOATKEY, m_eScaleType)(stream, m_pkScaleKeys, uiNumScaleKeys);
    }
}

bool NiTransformData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiTransformData* pkData = static_cast<NiTransformData*>(pkObject);
    if (m_usNumRotKeys != pkData->m_usNumRotKeys ||
        m_eRotType != pkData->m_eRotType ||
        m_usNumPosKeys != pkData->m_usNumPosKeys ||
        m_ePosType != pkData->m_ePosType ||
        m_usNumScaleKeys != pkData->m_usNumScaleKeys ||
        m_eScaleType != pkData->m_eScaleType) {
        return false;
    }

    if (m_ucRotSize != pkData->m_ucRotSize ||
        m_ucPosSize != pkData->m_ucPosSize ||
        m_ucScaleSize != pkData->m_ucScaleSize) {
        return false;
    }

    NiAnimationKey::EqualFunction pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::ROTKEY, m_eRotType);
    for (uint32_t i = 0; i < m_usNumRotKeys; i++) {
        if (!pfnEqual(*m_pkRotKeys->GetKeyAt(i, m_ucRotSize), *pkData->m_pkRotKeys->GetKeyAt(i, m_ucRotSize))) {
            return false;
        }
    }

    pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::POSKEY, m_ePosType);
    for (uint32_t i = 0; i < m_usNumPosKeys; i++) {
        if (!pfnEqual(*m_pkPosKeys->GetKeyAt(i, m_ucPosSize), *pkData->m_pkPosKeys->GetKeyAt(i, m_ucPosSize))) {
            return false;
        }
    }

    pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::FLOATKEY, m_eScaleType);
    for (uint32_t i = 0; i < m_usNumScaleKeys; i++) {
        if (!pfnEqual(*m_pkScaleKeys->GetKeyAt(i, m_ucScaleSize), *pkData->m_pkScaleKeys->GetKeyAt(i, m_ucScaleSize))) {
            return false;
        }
    }

    return true;
}

void NiTransformData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_usNumRotKeys", m_usNumRotKeys));
    pStrings->Add(NiGetViewerString("m_usNumPosKeys", m_usNumPosKeys));
    pStrings->Add(NiGetViewerString("m_usNumScaleKeys", m_usNumScaleKeys));
}

void NiTransformData::GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime) {
    int32_t x1;
    NiAnimationKey* pkKeys;
    uint16_t m_usNumRotKeys = this->m_usNumRotKeys;
    if (m_usNumRotKeys) {
        NiRotKey* m_pkRotKeys = this->m_pkRotKeys;
        x1 = m_usNumRotKeys;
        pkKeys = m_pkRotKeys;
        NiAnimationKey::GuaranteeKeyAtStartAndEnd(
            NiAnimationKey::ROTKEY,
            m_eRotType,
            pkKeys,
            *reinterpret_cast<uint32_t*>(&x1),
            fStartTime,
            fEndTime
        );
        NiRotKey* v6 = static_cast<NiRotKey*>(pkKeys);
        this->m_usNumRotKeys = x1;
        this->m_pkRotKeys = v6;
    }
    uint16_t m_usNumPosKeys = this->m_usNumPosKeys;
    if (m_usNumPosKeys) {
        NiPosKey* m_pkPosKeys = this->m_pkPosKeys;
        x1 = m_usNumPosKeys;
        pkKeys = m_pkPosKeys;
        NiAnimationKey::GuaranteeKeyAtStartAndEnd(
            NiAnimationKey::POSKEY,
            m_ePosType,
            pkKeys,
            *reinterpret_cast<uint32_t*>(&x1),
            fStartTime,
            fEndTime
        );
        NiPosKey* v9 = static_cast<NiPosKey*>(pkKeys);
        this->m_usNumPosKeys = x1;
        this->m_pkPosKeys = v9;
    }
    uint16_t m_usNumScaleKeys = this->m_usNumScaleKeys;
    if (m_usNumScaleKeys) {
        int32_t v11 = m_usNumScaleKeys;
        NiFloatKey* m_pkScaleKeys = this->m_pkScaleKeys;
        x1 = v11;
        pkKeys = m_pkScaleKeys;
        NiAnimationKey::GuaranteeKeyAtStartAndEnd(
            NiAnimationKey::FLOATKEY,
            m_eScaleType,
            pkKeys,
            *reinterpret_cast<uint32_t*>(&x1),
            fStartTime,
            fEndTime
        );
        NiFloatKey* v13 = static_cast<NiFloatKey*>(pkKeys);
        this->m_usNumScaleKeys = x1;
        this->m_pkScaleKeys = v13;
    }
}

void NiTransformData::GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiTransformData>& aspTransformData) {
    NiPointer<NiObject> spDeepCopy;
    CreateDeepCopy(spDeepCopy);
    NiTransformData* m_pObject = static_cast<NiTransformData*>(spDeepCopy.m_pObject);
    if (spDeepCopy.m_pObject) {
        NiIncRefCount(spDeepCopy.m_pObject);
    }
    uint16_t m_usNumRotKeys = this->m_usNumRotKeys;
    NiAnimationKey* pkNewKeys = nullptr;
    uint32_t uiNewNumKeys = 0;
    if (m_usNumRotKeys) {
        NiAnimationKey::CopySequence(
            NiAnimationKey::ROTKEY,
            m_eRotType,
            m_pkRotKeys,
            m_usNumRotKeys,
            fStartTime,
            fEndTime,
            pkNewKeys,
            uiNewNumKeys
        );
        m_pObject->ReplaceRotAnim(static_cast<NiRotKey*>(pkNewKeys), uiNewNumKeys, m_eRotType);
    }
    uint16_t m_usNumPosKeys = this->m_usNumPosKeys;
    if (m_usNumPosKeys) {
        NiAnimationKey::CopySequence(
            NiAnimationKey::POSKEY,
            m_ePosType,
            m_pkPosKeys,
            m_usNumPosKeys,
            fStartTime,
            fEndTime,
            pkNewKeys,
            uiNewNumKeys
        );
        m_pObject->ReplacePosAnim(static_cast<NiPosKey*>(pkNewKeys), uiNewNumKeys, m_ePosType);
    }
    uint16_t m_usNumScaleKeys = this->m_usNumScaleKeys;
    if (m_usNumScaleKeys) {
        NiAnimationKey::CopySequence(
            NiAnimationKey::FLOATKEY,
            m_eScaleType,
            m_pkScaleKeys,
            m_usNumScaleKeys,
            fStartTime,
            fEndTime,
            pkNewKeys,
            uiNewNumKeys
        );
        m_pObject->ReplaceScaleAnim(static_cast<NiFloatKey*>(pkNewKeys), uiNewNumKeys, m_eScaleType);
    }
    NiTransformData* v9 = aspTransformData.m_pObject;
    if (aspTransformData.m_pObject != m_pObject) {
        if (v9) {
            NiDecRefCount(v9);
        }
        aspTransformData.m_pObject = m_pObject;
        if (m_pObject) {
            NiIncRefCount(m_pObject);
        }
    }
    if (m_pObject) {
        NiDecRefCount(m_pObject);
    }
}

// Frees the current rotation keys (Euler keys first free their angle arrays) and takes over pkKeys.
void NiTransformData::ReplaceRotAnim(NiRotKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkRotKeys) {
        if (m_eRotType == NiAnimationKey::EULERKEY) {
            static_cast<NiEulerRotKey*>(m_pkRotKeys)->CleanUp();
        }
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::ROTKEY, m_eRotType)(m_pkRotKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_usNumRotKeys = static_cast<uint16_t>(uiNumKeys);
        m_pkRotKeys = pkKeys;
        m_eRotType = eType;
        m_ucRotSize = NiRotKey::GetKeySize(eType);
    }
    else {
        m_usNumRotKeys = 0;
        m_pkRotKeys = nullptr;
        m_eRotType = NiAnimationKey::NOINTERP;
        m_ucRotSize = 0;
    }
}

// Frees the current position keys and takes over pkKeys.
void NiTransformData::ReplacePosAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkPosKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::POSKEY, m_ePosType)(m_pkPosKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_usNumPosKeys = static_cast<uint16_t>(uiNumKeys);
        m_pkPosKeys = pkKeys;
        m_ePosType = eType;
        m_ucPosSize = NiPosKey::GetKeySize(eType);
    }
    else {
        m_usNumPosKeys = 0;
        m_pkPosKeys = nullptr;
        m_ePosType = NiAnimationKey::NOINTERP;
        m_ucPosSize = 0;
    }
}

// Frees the current scale keys and takes over pkKeys.
void NiTransformData::ReplaceScaleAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkScaleKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eScaleType)(m_pkScaleKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_usNumScaleKeys = static_cast<uint16_t>(uiNumKeys);
        m_pkScaleKeys = pkKeys;
        m_eScaleType = eType;
        m_ucScaleSize = NiFloatKey::GetKeySize(eType);
    }
    else {
        m_usNumScaleKeys = 0;
        m_pkScaleKeys = nullptr;
        m_eScaleType = NiAnimationKey::NOINTERP;
        m_ucScaleSize = 0;
    }
}

void NiTransformData::SetPosAnim(NiPosKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (pkKeys && uiNumKeys) {
        m_usNumPosKeys = static_cast<uint16_t>(uiNumKeys);
        m_pkPosKeys = pkKeys;
        m_ePosType = eType;
        m_ucPosSize = NiAnimationKey::ms_keysizes[static_cast<int32_t>(eType) + 6];
    }
    else {
        m_usNumPosKeys = 0;
        m_pkPosKeys = nullptr;
        m_ePosType = NiAnimationKey::NOINTERP;
        m_ucPosSize = 0;
    }
}

void NiTransformData::SetScaleAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (pkKeys && uiNumKeys) {
        m_usNumScaleKeys = static_cast<uint16_t>(uiNumKeys);
        m_pkScaleKeys = pkKeys;
        m_eScaleType = eType;
        m_ucScaleSize = NiAnimationKey::ms_keysizes[static_cast<int32_t>(eType)];
    }
    else {
        m_usNumScaleKeys = 0;
        m_pkScaleKeys = nullptr;
        m_ucScaleSize = 0;
        m_eScaleType = NiAnimationKey::NOINTERP;
    }
}

NiObject* NiTransformData::CreateObject() {
    return new NiTransformData;
}
