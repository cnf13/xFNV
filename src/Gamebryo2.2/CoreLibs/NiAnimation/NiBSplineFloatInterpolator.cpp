#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineFloatInterpolator.h"

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

NiRTTI NiBSplineFloatInterpolator::ms_RTTI("NiBSplineFloatInterpolator", &NiBSplineInterpolator::ms_RTTI);

// m_fFloatValue is deliberately left uninitialised, as the game does.
NiBSplineFloatInterpolator::NiBSplineFloatInterpolator(
    NiBSplineData* pkData,
    uint32_t kFloatCPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplineInterpolator(pkData, pkBasisData), m_kFloatCPHandle(kFloatCPHandle) {}

NiBSplineFloatInterpolator::~NiBSplineFloatInterpolator() {}

const NiRTTI* NiBSplineFloatInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplineFloatInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineFloatInterpolator* pkObject =
        static_cast<NiBSplineFloatInterpolator*>(NiMemObject::operator new(sizeof(NiBSplineFloatInterpolator)));
    NiBSplineFloatInterpolator* pkObject_2 = pkObject;
    if (pkObject) {
        ::new (pkObject) NiBSplineFloatInterpolator(
            static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE,
            static_cast<NiBSplineBasisData*>(nullptr)
        );
    }
    else {
        pkObject_2 = nullptr;
    }
    NiBSplineInterpolator::CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_fFloatValue = m_fFloatValue;
    pkObject_2->m_kFloatCPHandle = m_kFloatCPHandle;
    return pkObject_2;
}

void NiBSplineFloatInterpolator::LoadBinary(NiStream& kStream) {
    NiBSplineInterpolator::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_fFloatValue);
    NiStreamLoadBinary(kStream, m_kFloatCPHandle);
}

void NiBSplineFloatInterpolator::LinkObject(NiStream& kStream) {
    NiBSplineInterpolator::LinkObject(kStream);
}

bool NiBSplineFloatInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplineInterpolator::RegisterStreamables(kStream);
}

void NiBSplineFloatInterpolator::SaveBinary(NiStream& kStream) {
    NiBSplineInterpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_fFloatValue);
    NiStreamSaveBinary(kStream, m_kFloatCPHandle);
}

bool NiBSplineFloatInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplineInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineFloatInterpolator* pkOther = static_cast<NiBSplineFloatInterpolator*>(pkObject);
    return m_fFloatValue == pkOther->m_fFloatValue && m_kFloatCPHandle == pkOther->m_kFloatCPHandle;
}

void NiBSplineFloatInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplineInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_fFloatValue", m_fFloatValue));
    pkStrings->Add(NiGetViewerString("m_kFloatCPHandle", m_kFloatCPHandle));
}

// At an unchanged time the cached value is returned. Without control points the value stays as loaded.
bool NiBSplineFloatInterpolator::Update(float fTime, NiObjectNET*, float& fValue) {
    if (m_fLastTime == fTime) {
        fValue = m_fFloatValue;
        return true;
    }

    if (m_kFloatCPHandle != NiBSplineData::INVALID_HANDLE) {
        float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
        m_spData->GetValueDegree3(fNormTime, &m_fFloatValue, 1, m_spBasisData, m_kFloatCPHandle);
    }

    fValue = m_fFloatValue;
    m_fLastTime = fTime;
    return true;
}

uint32_t NiBSplineFloatInterpolator::GetDimension(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 ? 1 : 0;
}

// The one channel is cubic.
uint32_t NiBSplineFloatInterpolator::GetDegree(uint16_t usChannel) const {
    return usChannel == 0 ? 3 : 0;
}

bool NiBSplineFloatInterpolator::GetChannelPosed(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 && GetControlPointCount(0) == 0;
}

NiBSplineData::Handle NiBSplineFloatInterpolator::GetControlHandle(int32_t usChannel) const {
    return static_cast<uint16_t>(usChannel) == 0 ? m_kFloatCPHandle : NiBSplineData::INVALID_HANDLE;
}

void NiBSplineFloatInterpolator::SetControlHandle(NiBSplineData::Handle kControlHandle, uint16_t usChannel) {
    if (usChannel == 0) {
        m_kFloatCPHandle = kControlHandle;
    }
}

void NiBSplineFloatInterpolator::CopyMembers(NiBSplineFloatInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiBSplineInterpolator::CopyMembers(pkDest, kCloning);
    pkDest->m_fFloatValue = m_fFloatValue;
    pkDest->m_kFloatCPHandle = m_kFloatCPHandle;
}

NiObject* NiBSplineFloatInterpolator::CreateObject() {
    NiBSplineFloatInterpolator* pkObject =
        static_cast<NiBSplineFloatInterpolator*>(NiMemObject::operator new(sizeof(NiBSplineFloatInterpolator)));
    if (!pkObject) {
        return nullptr;
    }
    ::new (pkObject) NiBSplineFloatInterpolator(
        static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE, static_cast<NiBSplineBasisData*>(nullptr)
    );
    return pkObject;
}
