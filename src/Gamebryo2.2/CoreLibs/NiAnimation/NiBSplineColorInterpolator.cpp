#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineColorInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineBasisData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBSplineColorInterpolator::ms_RTTI("NiBSplineColorInterpolator", &NiBSplineInterpolator::ms_RTTI);

NiBSplineColorInterpolator::NiBSplineColorInterpolator(
    NiBSplineData* pkData,
    uint32_t kColorACPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplineInterpolator(pkData, pkBasisData), m_kColorAValue(0.0f, 0.0f, 0.0f, 0.0f),
    m_kColorACPHandle(kColorACPHandle) {}

NiBSplineColorInterpolator::~NiBSplineColorInterpolator() {}

const NiRTTI* NiBSplineColorInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplineColorInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineColorInterpolator* pkObject_2 = new NiBSplineColorInterpolator(
        static_cast<NiBSplineData*>(nullptr),
        NiBSplineData::INVALID_HANDLE,
        static_cast<NiBSplineBasisData*>(nullptr)
    );
    NiBSplineInterpolator::CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_kColorAValue = m_kColorAValue;
    pkObject_2->m_kColorACPHandle = m_kColorACPHandle;
    return pkObject_2;
}

void NiBSplineColorInterpolator::LoadBinary(NiStream& kStream) {
    NiBSplineInterpolator::LoadBinary(kStream);
    m_kColorAValue.LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_kColorACPHandle);
}

void NiBSplineColorInterpolator::LinkObject(NiStream& kStream) {
    NiBSplineInterpolator::LinkObject(kStream);
}

bool NiBSplineColorInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplineInterpolator::RegisterStreamables(kStream);
}

void NiBSplineColorInterpolator::SaveBinary(NiStream& kStream) {
    NiBSplineInterpolator::SaveBinary(kStream);
    m_kColorAValue.SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_kColorACPHandle);
}

bool NiBSplineColorInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplineInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineColorInterpolator* pkOther = static_cast<NiBSplineColorInterpolator*>(pkObject);
    return !(m_kColorAValue != pkOther->m_kColorAValue) && m_kColorACPHandle == pkOther->m_kColorACPHandle;
}

void NiBSplineColorInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplineInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_kColorACPHandle", m_kColorACPHandle));
}

// At an unchanged time the cached value is returned. Without control points the value stays as loaded.
bool NiBSplineColorInterpolator::Update(float fTime, NiObjectNET*, NiColorA& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kColorAValue;
        return true;
    }

    if (m_kColorACPHandle != NiBSplineData::INVALID_HANDLE) {
        float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
        float afValue[4];
        m_spData->GetValueDegree3(fNormTime, afValue, 4, m_spBasisData, m_kColorACPHandle);
        m_kColorAValue = NiColorA(afValue[0], afValue[1], afValue[2], afValue[3]);
    }

    kValue = m_kColorAValue;
    m_fLastTime = fTime;
    return true;
}

uint32_t NiBSplineColorInterpolator::GetDimension(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 ? 4 : 0;
}

uint32_t NiBSplineColorInterpolator::GetDegree(uint16_t usChannel) const {
    return usChannel == 0 ? 3 : 0;
}

bool NiBSplineColorInterpolator::GetChannelPosed(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 && GetControlPointCount(0) == 0;
}

NiBSplineData::Handle NiBSplineColorInterpolator::GetControlHandle(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 ? m_kColorACPHandle : NiBSplineData::INVALID_HANDLE;
}

void NiBSplineColorInterpolator::SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) {
    if (usChannel == 0) {
        m_kColorACPHandle = kControlHandle;
    }
}

void NiBSplineColorInterpolator::CopyMembers(NiBSplineColorInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiBSplineInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_kColorAValue = m_kColorAValue;
    pkDest->m_kColorACPHandle = m_kColorACPHandle;
}

NiObject* NiBSplineColorInterpolator::CreateObject() {
    return new NiBSplineColorInterpolator(
        static_cast<NiBSplineData*>(nullptr),
        NiBSplineData::INVALID_HANDLE,
        static_cast<NiBSplineBasisData*>(nullptr)
    );
}
