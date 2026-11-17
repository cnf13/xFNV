#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkMap.h"
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "Reconstruction.hpp"

class hkpEntity;

// Stores a1 at r_eax + 4 and returns r_eax (no callers).
float* sub_DEF530(float a1, float* r_eax);
// Stores a1 and a2 at r_eax and r_eax + 4 and returns r_eax.
float* sub_DEF540(float a1, float a2, float* r_eax);
// Stores a1 to a4 at r_eax + 0 to + 12 and returns r_eax.
float* sub_DEF550(float a1, float a2, float a3, float a4, float* r_eax);

// A collision filter with a set of entity pairs whose collisions are disabled; every other pair goes to the child
// filter (none: enabled). The constructor sets m_type to HK_FILTER_PAIR.
class hkpPairCollisionFilter : public hkpCollisionFilter {
public:
#if FNVD_64BIT
	// The map key for 8-byte pointers (the SDK's hkpPairCollisionFilter.h): both entities' addresses whole, the
	// smaller first, with the operations the map hashes and compares them with.
	struct PairFilterKey {
		uint64_t m_a;
		uint64_t m_b;
	};

	struct PairFilterPointerMapOperations {
		static uint32_t hash(PairFilterKey key, uint32_t modulus) {
			const uint64_t combinedKey = (key.m_a + (key.m_b << 28)) >> 4;
			return static_cast<uint32_t>((combinedKey * 0x9e3779b97f4a7c13ull) & modulus);
		}

		static void invalidate(PairFilterKey& key) {
			key.m_a = 0xffffffffffffffffull;
			key.m_b = 0xffffffffffffffffull;
		}

		static bool isValid(PairFilterKey key) {
			return key.m_a != 0xffffffffffffffffull && key.m_b != 0xffffffffffffffffull;
		}

		static bool equal(PairFilterKey key0, PairFilterKey key1) {
			return key0.m_a == key1.m_a && key0.m_b == key1.m_b;
		}
	};
#else
	// The map key for 4-byte pointers: the two entities' addresses, the larger in the low dword.
	using PairFilterKey = uint64_t;
#endif

	~hkpPairCollisionFilter() override;

	// The five filter interfaces: a disabled pair of entities answers false, everything else goes to the child
	// filter (each slot is in a secondary vtable: hkpCollidableCollidableFilter at 8, hkpShapeCollectionFilter at
	// 0xC, hkpRayShapeCollectionFilter at 0x10, hkpRayCollidableFilter at 0x14).
	hkBool isCollisionEnabled(const hkpCollidable& collidableA, const hkpCollidable& collidableB) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& a, const hkpCdBody& b,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpCollisionInput& input, const hkpCdBody& collectionBodyA,
		const hkpCdBody& collectionBodyB, const hkpShapeContainer& containerShapeA,
		const hkpShapeContainer& containerShapeB, uint32_t keyA, uint32_t keyB) const override;
	hkBool isCollisionEnabled(const hkpShapeRayCastInput& aInput, const hkpShape& bShape,
		const hkpShapeContainer& bContainer, uint32_t bKey) const override;
	hkBool isCollisionEnabled(const hkpWorldRayCastInput& inputA, const hkpCollidable& collidableB) const override;

	// The disabled entity pairs (a key present: the pair does not collide, 0xDEF3D0).
#if FNVD_64BIT
	hkMap<PairFilterKey, uint64_t, PairFilterPointerMapOperations, hkContainerHeapAllocator> m_disabledPairs;
#else
	hkMap<PairFilterKey, uint64_t, hkMapOperations<uint64_t>, hkContainerHeapAllocator> m_disabledPairs;
#endif
	const hkpCollisionFilter* m_childFilter;
	hkpPairCollisionFilter(const hkpCollisionFilter* a1);
	// Non-virtual test: a pair with a null entity collides, any other pair unless its key is in m_disabledPairs.
	hkBool isCollisionEnabled(const hkpEntity* entityA, const hkpEntity* entityB) const;
	// The map key of the pair, the smaller address first. hkUint64(entity) widens a 4-byte pointer sign-extended, so
	// each address is converted through the signed pointer-sized integer first.
	void calcKey(const hkpEntity* entityA, const hkpEntity* entityB, PairFilterKey& keyOut) const {
		const uint64_t ptrA = static_cast<uint64_t>(static_cast<int64_t>(reinterpret_cast<intptr_t>(entityA)));
		const uint64_t ptrB = static_cast<uint64_t>(static_cast<int64_t>(reinterpret_cast<intptr_t>(entityB)));
#if FNVD_64BIT
		if (ptrA < ptrB) {
			keyOut.m_a = ptrA;
			keyOut.m_b = ptrB;
		} else {
			keyOut.m_a = ptrB;
			keyOut.m_b = ptrA;
		}
#else
		if (ptrA < ptrB) {
			keyOut = (ptrA << 32) + ptrB;
		} else {
			keyOut = (ptrB << 32) + ptrA;
		}
#endif
	}
	int32_t enableCollisionsBetween(const hkpEntity* entityA, const hkpEntity* entityB);
	int32_t disableCollisionsBetween(const hkpEntity* entityA, const hkpEntity* entityB);
};
