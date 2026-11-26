#include "Gamebryo2.2/CoreLibs/NiAnimation/BSBlendTreadTransfInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/BSTreadTransfInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"

#include <limits>

NiRTTI BSBlendTreadTransfInterpolator::ms_RTTI("BSBlendTreadTransfInterpolator", &NiBlendInterpolator::ms_RTTI);

namespace {

    // Advances the lerp by a step and wraps it back into [0, 1] by one period.
    void AdvanceLerp(float& arCurrLerp, float afStep) {
        float fLerp = afStep + arCurrLerp;
        arCurrLerp = fLerp;
        if (!(fLerp <= 1.0f)) {
            arCurrLerp = fLerp - 1.0f;
        }
        else if (fLerp < 0.0f) {
            arCurrLerp = fLerp + 1.0f;
        }
    }

} // namespace

BSBlendTreadTransfInterpolator::BSBlendTreadTransfInterpolator() : NiBlendInterpolator(), fCurrLerp(0.0f) {}

BSBlendTreadTransfInterpolator::~BSBlendTreadTransfInterpolator() {}

NiObject* BSBlendTreadTransfInterpolator::CreateClone(NiCloningProcess& kCloning) {
    BSBlendTreadTransfInterpolator* pkObject_2 = new BSBlendTreadTransfInterpolator;
    CopyMembers(pkObject_2, kCloning);
    pkObject_2->fCurrLerp = fCurrLerp;
    return pkObject_2;
}

void BSBlendTreadTransfInterpolator::LoadBinary(NiStream& arStream) {
    NiBlendInterpolator::LoadBinary(arStream);
}

void BSBlendTreadTransfInterpolator::LinkObject(NiStream& arStream) {
    NiBlendInterpolator::LinkObject(arStream);
}

bool BSBlendTreadTransfInterpolator::RegisterStreamables(NiStream& arStream) {
    return NiBlendInterpolator::RegisterStreamables(arStream);
}

void BSBlendTreadTransfInterpolator::SaveBinary(NiStream& arStream) {
    NiBlendInterpolator::SaveBinary(arStream);
}

bool BSBlendTreadTransfInterpolator::IsEqual(NiObject* apObject) {
    return NiBlendInterpolator::IsEqual(apObject);
}

bool BSBlendTreadTransfInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
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

bool BSBlendTreadTransfInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
    return pkInterpolator->IsTransformValueSupported();
}

bool BSBlendTreadTransfInterpolator::StoreSingleValue(
    float fTime,
    NiObjectNET* pkInterpTarget,
    NiQuatTransform& kValue
) {
    if (GetSingleUpdateTime(fTime) && m_pkSingleInterpolator->Update(fTime, pkInterpTarget, kValue)) {
        AdvanceLerp(fCurrLerp, static_cast<BSTreadTransfInterpolator*>(m_pkSingleInterpolator)->fDiffLerp);
        if (kValue.IsRotateValid()) {
            NiQuaternion kRotate = kValue.m_kRotate;
            kRotate.Normalize();
            kValue.m_kRotate = kRotate;
        }
        return true;
    }
    kValue.m_kTranslate.x = -std::numeric_limits<float>::max();
    kValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
    kValue.m_fScale = -std::numeric_limits<float>::max();
    return false;
}

bool BSBlendTreadTransfInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
    float fLerpStep = 0.0f;
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        InterpArrayItem& kItem = m_pkInterpArray[uc];
        if (!kItem.m_spInterpolator || !(kItem.m_fNormalizedWeight > 0.0f)) {
            continue;
        }
        float fUpdateTime = fTime;
        if (!GetUpdateTimeForItem(fUpdateTime, kItem)) {
            continue;
        }
        NiQuatTransform kTransform;
        kTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
        kTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
        kTransform.m_fScale = -std::numeric_limits<float>::max();
        if (kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, kTransform)) {
            BSTreadTransfInterpolator* pkTread = static_cast<BSTreadTransfInterpolator*>(
                static_cast<NiInterpolator*>(kItem.m_spInterpolator)
            );
            fLerpStep = pkTread->fDiffLerp * kItem.m_fNormalizedWeight + fLerpStep;
        }
    }
    AdvanceLerp(fCurrLerp, fLerpStep);
    kValue.m_kTranslate.x = -std::numeric_limits<float>::max();
    kValue.m_kRotate.m_fX = -std::numeric_limits<float>::max();
    kValue.m_fScale = -std::numeric_limits<float>::max();
    return true;
}

NiObject* BSBlendTreadTransfInterpolator::CreateObject() {
    return new BSBlendTreadTransfInterpolator;
}
