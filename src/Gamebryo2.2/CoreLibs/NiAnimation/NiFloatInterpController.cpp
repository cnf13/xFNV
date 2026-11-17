#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpController.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiSingleInterpController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiFloatInterpController::ms_RTTI("NiFloatInterpController", &NiSingleInterpController::ms_RTTI);

NiFloatInterpController::NiFloatInterpController() {}

NiFloatInterpController::~NiFloatInterpController() {}

const NiRTTI* NiFloatInterpController::GetRTTI() const {
    return &ms_RTTI;
}

// NiBoolInterpController and NiPoint3InterpController use the same function.
void NiFloatInterpController::LoadBinary(NiStream& kStream) {
    NiSingleInterpController::LoadBinary(kStream);
}

// NiBoolInterpController and NiPoint3InterpController use the same function.
void NiFloatInterpController::LinkObject(NiStream& kStream) {
    NiSingleInterpController::LinkObject(kStream);
}

// NiBoolInterpController and NiPoint3InterpController use the same function.
bool NiFloatInterpController::RegisterStreamables(NiStream& kStream) {
    return NiSingleInterpController::RegisterStreamables(kStream);
}

// NiBoolInterpController and NiPoint3InterpController use the same function.
void NiFloatInterpController::SaveBinary(NiStream& kStream) {
    NiSingleInterpController::SaveBinary(kStream);
}

// NiBoolInterpController and NiPoint3InterpController use the same function.
bool NiFloatInterpController::IsEqual(NiObject* pkObject) {
    return NiSingleInterpController::IsEqual(pkObject);
}

void NiFloatInterpController::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiSingleInterpController::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// A new interpolator posed at the target's current value.
NiInterpolator* NiFloatInterpController::CreatePoseInterpolator(uint16_t) {
    float fValue;
    GetTargetFloatValue(fValue);
    void* pvMemory = NiMemObject::operator new(sizeof(NiFloatInterpolator));
    if (!pvMemory) {
        return nullptr;
    }
    return ::new (pvMemory) NiFloatInterpolator(fValue);
}

// Sets the pose of apInterp, which must be a NiFloatInterpolator, to the target's current value.
void NiFloatInterpController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
    NiFloatInterpolator* pkInterp = apInterp
        ? static_cast<NiFloatInterpolator*>(apInterp->DynamicCastMember(&NiFloatInterpolator::ms_RTTI))
        : nullptr;
    float fValue;
    GetTargetFloatValue(fValue);
    pkInterp->SetPoseValue(fValue);
}

NiBlendInterpolator* NiFloatInterpController::CreateBlendInterpolator(
    uint16_t,
    int32_t bManagerControlled,
    bool,
    float fWeightThreshold,
    int32_t ucArraySize
) const {
    void* pvMem = NiMemObject::operator new(sizeof(NiBlendFloatInterpolator));
    return pvMem ? ::new (pvMem) NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize) : nullptr;
}

bool NiFloatInterpController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
    return pkInterpolator->IsFloatValueSupported();
}

void NiFloatInterpController::CopyMembers(NiFloatInterpController* pkDest, NiCloningProcess& kCloning) {
    NiSingleInterpController::CopyMembers(pkDest, kCloning);
}
