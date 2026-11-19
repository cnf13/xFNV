#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplinePoint3Interpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBSplinePoint3Interpolator::ms_RTTI("NiBSplinePoint3Interpolator", &NiBSplineInterpolator::ms_RTTI);

// m_kPoint3Value is deliberately left uninitialised, as the game does.
NiBSplinePoint3Interpolator::NiBSplinePoint3Interpolator(
    NiBSplineData* pkData,
    uint32_t kPoint3CPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplineInterpolator(pkData, pkBasisData), m_kPoint3CPHandle(kPoint3CPHandle) {}

NiBSplinePoint3Interpolator::~NiBSplinePoint3Interpolator() {}

const NiRTTI* NiBSplinePoint3Interpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplinePoint3Interpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplinePoint3Interpolator* pkObject_2 = new NiBSplinePoint3Interpolator(
        static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE, static_cast<NiBSplineBasisData*>(nullptr)
    );
    NiBSplineInterpolator::CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_kPoint3Value = m_kPoint3Value;
    pkObject_2->m_kPoint3CPHandle = m_kPoint3CPHandle;
    return pkObject_2;
}

void NiBSplinePoint3Interpolator::LoadBinary(NiStream& kStream) {
    NiBSplineInterpolator::LoadBinary(kStream);
    m_kPoint3Value.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_kPoint3CPHandle);
}

void NiBSplinePoint3Interpolator::LinkObject(NiStream& kStream) {
    NiBSplineInterpolator::LinkObject(kStream);
}

bool NiBSplinePoint3Interpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplineInterpolator::RegisterStreamables(kStream);
}

void NiBSplinePoint3Interpolator::SaveBinary(NiStream& kStream) {
    NiBSplineInterpolator::SaveBinary(kStream);
    m_kPoint3Value.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_kPoint3CPHandle);
}

bool NiBSplinePoint3Interpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplineInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplinePoint3Interpolator* pkOther = static_cast<NiBSplinePoint3Interpolator*>(pkObject);
    return !(m_kPoint3Value != pkOther->m_kPoint3Value) && m_kPoint3CPHandle == pkOther->m_kPoint3CPHandle;
}

void NiBSplinePoint3Interpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplineInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_kPoint3CPHandle", m_kPoint3CPHandle));
}

// At an unchanged time the cached value is returned. Without control points the value stays as loaded.
bool NiBSplinePoint3Interpolator::Update(float fTime, NiObjectNET*, NiPoint3& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kPoint3Value;
        return true;
    }

    if (m_kPoint3CPHandle != NiBSplineData::INVALID_HANDLE) {
        float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
        float afValue[3];
        m_spData->GetValueDegree3(fNormTime, afValue, 3, m_spBasisData, m_kPoint3CPHandle);
        m_kPoint3Value.x = afValue[0];
        m_kPoint3Value.y = afValue[1];
        m_kPoint3Value.z = afValue[2];
    }

    kValue = m_kPoint3Value;
    m_fLastTime = fTime;
    return true;
}

uint32_t NiBSplinePoint3Interpolator::GetDimension(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 ? 3 : 0;
}

uint32_t NiBSplinePoint3Interpolator::GetDegree(uint16_t usChannel) const {
    return usChannel == 0 ? 3 : 0;
}

// A channel without control points holds its loaded value. Compares the low word of usChannel and passes 0 on to
// GetControlPointCount.
bool NiBSplinePoint3Interpolator::GetChannelPosed(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 && GetControlPointCount(0) == 0;
}

NiBSplineData::Handle NiBSplinePoint3Interpolator::GetControlHandle(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 ? m_kPoint3CPHandle : NiBSplineData::INVALID_HANDLE;
}

void NiBSplinePoint3Interpolator::SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) {
    if (usChannel == 0) {
        m_kPoint3CPHandle = kControlHandle;
    }
}

void NiBSplinePoint3Interpolator::CopyMembers(NiBSplinePoint3Interpolator* pkDest, NiCloningProcess& kCloning) {
    NiBSplineInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_kPoint3Value = m_kPoint3Value;
    pkDest->m_kPoint3CPHandle = m_kPoint3CPHandle;
}

NiObject* NiBSplinePoint3Interpolator::CreateObject() {
    return new NiBSplinePoint3Interpolator(
        static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE, static_cast<NiBSplineBasisData*>(nullptr)
    );
}
