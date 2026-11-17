#pragma once

#include "Havok/Source/Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Reconstruction.hpp"

// A ray hit collector that keeps only the nearest hit.
class hkpClosestRayHitCollector : public hkpRayHitCollector {
public:
	hkpClosestRayHitCollector();

	// Keeps the hit when it is nearer than the one held.
	void addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) override;
	~hkpClosestRayHitCollector() override;

	// The SDK's inline reset, which hides the base's: m_rayHit.reset(), then
	// hkpRayHitCollector::reset. The constructor and PathingSearchRayCast's
	// casts call it out of line.
	void reset();

	hkpWorldRayCastOutput m_rayHit;
};
inline constexpr fnvd::Vtable hkpClosestRayHitCollector_vftable{};
