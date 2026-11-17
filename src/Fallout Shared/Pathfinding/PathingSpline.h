#pragma once

#include "BSMain/BSSpline.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// One end of a path spline: a point and the tangent the spline leaves it with.
struct PathingSplineKey {
    PathingSplineKey();

    // Copies aKey's point and tangent.
    PathingSplineKey& operator=(const PathingSplineKey& aKey);

    NiPoint3 Location;
    NiPoint3 Tangent;
};

// A Hermite spline between two keys.
class PathingSpline {
public:
    PathingSpline();
    ~PathingSpline();

    // The point between the keys without the height blend.
    NiPoint3 InterpolateNoZ(const PathingSplineKey& aKey1, const PathingSplineKey& aKey2);
    // Sets the spline's parameter and Hermite blend factors for afParameter.
    void SetParameter(float afParameter);
    // Copies the two keys.
    void SetKeys(const PathingSplineKey& aKey1, const PathingSplineKey& aKey2);
    // The point at the current parameter between aKey1 and aKey2.
    NiPoint3 Interpolate(const PathingSplineKey& aKey1, const PathingSplineKey& aKey2);
    // Sets Spline's parameter and Hermite blend factors for afParameter.
    void ComputeHermiteBasis(float afParameter);
    // The spline's point at afParameter.
    NiPoint3 Interpolate(float afParameter);
    // The length of the polyline through aiSubdivisions + 1 points between afStartParam and afEndParam.
    float ApproxLength(float afStartParam, float afEndParam, uint32_t aiSubdivisions);
    // Walks afDistance along the spline from afStartParameter (forward when abForward) to
    // within afEpsilon, writing the parameter and point reached; false when the spline ends first.
    bool FindPointOnSpline(
        float afStartParameter,
        float afDistance,
        bool abForward,
        float* apfReturnParameter,
        NiPoint3* apReturnPoint,
        float afEpsilon
    );

    // The parameter and the Hermite blend factors, passed to BSSpline::Interpolate.
    BSSpline Spline;
    PathingSplineKey Key1;
    PathingSplineKey Key2;
};
