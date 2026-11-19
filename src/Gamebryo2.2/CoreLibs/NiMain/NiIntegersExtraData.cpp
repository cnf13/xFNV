#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegersExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiIntegersExtraData::ms_RTTI("NiIntegersExtraData", &NiExtraData::ms_RTTI);

// An empty array.
NiIntegersExtraData::NiIntegersExtraData() : m_uiSize(0), m_piValue(nullptr) {}

NiIntegersExtraData::NiIntegersExtraData(const uint32_t uiSize, const int32_t* piValue) : m_piValue(nullptr) {
    SetArray(uiSize, piValue);
}

NiIntegersExtraData::~NiIntegersExtraData() {
    _NiFree(m_piValue);
    m_piValue = nullptr;
}

const NiRTTI* NiIntegersExtraData::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiIntegersExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiIntegersExtraData* pkClone =
        static_cast<NiIntegersExtraData*>(NiMemObject::operator new(sizeof(NiIntegersExtraData)));
    if (pkClone) {
        ::new (pkClone) NiIntegersExtraData;
        CopyMembers(pkClone, kCloning);
        return pkClone;
    }

    CopyMembers(nullptr, kCloning);
    return nullptr;
}

// The old array is not freed.
void NiIntegersExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uiSize);
    if (m_uiSize > 0) {
        m_piValue = static_cast<int32_t*>(_NiMalloc(m_uiSize * sizeof(int32_t)));
        NiStreamLoadBinary(kStream, m_piValue, m_uiSize);
    }
    else {
        m_piValue = nullptr;
    }
}

void NiIntegersExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiIntegersExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

void NiIntegersExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiSize);
    NiStreamSaveBinary(kStream, m_piValue, m_uiSize);
}

// Only the arrays are compared: neither the class nor the name is checked.
bool NiIntegersExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    NiIntegersExtraData* pkOther = static_cast<NiIntegersExtraData*>(pObject);
    if (m_uiSize != pkOther->m_uiSize) {
        return false;
    }

    if ((m_piValue && !pkOther->m_piValue) || (!m_piValue && pkOther->m_piValue)) {
        return false;
    }

    if (m_piValue) {
        for (uint32_t i = 0; i < m_uiSize; i++) {
            if (m_piValue[i] != pkOther->m_piValue[i]) {
                return false;
            }
        }
    }

    return true;
}

void NiIntegersExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiSize", m_uiSize));
    for (uint32_t i = 0; i < m_uiSize; i++) {
        pStrings->Add(NiGetViewerString("m_piValue[i]", m_piValue[i]));
    }
}

void NiIntegersExtraData::SetArray(uint32_t uiSize, const int32_t* piValue) {
    _NiFree(m_piValue);
    if (!piValue || uiSize == 0) {
        m_uiSize = 0;
        m_piValue = nullptr;
        return;
    }

    m_uiSize = uiSize;
    m_piValue = static_cast<int32_t*>(_NiMalloc(uiSize * sizeof(int32_t)));
    for (uint32_t i = 0; i < uiSize; i++) {
        m_piValue[i] = piValue[i];
    }
}

// Unlike SetArray, apDest's old array is not freed.
void NiIntegersExtraData::CopyMembers(NiIntegersExtraData* pDest, NiCloningProcess& kCloning) {
    NiExtraData::CopyMembers(pDest, kCloning);
    if (!m_piValue || m_uiSize == 0) {
        pDest->m_piValue = nullptr;
        pDest->m_uiSize = 0;
        return;
    }

    pDest->m_uiSize = m_uiSize;
    pDest->m_piValue = static_cast<int32_t*>(_NiMalloc(m_uiSize * sizeof(int32_t)));
    for (uint32_t i = 0; i < m_uiSize; i++) {
        pDest->m_piValue[i] = m_piValue[i];
    }
}

NiObject* NiIntegersExtraData::CreateObject() {
    return new NiIntegersExtraData;
}
