#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMaterialColorController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3InterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPosData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

NiRTTI NiMaterialColorController::ms_RTTI("NiMaterialColorController", &NiPoint3InterpController::ms_RTTI);

NiMaterialColorController::NiMaterialColorController() : NiPoint3InterpController(), m_uFlags(0) {}

NiMaterialColorController::~NiMaterialColorController() {}

const NiRTTI* NiMaterialColorController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiMaterialColorController::CreateClone(NiCloningProcess& kCloning) {
    NiMaterialColorController* pkObject =
        static_cast<NiMaterialColorController*>(NiMemObject::operator new(sizeof(NiMaterialColorController)));
    NiMaterialColorController* pkObject_2 = pkObject;
    if (pkObject) {
        ::new (pkObject_2) NiMaterialColorController;
    }
    else {
        pkObject_2 = nullptr;
    }
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_uFlags = m_uFlags;
    return pkObject_2;
}

// Files before 10.0.1.2 keep the colour choice in the stream's time controller flags; files older than
// 10.1.0.104 also carry a link to an old data block, which is skipped.
void NiMaterialColorController::LoadBinary(NiStream& kStream) {
    NiPoint3InterpController::LoadBinary(kStream);
    if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 0, 1, 2)) {
        NiStreamLoadBinary(kStream, m_uFlags);
    }
    else {
        m_uFlags = static_cast<uint16_t>((kStream.m_usNiTimeControllerFlags >> 5) & 0xFF);
    }

    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

void NiMaterialColorController::LinkObject(NiStream& kStream) {
    NiPoint3InterpController::LinkObject(kStream);
    if ((&kStream)->GetFileVersion() < 0xA010068) {
        NiPoint3Interpolator* v5;
        NiPosData* pkPosData = static_cast<NiPosData*>(kStream.GetObjectFromLinkID());
        NiPoint3Interpolator* v4 =
            static_cast<NiPoint3Interpolator*>(NiMemObject::operator new(sizeof(NiPoint3Interpolator)));
        if (v4) {
            v5 = ::new (v4) NiPoint3Interpolator(pkPosData);
        }
        else {
            v5 = nullptr;
        }
        m_spInterpolator = v5;
        m_spInterpolator.m_pObject->Collapse();
    }
}

bool NiMaterialColorController::RegisterStreamables(NiStream& kStream) {
    return NiPoint3InterpController::RegisterStreamables(kStream);
}

// NiLightColorController uses the same function.
void NiMaterialColorController::SaveBinary(NiStream& kStream) {
    NiPoint3InterpController::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_uFlags);
}

bool NiMaterialColorController::IsEqual(NiObject* pkObject) {
    if (!NiPoint3InterpController::IsEqual(pkObject)) {
        return false;
    }

    return ((m_uFlags ^ static_cast<NiMaterialColorController*>(pkObject)->m_uFlags) & FIELD_MASK) == 0;
}

void NiMaterialColorController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPoint3InterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// The colour is clamped to [0, 1] (NaN passes). Only the specular and self-illumination colours exist, so ambient
// and diffuse animation does nothing. The target material is not null-checked.
void NiMaterialColorController::Update(NiUpdateData& UpdateData) {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = -FLT_MAX;
    }
    else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate())) {
        return;
    }

    if (!m_spInterpolator) {
        return;
    }

    NiPoint3 kValue;
    if (!m_spInterpolator->Update(m_fScaledTime, m_pkTarget, kValue)) {
        return;
    }

    NiColor kColor;
    kColor.r = kValue.x;
    kColor.g = kValue.y;
    kColor.b = kValue.z;
    if (kColor.r < 0.0f) {
        kColor.r = 0.0f;
    }
    else if (kColor.r > 1.0f) {
        kColor.r = 1.0f;
    }
    if (kColor.g < 0.0f) {
        kColor.g = 0.0f;
    }
    else if (kColor.g > 1.0f) {
        kColor.g = 1.0f;
    }
    if (kColor.b < 0.0f) {
        kColor.b = 0.0f;
    }
    else if (kColor.b > 1.0f) {
        kColor.b = 1.0f;
    }

    NiMaterialProperty* pkMaterial = static_cast<NiMaterialProperty*>(m_pkTarget);
    switch (m_uFlags & FIELD_MASK) {
        case SPEC:
            ++pkMaterial->m_uiRevID;
            pkMaterial->m_spec = kColor;
            break;
        case SELF_ILLUM:
            ++pkMaterial->m_uiRevID;
            pkMaterial->m_emit = kColor;
            break;
    }
}

const char* NiMaterialColorController::GetCtlrID() {
    switch (m_uFlags & FIELD_MASK) {
        case AMB: return "AMB";
        case DIFF: return "DIFF";
        case SPEC: return "SPEC";
        case SELF_ILLUM: return "SELF_ILLUM";
    }
    return nullptr;
}

// NiAlphaController and BSMaterialEmittanceMultController use the same function.
bool NiMaterialColorController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&NiMaterialProperty::ms_RTTI, pkTarget);
}

// Ambient and diffuse read as black; the self-illumination colour is the external one when set. Other colour
// numbers read as 0.
void NiMaterialColorController::GetTargetPoint3Value(NiPoint3& kValue) {
    NiColor kColor;
    kColor.r = 0.0f;
    kColor.g = 0.0f;
    kColor.b = 0.0f;
    NiMaterialProperty* pkMaterial = static_cast<NiMaterialProperty*>(m_pkTarget);
    switch (m_uFlags & FIELD_MASK) {
        case AMB:
        case DIFF: kColor = NiColor_BLACK; break;
        case SPEC: kColor = pkMaterial->m_spec; break;
        case SELF_ILLUM:
            kColor = pkMaterial->m_external_emit ? *pkMaterial->m_external_emit : pkMaterial->m_emit;
            break;
    }
    kValue.x = kColor.r;
    kValue.y = kColor.g;
    kValue.z = kColor.b;
}

NiObject* NiMaterialColorController::CreateObject() {
    NiMaterialColorController* pkObject =
        static_cast<NiMaterialColorController*>(NiMemObject::operator new(sizeof(NiMaterialColorController)));
    if (!pkObject) {
        return nullptr;
    }
    return ::new (pkObject) NiMaterialColorController;
}
