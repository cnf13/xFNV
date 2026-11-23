#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"

struct hkpShapeRayCastInput;

// Bethesda's triangle shape that also keeps the triangle's normal. Its one virtual function of its own is castRay.
class hkNormalTriangleShape : public hkpTriangleShape {
public:
    hkNormalTriangleShape(float afRadius);

    virtual hkBool castRay(const hkpShapeRayCastInput& arInput, hkpShapeRayCastOutput& arResults) const;

    hkVector4 Normal;
};
