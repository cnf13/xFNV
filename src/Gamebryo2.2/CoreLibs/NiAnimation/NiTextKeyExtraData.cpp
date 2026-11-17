#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKeyExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiRTTI NiTextKeyExtraData::ms_RTTI("NiTextKeyExtraData", &NiExtraData::ms_RTTI);

NiTextKeyExtraData::NiTextKeyExtraData() : m_uiNumKeys(0), m_pKeys(nullptr) {}

NiTextKeyExtraData::~NiTextKeyExtraData() {
    delete[] m_pKeys;
}

NiObject* NiTextKeyExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiTextKeyExtraData* pkObject =
        static_cast<NiTextKeyExtraData*>(NiMemObject::operator new(sizeof(NiTextKeyExtraData)));
    if (pkObject) {
        ::new (pkObject) NiTextKeyExtraData;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(nullptr, kCloning);
        return 0;
    }
}

// An empty key list in the file leaves the current keys as they are; otherwise the old array is not freed.
void NiTextKeyExtraData::LoadBinary(NiStream& stream) {
    NiExtraData::LoadBinary(stream);

    uint32_t uiNumKeys;
    NiStreamLoadBinary(stream, uiNumKeys);
    if (uiNumKeys == 0) {
        return;
    }

    // Allocates the NiTextKey array as new NiTextKey[uiNumKeys] does: the size saturates on overflow, the count cookie
    // goes before the array and the key constructor runs on each element.
    NiTextKey* pKeys = nullptr;
    uint64_t ulArraySize = static_cast<uint64_t>(sizeof(NiTextKey)) * uiNumKeys;
    uint32_t uiArraySize = ulArraySize > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(ulArraySize);
    uint32_t uiAllocSize = uiArraySize + static_cast<uint32_t>(sizeof(size_t));
    if (uiAllocSize < uiArraySize) {
        uiAllocSize = UINT32_MAX;
    }
    size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
    if (puiCookie) {
        pKeys = reinterpret_cast<NiTextKey*>(puiCookie + 1);
        *puiCookie = uiNumKeys;
        rt::eh_vector_constructor_iterator(pKeys, sizeof(NiTextKey), uiNumKeys, &fnvd::constructElement<NiTextKey>);
    }
    for (uint32_t i = 0; i < uiNumKeys; i++) {
        pKeys[i].LoadBinary(stream);
    }

    m_uiNumKeys = uiNumKeys;
    m_pKeys = pKeys;
}

// KEEP: the exe reads the key count into the stream argument's own slot (lea ecx,[esp+0x20] at 0xA46E22), so a short
// read (malformed data) leaves the stream's address as the count, which src cannot express; porting.md's
// unwritten-slot exception covers GetChunkData only.
void NiTextKeyExtraData::LinkObject(NiStream& stream) {
    NiExtraData::LinkObject(stream);
}

bool NiTextKeyExtraData::RegisterStreamables(NiStream& stream) {
    if (!NiExtraData::RegisterStreamables(stream)) {
        return false;
    }

    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        m_pKeys[i].RegisterStreamables(stream);
    }
    return true;
}

void NiTextKeyExtraData::SaveBinary(NiStream& stream) {
    NiExtraData::SaveBinary(stream);

    NiStreamSaveBinary(stream, m_uiNumKeys);
    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        m_pKeys[i].SaveBinary(stream);
    }
}

// Skips NiExtraData's check: the names are not compared. Keys compare by time and by text handle.
bool NiTextKeyExtraData::IsEqual(NiObject* pObject) {
    if (!pObject || !NiObject::IsEqual(pObject)) {
        return false;
    }

    NiTextKeyExtraData* pkOther = static_cast<NiTextKeyExtraData*>(pObject);
    if (m_uiNumKeys != pkOther->m_uiNumKeys) {
        return false;
    }

    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        if (m_pKeys[i].m_fTime != pkOther->m_pKeys[i].m_fTime) {
            return false;
        }
        if (!(m_pKeys[i].m_kText == pkOther->m_pKeys[i].m_kText)) {
            return false;
        }
    }
    return true;
}

void NiTextKeyExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiTextKeyExtraData::ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiNumKeys", m_uiNumKeys));

    // One "TextKey = Time = ... : Text = ..." line per key, joined from the two single-value strings.
    for (uint32_t i = 0; i < m_uiNumKeys; i++) {
        char* pcTime = NiGetViewerString("Time", m_pKeys[i].m_fTime);
        char* pcText = NiGetViewerString("Text", static_cast<const char*>(m_pKeys[i].m_kText));
        size_t stLen = std::strlen(pcTime) + std::strlen(pcText) + 4;
        char* pcKey = static_cast<char*>(_NiMalloc(stLen));
        NiSprintf(pcKey, stLen, "%s : %s", pcTime, pcText);
        pStrings->Add(NiGetViewerString("TextKey", static_cast<const char*>(pcKey)));
        _NiFree(pcTime);
        _NiFree(pcText);
        _NiFree(pcKey);
    }
}

NiTextKey* NiTextKeyExtraData::GetKeys(uint32_t& uiNumKeys) const {
    uiNumKeys = m_uiNumKeys;
    return m_pKeys;
}

void NiTextKeyExtraData::CopyMembers(NiTextKeyExtraData* pDest, NiCloningProcess& kCloning) {
    NiExtraData::CopyMembers(pDest, kCloning);
    NiTextKey* pKeys = nullptr;
    pDest->m_uiNumKeys = this->m_uiNumKeys;
    if (this->m_uiNumKeys && GetRTTI() == &NiTextKeyExtraData::ms_RTTI) {
        uint32_t uiNumKeys = this->m_uiNumKeys;
        // Allocates the NiTextKey array as new NiTextKey[uiNumKeys] does: the size saturates on overflow, the count
        // cookie goes before the array and the key constructor runs on each element.
        uint64_t ulArraySize = static_cast<uint64_t>(sizeof(NiTextKey)) * uiNumKeys;
        uint32_t uiArraySize = ulArraySize > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(ulArraySize);
        uint32_t uiAllocSize = uiArraySize + static_cast<uint32_t>(sizeof(size_t));
        if (uiAllocSize < uiArraySize) {
            uiAllocSize = UINT32_MAX;
        }
        size_t* puiCookie = static_cast<size_t*>(MemoryManager_Singleton.Allocate(uiAllocSize));
        if (puiCookie) {
            pKeys = reinterpret_cast<NiTextKey*>(puiCookie + 1);
            *puiCookie = uiNumKeys;
            rt::eh_vector_constructor_iterator(pKeys, sizeof(NiTextKey), uiNumKeys, &fnvd::constructElement<NiTextKey>);
        }
        uint32_t uiI = 0;
        for (pDest->m_pKeys = pKeys; uiI < this->m_uiNumKeys; ++uiI) {
            pDest->m_pKeys[uiI].m_fTime = m_pKeys[uiI].GetTime();
            pDest->m_pKeys[uiI].SetText(m_pKeys[uiI].m_kText);
        }
    }
    else {
        pDest->m_pKeys = 0;
    }
}

NiObject* NiTextKeyExtraData::CreateObject() {
    NiTextKeyExtraData* pkObject =
        static_cast<NiTextKeyExtraData*>(NiMemObject::operator new(sizeof(NiTextKeyExtraData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiTextKeyExtraData;
}
