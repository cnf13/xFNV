#pragma once

#include "Havok/Source/Physics/Dynamics/Collide/Filter/Pair/hkpPairCollisionFilter.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintListener.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpConstraintInstance;
class hkpWorld;

// A pair filter that disables collisions between the two entities of every constraint in the world: it listens to
// constraints being added, removed, broken and repaired. The constructor sets m_type to HK_FILTER_CONSTRAINT.
// It adds no data; the listener subobject is at 0x40.
class hkpConstraintCollisionFilter : public hkpPairCollisionFilter, public hkpConstraintListener {
public:
	hkpConstraintCollisionFilter(const hkpCollisionFilter* childFilter);
	~hkpConstraintCollisionFilter() override;

	void constraintAddedCallback(hkpConstraintInstance* constraint) override;
	void constraintRemovedCallback(hkpConstraintInstance* constraint) override;
	void constraintBreakingCallback(const hkpConstraintBrokenEvent& event) override;
	void constraintRepairedCallback(const hkpConstraintRepairedEvent& event) override;

	// Havok SDK signature (hkpDynamics.lib, tied by bytes,reloc).
	void _constraintBreakingCallbackImmediate(hkpConstraintInstance* instance, hkBool constraintBroken);
	void updateFromWorld(hkpWorld* world);

	// hkReferencedObject's, which hkpConstraintListener's own would make ambiguous.
	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
};
inline constexpr fnvd::Vtable hkpConstraintCollisionFilter_vftable{};
inline constexpr fnvd::Vtable hkpConstraintCollisionFilter_vftable_hkpCollidableCollidableFilter{};
inline constexpr fnvd::Vtable hkpConstraintCollisionFilter_vftable_hkpShapeCollectionFilter{};
inline constexpr fnvd::Vtable hkpConstraintCollisionFilter_vftable_hkpRayShapeCollectionFilter{};
inline constexpr fnvd::Vtable hkpConstraintCollisionFilter_vftable_hkpRayCollidableFilter{};
inline constexpr fnvd::Vtable hkpConstraintCollisionFilter_vftable_hkpConstraintListener{};
