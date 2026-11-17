#pragma once

#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhase.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayCastInput.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpCollisionFilter;
class hkpRayCollidableFilter;
class hkpRayHitCollector;
struct hkpWorldRayCastInput;

// Casts a world ray through the cached broad phase and hands every shape hit to a ray hit collector (the collector
// form of hkpWorld::castRay). The exe's vtable has 2 slots: its own destructor and addBroadPhaseHandle (0xCBF710); the
// PDB has that slot protected and this class public, as hkpSimpleWorldRayCaster's header does. The PDB's other
// members (castRayGroup, castRaysFromSinglePoint, the two constructors) are left out until their bodies are read.
class hkpWorldRayCaster : public hkpBroadPhaseCastCollector {
public:
	~hkpWorldRayCaster() override;

	float addBroadPhaseHandle(const hkpBroadPhaseHandle* broadPhaseHandle, int32_t castIndex) override;

	// Casts input through broadphase, narrowing each collidable the broad phase reports
	// against filter's ray collidable filter and handing the hits to collector; cache is the broad phase's AABB cache.
	void castRay(const hkpBroadPhase& broadphase, const hkpWorldRayCastInput& input, const hkpCollisionFilter* filter,
		const hkpBroadPhaseAabbCache* cache, hkpRayHitCollector& collector);
	// Havok SDK signature (hkpCollide.lib, tied by string).
	void castRayGroup(const hkpBroadPhase& broadphase, const hkpWorldRayCastInput* inputArray, int32_t numRays,
		const hkpCollisionFilter* filter, hkpRayHitCollector* collectorBase, int32_t collectorStriding);
	void castRaysFromSinglePoint(const hkpBroadPhase& broadphase, const hkpWorldRayCastInput* inputArray,
		int32_t numRays, const hkpCollisionFilter* filter, const char* cache, hkpRayHitCollector* collectorBase,
		int32_t collectorStriding);

	const hkpWorldRayCastInput* m_input;
	const hkpRayCollidableFilter* m_filter;
	hkpRayHitCollector* m_collectorBase;
	int32_t m_collectorStriding;
	// 16-byte aligned by hkVector4, so at 0x20; the exe writes its collection filter word at +0x24 of it.
	hkpShapeRayCastInput m_shapeInput;
};
inline constexpr fnvd::Vtable hkpWorldRayCaster_vftable{};
