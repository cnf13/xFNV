#include "Gamebryo2.2/CoreLibs/NiMain/NiBinaryExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

NiRTTI NiBinaryExtraData::ms_RTTI("NiBinaryExtraData", &NiExtraData::ms_RTTI);

NiBinaryExtraData::NiBinaryExtraData() : m_pcBinaryData(nullptr), m_uiSize(0) {}

NiBinaryExtraData::NiBinaryExtraData(const uint32_t uiSize, char* pcBinaryData) {
    SetValue(uiSize, pcBinaryData);
}

NiBinaryExtraData::~NiBinaryExtraData() {
    _NiFree(m_pcBinaryData);
    m_pcBinaryData = nullptr;
}

const NiRTTI* NiBinaryExtraData::GetRTTI() const {
    return &ms_RTTI;
}

// The clone gets its own _NiMalloc copy of the blob.
NiObject* NiBinaryExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiBinaryExtraData* pkClone = static_cast<NiBinaryExtraData*>(NiMemObject::operator new(sizeof(NiBinaryExtraData)));
    if (pkClone) {
        ::new (pkClone) NiBinaryExtraData;
    }
    else {
        pkClone = nullptr;
    }
    NiExtraData::CopyMembers(pkClone, kCloning);
    if (m_uiSize > 0) {
        pkClone->m_pcBinaryData = static_cast<char*>(_NiMalloc(m_uiSize));
        rt::memcpy(pkClone->m_pcBinaryData, m_pcBinaryData, m_uiSize);
        pkClone->m_uiSize = m_uiSize;
    }
    else {
        pkClone->m_uiSize = 0;
        pkClone->m_pcBinaryData = nullptr;
    }
    return pkClone;
}

// An empty blob leaves m_pcBinaryData as it was.
void NiBinaryExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uiSize);
    if (m_uiSize > 0) {
        m_pcBinaryData = static_cast<char*>(_NiMalloc(m_uiSize));
        NiStreamLoadBinary(kStream, m_pcBinaryData, m_uiSize);
    }
}

void NiBinaryExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiBinaryExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

void NiBinaryExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uiSize);
    if (m_uiSize > 0) {
        NiStreamSaveBinary(kStream, m_pcBinaryData, m_uiSize);
    }
}

// Only the blobs are compared: neither the class nor the name is checked.
bool NiBinaryExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    NiBinaryExtraData* pkOther = static_cast<NiBinaryExtraData*>(pObject);
    if (m_uiSize != pkOther->m_uiSize) {
        return false;
    }

    return m_uiSize == 0 || std::memcmp(m_pcBinaryData, pkOther->m_pcBinaryData, m_uiSize) == 0;
}

void NiBinaryExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_uiSize", m_uiSize));
}

void NiBinaryExtraData::SetValue(uint32_t uiSize, char* pcBinaryData) {
    m_uiSize = uiSize;
    m_pcBinaryData = pcBinaryData;
}

NiObject* NiBinaryExtraData::CreateObject() {
    NiBinaryExtraData* pkObject = static_cast<NiBinaryExtraData*>(NiMemObject::operator new(sizeof(NiBinaryExtraData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiBinaryExtraData;
}
