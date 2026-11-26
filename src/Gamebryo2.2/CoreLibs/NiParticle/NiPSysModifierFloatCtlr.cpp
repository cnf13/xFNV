#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierFloatCtlr.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifierCtlr.h"

#include <limits>

NiRTTI NiPSysModifierFloatCtlr::ms_RTTI("NiPSysModifierFloatCtlr", &NiPSysModifierCtlr::ms_RTTI);

NiPSysModifierFloatCtlr::NiPSysModifierFloatCtlr() {}

NiPSysModifierFloatCtlr::~NiPSysModifierFloatCtlr() {}

void NiPSysModifierFloatCtlr::LoadBinary(NiStream& kStream) {
    NiPSysModifierCtlr::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        kStream.ReadLinkID();
    }
}

// Files older than 10.1.0.104 carry a link to old-style float data; it becomes a float interpolator.
void NiPSysModifierFloatCtlr::LinkObject(NiStream& kStream) {
    NiPSysModifierCtlr::LinkObject(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 104)) {
        NiFloatData* pkData = static_cast<NiFloatData*>(kStream.GetObjectFromLinkID());
        NiFloatInterpolator* pkInterp = new NiFloatInterpolator(pkData);
        m_spInterpolator = pkInterp;
    }
}

bool NiPSysModifierFloatCtlr::RegisterStreamables(NiStream& kStream) {
    return NiPSysModifierCtlr::RegisterStreamables(kStream);
}

void NiPSysModifierFloatCtlr::SaveBinary(NiStream& kStream) {
    NiPSysModifierCtlr::SaveBinary(kStream);
}

bool NiPSysModifierFloatCtlr::IsEqual(NiObject* pkObject) {
    return NiPSysModifierCtlr::IsEqual(pkObject);
}

void NiPSysModifierFloatCtlr::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiPSysModifierCtlr::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiPSysModifierFloatCtlr::Update(NiUpdateData& UpdateData) {
    if (m_usFlags & MANAGERCONTROLLED_MASK) {
        m_fScaledTime = -std::numeric_limits<float>::max();
    }
    else if (DontDoUpdate(UpdateData.fTime) && (!m_spInterpolator || !m_spInterpolator->AlwaysUpdate())) {
        return;
    }

    if (!m_spInterpolator) {
        return;
    }

    float fValue;
    if (m_spInterpolator->Update(m_fScaledTime, m_pkTarget, fValue)) {
        SetTargetFloatValue(fValue);
    }
}

NiInterpolator* NiPSysModifierFloatCtlr::CreatePoseInterpolator(uint16_t) {
    float fValue;
    GetTargetFloatValue(fValue);
    return new NiFloatInterpolator(fValue);
}

// Sets the pose of apInterp, which must be a NiFloatInterpolator, to the target's current value.
void NiPSysModifierFloatCtlr::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
    NiFloatInterpolator* pkInterp = apInterp
        ? static_cast<NiFloatInterpolator*>(apInterp->DynamicCastMember(&NiFloatInterpolator::ms_RTTI))
        : nullptr;
    float fValue;
    GetTargetFloatValue(fValue);
    pkInterp->SetPoseValue(fValue);
}

// NiFloatInterpController and NiGeomMorpherController use the same function.
NiBlendInterpolator* NiPSysModifierFloatCtlr::CreateBlendInterpolator(
    uint16_t,
    int32_t bManagerControlled,
    bool,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    return new NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

bool NiPSysModifierFloatCtlr::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsFloatValueSupported();
}
