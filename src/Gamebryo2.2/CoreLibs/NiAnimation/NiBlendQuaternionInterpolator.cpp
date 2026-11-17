#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendQuaternionInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuaternionInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBlendQuaternionInterpolator::ms_RTTI("NiBlendQuaternionInterpolator", &NiBlendInterpolator::ms_RTTI);

NiBlendQuaternionInterpolator::NiBlendQuaternionInterpolator()
  : NiBlendInterpolator(), m_kQuaternionValue(NiInterpolator::INVALID_QUATERNION) {}

NiBlendQuaternionInterpolator::~NiBlendQuaternionInterpolator() {}

NiObject* NiBlendQuaternionInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBlendQuaternionInterpolator* pkObject =
        static_cast<NiBlendQuaternionInterpolator*>(NiMemObject::operator new(sizeof(NiBlendQuaternionInterpolator)));
    NiBlendQuaternionInterpolator* pkObject_2;
    if (pkObject) {
        pkObject_2 = ::new (pkObject) NiBlendQuaternionInterpolator;
    }
    else {
        pkObject_2 = 0;
    }
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_kQuaternionValue = m_kQuaternionValue;
    return pkObject_2;
}

void NiBlendQuaternionInterpolator::LoadBinary(NiStream& kStream) {
    NiBlendInterpolator::LoadBinary(kStream);
    m_kQuaternionValue.LoadBinary(kStream);
}

void NiBlendQuaternionInterpolator::LinkObject(NiStream& kStream) {
    NiBlendInterpolator::LinkObject(kStream);
}

bool NiBlendQuaternionInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBlendInterpolator::RegisterStreamables(kStream);
}

void NiBlendQuaternionInterpolator::SaveBinary(NiStream& kStream) {
    NiBlendInterpolator::SaveBinary(kStream);
    m_kQuaternionValue.SaveBinary(kStream);
}

bool NiBlendQuaternionInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBlendInterpolator::IsEqual(pkObject)) {
        return false;
    }
    NiBlendQuaternionInterpolator* pkOther = static_cast<NiBlendQuaternionInterpolator*>(pkObject);
    return !(m_kQuaternionValue != pkOther->m_kQuaternionValue);
}

void NiBlendQuaternionInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBlendInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kQuaternionValue.GetViewerString("m_kQuaternionValue"));
}

bool NiBlendQuaternionInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiQuaternion& kValue) {
    bool bReturnValue = false;
    if (m_ucInterpCount == 1) {
        bReturnValue = StoreSingleValue(fTime, pkInterpTarget, kValue);
    }
    else if (m_ucInterpCount > 0) {
        ComputeNormalizedWeights();
        bReturnValue = BlendValues(fTime, pkInterpTarget, kValue);
    }
    m_fLastTime = fTime;
    return bReturnValue;
}

bool NiBlendQuaternionInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
    return pkInterpolator->IsQuaternionValueSupported();
}

bool NiBlendQuaternionInterpolator::StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiQuaternion& kValue) {
    if (GetSingleUpdateTime(fTime) && m_pkSingleInterpolator->Update(fTime, pkInterpTarget, m_kQuaternionValue)) {
        kValue = m_kQuaternionValue;
        return true;
    }
    kValue = INVALID_QUATERNION;
    m_kQuaternionValue = kValue;
    return false;
}

// Each rotation that updates is weighed into the running sum, which is renormalized after every step; a rotation
// on the far hemisphere of the sum is negated first so the blend takes the short way.
bool NiBlendQuaternionInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuaternion& kValue) {
    m_kQuaternionValue.m_fW = 0.0f;
    m_kQuaternionValue.m_fX = 0.0f;
    m_kQuaternionValue.m_fY = 0.0f;
    m_kQuaternionValue.m_fZ = 0.0f;
    bool bValidValue = false;
    float fSumOfWeights = 0.0f;
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        InterpArrayItem& kItem = m_pkInterpArray[uc];
        if (!kItem.m_spInterpolator || !(kItem.m_fNormalizedWeight > 0.0f)) {
            continue;
        }
        float fUpdateTime = fTime;
        if (!GetUpdateTimeForItem(fUpdateTime, kItem)) {
            continue;
        }
        NiQuaternion kQuat;
        if (!kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, kQuat)) {
            continue;
        }

        if (fSumOfWeights > 0.0f) {
            float fDot = m_kQuaternionValue.m_fX * kQuat.m_fX + m_kQuaternionValue.m_fW * kQuat.m_fW +
                m_kQuaternionValue.m_fY * kQuat.m_fY + m_kQuaternionValue.m_fZ * kQuat.m_fZ;
            if (fDot < 0.0f) {
                kQuat = -kQuat;
            }
        }
        kQuat = kQuat * kItem.m_fNormalizedWeight;
        m_kQuaternionValue = m_kQuaternionValue * fSumOfWeights;
        m_kQuaternionValue.m_fW = kQuat.m_fW + m_kQuaternionValue.m_fW;
        m_kQuaternionValue.m_fX = kQuat.m_fX + m_kQuaternionValue.m_fX;
        m_kQuaternionValue.m_fY = kQuat.m_fY + m_kQuaternionValue.m_fY;
        m_kQuaternionValue.m_fZ = kQuat.m_fZ + m_kQuaternionValue.m_fZ;
        m_kQuaternionValue.Normalize();
        bValidValue = true;
        fSumOfWeights = kItem.m_fNormalizedWeight + fSumOfWeights;
    }

    if (!bValidValue) {
        m_kQuaternionValue = INVALID_QUATERNION;
    }
    kValue = m_kQuaternionValue;
    return bValidValue;
}

NiObject* NiBlendQuaternionInterpolator::CreateObject() {
    NiBlendQuaternionInterpolator* pkObject =
        static_cast<NiBlendQuaternionInterpolator*>(NiMemObject::operator new(sizeof(NiBlendQuaternionInterpolator)));
    if (!pkObject) {
        return 0;
    }
    return ::new (pkObject) NiBlendQuaternionInterpolator;
}
