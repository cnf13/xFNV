#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompColorInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstring>
#include <limits>

NiRTTI NiBSplineCompColorInterpolator::ms_RTTI("NiBSplineCompColorInterpolator", &NiBSplineColorInterpolator::ms_RTTI);

NiBSplineCompColorInterpolator::NiBSplineCompColorInterpolator(
    NiBSplineData* pkData,
    uint32_t kColorCPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplineColorInterpolator(pkData, kColorCPHandle, pkBasisData) {
    m_afCompScalars[COLORA_OFFSET] = std::numeric_limits<float>::max();
    m_afCompScalars[COLORA_RANGE] = std::numeric_limits<float>::max();
}

NiBSplineCompColorInterpolator::~NiBSplineCompColorInterpolator() {}

const NiRTTI* NiBSplineCompColorInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplineCompColorInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineCompColorInterpolator* pkObject_2 = new NiBSplineCompColorInterpolator(
        static_cast<NiBSplineData*>(nullptr),
        NiBSplineData::INVALID_HANDLE,
        static_cast<NiBSplineBasisData*>(nullptr)
    );
    CopyMembers(pkObject_2, kCloning);
    std::memcpy(pkObject_2->m_afCompScalars, m_afCompScalars, sizeof(pkObject_2->m_afCompScalars));
    return pkObject_2;
}

void NiBSplineCompColorInterpolator::LoadBinary(NiStream& kStream) {
    NiBSplineColorInterpolator::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

void NiBSplineCompColorInterpolator::LinkObject(NiStream& kStream) {
    NiBSplineColorInterpolator::LinkObject(kStream);
}

bool NiBSplineCompColorInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplineColorInterpolator::RegisterStreamables(kStream);
}

void NiBSplineCompColorInterpolator::SaveBinary(NiStream& kStream) {
    NiBSplineColorInterpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

bool NiBSplineCompColorInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplineColorInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineCompColorInterpolator* pkOther = static_cast<NiBSplineCompColorInterpolator*>(pkObject);
    for (uint32_t i = 0; i < NUM_SCALARS; i++) {
        if (m_afCompScalars[i] != pkOther->m_afCompScalars[i]) {
            return false;
        }
    }
    return true;
}

void NiBSplineCompColorInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplineColorInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// As the color interpolator's, over the compressed control points.
bool NiBSplineCompColorInterpolator::Update(float fTime, NiObjectNET*, NiColorA& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kColorAValue;
        return true;
    }

    if (m_kColorACPHandle != NiBSplineData::INVALID_HANDLE) {
        float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
        float afValue[4];
        m_spData->GetCompactedValueDegree3(
            fNormTime,
            afValue,
            4,
            m_spBasisData,
            m_kColorACPHandle,
            m_afCompScalars[COLORA_OFFSET],
            m_afCompScalars[COLORA_RANGE]
        );
        m_kColorAValue = NiColorA(afValue[0], afValue[1], afValue[2], afValue[3]);
    }

    kValue = m_kColorAValue;
    m_fLastTime = fTime;
    return true;
}

NiObject* NiBSplineCompColorInterpolator::CreateObject() {
    return new NiBSplineCompColorInterpolator(
        static_cast<NiBSplineData*>(nullptr),
        NiBSplineData::INVALID_HANDLE,
        static_cast<NiBSplineBasisData*>(nullptr)
    );
}
