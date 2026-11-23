#include "Gamebryo2.2/CoreLibs/NiMain/NiShadeProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiShadeProperty::ms_RTTI("NiShadeProperty", &NiProperty::ms_RTTI);
NiPointer<NiShadeProperty> NiShadeProperty::ms_spDefault;

NiShadeProperty::NiShadeProperty() : m_uFlags(SMOOTH_MASK), iShaderPropertyType(-1) {}

NiShadeProperty::~NiShadeProperty() {}

NiObject* NiShadeProperty::CreateClone(NiCloningProcess& kCloning) {
    NiShadeProperty* pkObject_2 = new NiShadeProperty;
    NiProperty::CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_uFlags = m_uFlags;
    return pkObject_2;
}

void NiShadeProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion < 0x0A000102) {
        m_uFlags = kStream.m_usNiPropertyFlags & ((1 << MAX_POS) - 1);
    }
    else {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
}

void NiShadeProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);
}

bool NiShadeProperty::RegisterStreamables(NiStream& kStream) {
    return NiProperty::RegisterStreamables(kStream);
}

void NiShadeProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
}

bool NiShadeProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }
    return (m_uFlags & SMOOTH_MASK) == (static_cast<NiShadeProperty*>(pkObject)->m_uFlags & SMOOTH_MASK);
}

void NiShadeProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiProperty::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_bSmooth", (m_uFlags & SMOOTH_MASK) != 0));
}

int32_t NiShadeProperty::Type() const {
    return SHADE;
}

int32_t NiShadeProperty::GetShaderPropertyType() const {
    return iShaderPropertyType;
}

void NiShadeProperty::CopyMembers(NiShadeProperty* pDest, NiCloningProcess& kCloning) {
    NiProperty::CopyMembers(pDest, kCloning);
    pDest->m_uFlags = m_uFlags;
}

int32_t NiShadeProperty::GetType() {
    return 3;
}

// The default property: shader property type -1 (none) and the smooth-shading flag.
void NiShadeProperty::_SDMInit() {
    NiShadeProperty* pkDefault = new NiShadeProperty;
    ms_spDefault = pkDefault;
}

// Releases ms_spDefault through the pointer's assignment.
void NiShadeProperty::_SDMShutdown() {
    ms_spDefault = nullptr;
}

NiObject* NiShadeProperty::CreateObject() {
    return new NiShadeProperty;
}
