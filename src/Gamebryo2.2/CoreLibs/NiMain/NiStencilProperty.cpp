#include "Gamebryo2.2/CoreLibs/NiMain/NiStencilProperty.h"

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

#include <cstring>

NiRTTI NiStencilProperty::ms_RTTI("NiStencilProperty", &NiProperty::ms_RTTI);
NiPointer<NiStencilProperty> NiStencilProperty::ms_spDefault;

NiStencilProperty::NiStencilProperty() : m_uFlags(0), m_uiRef(0), m_uiMask(0xFFFFFFFFu) {
    SetStencilOn(false);
    SetStencilFunction(TEST_GREATER);
    SetStencilFailAction(ACTION_KEEP);
    SetStencilPassZFailAction(ACTION_KEEP);
    SetStencilPassAction(ACTION_INCREMENT);
    SetDrawMode(DRAW_CCW_OR_BOTH);
}

// A property with controllers is cloned into a fresh default stencil property; without controllers the property is
// shared and only NiObject's members are copied onto itself.
NiObject* NiStencilProperty::CreateClone(NiCloningProcess& kCloning) {
    if (m_spControllers) {
        NiStencilProperty* pkClone = new NiStencilProperty;
        NiProperty::CopyMembers(pkClone, kCloning);
        pkClone->m_uFlags = m_uFlags;
        pkClone->m_uiRef = m_uiRef;
        pkClone->m_uiMask = m_uiMask;
        return pkClone;
    }
    NiObject::CopyMembers(this, kCloning);
    return this;
}

void NiStencilProperty::LoadBinary(NiStream& kStream) {
    NiProperty::LoadBinary(kStream);

    if (kStream.GetFileVersion() >= 0x14010002) {
        NiStreamLoadBinary(kStream, m_uFlags);
        NiStreamLoadBinary(kStream, m_uiRef);
        NiStreamLoadBinary(kStream, m_uiMask);
        return;
    }

    // Older files: one value per field. The values are shifted into place without being masked.
    uint8_t ucEnable;
    NiStreamLoadBinary(kStream, ucEnable);
    if (ucEnable != 0) {
        m_uFlags |= ENABLE_MASK;
    }
    else {
        m_uFlags &= 0xFFFE;
    }

    uint32_t uiValue;
    NiStreamLoadBinary(kStream, uiValue);
    m_uFlags = static_cast<uint16_t>((m_uFlags & 0x0FFF) | (uiValue << TESTFUNC_POS));
    NiStreamLoadBinary(kStream, m_uiRef);
    NiStreamLoadBinary(kStream, m_uiMask);
    NiStreamLoadBinary(kStream, uiValue);
    m_uFlags = static_cast<uint16_t>((m_uFlags & 0xFFF1) | (uiValue << FAILACTION_POS));
    NiStreamLoadBinary(kStream, uiValue);
    m_uFlags = static_cast<uint16_t>((m_uFlags & 0xFF8F) | (uiValue << ZFAILACTION_POS));
    NiStreamLoadBinary(kStream, uiValue);
    m_uFlags = static_cast<uint16_t>((m_uFlags & 0xFC7F) | (uiValue << PASSACTION_POS));
    NiStreamLoadBinary(kStream, uiValue);
    m_uFlags = static_cast<uint16_t>((m_uFlags & 0xF3FF) | (uiValue << DRAWMODE_POS));
}

// KEEP: niread: before 20.1.0.2 a short read leaves the enable byte (read into the stream argument's slot, whose low
// byte the exe then tests) or the first value (an unwritten local) as the exe left them, and the exe shifts them into
// m_uFlags.
void NiStencilProperty::LinkObject(NiStream& kStream) {
    NiProperty::LinkObject(kStream);
}

bool NiStencilProperty::RegisterStreamables(NiStream& kStream) {
    return NiProperty::RegisterStreamables(kStream);
}

void NiStencilProperty::SaveBinary(NiStream& kStream) {
    NiProperty::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
    NiStreamSaveBinary(kStream, m_uiRef);
    NiStreamSaveBinary(kStream, m_uiMask);
}

bool NiStencilProperty::IsEqual(NiObject* pkObject) {
    if (!NiProperty::IsEqual(pkObject)) {
        return false;
    }

    NiStencilProperty* pkStencil = static_cast<NiStencilProperty*>(pkObject);
    if (m_uFlags != pkStencil->m_uFlags) {
        return false;
    }
    if (m_uiRef != pkStencil->m_uiRef) {
        return false;
    }
    if (m_uiMask != pkStencil->m_uiMask) {
        return false;
    }
    return true;
}

void NiStencilProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pcStrings) {
    NiProperty::GetViewerStrings(pcStrings);

    pcStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pcStrings->Add(NiGetViewerString("Enabled", (m_uFlags & ENABLE_MASK) != 0));
    pcStrings->Add(NiGetViewerString("m_uiRef", m_uiRef));
    pcStrings->Add(NiGetViewerString("m_uiMask", m_uiMask));
    // The misspelling "Funtion" is deliberate, as the game does.
    pcStrings->Add(GetViewerString("Test Funtion", static_cast<TestFunc>(m_uFlags >> 12)));
    pcStrings->Add(GetViewerString("Test Fail Action", static_cast<Action>((m_uFlags >> 1) & 7)));
    pcStrings->Add(GetViewerString("Depth Fail Action", static_cast<Action>((m_uFlags >> 4) & 7)));
    pcStrings->Add(GetViewerString("Test Pass Action", static_cast<Action>((m_uFlags >> 7) & 7)));
    pcStrings->Add(GetViewerString("Draw Mode", static_cast<DrawMode>((m_uFlags >> 10) & 3)));
}

void NiStencilProperty::SetStencilOn(bool bOn) {
    SetBit(bOn, ENABLE_MASK);
}

void NiStencilProperty::SetBit(bool bVal, uint16_t uMask) {
    if (bVal) {
        m_uFlags = uint16_t(m_uFlags | uMask);
    }
    else {
        m_uFlags = uint16_t(m_uFlags & ~uint32_t(uMask));
    }
}

void NiStencilProperty::SetField(uint16_t uVal, uint16_t uMask, uint16_t uPos) {
    m_uFlags = uint16_t((m_uFlags & ~uint32_t(uMask)) | (uint32_t(uVal) << uPos));
}

void NiStencilProperty::SetStencilFunction(TestFunc eFunc) {
    SetField(static_cast<uint16_t>(eFunc), TESTFUNC_MASK, TESTFUNC_POS);
}

void NiStencilProperty::SetStencilFailAction(Action eAct) {
    SetField(static_cast<uint16_t>(eAct), FAILACTION_MASK, FAILACTION_POS);
}

void NiStencilProperty::SetStencilPassZFailAction(Action eAct) {
    SetField(static_cast<uint16_t>(eAct), ZFAILACTION_MASK, ZFAILACTION_POS);
}

void NiStencilProperty::SetStencilPassAction(Action eAct) {
    SetField(static_cast<uint16_t>(eAct), PASSACTION_MASK, PASSACTION_POS);
}

void NiStencilProperty::SetDrawMode(DrawMode eDraw) {
    SetField(static_cast<uint16_t>(eDraw), DRAWMODE_MASK, DRAWMODE_POS);
}

void NiStencilProperty::SetStencilReference(uint32_t uiRef) {
    m_uiRef = uiRef;
}

char* NiStencilProperty::GetViewerString(const char* apcPrefix, TestFunc aeFunc) {
    size_t stLength = strlen(apcPrefix) + 22;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeFunc) {
        case TEST_NEVER: NiSprintf(pcString, stLength, "%s = TEST_NEVER", apcPrefix); break;
        case TEST_LESS: NiSprintf(pcString, stLength, "%s = TEST_LESS", apcPrefix); break;
        case TEST_EQUAL: NiSprintf(pcString, stLength, "%s = TEST_EQUAL", apcPrefix); break;
        case TEST_LESSEQUAL: NiSprintf(pcString, stLength, "%s = TEST_LESSEQUAL", apcPrefix); break;
        case TEST_GREATER: NiSprintf(pcString, stLength, "%s = TEST_GREATER", apcPrefix); break;
        case TEST_NOTEQUAL: NiSprintf(pcString, stLength, "%s = TEST_NOTEQUAL", apcPrefix); break;
        case TEST_GREATEREQUAL: NiSprintf(pcString, stLength, "%s = TEST_GREATEREQUAL", apcPrefix); break;
        case TEST_ALWAYS: NiSprintf(pcString, stLength, "%s = TEST_ALWAYS", apcPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

char* NiStencilProperty::GetViewerString(const char* apcPrefix, Action aeAction) {
    size_t stLength = strlen(apcPrefix) + 22;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeAction) {
        case ACTION_KEEP: NiSprintf(pcString, stLength, "%s = ACTION_KEEP", apcPrefix); break;
        case ACTION_ZERO: NiSprintf(pcString, stLength, "%s = ACTION_ZERO", apcPrefix); break;
        case ACTION_REPLACE: NiSprintf(pcString, stLength, "%s = ACTION_REPLACE", apcPrefix); break;
        case ACTION_INCREMENT: NiSprintf(pcString, stLength, "%s = ACTION_INCREMENT", apcPrefix); break;
        case ACTION_DECREMENT: NiSprintf(pcString, stLength, "%s = ACTION_DECREMENT", apcPrefix); break;
        case ACTION_INVERT: NiSprintf(pcString, stLength, "%s = ACTION_INVERT", apcPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

char* NiStencilProperty::GetViewerString(const char* apcPrefix, DrawMode aeMode) {
    size_t stLength = strlen(apcPrefix) + 22;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    switch (aeMode) {
        case DRAW_CCW_OR_BOTH: NiSprintf(pcString, stLength, "%s = DRAW_CCW_OR_BOTH", apcPrefix); break;
        case DRAW_CCW: NiSprintf(pcString, stLength, "%s = DRAW_CCW", apcPrefix); break;
        case DRAW_CW: NiSprintf(pcString, stLength, "%s = DRAW_CW", apcPrefix); break;
        case DRAW_BOTH: NiSprintf(pcString, stLength, "%s = DRAW_BOTH", apcPrefix); break;
        default: NiSprintf(pcString, stLength, "%s = UNKNOWN!!!", apcPrefix); break;
    }
    return pcString;
}

// The engine's default stencil property. The previous default is released and the new one takes a reference.
void NiStencilProperty::_SDMInit() {
    NiStencilProperty* pkDefault = new NiStencilProperty;

    ms_spDefault = pkDefault;
}

// Drops the engine's default stencil property, releasing the one reference ms_spDefault held; the release deletes
// the object before the static is cleared.
void NiStencilProperty::_SDMShutdown() {
    ms_spDefault = nullptr;
}

NiObject* NiStencilProperty::CreateObject() {
    return new NiStencilProperty;
}
