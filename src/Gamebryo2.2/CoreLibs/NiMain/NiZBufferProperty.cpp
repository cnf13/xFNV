#include "Gamebryo2.2/CoreLibs/NiMain/NiZBufferProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiZBufferProperty::ms_RTTI("NiZBufferProperty", &NiProperty::ms_RTTI);
NiPointer<NiZBufferProperty> NiZBufferProperty::ms_spDefault;

// Without controllers the clone is this object, through the base's CopyMembers only. Otherwise a fresh property copies
// the members through NiProperty::CopyMembers (which leaves the flags alone) and then takes this object's flags.
NiObject* NiZBufferProperty::CreateClone(NiCloningProcess& kCloning) {
    if (m_spControllers) {
        NiZBufferProperty* pkClone = new NiZBufferProperty;
        NiProperty::CopyMembers(pkClone, kCloning);
        pkClone->m_uFlags = m_uFlags;
        return pkClone;
    }
    NiObject::CopyMembers(this, kCloning);
    return this;
}

void NiZBufferProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion < 0x0A000102) {
        m_uFlags = kStream.m_usNiPropertyFlags & ((1 << MAX_POS) - 1);
    }
    else {
        NiStreamLoadBinary(kStream, m_uFlags);
    }

    if (kStream.m_uiNifFileVersion >= 0x04010005 && kStream.m_uiNifFileVersion < 0x14010002) {
        uint32_t uiTestFunction;
        NiStreamLoadBinary(kStream, uiTestFunction);
        m_uFlags = static_cast<uint16_t>((m_uFlags & ~TEST_FUNCTION) | (uiTestFunction << TEST_FUNCTION_POS));
    }
}

void NiZBufferProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);
}

bool NiZBufferProperty::RegisterStreamables(NiStream& kStream) {
    return NiProperty::RegisterStreamables(kStream);
}

void NiZBufferProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
}

bool NiZBufferProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }

    NiZBufferProperty* pkZ = static_cast<NiZBufferProperty*>(pkObject);
    if ((m_uFlags & ZBUFFER_TEST) != (pkZ->m_uFlags & ZBUFFER_TEST)) {
        return false;
    }
    if ((m_uFlags & ZBUFFER_WRITE) != (pkZ->m_uFlags & ZBUFFER_WRITE)) {
        return false;
    }
    if ((m_uFlags & TEST_FUNCTION) != (pkZ->m_uFlags & TEST_FUNCTION)) {
        return false;
    }
    return true;
}

// KEEP: the exe indexes its 8-entry name table with the 4-bit test-function field, so flags 8 to 15 read past the
// table into the frame (the return address, the argument slot holding the last string, the caller's frame), which
// src cannot reproduce.
void NiZBufferProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiProperty::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_bZTest", (m_uFlags & ZBUFFER_TEST) != 0));
    pkStrings->Add(NiGetViewerString("m_bZWrite", (m_uFlags & ZBUFFER_WRITE) != 0));
    const char* apcTestFunctions[TEST_MAX_MODES] = {
        "TEST_ALWAYS",
        "TEST_LESS",
        "TEST_EQUAL",
        "TEST_LESSEQUAL",
        "TEST_GREATER",
        "TEST_NOTEQUAL",
        "TEST_GREATEREQUAL",
        "TEST_NEVER"
    };
    pkStrings->Add(
        NiGetViewerString("Test Function", apcTestFunctions[(m_uFlags & TEST_FUNCTION) >> TEST_FUNCTION_POS])
    );
}

bool NiZBufferProperty::IsEqualFast(const NiZBufferProperty& kProp) const {
    return m_uFlags == kProp.m_uFlags;
}
