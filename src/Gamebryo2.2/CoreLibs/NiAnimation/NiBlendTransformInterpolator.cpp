#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendTransformInterpolator.h"

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cfloat>
#include <cmath>

NiRTTI NiBlendTransformInterpolator::ms_RTTI("NiBlendTransformInterpolator", &NiBlendInterpolator::ms_RTTI);

// The start of the blended rotation sum. Nothing writes it, so it stays all zero.
static NiQuaternion kBlendRotateStart{};

NiBlendTransformInterpolator::NiBlendTransformInterpolator() : NiBlendInterpolator() {}

NiBlendTransformInterpolator::NiBlendTransformInterpolator(
    int32_t abManagerControlled,
    float afWeightThreshold,
    int32_t aucArraySize
)
  : NiBlendInterpolator(abManagerControlled, afWeightThreshold, aucArraySize) {}

NiBlendTransformInterpolator::~NiBlendTransformInterpolator() {}

NiObject* NiBlendTransformInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBlendInterpolator* v4 = new NiBlendTransformInterpolator;
    NiBlendInterpolator::CopyMembers(v4, kCloning);
    return v4;
}

// Files before 10.1.0.110 store a transform here that is no longer used; it is read and dropped.
void NiBlendTransformInterpolator::LoadBinary(NiStream& kStream) {
    NiBlendInterpolator::LoadBinary(kStream);
    if (kStream.GetFileVersion() < NiStream::GetVersion(10, 1, 0, 110)) {
        NiQuatTransform kUnusedValue;
        kUnusedValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
        kUnusedValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
        kUnusedValue.m_fScale = -FLT_MAX;
        kUnusedValue.LoadBinary(kStream);
    }
}

void NiBlendTransformInterpolator::LinkObject(NiStream& kStream) {
    NiBlendInterpolator::LinkObject(kStream);
}

// Also the RegisterStreamables of the other blend interpolators but NiBlendAccumTransformInterpolator.
bool NiBlendTransformInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBlendInterpolator::RegisterStreamables(kStream);
}

// Also BSBlendTreadTransfInterpolator::SaveBinary.
void NiBlendTransformInterpolator::SaveBinary(NiStream& kStream) {
    NiBlendInterpolator::SaveBinary(kStream);
}

bool NiBlendTransformInterpolator::IsEqual(NiObject* pkObject) {
    return NiBlendInterpolator::IsEqual(pkObject);
}

void NiBlendTransformInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBlendInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
}

bool NiBlendTransformInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
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

// Also the InterpolatorIsCorrectType of NiBlendAccumTransformInterpolator and BSBlendTreadTransfInterpolator.
bool NiBlendTransformInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
    return pkInterpolator->IsTransformValueSupported();
}

void NiBlendTransformInterpolator::CopyMembers(NiBlendTransformInterpolator* pkDest, NiCloningProcess& kCloning) {
    NiBlendInterpolator::CopyMembers(pkDest, kCloning);
}

bool NiBlendTransformInterpolator::StoreSingleValue(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
    if (GetSingleUpdateTime(fTime) && m_pkSingleInterpolator->Update(fTime, pkInterpTarget, kValue)) {
        if (kValue.IsRotateValid()) {
            NiQuaternion kRotate = kValue.m_kRotate;
            kRotate.Normalize();
            kValue.m_kRotate = kRotate;
        }
        return true;
    }
    kValue.m_kTranslate.x = -FLT_MAX;
    kValue.m_kRotate.m_fX = -FLT_MAX;
    kValue.m_fScale = -FLT_MAX;
    return false;
}

// Translation and scale are weighted averages over the interpolators that have them; the rotations are summed
// weighted, each after the first turned into the hemisphere of the sum, and the sum normalized.
bool NiBlendTransformInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
    float fTotalTransWeight = 1.0f;
    float fTotalScaleWeight = 1.0f;
    NiPoint3 kFinalTranslate = NiPoint3_ZERO;
    NiQuaternion kFinalRotate = kBlendRotateStart;
    float fFinalScale = 0.0f;
    bool bTransChanged = false;
    bool bRotChanged = false;
    bool bScaleChanged = false;
    bool bFirstRotation = true;
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        InterpArrayItem& kItem = m_pkInterpArray[uc];
        if (!kItem.m_spInterpolator || !(kItem.m_fNormalizedWeight > 0.0f)) {
            continue;
        }
        float fUpdateTime = fTime;
        if (!GetUpdateTimeForItem(fUpdateTime, kItem)) {
            fTotalTransWeight -= kItem.m_fNormalizedWeight;
            fTotalScaleWeight -= kItem.m_fNormalizedWeight;
            continue;
        }
        NiQuatTransform kTransform;
        kTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
        kTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
        kTransform.m_fScale = -FLT_MAX;
        if (!kItem.m_spInterpolator->Update(fUpdateTime, pkInterpTarget, kTransform)) {
            fTotalTransWeight -= kItem.m_fNormalizedWeight;
            fTotalScaleWeight -= kItem.m_fNormalizedWeight;
            continue;
        }

        if (kTransform.IsTranslateValid()) {
            bTransChanged = true;
            float fWeight = kItem.m_fNormalizedWeight;
            float fX = kTransform.m_kTranslate.x * fWeight;
            float fY = kTransform.m_kTranslate.y * fWeight;
            float fZ = kTransform.m_kTranslate.z * fWeight;
            kFinalTranslate.x = fX + kFinalTranslate.x;
            kFinalTranslate.y = kFinalTranslate.y + fY;
            kFinalTranslate.z = kFinalTranslate.z + fZ;
        }
        else {
            fTotalTransWeight -= kItem.m_fNormalizedWeight;
        }

        if (kTransform.IsRotateValid()) {
            NiQuaternion kRotate = kTransform.m_kRotate;
            if (bFirstRotation) {
                bFirstRotation = false;
            }
            else {
                float fDot = kRotate.m_fX * kFinalRotate.m_fX + kRotate.m_fW * kFinalRotate.m_fW +
                    kRotate.m_fY * kFinalRotate.m_fY + kRotate.m_fZ * kFinalRotate.m_fZ;
                if (fDot < 0.0f) {
                    kRotate = -kRotate;
                }
            }
            bRotChanged = true;
            float fWeight = kItem.m_fNormalizedWeight;
            kRotate.m_fW = kRotate.m_fW * fWeight;
            kRotate.m_fX = kRotate.m_fX * fWeight;
            kRotate.m_fY = kRotate.m_fY * fWeight;
            kFinalRotate.m_fW = kRotate.m_fW + kFinalRotate.m_fW;
            kFinalRotate.m_fX = kFinalRotate.m_fX + kRotate.m_fX;
            kFinalRotate.m_fY = kFinalRotate.m_fY + kRotate.m_fY;
            float fZ = fWeight * kRotate.m_fZ;
            kFinalRotate.m_fZ = fZ + kFinalRotate.m_fZ;
        }

        if (kTransform.IsScaleValid()) {
            bScaleChanged = true;
            fFinalScale = kTransform.m_fScale * kItem.m_fNormalizedWeight + fFinalScale;
        }
        else {
            fTotalScaleWeight -= kItem.m_fNormalizedWeight;
        }
    }

    kValue.m_kTranslate.x = -FLT_MAX;
    kValue.m_kRotate.m_fX = -FLT_MAX;
    kValue.m_fScale = -FLT_MAX;
    if (bTransChanged || bRotChanged || bScaleChanged) {
        if (fTotalTransWeight == 0.0f) {
            bTransChanged = false;
        }
        if (fTotalScaleWeight < 0.0001) {
            bScaleChanged = false;
        }

        if (bTransChanged) {
            float fInvTotal = 1.0f / fTotalTransWeight;
            kFinalTranslate.x = fInvTotal * kFinalTranslate.x;
            kValue.m_kTranslate.x = kFinalTranslate.x;
            kFinalTranslate.y = kFinalTranslate.y * fInvTotal;
            kValue.m_kTranslate.y = kFinalTranslate.y;
            kFinalTranslate.z = fInvTotal * kFinalTranslate.z;
            kValue.m_kTranslate.z = kFinalTranslate.z;
        }
        if (bRotChanged) {
            float fLengthSquared = kFinalRotate.m_fX * kFinalRotate.m_fX + kFinalRotate.m_fW * kFinalRotate.m_fW +
                kFinalRotate.m_fY * kFinalRotate.m_fY + kFinalRotate.m_fZ * kFinalRotate.m_fZ;
            float fLength = std::sqrt(fLengthSquared);
            float fInvLength = 1.0f / fLength;
            kFinalRotate.m_fW = fInvLength * kFinalRotate.m_fW;
            kValue.m_kRotate.m_fW = kFinalRotate.m_fW;
            kFinalRotate.m_fX = kFinalRotate.m_fX * fInvLength;
            kValue.m_kRotate.m_fX = kFinalRotate.m_fX;
            kFinalRotate.m_fY = kFinalRotate.m_fY * fInvLength;
            kValue.m_kRotate.m_fY = kFinalRotate.m_fY;
            kFinalRotate.m_fZ = fInvLength * kFinalRotate.m_fZ;
            kValue.m_kRotate.m_fZ = kFinalRotate.m_fZ;
        }
        if (bScaleChanged) {
            kValue.m_fScale = fFinalScale / fTotalScaleWeight;
        }
    }
    return !kValue.IsTransformInvalid();
}

void NiBlendTransformInterpolator::SetAlwaysBlend(bool abTF) {
    bsAlwaysBlend = abTF;
}

NiObject* NiBlendTransformInterpolator::CreateObject() {
    return new NiBlendTransformInterpolator;
}
