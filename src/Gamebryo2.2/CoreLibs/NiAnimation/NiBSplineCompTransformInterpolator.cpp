#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineCompTransformInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineData.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBSplineTransformInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstring>

NiRTTI NiBSplineCompTransformInterpolator::ms_RTTI(
    "NiBSplineCompTransformInterpolator",
    &NiBSplineTransformInterpolator::ms_RTTI
);

NiBSplineCompTransformInterpolator::NiBSplineCompTransformInterpolator(
    NiBSplineData* pkData,
    uint32_t kTransCPHandle,
    uint32_t kRotCPHandle,
    uint32_t kScaleCPHandle,
    NiBSplineBasisData* pkBasisData
)
  : NiBSplineTransformInterpolator(pkData, kTransCPHandle, kRotCPHandle, kScaleCPHandle, pkBasisData) {
    m_afCompScalars[POSITION_OFFSET] = 3.4028235e+38f;
    m_afCompScalars[POSITION_RANGE] = 3.4028235e+38f;
    m_afCompScalars[ROTATION_OFFSET] = 3.4028235e+38f;
    m_afCompScalars[ROTATION_RANGE] = 3.4028235e+38f;
    m_afCompScalars[SCALE_OFFSET] = 3.4028235e+38f;
    m_afCompScalars[SCALE_RANGE] = 3.4028235e+38f;
}

NiBSplineCompTransformInterpolator::~NiBSplineCompTransformInterpolator() {}

const NiRTTI* NiBSplineCompTransformInterpolator::GetRTTI() const {
    return &ms_RTTI;
}

NiObject* NiBSplineCompTransformInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBSplineCompTransformInterpolator* pkObject = static_cast<NiBSplineCompTransformInterpolator*>(
        NiMemObject::operator new(sizeof(NiBSplineCompTransformInterpolator))
    );
    NiBSplineCompTransformInterpolator* pkObject_2 = pkObject;
    if (pkObject) {
        ::new (pkObject) NiBSplineCompTransformInterpolator(
            static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE, NiBSplineData::INVALID_HANDLE,
            NiBSplineData::INVALID_HANDLE, static_cast<NiBSplineBasisData*>(nullptr)
        );
    }
    else {
        pkObject_2 = nullptr;
    }
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_afCompScalars[0] = m_afCompScalars[0];
    pkObject_2->m_afCompScalars[1] = m_afCompScalars[1];
    pkObject_2->m_afCompScalars[2] = m_afCompScalars[2];
    pkObject_2->m_afCompScalars[3] = m_afCompScalars[3];
    pkObject_2->m_afCompScalars[4] = m_afCompScalars[4];
    pkObject_2->m_afCompScalars[5] = m_afCompScalars[5];
    return pkObject_2;
}

void NiBSplineCompTransformInterpolator::LoadBinary(NiStream& kStream) {
    NiBSplineTransformInterpolator::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

void NiBSplineCompTransformInterpolator::LinkObject(NiStream& kStream) {
    NiBSplineTransformInterpolator::LinkObject(kStream);
}

bool NiBSplineCompTransformInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBSplineTransformInterpolator::RegisterStreamables(kStream);
}

void NiBSplineCompTransformInterpolator::SaveBinary(NiStream& kStream) {
    NiBSplineTransformInterpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_afCompScalars, NUM_SCALARS);
}

bool NiBSplineCompTransformInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBSplineTransformInterpolator::IsEqual(pkObject)) {
        return false;
    }

    NiBSplineCompTransformInterpolator* pkOther = static_cast<NiBSplineCompTransformInterpolator*>(pkObject);
    for (uint32_t i = 0; i < NUM_SCALARS; i++) {
        if (m_afCompScalars[i] != pkOther->m_afCompScalars[i]) {
            return false;
        }
    }
    return true;
}

void NiBSplineCompTransformInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBSplineTransformInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

// As the transform interpolator's, over the compressed control points.
bool NiBSplineCompTransformInterpolator::Update(float fTime, NiObjectNET*, NiQuatTransform& kValue) {
    if (m_fLastTime == fTime) {
        kValue = m_kTransformValue;
        return !m_kTransformValue.IsTransformInvalid();
    }

    float fNormTime = (fTime - m_fStartTime) / (m_fEndTime - m_fStartTime);

    if (m_kTransCPHandle != NiBSplineData::INVALID_HANDLE) {
        float afValue[3];
        m_spData->GetCompactedValueDegree3(
            fNormTime, afValue, 3, m_spBasisData, m_kTransCPHandle, m_afCompScalars[POSITION_OFFSET],
            m_afCompScalars[POSITION_RANGE]
        );
        NiPoint3 kTranslate;
        // EXE-INLINED 0x416870
        kTranslate.x = afValue[0];
        kTranslate.y = afValue[1];
        kTranslate.z = afValue[2];
        m_kTransformValue.m_kTranslate = kTranslate;
    }

    if (m_kRotCPHandle != NiBSplineData::INVALID_HANDLE) {
        // The control points hold w, x, y, z, the quaternion's own order; copied as they are.
        float afValue[4];
        m_spData->GetCompactedValueDegree3(
            fNormTime, afValue, 4, m_spBasisData, m_kRotCPHandle, m_afCompScalars[ROTATION_OFFSET],
            m_afCompScalars[ROTATION_RANGE]
        );
        std::memcpy(&m_kTransformValue.m_kRotate, afValue, sizeof(afValue));
        m_kTransformValue.m_kRotate.FastNormalize();
    }

    if (m_kScaleCPHandle != NiBSplineData::INVALID_HANDLE) {
        float fScale;
        m_spData->GetCompactedValueDegree3(
            fNormTime, &fScale, 1, m_spBasisData, m_kScaleCPHandle, m_afCompScalars[SCALE_OFFSET],
            m_afCompScalars[SCALE_RANGE]
        );
        m_kTransformValue.m_fScale = fScale;
    }

    kValue = m_kTransformValue;
    if (m_kTransformValue.IsTransformInvalid()) {
        return false;
    }
    m_fLastTime = fTime;
    return true;
}

NiObject* NiBSplineCompTransformInterpolator::CreateObject() {
    NiBSplineCompTransformInterpolator* pkObject = static_cast<NiBSplineCompTransformInterpolator*>(
        NiMemObject::operator new(sizeof(NiBSplineCompTransformInterpolator))
    );
    if (!pkObject) {
        return nullptr;
    }
    ::new (pkObject) NiBSplineCompTransformInterpolator(
        static_cast<NiBSplineData*>(nullptr), NiBSplineData::INVALID_HANDLE, NiBSplineData::INVALID_HANDLE,
        NiBSplineData::INVALID_HANDLE, static_cast<NiBSplineBasisData*>(nullptr)
    );
    return pkObject;
}
