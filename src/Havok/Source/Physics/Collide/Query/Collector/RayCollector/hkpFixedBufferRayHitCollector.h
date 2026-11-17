#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Collide/Shape/Query/hkpRayHitCollector.h"
#include "Reconstruction.hpp"

class hkpCdBody;
class hkpCollidable;
class hkpFixedBufferRayHitCollector;
struct hkpShapeRayCastCollectorOutput;
struct hkpWorldRayCastOutput;

// The fixed buffer collector's addRayHit callback (cdecl: 0xE2B9B0 pops nothing; it reads the collector's
// capacity and count at +0x10 and +0x14 of its third argument).
typedef void (*hkFixedBufferRayHitCollectorAddRayHitCallbackFunc)(const hkpCdBody& cdBody,
	const hkpShapeRayCastCollectorOutput& hitInfo, hkpFixedBufferRayHitCollector* collector);

// Collects world ray hits into caller-owned storage with a fixed capacity.
class hkpFixedBufferRayHitCollector : public hkpRayHitCollector {
public:
	// The hybrid's implicit one: the sites that inline the constructor (FNVD_INLINED_LOCAL) store the fields.
	hkpFixedBufferRayHitCollector() = default;

	hkpFixedBufferRayHitCollector(hkpWorldRayCastOutput* buffer, int32_t capacity) {
		m_rayCastOutputBase.m_storage = buffer;
		m_nextFreeOutput.m_storage = buffer;
		m_capacity.m_storage = capacity;
		m_numOutputs.m_storage = 0;
		m_collidableOnPpu.m_storage = nullptr;
	}

	// Havok SDK signature (hkpCollide.lib, tied by vtable).
	void addRayHit(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo) override;
	~hkpFixedBufferRayHitCollector() override;

	// DeletingDestruct's free (the thread's Havok heap, blockFree with the object's size).
	static void operator delete(void* apMemory);
	// Adds a hit to the buffer, replacing its farthest hit once it is full.
	static void addRayHitImplementation(const hkpCdBody& cdBody, const hkpShapeRayCastCollectorOutput& hitInfo,
		hkpFixedBufferRayHitCollector* collector);

	hkPadSpu<hkpWorldRayCastOutput*> m_rayCastOutputBase;
	hkPadSpu<hkpWorldRayCastOutput*> m_nextFreeOutput;
	hkPadSpu<int32_t> m_capacity;
	hkPadSpu<int32_t> m_numOutputs;
	hkPadSpu<hkpCollidable const*> m_collidableOnPpu;
};
inline constexpr fnvd::Vtable hkpFixedBufferRayHitCollector_vftable{};

// The addRayHit callback the collector calls; the ray cast jobs store addRayHitImplementation in it.
inline hkFixedBufferRayHitCollectorAddRayHitCallbackFunc g_FixedBufferRayHitCollectorAddRayHitCallbackFunc{};
