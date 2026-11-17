#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendPoint3Interpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiPoint3Interpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBlendPoint3Interpolator::ms_RTTI("NiBlendPoint3Interpolator", &NiBlendInterpolator::ms_RTTI);

NiBlendPoint3Interpolator::NiBlendPoint3Interpolator()
  : NiBlendInterpolator(), m_kPoint3Value(NiInterpolator::INVALID_POINT3), m_bValidValue(false) {}

NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(
    int32_t abManagerControlled,
    float afWeightThreshold,
    int32_t aucArraySize
)
  : NiBlendInterpolator(abManagerControlled, afWeightThreshold, aucArraySize),
    m_kPoint3Value(NiInterpolator::INVALID_POINT3), m_bValidValue(false) {}

NiBlendPoint3Interpolator::~NiBlendPoint3Interpolator() {}

NiObject* NiBlendPoint3Interpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBlendPoint3Interpolator* pkObject =
        static_cast<NiBlendPoint3Interpolator*>(NiMemObject::operator new(sizeof(NiBlendPoint3Interpolator)));
    NiBlendPoint3Interpolator* pkObject_2 = pkObject;
    if (pkObject) {
        ::new (pkObject) NiBlendPoint3Interpolator;
    }
    else {
        pkObject_2 = 0;
    }
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_kPoint3Value = m_kPoint3Value;
    pkObject_2->m_bValidValue = m_bValidValue;
    return pkObject_2;
}

// A value other than the invalid point counts as valid. The point compare treats any coordinate unequal, or unordered,
// as different.
void NiBlendPoint3Interpolator::LoadBinary(NiStream& kStream) {
    NiBlendInterpolator::LoadBinary(kStream);
    m_kPoint3Value.LoadBinary(kStream);
    // EXE-INLINED 0x439090
    if (m_kPoint3Value.x != INVALID_POINT3.x || m_kPoint3Value.y != INVALID_POINT3.y ||
        m_kPoint3Value.z != INVALID_POINT3.z) {
        m_bValidValue = true;
    }
}

void NiBlendPoint3Interpolator::LinkObject(NiStream& kStream) {
    NiBlendInterpolator::LinkObject(kStream);
}

bool NiBlendPoint3Interpolator::RegisterStreamables(NiStream& kStream) {
    return NiBlendInterpolator::RegisterStreamables(kStream);
}

void NiBlendPoint3Interpolator::SaveBinary(NiStream& kStream) {
    NiBlendInterpolator::SaveBinary(kStream);
    m_kPoint3Value.SaveBinary(kStream);
}

bool NiBlendPoint3Interpolator::IsEqual(NiObject* pkObject) {
    if (!NiBlendInterpolator::IsEqual(pkObject)) {
        return false;
    }
    NiBlendPoint3Interpolator* pkOther = static_cast<NiBlendPoint3Interpolator*>(pkObject);
    return !(m_kPoint3Value != pkOther->m_kPoint3Value);
}

void NiBlendPoint3Interpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBlendInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kPoint3Value.GetViewerString("m_kPoint3Value"));
}

bool NiBlendPoint3Interpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue) {
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

bool NiBlendPoint3Interpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
    return pkInterpolator->IsPoint3ValueSupported();
}

// Leaves m_bValidValue as it is.
bool NiBlendPoint3Interpolator::StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue) {
    if (!GetSingleUpdateTime(fTime)) {
        return false;
    }
    if (m_pkSingleInterpolator->Update(fTime, pkInterpTarget, m_kPoint3Value)) {
        kValue = m_kPoint3Value;
        return true;
    }
    m_kPoint3Value = INVALID_POINT3;
    kValue = m_kPoint3Value;
    return false;
}

bool NiBlendPoint3Interpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiPoint3& kValue) {
    float fTotalSumOfWeights = 1.0f;
    m_bValidValue = false;
    m_kPoint3Value = NiPoint3_ZERO;
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        InterpArrayItem& kItem = m_pkInterpArray[uc];
        if (!kItem.m_spInterpolator || !(kItem.m_fNormalizedWeight > 0.0f)) {
            continue;
        }
        float fUpdateTime = fTime;
        if (!GetUpdateTimeForItem(fUpdateTime, kItem)) {
            fTotalSumOfWeights -= kItem.m_fNormalizedWeight;
            continue;
        }
        NiPoint3 kPoint;
        if (kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, kPoint)) {
            float fWeight = kItem.m_fNormalizedWeight;
            float fX = kPoint.x * fWeight;
            float fY = kPoint.y * fWeight;
            float fZ = fWeight * kPoint.z;
            m_kPoint3Value.x = m_kPoint3Value.x + fX;
            m_kPoint3Value.y = fY + m_kPoint3Value.y;
            m_kPoint3Value.z = m_kPoint3Value.z + fZ;
            m_bValidValue = true;
        }
        else {
            fTotalSumOfWeights -= kItem.m_fNormalizedWeight;
        }
    }

    if (m_bValidValue) {
        float fInvTotal = 1.0f / fTotalSumOfWeights;
        m_kPoint3Value.x = m_kPoint3Value.x * fInvTotal;
        m_kPoint3Value.y = m_kPoint3Value.y * fInvTotal;
        m_kPoint3Value.z = fInvTotal * m_kPoint3Value.z;
        kValue = m_kPoint3Value;
        return true;
    }
    m_kPoint3Value = INVALID_POINT3;
    kValue = m_kPoint3Value;
    return false;
}

NiObject* NiBlendPoint3Interpolator::CreateObject() {
    NiBlendPoint3Interpolator* pkObject =
        static_cast<NiBlendPoint3Interpolator*>(NiMemObject::operator new(sizeof(NiBlendPoint3Interpolator)));
    if (!pkObject) {
        return 0;
    }
    ::new (pkObject) NiBlendPoint3Interpolator;
    return pkObject;
}
