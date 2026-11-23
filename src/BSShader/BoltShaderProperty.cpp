#include "BSShader/BoltShaderProperty.h"

#include "BSCore/MemoryManager.h"
#include "BSShader/BSShaderAccumulator.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BoltShader.h"
#include "BSShader/ShadowSceneLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

#include <cmath>

NiRTTI BoltShaderProperty::ms_RTTI("BoltShaderProperty", &BSShaderProperty::ms_RTTI);

// The tortuosities are random in [0, 1]; the segment array holds GetMaxSegments() zeroed segments.
BoltShaderProperty::BoltShaderProperty()
  : TargetLocation(0.0f, 0.0f, 0.0f), fTimer(0.0f), iLookBack(4), iBoltStartSegment(0), iBoltEndSegment(0),
    iTotalSegmentCount(0), fDistanceLong(0.0f), iNumBolts(1), fBoltLength(2048.0f), fBoltsRadius(100.0f),
    fBoltsRadiusStrength(3.0f), fBoltSmallWidth(3.0f), fBoltGrowWidth(1.0f),
    iSegmentsPerBolt(static_cast<int32_t>(BoltShader::GetMaxSegments())), fSegmentLength(1.0f), fSegmentVariance(1.0f),
    iSubSegments(static_cast<uint32_t>(BoltShader::iSubSegments)), fSubSegmentVariance(1.0f),
    CoreColor(NiColorA::WHITE), GlowColor(NiColorA::WHITE), bWrapped(0), bMoving(1), bGuided(1), bStartFromPoint(1),
    pMyGeometry(nullptr), fDistanceShort(0.0f), iBoltCurrentSegmentCount(0), iTotalAddedSegments(0), fAlpha(1.0f) {
    BoltShader::GetSegmentIndices();
    pSegmentData = new BoltShader::SegmentData[BoltShader::GetMaxSegments()]();
    for (float& rfTortuosity : fTortuosity) {
        rfTortuosity = NiUnitRandom();
    }
    ++BoltShaderProperty::iRefCount;
    iShader = 25;
    iShaderPropertyType = 15;
}

// The last bolt property frees the shared bolt data.
BoltShaderProperty::~BoltShaderProperty() {
    delete[] pSegmentData;
    if (!(--iRefCount)) {
        au_re_GameHeapFree_36();
    }
}

const NiRTTI* BoltShaderProperty::GetRTTI() const {
    return &BoltShaderProperty::ms_RTTI;
}

// The clone is built in an engine block through the constructor, then the copy body fills it in.
NiObject* BoltShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
    BoltShaderProperty* pClone = new BoltShaderProperty;
    CopyMembers(pClone, arCloneData);
    return pClone;
}

void BoltShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    BSShaderProperty::GetViewerStrings(apStrings);
    char* ViewerString = NiGetViewerString(BoltShaderProperty::ms_RTTI.GetName());
    uint32_t m_usSize = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    char* v6 = ViewerString;
    if (m_usSize >= m_usMaxSize) {
        apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(m_usSize, v6);
}

bool BoltShaderProperty::SetupGeometry(NiGeometry* apGeometry) {
    pMyGeometry = apGeometry;
    return true;
}

BSShaderProperty::RenderPassArray* BoltShaderProperty::GetRenderPasses(
    NiGeometry* apGeom,
    int32_t aiEnabledPasses,
    uint16_t& asCount,
    uint32_t auiRenderMode,
    BSShaderAccumulator* apAccumulator,
    int32_t abUpdateList
) {
    BSShaderProperty::RenderPassArray* pRenderPassArray;
    BSShaderProperty::CheckCreateRenderPassArray(1u);
    pRenderPassArray = this->pRenderPassList;
    if (!pRenderPassArray->iSize) {
        pRenderPassArray->Add(apGeom, BSShaderManager::BSSM_BOLT, 1, 0, nullptr, nullptr, nullptr, nullptr);
        this->pRenderPassList->pPassA.m_pBase[this->pRenderPassList->iSize - 1]
            ->eAccumulationHint = BSShaderProperty::BSSP_HINT_SORT;
    }
    return this->pRenderPassList;
}

void BoltShaderProperty::CreateSegment(
    int32_t aiBolt,
    int32_t aiSegment,
    int32_t aiLastSegment,
    int32_t aiLookBackSegment
) {
    NiPoint3 v25;
    uint32_t v6 = aiBolt * iSegmentsPerBolt;
    uint32_t v7 = v6 + aiSegment;
    BoltShader::SegmentData* pSegmentData = this->pSegmentData;
    int32_t v9 = aiLastSegment + v6;
    float x = pSegmentData[v9].fXPos;
    BoltShader::SegmentData* v11 = &pSegmentData[v9];
    float y = v11->fYPos;
    float z = v11->fZPos;
    int32_t aiLastSegmenta = iSegmentsPerBolt;
    int32_t v13 = aiLookBackSegment + aiBolt * aiLastSegmenta;
    float v14 = pSegmentData[v13].fYPos;
    BoltShader::SegmentData* v15 = &pSegmentData[v13];
    float v30 = v15->fXPos;
    float v16 = v15->fZPos;
    v25.x = x - v30;
    v25.y = y - v14;
    v25.z = z - v16;
    if (v25.y < 0.0) {
        v25 = NiPoint3::ZERO;
    }
    v25.y = v25.y + 1.0;
    v25.Unitize();
    float aiBolta = fSegmentLength;
    v25.x = aiBolta * v25.x;
    v25.y = aiBolta * v25.y;
    v25.z = aiBolta * v25.z;
    float aiBoltb = static_cast<double>(aiSegment) / static_cast<double>(aiLastSegmenta);
    float aiSegmenta = NiUnitRandom();
    float aiSegmentb = fSegmentVariance * aiSegmenta + (v25.x + x) - fSegmentVariance * 0.5;
    this->pSegmentData[v7].fXPos = aiSegmentb;
    float aiSegmentc = v25.y + y;
    this->pSegmentData[v7].fYPos = aiSegmentc;
    float aiSegmentd = NiUnitRandom();
    float aiSegmente = fSegmentVariance * aiSegmentd + (v25.z + z) - fSegmentVariance * 0.5;
    this->pSegmentData[v7].fZPos = aiSegmente;
    BoltShader::SegmentData* v19 = &this->pSegmentData[v7];
    v25.x = TargetLocation.x * aiBoltb - v19->fXPos;
    v25.y = aiBoltb * TargetLocation.z - v19->fZPos;
    float aiSegmentf = v25.y * v25.y + v25.x * v25.x;
    float aiSegmentg = std::sqrt(static_cast<double>(aiSegmentf));
    double v20 = aiSegmentg;
    if (fBoltsRadius < static_cast<double>(aiSegmentg)) {
        double v23;
        double v21 = v20 - fBoltsRadius;
        float aiSegmenth = fBoltsRadiusStrength * v21 / v20;
        double v22 = aiSegmenth;
        float aiSegmenti = aiSegmenth * v20;
        float aiSegmentj = std::fabs(static_cast<double>(aiSegmenti));
        if (!(fBoltsRadius < static_cast<double>(aiSegmentj))) {
            v23 = v22;
        }
        else {
            float aiSegmentk = v21 / v20;
            v23 = aiSegmentk;
        }
        BoltShader::SegmentData* v24 = this->pSegmentData;
        v25.x = v25.x * v23;
        v25.y = v23 * v25.y;
        v24[v7].fXPos = v24[v7].fXPos + v25.x;
        this->pSegmentData[v7].fZPos = this->pSegmentData[v7].fZPos + v25.y;
    }
}

// Each new segment advances the end of the ring (pushing the start when they meet) and is created for every bolt. The
// segment fields are compared signed, and bWrapped is re-read after each CreateSegment.
void BoltShaderProperty::ExtendSegments(int32_t aiNewSegments) {
    if (aiNewSegments <= 0) {
        return;
    }
    do {
        ++iBoltEndSegment;
        int32_t iSegmentsPerBolt_ = iSegmentsPerBolt;
        if (iBoltEndSegment >= iSegmentsPerBolt_) {
            iBoltEndSegment = 0;
        }
        int32_t iSegment = iBoltEndSegment;
        int32_t iStart = iBoltStartSegment;
        if (iSegment == iStart) {
            ++iStart;
            iBoltStartSegment = iStart;
            if (iStart >= iSegmentsPerBolt_) {
                iBoltStartSegment = iStart - iSegmentsPerBolt_;
            }
        }
        bWrapped = iBoltStartSegment > iSegment;
        for (int32_t iBolt = 0; iBolt < static_cast<int32_t>(iNumBolts); ++iBolt) {
            int32_t iLastSegment;
            if (iSegment != 0) {
                iLastSegment = iSegment - 1;
            }
            else if (bWrapped) {
                iLastSegment = iSegmentsPerBolt - 1;
            }
            else {
                iLastSegment = 0;
            }
            int32_t iLookBack_ = static_cast<int32_t>(iLookBack);
            int32_t iLookBackSegment;
            if (iSegment >= iLookBack_) {
                iLookBackSegment = iSegment - iLookBack_;
            }
            else if (bWrapped) {
                iLookBackSegment = iSegmentsPerBolt - iLookBack_ + iSegment;
            }
            else {
                iLookBackSegment = 0;
            }
            CreateSegment(iBolt, iSegment, iLastSegment, iLookBackSegment);
        }
    }
    while (--aiNewSegments != 0);
}

// Moves the end back (see CleanupSegments). Signed division and compares.
void BoltShaderProperty::RemoveSegments(int32_t aiRemoveSegments) {
    int32_t iSegmentsPerBolt_ = iSegmentsPerBolt;
    int32_t iRemove = aiRemoveSegments % iSegmentsPerBolt_;
    iBoltEndSegment -= iRemove;
    int32_t iEnd = iBoltEndSegment;
    if (bWrapped && iEnd < 0) {
        iBoltStartSegment += iSegmentsPerBolt_;
        bWrapped = 0;
    }
    iBoltCurrentSegmentCount -= iRemove;
    if (!bWrapped) {
        int32_t iStart = iBoltStartSegment;
        if (iEnd <= iStart) {
            iBoltEndSegment = iStart;
        }
    }
}

// Moves the start forward, dropping the oldest segments; RemoveSegments is the one that moves the end back. The segment
// fields are divided and compared signed.
void BoltShaderProperty::CleanupSegments(int32_t aiRemoveSegments) {
    int32_t iSegmentsPerBolt_ = iSegmentsPerBolt;
    int32_t iRemove = aiRemoveSegments % iSegmentsPerBolt_;
    iBoltStartSegment += iRemove;
    int32_t iStart = iBoltStartSegment;
    if (bWrapped && iStart >= iSegmentsPerBolt_) {
        iBoltStartSegment = iStart - iSegmentsPerBolt_;
        bWrapped = 0;
    }
    iBoltCurrentSegmentCount -= iRemove;
    if (!bWrapped) {
        int32_t iEnd = iBoltEndSegment;
        if (iBoltStartSegment >= iEnd) {
            iBoltStartSegment = iEnd;
        }
    }
}

void BoltShaderProperty::MoveAllSegments(float afMoveDelta) {
    uint8_t bWrapped = this->bWrapped;
    int32_t iBoltStartSegment = this->iBoltStartSegment;
    for (;;) {
        if (!bWrapped || iBoltStartSegment >= iSegmentsPerBolt) {
            if (iBoltStartSegment > iBoltEndSegment) {
                break;
            }
        }
        int32_t i;
        float* p_y;
        for (i = 0; i < static_cast<int32_t>(iNumBolts); *p_y = *p_y + afMoveDelta) {
            p_y = &pSegmentData[iBoltStartSegment + i * iSegmentsPerBolt].fYPos;
            ++i;
        }
        if (++iBoltStartSegment >= iSegmentsPerBolt && bWrapped) {
            iBoltStartSegment = 0;
            bWrapped = 0;
        }
    }
}

// Does the work of CleanupSegments here, with signed division and compares. The removed count is at least one.
void BoltShaderProperty::CleanupBolt(float afTimeDelta, float afSpeed) {
    float fDistance = afTimeDelta * afSpeed;
    int32_t iRemove = static_cast<int32_t>(afTimeDelta * (static_cast<int32_t>(iTotalAddedSegments) / fTimer));
    if (iRemove <= 0) {
        iRemove = 1;
    }
    int32_t iSegmentsPerBolt_ = iSegmentsPerBolt;
    int32_t iRemoveMod = iRemove % iSegmentsPerBolt_;
    iBoltStartSegment += iRemoveMod;
    int32_t iStart = iBoltStartSegment;
    if (bWrapped && iStart >= iSegmentsPerBolt_) {
        iBoltStartSegment = iStart - iSegmentsPerBolt_;
        bWrapped = 0;
    }
    if (!bWrapped) {
        int32_t iEnd = iBoltEndSegment;
        if (iBoltStartSegment >= iEnd) {
            iBoltStartSegment = iEnd;
        }
    }
    iBoltCurrentSegmentCount -= iRemoveMod;
    fDistanceShort = fDistanceShort + fDistance;
}

// The segment count's product with the segment length is loaded signed.
void BoltShaderProperty::ExtendBolt(float afTimeDelta, float afDesiredBoltDistance) {
    fTimer = afTimeDelta + fTimer;
    float fRemaining = afDesiredBoltDistance - this->fDistanceLong;
    float fSegmentCount = fRemaining / fSegmentLength;
    float fWholeSegments = std::ceil(static_cast<double>(fSegmentCount));
    this->fDistanceLong = fWholeSegments * fSegmentLength + this->fDistanceLong;
    int32_t v4 = static_cast<int32_t>(fWholeSegments);
    ExtendSegments(v4);
    float fMaxLength = static_cast<double>(iSegmentsPerBolt) * fSegmentLength;
    double fDistanceLong = this->fDistanceLong;
    if (!(fMaxLength > fDistanceLong)) {
        fDistanceLong = fMaxLength;
    }
    float fClampedLong = fDistanceLong;
    float fExcess = this->fDistanceLong - fClampedLong;
    double v6 = fExcess;
    if (fExcess < 0.0) {
        v6 = 0.0;
    }
    iTotalAddedSegments += v4;
    float fClampedExcess = v6;
    fDistanceShort = fClampedExcess;
}

bool BoltShaderProperty::IsAtMaxLength() {
    return false;
}

void BoltShaderProperty::UpdateBolt(float afTimeDelta, float afDesiredBoltDistance, float afSpeed, bool abCleanup) {
    if (abCleanup) {
        CleanupBolt(afTimeDelta, afSpeed);
    }
    else {
        ExtendBolt(afTimeDelta, afDesiredBoltDistance);
    }
}

void BoltShaderProperty::SetStartFromPoint(bool abStartFromPoint) {
    bStartFromPoint = abStartFromPoint;
}

void BoltShaderProperty::SetMoving(bool abMoving) {
    bMoving = abMoving;
}

void BoltShaderProperty::SetAbsorbNumBolts(int32_t aiNumBolts) {
    iNumBolts = static_cast<uint32_t>(aiNumBolts);
}

void BoltShaderProperty::SetAbsorbCoreColor(float afR, float afG, float afB, float afA) {
    CoreColor.r = afR;
    CoreColor.g = afG;
    CoreColor.b = afB;
    CoreColor.a = afA;
}

void BoltShaderProperty::SetAbsorbGlowColor(float afR, float afG, float afB, float afA) {
    GlowColor.r = afR;
    GlowColor.g = afG;
    GlowColor.b = afB;
    GlowColor.a = afA;
}

void BoltShaderProperty::SetAbsorbSegmentLength(float afSegmentLength) {
    fSegmentLength = afSegmentLength;
}

void BoltShaderProperty::SetAbsorbTortuosityVariance(float afVariance) {
    fSubSegmentVariance = afVariance;
}

void BoltShaderProperty::SetBoltsRadius(float afBoltsRadius) {
    fBoltsRadius = afBoltsRadius;
}

void BoltShaderProperty::SetBoltsRadiusStrength(float afStrength) {
    fBoltsRadiusStrength = afStrength;
}

void BoltShaderProperty::SetBoltSmallWidth(float afBoltSmallWidth) {
    fBoltSmallWidth = afBoltSmallWidth;
}

void BoltShaderProperty::SetBoltGrowWidth(float afBoltGrowWidth) {
    fBoltGrowWidth = afBoltGrowWidth;
}

void BoltShaderProperty::SetSegmentVariance(float afSegmentVariance) {
    fSegmentVariance = afSegmentVariance;
}

void BoltShaderProperty::UpdateAndMoveBolt(
    float afTimeDelta,
    float afDesiredBoltDistance,
    float afSpeed,
    bool abCleanup
) {
    double v7;
    BoltShader::SegmentData* pSegmentData;
    uint32_t iBoltStartSegment;
    double v10;
    float a1_4b;
    float a1_4c;
    float a1_4d;
    double v5 = afTimeDelta;
    if (afTimeDelta > 0.10000000149011612) {
        v5 = 0.10000000149011612f;
    }
    float fClampedDelta = static_cast<float>(v5);
    float fMoveDistance = fClampedDelta * afSpeed;
    MoveAllSegments(fMoveDistance);
    float fMinLength = fSegmentLength;
    if (!(afDesiredBoltDistance < static_cast<double>(fMinLength))) {
        v7 = afDesiredBoltDistance;
    }
    else {
        v7 = fMinLength;
    }
    pSegmentData = this->pSegmentData;
    iBoltStartSegment = this->iBoltStartSegment;
    fDistanceLong = pSegmentData[iBoltEndSegment].fYPos;
    float a1_4a = pSegmentData[iBoltStartSegment].fYPos;
    fDistanceShort = a1_4a;
    if ((-fSegmentLength) >= a1_4a) {
        a1_4b = (-a1_4a) / fSegmentLength;
        CleanupSegments(static_cast<int32_t>(a1_4b));
        fDistanceShort = this->pSegmentData[this->iBoltStartSegment].fYPos;
    }
    if (!(fDistanceLong < v7)) {
        v10 = v7 + fSegmentLength;
        if (fDistanceLong >= v10) {
            a1_4d = static_cast<float>((fDistanceLong - v10) / fSegmentLength);
            RemoveSegments(static_cast<int32_t>(a1_4d));
            fDistanceLong = this->pSegmentData[iBoltEndSegment].fYPos;
        }
    }
    else {
        a1_4c = static_cast<float>((v7 - fDistanceLong) / fSegmentLength);
        ExtendSegments(static_cast<int32_t>(a1_4c));
        fDistanceLong = this->pSegmentData[iBoltEndSegment].fYPos;
    }
}

void BoltShaderProperty::InitSegmentData() {
    int32_t iSegmentsPerBolt;
    int32_t iNumBolts;
    int32_t v10;
    float v11;
    float v12;
    float v13;
    float v14;
    float v15;
    float v16;
    float v17;
    float v18;
    int32_t maxSegments = static_cast<int32_t>(BoltShader::GetMaxSegments());
    double v3 = 0.0;
    iSegmentsPerBolt = this->iSegmentsPerBolt;
    fTimer = 0.0f;
    int32_t v5 = static_cast<int32_t>(iSegmentsPerBolt * this->iNumBolts);
    bWrapped = 0;
    if (maxSegments < v5) {
        v11 = static_cast<float>(static_cast<double>(v5 - maxSegments) / iSegmentsPerBolt);
        v12 = static_cast<float>(std::ceil(static_cast<double>(v11)));
        v3 = 0.0;
        this->iNumBolts -= static_cast<int32_t>(v12);
    }
    int32_t v6 = this->iSegmentsPerBolt;
    uint32_t v7 = 4;
    if (v6 <= 4) {
        v7 = this->iSegmentsPerBolt;
    }
    iLookBack = v7;
    iNumBolts = static_cast<int32_t>(this->iNumBolts);
    int32_t v9 = 0;
    iTotalSegmentCount = static_cast<uint32_t>(v6 * iNumBolts);
    if (iNumBolts > 0) {
        v10 = 0;
        do {
            if (bStartFromPoint) {
                pSegmentData[v10].fXPos = static_cast<float>(v3);
                pSegmentData[v10].fYPos = static_cast<float>(v3);
                pSegmentData[v10].fZPos = static_cast<float>(v3);
            }
            else {
                v13 = NiUnitRandom();
                v14 = static_cast<float>((fBoltsRadius * v13) - (fBoltsRadius * 0.5));
                pSegmentData[v10].fXPos = v14;
                v15 = NiUnitRandom();
                v16 = static_cast<float>((fBoltsRadius * v15) - (fBoltsRadius * 0.5));
                pSegmentData[v10].fYPos = v16;
                v17 = NiUnitRandom();
                v18 = static_cast<float>((fBoltsRadius * v17) - (fBoltsRadius * 0.5));
                pSegmentData[v10].fZPos = v18;
                v3 = 0.0;
            }
            ++v9;
            ++v10;
        }
        while (v9 < static_cast<int32_t>(this->iNumBolts));
    }
    iBoltEndSegment = 0;
    iBoltStartSegment = 0;
}

void BoltShaderProperty::CopyMembers(BoltShaderProperty* pDest, NiCloningProcess& arCloneData) {
    NiShadeProperty::CopyMembers(pDest, arCloneData);
    pDest->ulFlags[0] = ulFlags[0];
    pDest->ulFlags[1] = ulFlags[1];
    // The shader's alpha (+0x28), which the bolt's own fAlpha (+0x18C) hides.
    pDest->BSShaderProperty::fAlpha = BSShaderProperty::fAlpha;
    double fFadeAlpha = this->fFadeAlpha;
    pDest->iLastRenderPassState = 0;
    pDest->fFadeAlpha = fFadeAlpha;
    pDest->iShader = iShader;
    pDest->fDepthBias = fDepthBias;
}
