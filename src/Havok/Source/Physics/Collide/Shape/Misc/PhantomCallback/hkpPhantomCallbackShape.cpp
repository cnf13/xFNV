#include "Havok/Source/Physics/Collide/Shape/Misc/PhantomCallback/hkpPhantomCallbackShape.h"

#include "Havok/Source/Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"

#include <xmmintrin.h>

hkClass hkpPhantomCallbackShapeClass("hkpPhantomCallbackShape", &hkpShapeClass, 16, nullptr, 0, nullptr, 0, nullptr, 0,
	nullptr, nullptr, 0, 0);

hkpPhantomCallbackShape::~hkpPhantomCallbackShape() {}

void hkpPhantomCallbackShape::calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const {
	hkReferencedObject::calcContentStatistics(collector, &hkpShapeClass);
}

void hkpPhantomCallbackShape::getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const {
	out.m_min.m_quad = _mm_setzero_ps();
	out.m_max.m_quad = _mm_setzero_ps();
}

// A phantom callback shape has no geometry, so a ray never hits it.
hkBool hkpPhantomCallbackShape::castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const {
	hkBool result;
	result.m_bool = false;
	return result;
}

void hkpPhantomCallbackShape::castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
	hkpRayHitCollector& collector) const {
	;
}
