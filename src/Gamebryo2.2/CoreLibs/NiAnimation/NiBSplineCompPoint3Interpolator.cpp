#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompPoint3Interpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplinePoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstring>

NiRTTI
    NiBSplineCompPoint3Interpolator::ms_RTTI("NiBSplineCompPoint3Interpolator", &NiBSplinePoint3Interpolator::ms_RTTI);

NiBSplineCompPoint3Interpolator::NiBSplineCompPoint3Interpolator(
    NiBSplineData* pkData,
    uint32_t kPoint3CPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplinePoint3Interpolator(pkData, kPoint3CPHandle, pkBasisData) {
    m_afCompScalars[POINT3_OFFSET] = 3.4028235e+38f;
    m_afCompScalars[POINT3_RANGE] = 3.4028235e+38f;
}

NiBSplineCompPoint3Interpolator::~NiBSplineCompPoint3Interpolator() {}

const NiRTTI* NiBSplineCompPoint3Interpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplineCompPoint3Interpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineCompPoint3Interpolator* pkObject = static_cast<NiBSplineCompPoint3Interpolator*>(
        NiMemObject::operator new(sizeof(NiBSplineCompPoint3Interpolator))
    );
    NiBSplineCompPoint3Interpolator* pkObject_2 = pkObject;
    if (pkObject) {
        ::new (pkObject) NiBSplineCompPoint3Interpolator(
            static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE,
            static_cast<NiBSplineBasisData*>(nullptr)
        );
    }
    else {
        pkObject_2 = nullptr;
    }
    CopyMembers(pkObject_2, kCloning);
    std::memcpy(pkObject_2->m_afCompScalars, m_afCompScalars, sizeof(pkObject_2->m_afCompScalars));
    return pkObject_2;
}

void NiBSplineCompPoint3Interpolator::LoadBinary(NiStream& kStream) {
    NiBSplinePoint3Interpolator::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

void NiBSplineCompPoint3Interpolator::LinkObject(NiStream& kStream) {
    NiBSplinePoint3Interpolator::LinkObject(kStream);
}

bool NiBSplineCompPoint3Interpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplinePoint3Interpolator::RegisterStreamables(kStream);
}

void NiBSplineCompPoint3Interpolator::SaveBinary(NiStream& kStream) {
    NiBSplinePoint3Interpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

bool NiBSplineCompPoint3Interpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplinePoint3Interpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineCompPoint3Interpolator* pkOther = static_cast<NiBSplineCompPoint3Interpolator*>(pkObject);
    for (uint32_t i = 0; i < NUM_SCALARS; i++) {
        if (m_afCompScalars[i] != pkOther->m_afCompScalars[i]) {
            return false;
        }
    }
    return true;
}

void NiBSplineCompPoint3Interpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplinePoint3Interpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// As the point interpolator's, over the compressed control points.
bool NiBSplineCompPoint3Interpolator::Update(float fTime, NiObjectNET*, NiPoint3& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kPoint3Value;
        return true;
    }

    if (m_kPoint3CPHandle != NiBSplineData::INVALID_HANDLE) {
        float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);
        float afValue[3];
        m_spData->GetCompactedValueDegree3(
            fNormTime, afValue, 3, m_spBasisData, m_kPoint3CPHandle, m_afCompScalars[POINT3_OFFSET],
            m_afCompScalars[POINT3_RANGE]
        );
        NiPoint3 kValue;
        // EXE-INLINED 0x416870
        kValue.x = afValue[0];
        kValue.y = afValue[1];
        kValue.z = afValue[2];
        m_kPoint3Value = kValue;
    }

    kValue = m_kPoint3Value;
    m_fLastTime = fTime;
    return true;
}

NiObject* NiBSplineCompPoint3Interpolator::CreateObject() {
    NiBSplineCompPoint3Interpolator* pkObject = static_cast<NiBSplineCompPoint3Interpolator*>(
        NiMemObject::operator new(sizeof(NiBSplineCompPoint3Interpolator))
    );
    if (!pkObject) {
        return nullptr;
    }
    ::new (pkObject) NiBSplineCompPoint3Interpolator(
        static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE, static_cast<NiBSplineBasisData*>(nullptr)
    );
    return pkObject;
}
