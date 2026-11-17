#include "Gamebryo2.2/CoreLibs/NiMain/NiStringsExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/string.h"

#include <cstring>

NiRTTI NiStringsExtraData::ms_RTTI("NiStringsExtraData", &NiExtraData::ms_RTTI);

NiStringsExtraData::NiStringsExtraData() : NiExtraData(), m_uiSize(0), m_ppcValue(nullptr) {}

NiStringsExtraData::~NiStringsExtraData() {
    for (uint32_t i = 0; i < m_uiSize; ++i) {
        _NiFree(m_ppcValue[i]);
    }
    _NiFree(m_ppcValue);
    m_ppcValue = nullptr;
}

NiObject* NiStringsExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiStringsExtraData* pkObject =
        static_cast<NiStringsExtraData*>(NiMemObject::operator new(sizeof(NiStringsExtraData)));
    if (pkObject) {
        ::new (pkObject) NiStringsExtraData;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }
    else {
        CopyMembers(0, kCloning);
        return 0;
    }
}

// The old array and strings are not freed.
void NiStringsExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_uiSize);
    if (m_uiSize == 0) {
        m_ppcValue = nullptr;
        return;
    }

    m_ppcValue = static_cast<char**>(_NiMalloc(m_uiSize * sizeof(char*)));
    for (uint32_t i = 0; i < m_uiSize; i++) {
        m_ppcValue[i] = nullptr;
        kStream.LoadCString(m_ppcValue[i]);
    }
}

void NiStringsExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiStringsExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

void NiStringsExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiSize);
    for (uint32_t i = 0; i < m_uiSize; i++) {
        kStream.SaveCString(m_ppcValue[i]);
    }
}

// Neither the base class nor the type is checked. Null arrays and null strings only match null.
bool NiStringsExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    NiStringsExtraData* pkOther = static_cast<NiStringsExtraData*>(pObject);
    if (m_uiSize != pkOther->m_uiSize) {
        return false;
    }

    if ((m_ppcValue == nullptr) != (pkOther->m_ppcValue == nullptr)) {
        return false;
    }

    if (m_ppcValue) {
        for (uint32_t i = 0; i < m_uiSize; i++) {
            const char* pcValue = m_ppcValue[i];
            const char* pcOther = pkOther->m_ppcValue[i];
            if ((pcValue == nullptr) != (pcOther == nullptr)) {
                return false;
            }
            if (pcValue && std::strcmp(pcValue, pcOther) != 0) {
                return false;
            }
        }
    }
    return true;
}

void NiStringsExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiStringsExtraData::ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiSize", m_uiSize));
    for (uint32_t i = 0; i < m_uiSize; i++) {
        pStrings->Add(NiGetViewerString("m_ppcValue[i]", static_cast<const char*>(m_ppcValue[i])));
    }
}

// Copies the base data, then the array and every string; a null or empty array leaves the destination null and zero.
void NiStringsExtraData::CopyMembers(NiStringsExtraData* pDest, NiCloningProcess& kCloning) {
    NiExtraData::CopyMembers(pDest, kCloning);
    if (m_ppcValue && m_uiSize != 0) {
        pDest->m_uiSize = m_uiSize;
        pDest->m_ppcValue = static_cast<char**>(_NiMalloc(m_uiSize * sizeof(char*)));
        for (uint32_t i = 0; i < m_uiSize; ++i) {
            if (m_ppcValue[i]) {
                size_t uiLen = std::strlen(m_ppcValue[i]) + 1;
                pDest->m_ppcValue[i] = static_cast<char*>(_NiMalloc(uiLen));
                strcpy_s(pDest->m_ppcValue[i], uiLen, m_ppcValue[i]);
            }
            else {
                pDest->m_ppcValue[i] = nullptr;
            }
        }
    }
    else {
        pDest->m_ppcValue = nullptr;
        pDest->m_uiSize = 0;
    }
}

NiObject* NiStringsExtraData::CreateObject() {
    NiStringsExtraData* pkObject =
        static_cast<NiStringsExtraData*>(NiMemObject::operator new(sizeof(NiStringsExtraData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiStringsExtraData;
}
