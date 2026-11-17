#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

class hkSphere;

// A shape that can be represented by a few collision spheres. It adds no data, only the two virtual queries for the
// spheres.
class hkpSphereRepShape : public hkpShape {
public:
	hkpSphereRepShape(hkpShapeType type) : hkpShape(type) {}
	hkpSphereRepShape(hkFinishLoadedObjectFlag flag) : hkpShape(flag) { m_type = HK_SHAPE_SPHERE_REP; }

	virtual int32_t getNumCollisionSpheresImpl() const = 0;
	virtual const hkSphere* getCollisionSpheresImpl(hkSphere* aSphereBuffer) const = 0;
};

// The class's reflection record.
extern hkClass hkpSphereRepShapeClass;
