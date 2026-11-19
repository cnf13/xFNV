#include "Gamebryo2.2/CoreLibs/NiCollision/NiSqrDistance.h"

#include <cmath>

// Squared distance between a segment and a parallelogram (Eberly's segment-parallelogram minimisation). fSegParam is
// the closest segment parameter, fPgmParam0/1 the parallelogram parameters along m_kEdge0/m_kEdge1, all in [0, 1]. A
// segment parallel to the plane (|normal . direction| <= |direction| |normal| 1e-5, the double 9.999999747378752e-06)
// or a near-singular system (|det| < 1e-5f) is handled by the four edges and the two segment end points. Otherwise the
// unconstrained minimum (fSegParam, fPgmParam0, fPgmParam1) is solved by cofactors and, when it lies outside the unit
// cube, the minimum over the faces of that region is taken. Each test is written so that NaN behaves as in the game: `x
// < 0` / `x <= 1` fall to the else arm when unordered. In the region r > 1, s < 0, 0 <= t <= 1 the distance from the
// segment origin is also computed and dropped, as the game does. The interior distance uses the double 2.0.
float NiSqrDistance::Compute(
    const NiSegment& kSeg,
    const NiParallelogram& kPgm,
    float& fSegParam,
    float& fPgmParam0,
    float& fPgmParam1
) {
    NiPoint3 kNormal = kPgm.m_kEdge0.Cross(kPgm.m_kEdge1);

    float fNormalDotDir = kNormal.Dot(kSeg.m_kDirection);
    float fAbsNormalDotDir = std::fabsf(fNormalDotDir);
    float fNormalLength = kNormal.Length();
    float fDirLength = kSeg.m_kDirection.Length();
    float fParallelTolerance = fDirLength * fNormalLength * 9.999999747378752e-06;

    NiSegment kSegPgm;
    float fSqrDist;
    float fSqrDist0;
    float fR0;
    float fS0;
    float fT0;

    if (fAbsNormalDotDir > fParallelTolerance) {
        NiPoint3 kDiff = kPgm.m_kOrigin - kSeg.m_kOrigin;
        float fA00 = kSeg.m_kDirection.Dot(kSeg.m_kDirection);
        float fDot01 = kPgm.m_kEdge0.Dot(kSeg.m_kDirection);
        float fA01 = -fDot01;
        float fDot02 = kPgm.m_kEdge1.Dot(kSeg.m_kDirection);
        float fA02 = -fDot02;
        float fA11 = kPgm.m_kEdge0.Dot(kPgm.m_kEdge0);
        float fA12 = kPgm.m_kEdge1.Dot(kPgm.m_kEdge0);
        float fA22 = kPgm.m_kEdge1.Dot(kPgm.m_kEdge1);
        float fDot0 = kDiff.Dot(kSeg.m_kDirection);
        float fB0 = -fDot0;
        float fB1 = kPgm.m_kEdge0.Dot(kDiff);
        float fB2 = kPgm.m_kEdge1.Dot(kDiff);
        float fCof00 = fA11 * fA22 - fA12 * fA12;
        float fCof01 = fA02 * fA12 - fA01 * fA22;
        float fCof02 = fA01 * fA12 - fA02 * fA11;
        float fDet = fA00 * fCof00 + fA01 * fCof01 + fA02 * fCof02;

        if (std::fabsf(fDet) >= 1e-05f) {
            float fCof11 = fA00 * fA22 - fA02 * fA02;
            float fCof12 = fA02 * fA01 - fA00 * fA12;
            float fCof22 = fA00 * fA11 - fA01 * fA01;
            float fInvDet = 1.0f / fDet;
            float fRhs0 = -fB0 * fInvDet;
            float fRhs1 = -fB1 * fInvDet;
            float fRhs2 = -fB2 * fInvDet;

            fSegParam = fCof00 * fRhs0 + fCof01 * fRhs1 + fCof02 * fRhs2;
            fPgmParam0 = fCof01 * fRhs0 + fCof11 * fRhs1 + fCof12 * fRhs2;
            fPgmParam1 = fCof02 * fRhs0 + fCof12 * fRhs1 + fCof22 * fRhs2;

            if (fSegParam < 0.0f) {
                if (fPgmParam0 < 0.0f) {
                    if (fPgmParam1 < 0.0f) {
                        // min on face s=0 or t=0 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // min on face s=0 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else {
                        // min on face s=0 or t=1 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
                else if (fPgmParam0 <= 1.0f) {
                    if (fPgmParam1 < 0.0f) {
                        // min on face t=0 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam0);
                        fPgmParam1 = 0.0f;
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // global minimum is on face r=0
                        fSqrDist = Compute(kSeg.m_kOrigin, kPgm, fPgmParam0, fPgmParam1);
                        fSegParam = 0.0f;
                    }
                    else {
                        // min on face t=1 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam0);
                        fPgmParam1 = 1.0f;
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
                else {
                    if (fPgmParam1 < 0.0f) {
                        // min on face s=1 or t=0 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // min on face s=1 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else {
                        // min on face s=1 or t=1 or r=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        fR0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
            }
            else if (fSegParam <= 1.0f) {
                if (fPgmParam0 < 0.0f) {
                    if (fPgmParam1 < 0.0f) {
                        // min on face s=0 or t=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // global minimum is on face s=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                    }
                    else {
                        // min on face s=0 or t=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
                else if (fPgmParam0 <= 1.0f) {
                    if (fPgmParam1 < 0.0f) {
                        // global minimum is on face t=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam0);
                        fPgmParam1 = 0.0f;
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // global minimum is interior
                        fSqrDist = fSegParam * (fA00 * fSegParam + fA01 * fPgmParam0 + fA02 * fPgmParam1 + 2.0 * fB0) +
                            fPgmParam0 * (fA01 * fSegParam + fA11 * fPgmParam0 + fA12 * fPgmParam1 + 2.0 * fB1) +
                            fPgmParam1 * (fA02 * fSegParam + fA12 * fPgmParam0 + fA22 * fPgmParam1 + 2.0 * fB2) +
                            kDiff.Dot(kDiff);
                        return std::fabsf(fSqrDist);
                    }
                    else {
                        // global minimum is on face t=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam0);
                        fPgmParam1 = 1.0f;
                    }
                }
                else {
                    if (fPgmParam1 < 0.0f) {
                        // min on face s=1 or t=0
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // global minimum is on face s=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                    }
                    else {
                        // min on face s=1 or t=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
            }
            else {
                if (fPgmParam0 < 0.0f) {
                    if (fPgmParam1 < 0.0f) {
                        // min on face s=0 or t=0 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // min on face s=0 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        // The segment origin's distance is computed here too and never read, as the game does.
                        fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else {
                        // min on face s=0 or t=1 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 0.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
                else if (fPgmParam0 <= 1.0f) {
                    if (fPgmParam1 < 0.0f) {
                        // min on face t=0 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam0);
                        fPgmParam1 = 0.0f;
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // global minimum is on face r=1
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist = Compute(kPnt, kPgm, fPgmParam0, fPgmParam1);
                        fSegParam = 1.0f;
                    }
                    else {
                        // min on face t=1 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam0);
                        fPgmParam1 = 1.0f;
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
                else {
                    if (fPgmParam1 < 0.0f) {
                        // min on face s=1 or t=0 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 0.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else if (fPgmParam1 <= 1.0f) {
                        // min on face s=1 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                    else {
                        // min on face s=1 or t=1 or r=1
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge0;
                        kSegPgm.m_kDirection = kPgm.m_kEdge1;
                        fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam1);
                        fPgmParam0 = 1.0f;
                        kSegPgm.m_kOrigin = kPgm.m_kOrigin + kPgm.m_kEdge1;
                        kSegPgm.m_kDirection = kPgm.m_kEdge0;
                        fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
                        fT0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                        NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
                        fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
                        fR0 = 1.0f;
                        if (fSqrDist0 < fSqrDist) {
                            fSqrDist = fSqrDist0;
                            fSegParam = fR0;
                            fPgmParam0 = fS0;
                            fPgmParam1 = fT0;
                        }
                    }
                }
            }
            return fSqrDist;
        }
    }

    // The segment is parallel to the parallelogram (or the system is near singular): test the four edges and the two
    // segment end points. Only the edge segment's direction is reset between the first two edges.
    kSegPgm.m_kOrigin = kPgm.m_kOrigin;
    kSegPgm.m_kDirection = kPgm.m_kEdge0;
    fSqrDist = Compute(kSeg, kSegPgm, fSegParam, fPgmParam0);
    fPgmParam1 = 0.0f;

    kSegPgm.m_kDirection = kPgm.m_kEdge1;
    fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fT0);
    fS0 = 0.0f;
    if (fSqrDist0 < fSqrDist) {
        fSqrDist = fSqrDist0;
        fSegParam = fR0;
        fPgmParam0 = fS0;
        fPgmParam1 = fT0;
    }

    NiPoint3 kOrigin1 = kPgm.m_kEdge1 + kPgm.m_kOrigin;
    kSegPgm.m_kOrigin = kOrigin1;
    kSegPgm.m_kDirection = kPgm.m_kEdge0;
    fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fS0);
    fT0 = 1.0f;
    if (fSqrDist0 < fSqrDist) {
        fSqrDist = fSqrDist0;
        fSegParam = fR0;
        fPgmParam0 = fS0;
        fPgmParam1 = fT0;
    }

    NiPoint3 kOrigin0 = kPgm.m_kEdge0 + kPgm.m_kOrigin;
    kSegPgm.m_kOrigin = kOrigin0;
    kSegPgm.m_kDirection = kPgm.m_kEdge1;
    fSqrDist0 = Compute(kSeg, kSegPgm, fR0, fT0);
    fS0 = 1.0f;
    if (fSqrDist0 < fSqrDist) {
        fSqrDist = fSqrDist0;
        fSegParam = fR0;
        fPgmParam0 = fS0;
        fPgmParam1 = fT0;
    }

    fSqrDist0 = Compute(kSeg.m_kOrigin, kPgm, fS0, fT0);
    fR0 = 0.0f;
    if (fSqrDist0 < fSqrDist) {
        fSqrDist = fSqrDist0;
        fSegParam = fR0;
        fPgmParam0 = fS0;
        fPgmParam1 = fT0;
    }

    NiPoint3 kPnt = kSeg.m_kOrigin + kSeg.m_kDirection;
    fSqrDist0 = Compute(kPnt, kPgm, fS0, fT0);
    fR0 = 1.0f;
    if (fSqrDist0 < fSqrDist) {
        fSqrDist = fSqrDist0;
        fSegParam = fR0;
        fPgmParam0 = fS0;
        fPgmParam1 = fT0;
    }
    return fSqrDist;
}
