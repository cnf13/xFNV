#include "Fallout Shared/FaceGen/BSFaceGenKeyframe.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

#include <cmath>

BSFaceGenKeyframe::BSFaceGenKeyframe(BS_FACEGEN_MORPH_TYPE aeType) : eType(aeType), fTime(0.0f) {}

BSFaceGenKeyframe::~BSFaceGenKeyframe() {}

// False when the keyframe already has aeType.
bool BSFaceGenKeyframe::SetType(BS_FACEGEN_MORPH_TYPE aeType) {
    if (aeType == eType) {
        return false;
    }
    eType = aeType;
    return true;
}

float BSFaceGenKeyframe::GetTime() {
    return fTime;
}

bool BSFaceGenKeyframe::SetTime(float a2) {
    if (fTime == a2) {
        return false;
    }
    fTime = a2;
    return true;
}

bool BSFaceGenKeyframe::IsMultiple() {
    return false;
}

bool BSFaceGenKeyframe::IsExclusive() {
    return false;
}

void BSFaceGenKeyframe::operator delete(void* apMemory) {
    ourFree(apMemory);
}

BSFaceGenKeyframeMultiple::BSFaceGenKeyframeMultiple(BSFaceGenKeyframeMultiple* apSource)
  : BSFaceGenKeyframe(BS_FG_MORPH_NONE), pQuantities(nullptr), iNumQuantities(0) {
    BSFaceGenKeyframe::SetType(apSource->GetType());
    BSFaceGenKeyframe::SetTime(apSource->GetTime());
    BSFaceGenKeyframeMultiple::ResetQuantityArray(apSource->iNumQuantities, true);
    for (uint32_t i = 0; i < iNumQuantities; ++i) {
        SetQuantity(i, apSource->GetQuantity(i));
    }
}

BSFaceGenKeyframeMultiple::BSFaceGenKeyframeMultiple(BS_FACEGEN_MORPH_TYPE aeType, uint32_t aiNumQuantities)
  : BSFaceGenKeyframe(aeType), pQuantities(nullptr), iNumQuantities(0) {
    BSFaceGenKeyframeMultiple::ResetQuantityArray(aiNumQuantities, true);
}

// ResetQuantityArray(0) frees the quantity buffer.
BSFaceGenKeyframeMultiple::~BSFaceGenKeyframeMultiple() {
    BSFaceGenKeyframeMultiple::ResetQuantityArray(0, true);
}

// Appends a copy of this keyframe to apList, unless apList is null, the time is negative, or the list's last keyframe
// is not a multiple keyframe of the same type. A NaN time passes, as the game does.
bool BSFaceGenKeyframeMultiple::AddToList(NiTPointerList<BSFaceGenKeyframe*>* apList) {
    if (!apList) {
        return false;
    }
    if (GetTime() < 0.0) {
        return false;
    }
    NiTPointerListBase<BSFaceGenKeyframe*>* pList = static_cast<NiTPointerListBase<BSFaceGenKeyframe*>*>(apList);
    if (pList->GetSize()) {
        BSFaceGenKeyframe* pTail = pList->GetTail();
        BSFaceGenKeyframe* pLast = (pTail && pTail->IsMultiple()) ? pTail : nullptr;
        if (!pLast) {
            return false;
        }
        BS_FACEGEN_MORPH_TYPE eLastType = pLast->GetType();
        if (eLastType != GetType()) {
            return false;
        }
    }
    void* pMemory = ourMalloc(sizeof(BSFaceGenKeyframeMultiple));
    BSFaceGenKeyframe* pCopy = pMemory ? ::new (pMemory) BSFaceGenKeyframeMultiple(this) : nullptr;
    pList->AddTail(pCopy);
    return true;
}

bool BSFaceGenKeyframeMultiple::MergeMorphDataFrom(BSFaceGenKeyframe* a2, float a3, bool a4, bool a5) {
    float quantity;
    float afPrecision;
    float v10;
    double v11;
    BSFaceGenKeyframeMultiple* v12;
    uint32_t aiWhichMorph;
    float v16;
    if (a2 && a2->IsMultiple()) {
        v12 = static_cast<BSFaceGenKeyframeMultiple*>(a2);
    }
    else {
        v12 = nullptr;
    }
    int8_t v15 = 0;
    if (!v12) {
        return false;
    }
    if (v12 == this) {
        return false;
    }
    int32_t v5 = (int32_t)GetType();
    if (v5 != ((int32_t)v12->GetType())) {
        return false;
    }
    uint32_t uiNumQuantities = GetNumQuantities();
    if (uiNumQuantities != v12->GetNumQuantities()) {
        return false;
    }
    if (!(a3 < 0.0)) {
        if (!(a3 > 1.0)) {
            v16 = a3;
        }
        else {
            v16 = (float)1.0;
        }
    }
    else {
        v16 = (float)0.0;
    }
    for (aiWhichMorph = 0; aiWhichMorph < iNumQuantities; ++aiWhichMorph) {
        if (!a4) {
            quantity = v12->GetQuantity(aiWhichMorph);
            if (FloatEQ(quantity, (float)0.0, 0.0010000000474974513f)) {
                continue;
            }
        }
        if (!(v12->GetQuantity(aiWhichMorph) < 0x1.fffffe0000000p+127)) {
            if (a5) {
                v15 |= ((uint8_t)SetQuantity(aiWhichMorph, 0x1.fffffe0000000p+127f));
            }
        }
        else {
            if ((!(a3 < 1.0)) || (!(GetQuantity(aiWhichMorph) < 0x1.fffffe0000000p+127))) {
                afPrecision = v12->GetQuantity(aiWhichMorph);
                v15 |= ((uint8_t)SetQuantity(aiWhichMorph, afPrecision));
            }
            else {
                v11 = GetQuantity(aiWhichMorph) * (1.0 - v16);
                v10 = (float)((v12->GetQuantity(aiWhichMorph) * v16) + v11);
                v15 |= ((uint8_t)SetQuantity(aiWhichMorph, v10));
            }
        }
    }
    return (bool)v15;
}

bool BSFaceGenKeyframeMultiple::MergeMorphDataFrom(BSFaceGenKeyframe* a1, BSFaceGenKeyframe* a2, float a3) {
    BSFaceGenKeyframeMultiple* v14;
    BSFaceGenKeyframeMultiple* v15;
    float v24;
    if (a1 && a1->IsMultiple()) {
        v15 = static_cast<BSFaceGenKeyframeMultiple*>(a1);
    }
    else {
        v15 = nullptr;
    }
    BSFaceGenKeyframeMultiple* v25 = v15;
    if (a2 && a2->IsMultiple()) {
        v14 = static_cast<BSFaceGenKeyframeMultiple*>(a2);
    }
    else {
        v14 = nullptr;
    }
    BSFaceGenKeyframeMultiple* v22 = v14;
    char v23 = 0;
    char v21 = 0;
    bool v20 = false;
    if (!v15) {
        return false;
    }
    if (!v14) {
        return false;
    }
    if (v15 == v14) {
        return false;
    }
    BS_FACEGEN_MORPH_TYPE v4 = v15->GetType();
    if (v4 != v14->GetType()) {
        return false;
    }
    if (v15 == this) {
        BSFaceGenKeyframeMultiple* v13;
        BSFaceGenKeyframeMultiple* apMemory =
            static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
        if (apMemory) {
            v13 = ::new (apMemory) BSFaceGenKeyframeMultiple(this);
        }
        else {
            v13 = nullptr;
        }
        v25 = v13;
        v23 = 1;
    }
    else {
        if (v14 == this) {
            BSFaceGenKeyframeMultiple* v12;
            BSFaceGenKeyframeMultiple* v17 =
                static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
            if (v17) {
                v12 = ::new (v17) BSFaceGenKeyframeMultiple(this);
            }
            else {
                v12 = nullptr;
            }
            v22 = v12;
            v21 = 1;
        }
    }
    if (!(a3 < 0.0)) {
        if (!(a3 > 1.0)) {
            v24 = a3;
        }
        else {
            v24 = 1.0f;
        }
    }
    else {
        v24 = 0.0f;
    }
    uint32_t Thread = v25->iNumQuantities;
    if (Thread < v22->iNumQuantities) {}
    else {}
    BS_FACEGEN_MORPH_TYPE v7 = v25->GetType();
    SetType(v7);
    for (uint32_t aiWhichMorph = 0; aiWhichMorph < iNumQuantities; ++aiWhichMorph) {
        if (v24 != 0.0 && v22->IsValidQuantity(aiWhichMorph)) {
            if (v24 != 1.0 && v25->IsValidQuantity(aiWhichMorph)) {
                if (v25->IsValidQuantity(aiWhichMorph) && v22->IsValidQuantity(aiWhichMorph)) {
                    double v11 = v25->GetQuantity(aiWhichMorph) * (1.0 - v24);
                    float v10 = v22->GetQuantity(aiWhichMorph) * v24 + v11;
                    v20 = SetQuantity(aiWhichMorph, v10);
                }
            }
            else {
                if (v22->IsValidQuantity(aiWhichMorph)) {
                    float afQuantitya = v22->GetQuantity(aiWhichMorph);
                    v20 = SetQuantity(aiWhichMorph, afQuantitya);
                }
            }
        }
        else {
            if (v25->IsValidQuantity(aiWhichMorph)) {
                float afQuantity = v25->GetQuantity(aiWhichMorph);
                v20 = SetQuantity(aiWhichMorph, afQuantity);
            }
        }
    }
    if ((v23) && (v25)) {
        delete v25;
    }

    if ((v21) && (v22)) {
        delete v22;
    }

    return v20;
}

bool BSFaceGenKeyframeMultiple::ClearData(bool a1) {
    char v6 = GetTime() != 0.0;
    SetTime(0.0f);
    for (uint32_t i = 0; i < iNumQuantities; ++i) {
        if (IsValidQuantity(i)) {
            v6 = 1;
            break;
        }
    }
    uint32_t Thread = iNumQuantities;
    ResetQuantityArray(Thread, a1);
    return v6;
}

BSFaceGenKeyframe* BSFaceGenKeyframeMultiple::CreateCopy() {
    void* v4 = ourMalloc(sizeof(BSFaceGenKeyframeMultiple));
    if (v4) {
        return ::new (v4) BSFaceGenKeyframeMultiple(this);
    }

    return nullptr;
}

void BSFaceGenKeyframeMultiple::CopyFrom(BSFaceGenKeyframe* a1) {
    BSFaceGenKeyframeMultiple* v6;
    if (a1 && a1->IsMultiple()) {
        v6 = static_cast<BSFaceGenKeyframeMultiple*>(a1);
    }
    else {
        v6 = nullptr;
    }
    if (v6) {
        BS_FACEGEN_MORPH_TYPE v2 = v6->GetType();
        SetType(v2);
        float afQuantity = v6->GetTime();
        SetTime(afQuantity);
        uint32_t Thread = v6->iNumQuantities;
        ResetQuantityArray(Thread, true);
        for (uint32_t aiWhichMorph = 0; aiWhichMorph < iNumQuantities; ++aiWhichMorph) {
            float afQuantitya = v6->GetQuantity(aiWhichMorph);
            SetQuantity(aiWhichMorph, afQuantitya);
        }
    }
}

bool BSFaceGenKeyframeMultiple::Compare(BSFaceGenKeyframe* a1) {
    BSFaceGenKeyframeMultiple* v6;
    if (a1 && a1->IsMultiple()) {
        v6 = static_cast<BSFaceGenKeyframeMultiple*>(a1);
    }
    else {
        v6 = nullptr;
    }
    if (!v6) {
        return true;
    }
    BS_FACEGEN_MORPH_TYPE v2 = v6->GetType();
    if (v2 != GetType()) {
        return true;
    }
    uint32_t Thread = v6->iNumQuantities;
    if (Thread != iNumQuantities) {
        return true;
    }
    for (uint32_t aiWhichMorph = 0; aiWhichMorph < iNumQuantities; ++aiWhichMorph) {
        double Quantity = v6->GetQuantity(aiWhichMorph);
        if (Quantity != GetQuantity(aiWhichMorph)) {
            return true;
        }
    }
    return false;
}

float BSFaceGenKeyframeMultiple::GetStrength() {
    uint32_t i;
    float v4 = (float)0.0;
    for (i = 0; i < iNumQuantities; ++i) {
        if (IsValidQuantity(i) && (v4 < ((double)pQuantities[i]))) {
            v4 = pQuantities[i];
        }
    }
    return v4;
}

bool BSFaceGenKeyframeMultiple::IsEmpty() {
    uint32_t i;
    for (i = 0; i < iNumQuantities; ++i) {
        if (IsValidQuantity(i) && (pQuantities[i] > 0.0)) {
            return false;
        }
    }
    return true;
}

void BSFaceGenKeyframeMultiple::SetToZero() {
    BSFaceGenManager::SetToZero(pQuantities, iNumQuantities);
}

bool BSFaceGenKeyframeMultiple::UpdateToTarget(float a1, BSFaceGenKeyframe* a2) {
    BSFaceGenKeyframeMultiple* v12;
    if (a2 && a2->IsMultiple()) {
        v12 = static_cast<BSFaceGenKeyframeMultiple*>(a2);
    }
    else {
        v12 = nullptr;
    }
    char v15 = 0;
    if (a1 <= 0.0) {
        return false;
    }
    if (!v12) {
        return false;
    }
    BS_FACEGEN_MORPH_TYPE v3 = v12->GetType();
    if (v3 != GetType()) {
        return false;
    }
    uint32_t Thread = v12->iNumQuantities;
    if (Thread != iNumQuantities || !Compare(v12)) {
        return false;
    }
    double v11 = BSFaceGenAnimationData::GetExpressionTargetChangePerSec();
    double ExpressionStrengthAddPerSec = BSFaceGenAnimationData::GetExpressionStrengthAddPerSec();
    float v17 = (a2->GetStrength() * ExpressionStrengthAddPerSec + v11) * a1;
    for (uint32_t aiWhichMorph = 0; aiWhichMorph < iNumQuantities; ++aiWhichMorph) {
        if (IsValidQuantity(aiWhichMorph)) {
            double Quantity = v12->GetQuantity(aiWhichMorph);
            float v16 = Quantity - GetQuantity(aiWhichMorph);
            if (!(static_cast<double>(v17) > std::fabsf(v16))) {
                if (!(v16 < 0.0)) {
                    if (v16 > 0.0) {
                        float v7 = GetQuantity(aiWhichMorph) + v17;
                        v15 |= SetQuantity(aiWhichMorph, v7);
                    }
                }
                else {
                    float v8 = GetQuantity(aiWhichMorph) - v17;
                    v15 |= SetQuantity(aiWhichMorph, v8);
                }
            }
            else {
                float X = v12->GetQuantity(aiWhichMorph);
                v15 |= SetQuantity(aiWhichMorph, X);
            }
        }
    }
    return true;
}

bool BSFaceGenKeyframeMultiple::IsTargetAvailable() {
    float v2;
    uint32_t i;
    for (i = 0; i < ((uint32_t)((int32_t)iNumQuantities)); ++i) {
        v2 = pQuantities[i];
        if ((!(v2 < 0.0)) && ((!(v2 > 1.0)) && (v2 > 0.0))) {
            return true;
        }
    }
    return false;
}

bool BSFaceGenKeyframeMultiple::IsMultiple() {
    return true;
}

bool BSFaceGenKeyframeMultiple::IsValidQuantity(uint32_t aiWhichMorph) {
    if (aiWhichMorph >= iNumQuantities) {
        return false;
    }
    return (bool)((uint8_t)((pQuantities[aiWhichMorph] >= 0.0) && (pQuantities[aiWhichMorph] <= 1.0)));
}

void BSFaceGenKeyframeMultiple::ResetQuantityArray(uint32_t aiNumQuantities, bool abForceZero) {
    if (aiNumQuantities != iNumQuantities) {
        if (pQuantities) {
            ourFree(pQuantities);
            pQuantities = nullptr;
        }
        iNumQuantities = aiNumQuantities;
        if (iNumQuantities) {
            pQuantities = static_cast<float*>(
                ourMalloc(((((uint64_t)iNumQuantities) >> 30) != 0) ? ((uint32_t)(-1)) : (4 * iNumQuantities))
            );
            BSFaceGenManager::ResetQuantityArray(pQuantities, iNumQuantities, abForceZero);
        }
    }
    if (iNumQuantities) {
        BSFaceGenManager::ResetQuantityArray(pQuantities, iNumQuantities, abForceZero);
    }
}

float BSFaceGenKeyframeMultiple::GetQuantity(uint32_t aiWhichMorph) const {
    if (aiWhichMorph >= iNumQuantities) {
        return (float)0.0;
    }

    return pQuantities[aiWhichMorph];
}

void BSFaceGenKeyframeMultiple::SetQuantityNoTest(uint32_t aiWhichMorph, float afQuantity) {
    pQuantities[(int32_t)aiWhichMorph] = afQuantity;
}

bool BSFaceGenKeyframeMultiple::SetQuantity(uint32_t aiWhichMorph, float afQuantity) {
    if ((aiWhichMorph >= iNumQuantities) || (afQuantity == pQuantities[aiWhichMorph])) {
        return false;
    }
    pQuantities[aiWhichMorph] = afQuantity;
    return true;
}

uint32_t BSFaceGenKeyframeMultiple::GetNumQuantities() {
    return iNumQuantities;
}

void BSFaceGenKeyframeMultiple::operator=(BSFaceGenKeyframeMultiple* a2) {
    BS_FACEGEN_MORPH_TYPE v2 = GetType();
    if (v2 == a2->GetType()) {
        float afQuantity = a2->GetTime();
        SetTime(afQuantity);
        uint32_t Thread = a2->iNumQuantities;
        ResetQuantityArray(Thread, true);
        for (uint32_t aiWhichMorph = 0; aiWhichMorph < iNumQuantities; ++aiWhichMorph) {
            float afQuantitya = a2->GetQuantity(aiWhichMorph);
            SetQuantity(aiWhichMorph, afQuantitya);
        }
    }
}
