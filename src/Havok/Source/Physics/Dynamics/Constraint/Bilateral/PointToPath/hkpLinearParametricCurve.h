#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpParametricCurve.h"

// A curve made of straight segments between points, optionally smoothed. Only some of its fields are declared.
class hkpLinearParametricCurve : public hkpParametricCurve {
public:
    hkpLinearParametricCurve();

    // The SDK's finish constructor of a loaded object; assumes hkpParametricCurve's (the SDK's).
    hkpLinearParametricCurve(hkFinishLoadedObjectFlag aFlag)
      : hkpParametricCurve(aFlag), m_points(aFlag), m_distance(aFlag) {}

    ~hkpLinearParametricCurve() override;

    void getPoint(float t, hkVector4& pointOnCurve) const override;
    float getNearestPoint(float t, const hkVector4& nearPoint, hkVector4& pointOnCurve) const override;
    void getTangent(float t, hkVector4& tangent) const override;
    float getStart() const override;
    float getEnd() const override;
    float getLengthFromStart(float t) const override;
    void getBinormal(float t, hkVector4& up) const override;
    hkBool* isClosedLoop(hkBool* arResult) const override;
    void getPointsToDraw(hkArray<hkVector4>& pathPoints) const override;
    void transformPoints(const hkTransform& transformation) override;
    hkpParametricCurve* clone() override;

    // Appends p; its distance along the curve is the previous point's plus the segment's length.
    void addPoint(const hkVector4& p);
    void setClosedLoop(hkBool closeLoop);
    float getSmoothingFactor();
    void setSmoothingFactor(float smooth);

    // Havok SDK signature (hkpDynamics.lib, tied by target).
    static const hkClass& staticClass();

    float m_smoothingFactor;
    hkBool m_closedLoop;
    hkVector4 m_dirNotParallelToTangentAlongWholePath;
    hkArray<hkVector4> m_points;
    hkArray<float> m_distance;
};
