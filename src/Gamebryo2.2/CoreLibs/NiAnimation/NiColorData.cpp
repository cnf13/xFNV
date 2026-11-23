#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiColorData::ms_RTTI("NiColorData", &NiObject::ms_RTTI);

NiColorData::NiColorData() : m_uiNumKeys(0), m_pkKeys(nullptr), m_eType(NiAnimationKey::NOINTERP) {}

NiColorData::~NiColorData() {
    if (m_pkKeys) {
        NiAnimationKey::ms_deletes[m_eType + 18](m_pkKeys);
    }
}

const NiRTTI* NiColorData::GetRTTI() const {
    return &ms_RTTI;
}

void NiColorData::LoadBinary(NiStream& stream) {
    NiObject::LoadBinary(stream);

    uint32_t uiNumKeys;
    NiStreamLoadBinary(stream, uiNumKeys);
    if (uiNumKeys) {
        NiColorKey::KeyType eType;
        NiStreamLoadBinary(stream, eType);
        // No derived values are filled for color keys.
        NiColorKey* pkKeys = static_cast<NiColorKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::COLORKEY, eType)(stream, uiNumKeys)
        );
        ReplaceAnim(pkKeys, uiNumKeys, eType);
    }
}

// KEEP: the exe reads the key count into the stream argument's own slot and the key type into a slot nothing wrote,
// so a short read (malformed data) leaves the stream's address or stack garbage there, which src cannot express;
// porting.md's unwritten-slot exception covers GetChunkData only.
void NiColorData::LinkObject(NiStream& stream) {
    NiObject::LinkObject(stream);
}

bool NiColorData::RegisterStreamables(NiStream& stream) {
    return NiObject::RegisterStreamables(stream);
}

void NiColorData::SaveBinary(NiStream& stream) {
    NiObject::SaveBinary(stream);

    NiStreamSaveBinary(stream, m_uiNumKeys);
    if (m_uiNumKeys) {
        NiStreamSaveBinary(stream, m_eType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::COLORKEY, m_eType)(stream, m_pkKeys, m_uiNumKeys);
    }
}

bool NiColorData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiColorData* pkData = static_cast<NiColorData*>(pkObject);
    if (m_uiNumKeys != pkData->m_uiNumKeys || m_eType != pkData->m_eType || m_ucKeySize != pkData->m_ucKeySize) {
        return false;
    }

    // Tests only the low byte of the compare's result, and returns 1 after the loop.
    NiAnimationKey::EqualFunction pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::COLORKEY, m_eType);
    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        uint8_t ucEqual = pfnEqual(*m_pkKeys->GetKeyAt(i, m_ucKeySize), *pkData->m_pkKeys->GetKeyAt(i, m_ucKeySize));
        if (!ucEqual) {
            return false;
        }
    }
    return true;
}

void NiColorData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiNumKeys", m_uiNumKeys));
}

// Adds a key at each of afStartTime and afEndTime when the array lacks one, reallocating it; the asm keeps the key
// pointer in a local and hands the count field by reference.
void NiColorData::GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime) {
    NiAnimationKey* pkKeys = m_pkKeys;
    NiAnimationKey::GuaranteeKeyAtStartAndEnd(
        NiAnimationKey::COLORKEY,
        m_eType,
        pkKeys,
        m_uiNumKeys,
        fStartTime,
        fEndTime
    );
    m_pkKeys = static_cast<NiColorKey*>(pkKeys);
}

// A deep copy of this object narrowed to the keys between afStartTime and afEndTime. The deep copy is held in a smart
// pointer for the duration of the call, while the smart-pointer assignment to the output adds the reference the caller
// keeps.
void NiColorData::GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiColorData>& aspColorData) {
    NiPointer<NiObject> spCopy;
    CreateDeepCopy(spCopy);
    NiPointer<NiColorData> spNewData = static_cast<NiColorData*>(spCopy.m_pObject);
    if (m_uiNumKeys > 0) {
        NiAnimationKey* pkNewKeys = nullptr;
        uint32_t uiNewNumKeys = 0;
        NiAnimationKey::CopySequence(
            NiAnimationKey::COLORKEY,
            m_eType,
            m_pkKeys,
            m_uiNumKeys,
            fStartTime,
            fEndTime,
            pkNewKeys,
            uiNewNumKeys
        );
        spNewData->ReplaceAnim(static_cast<NiColorKey*>(pkNewKeys), uiNewNumKeys, m_eType);
    }
    aspColorData = spNewData;
}

// Frees the current keys and takes over pkKeys; with no keys the data is left empty.
void NiColorData::ReplaceAnim(NiColorKey* pkKeys, uint32_t uiNumKeys, NiColorKey::KeyType eType) {
    if (m_pkKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::COLORKEY, m_eType)(m_pkKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_uiNumKeys = uiNumKeys;
        m_pkKeys = pkKeys;
        m_eType = eType;
        m_ucKeySize = NiColorKey::GetKeySize(eType);
    }
    else {
        m_uiNumKeys = 0;
        m_pkKeys = nullptr;
        m_eType = NiAnimationKey::NOINTERP;
        m_ucKeySize = 0;
    }
}

NiObject* NiColorData::CreateObject() {
    return new NiColorData;
}
