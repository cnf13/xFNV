#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompFloatInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstring>
#include <limits>

NiRTTI NiBSplineCompFloatInterpolator::ms_RTTI("NiBSplineCompFloatInterpolator", &NiBSplineFloatInterpolator::ms_RTTI);

NiBSplineCompFloatInterpolator::NiBSplineCompFloatInterpolator(
    NiBSplineData* pkData,
    uint32_t kFloatCPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplineFloatInterpolator(pkData, kFloatCPHandle, pkBasisData) {
    m_afCompScalars[FLOAT_OFFSET] = std::numeric_limits<float>::max();
    m_afCompScalars[FLOAT_RANGE] = std::numeric_limits<float>::max();
}

NiBSplineCompFloatInterpolator::~NiBSplineCompFloatInterpolator() {}

const NiRTTI* NiBSplineCompFloatInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplineCompFloatInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineCompFloatInterpolator* pkObject_2 = new NiBSplineCompFloatInterpolator(
        static_cast<NiBSplineData*>(nullptr),
        NiBSplineData::INVALID_HANDLE,
        static_cast<NiBSplineBasisData*>(nullptr)
    );
    CopyMembers(pkObject_2, kCloning);
    std::memcpy(pkObject_2->m_afCompScalars, m_afCompScalars, sizeof(pkObject_2->m_afCompScalars));
    return pkObject_2;
}

void NiBSplineCompFloatInterpolator::LoadBinary(NiStream& kStream) {
    NiBSplineFloatInterpolator::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

void NiBSplineCompFloatInterpolator::LinkObject(NiStream& kStream) {
    NiBSplineFloatInterpolator::LinkObject(kStream);
}

bool NiBSplineCompFloatInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplineFloatInterpolator::RegisterStreamables(kStream);
}

void NiBSplineCompFloatInterpolator::SaveBinary(NiStream& kStream) {
    NiBSplineFloatInterpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

bool NiBSplineCompFloatInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplineFloatInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineCompFloatInterpolator* pkOther = static_cast<NiBSplineCompFloatInterpolator*>(pkObject);
    for (uint32_t i = 0; i < NUM_SCALARS; i++) {
        if (m_afCompScalars[i] != pkOther->m_afCompScalars[i]) {
            return false;
        }
    }
    return true;
}

void NiBSplineCompFloatInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplineFloatInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// As the float interpolator's, over the compressed control points.
bool NiBSplineCompFloatInterpolator::Update(float fTime, NiObjectNET*, float& fValue) {
    if (m_fLastTime == fTime) {
        fValue = m_fFloatValue;
        return true;
    }

    if (m_kFloatCPHandle != NiBSplineData::INVALID_HANDLE) {
        float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
        m_spData->GetCompactedValueDegree3(
            fNormTime,
            &m_fFloatValue,
            1,
            m_spBasisData,
            m_kFloatCPHandle,
            m_afCompScalars[FLOAT_OFFSET],
            m_afCompScalars[FLOAT_RANGE]
        );
    }

    fValue = m_fFloatValue;
    m_fLastTime = fTime;
    return true;
}

NiObject* NiBSplineCompFloatInterpolator::CreateObject() {
    return new NiBSplineCompFloatInterpolator(
        static_cast<NiBSplineData*>(nullptr),
        NiBSplineData::INVALID_HANDLE,
        static_cast<NiBSplineBasisData*>(nullptr)
    );
}
