#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Reconstruction.hpp"

class hkSphere;

// A shape that can be represented by a few collision spheres (the PDB places it in the heightfield sources). It adds
// no data, only the two virtual queries for the spheres.
class hkpSphereRepShape : public hkpShape {
public:
	// The SDK's constructors (inlined into its derived classes').
	hkpSphereRepShape(hkpShapeType type) : hkpShape(type) {}
	hkpSphereRepShape(hkFinishLoadedObjectFlag flag) : hkpShape(flag) { m_type = HK_SHAPE_SPHERE_REP; }

	// PDB getNumCollisionSpheresImpl (protected).
	virtual int32_t getNumCollisionSpheresImpl() const = 0;
	// PDB getCollisionSpheresImpl (protected).
	virtual const hkSphere* getCollisionSpheresImpl(hkSphere* aSphereBuffer) const = 0;
};

// The class's reflection record.
extern hkClass hkpSphereRepShapeClass;
