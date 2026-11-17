#pragma once

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct NavMeshSearchLink;

// The open list of an A* search (PDB: astarqueue.h): the search's nodes in 20 buckets by fitness, with the largest
// fitness a node may have (IDA has one untemplated struct AStarQueue for all instantiations; the layout is the
// same).
template <class NodeData, class Link> class AStarQueue {
public:
	// Sets fMaxFitness to -1 and zeroes the buckets, as the exe's constructor does.
	AStarQueue() : fMaxFitness(-1.0f), pBuckets{} {}

	// Sets fMaxFitness (thiscall, ret 4). The exe calls it out of line: an instantiation src calls gets an explicit
	// specialization forwarding to the exe's (folded) copy.
	void SetMaxFitness(float afMaxFitness) { fMaxFitness = afMaxFitness; }
	// Zeroes the buckets (thiscall, ret 0). The hybrid forwards to the exe's folded copy, which the CombatPlan
	// instance registers (0x6A84E0).
	void Clear() { ourMemset(pBuckets, 0, sizeof(pBuckets)); }

	// The bucket (0..19) for apNode's fitness: 19 at or past fMaxFitness, else the fitness scaled to 19 buckets
	// (thiscall, ret 4). The hybrid forwards to the exe's folded copy, which the CombatPlan instance registers
	// (0x6F4790).
	int32_t GetBucketForNode(AStarNode<NodeData, Link>* apNode) {
		float fFitness = apNode->GetFitness();
		if (!(fMaxFitness <= static_cast<double>(fFitness))) {
			return static_cast<int32_t>(fFitness * 19.0 / fMaxFitness);
		}
		return 19;
	}

	// Inserts apNode into its fitness bucket, before the first node whose fitness is not below its own (thiscall,
	// ret 4). The hybrid defines it per instantiation.
	void AddNode(AStarNode<NodeData, Link>* apNode);
	// Unlinks apNode from its fitness bucket (thiscall, ret 4). The hybrid defines it per instantiation.
	void RemoveNode(AStarNode<NodeData, Link>* apNode);
	// Unlinks and returns the first node of the lowest non-empty bucket, or null (thiscall, ret 0). The hybrid
	// defines it per instantiation.
	AStarNode<NodeData, Link>* PopFront();

	// IDA name; not in the PDB (PDB: fMaxFitness).
	float fMaxFitness;
	// IDA name; not in the PDB (PDB: Buckets).
	AStarNode<NodeData, Link>* pBuckets[20];
};
template <class NodeData, class Link> void AStarQueue<NodeData, Link>::AddNode(AStarNode<NodeData, Link>* apNode) {
	AStarNode<NodeData, Link>** ppCurrent;
	for (ppCurrent = &pBuckets[GetBucketForNode(apNode)]; *ppCurrent; ppCurrent = &(*ppCurrent)->pNextNode) {
		if (!(apNode->GetFitness() > (*ppCurrent)->GetFitness())) {
			break;
		}
	}
	if (*ppCurrent) {
		(*ppCurrent)->ppPrevNode = &apNode->pNextNode;
	}
	apNode->pNextNode = *ppCurrent;
	apNode->ppPrevNode = ppCurrent;
	*ppCurrent = apNode;
}

template <class NodeData, class Link> void AStarQueue<NodeData, Link>::RemoveNode(AStarNode<NodeData, Link>* apNode) {
	if (apNode->ppPrevNode) {
		*apNode->ppPrevNode = apNode->pNextNode;
	}
	if (apNode->pNextNode) {
		apNode->pNextNode->ppPrevNode = apNode->ppPrevNode;
	}
	apNode->ppPrevNode = nullptr;
	apNode->pNextNode = nullptr;
}

template <class NodeData, class Link> AStarNode<NodeData, Link>* AStarQueue<NodeData, Link>::PopFront() {
	for (uint32_t i = 0; i < 20; ++i) {
		AStarNode<NodeData, Link>** ppBucket = &pBuckets[i];
		if (*ppBucket) {
			AStarNode<NodeData, Link>* pNode = *ppBucket;
			if (pNode->pNextNode) {
				pNode->pNextNode->ppPrevNode = ppBucket;
				*ppBucket = pNode->pNextNode;
			} else {
				*ppBucket = nullptr;
			}
			pNode->ppPrevNode = nullptr;
			pNode->pNextNode = nullptr;
			return pNode;
		}
	}
	return nullptr;
}
