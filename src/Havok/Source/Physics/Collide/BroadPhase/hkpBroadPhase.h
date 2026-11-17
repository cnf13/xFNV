#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

#include <stdint.h>

class hkAabb;
struct hkAabbUint32;
class hkpBroadPhaseCastCollector;
class hkpBroadPhaseHandle;
class hkpCollidable;

// The cache calcAabbCache fills: a byte buffer of getAabbCacheSize() bytes.
typedef char hkpBroadPhaseAabbCache;

// The broad phase: finds the pairs of objects whose AABBs overlap. Every query is pure virtual, except
// checkDeterminism.
class hkpBroadPhase : public hkReferencedObject {
public:
	enum BroadPhaseType {
		BROADPHASE_16BIT = 0,
		BROADPHASE_32BIT = 1,
	};

	struct hkpCastRayInput;
	struct hkpCastAabbInput;

	using createBroadPhaseFunc = hkpBroadPhase* (*)(const hkVector4& worldMin, const hkVector4& worldMax,
		int32_t numMarkers);

	hkpBroadPhase();
	~hkpBroadPhase() override;

	virtual hkpBroadPhase::BroadPhaseType getType() const = 0;
	virtual void addObject(hkpBroadPhaseHandle* object, const hkAabbUint32& aabb,
		hkArray<hkpBroadPhaseHandlePair>& pairsOut) = 0;
	virtual void addObject(hkpBroadPhaseHandle* object, const hkAabb& aabb,
		hkArray<hkpBroadPhaseHandlePair>& pairsOut) = 0;
	virtual void addObjectBatch(const hkArrayBase<hkpBroadPhaseHandle*>& addObjectList,
		const hkArrayBase<hkAabb>& addAabbList, hkArray<hkpBroadPhaseHandlePair>& newPairs) = 0;
	virtual void removeObject(hkpBroadPhaseHandle* object, hkArray<hkpBroadPhaseHandlePair>& pairsOut) = 0;
	virtual void removeObjectBatch(const hkArrayBase<hkpBroadPhaseHandle*>& removeObjectList,
		hkArray<hkpBroadPhaseHandlePair>& delPairsOut) = 0;
	virtual int32_t getNumObjects() = 0;
	virtual void updateAabbs(hkpBroadPhaseHandle* objects[], const hkAabb* aabbs, int32_t numObjects,
		hkArray<hkpBroadPhaseHandlePair>& addedPairs, hkArray<hkpBroadPhaseHandlePair>& removedPairs) = 0;
	virtual void updateAabbsUint32(hkpBroadPhaseHandle* objects[], const hkAabbUint32* aabbs, int32_t numObjects,
		hkArray<hkpBroadPhaseHandlePair>& addedPairs, hkArray<hkpBroadPhaseHandlePair>& removedPairs) = 0;
	virtual void defragment() = 0;
	// Empty here; the 3-axis sweep overrides it.
	virtual void checkDeterminism() {}
	virtual void getAllAabbs(hkArray<hkAabb>& allAabbs) = 0;
	virtual void getAabb(const hkpBroadPhaseHandle* object, hkAabb& aabb) const = 0;
	virtual void getExtents(hkVector4& worldMinOut, hkVector4& worldMaxOut) const = 0;
	virtual void querySingleAabb(const hkAabb& aabb, hkArray<hkpBroadPhaseHandlePair>& pairsOut) const = 0;
	virtual void reQuerySingleObject(const hkpBroadPhaseHandle* object,
		hkArray<hkpBroadPhaseHandlePair>& pairsOut) const = 0;
	// Returns bool rather than hkBool.
	virtual bool areAabbsOverlapping(const hkpBroadPhaseHandle* bhA, const hkpBroadPhaseHandle* bhB) const = 0;
	virtual void shiftAllObjects(const hkVector4& shiftDistance, hkVector4& effectiveShiftDistanceOut,
		hkArray<hkpBroadPhaseHandlePair>& newCollisionPairs) = 0;
	virtual void shiftBroadPhase(const hkVector4& shiftDistance, hkVector4& effectiveShiftDistanceOut,
		hkArray<hkpBroadPhaseHandlePair>& newCollisionPairs) = 0;
	virtual void getOffsetLowHigh32bit(hkVector4& offsetLow32bit, hkVector4& offsetHigh32bit) const = 0;
	virtual void castRay(const hkpBroadPhase::hkpCastRayInput& input, hkpBroadPhaseCastCollector* collectorBase,
		int32_t collectorStriding) const = 0;
	virtual int32_t getAabbCacheSize() const = 0;
	virtual void calcAabbCache(const hkAabb& aabb, hkpBroadPhaseAabbCache* AabbCacheOut) const = 0;
	virtual void calcAabbCache(const hkArrayBase<hkpCollidable*>& overlappingCollidables,
		hkpBroadPhaseAabbCache* AabbCacheOut) const = 0;
	virtual void castAabb(const hkpBroadPhase::hkpCastAabbInput& input,
		hkpBroadPhaseCastCollector& collector) const = 0;
	virtual void set32BitOffsetAndScale(const hkVector4& offsetLow, const hkVector4& offsetHigh,
		const hkVector4& scale) = 0;

	// Enter and leave the broad phase's critical section.
	void lockImplementation();
	void unlockImplementation();
	// Havok SDK signature (hkpCollide.lib, tied by bytes,reloc).
	void enableMultiThreading(int32_t spinCountForCriticalSection);

	hkMultiThreadCheck m_multiThreadCheck;
	hkCriticalSection* m_criticalSection;

	// The function hkpWorld builds its broad phase with.
	static createBroadPhaseFunc s_createSweepAndPruneBroadPhaseFunction;
};

// One AABB sweep query: the swept box, and the cache info calcAabbCache returned for it (null when the broad phase
// keeps no cache). It is 16-aligned, as hkVector4 makes it.
struct hkpBroadPhase::hkpCastAabbInput {
	hkVector4 m_from;
	hkVector4 m_to;
	hkVector4 m_halfExtents;
	const hkpBroadPhaseAabbCache* m_aabbCacheInfo;
};

// One ray cast query, or several from one start point: m_numCasts end points m_toStriding bytes apart from m_toBase,
// and the cache info calcAabbCache returned (null when the broad phase keeps no cache).
struct hkpBroadPhase::hkpCastRayInput {
	hkVector4 m_from;
	int32_t m_numCasts;
	const hkVector4* m_toBase;
	int32_t m_toStriding;
	const hkpBroadPhaseAabbCache* m_aabbCacheInfo;
};
