#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3InterpController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiPoint3InterpController::ms_RTTI("NiPoint3InterpController", &NiSingleInterpController::ms_RTTI);

NiPoint3InterpController::NiPoint3InterpController() {}

NiPoint3InterpController::~NiPoint3InterpController() {}

const NiRTTI* NiPoint3InterpController::GetRTTI() const {
    return &ms_RTTI;
}

void NiPoint3InterpController::LoadBinary(NiStream& kStream) {
    NiSingleInterpController::LoadBinary(kStream);
}

void NiPoint3InterpController::LinkObject(NiStream& kStream) {
    NiSingleInterpController::LinkObject(kStream);
}

bool NiPoint3InterpController::RegisterStreamables(NiStream& kStream) {
    return NiSingleInterpController::RegisterStreamables(kStream);
}

void NiPoint3InterpController::SaveBinary(NiStream& kStream) {
    NiSingleInterpController::SaveBinary(kStream);
}

bool NiPoint3InterpController::IsEqual(NiObject* pkObject) {
    return NiSingleInterpController::IsEqual(pkObject);
}

void NiPoint3InterpController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiSingleInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A new interpolator posed at the target's current value.
NiInterpolator* NiPoint3InterpController::CreatePoseInterpolator(uint16_t) {
    NiPoint3 kValue;
    GetTargetPoint3Value(kValue);
    return new NiPoint3Interpolator(kValue);
}

// Sets the pose of apInterp, which must be a NiPoint3Interpolator, to the target's current value.
void NiPoint3InterpController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
    NiPoint3Interpolator* pkInterp = static_cast<NiPoint3Interpolator*>(
        apInterp ? apInterp->DynamicCastMember(&NiPoint3Interpolator::ms_RTTI) : nullptr
    );
    NiPoint3 kValue;
    GetTargetPoint3Value(kValue);
    pkInterp->SetPoseValue(kValue);
}

// A new blend interpolator for the animated point, which the controller then feeds its sources. bManagerControlled and
// ucArraySize are passed on to the constructor as the caller's whole argument words, unchanged.
NiBlendInterpolator* NiPoint3InterpController::CreateBlendInterpolator(
    uint16_t,
    int32_t bManagerControlled,
    bool,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    return new NiBlendPoint3Interpolator(bManagerControlled, fWeightThreshold, ucArraySize);
}

bool NiPoint3InterpController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsPoint3ValueSupported();
}

void NiPoint3InterpController::CopyMembers(NiPoint3InterpController* pkDest, NiCloningProcess& kCloning) {
    NiSingleInterpController::CopyMembers(pkDest, kCloning);
}
