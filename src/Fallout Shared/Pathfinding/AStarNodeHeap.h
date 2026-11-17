#pragma once

#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Reconstruction.hpp"

#include <new>

struct NavMeshSearchLink;

// The scrap heap an A* search allocates its nodes from. Only the members src uses are declared.
template <class NodeData, class Link, uint32_t SIZE> class AStarNodeHeap {
public:
	// Constructs Heap with room for SIZE nodes, as each instantiation's exe constructor does.
	AStarNodeHeap() : Heap(SIZE * sizeof(AStarNode<NodeData, Link>)) {}

	// A new node from Heap, constructed; null when the allocation fails. The hybrid defines it per instantiation.
	AStarNode<NodeData, Link>* AllocateNode();

	ScrapHeap Heap;
};
// operator new(sizeof(node), Heap), then the node's constructor when that allocated.
template <class NodeData, class Link, uint32_t SIZE>
AStarNode<NodeData, Link>* AStarNodeHeap<NodeData, Link, SIZE>::AllocateNode() {
	void* pMemory = operator new(sizeof(AStarNode<NodeData, Link>), Heap);
	if (pMemory) {
		return ::new (pMemory) AStarNode<NodeData, Link>;
	}
	return nullptr;
}
