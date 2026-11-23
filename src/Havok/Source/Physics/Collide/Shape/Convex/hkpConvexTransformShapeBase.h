#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"

#include <stdint.h>

// Base of convex shapes that wrap one convex child shape and cache its serialized size.
class hkpConvexTransformShapeBase : public hkpConvexShape {
public:
    hkpConvexTransformShapeBase(int32_t type, float radius, hkpConvexShape* childShape, int32_t ref);
    // The SDK's finish constructor of a loaded object (the derived shapes' finish constructors call it).
    hkpConvexTransformShapeBase(hkFinishLoadedObjectFlag flag) : hkpConvexShape(flag), m_childShape(flag) {}
    ~hkpConvexTransformShapeBase() override;

    hkpSingleShapeContainer m_childShape;
    // Mutable as in the SDK (hkpConvexShape.h): the const calcSizeForSpu overrides cache the child's size here.
    mutable int32_t m_childShapeSize;
};

// The class's reflection record.
extern hkClass hkpConvexTransformShapeBaseClass;
