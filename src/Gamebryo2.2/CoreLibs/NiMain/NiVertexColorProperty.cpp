#include "Gamebryo2.2/CoreLibs/NiMain/NiVertexColorProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstring>

NiRTTI NiVertexColorProperty::ms_RTTI("NiVertexColorProperty", &NiProperty::ms_RTTI);
NiPointer<NiVertexColorProperty> NiVertexColorProperty::ms_spDefault;

// Without controllers the clone is this object, through the base's CopyMembers only. Otherwise a fresh property copies
// the members through NiProperty::CopyMembers (which leaves the flags alone) and then takes this object's flags.
NiObject* NiVertexColorProperty::CreateClone(NiCloningProcess& kCloning) {
    if (m_spControllers) {
        NiVertexColorProperty* pkClone =
            static_cast<NiVertexColorProperty*>(NiMemObject::operator new(sizeof(NiVertexColorProperty)));
        if (pkClone) {
            ::new (pkClone) NiVertexColorProperty;
        }
        NiProperty::CopyMembers(pkClone, kCloning);
        pkClone->m_uFlags = m_uFlags;
        return pkClone;
    }
    NiObject::CopyMembers(this, kCloning);
    return this;
}

// KEEP (niread): the two mode values are read into one stack local that a short read leaves unwritten, and each is
// then shifted into the flags.
void NiVertexColorProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion < 0x0A000102) {
        m_uFlags = kStream.m_usNiPropertyFlags & 7;
    }
    else {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
    if (kStream.m_uiNifFileVersion < 0x14010002) {
        uint32_t uiValue;
        NiStreamLoadBinary(kStream, uiValue);
        m_uFlags = static_cast<uint16_t>((m_uFlags & 0xFFCF) | (uiValue << SOURCEMODE_POS));
        NiStreamLoadBinary(kStream, uiValue);
        m_uFlags = static_cast<uint16_t>((m_uFlags & 0xFFF7) | (uiValue << LIGHTINGMODE_POS));
    }
}

void NiVertexColorProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);
}

bool NiVertexColorProperty::RegisterStreamables(NiStream& kStream) {
    return NiProperty::RegisterStreamables(kStream);
}

void NiVertexColorProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
}

// The source mode is compared first.
bool NiVertexColorProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }
    NiVertexColorProperty* pkVC = static_cast<NiVertexColorProperty*>(pkObject);
    if ((m_uFlags & SOURCEMODE_MASK) != (pkVC->m_uFlags & SOURCEMODE_MASK)) {
        return false;
    }
    if ((m_uFlags & LIGHTINGMODE_MASK) != (pkVC->m_uFlags & LIGHTINGMODE_MASK)) {
        return false;
    }
    return true;
}

void NiVertexColorProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiProperty::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(GetViewerString("Source Mode", static_cast<SourceVertexMode>((m_uFlags >> 4) & 3)));
    pkStrings->Add(GetViewerString("Lighting Mode", static_cast<LightingMode>((m_uFlags >> 3) & 1)));
}

bool NiVertexColorProperty::IsEqualFast(const NiVertexColorProperty& kProp) const {
    return (bool)((uint8_t)(m_uFlags == (&kProp)->m_uFlags));
}

// The default flags are 8: lighting mode LIGHTING_E_A_D, source mode SOURCE_IGNORE.
NiObject* NiVertexColorProperty::CreateObject() {
    NiVertexColorProperty* pkObject =
        static_cast<NiVertexColorProperty*>(NiMemObject::operator new(sizeof(NiVertexColorProperty)));
    if (pkObject) {
        ::new (pkObject) NiVertexColorProperty;
    }
    return pkObject;
}

char* NiVertexColorProperty::GetViewerString(const char* apcPrefix, SourceVertexMode aeMode) {
    size_t stLength = strlen(apcPrefix) + 26;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeMode) {
        case SOURCE_IGNORE: NiSprintf(pcString, stLength, "%s = SOURCE_IGNORE", apcPrefix); break;
        case SOURCE_EMISSIVE: NiSprintf(pcString, stLength, "%s = SOURCE_EMISSIVE", apcPrefix); break;
        case SOURCE_AMB_DIFF: NiSprintf(pcString, stLength, "%s = SOURCE_AMB_DIFF", apcPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

char* NiVertexColorProperty::GetViewerString(const char* apcPrefix, LightingMode aeMode) {
    size_t stLength = strlen(apcPrefix) + 26;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeMode) {
        case LIGHTING_E: NiSprintf(pcString, stLength, "%s = LIGHTING_E", apcPrefix); break;
        case LIGHTING_E_A_D: NiSprintf(pcString, stLength, "%s = LIGHTING_E_A_D", apcPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}
