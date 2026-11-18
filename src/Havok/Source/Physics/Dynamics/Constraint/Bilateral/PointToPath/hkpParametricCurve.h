#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkTransform;

// A curve a point-to-path constraint keeps a pivot on, parametrised by t. The parameters and lengths are hkReal (a
// float).
class hkpParametricCurve : public hkReferencedObject {
public:
    // The constructor the derived classes' own use, and the SDK's finish constructor of a loaded object (the derived
    // classes' finish constructors call it): it keeps the fields the load wrote.
    hkpParametricCurve() = default;
    hkpParametricCurve(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag) {}

    virtual void getPoint(float t, hkVector4& pointOnCurve) const = 0;
    virtual float getNearestPoint(float t, const hkVector4& nearPoint, hkVector4& pointOnCurve) const = 0;
    virtual void getTangent(float t, hkVector4& tangent) const = 0;
    virtual float getStart() const = 0;
    virtual float getEnd() const = 0;
    virtual float getLengthFromStart(float t) const = 0;
    virtual void getBinormal(float t, hkVector4& up) const = 0;
    virtual hkBool isClosedLoop() const = 0;
    virtual void getPointsToDraw(hkArray<hkVector4>& pathPoints) const = 0;
    virtual void transformPoints(const hkTransform& transformation) = 0;
    virtual hkpParametricCurve* clone() = 0;
};

// The class's reflection record.
extern hkClass hkpParametricCurveClass;
