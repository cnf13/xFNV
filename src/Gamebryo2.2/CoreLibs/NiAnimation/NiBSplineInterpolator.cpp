#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <limits>

NiRTTI NiBSplineInterpolator::ms_RTTI("NiBSplineInterpolator", &NiInterpolator::ms_RTTI);

// An empty time range (FLT_MAX, -FLT_MAX); both pointers take a reference.
NiBSplineInterpolator::NiBSplineInterpolator(NiBSplineData* pkData, NiBSplineBasisData* pkBasisData)
  : NiInterpolator(), m_fStartTime(0x1.fffffe0000000p+127f), m_fEndTime(-0x1.fffffe0000000p+127f), m_spData(pkData),
    m_spBasisData(pkBasisData) {}

// m_spBasisData and m_spData are released by their destructors.
NiBSplineInterpolator::~NiBSplineInterpolator() {}

const NiRTTI* NiBSplineInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

// NiInterpolator adds nothing to the stream. The data and basis links resolve here, as they are read.
void NiBSplineInterpolator::LoadBinary(NiStream& kStream) {
    NiObject::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fStartTime);
    NiStreamLoadBinary(kStream, m_fEndTime);
    m_spData = static_cast<NiBSplineData*>(kStream.ResolveLinkID());
    m_spBasisData = static_cast<NiBSplineBasisData*>(kStream.ResolveLinkID());
}

void NiBSplineInterpolator::LinkObject(NiStream& kStream) {
    NiObject::LinkObject(kStream);
}

bool NiBSplineInterpolator::RegisterStreamables(NiStream& kStream) {
    if (!NiObject::RegisterStreamables(kStream)) {
        return false;
    }

    if (m_spData) {
        m_spData->RegisterStreamables(kStream);
    }
    if (m_spBasisData) {
        m_spBasisData->RegisterStreamables(kStream);
    }
    return true;
}

void NiBSplineInterpolator::SaveBinary(NiStream& kStream) {
    NiInterpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fStartTime);
    NiStreamSaveBinary(kStream, m_fEndTime);
    kStream.SaveLinkID(m_spData);
    kStream.SaveLinkID(m_spBasisData);
}

// Equal time ranges alone make two interpolators equal. Otherwise the data and basis are compared, but the
// result is unequal whatever they hold.
bool NiBSplineInterpolator::IsEqual(NiObject* apObject) {
    if (!NiObject::IsEqual(apObject)) {
        return false;
    }

    NiBSplineInterpolator* pkOther = static_cast<NiBSplineInterpolator*>(apObject);
    if (m_fStartTime == pkOther->m_fStartTime && m_fEndTime == pkOther->m_fEndTime) {
        return true;
    }

    if ((m_spData && !pkOther->m_spData) || (!m_spData && pkOther->m_spData)) {
        return false;
    }
    if (m_spData && !m_spData->IsEqual(pkOther->m_spData)) {
        return false;
    }

    if ((m_spBasisData && !pkOther->m_spBasisData) || (!m_spBasisData && pkOther->m_spBasisData)) {
        return false;
    }
    if (m_spBasisData) {
        m_spBasisData->IsEqual(pkOther->m_spBasisData);
    }
    return false;
}

void NiBSplineInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_fStartTime", m_fStartTime));
    pkStrings->Add(NiGetViewerString("m_fEndTime", m_fEndTime));
    if (m_spData) {
        m_spData->GetViewerStrings(pkStrings);
    }
    if (m_spBasisData) {
        m_spBasisData->GetViewerStrings(pkStrings);
    }
}

// The range passed in is the caller's running union; an untouched one (FLT_MAX, -FLT_MAX) becomes empty at 0.
// Any other is replaced by this interpolator's range.
void NiBSplineInterpolator::GetActiveTimeRange(float& fBeginTime, float& fEndTime) const {
    if (fBeginTime == std::numeric_limits<float>::max() && fEndTime == -std::numeric_limits<float>::max()) {
        fBeginTime = fEndTime = 0.0f;
    }
    else {
        fBeginTime = m_fStartTime;
        fEndTime = m_fEndTime;
    }
}

// A channel without data or without a handle has no control points; the others have as many as the basis. Only the low
// 16 bits of usChannel are compared against the channel count, and the full value is passed on to GetControlHandle.
uint32_t NiBSplineInterpolator::GetControlPointCount(int32_t usChannel) const {
    if (static_cast<uint16_t>(usChannel) >= GetChannelCount() ||
        !m_spData ||
        GetControlHandle(usChannel) == NiBSplineData::INVALID_HANDLE) {
        return 0;
    }
    return m_spBasisData->GetDegree3Basis().GetQuantity();
}

// The bytes of the channel's control points: 16-bit values when compressed, floats otherwise.
uint32_t NiBSplineInterpolator::GetAllocatedSize(int32_t usChannel) const {
    if (UsesCompressedControlPoints()) {
        return GetDimension(usChannel) * GetControlPointCount(usChannel) * sizeof(int16_t);
    }
    return GetDimension(usChannel) * GetControlPointCount(usChannel) * sizeof(float);
}

void NiBSplineInterpolator::CopyMembers(NiBSplineInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiObject::CopyMembers(pkDest, kCloning);
    pkDest->m_fEndTime = m_fEndTime;
    pkDest->m_fStartTime = m_fStartTime;
    pkDest->m_spData = m_spData;
    if (m_spBasisData) {
        pkDest->m_spBasisData = static_cast<NiBSplineBasisData*>(m_spBasisData->CreateSharedClone(kCloning));
    }
}
