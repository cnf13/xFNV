#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <limits>

float NiInterpolator::INVALID_FLOAT = -std::numeric_limits<float>::max();

NiPoint3 NiInterpolator::INVALID_POINT3{
    -std::numeric_limits<float>::max(),
    -std::numeric_limits<float>::max(),
    -std::numeric_limits<float>::max()
};
NiColorA NiInterpolator::INVALID_COLORA{
    -std::numeric_limits<float>::max(),
    -std::numeric_limits<float>::max(),
    -std::numeric_limits<float>::max(),
    -std::numeric_limits<float>::max()
};
NiRTTI NiInterpolator::ms_RTTI("NiInterpolator", &NiObject::ms_RTTI);

NiInterpolator::NiInterpolator() : m_fLastTime(-std::numeric_limits<float>::max()) {}

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
    fBeginKeyTime = 0.0f;
    fEndKeyTime = 0.0f;
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
