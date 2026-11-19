#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiCullingProperty::ms_RTTI("NiCullingProperty", &NiProperty::ms_RTTI);
NiPointer<NiCullingProperty> NiCullingProperty::ms_spDefault;

// Clears only bit 0 of the flags.
NiCullingProperty::NiCullingProperty() {
    m_uFlags &= ~1u;
}

NiCullingProperty::~NiCullingProperty() {}

NiObject* NiCullingProperty::CreateClone(NiCloningProcess& kCloning) {
    NiCullingProperty* pkObject_2 = new NiCullingProperty;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_uFlags = m_uFlags;
    return pkObject_2;
}

// No version check: the flags are read straight into the member.
void NiCullingProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_uFlags);
}

void NiCullingProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);
}

bool NiCullingProperty::RegisterStreamables(NiStream& kStream) {
    return NiProperty::RegisterStreamables(kStream);
}

// Shared with NiVertexColorProperty, NiZBufferProperty, NiShadeProperty, NiSpecularProperty and NiDitherProperty.
void NiCullingProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
}

// Shared with NiShadeProperty, NiSpecularProperty and NiDitherProperty (flags at +0x18 in all).
bool NiCullingProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }

    return (m_uFlags & 1) == (static_cast<NiCullingProperty*>(pkObject)->m_uFlags & 1);
}

void NiCullingProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiProperty::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("bCulled", (m_uFlags & 1) != 0));
}

void NiCullingProperty::_SDMInit() {
    NiCullingProperty* v1 = new NiCullingProperty;
    NiCullingProperty::ms_spDefault = v1;
}

// Drops the engine's default culling property, releasing the one reference ms_spDefault held; the release deletes
// the object before the static is cleared.
void NiCullingProperty::_SDMShutdown() {
    ms_spDefault = nullptr;
}

NiObject* NiCullingProperty::CreateObject() {
    return new NiCullingProperty;
}
