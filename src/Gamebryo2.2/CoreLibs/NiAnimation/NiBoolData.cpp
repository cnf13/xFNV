#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolData.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiAnimationKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBoolData::ms_RTTI("NiBoolData", &NiObject::ms_RTTI);

NiBoolData::NiBoolData() : m_uiNumKeys(0), m_pkKeys(nullptr), m_eType(NiAnimationKey::NOINTERP), m_ucKeySize(0) {}

// The keys' block starts at their count cookie, the size_t before the first key.
NiBoolData::~NiBoolData() {
    if (m_pkKeys) {
        MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(m_pkKeys) - 1);
    }
}

const NiRTTI* NiBoolData::GetRTTI() const {
    return &ms_RTTI;
}

void NiBoolData::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);

    uint32_t uiNumKeys;
    NiStreamLoadBinary(kStream, uiNumKeys);
    if (uiNumKeys) {
        // Files before 10.1.0.104 store no type: bool keys were always step keys.
        NiBoolKey::KeyType eType = NiAnimationKey::STEPKEY;
        if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 1, 0, 104)) {
            NiStreamLoadBinary(kStream, eType);
        }
        m_ucKeySize = NiBoolKey::GetKeySize(eType);
        NiBoolKey* pkKeys = static_cast<NiBoolKey*>(
            NiAnimationKey::GetCreateFunction(NiAnimationKey::BOOLKEY, eType)(kStream, uiNumKeys)
        );
        NiAnimationKey::GetFillDerivedFunction(NiAnimationKey::BOOLKEY, eType)(pkKeys, uiNumKeys, m_ucKeySize);
        ReplaceAnim(pkKeys, uiNumKeys, eType);
    }
}

// KEEP: the exe reads the key count into the stream argument's own slot and the key type into a slot nothing wrote,
// so a short read (malformed data) leaves the stream's address or stack garbage there, which src cannot express;
// porting.md's unwritten-slot exception covers GetChunkData only.
void NiBoolData::LinkObject(NiStream& stream) {
    NiObject::LinkObject(stream);
}

bool NiBoolData::RegisterStreamables(NiStream& stream) {
    return NiObject::RegisterStreamables(stream);
}

void NiBoolData::SaveBinary(NiStream& kStream) {
    NiObject::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiNumKeys);
    if (m_uiNumKeys) {
        NiStreamSaveBinary(kStream, m_eType);
        NiAnimationKey::GetSaveFunction(NiAnimationKey::BOOLKEY, m_eType)(kStream, m_pkKeys, m_uiNumKeys);
    }
}

bool NiBoolData::IsEqual(NiObject* pkObject) {
    if (!NiObject::IsEqual(pkObject)) {
        return false;
    }

    NiBoolData* pkData = static_cast<NiBoolData*>(pkObject);
    if (m_uiNumKeys != pkData->m_uiNumKeys || m_eType != pkData->m_eType || m_ucKeySize != pkData->m_ucKeySize) {
        return false;
    }

    // Tests only the low byte of the compare's result, and returns 1 after the loop.
    NiAnimationKey::EqualFunction pfnEqual = NiAnimationKey::GetEqualFunction(NiAnimationKey::BOOLKEY, m_eType);
    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        uint8_t ucEqual = pfnEqual(*m_pkKeys->GetKeyAt(i, m_ucKeySize), *pkData->m_pkKeys->GetKeyAt(i, m_ucKeySize));
        if (!ucEqual) {
            return false;
        }
    }
    return true;
}

void NiBoolData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiObject::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiNumKeys", m_uiNumKeys));
}

// Adds a key at each of afStartTime and afEndTime when the array lacks one, reallocating it; the asm keeps the key
// pointer in a local and hands the count field by reference.
void NiBoolData::GuaranteeKeysAtStartAndEnd(float fStartTime, float fEndTime) {
    NiAnimationKey* pkKeys = m_pkKeys;
    NiAnimationKey::GuaranteeKeyAtStartAndEnd(
        NiAnimationKey::BOOLKEY, m_eType, pkKeys, m_uiNumKeys, fStartTime, fEndTime
    );
    m_pkKeys = static_cast<NiBoolKey*>(pkKeys);
}

// A deep copy of this object narrowed to the keys between afStartTime and afEndTime. The asm holds the deep copy in
// a smart pointer: one reference taken right after the call and released at the end, while the
// smart-pointer assignment to the output adds the one the caller keeps.
void NiBoolData::GetSequenceData(float fStartTime, float fEndTime, NiPointer<NiBoolData>& aspBoolData) {
    NiPointer<NiObject> spCopy;
    CreateDeepCopy(spCopy);
    NiPointer<NiBoolData> spNewData = static_cast<NiBoolData*>(spCopy.m_pObject);
    if (m_uiNumKeys > 0) {
        NiAnimationKey* pkNewKeys = nullptr;
        uint32_t uiNewNumKeys = 0;
        NiAnimationKey::CopySequence(
            NiAnimationKey::BOOLKEY, m_eType, m_pkKeys, m_uiNumKeys, fStartTime, fEndTime, pkNewKeys, uiNewNumKeys
        );
        spNewData->ReplaceAnim(static_cast<NiBoolKey*>(pkNewKeys), uiNewNumKeys, m_eType);
    }
    aspBoolData = spNewData;
}

// Frees the current keys and takes over pkKeys; with no keys the data is left empty.
void NiBoolData::ReplaceAnim(NiBoolKey* pkKeys, uint32_t uiNumKeys, NiBoolKey::KeyType eType) {
    if (m_pkKeys) {
        NiAnimationKey::GetDeleteFunction(NiAnimationKey::BOOLKEY, m_eType)(m_pkKeys);
    }

    if (pkKeys && uiNumKeys) {
        m_uiNumKeys = uiNumKeys;
        m_pkKeys = pkKeys;
        m_eType = eType;
        m_ucKeySize = NiBoolKey::GetKeySize(eType);
    }
    else {
        m_uiNumKeys = 0;
        m_pkKeys = nullptr;
        m_eType = NiAnimationKey::NOINTERP;
        m_ucKeySize = 0;
    }
}

NiObject* NiBoolData::CreateObject() {
    NiBoolData* pkObject = static_cast<NiBoolData*>(NiMemObject::operator new(sizeof(NiBoolData)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiBoolData;
}
