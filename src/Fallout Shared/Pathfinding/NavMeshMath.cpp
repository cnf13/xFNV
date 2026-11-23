#include "Fallout Shared/Pathfinding/NavMeshMath.h"

#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/Segment.h"
#include "Fallout Shared/Pathfinding/Sphere.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "SDK/MSVC/ehvec.h"

#include <cmath>

// These constructors only run the results' element constructors, which leave them unwritten.
NavMeshMath::CSIResults::CSIResults() {}

NavMeshMath::CCTResults::CCTResults() {}

bool NavMeshMath::IsPointInsideTriangle(
    const NiPoint3& aTriVert1Loc,
    const NiPoint3& aTriVert2Loc,
    const NiPoint3& aTriVert3Loc,
    const NiPoint3& aLocation,
    float& afZDiff,
    float afMaxZdistAbove,
    float afMaxZdistBelow
) {
    float fArea = (aTriVert2Loc.x - aTriVert1Loc.x) * (aTriVert3Loc.y - aTriVert1Loc.y) -
        (aTriVert3Loc.x - aTriVert1Loc.x) * (aTriVert2Loc.y - aTriVert1Loc.y);
    if (fArea == 0.0) {
        return false;
    }
    float fDX2 = aTriVert2Loc.x - aLocation.x;
    float fDX3 = aTriVert3Loc.x - aLocation.x;
    float fDY2 = aTriVert2Loc.y - aLocation.y;
    float fDY3 = aTriVert3Loc.y - aLocation.y;
    float fInvArea = static_cast<float>(1.0 / fArea);
    float fWeight1 = (fDX2 * fDY3 - fDX3 * fDY2) * fInvArea;
    if (fWeight1 < 0.0) {
        return false;
    }
    float fDX1 = aTriVert1Loc.x - aLocation.x;
    float fDY1 = aTriVert1Loc.y - aLocation.y;
    float fWeight2 = (fDX3 * fDY1 - fDX1 * fDY3) * fInvArea;
    if (fWeight2 < 0.0) {
        return false;
    }
    float fWeight3 = (fDX1 * fDY2 - fDX2 * fDY1) * fInvArea;
    if (fWeight3 < 0.0) {
        return false;
    }
    float fZ = fWeight1 * aTriVert1Loc.z + fWeight2 * aTriVert2Loc.z + fWeight3 * aTriVert3Loc.z;
    float fZDiff = fZ - aLocation.z;
    if (afMaxZdistAbove < static_cast<double>(fZDiff)) {
        return false;
    }
    if (fZDiff < -afMaxZdistBelow) {
        return false;
    }
    afZDiff = fZDiff;
    return true;
}

bool NavMeshMath::Find2DIntersectionStable(
    const NiPoint3& aSegment1Start,
    const NiPoint3& aSegment1End,
    const NiPoint3& aSegment2Start,
    const NiPoint3& aSegment2End,
    NiPoint3* apReturnIntersection
) {
    float fSegment2EndSide = (aSegment1Start.x - aSegment2End.x) * (aSegment1End.y - aSegment2End.y) -
        (aSegment1Start.y - aSegment2End.y) * (aSegment1End.x - aSegment2End.x);
    float fSegment2StartSide = (aSegment1Start.x - aSegment2Start.x) * (aSegment1End.y - aSegment2Start.y) -
        (aSegment1Start.y - aSegment2Start.y) * (aSegment1End.x - aSegment2Start.x);
    float fSegment1StartSide = (aSegment2Start.x - aSegment1Start.x) * (aSegment2End.y - aSegment1Start.y) -
        (aSegment2Start.y - aSegment1Start.y) * (aSegment2End.x - aSegment1Start.x);
    float fSegment1EndSide = (aSegment2Start.x - aSegment1End.x) * (aSegment2End.y - aSegment1End.y) -
        (aSegment2Start.y - aSegment1End.y) * (aSegment2End.x - aSegment1End.x);
    if (!(fSegment2EndSide * fSegment2StartSide <= 0.0) || !(fSegment1StartSide * fSegment1EndSide <= 0.0)) {
        return false;
    }
    if (fSegment2EndSide == 0.0) {
        return false;
    }
    if (fSegment1EndSide == 0.0) {
        return false;
    }
    float fT = fSegment1StartSide / (fSegment1StartSide - fSegment1EndSide);
    NiPoint3 kDir = aSegment1End - aSegment1Start;
    NiPoint3 kOffset = fT * kDir;
    *apReturnIntersection = aSegment1Start + kOffset;
    return true;
}

int8_t NavMeshMath::Get2DBaryCentricCoord_0(float* apA, float* apB, float* apC, float* apD, float* apReturn) {
    float fArea = (apB[0] - apA[0]) * (apC[1] - apA[1]) - (apC[0] - apA[0]) * (apB[1] - apA[1]);
    if (fArea == 0.0) {
        return 0;
    }
    float fInvArea = 1.0 / fArea;
    apReturn[0] = ((apB[0] - apD[0]) * (apC[1] - apD[1]) - (apC[0] - apD[0]) * (apB[1] - apD[1])) * fInvArea;
    apReturn[1] = ((apC[0] - apD[0]) * (apA[1] - apD[1]) - (apA[0] - apD[0]) * (apC[1] - apD[1])) * fInvArea;
    apReturn[2] = ((apA[0] - apD[0]) * (apB[1] - apD[1]) - (apB[0] - apD[0]) * (apA[1] - apD[1])) * fInvArea;
    return 1;
}

bool NavMeshMath::Find2DIntersection(
    const NiPoint2& aSegment1Start,
    const NiPoint2& aSegment1End,
    const NiPoint2& aSegment2Start,
    const NiPoint2& aSegment2End
) {
    float fSegment2EndSide = (aSegment1Start.x - aSegment2End.x) * (aSegment1End.y - aSegment2End.y) -
        (aSegment1Start.y - aSegment2End.y) * (aSegment1End.x - aSegment2End.x);
    float fSegment2StartSide = (aSegment1Start.x - aSegment2Start.x) * (aSegment1End.y - aSegment2Start.y) -
        (aSegment1Start.y - aSegment2Start.y) * (aSegment1End.x - aSegment2Start.x);
    if (!(fSegment2EndSide * fSegment2StartSide <= 0.0)) {
        return false;
    }
    float fSegment1StartSide = (aSegment2Start.x - aSegment1Start.x) * (aSegment2End.y - aSegment1Start.y) -
        (aSegment2Start.y - aSegment1Start.y) * (aSegment2End.x - aSegment1Start.x);
    float fSegment1EndSide = fSegment1StartSide + fSegment2StartSide - fSegment2EndSide;
    if (fSegment1StartSide * fSegment1EndSide < 0.0) {
        return true;
    }
    return false;
}

bool NavMeshMath::Find2DIntersection3(
    const NiPoint3& aSegment1Start,
    const NiPoint3& aSegment1End,
    const NiPoint3& aSegment2Start,
    const NiPoint3& aSegment2End
) {
    NiPoint2 kSegment2End(aSegment2End.x, aSegment2End.y);
    NiPoint2 kSegment2Start(aSegment2Start.x, aSegment2Start.y);
    NiPoint2 kSegment1End(aSegment1End.x, aSegment1End.y);
    NiPoint2 kSegment1Start(aSegment1Start.x, aSegment1Start.y);
    return NavMeshMath::Find2DIntersection(kSegment1Start, kSegment1End, kSegment2Start, kSegment2End);
}

bool NavMeshMath::Find2DIntersectionPoint(
    const NiPoint3& aSegment1Start,
    const NiPoint3& aSegment1End,
    const NiPoint3& aSegment2Start,
    const NiPoint3& aSegment2End,
    NiPoint3* apReturnIntersection
) {
    float fSegment2EndSide = (aSegment1Start.x - aSegment2End.x) * (aSegment1End.y - aSegment2End.y) -
        (aSegment1Start.y - aSegment2End.y) * (aSegment1End.x - aSegment2End.x);
    float fSegment2StartSide = (aSegment1Start.x - aSegment2Start.x) * (aSegment1End.y - aSegment2Start.y) -
        (aSegment1Start.y - aSegment2Start.y) * (aSegment1End.x - aSegment2Start.x);
    if (!(fSegment2EndSide * fSegment2StartSide <= 0.0)) {
        return false;
    }
    float fSegment1StartSide = (aSegment2Start.x - aSegment1Start.x) * (aSegment2End.y - aSegment1Start.y) -
        (aSegment2Start.y - aSegment1Start.y) * (aSegment2End.x - aSegment1Start.x);
    float fSegment1EndSide = fSegment1StartSide + fSegment2StartSide - fSegment2EndSide;
    if (!(fSegment1StartSide * fSegment1EndSide < 0.0)) {
        return false;
    }
    float fT = fSegment1StartSide / (fSegment1StartSide - fSegment1EndSide);
    NiPoint3 kDir = aSegment1End - aSegment1Start;
    NiPoint3 kOffset = fT * kDir;
    *apReturnIntersection = aSegment1Start + kOffset;
    return true;
}

NiPoint3
NavMeshMath::ProjectPointOntoSegment(const NiPoint3& aPoint, const NiPoint3& aLineStart, const NiPoint3& aLineEnd) {
    NiPoint3 kResult;
    NiPoint3 v = aLineEnd - aLineStart;
    NiPoint3 w = aPoint - aLineStart;
    double c1 = w.Dot(v);
    if (c1 <= 0.0) {
        kResult = aLineStart;
        return kResult;
    }

    double c2 = v.Dot(v);
    if (c1 >= c2) {
        kResult = aLineEnd;
    }
    else {
        double b = c1 / c2;
        float fScale = b;
        kResult = aLineStart + fScale * v;
    }
    return kResult;
}

// The points are flattened to x and y and passed to the NiPoint2 overload, built from the last argument back. Its
// result is returned as is.
bool NavMeshMath::Get2DBaryCentricCoord(
    const NiPoint3& aV0,
    const NiPoint3& aV1,
    const NiPoint3& aV2,
    const NiPoint3& aLoc,
    NiPoint3& aCoord
) {
    NiPoint2 kLoc(aLoc.x, aLoc.y);
    NiPoint2 kV2(aV2.x, aV2.y);
    NiPoint2 kV1(aV1.x, aV1.y);
    NiPoint2 kV0(aV0.x, aV0.y);
    return Get2DBaryCentricCoord_0(&kV0.x, &kV1.x, &kV2.x, &kLoc.x, &aCoord.x);
}

// The point where the line crosses the segment's line; when both segment ends lie on the same side of the line, the
// segment end nearer to that point.
NiPoint2 NavMeshMath::ClosestPointOnSegmentToLine(
    const NiPoint2& aSegmentStart,
    const NiPoint2& aSegmentEnd,
    const NiPoint2& aLineStart,
    const NiPoint2& aLineEnd
) {
    float fEndSide = (aLineStart.x - aSegmentEnd.x) * (aLineEnd.y - aSegmentEnd.y) -
        (aLineStart.y - aSegmentEnd.y) * (aLineEnd.x - aSegmentEnd.x);
    float fStartSide = (aLineStart.x - aSegmentStart.x) * (aLineEnd.y - aSegmentStart.y) -
        (aLineStart.y - aSegmentStart.y) * (aLineEnd.x - aSegmentStart.x);
    float fArea = (aSegmentStart.x - aLineStart.x) * (aSegmentEnd.y - aLineStart.y) -
        (aSegmentStart.y - aLineStart.y) * (aSegmentEnd.x - aLineStart.x);
    float fArea2 = fArea + fStartSide - fEndSide;
    float fT = fArea / (fArea - fArea2);
    NiPoint2 kDir = aLineEnd - aLineStart;
    NiPoint2 kScaled;
    NiPoint2 kIntersection = aLineStart + *NiPoint2::Scale_(&kScaled, fT, &kDir);
    if (fEndSide * fStartSide > 0.0) {
        NiPoint2 kToStart = kIntersection - aSegmentStart;
        double dStartDist = kToStart.SqrLength();
        NiPoint2 kToEnd = kIntersection - aSegmentEnd;
        if (kToEnd.SqrLength() > dStartDist) {
            return aSegmentStart;
        }
        return aSegmentEnd;
    }
    return kIntersection;
}

NiPoint3 NavMeshMath::ClosestPointOnSegmentToLine3(
    const NiPoint3& aSegmentStart,
    const NiPoint3& aSegmentEnd,
    const NiPoint3& aLineStart,
    const NiPoint3& aLineEnd
) {
    NiPoint3 kResult;
    if (aSegmentStart == aSegmentEnd) {
        kResult = aSegmentStart;
        return kResult;
    }

    float fEndSide = (aLineStart.x - aSegmentEnd.x) * (aLineEnd.y - aSegmentEnd.y) -
        (aLineStart.y - aSegmentEnd.y) * (aLineEnd.x - aSegmentEnd.x);
    float fStartSide = (aLineStart.x - aSegmentStart.x) * (aLineEnd.y - aSegmentStart.y) -
        (aLineStart.y - aSegmentStart.y) * (aLineEnd.x - aSegmentStart.x);
    float fT = fStartSide / (fStartSide - fEndSide);
    NiPoint3 kDir = aSegmentEnd - aSegmentStart;
    NiPoint3 kOffset = fT * kDir;
    NiPoint3 kIntersection = aSegmentStart + kOffset;
    if (!(fEndSide * fStartSide > 0.0)) {
        kResult = kIntersection;
        return kResult;
    }

    NiPoint3 kToStart = kIntersection - aSegmentStart;
    double dStartDist = kToStart.SqrLength();
    NiPoint3 kToEnd = kIntersection - aSegmentEnd;
    if (kToEnd.SqrLength() > dStartDist) {
        kResult = aSegmentStart;
    }
    else {
        kResult = aSegmentEnd;
    }
    return kResult;
}

float NavMeshMath::DistanceFromSegment(
    const NiPoint3& aPoint,
    const NiPoint3& aLineStart,
    const NiPoint3& aLineEnd,
    bool* apRetInSegment
) {
    NiPoint3 kDir = aLineEnd - aLineStart;
    NiPoint3 kToPoint = aPoint - aLineStart;
    double dProjection = kToPoint.Dot(kDir);
    if (dProjection <= 0.0) {
        if (apRetInSegment) {
            *apRetInSegment = false;
        }
        NiPoint3 kFromStart = aPoint - aLineStart;
        return kFromStart.Length();
    }

    double dSqrLength = kDir.Dot(kDir);
    if (dProjection >= dSqrLength) {
        if (apRetInSegment) {
            *apRetInSegment = false;
        }
        NiPoint3 kFromEnd = aPoint - aLineEnd;
        return kFromEnd.Length();
    }

    double dRatio = dProjection / dSqrLength;
    float fT = dRatio;
    NiPoint3 kOffset = fT * kDir;
    NiPoint3 kProjected = aLineStart + kOffset;
    if (apRetInSegment) {
        *apRetInSegment = true;
    }
    NiPoint3 kFromProjected = aPoint - kProjected;
    return kFromProjected.Length();
}

bool NavMeshMath::DoCircleAndSegmentIntersect(
    const NiPoint3& aCircleCenter,
    float afCircleRadius,
    const NiPoint3& aSegmentStart,
    const NiPoint3& aSegmentEnd
) {
    NiPoint3 kClosest;
    NiPoint3 kDir = aSegmentEnd - aSegmentStart;
    NiPoint3 kToCenter = aCircleCenter - aSegmentStart;
    float fProjection = kDir.Dot(kToCenter);
    if (fProjection <= 0.0) {
        kClosest = aSegmentStart;
    }
    else {
        float fSqrLength = kDir.Dot(kDir);
        if (fProjection >= static_cast<double>(fSqrLength)) {
            kClosest = aSegmentEnd;
        }
        else {
            float fT = fProjection / fSqrLength;
            NiPoint3 kOffset = fT * kDir;
            kClosest = aSegmentStart + kOffset;
        }
    }
    NiPoint3 kFromClosest = aCircleCenter - kClosest;
    return afCircleRadius * afCircleRadius > kFromClosest.SqrLength();
}

bool NavMeshMath::CircleSegmentIntersection(
    const Sphere& aCircle,
    const Segment& aSegment,
    NavMeshMath::CSIResults& aResults
) {
    bool bdox;
    Segment relativeSegment;
    float fEpsilon = 0.001f;
    if (aCircle.fRadius <= 0.0) {
        return false;
    }
    NavMeshMath::CSIResult* pResult0 = aResults.Results;
    NavMeshMath::CSIResult* pResult1 = &aResults.Results[1];
    relativeSegment = aSegment;
    relativeSegment.Offset(-aCircle.Center);
    float fRadius = 1.0f;
    relativeSegment.Start /= aCircle.fRadius;
    relativeSegment.End /= aCircle.fRadius;
    NiPoint3 delta = relativeSegment.Delta();
    float fR2 = fRadius * fRadius;
    if (delta.y != 0.0 || delta.x == 0.0) {
        if (delta.y == 0.0 || delta.x != 0.0) {
            double dAbsDeltaX = std::fabs(delta.x);
            bdox = std::fabs(delta.y) < dAbsDeltaX;
        }
        else {
            bdox = false;
        }
    }
    else {
        bdox = true;
    }
    if (bdox) {
        float fInvDeltaX = 1.0 / delta.x;
        float fM = delta.y * fInvDeltaX;
        float fP = relativeSegment.Start.y - fM * relativeSegment.Start.x;
        float fM2 = fM * fM;
        float fB = fM2 + 1.0;
        float fP2 = fP * fP;
        float fDeltaCos = (fR2 * fB - fP2) * fR2;
        if (fDeltaCos < -fEpsilon) {
            aResults.eType = NavMeshMath::CSI_NONE;
            return false;
        }
        if (!(fEpsilon > static_cast<double>(fDeltaCos))) {
            AngleSinCos resultParam1;
            AngleSinCos resultParam2;
            float fInvDenom = 1.0 / (fR2 * fB);
            float fNom = fP * fRadius;
            aResults.eType = NavMeshMath::CSI_TWOPOINTS;
            float fSqrtDelta = std::sqrt(fDeltaCos);
            resultParam2.fCos = (-fNom * fM + fSqrtDelta) * fInvDenom;
            resultParam2.fSin = (fM * fSqrtDelta + fNom) * fInvDenom;
            resultParam1.fCos = (-fNom * fM - fSqrtDelta) * fInvDenom;
            resultParam1.fSin = (fNom - fM * fSqrtDelta) * fInvDenom;
            if (delta.x < 0.0) {
                pResult0->CircleParam = resultParam2;
                pResult1->CircleParam = resultParam1;
            }
            else {
                pResult0->CircleParam = resultParam1;
                pResult1->CircleParam = resultParam2;
            }
            pResult0->fSegmentParam = (fRadius * pResult0->CircleParam.fCos - relativeSegment.Start.x) * fInvDeltaX;
            pResult1->fSegmentParam = (fRadius * pResult1->CircleParam.fCos - relativeSegment.Start.x) * fInvDeltaX;
        }
        else {
            AngleSinCos resultParam;
            float fInvDenom = 1.0 / (fR2 * fB);
            float fNom = fP * fRadius;
            aResults.eType = NavMeshMath::CSI_TANGENT;
            resultParam.fCos = -fNom * fM * fInvDenom;
            resultParam.fSin = fNom * fInvDenom;
            pResult0->CircleParam = resultParam;
            pResult0->fSegmentParam = (fRadius * pResult0->CircleParam.fCos - relativeSegment.Start.x) * fInvDeltaX;
        }
    }
    else {
        float fInvDeltaY = 1.0 / delta.y;
        float fM = delta.x * fInvDeltaY;
        float fP = relativeSegment.Start.x - fM * relativeSegment.Start.y;
        float fM2 = fM * fM;
        float fB = fM2 + 1.0;
        float fP2 = fP * fP;
        float fDeltaSin = (fR2 * fB - fP2) * fR2;
        if (fDeltaSin < -fEpsilon) {
            aResults.eType = NavMeshMath::CSI_NONE;
            return false;
        }
        if (!(fEpsilon > static_cast<double>(fDeltaSin))) {
            AngleSinCos resultParam1;
            AngleSinCos resultParam2;
            float fInvDenom = 1.0 / (fR2 * fB);
            float fNom = fP * fRadius;
            aResults.eType = NavMeshMath::CSI_TWOPOINTS;
            float fSqrtDelta = std::sqrt(fDeltaSin);
            resultParam2.fCos = (fM * fSqrtDelta + fNom) * fInvDenom;
            resultParam2.fSin = (-fNom * fM + fSqrtDelta) * fInvDenom;
            resultParam1.fCos = (fNom - fM * fSqrtDelta) * fInvDenom;
            resultParam1.fSin = (-fNom * fM - fSqrtDelta) * fInvDenom;
            if (delta.y < 0.0) {
                pResult0->CircleParam = resultParam2;
                pResult1->CircleParam = resultParam1;
            }
            else {
                pResult0->CircleParam = resultParam1;
                pResult1->CircleParam = resultParam2;
            }
            pResult0->fSegmentParam = (fRadius * pResult0->CircleParam.fSin - relativeSegment.Start.y) * fInvDeltaY;
            pResult1->fSegmentParam = (fRadius * pResult1->CircleParam.fSin - relativeSegment.Start.y) * fInvDeltaY;
        }
        else {
            AngleSinCos resultParamY;
            float fInvDenomY = 1.0 / (fR2 * fB);
            float fNom = fP * fRadius;
            aResults.eType = NavMeshMath::CSI_TANGENT;
            resultParamY.fCos = fNom * fInvDenomY;
            resultParamY.fSin = -fNom * fM * fInvDenomY;
            pResult0->CircleParam = resultParamY;
            pResult0->fSegmentParam = (fRadius * pResult0->CircleParam.fSin - relativeSegment.Start.y) * fInvDeltaY;
        }
    }
    if (aResults.eType == NavMeshMath::CSI_TANGENT) {
        bool bInRange0 = pResult0->fSegmentParam >= -fEpsilon && pResult0->fSegmentParam <= fEpsilon + 1.0;
        if (bInRange0) {
            if (!(fEpsilon > static_cast<double>(pResult0->fSegmentParam)) &&
                !(pResult0->fSegmentParam > 1.0 - fEpsilon)) {
                aResults.Results[0u].eType = NavMeshMath::CSIT_EDGE;
            }
            else {
                aResults.Results[0u].eType = NavMeshMath::CSIT_ENDPOINT;
            }
            return true;
        }

        aResults.eType = NavMeshMath::CSI_NONE;
        return false;
    }

    bool bInRange0 = pResult0->fSegmentParam >= -fEpsilon && pResult0->fSegmentParam <= fEpsilon + 1.0;
    bool bInRange1 = pResult1->fSegmentParam >= -fEpsilon && pResult1->fSegmentParam <= fEpsilon + 1.0;
    if (bInRange0 && bInRange1) {
        aResults.eType = NavMeshMath::CSI_TWOPOINTS;
        if (!(fEpsilon > static_cast<double>(pResult0->fSegmentParam)) && !(pResult0->fSegmentParam > 1.0 - fEpsilon)) {
            aResults.Results[0u].eType = NavMeshMath::CSIT_EDGE;
        }
        else {
            aResults.Results[0u].eType = NavMeshMath::CSIT_ENDPOINT;
        }
        if (!(fEpsilon > static_cast<double>(pResult1->fSegmentParam)) && !(pResult1->fSegmentParam > 1.0 - fEpsilon)) {
            aResults.Results[1].eType = NavMeshMath::CSIT_EDGE;
        }
        else {
            aResults.Results[1].eType = NavMeshMath::CSIT_ENDPOINT;
        }
        return true;
    }

    if (!bInRange0 || bInRange1) {
        if (bInRange0 || !bInRange1) {
            if (!(fR2 > relativeSegment.Start.SqrXYLength())) {
                aResults.eType = NavMeshMath::CSI_NONE;
            }
            else {
                aResults.eType = NavMeshMath::CSI_CONTAINED;
            }
            return false;
        }

        aResults.eType = NavMeshMath::CSI_ONEPOINT_LOWER;
        pResult0->CircleParam = pResult1->CircleParam;
        pResult0->fSegmentParam = pResult1->fSegmentParam;
        if (!(fEpsilon > static_cast<double>(pResult0->fSegmentParam)) && !(pResult0->fSegmentParam > 1.0 - fEpsilon)) {
            aResults.Results[0u].eType = NavMeshMath::CSIT_EDGE;
        }
        else {
            aResults.Results[0u].eType = NavMeshMath::CSIT_ENDPOINT;
        }
        return true;
    }

    aResults.eType = NavMeshMath::CSI_ONEPOINT_UPPER;
    if (!(fEpsilon > static_cast<double>(pResult0->fSegmentParam)) && !(pResult0->fSegmentParam > 1.0 - fEpsilon)) {
        aResults.Results[0u].eType = NavMeshMath::CSIT_EDGE;
    }
    else {
        aResults.Results[0u].eType = NavMeshMath::CSIT_ENDPOINT;
    }
    return true;
}

NavMeshMath::CIRCLE_CIRCLE_TANGENT NavMeshMath::ComputeTangentPoints(
    const Sphere& aCircle1,
    const Sphere& aCircle2,
    ANGLE_DIRECTION aDirection,
    NavMeshMath::CCTResults& aResults
) {
    int32_t iValidCount = 0;
    NiPoint3 v = aCircle2.Center - aCircle1.Center;
    float fRadius1 = aCircle1.fRadius;
    float fRadius2 = aCircle2.fRadius;
    if (aCircle2.fRadius == 0.0) {
        if (aCircle1.fRadius != 0.0) {
            fRadius1 = 1.0f;
            fRadius2 = aCircle2.fRadius / aCircle1.fRadius;
            v /= aCircle1.fRadius;
        }
    }
    else {
        fRadius2 = 1.0f;
        fRadius1 = aCircle1.fRadius / aCircle2.fRadius;
        v /= aCircle2.fRadius;
    }
    float fSqrLength = v.SqrXYLength();
    if (fSqrLength == 0.0) {
        aResults.eType = NavMeshMath::CCT_NONE;
        return NavMeshMath::CCT_NONE;
    }

    float fDeltaRadius = fRadius1 - fRadius2;
    float fDeltaRadiusSqr = fDeltaRadius * fDeltaRadius;
    if (fDeltaRadiusSqr <= static_cast<double>(fSqrLength)) {
        double dSin;
        float fRatio = 1.0 / fSqrLength;
        float fDistSqr = fSqrLength - fDeltaRadiusSqr;
        float fDist = std::sqrt(fDistSqr);
        if (aDirection) {
            aResults.Results[0u].Direction = AD_CW;
            aResults.Results[0u].Circle1Param.fCos = (fDeltaRadius * v.x - v.y * fDist) * fRatio;
            dSin = (fDeltaRadius * v.y + v.x * fDist) * fRatio;
        }
        else {
            aResults.Results[0u].Direction = AD_CCW;
            aResults.Results[0u].Circle1Param.fCos = (fDeltaRadius * v.x + v.y * fDist) * fRatio;
            dSin = (fDeltaRadius * v.y - v.x * fDist) * fRatio;
        }
        aResults.Results[0u].Circle1Param.fSin = dSin;
        aResults.Results[iValidCount].Circle2Param.fCos = aResults.Results[iValidCount].Circle1Param.fCos;
        aResults.Results[iValidCount].Circle2Param.fSin = aResults.Results[iValidCount].Circle1Param.fSin;
        ++iValidCount;
    }
    float fSumRadius = fRadius1 + fRadius2;
    float fSumRadiusSqr = fSumRadius * fSumRadius;
    if (fSumRadiusSqr <= static_cast<double>(fSqrLength)) {
        double dSin;
        float fRatio = 1.0 / fSqrLength;
        float fDistSqr = fSqrLength - fSumRadiusSqr;
        float fDist = std::sqrt(fDistSqr);
        if (aDirection) {
            aResults.Results[iValidCount].Direction = AD_CCW;
            aResults.Results[iValidCount].Circle1Param.fCos = (fSumRadius * v.x - v.y * fDist) * fRatio;
            dSin = (fSumRadius * v.y + v.x * fDist) * fRatio;
        }
        else {
            aResults.Results[iValidCount].Direction = AD_CW;
            aResults.Results[iValidCount].Circle1Param.fCos = (fSumRadius * v.x + v.y * fDist) * fRatio;
            dSin = (fSumRadius * v.y - v.x * fDist) * fRatio;
        }
        aResults.Results[iValidCount].Circle1Param.fSin = dSin;
        aResults.Results[iValidCount].Circle2Param.fCos = -aResults.Results[iValidCount].Circle1Param.fCos;
        aResults.Results[iValidCount].Circle2Param.fSin = -aResults.Results[iValidCount].Circle1Param.fSin;
        ++iValidCount;
    }
    aResults.eType = static_cast<NavMeshMath::CIRCLE_CIRCLE_TANGENT>(iValidCount);
    return static_cast<NavMeshMath::CIRCLE_CIRCLE_TANGENT>(iValidCount);
}

float NavMeshMath::DistanceFromLine(const NiPoint3& aPoint, const NiPoint3& aLineStart, const NiPoint3& aLineEnd) {
    NiPoint3 kDir = aLineEnd - aLineStart;
    NiPoint3 kToPoint = aPoint - aLineStart;
    float fProjection = kToPoint.Dot(kDir);
    float fSqrLength = kDir.Dot(kDir);
    float fT = fProjection / fSqrLength;
    NiPoint3 kOffset = fT * kDir;
    NiPoint3 kProjected = aLineStart + kOffset;
    NiPoint3 kFromProjected = aPoint - kProjected;
    float fSqrDistance = kFromProjected.Dot(kFromProjected);
    return std::sqrt(fSqrDistance);
}

int32_t NavMeshMath::SegmentPlaneIntersection(
    const NiPoint3& aLineStart,
    const NiPoint3& aLineEnd,
    const NiPoint3& aPlaneV0,
    const NiPoint3& aPlaneNormal,
    NiPoint3* apRet
) {
    NiPoint3 kDir = aLineEnd - aLineStart;
    NiPoint3 kFromPlane = aLineStart - aPlaneV0;
    float fDenom = aPlaneNormal.Dot(kDir);
    float fNumer = -aPlaneNormal.Dot(kFromPlane);
    if (static_cast<double>(1e-07f) > std::fabs(fDenom)) {
        if (fNumer == 0.0) {
            return -1;
        }

        return 0;
    }

    float fT = fNumer / fDenom;
    if (!(fT < 0.0) && !(fT > 1.0)) {
        NiPoint3 kOffset = fT * kDir;
        *apRet = aLineStart + kOffset;
        return 1;
    }

    return 0;
}

bool NavMeshMath::SegmentCircleIntersection(
    const NiPoint3& aSegmentStart,
    const NiPoint3& aSegmentEnd,
    const NiPoint3& aCircleCenter,
    float afCircleRadius
) {
    NiPoint3 kClosest;
    NiPoint3 kDir = aSegmentEnd - aSegmentStart;
    NiPoint3 kToCenter = aCircleCenter - aSegmentStart;
    float fProjection = kDir.Dot(kToCenter);
    if (fProjection <= 0.0) {
        kClosest = aSegmentStart;
    }
    else {
        float fSqrLength = kDir.Dot(kDir);
        if (fProjection >= static_cast<double>(fSqrLength)) {
            kClosest = aSegmentEnd;
        }
        else {
            float fT = fProjection / fSqrLength;
            NiPoint3 kOffset = fT * kDir;
            kClosest = aSegmentStart + kOffset;
        }
    }
    NiPoint3 kFromClosest = aCircleCenter - kClosest;
    return afCircleRadius * afCircleRadius > kFromClosest.SqrLength();
}

bool NavMeshMath::CheckArcAgainstCircle(const Sphere& aCircle1, const AngleSinCosArc& aArc, const Sphere& aCircle2) {
    AngleSinCos angle1;
    AngleSinCos angle2;
    if (aCircle1.fRadius == 0.0) {
        return true;
    }
    if (aCircle2.fRadius == 0.0) {
        return true;
    }
    NiPoint3 delta = aCircle2.Center - aCircle1.Center;
    float fD3 = delta.SqrLength();
    float fD2 = delta.SqrXYLength();
    if (fD2 == 0.0) {
        return false;
    }
    float fSum = aCircle1.fRadius + aCircle2.fRadius;
    if (fD2 > fSum * fSum) {
        return true;
    }
    if (fD3 > fSum * fSum) {
        return true;
    }
    float fDiff = aCircle1.fRadius - aCircle2.fRadius;
    if (fD2 < fDiff * fDiff) {
        return true;
    }
    if (fD3 < fDiff * fDiff) {
        return true;
    }
    float fR12 = aCircle1.fRadius * aCircle1.fRadius;
    float fR22 = aCircle2.fRadius * aCircle2.fRadius;
    float fD = std::sqrt(fD2);
    float fInvD = 1.0 / fD;
    float fInvR1 = 1.0 / aCircle1.fRadius;
    float fA = (fR12 - fR22 + fD2) * 0.5 * fInvD;
    float fA2 = fA * fA;
    float fH2 = fR12 - fA2;
    float fH = std::sqrt(fH2);
    angle1.fCos = (fA * delta.x + fH * delta.y) * fInvR1 * fInvD;
    angle1.fSin = (fA * delta.y - fH * delta.x) * fInvR1 * fInvD;
    angle2.fCos = (fA * delta.x - fH * delta.y) * fInvR1 * fInvD;
    angle2.fSin = (fA * delta.y + fH * delta.x) * fInvR1 * fInvD;
    return !aArc.WithinRange(angle1) && !aArc.WithinRange(angle2);
}

NiPoint3 NavMeshMath::RandomPointOnTriangle(const NiPoint3& aVert0, const NiPoint3& aVert1, const NiPoint3& aVert2) {
    float fRandEdge1;
    float fRandEdge2;
    NiPoint3 edge1 = aVert1 - aVert0;
    NiPoint3 edge2 = aVert2 - aVert0;
    do {
        fRandEdge1 = RandomFloat0To1();
        fRandEdge2 = RandomFloat0To1();
    }
    while (fRandEdge1 + fRandEdge2 > 1.0);
    NiPoint3 kEdge2Offset = edge2 * fRandEdge2;
    return aVert0 + edge1 * fRandEdge1 + kEdge2Offset;
}

bool NavMeshMath::Find2DIntersection(
    const NiPoint2& aSegment1Start,
    const NiPoint2& aSegment1End,
    const NiPoint2& aSegment2Start,
    const NiPoint2& aSegment2End,
    NiPoint2* apReturnIntersection
) {
    float fSegment2EndSide = (aSegment1Start.x - aSegment2End.x) * (aSegment1End.y - aSegment2End.y) -
        (aSegment1Start.y - aSegment2End.y) * (aSegment1End.x - aSegment2End.x);
    float fSegment2StartSide = (aSegment1Start.x - aSegment2Start.x) * (aSegment1End.y - aSegment2Start.y) -
        (aSegment1Start.y - aSegment2Start.y) * (aSegment1End.x - aSegment2Start.x);
    if (!(fSegment2EndSide * fSegment2StartSide < 0.0)) {
        return false;
    }
    float fSegment1StartSide = (aSegment2Start.x - aSegment1Start.x) * (aSegment2End.y - aSegment1Start.y) -
        (aSegment2Start.y - aSegment1Start.y) * (aSegment2End.x - aSegment1Start.x);
    float fSegment1EndSide = fSegment1StartSide + fSegment2StartSide - fSegment2EndSide;
    if (std::fabs(fSegment1EndSide) < 0.0010000000474974513) {
        fSegment1EndSide = (aSegment2Start.x - aSegment1End.x) * (aSegment2End.y - aSegment1End.y) -
            (aSegment2Start.y - aSegment1End.y) * (aSegment2End.x - aSegment1End.x);
    }
    if (!(fSegment1StartSide * fSegment1EndSide < 0.0)) {
        return false;
    }
    float fT = fSegment1StartSide / (fSegment1StartSide - fSegment1EndSide);
    NiPoint2 kDir = aSegment1End - aSegment1Start;
    NiPoint2 kScaled;
    NiPoint2 kIntersection = aSegment1Start + *NiPoint2::Scale_(&kScaled, fT, &kDir);
    apReturnIntersection->x = kIntersection.x;
    apReturnIntersection->y = kIntersection.y;
    return true;
}

bool NavMeshMath::LineToLineSegment2DIntersection(
    const NiPoint3* apLeftVertex,
    const NiPoint3* apRightVertex,
    const NiPoint3* apTargetLocation,
    const NiPoint3* apCoverPoint,
    NiPoint3* apIntersection
) {
    NiPoint3 kEdge = *apRightVertex - *apLeftVertex;
    NiPoint3 kLine = *apCoverPoint - *apTargetLocation;
    float fDenom = kEdge.x * kLine.y - kEdge.y * kLine.x;
    if (fDenom == 0.0) {
        return false;
    }
    NiPoint3 kToTarget = *apTargetLocation - *apLeftVertex;
    float fLineCross = kToTarget.x * kEdge.y - kToTarget.y * kEdge.x;
    float fT = fLineCross / fDenom;
    if (fT < 0.0 || fT > 1.0) {
        return false;
    }
    float fEdgeCross = kToTarget.x * kLine.y - kToTarget.y * kLine.x;
    fT = fEdgeCross / fDenom;
    if (apIntersection) {
        NiPoint3 kOffset = fT * kEdge;
        *apIntersection = *apLeftVertex + kOffset;
    }
    return true;
}

char NavMeshMath::LineSegmentToLine2DIntersection(
    NiPoint3* apSegmentStart,
    NiPoint3* apSegmentEnd,
    NiPoint3* pt,
    NiPoint3* apLineEnd,
    NiPoint3* apIntersection
) {
    NiPoint3 kLine = *apLineEnd - *pt;
    NiPoint3 kSegment = *apSegmentEnd - *apSegmentStart;
    float fDenom = kLine.x * kSegment.y - kLine.y * kSegment.x;
    if (fDenom == 0.0) {
        return 0;
    }
    NiPoint3 kFromLine = *apSegmentStart - *pt;
    float fCross = kFromLine.x * kLine.y - kFromLine.y * kLine.x;
    float fT = fCross / fDenom;
    if (fT < 0.0 || fT > 1.0) {
        return 0;
    }
    if (apIntersection) {
        NiPoint3 kOffset = fT * kSegment;
        *apIntersection = *apSegmentStart + kOffset;
    }
    return 1;
}

char NavMeshMath::ComputeTangentPointsToCircleFromPoint(
    NiPoint3* pt,
    float afCircleRadius,
    NiPoint3* apPoint,
    float* apTangent1,
    float* apTangent2
) {
    NiPoint3 kToPoint = *apPoint - *pt;
    float fSqrDist = kToPoint.SqrXYLength();
    float fSqrRadius = afCircleRadius * afCircleRadius;
    if (!(fSqrRadius <= static_cast<double>(fSqrDist))) {
        return 0;
    }
    float fInvSqrDist = static_cast<float>(1.0 / fSqrDist);
    float fSqrTangentLength = fSqrDist - fSqrRadius;
    float fTangentLength = std::sqrt(fSqrTangentLength);
    apTangent1[0] = pt->x;
    apTangent1[1] = pt->y;
    apTangent1[2] = pt->z;
    apTangent2[0] = pt->x;
    apTangent2[1] = pt->y;
    apTangent2[2] = pt->z;
    apTangent1[0] = (afCircleRadius * kToPoint.x - kToPoint.y * fTangentLength) * afCircleRadius * fInvSqrDist +
        apTangent1[0];
    apTangent1[1] = (afCircleRadius * kToPoint.y + kToPoint.x * fTangentLength) * afCircleRadius * fInvSqrDist +
        apTangent1[1];
    apTangent2[0] = (afCircleRadius * kToPoint.x + kToPoint.y * fTangentLength) * afCircleRadius * fInvSqrDist +
        apTangent2[0];
    apTangent2[1] = (afCircleRadius * kToPoint.y - kToPoint.x * fTangentLength) * afCircleRadius * fInvSqrDist +
        apTangent2[1];
    return 1;
}

Segment::Segment(const NiPoint3& aSegment, const NiPoint3& aEnd) : Start(aSegment), End(aEnd) {}

void Segment::Offset(const NiPoint3& aOffset) {
    Start += aOffset;
    End += aOffset;
}

NiPoint3 Segment::Delta() {
    return End - Start;
}

Segment& Segment::operator=(const Segment& aSegment) {
    Start = aSegment.Start;
    End = aSegment.End;
    return *this;
}
