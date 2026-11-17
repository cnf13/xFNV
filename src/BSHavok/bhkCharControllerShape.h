#pragma once

#include "BSHavok/bhkConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"

// The Havok shape of a character controller: an hkpConvexVerticesShape with no members of its own (Init allocates
// sizeof(hkpConvexVerticesShape) and stamps this vtable after the base constructor).
class hkCharControllerShape : public hkpConvexVerticesShape {
public:
	hkCharControllerShape(const hkStridedVertices& aVerts, const hkArray<hkVector4>& arPlanes, float afRadius);

	// The AABB of the cached centre and half extents at localToWorld's translation, grown by tolerance. Havok's own
	// AABB function is getAabbImpl.
	virtual void getAabb(const hkTransform& arLocalToWorld, float afTolerance, hkAabb& arOut) const;
};

// The convex vertices shape of a character controller. Its members are not declared yet.
class bhkCharControllerShape : public bhkConvexVerticesShape {
public:
	bhkCharControllerShape(hkConvexVerticesShapeCinfo& arInfo);

	const NiRTTI* GetRTTI() const override;
	void Init(const hkSerializableCinfo& arInfo) override;

	// Builds a shape from a hull's vertices: their plane equations and rounded vertices become the creation info.
	// Returns no shape for empty vertices.
	static bhkCharControllerShape* CreateFromHull(hkArray<hkVector4, hkContainerHeapAllocator>& arVerts);

	static NiRTTI ms_RTTI;
};
