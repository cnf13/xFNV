#include "Fallout Shared/Region System/TESRegionPointList.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Region System/TESRegionPoint.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <limits>

TESRegionPointList::ScaleResult::ScaleResult() {}

bool TESRegionPointList::ScaleResult::Matches(TESRegionPoint* apPoint, float afDistance) {
    if (!apPoint) {
        return false;
    }
    return FloatEQ(afDistance, fdist, 0.001f) && *apPoint == point;
}

// The bounds start inverted (FLT_MAX / -FLT_MAX) so that the first point sets them.
TESRegionPointList::TESRegionPointList(bool abOwnsPointMemory)
  : BSSimpleList(), pLastScaleResult(nullptr), bOwnsPointMemory(abOwnsPointMemory),
    Minimums(0x1.fffffe0000000p+127f, 0x1.fffffe0000000p+127f),
    Maximums(-0x1.fffffe0000000p+127f, -0x1.fffffe0000000p+127f), iDistanceInsideAtMax(1024), iCount(0) {}

// The cached scale result is raw ourMalloc memory.
TESRegionPointList::~TESRegionPointList() {
    Clear();
    if (pLastScaleResult) {
        ourFree(pLastScaleResult);
    }
}

bool TESRegionPointList::Load(TESFile* apFile) {
    if (!apFile || apFile->GetTESChunk() != RPLD_ID) {
        return false;
    }
    uint32_t auiMaxSize = apFile->GetChunkSize();
    uint32_t uiPoints = auiMaxSize >> 3;
    if (auiMaxSize % 8) {
        Warning("Invalid Region Point List data in file \"%s\".", &apFile->m_Filename);
        return false;
    }

    if (auiMaxSize && uiPoints) {
        NiPoint2* pPoints = new NiPoint2[uiPoints];
        apFile->GetChunkData(pPoints, auiMaxSize);
        iCount = 0;
        for (uint32_t i = 0; i < uiPoints; ++i) {
            TESRegionPoint* pPoint;
            if (apFile->QEndian()) {
                ENDIAN<float>(pPoints[i].x, false);
                ENDIAN<float>(pPoints[i].y, false);
            }
            pPoint = new TESRegionPoint(pPoints[i]);
            if (static_cast<double>(Minimums.x) > pPoint->Point.x) {
                Minimums.x = pPoint->Point.x;
            }
            if (static_cast<double>(Minimums.y) > pPoint->Point.y) {
                Minimums.y = pPoint->Point.y;
            }
            if (static_cast<double>(Maximums.x) < pPoint->Point.x) {
                Maximums.x = pPoint->Point.x;
            }
            if (static_cast<double>(Maximums.y) < pPoint->Point.y) {
                Maximums.y = pPoint->Point.y;
            }
            AddHead(pPoint);
            ++iCount;
        }
        delete[] pPoints;
        return true;
    }

    return false;
}

bool TESRegionPointList::Validate(bool abMinPointsTest) {
    return (!abMinPointsTest || iCount >= 3) && !SelfIntersects(true);
}

void TESRegionPointList::SetDistanceInsideAtMax(uint32_t aiDistance) {
    iDistanceInsideAtMax = aiDistance;
}

bool TESRegionPointList::SelfIntersects(bool abClosePolygon) {
    bool bQuit = false;
    if (iCount < 4) {
        return false;
    }
    BSSimpleList<TESRegionPoint*>* pP1 = this;
    BSSimpleList<TESRegionPoint*>* pP2 = pP1->m_pkNext;
    BSSimpleList<TESRegionPoint*>* pR1 = pP2->m_pkNext;
    BSSimpleList<TESRegionPoint*>* pR2 = pR1->m_pkNext;
    while (pP2->m_pkNext) {
        pR1 = pP2->m_pkNext;
        if (pR1) {
            pR2 = pR1->m_pkNext;
        }
        else {
            pR2 = nullptr;
        }
        bQuit = false;
        do {
            if (!pR2) {
                // The closing edge runs from the last point back to the list's head.
                if (abClosePolygon && pP1 != this) {
                    pR2 = this;
                }
                bQuit = true;
            }
            if (pP1 &&
                pP2 &&
                pR1 &&
                pR2 &&
                TESRegionPoint::LineSegmentsIntersect(pP1->m_item, pP2->m_item, pR1->m_item, pR2->m_item)) {
                return true;
            }
            pR1 = pR2;
            if (pR2) {
                pR2 = pR2->m_pkNext;
            }
        }
        while (!bQuit);
        pP1 = pP2;
        pP2 = pP2->m_pkNext;
    }
    return false;
}

bool TESRegionPointList::Add(TESRegionPoint* apPoint, float afMaxDelta) {
    if (!apPoint) {
        return false;
    }
    if (m_item) {
        double dDistanceSquared = m_item->GetDistanceSquared(apPoint);
        if (afMaxDelta * afMaxDelta > dDistanceSquared) {
            return false;
        }
    }
    BSSimpleList<TESRegionPoint*>::AddHead(apPoint);
    if (SelfIntersects(false)) {
        BSSimpleList<TESRegionPoint*>::RemoveHead();
        return false;
    }

    ++iCount;
    if (static_cast<double>(Minimums.x) > apPoint->GetX()) {
        Minimums.x = apPoint->GetX();
    }
    if (static_cast<double>(Minimums.y) > apPoint->GetY()) {
        Minimums.y = apPoint->GetY();
    }
    if (static_cast<double>(Maximums.x) < apPoint->GetX()) {
        Maximums.x = apPoint->GetX();
    }
    if (static_cast<double>(Maximums.y) < apPoint->GetY()) {
        Maximums.y = apPoint->GetY();
    }
    return true;
}

bool TESRegionPointList::IsInsideRegion(TESRegionPoint* apTest) {
    BSSimpleList<TESRegionPoint*>* pnext;
    int32_t icrossings = 0;
    if (!apTest || iCount < 3) {
        return false;
    }
    float fxtest = apTest->Point.x;
    float fytest = apTest->Point.y;
    if (static_cast<double>(fxtest) < GetMinX() ||
        static_cast<double>(fxtest) > GetMaxX() ||
        static_cast<double>(fytest) < GetMinY() ||
        static_cast<double>(fytest) > GetMaxY()) {
        return false;
    }
    BSSimpleList<TESRegionPoint*>* pcurrent = this;
    do {
        pnext = pcurrent->m_pkNext;
        if (!pnext) {
            pnext = this;
        }
        TESRegionPoint* pcurrentItem = pcurrent->m_item;
        TESRegionPoint* pnextItem = pnext->m_item;
        float fxcurrent = pcurrentItem->Point.x;
        float fxnext = pnextItem->Point.x;
        if ((fxtest < static_cast<double>(fxcurrent) && fxtest < static_cast<double>(fxnext)) ||
            ((fxtest < static_cast<double>(fxcurrent) || fxtest < static_cast<double>(fxnext)) &&
             static_cast<double>(fxtest) < TESRegionPoint::XIntercept(pcurrentItem, pnextItem, fytest))) {
            float fycurrent = pcurrentItem->Point.y;
            float fynext = pnextItem->Point.y;
            if ((fytest < static_cast<double>(fycurrent) && fynext <= static_cast<double>(fytest)) ||
                (fytest >= static_cast<double>(fycurrent) && fynext > static_cast<double>(fytest))) {
                ++icrossings;
            }
        }
        pcurrent = pnext;
    }
    while (pnext != this);
    return icrossings % 2 != 0;
}

void TESRegionPointList::Save() {
    TESForm::AddChunk(RPLI_ID, iDistanceInsideAtMax);
    BSSimpleList<TESRegionPoint*>* pList = this;
    if (!IsEmpty()) {
        uint32_t iCount = 0;
        uint32_t iResizes = 0;
        uint32_t iCurrSize = 1024;
        NiPoint2* pPoints = new NiPoint2[8192];
        while (pList && !pList->IsEmpty()) {
            pPoints[iCount++] = pList->m_item->Point;
            if (iCount >= iCurrSize) {
                iCurrSize = (++iResizes + 1) << 10;
                NiPoint2* pTemp = pPoints;
                pPoints = new NiPoint2[(iResizes + 1) << 13];
                ourMemcpy(pPoints, pTemp, sizeof(NiPoint2) * iCount);
                delete[] pTemp;
            }
            pList = pList->m_pkNext;
        }
        if (iCount) {
            TESForm::AddChunkArray(RPLD_ID, reinterpret_cast<const uint32_t*>(pPoints), 2 * iCount);
        }
        ourFree(pPoints);
    }
}

TESRegionPointList* TESRegionPointList::Clear() {
    while (iCount) {
        TESRegionPoint* pItem = this->m_item;
        RemoveHead();
        --iCount;
        if (pItem && bOwnsPointMemory) {
            ourFree(pItem);
        }
    }
    iCount = 0;
    Minimums.y = std::numeric_limits<float>::max();
    Minimums.x = std::numeric_limits<float>::max();
    Maximums.y = -std::numeric_limits<float>::max();
    Maximums.x = -std::numeric_limits<float>::max();
    return this;
}

float TESRegionPointList::GetMinX() {
    float fCurMinX = std::numeric_limits<float>::max();
    BSSimpleList<TESRegionPoint*>* pcurrent = this;
    if (Minimums.x < 3.4028234663852886e+38) {
        return Minimums.x;
    }
    while (pcurrent && pcurrent->m_item) {
        if (static_cast<double>(fCurMinX) > pcurrent->m_item->Point.x) {
            fCurMinX = pcurrent->m_item->Point.x;
        }
        pcurrent = pcurrent->m_pkNext;
    }
    return fCurMinX;
}

float TESRegionPointList::GetMaxX() {
    float fCurMaxX = -std::numeric_limits<float>::max();
    BSSimpleList<TESRegionPoint*>* pcurrent = this;
    if (Maximums.x > -3.4028234663852886e+38) {
        return Maximums.x;
    }
    while (pcurrent && pcurrent->m_item) {
        if (static_cast<double>(fCurMaxX) < pcurrent->m_item->Point.x) {
            fCurMaxX = pcurrent->m_item->Point.x;
        }
        pcurrent = pcurrent->m_pkNext;
    }
    return fCurMaxX;
}

float TESRegionPointList::GetMinY() {
    float fCurMinY = std::numeric_limits<float>::max();
    BSSimpleList<TESRegionPoint*>* pcurrent = this;
    if (Minimums.y < 3.4028234663852886e+38) {
        return Minimums.y;
    }
    while (pcurrent && pcurrent->m_item) {
        if (static_cast<double>(fCurMinY) > pcurrent->m_item->Point.y) {
            fCurMinY = pcurrent->m_item->Point.y;
        }
        pcurrent = pcurrent->m_pkNext;
    }
    return fCurMinY;
}

float TESRegionPointList::GetMaxY() {
    float fCurMaxY = -std::numeric_limits<float>::max();
    BSSimpleList<TESRegionPoint*>* pcurrent = this;
    if (Maximums.y > -3.4028234663852886e+38) {
        return Maximums.y;
    }
    while (pcurrent && pcurrent->m_item) {
        if (static_cast<double>(fCurMaxY) < pcurrent->m_item->Point.y) {
            fCurMaxY = pcurrent->m_item->Point.y;
        }
        pcurrent = pcurrent->m_pkNext;
    }
    return fCurMaxY;
}

TESRegionPointList* TESRegionPointList::Copy(TESRegionPointList* apCopyInto) {
    TESRegionPointList* pcopy;
    BSSimpleList<TESRegionPoint*>* pcurrent = this;
    if (apCopyInto) {
        pcopy = apCopyInto;
    }
    else {
        pcopy = new TESRegionPointList(bOwnsPointMemory);
    }
    while (pcurrent) {
        if (pcopy->bOwnsPointMemory) {
            TESRegionPoint* ppointCopy = pcurrent->m_item->Copy(nullptr);
            if (!pcopy->Add(ppointCopy, 0.0f)) {
                ourFree(ppointCopy);
            }
        }
        else {
            pcopy->Add(pcurrent->m_item, 0.0f);
        }
        pcurrent = pcurrent->m_pkNext;
    }
    return pcopy;
}

float TESRegionPointList::GetScaleBasedOnLocation(TESRegionPoint* apTest, float afDistInsideAtMax) {
    bool bQuit = false;
    if (pLastScaleResult && pLastScaleResult->Matches(apTest, afDistInsideAtMax)) {
        return pLastScaleResult->fscale;
    }

    bool bisInsideRegion = IsInsideRegion(apTest);
    if (apTest && bisInsideRegion) {
        if (afDistInsideAtMax < 1.0) {
            return 1.0f;
        }
        TESRegionPointList::ScaleResult* pScaleResult;
        float freturn;
        float fminDistPI = std::numeric_limits<float>::max();
        BSSimpleList<TESRegionPoint*>* pcurrent = this;
        do {
            float fdistPI;
            BSSimpleList<TESRegionPoint*>* pnext = pcurrent->m_pkNext;
            if (!pnext) {
                pnext = this;
                bQuit = true;
            }
            float fdistsqPV1 = apTest->GetDistanceSquared(pcurrent->m_item);
            float fdistsqPV2 = apTest->GetDistanceSquared(pnext->m_item);
            float fdistV1V2 = pcurrent->m_item->GetDistance(pnext->m_item);
            float fdistsqV1V2 = pcurrent->m_item->GetDistanceSquared(pnext->m_item);
            float fNumerator = fdistsqV1V2 - fdistsqPV2 + fdistsqPV1;
            float fdistV1I = fNumerator / (fdistV1V2 * 2.0);
            if (fdistV1V2 <= static_cast<double>(fdistV1I)) {
                fdistPI = fdistsqPV2;
            }
            else if (fdistV1I <= 0.0) {
                fdistPI = fdistsqPV1;
            }
            else {
                fdistPI = fdistsqPV1 - fdistV1I * fdistV1I;
            }
            if (fminDistPI > static_cast<double>(fdistPI)) {
                fminDistPI = fdistPI;
            }
            pcurrent = pnext;
        }
        while (!bQuit);
        float fMinDist = std::sqrtf(fminDistPI);
        if (afDistInsideAtMax <= static_cast<double>(fMinDist)) {
            freturn = 1.0f;
        }
        else {
            freturn = fMinDist / afDistInsideAtMax;
        }
        if (pLastScaleResult) {
            ourFree(pLastScaleResult);
        }
        pScaleResult = new TESRegionPointList::ScaleResult;
        pLastScaleResult = pScaleResult;
        apTest->Copy(&pLastScaleResult->point);
        pLastScaleResult->fdist = afDistInsideAtMax;
        pLastScaleResult->fscale = freturn;
        return freturn;
    }

    return 0.0f;
}
