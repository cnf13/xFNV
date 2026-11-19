#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"

#include "Fallout Shared/SpeedTree/BSTreeModel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiRTTI NiAlphaProperty::ms_RTTI("NiAlphaProperty", &NiProperty::ms_RTTI);
NiPointer<NiAlphaProperty> NiAlphaProperty::ms_spDefault;

NiAlphaProperty::NiAlphaProperty() : m_uFlags(0) {
    SetAlphaBlending(false);
    SetSrcBlendMode(ALPHA_SRCALPHA);
    SetDestBlendMode(ALPHA_INVSRCALPHA);
    SetAlphaTesting(false);
    SetTestMode(TEST_ALWAYS);
    SetTestRef(0);
}

// A copy is made only when the property has controllers or the tree model's clone flag is set; otherwise the property
// is shared and only NiObject's members are copied onto itself.
NiObject* NiAlphaProperty::CreateClone(NiCloningProcess& kCloning) {
    if (m_spControllers || abCloneUnique) {
        NiAlphaProperty* pkClone = new NiAlphaProperty;
        NiProperty::CopyMembers(pkClone, kCloning);
        pkClone->m_uFlags = m_uFlags;
        pkClone->m_ucAlphaTestRef = m_ucAlphaTestRef;
        return pkClone;
    }
    NiObject::CopyMembers(this, kCloning);
    return this;
}

// Files older than 10.0.1.2 keep the flags of every property in the stream's shared word, which NiProperty's load
// filled in; the upper two bits are dropped.
void NiAlphaProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);
    if (kStream.m_uiNifFileVersion < 0x0A000102) {
        m_uFlags = uint16_t(kStream.m_usNiPropertyFlags & 0x3FFF);
    }
    else {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
    NiStreamLoadBinary(kStream, m_ucAlphaTestRef);
}

void NiAlphaProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);
}

bool NiAlphaProperty::RegisterStreamables(NiStream& kStream) {
    return NiProperty::RegisterStreamables(kStream);
}

void NiAlphaProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
    NiStreamSaveBinary(kStream, m_ucAlphaTestRef);
}

// Compares the flag fields one at a time, in the order of the fields.
bool NiAlphaProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }

    NiAlphaProperty* pkAlpha = static_cast<NiAlphaProperty*>(pkObject);
    if ((m_uFlags & ALPHA_BLEND_MASK) != (pkAlpha->m_uFlags & ALPHA_BLEND_MASK)) {
        return false;
    }
    if ((m_uFlags & SRC_BLEND_MASK) != (pkAlpha->m_uFlags & SRC_BLEND_MASK)) {
        return false;
    }
    if ((m_uFlags & DEST_BLEND_MASK) != (pkAlpha->m_uFlags & DEST_BLEND_MASK)) {
        return false;
    }
    if ((m_uFlags & TEST_ENABLE_MASK) != (pkAlpha->m_uFlags & TEST_ENABLE_MASK)) {
        return false;
    }
    if ((m_uFlags & TEST_FUNC_MASK) != (pkAlpha->m_uFlags & TEST_FUNC_MASK)) {
        return false;
    }
    if (m_ucAlphaTestRef != pkAlpha->m_ucAlphaTestRef) {
        return false;
    }
    if ((m_uFlags & ALPHA_NOSORTER_MASK) != (pkAlpha->m_uFlags & ALPHA_NOSORTER_MASK)) {
        return false;
    }
    return true;
}

void NiAlphaProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiProperty::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_bAlpha", (m_uFlags & ALPHA_BLEND_MASK) != 0));
    pkStrings->Add(GetViewerString("m_eSrcBlend", static_cast<AlphaFunction>((m_uFlags >> SRC_BLEND_POS) & 0xF)));
    pkStrings->Add(GetViewerString("m_eDestBlend", static_cast<AlphaFunction>((m_uFlags >> DEST_BLEND_POS) & 0xF)));
    pkStrings->Add(NiGetViewerString("m_bAlphaTest", ((m_uFlags >> 9) & 1) != 0));
    pkStrings->Add(GetViewerString("m_eTestMode", static_cast<TestFunction>((m_uFlags >> TEST_FUNC_POS) & 7)));
    pkStrings->Add(NiGetViewerString("m_ucTestRef", m_ucAlphaTestRef));
    pkStrings->Add(NiGetViewerString("m_bNoSorter", ((m_uFlags >> 13) & 1) != 0));
}

void NiAlphaProperty::SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos) {
    m_uFlags = uint16_t((m_uFlags & ~uint32_t(uMask)) | (uint32_t(uVal) << uPos));
}

void NiAlphaProperty::SetBit(bool bVal, uint16_t uMask) {
    if (bVal) {
        m_uFlags = uint16_t(m_uFlags | uMask);
    }
    else {
        m_uFlags = uint16_t(m_uFlags & ~uint32_t(uMask));
    }
}

void NiAlphaProperty::SetAlphaBlending(bool bAlpha) {
    SetBit(bAlpha, ALPHA_BLEND_MASK);
}

void NiAlphaProperty::SetSrcBlendMode(AlphaFunction eSrcBlend) {
    SetField(uint16_t(eSrcBlend), SRC_BLEND_MASK, SRC_BLEND_POS);
}

void NiAlphaProperty::SetDestBlendMode(AlphaFunction eDestBlend) {
    SetField(uint16_t(eDestBlend), DEST_BLEND_MASK, DEST_BLEND_POS);
}

void NiAlphaProperty::SetAlphaTesting(bool bAlpha) {
    SetBit(bAlpha, TEST_ENABLE_MASK);
}

void NiAlphaProperty::SetTestMode(TestFunction eTestFunc) {
    SetField(uint16_t(eTestFunc), TEST_FUNC_MASK, TEST_FUNC_POS);
}

void NiAlphaProperty::SetTestRef(uint8_t ucRef) {
    m_ucAlphaTestRef = ucRef;
}

void NiAlphaProperty::SetNoSorter(bool bNoSort) {
    SetBit(bNoSort, ALPHA_NOSORTER_MASK);
}

bool NiAlphaProperty::GetFlag(uint32_t auiMask) const {
    return (m_uFlags & auiMask) != 0;
}

bool NiAlphaProperty::IsEnabled() {
    return (bool)((uint8_t)GetFlag(1u));
}

// The blend mode names are all shorter than the 22 bytes added to the prefix; an unknown mode gets "UNKNOWN!!!".
char* NiAlphaProperty::GetViewerString(const char* apcPrefix, AlphaFunction aeMode) {
    size_t stLength = strlen(apcPrefix) + 22;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeMode) {
        case ALPHA_ONE: NiSprintf(pcString, stLength, "%s = ALPHA_ONE", apcPrefix); break;
        case ALPHA_ZERO: NiSprintf(pcString, stLength, "%s = ALPHA_ZERO", apcPrefix); break;
        case ALPHA_SRCCOLOR: NiSprintf(pcString, stLength, "%s = ALPHA_SRCCOLOR", apcPrefix); break;
        case ALPHA_INVSRCCOLOR: NiSprintf(pcString, stLength, "%s = ALPHA_INVSRCCOLOR", apcPrefix); break;
        case ALPHA_DESTCOLOR: NiSprintf(pcString, stLength, "%s = ALPHA_DESTCOLOR", apcPrefix); break;
        case ALPHA_INVDESTCOLOR: NiSprintf(pcString, stLength, "%s = ALPHA_INVDESTCOLOR", apcPrefix); break;
        case ALPHA_SRCALPHA: NiSprintf(pcString, stLength, "%s = ALPHA_SRCALPHA", apcPrefix); break;
        case ALPHA_INVSRCALPHA: NiSprintf(pcString, stLength, "%s = ALPHA_INVSRCALPHA", apcPrefix); break;
        case ALPHA_DESTALPHA: NiSprintf(pcString, stLength, "%s = ALPHA_DESTALPHA", apcPrefix); break;
        case ALPHA_INVDESTALPHA: NiSprintf(pcString, stLength, "%s = ALPHA_INVDESTALPHA", apcPrefix); break;
        case ALPHA_SRCALPHASAT: NiSprintf(pcString, stLength, "%s = ALPHA_SRCALPHASAT", apcPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

char* NiAlphaProperty::GetViewerString(const char* apcPrefix, TestFunction aeMode) {
    size_t stLength = strlen(apcPrefix) + 22;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeMode) {
        case TEST_ALWAYS: NiSprintf(pcString, stLength, "%s = TEST_ALWAYS", apcPrefix); break;
        case TEST_LESS: NiSprintf(pcString, stLength, "%s = TEST_LESS", apcPrefix); break;
        case TEST_EQUAL: NiSprintf(pcString, stLength, "%s = TEST_EQUAL", apcPrefix); break;
        case TEST_LESSEQUAL: NiSprintf(pcString, stLength, "%s = TEST_LESSEQUAL", apcPrefix); break;
        case TEST_GREATER: NiSprintf(pcString, stLength, "%s = TEST_GREATER", apcPrefix); break;
        case TEST_NOTEQUAL: NiSprintf(pcString, stLength, "%s = TEST_NOTEQUAL", apcPrefix); break;
        case TEST_GREATEREQUAL: NiSprintf(pcString, stLength, "%s = TEST_GREATEREQUAL", apcPrefix); break;
        case TEST_NEVER: NiSprintf(pcString, stLength, "%s = TEST_NEVER", apcPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

void NiAlphaProperty::_SDMInit() {
    NiAlphaProperty* pkDefault = new NiAlphaProperty;
    // SWEEP-INLINE 0xA5CF1F
    ms_spDefault = pkDefault;
}

void NiAlphaProperty::_SDMShutdown() {
    // SWEEP-INLINE 0xA5CF60
    ms_spDefault = nullptr;
}

NiObject* NiAlphaProperty::CreateObject() {
    return new NiAlphaProperty;
}
