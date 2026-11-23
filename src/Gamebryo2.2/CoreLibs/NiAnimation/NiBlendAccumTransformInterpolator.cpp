#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendAccumTransformInterpolator.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendBoolInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiBlendInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiQuatTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

#include <cstring>
#include <limits>

NiRTTI NiBlendAccumTransformInterpolator::ms_RTTI("NiBlendAccumTransformInterpolator", &NiBlendInterpolator::ms_RTTI);

NiBlendAccumTransformInterpolator::AccumArrayItem::AccumArrayItem() {
    m_kLastValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    m_kLastValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    m_kLastValue.m_fScale = -std::numeric_limits<float>::max();
    m_kDeltaValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    m_kDeltaValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    m_kDeltaValue.m_fScale = -std::numeric_limits<float>::max();
    ClearValues();
}

NiBlendAccumTransformInterpolator::NiBlendAccumTransformInterpolator()
  : NiBlendInterpolator(), m_pkAccumArray(nullptr), m_bReset(false) {
    m_kAccumulatedTransformValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    m_kAccumulatedTransformValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    m_kAccumulatedTransformValue.m_fScale = -std::numeric_limits<float>::max();
}

NiBlendAccumTransformInterpolator::NiBlendAccumTransformInterpolator(
    int32_t abManagerControlled,
    float afWeightThreshold,
    int32_t aucArraySize
)
  : NiBlendInterpolator(abManagerControlled, afWeightThreshold, aucArraySize), m_pkAccumArray(nullptr),
    m_bReset(false) {
    m_kAccumulatedTransformValue.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    m_kAccumulatedTransformValue.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    m_kAccumulatedTransformValue.m_fScale = -std::numeric_limits<float>::max();
    if (m_ucArraySize) {
        m_pkAccumArray = NiTNewInterface<AccumArrayItem>::Allocate(m_ucArraySize);
    }
}

// m_pkAccumArray is MemoryManager storage without a count cookie (the items have no destructor).
NiBlendAccumTransformInterpolator::~NiBlendAccumTransformInterpolator() {
    NiTNewInterface<AccumArrayItem>::Deallocate(m_pkAccumArray);
}

NiObject* NiBlendAccumTransformInterpolator::CreateClone(NiCloningProcess& kCloning) {
    NiBlendAccumTransformInterpolator* v4 = new NiBlendAccumTransformInterpolator;
    CopyMembers(v4, kCloning);
    return v4;
}

void NiBlendAccumTransformInterpolator::LoadBinary(NiStream& kStream) {
    NiBlendInterpolator::LoadBinary(kStream);
    uint32_t uiCount = m_ucArraySize;
    AccumArrayItem* pkAccumArray = new AccumArrayItem[uiCount];
    m_pkAccumArray = pkAccumArray;
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        AccumArrayItem& kItem = m_pkAccumArray[uc];
        kItem.m_kLastValue.LoadBinary(kStream);
        kItem.m_kDeltaValue.LoadBinary(kStream);
        kItem.m_kRefFrame.LoadBinary(kStream);
    }
    m_kAccumulatedTransformValue.LoadBinary(kStream);
}

void NiBlendAccumTransformInterpolator::LinkObject(NiStream& kStream) {
    NiBlendInterpolator::LinkObject(kStream);
}

bool NiBlendAccumTransformInterpolator::RegisterStreamables(NiStream& kStream) {
    return NiBlendInterpolator::RegisterStreamables(kStream);
}

void NiBlendAccumTransformInterpolator::SaveBinary(NiStream& kStream) {
    NiBlendInterpolator::SaveBinary(kStream);
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        AccumArrayItem& kItem = m_pkAccumArray[uc];
        kItem.m_kLastValue.SaveBinary(kStream);
        kItem.m_kDeltaValue.SaveBinary(kStream);
        kItem.m_kRefFrame.SaveBinary(kStream);
    }
    m_kAccumulatedTransformValue.SaveBinary(kStream);
}

bool NiBlendAccumTransformInterpolator::IsEqual(NiObject* pkObject) {
    if (!NiBlendInterpolator::IsEqual(pkObject)) {
        return false;
    }
    NiBlendAccumTransformInterpolator* pkOther = static_cast<NiBlendAccumTransformInterpolator*>(pkObject);
    return m_kAccumulatedTransformValue == pkOther->m_kAccumulatedTransformValue;
}

void NiBlendAccumTransformInterpolator::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
    NiBlendInterpolator::GetViewerStrings(pkStrings);
    pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
    m_kAccumulatedTransformValue.GetViewerStrings(pkStrings);
}

// Every interpolator with a weight samples its motion delta first; the deltas are then blended.
bool NiBlendAccumTransformInterpolator::Update(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
    bool bReturnValue = false;
    if (m_ucInterpCount == 1) {
        bReturnValue = StoreSingleValue(fTime, pkInterpTarget, kValue);
    }
    else if (m_ucInterpCount > 0) {
        ComputeNormalizedWeights();
        for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
            InterpArrayItem& kItem = m_pkInterpArray[uc];
            if (!kItem.m_spInterpolator || 0.0f == kItem.m_fNormalizedWeight) {
                continue;
            }
            float fUpdateTime = fTime;
            if (GetUpdateTimeForItem(fUpdateTime, kItem)) {
                CalculateValue(uc, fUpdateTime, pkInterpTarget);
            }
        }
        bReturnValue = BlendValues(fTime, pkInterpTarget, kValue);
    }
    m_fLastTime = fTime;
    return bReturnValue;
}

// A second interpolator ends the single one's run: its last motion is folded into the accumulated transform and
// its sample state cleared.
uint8_t NiBlendAccumTransformInterpolator::AddInterpInfo(
    NiInterpolator* pkInterpolator,
    float fWeight,
    char cPriority,
    float fEaseSpinner
) {
    uint8_t ucOldSingleIdx = m_ucSingleIdx;
    uint8_t ucIndex = NiBlendInterpolator::AddInterpInfo(pkInterpolator, fWeight, cPriority, fEaseSpinner);
    if (ucIndex == INVALID_INDEX) {
        return ucIndex;
    }

    m_pkAccumArray[ucIndex].ClearValues();
    if (m_ucInterpCount == 1) {
        m_bReset = true;
        return ucIndex;
    }
    if (m_ucInterpCount == 2) {
        if (!m_kAccumulatedTransformValue.IsTransformInvalid()) {
            const NiQuatTransform& kLastValue = m_pkAccumArray[ucOldSingleIdx].m_kLastValue;
            if (!kLastValue.IsTransformInvalid()) {
                m_kAccumulatedTransformValue = m_kAccumulatedTransformValue.HierApply(kLastValue);
            }
        }
        m_pkAccumArray[ucOldSingleIdx].ClearValues();
    }
    return ucIndex;
}

// Removing the single interpolator folds its last motion into the accumulated transform; the one left single
// afterwards starts over.
void NiBlendAccumTransformInterpolator::RemoveInterpInfo(uint8_t ucIndex) {
    if (m_ucInterpCount == 1 && ucIndex == m_ucSingleIdx && !m_kAccumulatedTransformValue.IsTransformInvalid()) {
        const NiQuatTransform& kLastValue = m_pkAccumArray[m_ucSingleIdx].m_kLastValue;
        if (!kLastValue.IsTransformInvalid()) {
            m_kAccumulatedTransformValue = m_kAccumulatedTransformValue.HierApply(kLastValue);
        }
    }
    bool bHadInterpolator = m_pkInterpArray[ucIndex].m_spInterpolator != nullptr;
    NiBlendInterpolator::RemoveInterpInfo(ucIndex);
    if (m_ucInterpCount == 1 && bHadInterpolator) {
        m_bReset = true;
        m_pkAccumArray[m_ucSingleIdx].ClearValues();
    }
}

bool NiBlendAccumTransformInterpolator::InterpolatorIsCorrectType(NiInterpolator* pkInterpolator) const {
    return pkInterpolator->IsTransformValueSupported();
}

// The accumulation array grows with the interpolator array. The old array is freed even when null.
bool NiBlendAccumTransformInterpolator::ReallocateArrays() {
    uint8_t ucOldArraySize = m_ucArraySize;
    bool bReallocated = NiBlendInterpolator::ReallocateArrays();
    if (bReallocated) {
        uint32_t uiCount = m_ucArraySize;
        AccumArrayItem* pkNewAccumArray = new AccumArrayItem[uiCount];
        for (uint8_t uc = 0; uc < ucOldArraySize; uc++) {
            pkNewAccumArray[uc] = m_pkAccumArray[uc];
        }
        MemoryManager::Instance().Deallocate(m_pkAccumArray);
        m_pkAccumArray = pkNewAccumArray;
    }
    return bReallocated;
}

// With a single interpolator its last motion is applied on top of the accumulated transform.
bool NiBlendAccumTransformInterpolator::GetValue(NiQuatTransform& kValue) const {
    if (m_kAccumulatedTransformValue.IsTransformInvalid()) {
        return false;
    }
    if (m_ucInterpCount == 1) {
        kValue = m_kAccumulatedTransformValue.HierApply(m_pkAccumArray[m_ucSingleIdx].m_kLastValue);
    }
    else {
        kValue = m_kAccumulatedTransformValue;
    }
    return true;
}

void NiBlendAccumTransformInterpolator::ClearAccumulatedTransform() {
    if (m_ucInterpCount == 1) {
        NiQuatTransform v2;
        std::memcpy(&m_kAccumulatedTransformValue, &v2, sizeof(m_kAccumulatedTransformValue));
    }
    else {
        m_kAccumulatedTransformValue.SetTranslate(NiPoint3::ZERO);
        m_kAccumulatedTransformValue.SetRotate(NiQuaternion::IDENTITY);
        m_kAccumulatedTransformValue.SetScale(1.0);
    }
    m_bReset = true;
}

bool NiBlendAccumTransformInterpolator::StoreSingleValue(
    float fTime,
    NiObjectNET* pkInterpTarget,
    NiQuatTransform& kValue
) {
    NiQuatTransform* p_m_kAccumulatedTransformValue;
    NiQuatTransform arTransform;
    NiQuatTransform result;
    if ((m_uFlags & MANAGER_CONTROLLED_MASK) != 0) {
        fTime = m_fSingleTime;
    }
    if (-std::numeric_limits<float>::max() == fTime) {
        return false;
    }
    CalculateSingleValue(fTime, pkInterpTarget);
    double v6 = -3.4028234663852886e+38;
    if (m_bReset ||
        ((p_m_kAccumulatedTransformValue = &m_kAccumulatedTransformValue,
          m_kAccumulatedTransformValue.m_fScale == -3.4028234663852886e+38) &&
         (m_kAccumulatedTransformValue.m_kRotate.m_fX == -3.4028234663852886e+38 &&
          p_m_kAccumulatedTransformValue->m_kTranslate.x == -3.4028234663852886e+38))) {
        NiQuatTransform* p_arTransform;
        arTransform.m_fScale = -std::numeric_limits<float>::max();
        arTransform.m_kTranslate.z = NiQuatTransform::INVALID_TRANSLATE.z;
        arTransform.m_kRotate.m_fY = NiQuatTransform::INVALID_ROTATE.m_fY;
        int32_t m_ucSingleIdx = this->m_ucSingleIdx;
        arTransform.m_kTranslate.x = NiQuatTransform::INVALID_TRANSLATE.x;
        arTransform.m_kRotate.m_fW = NiQuatTransform::INVALID_ROTATE.m_fW;
        arTransform.m_kTranslate.y = NiQuatTransform::INVALID_TRANSLATE.y;
        arTransform.m_kRotate.m_fZ = NiQuatTransform::INVALID_ROTATE.m_fZ;
        NiBlendAccumTransformInterpolator::AccumArrayItem* m_pkAccumArray = this->m_pkAccumArray;
        arTransform.m_kRotate.m_fX = NiQuatTransform::INVALID_ROTATE.m_fX;
        m_pkAccumArray[m_ucSingleIdx].m_kLastValue.HierInvert(arTransform);
        p_m_kAccumulatedTransformValue = &m_kAccumulatedTransformValue;
        v6 = -3.4028234663852886e+38;
        if (-3.4028234663852886e+38 == m_kAccumulatedTransformValue.m_fScale &&
            (m_kAccumulatedTransformValue.m_kRotate.m_fX == -3.4028234663852886e+38 &&
             p_m_kAccumulatedTransformValue->m_kTranslate.x == -3.4028234663852886e+38)) {
            p_arTransform = &arTransform;
        }
        else {
            v6 = -3.4028234663852886e+38;
            p_arTransform = &(result = m_kAccumulatedTransformValue.HierApply(arTransform));
        }
        *p_m_kAccumulatedTransformValue = *p_arTransform;
        m_bReset = false;
    }
    if (p_m_kAccumulatedTransformValue->m_fScale == v6 &&
        (p_m_kAccumulatedTransformValue->m_kRotate.m_fX == v6 &&
         p_m_kAccumulatedTransformValue->m_kTranslate.x == v6)) {
        return false;
    }
    kValue =
        (result = p_m_kAccumulatedTransformValue->HierApply(this->m_pkAccumArray[this->m_ucSingleIdx].m_kLastValue));
    return true;
}

bool NiBlendAccumTransformInterpolator::BlendValues(float fTime, NiObjectNET* pkInterpTarget, NiQuatTransform& kValue) {
    char v22;
    NiQuaternion kFinalRotate;
    NiQuatTransform kFinalTransform;
    NiQuatTransform v41;
    float v36 = 1.0f;
    float v34 = 1.0f;
    NiPoint3 v31 = NiPoint3::ZERO;
    kFinalRotate.SetValues(0.0f, 0.0f, 0.0f, 0.0f);
    float v35 = 0.0f;
    NiQuatTransform* p_m_kAccumulatedTransformValue = &m_kAccumulatedTransformValue;
    char v20 = 0;
    char v18 = 0;
    bool v6 = -3.4028234663852886e+38 == m_kAccumulatedTransformValue.m_fScale;
    char v19 = 0;
    NiQuatTransform* v32 = &m_kAccumulatedTransformValue;
    double v7 = -3.4028234663852886e+38;
    if (v6 &&
        (m_kAccumulatedTransformValue.m_kRotate.m_fX == -std::numeric_limits<float>::max() &&
         p_m_kAccumulatedTransformValue->m_kTranslate.x == -std::numeric_limits<float>::max())) {
        p_m_kAccumulatedTransformValue->m_kTranslate.x = NiPoint3::ZERO.x;
        m_kAccumulatedTransformValue.m_kTranslate.y = NiPoint3::ZERO.y;
        m_kAccumulatedTransformValue.m_kTranslate.z = NiPoint3::ZERO.z;
        m_kAccumulatedTransformValue.m_kRotate = NiQuaternion::IDENTITY;
        m_kAccumulatedTransformValue.m_fScale = 1.0f;
        v22 = 1;
    }
    else {
        v22 = 0;
    }
    uint8_t uc_2 = 0;
    char v21 = 1;
    uint8_t uc = 0;
    if (!m_ucArraySize) {
        return false;
    }
    do {
        NiBlendInterpolator::InterpArrayItem* kItem = &m_pkInterpArray[uc_2];
        NiBlendAccumTransformInterpolator::AccumArrayItem* kAccumItem = &m_pkAccumArray[uc_2];
        if (kItem->m_spInterpolator.m_pObject && (kItem->m_fNormalizedWeight > 1e-05f)) {
            NiQuatTransform* p_m_kLastValue = &kAccumItem->m_kLastValue;
            if (!v22) {
                p_m_kLastValue = &kAccumItem->m_kDeltaValue;
            }
            v41 = *p_m_kLastValue;
            if (v41.m_kTranslate.x == v7) {
                v36 = v36 - kItem->m_fNormalizedWeight;
            }
            else {
                v20 = 1;
                float m_fNormalizedWeight = kItem->m_fNormalizedWeight;
                float v38 = v41.m_kTranslate.x * m_fNormalizedWeight;
                float v39 = v41.m_kTranslate.y * m_fNormalizedWeight;
                float v40 = m_fNormalizedWeight * v41.m_kTranslate.z;
                v31.x = v38 + v31.x;
                v31.y = v31.y + v39;
                v31.z = v31.z + v40;
            }
            if (v41.m_kRotate.m_fX != v7) {
                NiQuaternion kRotValue = v41.m_kRotate;
                if (v21) {
                    v21 = 0;
                }
                else {
                    float v25 = v41.m_kRotate.m_fX * kFinalRotate.m_fX + v41.m_kRotate.m_fW * kFinalRotate.m_fW +
                        v41.m_kRotate.m_fY * kFinalRotate.m_fY + v41.m_kRotate.m_fZ * kFinalRotate.m_fZ;
                    if (v25 < 0.0f) {
                        kRotValue = -kRotValue;
                    }
                }
                NiQuaternion kWeighted = kRotValue * kItem->m_fNormalizedWeight;
                kRotValue.m_fW = kWeighted.m_fW;
                kRotValue.m_fX = kWeighted.m_fX;
                kRotValue.m_fY = kWeighted.m_fY;
                float m_fZ = kWeighted.m_fZ;
                kFinalRotate.m_fW = kRotValue.m_fW + kFinalRotate.m_fW;
                kRotValue.m_fZ = m_fZ;
                uc_2 = uc;
                v18 = 1;
                kFinalRotate.m_fX = kFinalRotate.m_fX + kRotValue.m_fX;
                kFinalRotate.m_fY = kFinalRotate.m_fY + kRotValue.m_fY;
                kFinalRotate.m_fZ = kFinalRotate.m_fZ + kRotValue.m_fZ;
                v7 = -3.4028234663852886e+38;
            }
            p_m_kAccumulatedTransformValue = v32;
            if (v41.m_fScale == v7) {
                v34 = v34 - kItem->m_fNormalizedWeight;
            }
            else {
                v19 = 1;
                v35 = v41.m_fScale * kItem->m_fNormalizedWeight + v35;
            }
        }

        uc = ++uc_2;
    }
    while (uc_2 < m_ucArraySize);
    if (!v20 && (!v18 && !v19)) {
        return false;
    }
    kFinalTransform.m_fScale = -std::numeric_limits<float>::max();
    double v14 = 1.0;
    kFinalTransform.m_kTranslate = NiQuatTransform::INVALID_TRANSLATE;
    kFinalTransform.m_kRotate = NiQuatTransform::INVALID_ROTATE;
    if (v20) {
        float v26 = 1.0 / v36;
        v31.x = v26 * v31.x;
        v31.y = v31.y * v26;
        v31.z = v26 * v31.z;
        kFinalTransform.m_kTranslate = v31;
    }
    if (v18) {
        float v27 = kFinalRotate.m_fX * kFinalRotate.m_fX + kFinalRotate.m_fW * kFinalRotate.m_fW +
            kFinalRotate.m_fY * kFinalRotate.m_fY + kFinalRotate.m_fZ * kFinalRotate.m_fZ;
        float v28 = std::sqrt(static_cast<double>(v27));
        p_m_kAccumulatedTransformValue = v32;
        v14 = 1.0;
        float v29 = 1.0 / v28;
        kFinalRotate.m_fW = v29 * kFinalRotate.m_fW;
        kFinalRotate.m_fX = kFinalRotate.m_fX * v29;
        kFinalRotate.m_fY = kFinalRotate.m_fY * v29;
        kFinalRotate.m_fZ = v29 * kFinalRotate.m_fZ;
        kFinalTransform.m_kRotate = kFinalRotate;
    }
    if (v19 && (v34 > 1e-05f)) {
        kFinalTransform.m_fScale = v14 / v34 * v35;
    }

    NiQuatTransform* v16 = ::new (&v41) NiQuatTransform(*p_m_kAccumulatedTransformValue * kFinalTransform);
    *v32 = *v16;
    if (-3.4028234663852886e+38 == v32->m_fScale &&
        (v32->m_kRotate.m_fX == -std::numeric_limits<float>::max() &&
         v32->m_kTranslate.x == -std::numeric_limits<float>::max())) {
        return false;
    }
    kValue = *v32;
    return true;
}

void NiBlendAccumTransformInterpolator::CalculateValue(uint8_t ucIndex, float fTime, NiObjectNET* pkInterpTarget) {
    NiQuatTransform* p_m_kLastValue;
    NiQuaternion kAccumRotate;
    NiQuaternion kAccumRot;
    NiQuaternion kRefFrame__kInvLast__kInvBegin;
    NiQuatTransform kDeltaTransform;
    NiQuatTransform result;
    double v4 = fTime;
    NiBlendAccumTransformInterpolator::AccumArrayItem* kAccumItem = &m_pkAccumArray[ucIndex];
    NiInterpolator* pkInterpolator = m_pkInterpArray[ucIndex].m_spInterpolator.m_pObject;
    if (!pkInterpolator || v4 == kAccumItem->m_fLastTime) {
        kAccumItem->m_kDeltaValue.m_kTranslate = NiPoint3::ZERO;
        kAccumItem->m_kDeltaValue.m_kRotate = NiQuaternion::IDENTITY;
        kAccumItem->m_kDeltaValue.m_fScale = 1.0f;
        kAccumItem->m_fLastTime = fTime;
        return;
    }
    p_m_kLastValue = &kAccumItem->m_kLastValue;
    if (m_bReset || kAccumItem->m_kLastValue.IsTransformInvalid()) {
        v4 = fTime;
        pkInterpolator->Update(v4, pkInterpTarget, kAccumItem->m_kLastValue);
        kAccumRot.m_fY = NiQuaternion::IDENTITY.m_fY;
        kAccumRot.m_fZ = NiQuaternion::IDENTITY.m_fZ;
        double m_fX = m_kAccumulatedTransformValue.m_kRotate.m_fX;
        kAccumRot.m_fW = NiQuaternion::IDENTITY.m_fW;
        kAccumRot.m_fX = NiQuaternion::IDENTITY.m_fX;
        if (-3.4028234663852886e+38 != m_fX) {
            kAccumRot = m_kAccumulatedTransformValue.m_kRotate;
        }
        double v9 = kAccumItem->m_kLastValue.m_kRotate.m_fX;
        kAccumRotate = NiQuaternion::IDENTITY;
        if (v9 != -3.4028234663852886e+38) {
            kAccumRotate = NiQuaternion::UnitInverse(kAccumItem->m_kLastValue.m_kRotate);
        }
        ::new (&kRefFrame__kInvLast__kInvBegin) NiQuaternion(kAccumRot * kAccumRotate);
        kRefFrame__kInvLast__kInvBegin.ToRotation(kAccumItem->m_kRefFrame);
    }
    NiQuatTransform kNowTransform;
    if (!(kAccumItem->m_fLastTime > static_cast<double>(fTime)) || m_bReset) {
        pkInterpolator->Update(fTime, pkInterpTarget, kNowTransform);
        NiQuatTransform kInvLast__kEndTransform;
        p_m_kLastValue->Invert(kInvLast__kEndTransform);
        kAccumItem->m_kDeltaValue = *::new (&kDeltaTransform) NiQuatTransform(kInvLast__kEndTransform * kNowTransform);
        if (kAccumItem->m_kDeltaValue.m_kTranslate.x != -3.4028234663852886e+38) {
            kAccumItem->m_kDeltaValue.m_kTranslate = kAccumItem->m_kRefFrame * kAccumItem->m_kDeltaValue.m_kTranslate;
        }
    }
    else {
        float fBeginTime;
        float fEndTime;
        pkInterpolator->GetActiveTimeRange(fBeginTime, fEndTime);
        NiQuatTransform kInvLast__kEndTransform;
        pkInterpolator->Update(fEndTime, pkInterpTarget, kInvLast__kEndTransform);
        NiQuatTransform kBeginTransform;
        pkInterpolator->Update(fBeginTime, pkInterpTarget, kBeginTransform);
        pkInterpolator->Update(fTime, pkInterpTarget, kNowTransform);
        NiQuatTransform v29;
        p_m_kLastValue->Invert(v29);
        kAccumItem->m_kDeltaValue = *::new (&result) NiQuatTransform(v29 * kInvLast__kEndTransform);
        double v14 = -3.4028234663852886e+38;
        if (-3.4028234663852886e+38 != kAccumItem->m_kDeltaValue.m_kTranslate.x) {
            v14 = -3.4028234663852886e+38;
            kAccumItem->m_kDeltaValue.m_kTranslate = kAccumItem->m_kRefFrame * kAccumItem->m_kDeltaValue.m_kTranslate;
        }
        kAccumRotate.m_fZ = NiQuaternion::IDENTITY.m_fZ;
        double v15 = m_kAccumulatedTransformValue.m_kRotate.m_fX;
        kAccumRotate.m_fW = NiQuaternion::IDENTITY.m_fW;
        kAccumRotate.m_fX = NiQuaternion::IDENTITY.m_fX;
        kAccumRotate.m_fY = NiQuaternion::IDENTITY.m_fY;
        if (v15 != v14) {
            kAccumRotate.m_fW = m_kAccumulatedTransformValue.m_kRotate.m_fW;
            kAccumRotate.m_fX = m_kAccumulatedTransformValue.m_kRotate.m_fX;
            float v16 = m_kAccumulatedTransformValue.m_kRotate.m_fZ;
            kAccumRotate.m_fY = m_kAccumulatedTransformValue.m_kRotate.m_fY;
            kAccumRotate.m_fZ = v16;
        }
        kAccumRot = NiQuaternion::IDENTITY;
        if (kBeginTransform.m_kRotate.m_fX != v14) {
            kAccumRot = NiQuaternion::UnitInverse(kBeginTransform.m_kRotate);
        }
        ::new (&kRefFrame__kInvLast__kInvBegin) NiQuaternion(kAccumRotate * kAccumRot);
        kRefFrame__kInvLast__kInvBegin.ToRotation(kAccumItem->m_kRefFrame);
        kBeginTransform.Invert(v29);
        *p_m_kLastValue = *::new (&result) NiQuatTransform(v29 * kNowTransform);
        if (p_m_kLastValue->m_kTranslate.x != -3.4028234663852886e+38) {
            p_m_kLastValue->m_kTranslate = kAccumItem->m_kRefFrame * p_m_kLastValue->m_kTranslate;
        }
        kAccumItem->m_kDeltaValue = *::new (&result) NiQuatTransform(kAccumItem->m_kDeltaValue * *p_m_kLastValue);
    }
    *p_m_kLastValue = kNowTransform;
    m_bReset = false;
    kAccumItem->m_fLastTime = fTime;
}

// m_bReset is not copied.
void NiBlendAccumTransformInterpolator::CopyMembers(
    NiBlendAccumTransformInterpolator* pkDest,
    NiCloningProcess& kCloning
) {
    NiBlendInterpolator::CopyMembers(pkDest, kCloning);
    uint32_t uiCount = m_ucArraySize;
    AccumArrayItem* pkAccumArray = new AccumArrayItem[uiCount];
    pkDest->m_pkAccumArray = pkAccumArray;
    for (uint8_t uc = 0; uc < m_ucArraySize; uc++) {
        pkDest->m_pkAccumArray[uc] = m_pkAccumArray[uc];
    }
    pkDest->m_kAccumulatedTransformValue = m_kAccumulatedTransformValue;
}

void NiBlendAccumTransformInterpolator::CalculateSingleValue(float fTime, NiObjectNET* pkInterpTarget) {
    NiBlendAccumTransformInterpolator* pThis = this;
    NiQuatTransform result;
    int32_t m_ucSingleIdx = pThis->m_ucSingleIdx;
    NiInterpolator* m_pObject = pThis->m_pkInterpArray[m_ucSingleIdx].m_spInterpolator.m_pObject;
    NiBlendAccumTransformInterpolator::AccumArrayItem* v5 = &pThis->m_pkAccumArray[m_ucSingleIdx];
    NiBlendAccumTransformInterpolator* v11 = pThis;
    double v6 = fTime;
    if (fTime != v5->m_fLastTime) {
        if (v5->m_fLastTime > v6) {
            NiQuatTransform* p_m_kDeltaValue = &v5->m_kDeltaValue;
            double v8 = -3.4028234663852886e+38;
            if (-3.4028234663852886e+38 == v5->m_kDeltaValue.m_fScale &&
                v5->m_kDeltaValue.m_kRotate.m_fX == -3.4028234663852886e+38 &&
                p_m_kDeltaValue->m_kTranslate.x == -3.4028234663852886e+38) {
                float v12;
                float v13;
                m_pObject->GetActiveTimeRange(v13, v12);
                NiQuatTransform apMemory;
                m_pObject->Update(v13, pkInterpTarget, apMemory);
                NiQuatTransform v15;
                m_pObject->Update(v12, pkInterpTarget, v15);
                NiQuatTransform arTransform;
                apMemory.HierInvert(arTransform);
                v6 = fTime;
                v8 = -3.4028234663852886e+38;
                *p_m_kDeltaValue = (result = v15.HierApply(arTransform));
                pThis = v11;
            }
            NiQuatTransform* p_m_kAccumulatedTransformValue = &pThis->m_kAccumulatedTransformValue;
            if (pThis->m_kAccumulatedTransformValue.m_fScale != v8 ||
                (pThis->m_kAccumulatedTransformValue.m_kRotate.m_fX != v8 ||
                 p_m_kAccumulatedTransformValue->m_kTranslate.x != v8)) {
                v6 = fTime;
                *p_m_kAccumulatedTransformValue =
                    (result = pThis->m_kAccumulatedTransformValue.HierApply(*p_m_kDeltaValue));
            }
        }
        float v10 = v6;
        m_pObject->Update(v10, pkInterpTarget, v5->m_kLastValue);
        v6 = fTime;
    }
    v5->m_fLastTime = v6;
}

NiObject* NiBlendAccumTransformInterpolator::CreateObject() {
    return new NiBlendAccumTransformInterpolator;
}

// Each component times fScalar, rounded to float, set through SetValues.
NiQuaternion NiQuaternion::operator*(float fScalar) const {
    NiQuaternion kResult;
    kResult.SetValues(m_fW * fScalar, m_fX * fScalar, m_fY * fScalar, m_fZ * fScalar);
    return kResult;
}
