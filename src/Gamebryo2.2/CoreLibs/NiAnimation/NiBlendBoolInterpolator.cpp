#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiBlendBoolInterpolator::ms_RTTI("NiBlendBoolInterpolator", &NiBlendInterpolator::ms_RTTI);

NiBlendBoolInterpolator::NiBlendBoolInterpolator()
  : NiBlendInterpolator(), m_bBoolValue(NiInterpolator::INVALID_BOOL) {}

NiBlendBoolInterpolator::NiBlendBoolInterpolator(
    int32_t abManagerControlled,
    float afWeightThreshold,
    int32_t aucArraySize
)
  : NiBlendInterpolator(abManagerControlled, afWeightThreshold, aucArraySize),
    m_bBoolValue(NiInterpolator::INVALID_BOOL) {}

NiBlendBoolInterpolator::~NiBlendBoolInterpolator() {}

NiObject* NiBlendBoolInterpolator::CreateClone(NiCloningProcess& kCloning) {
    void* v3 = NiMemObject::operator new(sizeof(NiBlendBoolInterpolator));
    NiBlendBoolInterpolator* v4 = static_cast<NiBlendBoolInterpolator*>(v3);
    if (v3) {
        ::new (v3) NiBlendBoolInterpolator;
    }
    else {
        v4 = nullptr;
    }
    NiBlendInterpolator::CopyMembers(v4, kCloning);
    v4->m_bBoolValue = m_bBoolValue;
    return v4;
}

void NiBlendBoolInterpolator::LoadBinary(NiStream& kStream) {
    NiBlendInterpolator::LoadBinary(kStream);
    NiStreamLoadBinary(kStream, m_bBoolValue);
}

void NiBlendBoolInterpolator::LinkObject(NiStream& kStream) {
    NiBlendInterpolator::LinkObject(kStream);
}

bool NiBlendBoolInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBlendInterpolator::RegisterStreamables(kStream);
}

void NiBlendBoolInterpolator::SaveBinary(NiStream& kStream) {
    NiBlendInterpolator::SaveBinary(kStream);
    NiStreamSaveBinary(kStream, m_bBoolValue);
}

bool NiBlendBoolInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBlendInterpolator::IsEqual(pkObject)) {
        return false;
    }
    NiBlendBoolInterpolator* pkOther = static_cast<NiBlendBoolInterpolator*>(pkObject);
    return m_bBoolValue == pkOther->m_bBoolValue;
}

void NiBlendBoolInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBlendInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    pkStrings->Add(NiGetViewerString("m_bBoolValue", m_bBoolValue));
}

bool NiBlendBoolInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, bool& bValue) {
    bool bReturnValue = false;
    if (m_ucInterpCount == 1) {
        bReturnValue = StoreSingleValue(fTime, pkInterpTarget, bValue);
    }
    else if (m_ucInterpCount > 0) {
        ComputeNormalizedWeights();
        bReturnValue = BlendValues(fTime, pkInterpTarget, bValue);
    }
    m_fLastTime = fTime;
    return bReturnValue;
}

bool NiBlendBoolInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
    return pkInterpolator->IsBoolValueSupported();
}

bool NiBlendBoolInterpolator::StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, bool& bValue) {
    if (!GetSingleUpdateTime(fTime)) {
        return false;
    }
    if (m_pkSingleInterpolator->Update(fTime, pkInterpTarget, bValue)) {
        // the byte is copied as is: the interpolators write 2 (invalid) through the bool too
        m_bBoolValue = reinterpret_cast<uint8_t&>(bValue);
        return true;
    }
    bValue = false;
    m_bBoolValue = INVALID_BOOL;
    return false;
}

bool NiBlendBoolInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, bool& bValue) {
    float fTotalSumOfWeights = 1.0f;
    float fFloatValue = 0.0f;
    bool bValidValue = false;
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
        bool bValue;
        if (kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, bValue)) {
            bValidValue = true;
            fFloatValue = kItem.m_fNormalizedWeight * reinterpret_cast<uint8_t&>(bValue) + fFloatValue;
        }
        else {
            fTotalSumOfWeights -= kItem.m_fNormalizedWeight;
        }
    }

    if (!bValidValue) {
        bValue = false;
        m_bBoolValue = INVALID_BOOL;
        return false;
    }
    // a weighted average of at least one half rounds to true
    fFloatValue = fFloatValue / fTotalSumOfWeights;
    if (fFloatValue >= 0.5f) {
        m_bBoolValue = 1;
        bValue = true;
    }
    else {
        m_bBoolValue = 0;
        bValue = false;
    }
    return true;
}

NiObject* NiBlendBoolInterpolator::CreateObject() {
    NiBlendBoolInterpolator* pkObject =
        static_cast<NiBlendBoolInterpolator*>(NiMemObject::operator new(sizeof(NiBlendBoolInterpolator)));
    if (!pkObject) {
        return nullptr;
    }
    ::new (pkObject) NiBlendBoolInterpolator;
    return pkObject;
}
