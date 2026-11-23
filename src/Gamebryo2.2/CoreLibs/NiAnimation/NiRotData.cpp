#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiRotKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiRotData::ms_RTTI("NiRotData", &NiObject::ms_RTTI);

NiRotData::NiRotData()
  : NiObject(), m_uiNumKeys(0), m_pkKeys(nullptr), m_eType(NiAnimationKey::NOINTERP), m_ucKeySize(0) {}

NiRotData::~NiRotData() {
    if (m_pkKeys) {
        NiAnimationKey::ms_deletes[m_eType + 12](m_pkKeys);
    }
}

const NiRTTI* NiRotData::GetRTTI() const {
    return &ms_RTTI;
}

void NiRotData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    uint32_t uiNumKeys;
    NiStreamLoadBinary(kStream, uiNumKeys);
    if (uiNumKeys) {
        NiRotKey::KeyType eType;
        NiStreamLoadBinary(kStream, eType);
        m_ucKeySize = NiRotKey::GetKeySize(eType);
        NiRotKey* pkKeys = static_cast<NiRotKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::ROTKEY, eType)(kStream, uiNumKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::ROTKEY, eType)(pkKeys, uiNumKeys, m_ucKeySize);
        ReplaceAnim(pkKeys, uiNumKeys, eType);
    }
}

// KEEP: the exe reads the key count into the stream argument's own slot and the key type into a slot nothing wrote,
// so a short read (malformed data) leaves the stream's address or stack garbage there, which src cannot express;
// porting.md's unwritten-slot exception covers GetChunkData only.
void NiRotData::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiRotData::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

void NiRotData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiNumKeys);
    if (m_uiNumKeys) {
        NiStreamSaveBinary(kStream, m_eType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::ROTKEY, m_eType)(kStream, m_pkKeys, m_uiNumKeys);
    }
}

bool NiRotData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiRotData* pkData = static_cast<NiRotData*>(pkObject);
    if (m_uiNumKeys != pkData->m_uiNumKeys || m_eType != pkData->m_eType || m_ucKeySize != pkData->m_ucKeySize) {
        return false;
    }

    NiAnimationKey::EqualFunction pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::ROTKEY, m_eType);
    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        if (!pfnEqual(*m_pkKeys->GetKeyAt(i, m_ucKeySize), *pkData->m_pkKeys->GetKeyAt(i, m_ucKeySize))) {
            return false;
        }
    }

    return true;
}

void NiRotData::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiObject::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_uiNumKeys", m_uiNumKeys));
}

// Passes the position key content here too, as Gamebryo's source does.
void NiRotData::GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime) {
    NiAnimationKey* pkKeys = m_pkKeys;
    NiAnimationKey::GuaranteeKeyAtStartAndEnd(
        NiAnimationKey::POSKEY,
        m_eType,
        pkKeys,
        m_uiNumKeys,
        fStartTime,
        fEndTime
    );
    m_pkKeys = static_cast<NiRotKey*>(pkKeys);
}

// A deep copy of this data reduced to the keys between the start and end times, stored in aspNewData.
void NiRotData::GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiRotData>& aspRotData) {
    NiPointer<NiObject> spCopy;
    CreateDeepCopy(spCopy);
    NiPointer<NiRotData> spNewData = static_cast<NiRotData*>(spCopy.m_pObject);
    if (m_uiNumKeys) {
        NiAnimationKey* pkNewKeys = nullptr;
        uint32_t uiNewNumKeys = 0;
        NiAnimationKey::CopySequence(
            NiAnimationKey::ROTKEY,
            m_eType,
            m_pkKeys,
            m_uiNumKeys,
            fStartTime,
            fEndTime,
            pkNewKeys,
            uiNewNumKeys
        );
        spNewData->ReplaceAnim(static_cast<NiRotKey*>(pkNewKeys), uiNewNumKeys, m_eType);
    }
    aspRotData = spNewData;
}

// Frees the current keys and takes over pkKeys; with no keys the data is left empty.
void NiRotData::ReplaceAnim(NiRotKey* pkKeys, uint32_t uiNumKeys, NiRotKey::KeyType eType) {
    if (m_pkKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::ROTKEY, m_eType)(m_pkKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_uiNumKeys = uiNumKeys;
        m_pkKeys = pkKeys;
        m_eType = eType;
        m_ucKeySize = NiRotKey::GetKeySize(eType);
    }
    else {
        m_uiNumKeys = 0;
        m_pkKeys = nullptr;
        m_eType = NiAnimationKey::NOINTERP;
        m_ucKeySize = 0;
    }
}

NiObject* NiRotData::CreateObject() {
    return new NiRotData;
}
