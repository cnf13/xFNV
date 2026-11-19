#include "Gamebryo2.2/CoreLibs/NiMain/NiSpecularProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiSpecularProperty::ms_RTTI("NiSpecularProperty", &NiProperty::ms_RTTI);
NiPointer<NiSpecularProperty> NiSpecularProperty::ms_spDefault;

NiSpecularProperty::NiSpecularProperty() : m_uFlags(0) {}

NiSpecularProperty::~NiSpecularProperty() {}

NiObject* NiSpecularProperty::CreateClone(NiCloningProcess& kCloning) {
    NiSpecularProperty* pkObject_2 = new NiSpecularProperty;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_uFlags = m_uFlags;
    return pkObject_2;
}

void NiSpecularProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion < 0x0A000102) {
        m_uFlags = kStream.m_usNiPropertyFlags & 1;
    }
    else {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
}

void NiSpecularProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);
}

bool NiSpecularProperty::RegisterStreamables(NiStream& kStream) {
    return NiProperty::RegisterStreamables(kStream);
}

void NiSpecularProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
}

bool NiSpecularProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }

    return (m_uFlags & 1) == (static_cast<NiSpecularProperty*>(pkObject)->m_uFlags & 1);
}

void NiSpecularProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiProperty::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_bSpec", (m_uFlags & 1) != 0));
}

NiObject* NiSpecularProperty::CreateObject() {
    return new NiSpecularProperty;
}
