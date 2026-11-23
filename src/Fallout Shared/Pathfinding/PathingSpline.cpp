#include "Fallout Shared/Pathfinding/PathingSpline.h"

#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

#include <cstdint>

PathingSpline::PathingSpline() : Key1(), Key2() {}

PathingSpline::~PathingSpline() {}

void PathingSpline::SetParameter(float afParameter) {
    ComputeHermiteBasis(afParameter);
}

void PathingSpline::SetKeys(const PathingSplineKey& aKey1, const PathingSplineKey& aKey2) {
    Key1 = aKey1;
    Key2 = aKey2;
}

void PathingSpline::ComputeHermiteBasis(float afParameter) {
    Spline.fParameter = afParameter;
    float t2 = afParameter * afParameter;
    float t3 = t2 * afParameter;
    Spline.fH01 = t3 * -2.0 + t2 * 3.0;
    // The blend factor is negated and then 1.0 is added, which flips a NaN's sign bit where 1.0 - x
    // would not; the negation in its own local keeps clang from folding the pair into a subtraction.
    float fNegH01 = -Spline.fH01;
    Spline.fH00 = fNegH01 + 1.0;
    Spline.fH11 = t3 - t2;
    Spline.fH10 = Spline.fH11 - t2 + afParameter;
}

NiPoint3 PathingSpline::Interpolate(float afParameter) {
    NiPoint3 kPoint;
    SetParameter(afParameter);
    kPoint = InterpolateNoZ(Key1, Key2);
    return kPoint;
}

float PathingSpline::ApproxLength(float afStartParam, float afEndParam, uint32_t aiSubdivisions) {
    NiPoint3 kLastPoint = Interpolate(afStartParam);
    float fLength = 0.0f;
    for (uint32_t i = 0; i < aiSubdivisions; ++i) {
        float fParameter = static_cast<float>(
            (static_cast<double>(afEndParam) - static_cast<double>(afStartParam)) * static_cast<double>(i + 1) /
                static_cast<double>(aiSubdivisions) +
            static_cast<double>(afStartParam)
        );
        BSClamp(fParameter, 0.0f, 1.0f);
        NiPoint3 kPoint = Interpolate(fParameter);
        fLength = static_cast<float>(static_cast<double>((kPoint - kLastPoint).Length()) + fLength);
        kLastPoint = kPoint;
    }
    return fLength;
}

bool PathingSpline::FindPointOnSpline(
    float afStartParameter,
    float afDistance,
    bool abForward,
    float* apfReturnParameter,
    NiPoint3* apReturnPoint,
    float afEpsilon
) {
    NiPoint3 kDelta;
    NiPoint3 kStartPoint;
    if (!(afEpsilon >= static_cast<double>(afDistance))) {
        float fLow;
        float fHigh;
        float fParameter;
        float fMaxDistSqr = (afDistance + afEpsilon) * (afDistance + afEpsilon);
        float fMinDistSqr = (afDistance - afEpsilon) * (afDistance - afEpsilon);
        BSClamp(afStartParameter, 0.0, 1.0);
        if (abForward) {
            fLow = afStartParameter;
            fHigh = 1.0;
            fParameter = 1.0;
        }
        else {
            fLow = 0.0;
            fHigh = afStartParameter;
            fParameter = 0.0;
        }
        kStartPoint = Interpolate(afStartParameter);
        PathingSplineKey* pEndKey = abForward ? &Key2 : &Key1;
        NiPoint3 kPoint = pEndKey->Location;
        char cFound = 0;
        while (!cFound) {
            kPoint = Interpolate(fParameter);
            kDelta = kStartPoint - kPoint;
            float fDistSqr = kDelta.SqrLength();
            if ((!(fMinDistSqr > static_cast<double>(fDistSqr)) || !abForward) &&
                (!(fMaxDistSqr < static_cast<double>(fDistSqr)) || abForward)) {
                if ((!(fMaxDistSqr < static_cast<double>(fDistSqr)) || !abForward) &&
                    (!(fMinDistSqr > static_cast<double>(fDistSqr)) || abForward)) {
                    cFound = 1;
                    break;
                }
                if (fParameter <= 0.009999999776482582) {
                    cFound = 0;
                    break;
                }
                fHigh = fParameter;
            }
            else {
                if (fParameter >= 0.9900000095367432) {
                    cFound = 0;
                    break;
                }
                fLow = fParameter;
            }
            fParameter = (fLow + fHigh) * 0.5;
        }
        if (cFound) {
            *apfReturnParameter = fParameter;
            *apReturnPoint = kPoint;
            return true;
        }

        if (abForward) {
            *apfReturnParameter = 1.0;
            *apReturnPoint = Key2.Location;
        }
        else {
            *apfReturnParameter = 0.0;
            *apReturnPoint = Key1.Location;
        }
        return false;
    }

    kStartPoint = Interpolate(afStartParameter);
    *apReturnPoint = kStartPoint;
    *apfReturnParameter = afStartParameter;
    return true;
}
