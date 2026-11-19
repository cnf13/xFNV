#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBoolInterpController::ms_RTTI("NiBoolInterpController", &NiSingleInterpController::ms_RTTI);

NiBoolInterpController::NiBoolInterpController() : NiSingleInterpController() {}

NiBoolInterpController::~NiBoolInterpController() {}

const NiRTTI* NiBoolInterpController::GetRTTI() const {
    return &ms_RTTI;
}

void NiBoolInterpController::LoadBinary(NiStream& kStream) {
    NiSingleInterpController::LoadBinary(kStream);
}

void NiBoolInterpController::LinkObject(NiStream& kStream) {
    NiSingleInterpController::LinkObject(kStream);
}

bool NiBoolInterpController::RegisterStreamables(NiStream& kStream) {
    return NiSingleInterpController::RegisterStreamables(kStream);
}

void NiBoolInterpController::SaveBinary(NiStream& kStream) {
    NiSingleInterpController::SaveBinary(kStream);
}

bool NiBoolInterpController::IsEqual(NiObject* pkObject) {
    return NiSingleInterpController::IsEqual(pkObject);
}

void NiBoolInterpController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiSingleInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

NiInterpolator* NiBoolInterpController::CreatePoseInterpolator(uint16_t) {
    bool bValue;
    GetTargetBoolValue(bValue);
    return new NiBoolInterpolator(bValue);
}

// Sets the pose of apInterp, which must be a NiBoolInterpolator, to the target's current value.
void NiBoolInterpController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
    NiBoolInterpolator* pkInterp = apInterp
        ? static_cast<NiBoolInterpolator*>(apInterp->DynamicCastMember(&NiBoolInterpolator::ms_RTTI))
        : nullptr;
    bool bValue;
    GetTargetBoolValue(bValue);
    pkInterp->SetPoseValue(bValue);
}

NiBlendInterpolator* NiBoolInterpController::CreateBlendInterpolator(
    uint16_t,
    int32_t bManagerControlled,
    bool,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    return new NiBlendBoolInterpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

bool NiBoolInterpController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsBoolValueSupported();
}

void NiBoolInterpController::CopyMembers(NiBoolInterpController* pkDest, NiCloningProcess& kCloning) {
    NiSingleInterpController::CopyMembers(pkDest, kCloning);
}
