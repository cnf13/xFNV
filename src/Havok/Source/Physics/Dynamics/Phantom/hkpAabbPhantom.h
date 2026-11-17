#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpPhantom.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkpCdPointCollector;
class hkpLinearCastInput;
class hkpRayHitCollector;
struct hkpWorldRayCastInput;

// A phantom that is just a box: it tells its listeners which bodies overlap its aabb.
class hkpAabbPhantom : public hkpPhantom {
public:
	// ensureDeterministicOrder's sort order: the collidables by broad phase id. The compare is unsigned.
	class OrderByUid {
	public:
		hkBool operator()(const hkpCollidable* a, const hkpCollidable* b) {
			return a->m_broadPhaseHandle.m_id < b->m_broadPhaseHandle.m_id;
		}
	};

	hkpAabbPhantom(const hkAabb& a1, uint32_t a2);
	hkpAabbPhantom(hkFinishLoadedObjectFlag aFlag);
	~hkpAabbPhantom() override;

	void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
	hkMotionState* getMotionState() override;
	hkpPhantomType getType() const override;
	void calcAabb(hkAabb& aabb) override;
	void addOverlappingCollidable(hkpCollidable* collidable) override;
	hkBool* isOverlappingCollidableAdded(hkBool* arResult, const hkpCollidable* apCollidable) override;
	void removeOverlappingCollidable(hkpCollidable* collidable) override;
	void ensureDeterministicOrder() override;
	hkpPhantom* clone() const override;
	void deallocateInternalArrays() override;

	// Havok SDK signature (hkpDynamics.lib, tied by reloc).
	void setAabb(const hkAabb& newAabb);
	void castRay(const hkpWorldRayCastInput& input, hkpRayHitCollector& collector) const;
	// Havok SDK signature (hkpDynamics.lib, tied by string).
	void linearCast(const hkpCollidable* const toBeCast, const hkpLinearCastInput& input,
		hkpCdPointCollector& castCollector, hkpCdPointCollector* startCollector) const;
	void castRay(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput& output) const;

	static const hkClass& staticClass();

	hkAabb m_aabb;
	// The collidables whose AABB overlaps this phantom's; m_orderDirty says the array must be sorted again.
	hkArray<hkpCollidable*> m_overlappingCollidables;
	hkBool m_orderDirty;
	uint8_t paddingDD[3];
};

extern hkClass hkpAabbPhantomClass;
extern hkTypeInfo hkpAabbPhantomTypeInfo;
extern hkClass hkpPhantomClass;
