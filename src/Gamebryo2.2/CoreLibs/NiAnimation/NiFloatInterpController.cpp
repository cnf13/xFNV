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

// Unregistered (KEEP, uninitialised read): the exe's value slot is the dword its `push ecx` (0xA575E0) left there,
// and NiLightDimmerController::GetTargetFloatValue (0xA4CF70) leaves it unwritten when the target is null, so the
// exe poses the interpolator at the low bits of `this`; src cannot read that slot.
// Sets the pose of apInterp, which must be a NiFloatInterpolator, to the target's current value. The exe tests
// apInterp for null inline and calls the member DynamicCast out of line (0x653290).
void NiFloatInterpController::SynchronizePoseInterpolator(NiInterpolator* apInterp, uint16_t) {
	NiFloatInterpolator* pkInterp = apInterp
		? static_cast<NiFloatInterpolator*>(apInterp->DynamicCastMember(&NiFloatInterpolator::ms_RTTI))
		: nullptr;
	float fValue;
	GetTargetFloatValue(fValue);
	pkInterp->SetPoseValue(fValue);
}

NiBlendInterpolator* NiFloatInterpController::CreateBlendInterpolator(uint16_t, int32_t bManagerControlled, bool,
	float fWeightThreshold, int32_t ucArraySize) const {
	void* pvMem = NiMemObject::operator new(sizeof(NiBlendFloatInterpolator));
	return pvMem ? ::new (pvMem) NiBlendFloatInterpolator(bManagerControlled, fWeightThreshold, ucArraySize) : nullptr;
}

// Unregistered (KEEP, uninitialised read): the exe's value slot is apInterp's argument slot ([esp+0xC] at 0xA57694),
// and NiLightDimmerController::GetTargetFloatValue (0xA4CF70) leaves it unwritten when the target is null, so the
// exe passes apInterp's bits to SetPoseValue; src cannot read that slot.
bool NiFloatInterpController::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator, uint16_t) const {
	return pkInterpolator->IsFloatValueSupported();
}

void NiFloatInterpController::CopyMembers(NiFloatInterpController* pkDest, NiCloningProcess& kCloning) {
	NiSingleInterpController::CopyMembers(pkDest, kCloning);
}
