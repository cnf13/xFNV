#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"

#include <stdint.h>

void cleanupLoadedObjecthkpMultiRayShape(void* p);

// A shape made of rays (start and end points), with the distance a ray may penetrate.
class hkpMultiRayShape : public hkpShape {
public:
	// One ray of the shape.
	struct Ray {
		static const hkClass& staticClass();

		hkVector4 m_start;
		hkVector4 m_end;
	};

	// The reflection's member table.
	using MembersArray = const hkClassMember[2];

	hkpMultiRayShape(const hkpMultiRayShape::Ray* Rays, int32_t nRays, float rayPenetrationDistance);

	hkpMultiRayShape(hkFinishLoadedObjectFlag aFlag) : hkpShape(aFlag), m_rays(aFlag) {
		m_type.m_storage = HK_SHAPE_MULTI_RAY;
	}

	~hkpMultiRayShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;

	static const hkClass& staticClass();

	hkArray<Ray> m_rays;
	float m_rayPenetrationDistance;

	static MembersArray Members;
};

extern hkClass hkpMultiRayShapeRayClass;
extern hkClass hkpMultiRayShapeClass;
extern hkTypeInfo hkpMultiRayShapeTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkpMultiRayShapeRayTypeInfo;
