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
    (&Key1)->operator=(aKey1);
    (&Key2)->operator=(aKey2);
}

void PathingSpline::ComputeHermiteBasis(float afParameter) {
    Spline.fParameter = afParameter;
    float t2 = afParameter * afParameter;
    float t3 = t2 * afParameter;
    Spline.fH01 = t3 * -2.0 + t2 * 3.0;
    // The blend factor is negated and then 1.0 is added, as the game does, which flips a NaN's sign bit where 1.0 - x
    // would not; the negation in its own local keeps clang from folding the pair into a subtraction.
    float fNegH01 = -Spline.fH01;
    Spline.fH00 = fNegH01 + 1.0;
    Spline.fH11 = t3 - t2;
    Spline.fH10 = Spline.fH11 - t2 + afParameter;
}

NiPoint3 PathingSpline::Interpolate(float afParameter) {
    NiPoint3 result;
    SetParameter(afParameter);
    result = InterpolateNoZ(Key1, Key2);
    return result;
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
    NiPoint3 result;
    NiPoint3 v15;
    if (!(afEpsilon >= static_cast<double>(afDistance))) {
        PathingSplineKey* p_Key2;
        float v18;
        float v19;
        float v22;
        NiPoint3 v23;
        float v24 = (afDistance + afEpsilon) * (afDistance + afEpsilon);
        float v17 = (afDistance - afEpsilon) * (afDistance - afEpsilon);
        BSClamp(afStartParameter, 0.0, 1.0);
        if (abForward) {
            v18 = afStartParameter;
            v19 = 1.0;
            v22 = 1.0;
        }
        else {
            v18 = 0.0;
            v19 = afStartParameter;
            v22 = 0.0;
        }
        v23 = Interpolate(afStartParameter);
        if (abForward) {
            p_Key2 = &Key2;
        }
        else {
            p_Key2 = &Key1;
        }
        NiPoint3 pt = p_Key2->Location;
        char v20 = 0;
        while (!v20) {
            NiPoint3 v13;
            pt = (v13 = Interpolate(v22));
            NiPoint3* v9 = &(result = v23 - pt);
            float v16 = v9->SqrLength();
            if ((!(v17 > static_cast<double>(v16)) || !abForward) && (!(v24 < static_cast<double>(v16)) || abForward)) {
                if ((!(v24 < static_cast<double>(v16)) || !abForward) &&
                    (!(v17 > static_cast<double>(v16)) || abForward)) {
                    v20 = 1;
                    break;
                }
                if (v22 <= 0.009999999776482582) {
                    v20 = 0;
                    break;
                }
                v19 = v22;
            }
            else {
                if (v22 >= 0.9900000095367432) {
                    v20 = 0;
                    break;
                }
                v18 = v22;
            }
            v22 = (v18 + v19) * 0.5;
        }
        if (v20) {
            *apfReturnParameter = v22;
            *apReturnPoint = pt;
            return true;
        }
        else {
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
    }
    else {
        NiPoint3* v7 = &(v15 = Interpolate(afStartParameter));
        *apReturnPoint = *v7;
        *apfReturnParameter = afStartParameter;
        return true;
    }
}
