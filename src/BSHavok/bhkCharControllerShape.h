#pragma once

#include "BSHavok/bhkConvexVerticesShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h"
#include "Reconstruction.hpp"

// The Havok shape of a character controller: an hkpConvexVerticesShape with no members of its own (Init allocates
// sizeof(hkpConvexVerticesShape) and stamps this vtable after the base constructor). The exe's vtable 0x10CB45C
// keeps the base's 18 entries (destructor 0xD5A4E0 included) and adds slot 18.
class hkCharControllerShape : public hkpConvexVerticesShape {
public:
	hkCharControllerShape(const hkStridedVertices& aVerts, const hkArray<hkVector4>& arPlanes, float afRadius);

	// slot 18: the AABB of the cached centre and half extents at localToWorld's translation, grown by tolerance.
	// Havok's own AABB slot is getAabbImpl.
	virtual void getAabb(const hkTransform& arLocalToWorld, float afTolerance, hkAabb& arOut) const;
};

// The convex vertices shape of a character controller. Its members are not declared yet.
class bhkCharControllerShape : public bhkConvexVerticesShape {
public:
	bhkCharControllerShape(hkConvexVerticesShapeCinfo& arInfo);

	const NiRTTI* GetRTTI() const override;
	void Init(const hkSerializableCinfo& arInfo) override;

	// Builds a shape from a hull's vertices: their plane equations and rounded vertices become the creation info. A
	// static in the exe, which returns no shape for empty vertices (cdecl, ret 0, 0xCD7F50).
	static bhkCharControllerShape* CreateFromHull(hkArray<hkVector4, hkContainerHeapAllocator>& arVerts);

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable hkCharControllerShape_vftable{};
inline constexpr fnvd::Vtable bhkCharControllerShape_vftable{};
