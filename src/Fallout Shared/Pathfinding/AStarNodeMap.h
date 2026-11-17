#pragma once

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"

class CombatPlanLink;
class CombatPlanNode;

// The hash table of an A* search's nodes of type T (an AStarNode instantiation): 2039 buckets, each chaining its nodes
// through pNextNodeInBucket, and the number used. The explicit specializations for the CombatPlan instance below hold
// the member bodies (AStarNodeMap.cpp).
template <class T> class AStarMap {
public:
	AStarMap() : Buckets{}, iCount(0) {}

	// The first node of the first used bucket, or null.
	void* GetFirstPos() {
		for (uint32_t i = 0; i < 2039; ++i) {
			if (Buckets[i]) {
				return Buckets[i];
			}
		}
		return nullptr;
	}

	// Reads the node at rpPos into ruiKey and rpNode and moves rpPos to the next node (in its bucket, else the first
	// of the next used bucket, else null); false when rpPos was null or the walk ended.
	bool GetNext(void*& aIterator, uintptr_t& aiKey, T*& aNode) {
		T* pNode = static_cast<T*>(aIterator);
		if (!aIterator) {
			return false;
		}
		aiKey = pNode->iKey;
		aNode = pNode;
		if (pNode->pNextNodeInBucket) {
			aIterator = pNode->pNextNodeInBucket;
			return true;
		}
		for (uint32_t i = GetBucketFromKey(pNode->iKey) + 1; i < 2039; ++i) {
			if (Buckets[i]) {
				aIterator = Buckets[i];
				return true;
			}
		}
		aIterator = nullptr;
		return false;
	}

	// Zeroes the buckets when any is used.
	void Clear() {
		if (iCount) {
			ourMemset(Buckets, 0, sizeof(Buckets));
			iCount = 0;
		}
	}

	// The bucket of key aiKey: the key modulo the 2039 buckets (unsigned remainder).
	uint32_t GetBucketFromKey(uintptr_t aiKey) { return static_cast<uint32_t>(aiKey % 2039); }

	// The buckets' first nodes.
	T* Buckets[2039];
	uint32_t iCount;
};
