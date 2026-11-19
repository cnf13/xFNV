#include "Gamebryo2.2/CoreLibs/NiAnimation/NiUVData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiUVData::ms_RTTI("NiUVData", &NiObject::ms_RTTI);

NiUVData::NiUVData()
  : m_uiNumUOffsetKeys(0), m_pkUOffsetKeys(nullptr), m_eUOffsetType(NiAnimationKey::NOINTERP), m_uiNumVOffsetKeys(0),
    m_pkVOffsetKeys(nullptr), m_eVOffsetType(NiAnimationKey::NOINTERP), m_uiNumUTilingKeys(0), m_pkUTilingKeys(nullptr),
    m_eUTilingType(NiAnimationKey::NOINTERP), m_uiNumVTilingKeys(0), m_pkVTilingKeys(nullptr),
    m_eVTilingType(NiAnimationKey::NOINTERP), m_fLastUOffset(0.0f), m_fLastVOffset(0.0f), m_fLastUTiling(1.0f),
    m_fLastVTiling(1.0f), m_ucUOffsetSize(0), m_ucVOffsetSize(0), m_ucUTilingSize(0), m_ucVTilingSize(0) {}

NiUVData::~NiUVData() {
    if (m_pkUOffsetKeys) {
        NiAnimationKey::ms_deletes[m_eUOffsetType](m_pkUOffsetKeys);
    }
    if (m_pkVOffsetKeys) {
        NiAnimationKey::ms_deletes[m_eVOffsetType](m_pkVOffsetKeys);
    }
    if (m_pkUTilingKeys) {
        NiAnimationKey::ms_deletes[m_eUTilingType](m_pkUTilingKeys);
    }
    if (m_pkVTilingKeys) {
        NiAnimationKey::ms_deletes[m_eVTilingType](m_pkVTilingKeys);
    }
}

const NiRTTI* NiUVData::GetRTTI() const {
    return &ms_RTTI;
}

// Each channel loads its count, then its type and keys.
void NiUVData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    uint32_t uiNumUOffsetKeys;
    NiStreamLoadBinary(kStream, uiNumUOffsetKeys);
    if (uiNumUOffsetKeys) {
        NiFloatKey::KeyType eType;
        NiStreamLoadBinary(kStream, eType);
        m_ucUOffsetSize = NiFloatKey::GetKeySize(eType);
        NiFloatKey* pkKeys = static_cast<NiFloatKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY, eType)(kStream, uiNumUOffsetKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(
            pkKeys, uiNumUOffsetKeys, m_ucUOffsetSize
        );
        ReplaceUOffsetAnim(pkKeys, uiNumUOffsetKeys, eType);
    }

    uint32_t uiNumVOffsetKeys;
    NiStreamLoadBinary(kStream, uiNumVOffsetKeys);
    if (uiNumVOffsetKeys) {
        NiFloatKey::KeyType eType;
        NiStreamLoadBinary(kStream, eType);
        m_ucVOffsetSize = NiFloatKey::GetKeySize(eType);
        NiFloatKey* pkKeys = static_cast<NiFloatKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY, eType)(kStream, uiNumVOffsetKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(
            pkKeys, uiNumVOffsetKeys, m_ucVOffsetSize
        );
        ReplaceVOffsetAnim(pkKeys, uiNumVOffsetKeys, eType);
    }

    uint32_t uiNumUTilingKeys;
    NiStreamLoadBinary(kStream, uiNumUTilingKeys);
    if (uiNumUTilingKeys) {
        NiFloatKey::KeyType eType;
        NiStreamLoadBinary(kStream, eType);
        m_ucUTilingSize = NiFloatKey::GetKeySize(eType);
        NiFloatKey* pkKeys = static_cast<NiFloatKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY, eType)(kStream, uiNumUTilingKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(
            pkKeys, uiNumUTilingKeys, m_ucUTilingSize
        );
        ReplaceUTilingAnim(pkKeys, uiNumUTilingKeys, eType);
    }

    uint32_t uiNumVTilingKeys;
    NiStreamLoadBinary(kStream, uiNumVTilingKeys);
    if (uiNumVTilingKeys) {
        NiFloatKey::KeyType eType;
        NiStreamLoadBinary(kStream, eType);
        m_ucVTilingSize = NiFloatKey::GetKeySize(eType);
        NiFloatKey* pkKeys = static_cast<NiFloatKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::FLOATKEY, eType)(kStream, uiNumVTilingKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::FLOATKEY, eType)(
            pkKeys, uiNumVTilingKeys, m_ucVTilingSize
        );
        ReplaceVTilingAnim(pkKeys, uiNumVTilingKeys, eType);
    }
}

// KEEP: the exe reads each key count into the stream argument's own slot (lea ecx,[esp+0x28] at 0xA453C9) and each
// key type into a slot nothing wrote, so a short read (malformed data) leaves the stream's address or stack garbage
// there, which src cannot express; porting.md's unwritten-slot exception covers GetChunkData only.
void NiUVData::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiUVData::RegisterStreamables(NiStream& kStream) {
    return NiObject::RegisterStreamables(kStream);
}

// Each channel saves its count, then its type and keys.
void NiUVData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiNumUOffsetKeys);
    if (m_uiNumUOffsetKeys) {
        NiStreamSaveBinary(kStream, m_eUOffsetType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::FLOATKEY, m_eUOffsetType)(
            kStream, m_pkUOffsetKeys, m_uiNumUOffsetKeys
        );
    }

    NiStreamSaveBinary(kStream, m_uiNumVOffsetKeys);
    if (m_uiNumVOffsetKeys) {
        NiStreamSaveBinary(kStream, m_eVOffsetType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::FLOATKEY, m_eVOffsetType)(
            kStream, m_pkVOffsetKeys, m_uiNumVOffsetKeys
        );
    }

    NiStreamSaveBinary(kStream, m_uiNumUTilingKeys);
    if (m_uiNumUTilingKeys) {
        NiStreamSaveBinary(kStream, m_eUTilingType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::FLOATKEY, m_eUTilingType)(
            kStream, m_pkUTilingKeys, m_uiNumUTilingKeys
        );
    }

    NiStreamSaveBinary(kStream, m_uiNumVTilingKeys);
    if (m_uiNumVTilingKeys) {
        NiStreamSaveBinary(kStream, m_eVTilingType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::FLOATKEY, m_eVTilingType)(
            kStream, m_pkVTilingKeys, m_uiNumVTilingKeys
        );
    }
}

bool NiUVData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiUVData* pkData = static_cast<NiUVData*>(pkObject);
    if (m_uiNumUOffsetKeys != pkData->m_uiNumUOffsetKeys || m_eUOffsetType != pkData->m_eUOffsetType ||
        m_uiNumVOffsetKeys != pkData->m_uiNumVOffsetKeys || m_eVOffsetType != pkData->m_eVOffsetType ||
        m_uiNumUTilingKeys != pkData->m_uiNumUTilingKeys || m_eUTilingType != pkData->m_eUTilingType ||
        m_uiNumVTilingKeys != pkData->m_uiNumVTilingKeys || m_eVTilingType != pkData->m_eVTilingType) {
        return false;
    }

    if (m_ucUOffsetSize != pkData->m_ucUOffsetSize || m_ucVOffsetSize != pkData->m_ucVOffsetSize ||
        m_ucUTilingSize != pkData->m_ucUTilingSize || m_ucVTilingSize != pkData->m_ucVTilingSize) {
        return false;
    }

    NiAnimationKey::EqualFunction pfnUOffsetEqual =
        NiAnimationKey::GetEqualFunction(NiAnimationKey::FLOATKEY, m_eUOffsetType);
    for (uint32_t i = 0; i < m_uiNumUOffsetKeys; i++) {
        if (!pfnUOffsetEqual(
                *m_pkUOffsetKeys->GetKeyAt(i, m_ucUOffsetSize), *pkData->m_pkUOffsetKeys->GetKeyAt(i, m_ucUOffsetSize)
            )) {
            return false;
        }
    }

    NiAnimationKey::EqualFunction pfnVOffsetEqual =
        NiAnimationKey::GetEqualFunction(NiAnimationKey::FLOATKEY, m_eVOffsetType);
    for (uint32_t i = 0; i < m_uiNumVOffsetKeys; i++) {
        if (!pfnVOffsetEqual(
                *m_pkVOffsetKeys->GetKeyAt(i, m_ucVOffsetSize), *pkData->m_pkVOffsetKeys->GetKeyAt(i, m_ucVOffsetSize)
            )) {
            return false;
        }
    }

    NiAnimationKey::EqualFunction pfnUTilingEqual =
        NiAnimationKey::GetEqualFunction(NiAnimationKey::FLOATKEY, m_eUTilingType);
    for (uint32_t i = 0; i < m_uiNumUTilingKeys; i++) {
        if (!pfnUTilingEqual(
                *m_pkUTilingKeys->GetKeyAt(i, m_ucUTilingSize), *pkData->m_pkUTilingKeys->GetKeyAt(i, m_ucUTilingSize)
            )) {
            return false;
        }
    }

    NiAnimationKey::EqualFunction pfnVTilingEqual =
        NiAnimationKey::GetEqualFunction(NiAnimationKey::FLOATKEY, m_eVTilingType);
    for (uint32_t i = 0; i < m_uiNumVTilingKeys; i++) {
        if (!pfnVTilingEqual(
                *m_pkVTilingKeys->GetKeyAt(i, m_ucVTilingSize), *pkData->m_pkVTilingKeys->GetKeyAt(i, m_ucVTilingSize)
            )) {
            return false;
        }
    }

    return true;
}

void NiUVData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiNumUOffsetKeys", m_uiNumUOffsetKeys));
    pStrings->Add(NiGetViewerString("m_uiNumUTilingKeys", m_uiNumUTilingKeys));
    pStrings->Add(NiGetViewerString("m_uiNumVOffsetKeys", m_uiNumVOffsetKeys));
    pStrings->Add(NiGetViewerString("m_uiNumVTilingKeys", m_uiNumVTilingKeys));
}

// Frees the current UOffset keys and takes over pkKeys.
void NiUVData::ReplaceUOffsetAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkUOffsetKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eUOffsetType)(m_pkUOffsetKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_uiNumUOffsetKeys = uiNumKeys;
        m_pkUOffsetKeys = pkKeys;
        m_eUOffsetType = eType;
        m_ucUOffsetSize = NiFloatKey::GetKeySize(eType);
    }
    else {
        m_uiNumUOffsetKeys = 0;
        m_pkUOffsetKeys = nullptr;
        m_eUOffsetType = NiAnimationKey::NOINTERP;
        m_ucUOffsetSize = 0;
    }
}

// Frees the current VOffset keys and takes over pkKeys.
void NiUVData::ReplaceVOffsetAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkVOffsetKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eVOffsetType)(m_pkVOffsetKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_uiNumVOffsetKeys = uiNumKeys;
        m_pkVOffsetKeys = pkKeys;
        m_eVOffsetType = eType;
        m_ucVOffsetSize = NiFloatKey::GetKeySize(eType);
    }
    else {
        m_uiNumVOffsetKeys = 0;
        m_pkVOffsetKeys = nullptr;
        m_eVOffsetType = NiAnimationKey::NOINTERP;
        m_ucVOffsetSize = 0;
    }
}

// Frees the current UTiling keys and takes over pkKeys.
void NiUVData::ReplaceUTilingAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkUTilingKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eUTilingType)(m_pkUTilingKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_uiNumUTilingKeys = uiNumKeys;
        m_pkUTilingKeys = pkKeys;
        m_eUTilingType = eType;
        m_ucUTilingSize = NiFloatKey::GetKeySize(eType);
    }
    else {
        m_uiNumUTilingKeys = 0;
        m_pkUTilingKeys = nullptr;
        m_eUTilingType = NiAnimationKey::NOINTERP;
        m_ucUTilingSize = 0;
    }
}

// Frees the current VTiling keys and takes over pkKeys.
void NiUVData::ReplaceVTilingAnim(NiFloatKey* pkKeys, uint32_t uiNumKeys, NiAnimationKey::KeyType eType) {
    if (m_pkVTilingKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::FLOATKEY, m_eVTilingType)(m_pkVTilingKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_uiNumVTilingKeys = uiNumKeys;
        m_pkVTilingKeys = pkKeys;
        m_eVTilingType = eType;
        m_ucVTilingSize = NiFloatKey::GetKeySize(eType);
    }
    else {
        m_uiNumVTilingKeys = 0;
        m_pkVTilingKeys = nullptr;
        m_eVTilingType = NiAnimationKey::NOINTERP;
        m_ucVTilingSize = 0;
    }
}

NiObject* NiUVData::CreateObject() {
    return new NiUVData;
}
