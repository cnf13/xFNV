#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class hkAabb;
class hkKdTree;
class hkKdTreeCastCollector;
struct hkKdTreeNode;
struct hkKdTreeProjectedEntry;

// The kd-tree's AABB queries (statics only). No data.
class hkKdTreeAabbQueryUtils {
public:
	// The depth of the stack an iterative query reserves.
	enum : int32_t {
		DEFAULT_STACK_SIZE = 128,
	};

	// A hit is an hkPrimitiveId (hkUlong): the address of the primitive's collidable, rebuilt from the projected
	// entry's two id words.
	static void queryAabbIterative(const hkKdTree* tree, const hkAabb& aabbIn,
		hkArray<hkUlong, hkContainerTempAllocator>& hits);
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	static uint32_t queryAabbIterative(const hkKdTree* root, const hkAabb& aabb, hkUlong** hitsOut, int32_t maxHits);
	// The collector overload, which hands collector to queryAabbInternal.
	static void queryAabbIterative(const hkKdTree* root, const hkAabb& aabb, hkKdTreeCastCollector* collector);
	// Appends to hits the id of each primitive of tree whose box overlaps aabb, walking the tree recursively.
	static void queryAabbRecursive(const hkKdTree* tree, const hkAabb& aabb,
		hkArray<hkUlong, hkContainerTempAllocator>& hits);

protected:
	// Private recursive walk below node: appends to hits the id of each primitive whose box overlaps aabb. root is only
	// passed on.
	static void _queryAabbRecursive(const hkKdTreeNode* root, const hkKdTreeNode* ppuNode,
		const hkKdTreeProjectedEntry* projectedEntries, const hkVector4& scale, const hkVector4& offset,
		const hkAabb& aabb, hkArray<hkUlong, hkContainerTempAllocator>& hits);
	// Walks root's tree and passes each primitive whose box overlaps aabb to collector, else to hitArray, else to
	// hitsOut, at most maxHits of them; returns how many went to hitsOut.
	static uint32_t queryAabbInternal(const hkKdTreeNode* root, const hkKdTreeProjectedEntry* projectedEntries,
		const hkVector4& scale, const hkVector4& offset, const hkAabb& aabbIn, int32_t stackSize,
		hkArray<hkUlong, hkContainerTempAllocator>* hitArray, hkKdTreeCastCollector* collector, hkUlong** hitsOut,
		int32_t maxHits);
};
