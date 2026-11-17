#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiFloatData::ms_RTTI("NiFloatData", &NiObject::ms_RTTI);

NiFloatData::NiFloatData() : m_uiNumKeys(0), m_pkKeys(nullptr), m_eType(NiAnimationKey::NOINTERP), m_ucKeySize(0) {}

NiFloatData::~NiFloatData() {
    if (m_pkKeys) {
        NiAnimationKey::ms_deletes[m_eType](m_pkKeys);
    }
}

const NiRTTI* NiFloatData::GetRTTI() const {
    return &ms_RTTI;
}

void NiFloatData::LoadBinary(NiStream& stream) {
    NiObject::LoadBinary(stream);

    // uiNumKeys starts as the stream's address so that a read that fills nothing leaves that value, as the game does.
    uint32_t uiNumKeys = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(&stream));
    NiStreamLoadBinary(stream, uiNumKeys);
    if (uiNumKeys) {
        NiFloatKey::KeyType eType;
        NiStreamLoadBinary(stream, eType);
        m_ucKeySize = NiFloatKey::GetKeySize(eType);
        NiFloatKey* pkKeys = static_cast<NiFloatKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY, eType)(stream, uiNumKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(pkKeys, uiNumKeys, m_ucKeySize);
        ReplaceAnim(pkKeys, uiNumKeys, eType);
    }
}

void NiFloatData::LinkObject(NiStream& stream) {
    NiObject::LinkObject(stream);
}

bool NiFloatData::RegisterStreamables(NiStream& stream) {
    return NiObject::RegisterStreamables(stream);
}

void NiFloatData::SaveBinary(NiStream& stream) {
    NiObject::SaveBinary(stream);

    NiStreamSaveBinary(stream, m_uiNumKeys);
    if (m_uiNumKeys) {
        NiStreamSaveBinary(stream, m_eType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::FLOATKEY, m_eType)(stream, m_pkKeys, m_uiNumKeys);
    }
}

bool NiFloatData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiFloatData* pkData = static_cast<NiFloatData*>(pkObject);
    if (m_uiNumKeys != pkData->m_uiNumKeys || m_eType != pkData->m_eType || m_ucKeySize != pkData->m_ucKeySize) {
        return false;
    }

    // Tests only the low byte of the equal function's result, and returns 1 after the loop.
    NiAnimationKey::EqualFunction pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::FLOATKEY, m_eType);
    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        uint8_t ucEqual = pfnEqual(*m_pkKeys->GetKeyAt(i, m_ucKeySize), *pkData->m_pkKeys->GetKeyAt(i, m_ucKeySize));
        if (!ucEqual) {
            return false;
        }
    }

    return true;
}

void NiFloatData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiNumKeys", m_uiNumKeys));
}

void NiFloatData::GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime) {
    NiAnimationKey* pkKeys = m_pkKeys;
    NiAnimationKey::GuaranteeKeyAtStartAndEnd(
        NiAnimationKey::FLOATKEY, m_eType, pkKeys, m_uiNumKeys, fStartTime, fEndTime
    );
    m_pkKeys = static_cast<NiFloatKey*>(pkKeys);
}

// A deep copy of this object narrowed to the keys between afStartTime and afEndTime. The asm holds the deep copy in
// a smart pointer: one reference taken right after the call and released at the end, while the
// smart-pointer assignment to the output adds the one the caller keeps.
void NiFloatData::GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiFloatData>& aspFloatData) {
    NiPointer<NiObject> spCopy;
    CreateDeepCopy(spCopy);
    NiPointer<NiFloatData> spNewData = static_cast<NiFloatData*>(spCopy.m_pObject);
    if (m_uiNumKeys > 0) {
        NiAnimationKey* pkNewKeys = nullptr;
        uint32_t uiNewNumKeys = 0;
        NiAnimationKey::CopySequence(
            NiAnimationKey::FLOATKEY, m_eType, m_pkKeys, m_uiNumKeys, fStartTime, fEndTime, pkNewKeys, uiNewNumKeys
        );
        spNewData->ReplaceAnim(static_cast<NiFloatKey*>(pkNewKeys), uiNewNumKeys, m_eType);
    }
    aspFloatData = spNewData;
}

// Frees the current keys and takes over pkKeys; with no keys or no interpolation type the data is left empty.
void NiFloatData::ReplaceAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eType)(m_pkKeys);
    }

    if (pkKeys && uiNumKeys && eType != NiAnimationKey::NOINTERP) {
        m_uiNumKeys = uiNumKeys;
        m_pkKeys = pkKeys;
        m_eType = eType;
        m_ucKeySize = NiFloatKey::GetKeySize(eType);
    }
    else {
        m_uiNumKeys = 0;
        m_pkKeys = nullptr;
        m_eType = NiAnimationKey::NOINTERP;
        m_ucKeySize = 0;
    }
}

// With no keys or no interpolation type the data is left empty; the current keys are not freed.
void NiFloatData::SetAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (!pkKeys || !uiNumKeys || eType == NiAnimationKey::NOINTERP) {
        m_uiNumKeys = 0;
        m_pkKeys = nullptr;
        m_eType = NiAnimationKey::NOINTERP;
        m_ucKeySize = 0;
        return;
    }
    m_pkKeys = pkKeys;
    m_uiNumKeys = uiNumKeys;
    m_eType = eType;
    m_ucKeySize = NiFloatKey::GetKeySize(eType);
}

NiObject* NiFloatData::CreateObject() {
    void* pkObject = NiMemObject::operator new(sizeof(NiFloatData));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiFloatData;
}
