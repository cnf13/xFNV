#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendAccumTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cmath>
#include <limits>

NiRTTI NiTransformController::ms_RTTI("NiTransformController", &NiSingleInterpController::ms_RTTI);

NiTransformController::NiTransformController() {}

NiTransformController::~NiTransformController() {}

const NiRTTI* NiTransformController::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiTransformController::CreateClone(NiCloningProcess& kCloning) {
    NiTransformController* v4 = new NiTransformController;
    NiSingleInterpController::CopyMembers(v4, kCloning);
    return v4;
}

// Files before 10.1.0.104 also store the link to an old-style keyframe data, which LinkObject resolves.
void NiTransformController::LoadBinary(NiStream& kStream) {
    NiSingleInterpController::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

void NiTransformController::LinkObject(NiStream& kStream) {
    NiSingleInterpController::LinkObject(kStream);
    if (kStream.GetFileVersion() < 0xA010068) {
        NiTransformInterpolator* v5;
        NiTransformData* pkKeyframeData = static_cast<NiTransformData*>(kStream.GetObjectFromLinkID());
        v5 = new NiTransformInterpolator(pkKeyframeData);
        m_spInterpolator = v5;
        m_spInterpolator->Collapse();
    }
}

bool NiTransformController::RegisterStreamables(NiStream& kStream) {
    return NiSingleInterpController::RegisterStreamables(kStream);
}

void NiTransformController::SaveBinary(NiStream& kStream) {
    NiSingleInterpController::SaveBinary(kStream);
}

bool NiTransformController::IsEqual(NiObject* pkObject) {
    return NiSingleInterpController::IsEqual(pkObject);
}

void NiTransformController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiSingleInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A manager-controlled controller always samples its interpolator, at the invalid time (-FLT_MAX). Only the valid
// parts of the sampled transform are written to the target's local transform; the scale is made positive.
void NiTransformController::Update(NiUpdateData& UpdateData) {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = -std::numeric_limits<float>::max();
    }
    else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate())) {
        return;
    }

    if (!m_spInterpolator) {
        return;
    }

    NiQuatTransform kTransform;
    kTransform.m_fScale = -std::numeric_limits<float>::max();
    kTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    kTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    if (!m_spInterpolator->Update(m_fScaledTime, m_pkTarget, kTransform)) {
        return;
    }

    NiAVObject* pkTarget = static_cast<NiAVObject*>(m_pkTarget);
    if (kTransform.IsTranslateValid()) {
        pkTarget->m_kLocal.m_Translate = kTransform.m_kTranslate;
    }
    if (kTransform.IsRotateValid()) {
        kTransform.m_kRotate.ToRotation(pkTarget->m_kLocal.m_Rotate);
    }
    if (kTransform.IsScaleValid()) {
        pkTarget->m_kLocal.m_fScale = std::fabs(kTransform.m_fScale);
    }
}

// The pose is the target's current local transform, built on the stack and passed by value.
NiInterpolator* NiTransformController::CreatePoseInterpolator(uint16_t) {
    NiAVObject* pkTarget = static_cast<NiAVObject*>(m_pkTarget);
    NiQuaternion kRotate;
    kRotate.FromRotation(pkTarget->m_kLocal.m_Rotate);
    NiQuatTransform kPose;
    kPose.m_fScale = pkTarget->m_kLocal.m_fScale;
    kPose.m_kTranslate = pkTarget->m_kLocal.m_Translate;
    kPose.m_kRotate = kRotate;
    return new NiTransformInterpolator(kPose);
}

// The pose is the target's current local transform. apInterp must be a NiTransformInterpolator.
void NiTransformController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
    // DynamicCast is only called for a non-null interpolator.
    NiTransformInterpolator* pkInterp = apInterp
        ? static_cast<NiTransformInterpolator*>(apInterp->DynamicCastMember(&NiTransformInterpolator::ms_RTTI))
        : nullptr;
    NiAVObject* pkTarget = static_cast<NiAVObject*>(m_pkTarget);

    NiQuaternion kRotate;
    kRotate.FromRotation(pkTarget->m_kLocal.m_Rotate);
    NiQuatTransform kPose;
    kPose.m_kTranslate = pkTarget->m_kLocal.m_Translate;
    kPose.m_kRotate = kRotate;
    kPose.m_fScale = pkTarget->m_kLocal.m_fScale;
    pkInterp->SetPoseValue(kPose);
}

NiBlendInterpolator* NiTransformController::CreateBlendInterpolator(
    uint16_t usIndex,
    int32_t bManagerControlled,
    bool bAccumulateAnimations,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    if (bAccumulateAnimations) {
        return new NiBlendAccumTransformInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
    }
    return new NiBlendTransformInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

bool NiTransformController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsTransformValueSupported();
}

bool NiTransformController::InterpTargetIsCorrectType(NiObjectNET* pkTarget) const {
    return NiObject::IsKindOf(&NiAVObject::ms_RTTI, pkTarget);
}

NiObject* NiTransformController::CreateObject() {
    return new NiTransformController;
}
