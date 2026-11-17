#include "Gamebryo2.2/CoreLibs/NiMain/NiFloatExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiFloatExtraData::ms_RTTI("NiFloatExtraData", &NiExtraData::ms_RTTI);

NiFloatExtraData::NiFloatExtraData() : m_fValue(0.0f) {}

NiFloatExtraData::~NiFloatExtraData() {}

const NiRTTI* NiFloatExtraData::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiFloatExtraData::CreateClone(NiCloningProcess& kCloning) {
    NiFloatExtraData* pkObject = static_cast<NiFloatExtraData*>(NiMemObject::operator new(sizeof(NiFloatExtraData)));
    NiFloatExtraData* pkObject_2 = pkObject;
    if (pkObject) {
        pkObject_2 = ::new (pkObject) NiFloatExtraData;
    }
    else {
        pkObject_2 = 0;
    }
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_fValue = m_fValue;
    return pkObject_2;
}

void NiFloatExtraData::LoadBinary(NiStream& kStream) {
    NiExtraData::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fValue);
}

void NiFloatExtraData::LinkObject(NiStream& kStream) {
    NiExtraData::LinkObject(kStream);
}

bool NiFloatExtraData::RegisterStreamables(NiStream& kStream) {
    return NiExtraData::RegisterStreamables(kStream);
}

void NiFloatExtraData::SaveBinary(NiStream& kStream) {
    NiExtraData::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fValue);
}

// Only the values are compared: neither the class nor the name is checked. Unordered (NaN) is unequal.
bool NiFloatExtraData::IsEqual(NiObject* pObject) {
    if (!pObject) {
        return false;
    }

    return m_fValue == static_cast<NiFloatExtraData*>(pObject)->m_fValue;
}

void NiFloatExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
    NiExtraData::GetViewerStrings(pStrings);
    pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pStrings->Add(NiGetViewerString("m_fValue", m_fValue));
}

NiObject* NiFloatExtraData::CreateObject() {
    NiFloatExtraData* pkObject = static_cast<NiFloatExtraData*>(NiMemObject::operator new(sizeof(NiFloatExtraData)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiFloatExtraData;
}
