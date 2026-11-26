#include "Gamebryo2.2/CoreLibs/NiMain/NiBooleanExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBooleanExtraData::ms_RTTI("NiBooleanExtraData", &NiExtraData::ms_RTTI);

NiBooleanExtraData::NiBooleanExtraData() : m_bValue(false) {}

NiBooleanExtraData::~NiBooleanExtraData() {}

const NiRTTI* NiBooleanExtraData::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBooleanExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiBooleanExtraData* pkObject_2 = new NiBooleanExtraData;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_bValue = m_bValue;
    return pkObject_2;
}

// The value is stored as one byte; any non-zero byte reads as true.
void NiBooleanExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);
    uint8_t ucValue;
    NiStreamLoadBinary(kStream, ucValue);
    m_bValue = ucValue != 0;
}

void NiBooleanExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiBooleanExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

// The value is written from a one-byte copy.
void NiBooleanExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);
    uint8_t ucValue = m_bValue;
    NiStreamSaveBinary(kStream, ucValue);
}

// Only the values are compared: neither the class nor the name is checked.
bool NiBooleanExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    return m_bValue == static_cast<NiBooleanExtraData*>(pObject)->m_bValue;
}

void NiBooleanExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_bValue", m_bValue));
}

NiObject* NiBooleanExtraData::CreateObject() {
    return new NiBooleanExtraData;
}
