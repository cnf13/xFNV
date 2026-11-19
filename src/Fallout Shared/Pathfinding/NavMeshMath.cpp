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

// These constructors only run the results' element constructors, which leave them unwritten, as the game does.
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
    float v13 = (((&aTriVert2Loc)->x - (&aTriVert1Loc)->x) * ((&aTriVert3Loc)->y - (&aTriVert1Loc)->y)) -
        (((&aTriVert3Loc)->x - (&aTriVert1Loc)->x) * ((&aTriVert2Loc)->y - (&aTriVert1Loc)->y));
    if (v13 == 0.0) {
        return false;
    }
    float v9 = (&aTriVert2Loc)->x - (&aLocation)->x;
    float v10 = (&aTriVert3Loc)->x - (&aLocation)->x;
    float v8 = (&aTriVert2Loc)->y - (&aLocation)->y;
    float v19 = (&aTriVert3Loc)->y - (&aLocation)->y;
    float v17 = (float)(1.0 / v13);
    float v18 = ((v9 * v19) - (v10 * v8)) * v17;
    if (v18 < 0.0) {
        return false;
    }
    float v11 = (&aTriVert1Loc)->x - (&aLocation)->x;
    float v20 = (&aTriVert1Loc)->y - (&aLocation)->y;
    float v16 = ((v10 * v20) - (v11 * v19)) * v17;
    if (v16 < 0.0) {
        return false;
    }
    float v14 = ((v11 * v8) - (v9 * v20)) * v17;
    if (v14 < 0.0) {
        return false;
    }
    float v12 = ((v18 * (&aTriVert1Loc)->z) + (v16 * (&aTriVert2Loc)->z)) + (v14 * (&aTriVert3Loc)->z);
    float v15 = v12 - (&aLocation)->z;
    if (afMaxZdistAbove < ((double)v15)) {
        return false;
    }
    if (v15 < (-afMaxZdistBelow)) {
        return false;
    }
    afZDiff = v15;
    return true;
}

bool NavMeshMath::Find2DIntersectionStable(
    const NiPoint3& aSegment1Start,
    const NiPoint3& aSegment1End,
    const NiPoint3& aSegment2Start,
    const NiPoint3& aSegment2End,
    NiPoint3* apReturnIntersection
) {
    NiPoint3 v8;
    NiPoint3 dst;
    NiPoint3 result;
    float v15 = (aSegment1Start.x - aSegment2End.x) * (aSegment1End.y - aSegment2End.y) -
        (aSegment1Start.y - aSegment2End.y) * (aSegment1End.x - aSegment2End.x);
    float v12 = (aSegment1Start.x - aSegment2Start.x) * (aSegment1End.y - aSegment2Start.y) -
        (aSegment1Start.y - aSegment2Start.y) * (aSegment1End.x - aSegment2Start.x);
    float v13 = (aSegment2Start.x - aSegment1Start.x) * (aSegment2End.y - aSegment1Start.y) -
        (aSegment2Start.y - aSegment1Start.y) * (aSegment2End.x - aSegment1Start.x);
    float v14 = (aSegment2Start.x - aSegment1End.x) * (aSegment2End.y - aSegment1End.y) -
        (aSegment2Start.y - aSegment1End.y) * (aSegment2End.x - aSegment1End.x);
    if (!(v15 * v12 <= 0.0) || !(v13 * v14 <= 0.0)) {
        return false;
    }
    if (v15 == 0.0) {
        return false;
    }
    if (v14 == 0.0) {
        return false;
    }
    float v11 = v13 / (v13 - v14);
    NiPoint3* v6 = &(result = aSegment1End - aSegment1Start);
    NiPoint3* v7 = &(dst = v11 * *v6);
    *apReturnIntersection = (v8 = aSegment1Start + *v7);
    return true;
}

int8_t NavMeshMath::Get2DBaryCentricCoord_0(float* apA, float* apB, float* apC, float* apD, float* apReturn) {
    float v6 = (*apB - *apA) * (apC[1] - apA[1]) - (*apC - *apA) * (apB[1] - apA[1]);
    if (v6 == 0.0) {
        return 0;
    }
    float v7 = 1.0 / v6;
    *apReturn = ((*apB - *apD) * (apC[1] - apD[1]) - (*apC - *apD) * (apB[1] - apD[1])) * v7;
    apReturn[1] = ((*apC - *apD) * (apA[1] - apD[1]) - (*apA - *apD) * (apC[1] - apD[1])) * v7;
    apReturn[2] = ((*apA - *apD) * (apB[1] - apD[1]) - (*apB - *apD) * (apA[1] - apD[1])) * v7;
    return 1;
}

bool NavMeshMath::Find2DIntersection(const NiPoint2& a1, const NiPoint2& a2, const NiPoint2& a3, const NiPoint2& a4) {
    float v8 = (((&a1)->x - (&a4)->x) * ((&a2)->y - (&a4)->y)) - (((&a1)->y - (&a4)->y) * ((&a2)->x - (&a4)->x));
    float v7 = (((&a1)->x - (&a3)->x) * ((&a2)->y - (&a3)->y)) - (((&a1)->y - (&a3)->y) * ((&a2)->x - (&a3)->x));
    uint8_t result = 0;
    if (!((v8 * v7) <= 0.0)) {
        return (bool)result;
    }
    float v5 = (((&a3)->x - (&a1)->x) * ((&a4)->y - (&a1)->y)) - (((&a3)->y - (&a1)->y) * ((&a4)->x - (&a1)->x));
    float v6 = (v5 + v7) - v8;
    if ((v5 * v6) < 0.0) {
        return true;
    }
    return (bool)result;
}

bool NavMeshMath::Find2DIntersection3(
    const NiPoint3& aSegment1Start,
    const NiPoint3& aSegment1End,
    const NiPoint3& aSegment2Start,
    const NiPoint3& aSegment2End
) {
    NiPoint2 v9;
    NiPoint2 v10;
    NiPoint2 v11;
    NiPoint2 v12;
    NiPoint2* y = ::new (&v12) NiPoint2(aSegment2End.x, aSegment2End.y);
    NiPoint2* x = ::new (&v11) NiPoint2(aSegment2Start.x, aSegment2Start.y);
    NiPoint2* a1 = ::new (&v10) NiPoint2(aSegment1End.x, aSegment1End.y);
    NiPoint2* v4 = ::new (&v9) NiPoint2(aSegment1Start.x, aSegment1Start.y);
    return NavMeshMath::Find2DIntersection(*v4, *a1, *x, *y);
}

bool NavMeshMath::Find2DIntersectionPoint(
    const NiPoint3& aSegment1Start,
    const NiPoint3& aSegment1End,
    const NiPoint3& aSegment2Start,
    const NiPoint3& aSegment2End,
    NiPoint3* apReturnIntersection
) {
    NiPoint3 v8;
    NiPoint3 dst;
    NiPoint3 result;
    float v15 = (aSegment1Start.x - aSegment2End.x) * (aSegment1End.y - aSegment2End.y) -
        (aSegment1Start.y - aSegment2End.y) * (aSegment1End.x - aSegment2End.x);
    float v14 = (aSegment1Start.x - aSegment2Start.x) * (aSegment1End.y - aSegment2Start.y) -
        (aSegment1Start.y - aSegment2Start.y) * (aSegment1End.x - aSegment2Start.x);
    if (!(v15 * v14 <= 0.0)) {
        return false;
    }
    float v12 = (aSegment2Start.x - aSegment1Start.x) * (aSegment2End.y - aSegment1Start.y) -
        (aSegment2Start.y - aSegment1Start.y) * (aSegment2End.x - aSegment1Start.x);
    float v13 = v12 + v14 - v15;
    if (!(v12 * v13 < 0.0)) {
        return false;
    }
    float v11 = v12 / (v12 - v13);
    NiPoint3* v5 = &(result = aSegment1End - aSegment1Start);
    NiPoint3* v6 = &(dst = v11 * *v5);
    *apReturnIntersection = (v8 = aSegment1Start + *v6);
    return true;
}

NiPoint3
NavMeshMath::ProjectPointOntoSegment(const NiPoint3& aPoint, const NiPoint3& aLineStart, const NiPoint3& aLineEnd) {
    NiPoint3 result;
    NiPoint3 v = aLineEnd - aLineStart;
    NiPoint3 w = aPoint - aLineStart;
    double c1 = w.Dot(v);
    if (!(c1 <= 0.0)) {
        double c2 = v.Dot(v);
        if (!(c1 >= c2)) {
            double b = c1 / c2;
            float v6 = b;
            result = aLineStart + v6 * v;
        }
        else {
            result = aLineEnd;
        }
        return result;
    }

    result = aLineStart;
    return result;
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
    NiPoint3 result;
    NiPoint3 v11;
    NiPoint3 v12;
    NiPoint3 dst;
    NiPoint3 resulta;
    if (aSegmentStart == aSegmentEnd) {
        result = aSegmentStart;
        return result;
    }

    NiPoint3 v18;
    float v17 = (aLineStart.x - aSegmentEnd.x) * (aLineEnd.y - aSegmentEnd.y) -
        (aLineStart.y - aSegmentEnd.y) * (aLineEnd.x - aSegmentEnd.x);
    float v15 = (aLineStart.x - aSegmentStart.x) * (aLineEnd.y - aSegmentStart.y) -
        (aLineStart.y - aSegmentStart.y) * (aLineEnd.x - aSegmentStart.x);
    float v16 = v15 / (v15 - v17);
    NiPoint3* v6 = &(resulta = aSegmentEnd - aSegmentStart);
    NiPoint3* v7 = &(dst = v16 * *v6);
    v18 = aSegmentStart + *v7;
    if (!(v17 * v15 > 0.0)) {
        result = v18;
        return result;
    }

    NiPoint3* v8 = &(v12 = v18 - aSegmentStart);
    double v10 = v8->SqrLength();
    NiPoint3* v9 = &(v11 = v18 - aSegmentEnd);
    if (!(v9->SqrLength() > v10)) {
        result = aSegmentEnd;
    }
    else {
        result = aSegmentStart;
    }
    return result;
}

float NavMeshMath::DistanceFromSegment(
    const NiPoint3& aPoint,
    const NiPoint3& aLineStart,
    const NiPoint3& aLineEnd,
    bool* apRetInSegment
) {
    NiPoint3 v11;
    NiPoint3 dst;
    NiPoint3 v13;
    NiPoint3 v14;
    NiPoint3 result;
    NiPoint3 v20;
    result = aLineEnd - aLineStart;
    v20 = aPoint - aLineStart;
    double v18 = v20.Dot(result);
    if (!(v18 <= 0.0)) {
        double v17 = result.Dot(result);
        if (!(v18 >= v17)) {
            NiPoint3 v15;
            double v16 = v18 / v17;
            float v10 = v16;
            NiPoint3* v7 = &(dst = v10 * result);
            v15 = aLineStart + *v7;
            if (apRetInSegment) {
                *apRetInSegment = true;
            }
            NiPoint3* v8 = &(v11 = aPoint - v15);
            return v8->Length();
        }

        if (apRetInSegment) {
            *apRetInSegment = false;
        }
        NiPoint3* v6 = &(v13 = aPoint - aLineEnd);
        return v6->Length();
    }

    if (apRetInSegment) {
        *apRetInSegment = false;
    }
    NiPoint3* v4 = &(v14 = aPoint - aLineStart);
    return v4->Length();
}

bool NavMeshMath::DoCircleAndSegmentIntersect(
    const NiPoint3& aCircleCenter,
    float afCircleRadius,
    const NiPoint3& aSegmentStart,
    const NiPoint3& aSegmentEnd
) {
    NiPoint3 v8;
    NiPoint3 dst;
    NiPoint3 result;
    NiPoint3 ecx0;
    NiPoint3 v15;
    result = aSegmentEnd - aSegmentStart;
    v15 = aCircleCenter - aSegmentStart;
    float v12 = result.Dot(v15);
    if (!(v12 <= 0.0)) {
        float v11 = result.Dot(result);
        if (!(v12 >= static_cast<double>(v11))) {
            NiPoint3 v9;
            float v7 = v12 / v11;
            NiPoint3* v4 = &(dst = v7 * result);
            ecx0 = (v9 = aSegmentStart + *v4);
        }
        else {
            ecx0 = aSegmentEnd;
        }
    }
    else {
        ecx0 = aSegmentStart;
    }
    NiPoint3* v5 = &(v8 = aCircleCenter - ecx0);
    return afCircleRadius * afCircleRadius > v5->SqrLength();
}

bool NavMeshMath::CircleSegmentIntersection(
    const Sphere& aCircle,
    const Segment& aSegment,
    NavMeshMath::CSIResults& aResults
) {
    AngleSinCos resultParamY;
    float finvDenomY;
    float finvDeltaY;
    bool bdox;
    Segment relativeSegment;
    float fEpsilon = 0.001f;
    if (aCircle.fRadius <= 0.0) {
        return false;
    }
    NavMeshMath::CSIResult* result0 = aResults.Results;
    NavMeshMath::CSIResult* result1 = &aResults.Results[1];
    relativeSegment = aSegment;
    relativeSegment.Offset(-aCircle.Center);
    float fradius = 1.0f;
    relativeSegment.Start /= aCircle.fRadius;
    relativeSegment.End /= aCircle.fRadius;
    NiPoint3 delta = relativeSegment.Delta();
    float fr2 = fradius * fradius;
    if (delta.y != 0.0 || delta.x == 0.0) {
        if (delta.y == 0.0 || delta.x != 0.0) {
            double v9 = std::fabs(delta.x);
            bdox = std::fabs(delta.y) < v9;
        }
        else {
            bdox = false;
        }
    }
    else {
        bdox = true;
    }
    if (bdox) {
        float finvDeltaX = 1.0 / delta.x;
        float fm = delta.y * finvDeltaX;
        float fp = relativeSegment.Start.y - fm * relativeSegment.Start.x;
        float fm2 = fm * fm;
        float fb = fm2 + 1.0;
        float fp2 = fp * fp;
        float fdeltaCos = (fr2 * fb - fp2) * fr2;
        if (fdeltaCos < -fEpsilon) {
            aResults.eType = NavMeshMath::CSI_NONE;
            return false;
        }
        if (!(fEpsilon > static_cast<double>(fdeltaCos))) {
            AngleSinCos v27;
            AngleSinCos v28;
            aResults.eType = NavMeshMath::CSI_TWOPOINTS;
            float finvDenom_2 = 1.0 / (fr2 * fb);
            float fsqrtDelta = std::sqrt(fdeltaCos);
            float fnom_2 = fp * fradius;
            v28.fCos = (-fnom_2 * fm + fsqrtDelta) * finvDenom_2;
            v28.fSin = (fm * fsqrtDelta + fnom_2) * finvDenom_2;
            v27.fCos = (-fnom_2 * fm - fsqrtDelta) * finvDenom_2;
            v27.fSin = (fnom_2 - fm * fsqrtDelta) * finvDenom_2;
            if (!(delta.x < 0.0)) {
                result0->CircleParam = v27;
                result1->CircleParam = v28;
            }
            else {
                result0->CircleParam = v28;
                result1->CircleParam = v27;
            }
            result0->fSegmentParam = (fradius * result0->CircleParam.fCos - relativeSegment.Start.x) * finvDeltaX;
            result1->fSegmentParam = (fradius * result1->CircleParam.fCos - relativeSegment.Start.x) * finvDeltaX;
        }
        else {
            AngleSinCos resultParam;
            aResults.eType = NavMeshMath::CSI_TANGENT;
            float finvDenom = 1.0 / (fr2 * fb);
            float fnom = fp * fradius;
            resultParam.fCos = -fnom * fm * finvDenom;
            resultParam.fSin = fnom * finvDenom;
            result0->CircleParam = resultParam;
            result0->fSegmentParam = (fradius * result0->CircleParam.fCos - relativeSegment.Start.x) * finvDeltaX;
        }
    }
    else {
        finvDeltaY = 1.0 / delta.y;
        float v21 = delta.x * finvDeltaY;
        float v22 = relativeSegment.Start.x - v21 * relativeSegment.Start.y;
        float v26 = v21 * v21;
        float v24 = v26 + 1.0;
        float v25 = v22 * v22;
        float fdeltaSin = (fr2 * v24 - v25) * fr2;
        if (fdeltaSin < -fEpsilon) {
            aResults.eType = NavMeshMath::CSI_NONE;
            return false;
        }
        if (!(fEpsilon > static_cast<double>(fdeltaSin))) {
            AngleSinCos v14;
            AngleSinCos v15;
            aResults.eType = NavMeshMath::CSI_TWOPOINTS;
            float v18 = 1.0 / (fr2 * v24);
            float fsqrtDelta_2 = std::sqrt(fdeltaSin);
            float v17 = v22 * fradius;
            v15.fCos = (v21 * fsqrtDelta_2 + v17) * v18;
            v15.fSin = (-v17 * v21 + fsqrtDelta_2) * v18;
            v14.fCos = (v17 - v21 * fsqrtDelta_2) * v18;
            v14.fSin = (-v17 * v21 - fsqrtDelta_2) * v18;
            if (!(delta.y < 0.0)) {
                result0->CircleParam = v14;
                result1->CircleParam = v15;
            }
            else {
                result0->CircleParam = v15;
                result1->CircleParam = v14;
            }
            result0->fSegmentParam = (fradius * result0->CircleParam.fSin - relativeSegment.Start.y) * finvDeltaY;
            result1->fSegmentParam = (fradius * result1->CircleParam.fSin - relativeSegment.Start.y) * finvDeltaY;
        }
        else {
            aResults.eType = NavMeshMath::CSI_TANGENT;
            finvDenomY = 1.0 / (fr2 * v24);
            float v19 = v22 * fradius;
            resultParamY.fCos = v19 * finvDenomY;
            resultParamY.fSin = -v19 * v21 * finvDenomY;
            result0->CircleParam = resultParamY;
            result0->fSegmentParam = (fradius * result0->CircleParam.fSin - relativeSegment.Start.y) * finvDeltaY;
        }
    }
    if (aResults.eType == NavMeshMath::CSI_TANGENT) {
        bool v8 = result0->fSegmentParam >= -fEpsilon && result0->fSegmentParam <= fEpsilon + 1.0;
        if (v8) {
            if (!(fEpsilon > static_cast<double>(result0->fSegmentParam)) &&
                !(result0->fSegmentParam > 1.0 - fEpsilon)) {
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

    bool v7 = result0->fSegmentParam >= -fEpsilon && result0->fSegmentParam <= fEpsilon + 1.0;
    bool v6 = result1->fSegmentParam >= -fEpsilon && result1->fSegmentParam <= fEpsilon + 1.0;
    if (v7 && v6) {
        aResults.eType = NavMeshMath::CSI_TWOPOINTS;
        if (!(fEpsilon > static_cast<double>(result0->fSegmentParam)) && !(result0->fSegmentParam > 1.0 - fEpsilon)) {
            aResults.Results[0u].eType = NavMeshMath::CSIT_EDGE;
        }
        else {
            aResults.Results[0u].eType = NavMeshMath::CSIT_ENDPOINT;
        }
        if (!(fEpsilon > static_cast<double>(result1->fSegmentParam)) && !(result1->fSegmentParam > 1.0 - fEpsilon)) {
            aResults.Results[1].eType = NavMeshMath::CSIT_EDGE;
        }
        else {
            aResults.Results[1].eType = NavMeshMath::CSIT_ENDPOINT;
        }
        return true;
    }

    if (!v7 || v6) {
        if (v7 || !v6) {
            if (!(fr2 > relativeSegment.Start.SqrXYLength())) {
                aResults.eType = NavMeshMath::CSI_NONE;
            }
            else {
                aResults.eType = NavMeshMath::CSI_CONTAINED;
            }
            return false;
        }

        aResults.eType = NavMeshMath::CSI_ONEPOINT_LOWER;
        result0->CircleParam.operator=(result1->CircleParam);
        result0->fSegmentParam = result1->fSegmentParam;
        if (!(fEpsilon > static_cast<double>(result0->fSegmentParam)) && !(result0->fSegmentParam > 1.0 - fEpsilon)) {
            aResults.Results[0u].eType = NavMeshMath::CSIT_EDGE;
        }
        else {
            aResults.Results[0u].eType = NavMeshMath::CSIT_ENDPOINT;
        }
        return true;
    }

    aResults.eType = NavMeshMath::CSI_ONEPOINT_UPPER;
    if (!(fEpsilon > static_cast<double>(result0->fSegmentParam)) && !(result0->fSegmentParam > 1.0 - fEpsilon)) {
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
    int32_t ivalidCount = 0;
    NiPoint3 v = aCircle2.Center - aCircle1.Center;
    float fradius1 = aCircle1.fRadius;
    float fradius2 = aCircle2.fRadius;
    if (aCircle2.fRadius == 0.0) {
        if (aCircle1.fRadius != 0.0) {
            fradius1 = 1.0f;
            fradius2 = aCircle2.fRadius / aCircle1.fRadius;
            v /= aCircle1.fRadius;
        }
    }
    else {
        fradius2 = 1.0f;
        fradius1 = aCircle1.fRadius / aCircle2.fRadius;
        v /= aCircle2.fRadius;
    }
    float sqr_length = v.SqrXYLength();
    if (sqr_length == 0.0) {
        aResults.eType = NavMeshMath::CCT_NONE;
        return NavMeshMath::CCT_NONE;
    }

    float deltaRadius = fradius1 - fradius2;
    float deltaRadius_sqr = deltaRadius * deltaRadius;
    if (deltaRadius_sqr <= static_cast<double>(sqr_length)) {
        double v6;
        float ratio = 1.0 / sqr_length;
        float v9 = sqr_length - deltaRadius_sqr;
        float dist = std::sqrt(v9);
        if (aDirection) {
            aResults.Results[0u].Direction = AD_CW;
            aResults.Results[0u].Circle1Param.fCos = (deltaRadius * v.x - v.y * dist) * ratio;
            v6 = (deltaRadius * v.y + v.x * dist) * ratio;
        }
        else {
            aResults.Results[0u].Direction = AD_CCW;
            aResults.Results[0u].Circle1Param.fCos = (deltaRadius * v.x + v.y * dist) * ratio;
            v6 = (deltaRadius * v.y - v.x * dist) * ratio;
        }
        aResults.Results[0u].Circle1Param.fSin = v6;
        aResults.Results[ivalidCount].Circle2Param.fCos = aResults.Results[ivalidCount].Circle1Param.fCos;
        aResults.Results[ivalidCount].Circle2Param.fSin = aResults.Results[ivalidCount].Circle1Param.fSin;
        ++ivalidCount;
    }
    float sumRadius = fradius1 + fradius2;
    float sumRadius_sqr = sumRadius * sumRadius;
    if (sumRadius_sqr <= static_cast<double>(sqr_length)) {
        double v7;
        float ratio_2 = 1.0 / sqr_length;
        float v8 = sqr_length - sumRadius_sqr;
        float dist_2 = std::sqrt(v8);
        if (aDirection) {
            aResults.Results[ivalidCount].Direction = AD_CCW;
            aResults.Results[ivalidCount].Circle1Param.fCos = (sumRadius * v.x - v.y * dist_2) * ratio_2;
            v7 = (sumRadius * v.y + v.x * dist_2) * ratio_2;
        }
        else {
            aResults.Results[ivalidCount].Direction = AD_CW;
            aResults.Results[ivalidCount].Circle1Param.fCos = (sumRadius * v.x + v.y * dist_2) * ratio_2;
            v7 = (sumRadius * v.y - v.x * dist_2) * ratio_2;
        }
        aResults.Results[ivalidCount].Circle1Param.fSin = v7;
        aResults.Results[ivalidCount].Circle2Param.fCos = -aResults.Results[ivalidCount].Circle1Param.fCos;
        aResults.Results[ivalidCount].Circle2Param.fSin = -aResults.Results[ivalidCount].Circle1Param.fSin;
        ++ivalidCount;
    }
    aResults.eType = static_cast<NavMeshMath::CIRCLE_CIRCLE_TANGENT>(ivalidCount);
    return static_cast<NavMeshMath::CIRCLE_CIRCLE_TANGENT>(ivalidCount);
}

float NavMeshMath::DistanceFromLine(const NiPoint3& aPoint, const NiPoint3& aLineStart, const NiPoint3& aLineEnd) {
    NiPoint3 dst;
    NiPoint3 v8;
    NiPoint3 result;
    NiPoint3 v13;
    NiPoint3 v14;
    result = aLineEnd - aLineStart;
    v13 = aPoint - aLineStart;
    float v11 = v13.Dot(result);
    float v10 = result.Dot(result);
    float v9 = v11 / v10;
    NiPoint3* v3 = &(dst = v9 * result);
    v14 = aLineStart + *v3;
    v8 = aPoint - v14;
    float afX = v8.Dot(v8);
    return std::sqrt(afX);
}

int32_t NavMeshMath::SegmentPlaneIntersection(
    const NiPoint3& aLineStart,
    const NiPoint3& aLineEnd,
    const NiPoint3& aPlaneV0,
    const NiPoint3& aPlaneNormal,
    NiPoint3* apRet
) {
    NiPoint3 dst;
    NiPoint3 result;
    NiPoint3 v14;
    float v11 = 1e-07f;
    result = aLineEnd - aLineStart;
    v14 = aLineStart - aPlaneV0;
    float v9 = aPlaneNormal.Dot(result);
    float v10 = -aPlaneNormal.Dot(v14);
    if (!(static_cast<double>(1e-07f) > std::fabs(v9))) {
        float v13 = v10 / v9;
        if (!(v13 < 0.0) && !(v13 > 1.0)) {
            NiPoint3 v7;
            NiPoint3* v6 = &(dst = v13 * result);
            *apRet = (v7 = aLineStart + *v6);
            return 1;
        }

        return 0;
    }

    if (v10 == 0.0) {
        return -1;
    }

    return 0;
}

bool NavMeshMath::SegmentCircleIntersection(
    const NiPoint3& aSegmentStart,
    const NiPoint3& aSegmentEnd,
    const NiPoint3& aCircleCenter,
    float afCircleRadius
) {
    NiPoint3 v8;
    NiPoint3 dst;
    NiPoint3 result;
    NiPoint3 ecx0;
    NiPoint3 pt;
    result = aSegmentEnd - aSegmentStart;
    pt = aCircleCenter - aSegmentStart;
    float v12 = result.Dot(pt);
    if (!(v12 <= 0.0)) {
        float v11 = result.Dot(result);
        if (!(v12 >= static_cast<double>(v11))) {
            NiPoint3 v9;
            float v7 = v12 / v11;
            NiPoint3* v4 = &(dst = v7 * result);
            ecx0 = (v9 = aSegmentStart + *v4);
        }
        else {
            ecx0 = aSegmentEnd;
        }
    }
    else {
        ecx0 = aSegmentStart;
    }
    NiPoint3* v5 = &(v8 = aCircleCenter - ecx0);
    return afCircleRadius * afCircleRadius > v5->SqrLength();
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
    float fd3 = delta.SqrLength();
    float fd2 = delta.SqrXYLength();
    if (fd2 == 0.0) {
        return false;
    }
    float fsum = aCircle1.fRadius + aCircle2.fRadius;
    if (fd2 > fsum * fsum) {
        return true;
    }
    if (fd3 > fsum * fsum) {
        return true;
    }
    float fdiff = aCircle1.fRadius - aCircle2.fRadius;
    if (fd2 < fdiff * fdiff) {
        return true;
    }
    if (fd3 < fdiff * fdiff) {
        return true;
    }
    float fr12 = aCircle1.fRadius * aCircle1.fRadius;
    float fr22 = aCircle2.fRadius * aCircle2.fRadius;
    float fD = std::sqrt(fd2);
    float finvD = 1.0 / fD;
    float finvR1 = 1.0 / aCircle1.fRadius;
    float fa = (fr12 - fr22 + fd2) * 0.5 * finvD;
    float fa2 = fa * fa;
    float v5 = fr12 - fa2;
    float fh = std::sqrt(v5);
    angle1.fCos = (fa * delta.x + fh * delta.y) * finvR1 * finvD;
    angle1.fSin = (fa * delta.y - fh * delta.x) * finvR1 * finvD;
    angle2.fCos = (fa * delta.x - fh * delta.y) * finvR1 * finvD;
    angle2.fSin = (fa * delta.y + fh * delta.x) * finvR1 * finvD;
    return !aArc.WithinRange(angle1) && !aArc.WithinRange(angle2);
}

NiPoint3 NavMeshMath::RandomPointOnTriangle(const NiPoint3& aVert0, const NiPoint3& aVert1, const NiPoint3& aVert2) {
    NiPoint3 v8;
    float frandEdge2;
    NiPoint3 edge1 = aVert1 - aVert0;
    NiPoint3 edge2 = aVert2 - aVert0;
    float frandEdge1 = RandomFloat0To1();
    for (frandEdge2 = RandomFloat0To1(); frandEdge1 + frandEdge2 > 1.0; frandEdge2 = RandomFloat0To1()) {
        frandEdge1 = RandomFloat0To1();
    }
    NiPoint3* fScalar = ::new (&v8) NiPoint3(edge2 * frandEdge2);
    return aVert0 + edge1 * frandEdge1 + *fScalar;
}

bool NavMeshMath::Find2DIntersection_006B9E90(
    const NiPoint2& aSegment1Start,
    const NiPoint2& aSegment1End,
    const NiPoint2& aSegment2Start,
    const NiPoint2& aSegment2End,
    NiPoint2* apReturnIntersection
) {
    NiPoint2 v10;
    NiPoint2 apResult;
    NiPoint2 result;
    float v17 = (aSegment1Start.x - aSegment2End.x) * (aSegment1End.y - aSegment2End.y) -
        (aSegment1Start.y - aSegment2End.y) * (aSegment1End.x - aSegment2End.x);
    float v16 = (aSegment1Start.x - aSegment2Start.x) * (aSegment1End.y - aSegment2Start.y) -
        (aSegment1Start.y - aSegment2Start.y) * (aSegment1End.x - aSegment2Start.x);
    if (!(v17 * v16 < 0.0)) {
        return false;
    }
    float v14 = (aSegment2Start.x - aSegment1Start.x) * (aSegment2End.y - aSegment1Start.y) -
        (aSegment2Start.y - aSegment1Start.y) * (aSegment2End.x - aSegment1Start.x);
    float v15 = v14 + v16 - v17;
    if (std::fabs(v15) < 0.0010000000474974513) {
        v15 = (aSegment2Start.x - aSegment1End.x) * (aSegment2End.y - aSegment1End.y) -
            (aSegment2Start.y - aSegment1End.y) * (aSegment2End.x - aSegment1End.x);
    }
    if (!(v14 * v15 < 0.0)) {
        return false;
    }
    float afScale = v14 / (v14 - v15);
    NiPoint2* v5 = &(result = aSegment1End - aSegment1Start);
    NiPoint2* v6 = NiPoint2::Scale_(&apResult, afScale, v5);
    NiPoint2* v7 = &(v10 = aSegment1Start + *v6);
    float y = v7->y;
    apReturnIntersection->x = v7->x;
    apReturnIntersection->y = y;
    return true;
}

bool NavMeshMath::LineToLineSegment2DIntersection(
    const NiPoint3* apLeftVertex,
    const NiPoint3* apRightVertex,
    const NiPoint3* apTargetLocation,
    const NiPoint3* apCoverPoint,
    NiPoint3* apIntersection
) {
    NiPoint3 dst;
    NiPoint3 v10;
    NiPoint3 result;
    NiPoint3 v13;
    result = *apRightVertex - *apLeftVertex;
    v13 = *apCoverPoint - *apTargetLocation;
    float v14 = result.x * v13.y - result.y * v13.x;
    if (v14 == 0.0) {
        return false;
    }
    v10 = *apTargetLocation - *apLeftVertex;
    float v9 = v10.x * result.y - v10.y * result.x;
    float v15 = v9 / v14;
    if (v15 < 0.0 || v15 > 1.0) {
        return false;
    }
    float v12 = v10.x * v13.y - v10.y * v13.x;
    v15 = v12 / v14;
    if (apIntersection) {
        NiPoint3 v7;
        NiPoint3* v6 = &(dst = v15 * result);
        *apIntersection = (v7 = *apLeftVertex + *v6);
    }
    return true;
}

char NavMeshMath::LineSegmentToLine2DIntersection(
    NiPoint3* a1,
    NiPoint3* a2,
    NiPoint3* pt,
    NiPoint3* a4,
    NiPoint3* a5
) {
    NiPoint3 dst;
    NiPoint3 v10;
    NiPoint3 result;
    NiPoint3 v12;
    result = *a4 - *pt;
    v12 = *a2 - *a1;
    float v13 = result.x * v12.y - result.y * v12.x;
    if (v13 == 0.0) {
        return 0;
    }
    v10 = *a1 - *pt;
    float v9 = v10.x * result.y - v10.y * result.x;
    float v14 = v9 / v13;
    if (v14 < 0.0 || v14 > 1.0) {
        return 0;
    }
    if (a5) {
        NiPoint3 v7;
        NiPoint3* v6 = &(dst = v14 * v12);
        *a5 = (v7 = *a1 + *v6);
    }
    return 1;
}

char NavMeshMath::ComputeTangentPointsToCircleFromPoint(NiPoint3* pt, float a2, NiPoint3* a3, float* a4, float* a5) {
    NiPoint3 result;
    (result = *a3 - *pt);
    float v9 = (&result)->SqrXYLength();
    float v10 = a2 * a2;
    if (!(v10 <= ((double)v9))) {
        return 0;
    }
    float v8 = (float)(1.0 / v9);
    float afX = v9 - v10;
    float v7 = std::sqrt(afX);
    *a4 = pt->x;
    a4[1] = pt->y;
    a4[2] = pt->z;
    *a5 = pt->x;
    a5[1] = pt->y;
    a5[2] = pt->z;
    *a4 = (((((a2 * result.x) - (result.y * v7)) * a2) * v8) + (*a4));
    a4[1] = (((((a2 * result.y) + (result.x * v7)) * a2) * v8) + a4[1]);
    *a5 = (((((a2 * result.x) + (result.y * v7)) * a2) * v8) + (*a5));
    a5[1] = (((((a2 * result.y) - (result.x * v7)) * a2) * v8) + a5[1]);
    return 1;
}

Segment::Segment(const NiPoint3& aSegment, const NiPoint3& a2) : Start(aSegment), End(a2) {}

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
