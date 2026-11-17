#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchStringExtraData.h"

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

NiRTTI NiSwitchStringExtraData::ms_RTTI("NiSwitchStringExtraData", &NiExtraData::ms_RTTI);

NiSwitchStringExtraData::NiSwitchStringExtraData() : NiExtraData(), m_uiSize(0), m_ppcValue(nullptr), m_iIndex(-1) {}

NiSwitchStringExtraData::~NiSwitchStringExtraData() {
    for (uint32_t i = 0; i < m_uiSize; ++i) {
        _NiFree(m_ppcValue[i]);
    }
    _NiFree(m_ppcValue);
    m_ppcValue = nullptr;
}

NiObject* NiSwitchStringExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiSwitchStringExtraData* pkObject =
        static_cast<NiSwitchStringExtraData*>(NiMemObject::operator new(sizeof(NiSwitchStringExtraData)));
    if (pkObject) {
        ::new (pkObject) NiSwitchStringExtraData;
        CopyMembers(pkObject, kCloning);
        return pkObject;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// The old array and strings are not freed.
void NiSwitchStringExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);

    NiStreamLoadBinary(kStream, m_uiSize);
    if (m_uiSize == 0) {
        m_ppcValue = nullptr;
    }
    else {
        m_ppcValue = static_cast<char**>(_NiMalloc(m_uiSize * sizeof(char*)));
        for (uint32_t i = 0; i < m_uiSize; i++) {
            kStream.LoadCString(m_ppcValue[i]);
        }
    }

    NiStreamLoadBinary(kStream, m_iIndex);
}

void NiSwitchStringExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiSwitchStringExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

void NiSwitchStringExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);

    NiStreamSaveBinary(kStream, m_uiSize);
    for (uint32_t i = 0; i < m_uiSize; i++) {
        kStream.SaveCString(m_ppcValue[i]);
    }
    NiStreamSaveBinary(kStream, m_iIndex);
}

// Neither the base class nor the type is checked. Null arrays and null strings only match null.
bool NiSwitchStringExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    NiSwitchStringExtraData* pkOther = static_cast<NiSwitchStringExtraData*>(pObject);
    if (m_uiSize != pkOther->m_uiSize || m_iIndex != pkOther->m_iIndex) {
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

void NiSwitchStringExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);

    pStrings->Add(NiGetViewerString(NiSwitchStringExtraData::ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiSize", m_uiSize));
    for (uint32_t i = 0; i < m_uiSize; i++) {
        pStrings->Add(NiGetViewerString("m_ppcValue[i]", static_cast<const char*>(m_ppcValue[i])));
    }
    pStrings->Add(NiGetViewerString("m_iIndex", m_iIndex));
}

// Copies the base data, then the array and every string, and the index clamped to the new size (-1 when it falls
// outside it); a null or empty array leaves the destination null and zero.
void NiSwitchStringExtraData::CopyMembers(NiSwitchStringExtraData* pDest, NiCloningProcess& kCloning) {
    NiExtraData::CopyMembers(pDest, kCloning);
    if (m_ppcValue && m_uiSize != 0) {
        pDest->m_uiSize = m_uiSize;
        if (m_iIndex > -1 && m_iIndex < static_cast<int32_t>(m_uiSize)) {
            pDest->m_iIndex = m_iIndex;
        }
        else {
            pDest->m_iIndex = -1;
        }
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

NiObject* NiSwitchStringExtraData::CreateObject() {
    NiSwitchStringExtraData* pkObject =
        static_cast<NiSwitchStringExtraData*>(NiMemObject::operator new(sizeof(NiSwitchStringExtraData)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiSwitchStringExtraData;
}
