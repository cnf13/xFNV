#pragma once

#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeBundleCastCollector.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpShapeRayBundleCastInput.h"
#include "Reconstruction.hpp"

struct hkKdTreeRayCastBundleCommand;
class hkpCollisionFilter;
struct hkpWorldRayCastInput;

// The collector of the CPU k-d tree ray bundle job: for each collidable the four rays reach it asks the filter,
// casts the bundle (moved into the collidable's space in m_workInput) against its shape and keeps each ray's closest
// hit in the command's results. Layout from the PC PDB, checked against addMidPhaseHit (0xE4DE10).
class hkKdTreeCpuBroadphaseBundleCastCollector : public hkKdTreeBundleCastCollector {
public:
	hkKdTreeCpuBroadphaseBundleCastCollector();
	~hkKdTreeCpuBroadphaseBundleCastCollector() override;

	// Slot 1.
	void addMidPhaseHit(hkPrimitiveId primId, const hkVector4Comparison& mask) override;
	// Slot 2.
	void getHitFractions(hkVector4& hitFractions) const override;

	// DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size).
	static void operator delete(void* apMemory);

	const hkpCollisionFilter* m_filter;
	// The four rays, one hkpWorldRayCastInput each.
	const hkpWorldRayCastInput* m_originalInput;
	hkKdTreeRayCastBundleCommand* m_command;
	hkpShapeRayBundleCastInput m_workInput;
	// Per ray, whether it has a hit yet.
	int32_t m_numHits[4];
};
inline constexpr fnvd::Vtable hkKdTreeCpuBroadphaseBundleCastCollector_vftable{};
