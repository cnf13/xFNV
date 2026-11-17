#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkAabb;
class hkKdTree;
class hkKdTreeCastCollector;
struct hkKdTreeNode;
struct hkKdTreeProjectedEntry;

// The kd-tree's AABB queries (statics only; the exe's are in hkKdTreeAabbQueryUtils.cpp). No data: sizeof 1 in
// Havok's release hkinternal.pdb.
class hkKdTreeAabbQueryUtils {
public:
	// The depth of the stack an iterative query reserves.
	enum : int32_t {
		DEFAULT_STACK_SIZE = 128,
	};

	// Havok SDK signature (hkInternal.lib, tied by reloc). A hit is the SDK's hkPrimitiveId (hkUlong): the exe stores
	// the 32-bit address of the primitive's collidable, rebuilt from the projected entry's two id words.
	static void queryAabbIterative(const hkKdTree* tree, const hkAabb& aabbIn,
		hkArray<hkUlong, hkContainerTempAllocator>& hits);
	// Havok SDK signature (hkInternal.lib, tied by reloc).
	static uint32_t queryAabbIterative(const hkKdTree* root, const hkAabb& aabb, hkUlong** hitsOut, int32_t maxHits);
	// Havok SDK signature: the SDK's collector overload (hkKdTreeAabbQueryUtils.h), the exe's 0xD1B9B0
	// (cdecl, ret 0), which hands collector to queryAabbInternal (0xD1B4D0).
	static void queryAabbIterative(const hkKdTree* root, const hkAabb& aabb, hkKdTreeCastCollector* collector);
	// Havok SDK signature (hkInternal.lib's hkKdTreeAabbQueryUtils.obj): appends to hits the id of each primitive of
	// tree whose box overlaps aabb, walking the tree recursively; the exe's 0xD1B7F0 (cdecl, ret 0).
	static void queryAabbRecursive(const hkKdTree* tree, const hkAabb& aabb,
		hkArray<hkUlong, hkContainerTempAllocator>& hits);

protected:
	// The SDK's private recursive walk below node (hkInternal.lib's hkKdTreeAabbQueryUtils.obj; protected here for its
	// registration): appends to hits the id of each primitive whose box overlaps aabb. root is only passed on. The
	// exe's copy is cdecl, ret 0, 0xD1B290.
	static void _queryAabbRecursive(const hkKdTreeNode* root, const hkKdTreeNode* ppuNode,
		const hkKdTreeProjectedEntry* projectedEntries, const hkVector4& scale, const hkVector4& offset,
		const hkAabb& aabb, hkArray<hkUlong, hkContainerTempAllocator>& hits);
	// Walks root's tree and passes each primitive whose box overlaps aabb to collector, else to hitArray, else to
	// hitsOut, at most maxHits of them; returns how many went to hitsOut. The SDK's private member (protected here
	// for its registration); the exe's copy is cdecl, ret 0, 0xD1B4D0.
	static uint32_t queryAabbInternal(const hkKdTreeNode* root, const hkKdTreeProjectedEntry* projectedEntries,
		const hkVector4& scale, const hkVector4& offset, const hkAabb& aabbIn, int32_t stackSize,
		hkArray<hkUlong, hkContainerTempAllocator>* hitArray, hkKdTreeCastCollector* collector, hkUlong** hitsOut,
		int32_t maxHits);
};
