#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Physics/Collide/BroadPhase/hkpBroadPhaseHandlePair.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpNullBroadPhaseListener.h"
#include "Havok/Source/Physics/Collide/Dispatch/BroadPhase/hkpTypedBroadPhaseHandlePair.h"
#include "Reconstruction.hpp"

class hkpCollidableCollidableFilter;

// The PDB's file-local keyFromPair of hkpTypedBroadPhaseDispatcher.cpp: the pair as a 64-bit key (the pair in ecx,
// the key in edx:eax, ret 0, 0xCF6F80).
int64_t keyFromPair(const hkpBroadPhaseHandlePair& pair);

// Dispatches the broad phase's new and removed pairs to the listener registered for the pair's two handle types.
// hkpWorld::hkpWorld allocates 268 bytes for it (IDA's 0x108 takes the null listener as 8 bytes).
class hkpTypedBroadPhaseDispatcher {
public:
	hkpTypedBroadPhaseDispatcher();

	void addPairs(hkpTypedBroadPhaseHandlePair* newPairs, int32_t numNewPairs,
		const hkpCollidableCollidableFilter* filter) const;
	void removePairs(hkpTypedBroadPhaseHandlePair* deletedPairs, int32_t numDeletedPairs) const;

	inline hkpBroadPhaseListener* getBroadPhaseListener(int32_t typeA, int32_t typeB) {
		return m_broadPhaseListeners[typeA][typeB];
	}

	inline void setBroadPhaseListener(hkpBroadPhaseListener* listener, int32_t a, int32_t b) {
		m_broadPhaseListeners[a][b] = listener;
	}

	inline hkpBroadPhaseListener* getNullBroadPhaseListener() { return &m_nullBroadPhaseListener; }

	// The implicit destructor destroys m_nullBroadPhaseListener.
	// Havok SDK.
	static void removeDuplicates(hkArray<hkpBroadPhaseHandlePair>& newPairs,
		hkArray<hkpBroadPhaseHandlePair>& delPairs);

	hkpBroadPhaseListener* m_broadPhaseListeners[8][8];
	hkpNullBroadPhaseListener m_nullBroadPhaseListener;
};
