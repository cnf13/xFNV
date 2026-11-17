#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/hkpHeightFieldShape.h"
#include "Reconstruction.hpp"

#include <xmmintrin.h>

class hkAabb;

// An infinite plane as a height field: the plane equation and the box that bounds the part of it the shape reports.
// The exe's own vtable slots are the destructor, calcContentStatistics and the shape queries below; the rest are the
// bases'.
class hkpPlaneShape : public hkpHeightFieldShape {
public:
	hkpPlaneShape(const hkVector4& plane, const hkAabb& aabb);
	// Assumes hkpHeightFieldShape's finish constructor (the SDK's).
	hkpPlaneShape(hkFinishLoadedObjectFlag aFlag) : hkpHeightFieldShape(aFlag) { m_type.m_storage = HK_SHAPE_PLANE; }
	hkpPlaneShape(const hkVector4& direction, const hkVector4& center, const hkVector4& halfExtents);
	~hkpPlaneShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& inputLocal, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	void collideSpheres(const CollideSpheresInput& input, hkVector4* outputArray) const override;
	void castSphere(const hkpSphereCastInput& inputLocal, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;

	// Havok SDK signature (hkpCollide.lib, tied by target).
	static const hkClass& staticClass();

	hkVector4 m_plane;
	hkVector4 m_aabbCenter;
	hkVector4 m_aabbHalfExtents;
};
inline constexpr fnvd::Vtable hkpPlaneShape_vftable{};
