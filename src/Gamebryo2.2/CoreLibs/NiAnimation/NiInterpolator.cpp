#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>

float NiInterpolator::INVALID_FLOAT = -FLT_MAX;

NiPoint3 NiInterpolator::INVALID_POINT3{-FLT_MAX, -FLT_MAX, -FLT_MAX};
NiColorA NiInterpolator::INVALID_COLORA{-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};
NiRTTI NiInterpolator::ms_RTTI("NiInterpolator", &NiObject::ms_RTTI);

NiInterpolator::NiInterpolator() : m_fLastTime(-3.4028235e+38f) {}

NiInterpolator::~NiInterpolator() {}

void NiInterpolator::LoadBinary(NiStream& kStream) {
	NiObject::LoadBinary(kStream);
}

void NiInterpolator::LinkObject(NiStream& kStream) {
	NiObject::LinkObject(kStream);
}

bool NiInterpolator::RegisterStreamables(NiStream& kStream) {
	return NiObject::RegisterStreamables(kStream);
}

void NiInterpolator::SaveBinary(NiStream& kStream) {
	NiObject::SaveBinary(kStream);
}

bool NiInterpolator::IsEqual(NiObject* pkObject) {
	return NiObject::IsEqual(pkObject);
}

void NiInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiObject::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

void NiInterpolator::GetActiveTimeRange(float& fBeginKeyTime, float& fEndKeyTime) const {
	fBeginKeyTime = (float)0.0;
	fEndKeyTime = (float)0.0;
}

// A copy of this interpolator for the [afStartTime, afEndTime] part of a sequence; the base copies everything.
NiInterpolator* NiInterpolator::GetSequenceInterpolator(float, float) {
	NiInterpolator* pkSeqInterp = static_cast<NiInterpolator*>(Clone());
	pkSeqInterp->SetUpDependencies();
	return pkSeqInterp;
}

void NiInterpolator::CopyMembers(NiInterpolator* pkDest, NiCloningProcess& kCloning) {
	NiObject::CopyMembers(pkDest, kCloning);
}
