#include "SpeedTree/Branch.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SpeedTree/BranchInfo.h"
#include "SpeedTree/CRotTransform.h"
#include "SpeedTree/FrondEngine.h"
#include "SpeedTree/IndexedGeometry.h"
#include "SpeedTree/LeafInfo.h"
#include "SpeedTree/LibSpline_Source/IdvSpline.h"
#include "SpeedTree/LibVector_Source/IdvVector.h"
#include "SpeedTree/SIdvBranchVertex.h"
#include "SpeedTree/SpeedTreeRT.h"
#include "SpeedTree/StructsSupport.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <vector>

CIdvRandom CIdvBranch::m_cRandom;
const CVec3 g_cOut(1.0f, 0.0f, 0.0f);
static const CVec3 g_cDown(0.0f, 0.0f, -1.0f);
static const CVec3 g_cUp(0.0f, 0.0f, 1.0f);
IntVector CIdvBranch::m_vBlossoms;
IntVector CIdvBranch::m_vNonBlossoms;
BranchInfoVector CIdvBranch::m_vBranchInfo;
LeafPtrVector CIdvBranch::m_vLocalLeaves;

// -1 marks the vertex count and starting offset as unset.
CIdvBranch::CIdvBranch(CIdvBranch* pParent)
  : m_pParent(pParent), m_fPercentOfParent(0.0f), m_pVertices(nullptr), m_nVertexCount(-1), m_usCrossSectionSegments(0),
    m_nStartingOffset(-1), m_fVolume(0.0f), m_fFuzzyVolume(0.0f) {}

// The vertex array is a raw ourMalloc buffer; the child branches are owned.
CIdvBranch::~CIdvBranch() {
    ourFree(m_pVertices);
    m_pVertices = nullptr;
    for (size_t i = 0; i < m_vChildren.size(); ++i) {
        delete m_vChildren[i].m_pBranch;
        m_vChildren[i].m_pBranch = nullptr;
    }
}

void CIdvBranch::ComputeVolume() {
    SIdvBranchVertex* v2;
    float v3;
    int32_t i;
    if (m_pVertices) {
        if (m_nVertexCount > 1) {
            m_fVolume = (float)0.0;
            for (i = 0; i < (m_nVertexCount - 1); ++i) {
                v2 = &m_pVertices[i];
                v3 = (&v2->m_cPos)->Distance(v2[1].m_cPos);
                m_fVolume = (((v2->m_fRadius + v2[1].m_fRadius) * v3) + m_fVolume);
            }
        }
    }
}

// Differs from speedtree-2's Compute: - a sixth argument, fCombinedPercentOfParent: the cross sections' combined
// progress (0 on the trunk's first vertex; on the trunk's later vertices 1 before the first branch, else
// -m_fFirstBranch / (1 - m_fFirstBranch)); a child gets (1 - it) * its profile percent + it, squared for children of
// levels 0 and 1; - above the weight level a vertex takes ComputeHighLevelVertexWeight's weight when
// m_bPropagateFlexibility is set, and the child's weight is interpolated then too; - the trunk stores twice its base
// radius and its length to the first branch in pBranchGeometry; - the profile percent stays 1 when the first and last
// branch positions are equal. Some intermediate values are float locals and others are computed in double (the tile
// offset and the tile's s and t also through a double temporary), to match the game's rounding.
void CIdvBranch::Compute(
    uint32_t nSeed,
    float fSize,
    int32_t nLevel,
    const CVec3& cBasePos,
    float fPercentOfParent,
    float fCombinedPercentOfParent,
    const CRotTransform& cBaseTransform,
    const CVec3& cZAxis,
    CIndexedGeometry* pBranchGeometry,
    LeafPtrVector& vLeaves,
    float fWindWeight,
    int32_t nWindGroup,
    float fParentRadius
) {
    // GetLevel's unsigned level is compared signed.
    bool bFrond = m_pFrondEngine->Enabled() && nLevel >= m_pFrondEngine->GetLevel();
    if (nLevel == m_nWeightLevel) {
        nWindGroup = m_nRoundRobinWindLevel++;
    }
    if (nLevel == 0) {
        BuildBlossomVectors();
    }
    const SIdvBranchInfo& sInfo = *m_vBranchInfo[nLevel];
    if (!bFrond) {
        m_nStartingOffset = pBranchGeometry->m_nVertexCounter;
    }
    m_fPercentOfParent = fPercentOfParent;
    if (!bFrond) {
        ComputeFlareEntries(sInfo);
    }

    float fLength = sInfo.m_pLength->Evaluate(fPercentOfParent) * fSize;
    float fSegmentLength = fLength / sInfo.m_nSegments;
    float fAzimuth = m_cRandom.GetUniform(-180.0f, 180.0f);
    float fStartAngle = sInfo.m_pStartAngle->Evaluate(fPercentOfParent);
    float fGravity = sInfo.m_pGravity->Evaluate(fPercentOfParent);
    float fRadius = sInfo.m_pRadius->Evaluate(fPercentOfParent) * fSize;
    float fFlexibility = sInfo.m_pFlexibility->Evaluate(fPercentOfParent);
    m_usCrossSectionSegments = static_cast<uint16_t>(sInfo.m_nCrossSectionSegments);

    // Adds the double 0.5 (speedtree-2's 180.0f / 360.0f).
    float fTileOffset = sInfo.m_bRandomTCoordOffset ? fAzimuth + 0.5 + fPercentOfParent : 0.0;
    CVec3 cTile;
    cTile.m_afData[0] = sInfo.m_bSTileAbsolute ? static_cast<double>(sInfo.m_fSTile)
                                               : static_cast<double>(sInfo.m_fSTile) * fRadius * c_fTwoPi;
    cTile.m_afData[1] = sInfo.m_bTTileAbsolute ? static_cast<double>(sInfo.m_fTTile)
                                               : static_cast<double>(fLength) / fSize * sInfo.m_fTTile;
    cTile.m_afData[2] = (nSeed + m_vChildren.size()) % 2 == 0 ? sInfo.m_fTwist : -sInfo.m_fTwist;

    // The parent radius test against the double 0.0, the factor the float 0.85f.
    float fFirstRadius = sInfo.m_pRadiusScale->Evaluate(0.0f) * fRadius;
    if (fParentRadius > 0.0 && fFirstRadius > static_cast<double>(fParentRadius) * 0.85f) {
        fFirstRadius = static_cast<double>(fParentRadius) * 0.85f;
        fRadius = fFirstRadius;
    }

    m_nVertexCount = sInfo.m_nSegments + 1;
    if (m_nVertexCount >= 2) {
        // new SIdvBranchVertex[m_nVertexCount]: a size that overflows asks for SIZE_MAX bytes.
        int32_t nCount = m_nVertexCount;
        uint32_t uiCount = static_cast<uint32_t>(nCount);
        void* pMemory =
            ourMalloc(uiCount > SIZE_MAX / sizeof(SIdvBranchVertex) ? SIZE_MAX : sizeof(SIdvBranchVertex) * uiCount);
        SIdvBranchVertex* pVertices = nullptr;
        if (pMemory != nullptr) {
            SIdvBranchVertex* pConstruct = static_cast<SIdvBranchVertex*>(pMemory);
            for (int32_t nLeft = nCount; --nLeft >= 0; ++pConstruct) {
                ::new (pConstruct) SIdvBranchVertex;
            }
            pVertices = static_cast<SIdvBranchVertex*>(pMemory);
        }
        m_pVertices = pVertices;

        if (!bFrond) {
            uint16_t nIndexCount =
                static_cast<uint16_t>((m_nVertexCount - 1) * (sInfo.m_nCrossSectionSegments * 2 + 4));
            // An overflow check of the doubled size would never fire for a 16-bit count, so none is done.
            uint16_t* pBranchStrip = static_cast<uint16_t*>(ourMalloc(nIndexCount * sizeof(uint16_t)));
            uint16_t nStart = pBranchGeometry->m_nVertexCounter;
            int32_t nVertexIndex = 0;
            int32_t nIndex2 = 0;
            for (int32_t i = 0; i < m_nVertexCount - 1; ++i) {
                int32_t nLoopAround = nIndex2;
                for (int32_t j = 0; j < sInfo.m_nCrossSectionSegments + 1; ++j) {
                    pBranchStrip[nVertexIndex++] =
                        static_cast<uint16_t>(nStart + (sInfo.m_nCrossSectionSegments + 1) + nIndex2);
                    pBranchStrip[nVertexIndex++] = static_cast<uint16_t>(nStart + nIndex2);
                    nIndex2++;
                }
                pBranchStrip[nVertexIndex++] =
                    static_cast<uint16_t>(nStart + (sInfo.m_nCrossSectionSegments + 1) + nLoopAround);
                pBranchStrip[nVertexIndex++] =
                    static_cast<uint16_t>(nStart + (sInfo.m_nCrossSectionSegments + 1) + nLoopAround);
            }
            pBranchGeometry->AddStrip(0, pBranchStrip, nIndexCount);
            ++pBranchGeometry->m_nStripCounter;
        }

        SIdvBranchVertex* pFirstVertex = m_pVertices;
        pFirstVertex->m_cPos = cBasePos;
        float fAzimuthDisturbance = sInfo.m_pDisturbance->ScaledVariance(0.0f);
        float fPitchDisturbance = sInfo.m_pDisturbance->ScaledVariance(0.0f);
        pFirstVertex->m_cTrans = cBaseTransform;
        CVec3 cRotAxis = cBaseTransform * cZAxis;
        pFirstVertex->m_cTrans.RotateAxis(fAzimuth, cRotAxis);
        float fPitch = fStartAngle + fPitchDisturbance;
        pFirstVertex->m_cTrans.RotateYZ(fPitch, fAzimuthDisturbance);
        pFirstVertex->m_cDirection = g_cOut * pFirstVertex->m_cTrans;
        pFirstVertex->m_fRadius = sInfo.m_pRadiusScale->Evaluate(0.0f) * fRadius;
        float fVertexFlexibility = sInfo.m_pFlexibilityScale->Evaluate(0.0f) * fFlexibility;

        // VecRad2Deg multiplies by the double 57.295780181884766; c_fInverseOf90 is the float 1/90.
        float fAngleBetween = pFirstVertex->m_cDirection.AngleBetween(g_cDown);
        float fGravityAngle = fAngleBetween * 57.295780181884766;
        float fFromLevel = 90.0 - fGravityAngle;
        float fAbsFromLevel = static_cast<float>(std::fabs(static_cast<double>(fFromLevel)));
        float fFacingFactor = 1.0 - static_cast<double>(fAbsFromLevel) * 0.0111111114f;
        CVec3 cAdjustmentAxis = pFirstVertex->m_cDirection.Cross(g_cDown);
        cAdjustmentAxis.Normalize();
        float fGravityProfile = (sInfo.m_pAngleProfile->Evaluate(0.0f) - 0.5) * 2.0 * -1.0;
        CRotTransform cTransformAdjustment;
        cTransformAdjustment.m_afData[0][0] = 1.0f;
        cTransformAdjustment.m_afData[0][1] = 0.0f;
        cTransformAdjustment.m_afData[0][2] = 0.0f;
        cTransformAdjustment.m_afData[1][0] = 0.0f;
        cTransformAdjustment.m_afData[1][1] = 1.0f;
        cTransformAdjustment.m_afData[1][2] = 0.0f;
        cTransformAdjustment.m_afData[2][0] = 0.0f;
        cTransformAdjustment.m_afData[2][1] = 0.0f;
        cTransformAdjustment.m_afData[2][2] = 1.0f;
        float fAdjustment = static_cast<double>(fGravityProfile) * fGravity * fGravityAngle * fFacingFactor;
        cTransformAdjustment.RotateAxisFromIdentity(fAdjustment, cAdjustmentAxis);
        pFirstVertex->m_cTrans = pFirstVertex->m_cTrans * cTransformAdjustment;
        pFirstVertex->m_cDirection = g_cOut * pFirstVertex->m_cTrans;

        // The trunk's width and length to its first branch.
        if (pBranchGeometry != nullptr && nLevel == 0) {
            float fTrunkWidth = pFirstVertex->m_fRadius + pFirstVertex->m_fRadius;
            pBranchGeometry->m_fTrunkWidth = fTrunkWidth;
            float fTrunkLength = fLength * sInfo.m_fFirstBranch;
            pBranchGeometry->m_fTrunkLength = fTrunkLength;
        }

        float fCrossSectionWeight = fWindWeight;
        if (nLevel <= m_nWeightLevel) {
            pFirstVertex->m_fWindWeight = ComputeVertexWeight(nLevel, 0.0f, fVertexFlexibility);
            fCrossSectionWeight = pFirstVertex->m_fWindWeight;
        }
        else if (m_bPropagateFlexibility) {
            pFirstVertex->m_fWindWeight = ComputeHighLevelVertexWeight(fWindWeight, fVertexFlexibility);
            fCrossSectionWeight = pFirstVertex->m_fWindWeight;
        }
        if (!bFrond) {
            float fCombinedProgress = 0.0f;
            if (nLevel != 0) {
                fCombinedProgress = fCombinedPercentOfParent;
            }
            BuildCrossSection(
                pFirstVertex, 0.0f, fCombinedProgress, m_usCrossSectionSegments, pBranchGeometry, cTile,
                fCrossSectionWeight, nWindGroup, fTileOffset, nLevel
            );
        }
        else {
            m_pFrondEngine->StartGuide();
            m_pFrondEngine->AddGuideVertex(
                pFirstVertex->m_cPos, pFirstVertex->m_cTrans, fCrossSectionWeight, nWindGroup
            );
        }

        float fRunningLength = 0.0f;
        m_pVertices[0].m_fRunningLength = 0.0f;
        SIdvBranchVertex* pLastVertex = pFirstVertex;
        for (int32_t i = 1; i < m_nVertexCount; ++i) {
            SIdvBranchVertex* pVertex = m_pVertices + i;

            // Divides by the integer m_nVertexCount - 1.
            float fLinearProgress = static_cast<double>(i) / (m_nVertexCount - 1);
            float fExponent = sInfo.m_fSegmentPackingExponent;
            float fProgress =
                static_cast<float>(std::pow(static_cast<double>(fLinearProgress), static_cast<double>(fExponent)));
            fSegmentLength = static_cast<double>(fProgress) * fLength - fRunningLength;

            pVertex->m_fRadius = sInfo.m_pRadiusScale->Evaluate(fProgress) * fRadius;
            float fLocalVertexFlexibility = sInfo.m_pFlexibilityScale->Evaluate(fProgress) * fFlexibility;

            pVertex->m_cTrans = pLastVertex->m_cTrans;
            pVertex->m_cDirection = g_cOut * pVertex->m_cTrans;

            float fLocalAngleBetween = pVertex->m_cDirection.AngleBetween(g_cDown);
            float fLocalGravityAngle = fLocalAngleBetween * 57.295780181884766;
            float fLocalFromLevel = 90.0 - fLocalGravityAngle;
            float fLocalAbsFromLevel = static_cast<float>(std::fabs(static_cast<double>(fLocalFromLevel)));
            float fLocalFacingFactor = 1.0 - static_cast<double>(fLocalAbsFromLevel) * 0.0111111114f;
            CVec3 cThisAdjustmentAxis = pVertex->m_cDirection.Cross(g_cDown);
            cThisAdjustmentAxis.Normalize();
            float fLocalGravityProfile = (sInfo.m_pAngleProfile->Evaluate(fProgress) - 0.5) * 2.0 * -1.0;
            float fLocalAdjustment =
                static_cast<double>(fLocalGravityProfile) * fGravity * fLocalGravityAngle * fLocalFacingFactor;
            cTransformAdjustment.RotateAxisFromIdentity(fLocalAdjustment, cThisAdjustmentAxis);
            pVertex->m_cTrans = pVertex->m_cTrans * cTransformAdjustment;

            float fLocalAzimuthDisturbance = sInfo.m_pDisturbance->ScaledVariance(fProgress);
            float fLocalPitchDisturbance = sInfo.m_pDisturbance->ScaledVariance(fProgress);
            pVertex->m_cTrans.RotateYZ(fLocalPitchDisturbance, fLocalAzimuthDisturbance);
            pVertex->m_cDirection = g_cOut * pVertex->m_cTrans;

            // Computed as CVec3's operator*(float) and operator+ would: each component rounded to float.
            const CVec3* pPos = &cBasePos;
            CVec3 cNextPos;
            if (pLastVertex != nullptr) {
                CVec3 cStep;
                cStep.m_afData[0] = fSegmentLength * pLastVertex->m_cDirection.m_afData[0];
                cStep.m_afData[1] = fSegmentLength * pLastVertex->m_cDirection.m_afData[1];
                cStep.m_afData[2] = fSegmentLength * pLastVertex->m_cDirection.m_afData[2];
                cNextPos.m_afData[0] = cStep.m_afData[0] + pLastVertex->m_cPos.m_afData[0];
                cNextPos.m_afData[1] = cStep.m_afData[1] + pLastVertex->m_cPos.m_afData[1];
                cNextPos.m_afData[2] = cStep.m_afData[2] + pLastVertex->m_cPos.m_afData[2];
                pPos = &cNextPos;
            }
            pVertex->m_cPos = *pPos;

            float fLocalCrossSectionWeight = fWindWeight;
            if (nLevel <= m_nWeightLevel) {
                pVertex->m_fWindWeight = ComputeVertexWeight(nLevel, fProgress, fLocalVertexFlexibility);
                fLocalCrossSectionWeight = pVertex->m_fWindWeight;
            }
            else if (m_bPropagateFlexibility) {
                pVertex->m_fWindWeight = ComputeHighLevelVertexWeight(fWindWeight, fLocalVertexFlexibility);
                fLocalCrossSectionWeight = pVertex->m_fWindWeight;
            }
            if (!bFrond) {
                float fCombinedProgress = 0.0f;
                if (nLevel == 0) {
                    // Compares a float copy of fProgress; when it is not below m_fFirstBranch, or the compare is
                    // unordered, the quotient is taken.
                    float fTrunkProgress = fProgress;
                    if (fTrunkProgress < sInfo.m_fFirstBranch) {
                        fCombinedProgress = 1.0f;
                    }
                    else {
                        fCombinedProgress =
                            static_cast<double>(sInfo.m_fFirstBranch) * -1.0 / (1.0 - sInfo.m_fFirstBranch);
                    }
                }
                else {
                    fCombinedProgress = fCombinedPercentOfParent;
                }
                BuildCrossSection(
                    pVertex, fProgress, fCombinedProgress, m_usCrossSectionSegments, pBranchGeometry, cTile,
                    fLocalCrossSectionWeight, nWindGroup, fTileOffset, nLevel
                );
            }
            else {
                m_pFrondEngine->AddGuideVertex(
                    pVertex->m_cPos, pVertex->m_cTrans, fLocalCrossSectionWeight, nWindGroup
                );
            }

            pLastVertex = pVertex;
            fRunningLength += fSegmentLength;
            m_pVertices[i].m_fRunningLength = fRunningLength;
        }

        if (bFrond) {
            m_pFrondEngine->EndGuide(fFirstRadius);
        }
        else {
            pBranchGeometry->m_nVertexCounter = static_cast<uint16_t>(m_nStartingOffset);
            ComputeBranchNormals(pBranchGeometry, m_usCrossSectionSegments);
        }

        // Truncated through _ftol2_sse.
        int32_t nNumChildBranches = static_cast<int32_t>(static_cast<double>(sInfo.m_fFrequency) / fSize * fLength);
        ++nLevel;
        bool bChildrenAreBuds = nLevel >= static_cast<int32_t>(m_vBranchInfo.size() - 1);
        if (!bChildrenAreBuds || m_bComputeLeaves) {
            for (int32_t i = 0; i < nNumChildBranches; ++i) {
                SIdvBranch sBranch;
                sBranch.m_nPreVertexIndex = 0;
                sBranch.m_fPercent = 0.0f;
                sBranch.m_pBranch = nullptr;
                if (!bChildrenAreBuds) {
                    nSeed += 3;
                    m_cRandom.Reseed(nSeed);
                }

                float fPercent = 0.0f;
                if (!bChildrenAreBuds && i == 0) {
                    // Two VecInterpolate(float) computations, each start through a float copy: 0.95f (the upper bound)
                    // first, then 0.85f.
                    float fStart95 = sInfo.m_fFirstBranch;
                    float fUpper = (static_cast<double>(sInfo.m_fLastBranch) - fStart95) * 0.95f + fStart95;
                    float fStart85 = sInfo.m_fFirstBranch;
                    float fLower = (static_cast<double>(sInfo.m_fLastBranch) - fStart85) * 0.85f + fStart85;
                    fPercent = m_cRandom.GetUniform(fLower, fUpper);
                }
                else {
                    fPercent = m_cRandom.GetUniform(sInfo.m_fFirstBranch, sInfo.m_fLastBranch);
                }
                float fChildGrowthPos = fPercent * fLength;
                FillBranch(sBranch, fChildGrowthPos);

                // Interpolates as VecInterpolate(float) does, with the start read through a float copy.
                float fWeightAtChild = fWindWeight;
                if (nLevel - 1 == m_nWeightLevel || m_bPropagateFlexibility) {
                    float fWeightStart = m_pVertices[sBranch.m_nPreVertexIndex].m_fWindWeight;
                    fWeightAtChild = static_cast<double>(sBranch.m_fPercent) *
                            (static_cast<double>(m_pVertices[sBranch.m_nPreVertexIndex + 1].m_fWindWeight) -
                             fWeightStart) +
                        fWeightStart;
                }

                if (!bChildrenAreBuds) {
                    m_cRandom.Reseed(nSeed);
                    (void)m_cRandom.GetUniform(0.0f, 100.0f);
                }

                CVec3 cPos = VecInterpolate(
                    m_pVertices[sBranch.m_nPreVertexIndex].m_cPos, m_pVertices[sBranch.m_nPreVertexIndex + 1].m_cPos,
                    sBranch.m_fPercent
                );
                float fProfilePercent = 1.0f;
                if (sInfo.m_fFirstBranch != sInfo.m_fLastBranch) {
                    fProfilePercent = (static_cast<double>(fPercent) - sInfo.m_fFirstBranch) /
                        (static_cast<double>(sInfo.m_fLastBranch) - sInfo.m_fFirstBranch);
                }
                if (bChildrenAreBuds) {
                    ComputeBud(
                        this, fSize, nLevel, cPos, fProfilePercent, m_pVertices[sBranch.m_nPreVertexIndex].m_cTrans,
                        m_pVertices[sBranch.m_nPreVertexIndex].m_cDirection, vLeaves, fWeightAtChild, nWindGroup
                    );
                }
                else {
                    // Interpolates as VecInterpolate(float) does, with the start read through a float copy.
                    float fRadiusStart = m_pVertices[sBranch.m_nPreVertexIndex].m_fRadius;
                    float fRadiusOfParentAtBranch = static_cast<double>(sBranch.m_fPercent) *
                            (static_cast<double>(m_pVertices[sBranch.m_nPreVertexIndex + 1].m_fRadius) - fRadiusStart) +
                        fRadiusStart;
                    void* pBranchMemory = ourMalloc(sizeof(CIdvBranch));
                    CIdvBranch* pChild = nullptr;
                    if (pBranchMemory != nullptr) {
                        pChild = ::new (pBranchMemory) CIdvBranch(this);
                    }
                    sBranch.m_pBranch = pChild;
                    float fChildCombinedPercent =
                        (1.0 - fCombinedPercentOfParent) * fProfilePercent + fCombinedPercentOfParent;
                    if (nLevel <= 1) {
                        fChildCombinedPercent = fChildCombinedPercent * fChildCombinedPercent;
                    }
                    sBranch.m_pBranch->Compute(
                        nSeed, fSize, nLevel, cPos, fProfilePercent, fChildCombinedPercent,
                        m_pVertices[sBranch.m_nPreVertexIndex].m_cTrans,
                        m_pVertices[sBranch.m_nPreVertexIndex].m_cDirection, pBranchGeometry, vLeaves, fWeightAtChild,
                        nWindGroup, fRadiusOfParentAtBranch
                    );

                    if (!m_pFrondEngine->Enabled() || nLevel < m_pFrondEngine->GetLevel()) {
                        m_vChildren.push_back(sBranch);
                    }
                    else {
                        CIdvBranch* pDelete = sBranch.m_pBranch;
                        if (pDelete != nullptr) {
                            pDelete->~CIdvBranch();
                            ourFree(pDelete);
                        }
                        sBranch.m_pBranch = nullptr;
                    }
                }
            }
        }

        if (bChildrenAreBuds) {
            m_vLocalLeaves.clear();
        }
        ComputeVolume();
    }
}

// 1 - fProgress * fFlexibility on the weight level, else 1; the product stays on the x87 stack.
float CIdvBranch::ComputeVertexWeight(int32_t nLevel, float fProgress, float fFlexibility) {
    float fWeight = 1.0f;
    if (nLevel == m_nWeightLevel) {
        fWeight = fWeight - static_cast<double>(fProgress) * fFlexibility;
    }
    return fWeight;
}

void CIdvBranch::BuildBlossomVectors() {
    CIdvBranch::m_vBlossoms.clear();
    CIdvBranch::m_vNonBlossoms.clear();
    for (uint32_t i = 0; i < CIdvBranch::m_pLeafInfo->m_vLeafTextures.size(); ++i) {
        if (CIdvBranch::m_pLeafInfo->m_vLeafTextures[i].m_bBlossom) {
            int32_t _Val = 2 * i;
            CIdvBranch::m_vBlossoms.push_back(_Val);
            int32_t v3 = 2 * i + 1;
            CIdvBranch::m_vBlossoms.push_back(v3);
        }
        else {
            int32_t v2 = 2 * i;
            CIdvBranch::m_vNonBlossoms.push_back(v2);
            int32_t uExitCode = 2 * i + 1;
            CIdvBranch::m_vNonBlossoms.push_back(uExitCode);
        }
    }
}

void CIdvBranch::ComputeFlareEntries(const SIdvBranchInfo& sInfo) {
    if (sInfo.m_nNumFlares) {
        float fInitialAngle = GetRandom(0.0f, c_fTwoPi);
        float fBalanceAngle = c_fTwoPi / static_cast<double>(sInfo.m_nNumFlares);
        for (int32_t i = 0; i < sInfo.m_nNumFlares; ++i) {
            SIdvBranchFlare _Val;
            _Val.m_fAngle = 0.0f;
            _Val.m_fRadialInfluence = 0.0f;
            _Val.m_fRadialExponent = 1.0f;
            _Val.m_fLengthInfluence = 0.0f;
            _Val.m_fLengthExponent = 1.0f;
            _Val.m_fDistance = 0.0f;
            float fMin = sInfo.m_fFlareBalance * fBalanceAngle;
            double v9 = static_cast<double>(i);
            double Random_ = GetRandom(fMin, fBalanceAngle);
            _Val.m_fAngle = Random_ * v9 + fInitialAngle;
            if (c_fTwoPi < static_cast<double>(_Val.m_fAngle)) {
                _Val.m_fAngle = _Val.m_fAngle - c_fTwoPi;
            }
            _Val.m_fRadialExponent = sInfo.m_fRadialExponent;
            _Val.m_fLengthExponent = sInfo.m_fLengthExponent;
            float v8 = sInfo.m_fRadialInfluenceVariance + sInfo.m_fRadialInfluence;
            float v7 = sInfo.m_fRadialInfluence - sInfo.m_fRadialInfluenceVariance;
            float v12 = GetRandom(v7, v8);
            _Val.m_fRadialInfluence = v12 / 57.295780181884766;
            float v6 = sInfo.m_fLengthDistance + sInfo.m_fLengthVariance;
            float v5 = sInfo.m_fLengthDistance - sInfo.m_fLengthVariance;
            _Val.m_fLengthInfluence = GetRandom(v5, v6);
            float v4 = sInfo.m_fRadialDistance + sInfo.m_fRadialVariance;
            float v3 = sInfo.m_fRadialDistance - sInfo.m_fRadialVariance;
            _Val.m_fDistance = GetRandom(v3, v4);
            m_vFlares.push_back(_Val);
        }
    }
}

void CIdvBranch::FillBranch(SIdvBranch& sBranch, float fChildGrowthPos) const {
    int32_t i;
    if (m_pVertices && (m_nVertexCount >= 2)) {
        (&sBranch)->m_nPreVertexIndex = 0;
        for (i = 1; i < m_nVertexCount; ++i) {
            if (m_pVertices[i].m_fRunningLength > ((double)fChildGrowthPos)) {
                (&sBranch)->m_nPreVertexIndex = i - 1;
                break;
            }
        }
        (&sBranch)->m_fPercent =
            ((fChildGrowthPos - m_pVertices[(&sBranch)->m_nPreVertexIndex].m_fRunningLength) /
             (m_pVertices[(&sBranch)->m_nPreVertexIndex + 1].m_fRunningLength -
              m_pVertices[(&sBranch)->m_nPreVertexIndex].m_fRunningLength));
    }
}

// The segment length is the larger of 0.01 and the length per segment.
void CIdvBranch::ComputeBud(
    const CIdvBranch* pParent,
    float fSize,
    int32_t nLevel,
    const CVec3& cBasePos,
    float fPercentOfParent,
    const CRotTransform& cBaseTransform,
    const CVec3& cZAxis,
    LeafPtrVector& vLeaves,
    float fWindWeight,
    int32_t nWindGroup
) const {
    const SIdvBranchInfo* sInfo = m_vBranchInfo[static_cast<uint32_t>(nLevel)];
    float c_fStartAngle = 60.0f;
    SIdvBranchVertex acVertices[2];
    SIdvBranchVertex* pConstruct = acVertices;
    for (int32_t nCount = 2; --nCount >= 0; ++pConstruct) {
        ::new (pConstruct) SIdvBranchVertex;
    }
    float fLength = sInfo->m_pLength->Evaluate(fPercentOfParent) * fSize;
    float fMinLength = 0.01f;
    float fLengthPerSegment = fLength / static_cast<double>(sInfo->m_nSegments);
    float fSegmentLength = (fMinLength > fLengthPerSegment) ? fMinLength : fLengthPerSegment;
    float fAzimuth = m_cRandom.GetUniform(-180.0f, 180.0f);
    CVec3 cRotAxis = cBaseTransform * cZAxis;
    SIdvBranchVertex* pFirstVertex = &acVertices[0];
    pFirstVertex->m_cPos = cBasePos;
    pFirstVertex->m_cTrans = cBaseTransform;
    pFirstVertex->m_cTrans.RotateAxis(fAzimuth, cRotAxis);
    pFirstVertex->m_cTrans.RotateY(c_fStartAngle);
    pFirstVertex->m_cDirection = g_cOut * pFirstVertex->m_cTrans;
    SIdvBranchVertex* pVertex = &acVertices[1];
    CVec3 cScaled;
    CVec3 cSum;
    pVertex->m_cPos = *pFirstVertex->m_cPos.Add(&cSum, pFirstVertex->m_cDirection.Scale(&cScaled, fSegmentLength));
    CVec3 cNormal;
    pVertex->m_cPos.Diff(&cNormal, &pFirstVertex->m_cPos);
    cNormal.Normalize();
    CVec3 cParentDir;
    cParentDir.m_afData[2] = 0.0f;
    cParentDir.m_afData[1] = 0.0f;
    cParentDir.m_afData[0] = 0.0f;
    if (pParent && pParent->m_nVertexCount) {
        CVec3 cBase = pParent->m_pVertices->m_cPos;
        if (m_pLeafInfo->m_nBlossomLevel) {
            int32_t nLevels = 0;
            const CIdvBranch* pBranch = pParent;
            while (pBranch && nLevels < m_pLeafInfo->m_nBlossomLevel) {
                pBranch = pBranch->m_pParent;
                ++nLevels;
            }
            if (pBranch) {
                cBase = pBranch->m_pVertices->m_cPos;
            }
        }
        CVec3 cDiff;
        cParentDir = *pVertex->m_cPos.Diff(&cDiff, &cBase);
    }
    else {
        cParentDir = cNormal;
    }
    cParentDir.Normalize();
    MakeLeaf(pVertex->m_cPos, fPercentOfParent, this, cNormal, cParentDir, fWindWeight, nWindGroup, vLeaves);
}

void CIdvBranch::BuildCrossSection(
    const SIdvBranchVertex* pVertex,
    float fBranchProgress,
    float fCombinedBranchProgress,
    int32_t nSegments,
    CIndexedGeometry* pBranchGeometry,
    const CVec3& cTile,
    float fWindWeight,
    int32_t nWindGroup,
    float fTileOffset,
    int32_t nLevel
) const {
    CVec3 cNormal;
    float afColor[4];
    float fcolorVal;
    float fProgress = 0.0f;
    float fProgressInc = 1.0 / static_cast<double>(nSegments);
    if (CIdvBranch::m_pLeafInfo) {
        fcolorVal = (1.0 - fCombinedBranchProgress) * fBranchProgress + fCombinedBranchProgress;
        if (!nLevel) {
            fcolorVal = fcolorVal * fcolorVal;
        }
        fcolorVal = fcolorVal * CIdvBranch::m_pLeafInfo->m_fBranchDimmingScalar +
            (1.0 - CIdvBranch::m_pLeafInfo->m_fBranchDimmingScalar);
    }
    else {
        fcolorVal = 1.0f;
    }
    afColor[2] = fcolorVal;
    afColor[1] = fcolorVal;
    afColor[0] = fcolorVal;
    afColor[3] = 1.0f;
    for (int32_t i = 0; i <= nSegments; ++i) {
        CVec3 cTangent;
        float afTexCoords[2];
        float afOriginalPos[3];
        CVec3 cVec;
        float fAngle = c_fTwoPi * fProgress;
        afTexCoords[0] = fProgress * cTile.m_afData[0];
        afTexCoords[0] = cTile.m_afData[2] * fBranchProgress + afTexCoords[0];
        afTexCoords[1] = (fBranchProgress + fTileOffset) * cTile.m_afData[1];
        pBranchGeometry->AddVertexTexCoord0(afTexCoords, -1);
        float v21 = std::cos(static_cast<double>(fAngle));
        float fCosine = v21;
        float v20 = std::sin(static_cast<double>(fAngle));
        float fSine = v20;
        cVec.m_afData[2] = 0.0f;
        cVec.m_afData[1] = 0.0f;
        cVec.m_afData[0] = 0.0f;
        float v19 = fCosine * pVertex->m_cTrans.m_afData[1][0] + v20 * pVertex->m_cTrans.m_afData[2][0];
        cVec.m_afData[0] = v19;
        float v18 = fCosine * pVertex->m_cTrans.m_afData[1][1] + v20 * pVertex->m_cTrans.m_afData[2][1];
        cVec.m_afData[1] = v18;
        float v17 = fCosine * pVertex->m_cTrans.m_afData[1][2] + v20 * pVertex->m_cTrans.m_afData[2][2];
        cVec.m_afData[2] = v17;
        float v28 = c_fHalfPi + fAngle;
        float v16 = std::cos(static_cast<double>(v28));
        fCosine = v16;
        float v27 = c_fHalfPi + fAngle;
        float v15 = std::sin(static_cast<double>(v27));
        fSine = v15;
        cTangent.m_afData[2] = 0.0f;
        cTangent.m_afData[1] = 0.0f;
        cTangent.m_afData[0] = 0.0f;
        float v14 = fCosine * pVertex->m_cTrans.m_afData[1][0] + v15 * pVertex->m_cTrans.m_afData[2][0];
        cTangent.m_afData[0] = v14;
        float uExitCode = fCosine * pVertex->m_cTrans.m_afData[1][1] + v15 * pVertex->m_cTrans.m_afData[2][1];
        cTangent.m_afData[1] = uExitCode;
        float hProcess = fCosine * pVertex->m_cTrans.m_afData[1][2] + v15 * pVertex->m_cTrans.m_afData[2][2];
        cTangent.m_afData[2] = hProcess;
        pBranchGeometry->AddVertexTangent(cTangent.m_afData);
        CVec3* pBinormal = &(cNormal = cTangent.Cross(cVec));
        pBranchGeometry->AddVertexBinormal(pBinormal->m_afData);
        float fFlareAdjust = 1.0f;
        float fFlareAdditions = 0.0f;
        for (uint32_t j = 0; j < m_vFlares.size(); ++j) {
            double v11 = m_vFlares[j].Distance(fAngle, fBranchProgress);
            fFlareAdditions = v11 + fFlareAdditions;
        }
        fFlareAdjust = fFlareAdjust + fFlareAdditions;
        afOriginalPos[0] = cVec.m_afData[0] * pVertex->m_fRadius + pVertex->m_cPos.m_afData[0u];
        afOriginalPos[1] = cVec.m_afData[1] * pVertex->m_fRadius + pVertex->m_cPos.m_afData[1];
        afOriginalPos[2] = cVec.m_afData[2] * pVertex->m_fRadius + pVertex->m_cPos.m_afData[2];
        if (fFlareAdjust != 1.0) {
            float afPos[3];
            CVec3 cNewNormal;
            afPos[0] = cVec.m_afData[0] * pVertex->m_fRadius * fFlareAdjust + pVertex->m_cPos.m_afData[0u];
            afPos[1] = cVec.m_afData[1] * pVertex->m_fRadius * fFlareAdjust + pVertex->m_cPos.m_afData[1];
            afPos[2] = cVec.m_afData[2] * pVertex->m_fRadius * fFlareAdjust + pVertex->m_cPos.m_afData[2];
            float v23 = afPos[0] - afOriginalPos[0];
            float v24 = afPos[1] - afOriginalPos[1];
            float v25 = afPos[2] - afOriginalPos[2];
            cNewNormal.m_afData[0] = v23;
            cNewNormal.m_afData[1] = v24;
            cNewNormal.m_afData[2] = v25;
            cNewNormal.Normalize();
            cVec = cNewNormal;
            rt::memcpy(afOriginalPos, afPos, sizeof(afOriginalPos));
        }
        pBranchGeometry->AddVertexCoord(afOriginalPos);
        pBranchGeometry->AddVertexColor(afColor);
        if (pBranchGeometry->m_bVertexWeighting) {
            pBranchGeometry->AddVertexWind(fWindWeight, nWindGroup);
        }
        ++pBranchGeometry->m_nVertexCounter;
        fProgress = fProgress + fProgressInc;
    }
}

// Returns fBaseWeight - fBaseWeight * fFlexibility, rounded to float; this is unused.
float CIdvBranch::ComputeHighLevelVertexWeight(float fBaseWeight, float fFlexibility) {
    return (float)((fFlexibility * (0.0 - fBaseWeight)) + fBaseWeight);
}

void CIdvBranch::BuildBranchVector(std::vector<CIdvBranch*, std::allocator<CIdvBranch*>>& vAllBranches) {
    CIdvBranch* pThis = this;
    vAllBranches.push_back(pThis);
    for (size_t i = 0; i < m_vChildren.size(); ++i) {
        m_vChildren[i].m_pBranch->BuildBranchVector(vAllBranches);
    }
}

// The loops compare signed, on m_nVertexCount and on the zero-extended usSegments. Each difference component is stored
// as a float, and the normal is the first direction's Cross with the second.
void CIdvBranch::ComputeBranchNormals(CIndexedGeometry* pBranchGeometry, uint16_t usSegments) {
    for (int32_t i = 0; i < m_nVertexCount; ++i) {
        int32_t nBase = (usSegments + 1) * i;
        for (int32_t j = 0; j <= usSegments; ++j) {
            int32_t nLeft = j == 0 ? nBase + usSegments - 1 : nBase + j - 1;
            int32_t nRight = j == usSegments ? nBase + 1 : nBase + j + 1;
            nLeft += m_nStartingOffset;
            nRight += m_nStartingOffset;
            const float* pLeft = pBranchGeometry->GetVertexCoord(nLeft);
            const float* pRight = pBranchGeometry->GetVertexCoord(nRight);
            CVec3 cAround;
            cAround.m_afData[0] = pRight[0] - pLeft[0];
            cAround.m_afData[1] = pRight[1] - pLeft[1];
            cAround.m_afData[2] = pRight[2] - pLeft[2];
            cAround.Normalize();
            int32_t nBelow = i == 0 ? nBase + j : (i - 1) * (usSegments + 1) + j;
            int32_t nAbove = i == m_nVertexCount - 1 ? nBase + j : (i + 1) * (usSegments + 1) + j;
            nBelow += m_nStartingOffset;
            nAbove += m_nStartingOffset;
            const float* pBelow = pBranchGeometry->GetVertexCoord(nBelow);
            const float* pAbove = pBranchGeometry->GetVertexCoord(nAbove);
            CVec3 cAlong;
            cAlong.m_afData[0] = pAbove[0] - pBelow[0];
            cAlong.m_afData[1] = pAbove[1] - pBelow[1];
            cAlong.m_afData[2] = pAbove[2] - pBelow[2];
            cAlong.Normalize();
            CVec3 cNormal = cAround.Cross(cAlong);
            pBranchGeometry->AddVertexNormal(cNormal.m_afData);
            ++pBranchGeometry->m_nVertexCounter;
        }
    }
}

void CIdvBranch::ComputeLod(int32_t nDiscreteLodLevel, CIndexedGeometry* pGeometry) {
    if (this->m_usCrossSectionSegments >= 2u) {
        double v3;
        int32_t m_usCrossSectionSegments = this->m_usCrossSectionSegments;
        if (this->m_usCrossSectionSegments) {
            v3 = static_cast<double>(this->m_usCrossSectionSegments) / static_cast<double>(m_usCrossSectionSegments);
        }
        else {
            v3 = 1.0;
        }
        float v8 = v3;
        int32_t nTargetCrossSection = m_nVertexCount;
        int32_t nIndexCount = (2 * m_usCrossSectionSegments + 4) * (nTargetCrossSection - 1);
        // Allocates new uint16_t[nIndexCount] with a checked multiply that asks for 0xFFFFFFFF bytes when twice the
        // count does not fit in 32 bits.
        uint32_t uiCount = static_cast<uint32_t>(nIndexCount);
        uint32_t uiBytes = (uiCount >> 31) != 0 ? 0xFFFFFFFFu : 2 * uiCount;
        uint16_t* pStrip = static_cast<uint16_t*>(ourMalloc(uiBytes));
        pGeometry->AddStrip(nDiscreteLodLevel, pStrip, nIndexCount);
        int32_t v14 = 0;
        float v11 = 0.0f;
        for (int32_t i = 0; i < nTargetCrossSection - 1; ++i) {
            int32_t v6 = static_cast<int32_t>(v11);
            pStrip[v14] = v6 + (static_cast<uint16_t>(m_nStartingOffset) + this->m_usCrossSectionSegments + 1);
            int32_t v15 = v14 + 1;
            pStrip[v15] = static_cast<uint16_t>(m_nStartingOffset) + v6;
            int32_t v16 = v15 + 1;
            float v12 = v11 + v8;
            for (int32_t j = 1; j < m_usCrossSectionSegments; ++j) {
                pStrip[v16] = static_cast<int32_t>(v12) +
                    (static_cast<uint16_t>(m_nStartingOffset) + this->m_usCrossSectionSegments + 1);
                int32_t v17 = v16 + 1;
                pStrip[v17] = static_cast<uint16_t>(m_nStartingOffset) + static_cast<int32_t>(v12);
                v16 = v17 + 1;
                v12 = v12 + v8;
            }
            pStrip[v16] =
                this->m_usCrossSectionSegments + (v6 + (m_nStartingOffset + this->m_usCrossSectionSegments + 1));
            int32_t v18 = v16 + 1;
            pStrip[v18++] = this->m_usCrossSectionSegments + (v6 + m_nStartingOffset);
            v11 = static_cast<float>(v6 + this->m_usCrossSectionSegments + 1);
            pStrip[v18++] = v6 + (m_nStartingOffset + this->m_usCrossSectionSegments + 1);
            pStrip[v18] = v6 + (m_nStartingOffset + this->m_usCrossSectionSegments + 1);
            v14 = v18 + 1;
        }
        ++pGeometry->m_nStripCounter;
    }
}

// Climbs the parents only where the blossom level is 0, so that the level test ends the climb before its first step and
// the test percent is pParent's own.
bool CIdvBranch::IsBlossom(const CIdvBranch* pParent, float fPercentOfParent) const {
    bool bIsBlossom = false;
    float fTestPercent = fPercentOfParent;
    if (m_pLeafInfo->m_nBlossomLevel == 0) {
        int32_t nLevels = 1;
        const CIdvBranch* pBranch = pParent;
        while (pBranch && pBranch->m_pParent && nLevels < m_pLeafInfo->m_nBlossomLevel) {
            pBranch = pBranch->m_pParent;
            ++nLevels;
        }
        if (pBranch) {
            fTestPercent = pBranch->m_fPercentOfParent;
        }
    }
    if (m_pLeafInfo->m_fBlossomDistance < fTestPercent) {
        if (m_pLeafInfo->m_fBlossomWeighting >= m_cRandom.GetUniform(0.0f, 1.0f)) {
            bIsBlossom = true;
        }
    }
    return bIsBlossom;
}

// The random indices are the float draws converted to unsigned.
void CIdvBranch::MakeLeaf(
    const CVec3& cPos,
    float fPercent,
    const CIdvBranch* pParent,
    const CVec3& cNormal,
    const CVec3& cParentDir,
    float fWindWeight,
    int32_t nWindGroup,
    LeafPtrVector& vLeaves
) const {
    std::vector<SIdvLeafTexture, std::allocator<SIdvLeafTexture>>* pAllTextures = &m_pLeafInfo->m_vLeafTextures;
    if (!pAllTextures->size()) {
        return;
    }
    int32_t nTextureIndex = 0;
    uint32_t uiBlossoms = m_vBlossoms.size();
    uint32_t uiNonBlossoms = m_vNonBlossoms.size();
    bool bIsBlossom;
    if (uiBlossoms) {
        bIsBlossom = IsBlossom(pParent, fPercent);
    }
    else {
        bIsBlossom = false;
    }
    if (bIsBlossom) {
        uint32_t nIndex = 0;
        if (uiBlossoms > 1) {
            nIndex = static_cast<uint32_t>(static_cast<int64_t>(m_cRandom.GetUniform(0.0f, 100000.0f))) % uiBlossoms;
        }
        nTextureIndex = m_vBlossoms[nIndex];
    }
    else {
        uint32_t nIndex = 0;
        if (uiNonBlossoms > 1) {
            float fRandom = m_cRandom.GetUniform(0.0f, 1e+06f);
            nIndex = static_cast<uint32_t>(static_cast<int64_t>(fRandom)) % uiNonBlossoms;
            nTextureIndex = m_vNonBlossoms[nIndex];
        }
        else {
            if (!uiNonBlossoms) {
                return;
            }
            nTextureIndex = m_vNonBlossoms[0];
        }
    }
    bool bValid = true;
    SIdvLeafInfo::ECollisionType eCollisionType = m_pLeafInfo->m_eCollisionType;
    if (eCollisionType == SIdvLeafInfo::BRANCH) {
        bValid = RoomForLeaf(cPos, nTextureIndex / 2, m_vLocalLeaves);
    }
    else if (eCollisionType == SIdvLeafInfo::TREE) {
        bValid = RoomForLeaf(cPos, nTextureIndex / 2, vLeaves);
    }
    if (!bValid) {
        return;
    }
    float fColorScale = 1.0f;
    if (m_pLeafInfo->m_bDimming) {
        float fDimPercent = ComputeDimPercent(pParent);
        fDimPercent = (1.0f - fDimPercent) * fPercent + fDimPercent;
        fDimPercent = fDimPercent * fDimPercent * fDimPercent;
        float fScalar = 1.0f - m_pLeafInfo->m_fDimmingScalar;
        fColorScale = fScalar * (1.0f - fDimPercent) + fDimPercent;
    }
    CBillboardLeaf* pLeaf;
    void* pMemory = ourMalloc(sizeof(CBillboardLeaf));
    if (pMemory) {
        int32_t nAngleIndex =
            static_cast<int32_t>(m_cRandom.GetUniform(0.0f, 10000.0f)) % m_pLeafInfo->m_nNumRockingGroups;
        pLeaf = ::new (pMemory) CBillboardLeaf(
            cPos, static_cast<uint16_t>(static_cast<int32_t>(fColorScale * 255.0)), nAngleIndex, fWindWeight, nWindGroup
        );
    }
    else {
        pLeaf = nullptr;
    }
    vLeaves.push_back(pLeaf);
    if (m_pLeafInfo->m_eCollisionType == SIdvLeafInfo::BRANCH) {
        m_vLocalLeaves.push_back(pLeaf);
    }
    int32_t nTexture = nTextureIndex / 2;
    const SIdvLeafTexture& sTexture = m_pLeafInfo->m_vLeafTextures[static_cast<uint32_t>(nTexture)];
    pLeaf->m_ucTextureIndex = static_cast<uint8_t>(nTextureIndex);
    CVec3 cAdjustedNormal;
    cAdjustedNormal.m_afData[2] = 0.0f;
    cAdjustedNormal.m_afData[1] = 0.0f;
    cAdjustedNormal.m_afData[0] = 0.0f;
    if (!(cNormal.Magnitude() < 0.0001)) {
        float fVariance = sTexture.m_fColorVariance;
        CVec3 cDiff;
        CVec3 cScaled;
        CVec3 cSum;
        cParentDir.Add(&cSum, cNormal.Diff(&cDiff, &cParentDir)->Scale(&cScaled, fVariance));
        cAdjustedNormal = cSum;
    }
    else {
        cAdjustedNormal = cParentDir;
    }
    cAdjustedNormal.Normalize();
    pLeaf->m_cNormal = cAdjustedNormal;
    CVec3 cTangent = cAdjustedNormal;
    pLeaf->m_cTangent = cTangent;
    CVec3 cBinormal = cTangent.Cross(cAdjustedNormal);
    cBinormal.Normalize();
    pLeaf->m_cBinormal = cBinormal;
    float fOffset = m_cRandom.GetUniform(-sTexture.m_fColorVariance, sTexture.m_fColorVariance);
    float fRed = fOffset + sTexture.m_cColor.m_afData[0];
    float fGreen = fOffset + sTexture.m_cColor.m_afData[1];
    float fBlue = fOffset + sTexture.m_cColor.m_afData[2];
    CVec3 cColor;
    cColor.m_afData[0] = fRed;
    cColor.m_afData[1] = fGreen;
    cColor.m_afData[2] = fBlue;
    pLeaf->SetColor(cColor, true);
}

bool CIdvBranch::RoomForLeaf(const CVec3& cPos, int32_t nTextureIndex, LeafPtrVector& vLeaves) const {
    bool bIsRoom = true;
    if (!m_pLeafInfo->m_vLeafTextures.size()) {
        return false;
    }
    const SIdvLeafTexture& sTexture = m_pLeafInfo->m_vLeafTextures[static_cast<uint32_t>(nTextureIndex)];
    float fLargerSide;
    if (!(sTexture.m_cSizeUsed.m_afData[1] < sTexture.m_cSizeUsed.m_afData[0])) {
        fLargerSide = sTexture.m_cSizeUsed.m_afData[1];
    }
    else {
        fLargerSide = sTexture.m_cSizeUsed.m_afData[0];
    }
    fLargerSide = fLargerSide * m_pLeafInfo->m_fSpacingTolerance;
    for (uint32_t i = 0; i < vLeaves.size() && bIsRoom; ++i) {
        const CVec3& cOther = vLeaves[i]->m_cPosition;
        if (cPos.m_afData[0] < cOther.m_afData[0] + fLargerSide &&
            cPos.m_afData[0] > cOther.m_afData[0] - fLargerSide &&
            cPos.m_afData[1] < cOther.m_afData[1] + fLargerSide &&
            cPos.m_afData[1] > cOther.m_afData[1] - fLargerSide &&
            cPos.m_afData[2] < cOther.m_afData[2] + fLargerSide &&
            cPos.m_afData[2] > cOther.m_afData[2] - fLargerSide) {
            bIsRoom = false;
        }
    }
    return bIsRoom;
}

void CIdvBranch::SortBranchVector(std::vector<CIdvBranch*, std::allocator<CIdvBranch*>>& vAllBranches) {
    std::sort(vAllBranches.begin(), vAllBranches.end(), CBranchVolumeSorter{0});
}

// Sorts by descending fuzzy volume; the comparison is false when either volume is NaN.
bool CBranchVolumeSorter::operator()(const CIdvBranch* pLeft, const CIdvBranch* pRight) const {
    return pLeft->m_fFuzzyVolume > pRight->m_fFuzzyVolume;
}

// The point fPercent of the way from cStart to cEnd: cStart plus the difference scaled by fPercent, through Diff,
// Scale and Add in that order.
CVec3 VecInterpolate(const CVec3& cStart, const CVec3& cEnd, float fPercent) {
    CVec3 cDelta;
    cEnd.Diff(&cDelta, &cStart);
    CVec3 cScaled;
    CVec3 cResult;
    cStart.Add(&cResult, cDelta.Scale(&cScaled, fPercent));
    return cResult;
}

// rand() over the double 32767.0, the range and the result each stored as a float.
float GetRandom(float fMin, float fMax) {
    float fUnit = rt::rand() / 32767.0;
    float fRange = fMax - fMin;
    float fResult = fUnit * fRange + fMin;
    return fResult;
}

float ComputeDimPercent(const CIdvBranch* pBranch) {
    if (!pBranch || !pBranch->m_pParent) {
        return 0.0f;
    }
    float fStart = ComputeDimPercent(pBranch->m_pParent);
    float fPercent = pBranch->m_fPercentOfParent;
    float fResult = (1.0f - fStart) * fPercent + fStart;
    return fResult;
}

CRotTransform::CRotTransform() : m_afData{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}} {}

// this * cTrans: each row of this times each column of cTrans. Every product and sum is computed in double and rounded
// only once per element. The result is built in a temporary and copied out through a reference return, which is not an
// id-expression and therefore cannot be elided.
CRotTransform CRotTransform::operator*(const CRotTransform& cTrans) const {
    CRotTransform cTemp;
    cTemp.~CRotTransform();
    ::new (&cTemp) CRotTransform;
    cTemp.m_afData[0][0] =
        (float)((double)m_afData[0][0] * cTrans.m_afData[0][0] + (double)m_afData[0][1] * cTrans.m_afData[1][0] +
                (double)m_afData[0][2] * cTrans.m_afData[2][0]);
    cTemp.m_afData[0][1] =
        (float)((double)m_afData[0][0] * cTrans.m_afData[0][1] + (double)m_afData[0][1] * cTrans.m_afData[1][1] +
                (double)m_afData[0][2] * cTrans.m_afData[2][1]);
    cTemp.m_afData[0][2] =
        (float)((double)m_afData[0][0] * cTrans.m_afData[0][2] + (double)m_afData[0][1] * cTrans.m_afData[1][2] +
                (double)m_afData[0][2] * cTrans.m_afData[2][2]);
    cTemp.m_afData[1][0] =
        (float)((double)m_afData[1][0] * cTrans.m_afData[0][0] + (double)m_afData[1][1] * cTrans.m_afData[1][0] +
                (double)m_afData[1][2] * cTrans.m_afData[2][0]);
    cTemp.m_afData[1][1] =
        (float)((double)m_afData[1][0] * cTrans.m_afData[0][1] + (double)m_afData[1][1] * cTrans.m_afData[1][1] +
                (double)m_afData[1][2] * cTrans.m_afData[2][1]);
    cTemp.m_afData[1][2] =
        (float)((double)m_afData[1][0] * cTrans.m_afData[0][2] + (double)m_afData[1][1] * cTrans.m_afData[1][2] +
                (double)m_afData[1][2] * cTrans.m_afData[2][2]);
    cTemp.m_afData[2][0] =
        (float)((double)m_afData[2][0] * cTrans.m_afData[0][0] + (double)m_afData[2][1] * cTrans.m_afData[1][0] +
                (double)m_afData[2][2] * cTrans.m_afData[2][0]);
    cTemp.m_afData[2][1] =
        (float)((double)m_afData[2][0] * cTrans.m_afData[0][1] + (double)m_afData[2][1] * cTrans.m_afData[1][1] +
                (double)m_afData[2][2] * cTrans.m_afData[2][1]);
    cTemp.m_afData[2][2] =
        (float)((double)m_afData[2][0] * cTrans.m_afData[0][2] + (double)m_afData[2][1] * cTrans.m_afData[1][2] +
                (double)m_afData[2][2] * cTrans.m_afData[2][2]);
    return static_cast<const CRotTransform&>(cTemp);
}

// The degrees are divided by the double 57.295780181884766 and rounded to float before _cos and _sin take them. RotateX
// and RotateZ are the same shape but different rotations, not copies of this one.
void CRotTransform::RotateY(float fAngle) {
    CRotTransform cRotMatrix;
    cRotMatrix.~CRotTransform();
    ::new (&cRotMatrix) CRotTransform;
    float fRadians = fAngle / 57.295780181884766;
    float fCosine = static_cast<float>(std::cos(static_cast<double>(fRadians)));
    fRadians = fAngle / 57.295780181884766;
    float fSine = static_cast<float>(std::sin(static_cast<double>(fRadians)));
    cRotMatrix.m_afData[0][0] = fCosine;
    cRotMatrix.m_afData[0][1] = 0.0f;
    cRotMatrix.m_afData[0][2] = -fSine;
    cRotMatrix.m_afData[1][0] = 0.0f;
    cRotMatrix.m_afData[1][1] = 1.0f;
    cRotMatrix.m_afData[1][2] = 0.0f;
    cRotMatrix.m_afData[2][0] = fSine;
    cRotMatrix.m_afData[2][1] = 0.0f;
    cRotMatrix.m_afData[2][2] = fCosine;
    *this = cRotMatrix * *this;
}

void CRotTransform::RotateX(float fAngle) {
    CRotTransform result;
    CRotTransform apMemory;
    apMemory.~CRotTransform();
    ::new (&apMemory) CRotTransform;
    float v4 = (float)(fAngle / 57.295780181884766);
    float v3 = (float)std::cos((double)v4);
    float v8 = v3;
    float v2 = (float)std::sin((double)v4);
    float v9 = v2;
    apMemory.m_afData[0][0] = (float)1.0;
    apMemory.m_afData[0][1] = (float)0.0;
    apMemory.m_afData[0][2] = (float)0.0;
    apMemory.m_afData[1][0] = (float)0.0;
    apMemory.m_afData[1][1] = v8;
    apMemory.m_afData[1][2] = v2;
    apMemory.m_afData[2][0] = (float)0.0;
    apMemory.m_afData[2][1] = (-v2);
    apMemory.m_afData[2][2] = v8;
    __builtin_memcpy(this, &(result = (&apMemory)->operator*(*this)), sizeof(CRotTransform));
}

// The rotation of fAngle degrees about the unit axis cAxis, multiplied into this from the left. The angle is divided by
// the double 57.295780181884766 to a float, the double sin then cos of it are taken, and 1 - cos is formed. Each
// element's products and sum are computed in double.
void CRotTransform::RotateAxis(float fAngle, CVec3 cAxis) {
    CRotTransform cRotation;
    cRotation.~CRotTransform();
    ::new (&cRotation) CRotTransform;
    float fRadians = fAngle / 57.295780181884766;
    float fSin = (float)std::sin(static_cast<double>(fRadians));
    float fCos = (float)std::cos(static_cast<double>(fRadians));
    float fOneMinusCos = 1.0f - fCos;
    float fX = cAxis.m_afData[0];
    float fY = cAxis.m_afData[1];
    float fZ = cAxis.m_afData[2];
    cRotation.m_afData[0][0] = (float)((double)fOneMinusCos * fX * fX + fCos);
    cRotation.m_afData[0][1] = (float)((double)fOneMinusCos * fX * fY + (double)fSin * fZ);
    cRotation.m_afData[0][2] = (float)((double)fOneMinusCos * fX * fZ - (double)fSin * fY);
    cRotation.m_afData[1][0] = (float)((double)fOneMinusCos * fX * fY - (double)fSin * fZ);
    cRotation.m_afData[1][1] = (float)((double)fOneMinusCos * fY * fY + fCos);
    cRotation.m_afData[1][2] = (float)((double)fOneMinusCos * fY * fZ + (double)fSin * fX);
    cRotation.m_afData[2][0] = (float)((double)fOneMinusCos * fX * fZ + (double)fSin * fY);
    cRotation.m_afData[2][1] = (float)((double)fOneMinusCos * fY * fZ - (double)fSin * fX);
    cRotation.m_afData[2][2] = (float)((double)fOneMinusCos * fZ * fZ + fCos);
    CRotTransform cResult = cRotation * *this;
    *this = cResult;
}

// The rotation of fYangle degrees about y then fZangle about z, multiplied into this from the left.
void CRotTransform::RotateYZ(float fYangle, float fZangle) {
    CRotTransform cRotation;
    cRotation.~CRotTransform();
    ::new (&cRotation) CRotTransform;
    float fYradians = fYangle / 57.295780181884766;
    float fZradians = fZangle / 57.295780181884766;
    float fCosY = (float)std::cos(static_cast<double>(fYradians));
    float fSinY = (float)std::sin(static_cast<double>(fYradians));
    float fCosZ = (float)std::cos(static_cast<double>(fZradians));
    float fSinZ = (float)std::sin(static_cast<double>(fZradians));
    cRotation.m_afData[0][0] = fCosY * fCosZ;
    cRotation.m_afData[0][1] = fCosY * fSinZ;
    cRotation.m_afData[0][2] = -fSinY;
    cRotation.m_afData[1][0] = -fSinZ;
    cRotation.m_afData[1][1] = fCosZ;
    cRotation.m_afData[1][2] = 0.0f;
    cRotation.m_afData[2][0] = fSinY * fCosZ;
    cRotation.m_afData[2][1] = fSinY * fSinZ;
    cRotation.m_afData[2][2] = fCosY;
    CRotTransform cResult = cRotation * *this;
    *this = cResult;
}

// Sets this to the rotation of fAngle degrees about the unit axis cAxis: RotateAxis's terms stored straight into the
// matrix, with no identity temporary and no multiply. Same sin, cos, cAxis-to-float-copies and 1 - cos sequence as
// there, and the same double computation of each element's products and sum before the single float store.
void CRotTransform::RotateAxisFromIdentity(float fAngle, CVec3 cAxis) {
    float fRadians = fAngle / 57.295780181884766;
    float fSin = (float)std::sin(static_cast<double>(fRadians));
    float fCos = (float)std::cos(static_cast<double>(fRadians));
    float fOneMinusCos = 1.0f - fCos;
    float fX = cAxis.m_afData[0];
    float fY = cAxis.m_afData[1];
    float fZ = cAxis.m_afData[2];
    m_afData[0][0] = (float)((double)fOneMinusCos * fX * fX + fCos);
    m_afData[0][1] = (float)((double)fOneMinusCos * fX * fY + (double)fSin * fZ);
    m_afData[0][2] = (float)((double)fOneMinusCos * fX * fZ - (double)fSin * fY);
    m_afData[1][0] = (float)((double)fOneMinusCos * fX * fY - (double)fSin * fZ);
    m_afData[1][1] = (float)((double)fOneMinusCos * fY * fY + fCos);
    m_afData[1][2] = (float)((double)fOneMinusCos * fY * fZ + (double)fSin * fX);
    m_afData[2][0] = (float)((double)fOneMinusCos * fX * fZ + (double)fSin * fY);
    m_afData[2][1] = (float)((double)fOneMinusCos * fY * fZ - (double)fSin * fX);
    m_afData[2][2] = (float)((double)fOneMinusCos * fZ * fZ + fCos);
}

// The vector this * cVec3: each component is a dot product of cVec3 with a row of the matrix. The factors accumulate x,
// y, z in that order, and each product and sum is kept in double precision, rounding once per component, as the game
// does.
CVec3 CRotTransform::operator*(const CVec3& cVec3) const {
    CVec3 cResult;
    cResult.m_afData[0] =
        (float)((double)cVec3.m_afData[0] * m_afData[0][0] + (double)cVec3.m_afData[1] * m_afData[0][1] +
                (double)cVec3.m_afData[2] * m_afData[0][2]);
    cResult.m_afData[1] =
        (float)((double)cVec3.m_afData[0] * m_afData[1][0] + (double)cVec3.m_afData[1] * m_afData[1][1] +
                (double)cVec3.m_afData[2] * m_afData[1][2]);
    cResult.m_afData[2] =
        (float)((double)cVec3.m_afData[0] * m_afData[2][0] + (double)cVec3.m_afData[1] * m_afData[2][1] +
                (double)cVec3.m_afData[2] * m_afData[2][2]);
    return cResult;
}

void CRotTransform::RotateZ(float fAngle) {
    CRotTransform result;
    CRotTransform cRotMatrix;
    cRotMatrix.~CRotTransform();
    ::new (&cRotMatrix) CRotTransform;
    float v4 = (float)(fAngle / 57.295780181884766);
    float v3 = (float)std::cos((double)v4);
    float fCosine = v3;
    float fSine = (float)std::sin((double)v4);
    float v9 = fSine;
    cRotMatrix.m_afData[0][0] = fCosine;
    cRotMatrix.m_afData[0][1] = fSine;
    cRotMatrix.m_afData[0][2] = (float)0.0;
    cRotMatrix.m_afData[1][0] = (-fSine);
    cRotMatrix.m_afData[1][1] = fCosine;
    cRotMatrix.m_afData[1][2] = (float)0.0;
    cRotMatrix.m_afData[2][0] = (float)0.0;
    cRotMatrix.m_afData[2][1] = (float)0.0;
    cRotMatrix.m_afData[2][2] = (float)1.0;
    __builtin_memcpy(this, &(result = (&cRotMatrix)->operator*(*this)), sizeof(CRotTransform));
}

// The transform is the identity through its own constructor.
SIdvBranchVertex::SIdvBranchVertex() : m_cDirection(0.0f, 0.0f, 0.0f), m_cPos(0.0f, 0.0f, 0.0f) {}

// The clamp compares against double constants and stores float ones.
float CVec3::AngleBetween(const CVec3& cVec) const {
    float fDot = *this * cVec;
    if (fDot < -1.0) {
        fDot = -1.0f;
    }
    if (fDot > 1.0) {
        fDot = 1.0f;
    }
    float fAngle = static_cast<float>(std::acos(static_cast<double>(fDot)));
    return fAngle;
}

float SIdvBranchFlare::Distance(float fInputAngle, float fInputProgress) const {
    float fDistance = 0.0f;
    float fTestAngle = m_fAngle;
    float v10 = fInputAngle - fTestAngle;
    float v9 = std::fabs(static_cast<double>(v10));
    if (v9 > 3.1415927f) {
        if (!(fTestAngle > static_cast<double>(fInputAngle))) {
            fTestAngle = fTestAngle + c_fTwoPi;
        }
        else {
            fInputAngle = fInputAngle + c_fTwoPi;
        }
    }
    float v8 = fInputAngle - fTestAngle;
    float fAngleDifference = std::fabs(static_cast<double>(v8));
    if (m_fRadialInfluence > static_cast<double>(fAngleDifference)) {
        float fProgressDifference = m_fLengthInfluence - fInputProgress;
        if (fProgressDifference > 0.0) {
            float fRadialContribution = 1.0 - fAngleDifference / m_fRadialInfluence;
            float v6 = std::pow(static_cast<double>(fRadialContribution), static_cast<double>(m_fRadialExponent));
            float fVerticalContribution = fProgressDifference / m_fLengthInfluence;
            float v5 = std::pow(static_cast<double>(fVerticalContribution), static_cast<double>(m_fLengthExponent));
            return m_fDistance * v6 * v5;
        }
    }
    return fDistance;
}
