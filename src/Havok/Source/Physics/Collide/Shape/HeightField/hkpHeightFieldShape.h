#pragma once

#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

class hkSphere;

// Base of the heightfield shapes: adds the two queries the heightfield agents use, colliding a set of spheres
// against the field and casting a sphere along it.
class hkpHeightFieldShape : public hkpShape {
public:
	// The spheres collideSpheres tests and the tolerance it uses.
	struct CollideSpheresInput {
		hkSphere* m_spheres;
		int32_t m_numSpheres;
		float m_tolerance;
	};

	// A ray cast input whose ray is the sphere's centre path, plus the sphere's radius and how far it may penetrate
	// the field (the PDB's members at 0x30 and 0x34, after the 0x30 bytes of hkpShapeRayCastInput).
	struct hkpSphereCastInput : public hkpShapeRayCastInput {
		float m_radius;
		float m_maxExtraPenetration;
	};

	// The SDK's constructors (inlined into its derived classes').
	hkpHeightFieldShape(hkpShapeType type) : hkpShape(type) {}
	hkpHeightFieldShape(hkFinishLoadedObjectFlag flag) : hkpShape(flag) { m_type = HK_SHAPE_HEIGHT_FIELD; }
	~hkpHeightFieldShape() override;

	// One output vector per input sphere (the PDB's SphereCollisionOutput).
	virtual void collideSpheres(const CollideSpheresInput& input, hkVector4* apOutputArray) const = 0;
	virtual void castSphere(const hkpSphereCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const = 0;
};

// The class's reflection record (hkpHeightFieldShape.cpp).
extern hkClass hkpHeightFieldShapeClass;
