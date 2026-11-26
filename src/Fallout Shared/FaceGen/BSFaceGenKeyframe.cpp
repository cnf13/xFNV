#include "Fallout Shared/FaceGen/BSFaceGenKeyframe.h"

#include "BSCore/NewOverloads.h"
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

bool BSFaceGenKeyframe::SetTime(float afTime) {
    if (fTime == afTime) {
        return false;
    }
    fTime = afTime;
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
// is not a multiple keyframe of the same type. A NaN time passes.
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
    BSFaceGenKeyframe* pCopy = new BSFaceGenKeyframeMultiple(this);
    pList->AddTail(pCopy);
    return true;
}

bool BSFaceGenKeyframeMultiple::MergeMorphDataFrom(
    BSFaceGenKeyframe* apSource,
    float afBlend,
    bool abMergeZeroValues,
    bool abClearDestValueIfNoUpdate
) {
    BSFaceGenKeyframeMultiple* pSource = nullptr;
    if (apSource && apSource->IsMultiple()) {
        pSource = static_cast<BSFaceGenKeyframeMultiple*>(apSource);
    }
    int8_t cChanged = 0;
    if (!pSource) {
        return false;
    }
    if (pSource == this) {
        return false;
    }
    int32_t iType = GetType();
    if (iType != pSource->GetType()) {
        return false;
    }
    uint32_t uiNumQuantities = GetNumQuantities();
    if (uiNumQuantities != pSource->GetNumQuantities()) {
        return false;
    }
    float fBlend;
    if (afBlend < 0.0) {
        fBlend = 0.0f;
    }
    else if (afBlend > 1.0) {
        fBlend = 1.0f;
    }
    else {
        fBlend = afBlend;
    }
    for (uint32_t uiWhichMorph = 0; uiWhichMorph < iNumQuantities; ++uiWhichMorph) {
        if (!abMergeZeroValues) {
            float fQuantity = pSource->GetQuantity(uiWhichMorph);
            if (FloatEQ(fQuantity, 0.0f, 0.0010000000474974513f)) {
                continue;
            }
        }
        if (!(pSource->GetQuantity(uiWhichMorph) < 0x1.fffffe0000000p+127)) {
            if (abClearDestValueIfNoUpdate) {
                cChanged |= SetQuantity(uiWhichMorph, 0x1.fffffe0000000p+127f);
            }
        }
        else if (!(afBlend < 1.0) || !(GetQuantity(uiWhichMorph) < 0x1.fffffe0000000p+127)) {
            float fQuantity = pSource->GetQuantity(uiWhichMorph);
            cChanged |= SetQuantity(uiWhichMorph, fQuantity);
        }
        else {
            double dCurrentPart = GetQuantity(uiWhichMorph) * (1.0 - fBlend);
            float fMerged = static_cast<float>(pSource->GetQuantity(uiWhichMorph) * fBlend + dCurrentPart);
            cChanged |= SetQuantity(uiWhichMorph, fMerged);
        }
    }
    return cChanged;
}

bool BSFaceGenKeyframeMultiple::MergeMorphDataFrom(BSFaceGenKeyframe* apFrom, BSFaceGenKeyframe* apTo, float afBlend) {
    BSFaceGenKeyframeMultiple* pFrom = nullptr;
    if (apFrom && apFrom->IsMultiple()) {
        pFrom = static_cast<BSFaceGenKeyframeMultiple*>(apFrom);
    }
    BSFaceGenKeyframeMultiple* pMergeFrom = pFrom;
    BSFaceGenKeyframeMultiple* pTo = nullptr;
    if (apTo && apTo->IsMultiple()) {
        pTo = static_cast<BSFaceGenKeyframeMultiple*>(apTo);
    }
    BSFaceGenKeyframeMultiple* pMergeTo = pTo;
    char cDeleteFrom = 0;
    char cDeleteTo = 0;
    bool bChanged = false;
    if (!pFrom) {
        return false;
    }
    if (!pTo) {
        return false;
    }
    if (pFrom == pTo) {
        return false;
    }
    BS_FACEGEN_MORPH_TYPE eFromType = pFrom->GetType();
    if (eFromType != pTo->GetType()) {
        return false;
    }
    if (pFrom == this) {
        pMergeFrom = new BSFaceGenKeyframeMultiple(this);
        cDeleteFrom = 1;
    }
    else if (pTo == this) {
        pMergeTo = new BSFaceGenKeyframeMultiple(this);
        cDeleteTo = 1;
    }
    float fBlend;
    if (afBlend < 0.0) {
        fBlend = 0.0f;
    }
    else if (afBlend > 1.0) {
        fBlend = 1.0f;
    }
    else {
        fBlend = afBlend;
    }
    uint32_t uiNumQuantities = pMergeFrom->iNumQuantities;
    if (uiNumQuantities < pMergeTo->iNumQuantities) {}
    else {}
    BS_FACEGEN_MORPH_TYPE eType = pMergeFrom->GetType();
    SetType(eType);
    for (uint32_t uiWhichMorph = 0; uiWhichMorph < iNumQuantities; ++uiWhichMorph) {
        if (fBlend != 0.0 && pMergeTo->IsValidQuantity(uiWhichMorph)) {
            if (fBlend != 1.0 && pMergeFrom->IsValidQuantity(uiWhichMorph)) {
                if (pMergeFrom->IsValidQuantity(uiWhichMorph) && pMergeTo->IsValidQuantity(uiWhichMorph)) {
                    double dFromPart = pMergeFrom->GetQuantity(uiWhichMorph) * (1.0 - fBlend);
                    float fMerged = pMergeTo->GetQuantity(uiWhichMorph) * fBlend + dFromPart;
                    bChanged = SetQuantity(uiWhichMorph, fMerged);
                }
            }
            else if (pMergeTo->IsValidQuantity(uiWhichMorph)) {
                float fQuantity = pMergeTo->GetQuantity(uiWhichMorph);
                bChanged = SetQuantity(uiWhichMorph, fQuantity);
            }
        }
        else if (pMergeFrom->IsValidQuantity(uiWhichMorph)) {
            float fQuantity = pMergeFrom->GetQuantity(uiWhichMorph);
            bChanged = SetQuantity(uiWhichMorph, fQuantity);
        }
    }
    if (cDeleteFrom && pMergeFrom) {
        delete pMergeFrom;
    }

    if (cDeleteTo && pMergeTo) {
        delete pMergeTo;
    }

    return bChanged;
}

bool BSFaceGenKeyframeMultiple::ClearData(bool abForceZero) {
    char cChanged = GetTime() != 0.0;
    SetTime(0.0f);
    for (uint32_t i = 0; i < iNumQuantities; ++i) {
        if (IsValidQuantity(i)) {
            cChanged = 1;
            break;
        }
    }
    ResetQuantityArray(iNumQuantities, abForceZero);
    return cChanged;
}

BSFaceGenKeyframe* BSFaceGenKeyframeMultiple::CreateCopy() {
    return new BSFaceGenKeyframeMultiple(this);
}

void BSFaceGenKeyframeMultiple::CopyFrom(BSFaceGenKeyframe* apSource) {
    BSFaceGenKeyframeMultiple* pSource = nullptr;
    if (apSource && apSource->IsMultiple()) {
        pSource = static_cast<BSFaceGenKeyframeMultiple*>(apSource);
    }
    if (pSource) {
        BS_FACEGEN_MORPH_TYPE eType = pSource->GetType();
        SetType(eType);
        float fTime = pSource->GetTime();
        SetTime(fTime);
        ResetQuantityArray(pSource->iNumQuantities, true);
        for (uint32_t uiWhichMorph = 0; uiWhichMorph < iNumQuantities; ++uiWhichMorph) {
            float fQuantity = pSource->GetQuantity(uiWhichMorph);
            SetQuantity(uiWhichMorph, fQuantity);
        }
    }
}

bool BSFaceGenKeyframeMultiple::Compare(BSFaceGenKeyframe* apOther) {
    BSFaceGenKeyframeMultiple* pOther = nullptr;
    if (apOther && apOther->IsMultiple()) {
        pOther = static_cast<BSFaceGenKeyframeMultiple*>(apOther);
    }
    if (!pOther) {
        return true;
    }
    BS_FACEGEN_MORPH_TYPE eType = pOther->GetType();
    if (eType != GetType()) {
        return true;
    }
    if (pOther->iNumQuantities != iNumQuantities) {
        return true;
    }
    for (uint32_t uiWhichMorph = 0; uiWhichMorph < iNumQuantities; ++uiWhichMorph) {
        double dQuantity = pOther->GetQuantity(uiWhichMorph);
        if (dQuantity != GetQuantity(uiWhichMorph)) {
            return true;
        }
    }
    return false;
}

float BSFaceGenKeyframeMultiple::GetStrength() {
    float fStrength = 0.0f;
    for (uint32_t i = 0; i < iNumQuantities; ++i) {
        if (IsValidQuantity(i) && fStrength < static_cast<double>(pQuantities[i])) {
            fStrength = pQuantities[i];
        }
    }
    return fStrength;
}

bool BSFaceGenKeyframeMultiple::IsEmpty() {
    for (uint32_t i = 0; i < iNumQuantities; ++i) {
        if (IsValidQuantity(i) && pQuantities[i] > 0.0) {
            return false;
        }
    }
    return true;
}

void BSFaceGenKeyframeMultiple::SetToZero() {
    BSFaceGenManager::SetToZero(pQuantities, iNumQuantities);
}

bool BSFaceGenKeyframeMultiple::UpdateToTarget(float afTimeDelta, BSFaceGenKeyframe* apTarget) {
    BSFaceGenKeyframeMultiple* pTarget = nullptr;
    if (apTarget && apTarget->IsMultiple()) {
        pTarget = static_cast<BSFaceGenKeyframeMultiple*>(apTarget);
    }
    if (afTimeDelta <= 0.0) {
        return false;
    }
    if (!pTarget) {
        return false;
    }
    BS_FACEGEN_MORPH_TYPE eType = pTarget->GetType();
    if (eType != GetType()) {
        return false;
    }
    if (pTarget->iNumQuantities != iNumQuantities || !Compare(pTarget)) {
        return false;
    }
    double dChangePerSec = BSFaceGenAnimationData::GetExpressionTargetChangePerSec();
    double dStrengthAddPerSec = BSFaceGenAnimationData::GetExpressionStrengthAddPerSec();
    float fMaxChange = (apTarget->GetStrength() * dStrengthAddPerSec + dChangePerSec) * afTimeDelta;
    for (uint32_t uiWhichMorph = 0; uiWhichMorph < iNumQuantities; ++uiWhichMorph) {
        if (IsValidQuantity(uiWhichMorph)) {
            double dTarget = pTarget->GetQuantity(uiWhichMorph);
            float fDelta = dTarget - GetQuantity(uiWhichMorph);
            if (static_cast<double>(fMaxChange) > std::fabsf(fDelta)) {
                float fQuantity = pTarget->GetQuantity(uiWhichMorph);
                SetQuantity(uiWhichMorph, fQuantity);
            }
            else if (fDelta < 0.0) {
                float fQuantity = GetQuantity(uiWhichMorph) - fMaxChange;
                SetQuantity(uiWhichMorph, fQuantity);
            }
            else if (fDelta > 0.0) {
                float fQuantity = GetQuantity(uiWhichMorph) + fMaxChange;
                SetQuantity(uiWhichMorph, fQuantity);
            }
        }
    }
    return true;
}

bool BSFaceGenKeyframeMultiple::IsTargetAvailable() {
    for (uint32_t i = 0; i < iNumQuantities; ++i) {
        float fQuantity = pQuantities[i];
        if (!(fQuantity < 0.0) && !(fQuantity > 1.0) && fQuantity > 0.0) {
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
    return pQuantities[aiWhichMorph] >= 0.0 && pQuantities[aiWhichMorph] <= 1.0;
}

void BSFaceGenKeyframeMultiple::ResetQuantityArray(uint32_t aiNumQuantities, bool abForceZero) {
    if (aiNumQuantities != iNumQuantities) {
        if (pQuantities) {
            ourFree(pQuantities);
            pQuantities = nullptr;
        }
        iNumQuantities = aiNumQuantities;
        if (iNumQuantities) {
            pQuantities = static_cast<float*>(ourMalloc(
                ((static_cast<uint64_t>(iNumQuantities) >> 30) != 0) ? static_cast<uint32_t>(-1) : (4 * iNumQuantities)
            ));
            BSFaceGenManager::ResetQuantityArray(pQuantities, iNumQuantities, abForceZero);
        }
    }
    if (iNumQuantities) {
        BSFaceGenManager::ResetQuantityArray(pQuantities, iNumQuantities, abForceZero);
    }
}

float BSFaceGenKeyframeMultiple::GetQuantity(uint32_t aiWhichMorph) const {
    if (aiWhichMorph >= iNumQuantities) {
        return 0.0f;
    }

    return pQuantities[aiWhichMorph];
}

void BSFaceGenKeyframeMultiple::SetQuantityNoTest(uint32_t aiWhichMorph, float afQuantity) {
    pQuantities[static_cast<int32_t>(aiWhichMorph)] = afQuantity;
}

bool BSFaceGenKeyframeMultiple::SetQuantity(uint32_t aiWhichMorph, float afQuantity) {
    if (aiWhichMorph >= iNumQuantities || afQuantity == pQuantities[aiWhichMorph]) {
        return false;
    }
    pQuantities[aiWhichMorph] = afQuantity;
    return true;
}

uint32_t BSFaceGenKeyframeMultiple::GetNumQuantities() {
    return iNumQuantities;
}

void BSFaceGenKeyframeMultiple::operator=(BSFaceGenKeyframeMultiple* apSource) {
    BS_FACEGEN_MORPH_TYPE eType = GetType();
    if (eType == apSource->GetType()) {
        float fTime = apSource->GetTime();
        SetTime(fTime);
        ResetQuantityArray(apSource->iNumQuantities, true);
        for (uint32_t uiWhichMorph = 0; uiWhichMorph < iNumQuantities; ++uiWhichMorph) {
            float fQuantity = apSource->GetQuantity(uiWhichMorph);
            SetQuantity(uiWhichMorph, fQuantity);
        }
    }
}
