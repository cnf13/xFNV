#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendColorInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBlendColorInterpolator::ms_RTTI("NiBlendColorInterpolator", &NiBlendInterpolator::ms_RTTI);

NiBlendColorInterpolator::NiBlendColorInterpolator()
  : NiBlendInterpolator(), m_kColorValue(NiInterpolator::INVALID_COLORA), m_bValidValue(false) {}

NiBlendColorInterpolator::NiBlendColorInterpolator(
    int32_t abManagerControlled,
    float afWeightThreshold,
    int32_t aucArraySize
)
  : NiBlendInterpolator(abManagerControlled, afWeightThreshold, aucArraySize),
    m_kColorValue(NiInterpolator::INVALID_COLORA), m_bValidValue(false) {}

NiBlendColorInterpolator::~NiBlendColorInterpolator() {}

NiObject* NiBlendColorInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBlendColorInterpolator* pkObject_2 = new NiBlendColorInterpolator;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->m_kColorValue = m_kColorValue;
    pkObject_2->m_bValidValue = m_bValidValue;
    return pkObject_2;
}

// A value other than the invalid color counts as valid.
void NiBlendColorInterpolator::LoadBinary(NiStream& kStream) {
    NiBlendInterpolator::LoadBinary(kStream);
    m_kColorValue.LoadBinary(kStream);
    if (m_kColorValue != INVALID_COLORA) {
        m_bValidValue = true;
    }
}

void NiBlendColorInterpolator::LinkObject(NiStream& kStream) {
    NiBlendInterpolator::LinkObject(kStream);
}

bool NiBlendColorInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBlendInterpolator::RegisterStreamables(kStream);
}

void NiBlendColorInterpolator::SaveBinary(NiStream& kStream) {
    NiBlendInterpolator::SaveBinary(kStream);
    m_kColorValue.SaveBinary(kStream);
}

bool NiBlendColorInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBlendInterpolator::IsEqual(pkObject)) {
        return false;
    }
    NiBlendColorInterpolator* pkOther = static_cast<NiBlendColorInterpolator*>(pkObject);
    return !(m_kColorValue != pkOther->m_kColorValue);
}

void NiBlendColorInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBlendInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(m_kColorValue.GetViewerString("m_kColorValue"));
}

bool NiBlendColorInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiColorA& kValue) {
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

bool NiBlendColorInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
    return pkInterpolator->IsColorAValueSupported();
}

// Leaves m_bValidValue as it is.
bool NiBlendColorInterpolator::StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiColorA& kValue) {
    if (!GetSingleUpdateTime(fTime)) {
        return false;
    }
    if (m_pkSingleInterpolator->Update(fTime, pkInterpTarget, m_kColorValue)) {
        kValue = m_kColorValue;
        return true;
    }
    kValue = INVALID_COLORA;
    m_kColorValue = kValue;
    return false;
}

bool NiBlendColorInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiColorA& kValue) {
    m_kColorValue.r = 0.0f;
    m_kColorValue.g = 0.0f;
    m_kColorValue.b = 0.0f;
    m_kColorValue.a = 0.0f;
    m_bValidValue = false;
    float fTotalSumOfWeights = 1.0f;
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        InterpArrayItem& kItem = m_pkInterpArray[uc];
        if (!kItem.m_spInterpolator || !(kItem.m_fNormalizedWeight > 0.0f)) {
            continue;
        }
        NiColorA kColor(0.0f, 0.0f, 0.0f, 0.0f);
        float fUpdateTime = fTime;
        if (!GetUpdateTimeForItem(fUpdateTime, kItem)) {
            fTotalSumOfWeights -= kItem.m_fNormalizedWeight;
            continue;
        }
        if (kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, kColor)) {
            float fWeight = kItem.m_fNormalizedWeight;
            float fR = kColor.r * fWeight;
            float fG = kColor.g * fWeight;
            float fB = kColor.b * fWeight;
            float fA = fWeight * kColor.a;
            m_kColorValue.r = m_kColorValue.r + fR;
            m_kColorValue.g = m_kColorValue.g + fG;
            m_kColorValue.b = m_kColorValue.b + fB;
            m_kColorValue.a = m_kColorValue.a + fA;
            m_bValidValue = true;
        }
        else {
            fTotalSumOfWeights -= kItem.m_fNormalizedWeight;
        }
    }

    if (m_bValidValue) {
        m_kColorValue.r = m_kColorValue.r / fTotalSumOfWeights;
        m_kColorValue.g = m_kColorValue.g / fTotalSumOfWeights;
        m_kColorValue.b = m_kColorValue.b / fTotalSumOfWeights;
        m_kColorValue.a = m_kColorValue.a / fTotalSumOfWeights;
        kValue = m_kColorValue;
        return true;
    }
    m_kColorValue = INVALID_COLORA;
    kValue = m_kColorValue;
    return false;
}

NiObject* NiBlendColorInterpolator::CreateObject() {
    return new NiBlendColorInterpolator;
}
