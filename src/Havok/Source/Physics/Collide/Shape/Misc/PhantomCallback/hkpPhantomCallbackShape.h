#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Reconstruction.hpp"

class hkpCollidable;
class hkpCollisionInput;

// A shape with no geometry that tells its owner when other collidables enter or leave it (a trigger volume); the
// derived shapes implement the two events. It adds no data to hkpShape. The PDB leaves the events' parameters unnamed
// in the base; the types are those of the derived shapes' PDB names (collidableA, collidableB, the collision input).
// phantomLeaveEvent takes no collision input: the exe's overrides (0xC9E670, 0xCA0BD0) end in `ret 8`, as the SDK
// and the PC PDB declare it.
class hkpPhantomCallbackShape : public hkpShape {
public:
	hkpPhantomCallbackShape();
	~hkpPhantomCallbackShape() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	void getAabbImpl(const hkTransform& localToWorld, float tolerance, hkAabb& out) const override;
	hkBool castRayImpl(const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results) const override;
	void castRayWithCollectorImpl(const hkpShapeRayCastInput& input, const hkpCdBody& cdBody,
		hkpRayHitCollector& collector) const override;
	virtual void phantomEnterEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB,
		const hkpCollisionInput& env) = 0;
	virtual void phantomLeaveEvent(const hkpCollidable* collidableA, const hkpCollidable* collidableB) = 0;
};
inline constexpr fnvd::Vtable hkpPhantomCallbackShape_vftable{};

// The class's reflection record.
extern hkClass hkpPhantomCallbackShapeClass;
