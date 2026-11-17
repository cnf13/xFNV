#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTree.h"
#include "Havok/Source/Common/Internal/Collide/Tree/AabbTree/hkAabbTreeData.h"
#include "Reconstruction.hpp"

#include <stddef.h>
#include <stdint.h>

class hkCpuWorldRayCastCollector;
class hkpCollisionFilter;
class hkpFixedBufferRayHitCollector;
struct hkpWorldRayCastCommand;

// The PDB's setCollector (hkAabbTreeRaycastUtil.cpp): readies collector for command's single ray, with filter as its
// shape collection filter when the ray enables one (cdecl, ret 0).
void setCollector(hkCpuWorldRayCastCollector* collector, hkpWorldRayCastCommand* command,
	const hkpCollisionFilter* filter);
// The PDB's bundleRaysWithInvDir (hkAabbTreeRaycastUtil.cpp): packs four rays, by their starts and inverse
// directions, into a RayBundle with the lanes mask selects. The RayBundle result through the hidden pointer
// apResult, returned in eax (cdecl, ret 0).
hkAabbTreeData::RayBundle* bundleRaysWithInvDir(hkAabbTreeData::RayBundle* apResult, const hkVector4& from0,
	const hkVector4& invDir0, const hkVector4& from1, const hkVector4& invDir1, const hkVector4& from2,
	const hkVector4& invDir2, const hkVector4& from3, const hkVector4& invDir3, hkVector4Comparison::Mask mask);

// The world ray cast job's filtered casts through an hkAabbTree (castRayFiltering, filterRays). Statics only: sizeof 1
// in Havok's release hkpcollide.pdb; the game PDB has both members for <hkAabbTreeData::hkAabbTreeAabb, uint16_t> and
// <hkAabbTreeData::hkAabbTreeAabb16, uint16_t>.
template <typename BOUNDING_VOLUME, typename hkIdxType = uint16_t> class hkAabbTreeRayCastUtil {
public:
	// The PDB's unnamed enum: the room per lane of ray indices, and the most tasks castRayFiltering's stack holds.
	enum {
		ACTIVESTACK_MAX = 250,
		TASK_MAX = 10,
	};

	using Tree = hkAabbTree<BOUNDING_VOLUME, hkIdxType>;

	// The SDK's private member (public here, so that its registration can name it). Tests the rays raysIdx names
	// against node: a leaf goes to every ray's collector that reaches it; an inner node either splits the rays between
	// its two children (into raysIdxOut0/1, the children copied to child0Out/1) or casts them in bundles of four from
	// node down. The SDK's references (numRaysOut0, numRaysOut1, child0Out, child1Out) are the pointers they compile
	// to.
	static void filterRays(const Tree* tree, const typename Tree::Node* nodeSpu, const typename Tree::Node* skipNode,
		const hkAabbTreeData::Ray* rays, hkCpuWorldRayCastCollector* collectors,
		hkpFixedBufferRayHitCollector* fixedBufferCollectors, hkpWorldRayCastCommand* commandArray,
		const hkpCollisionFilter* filter, float* earlyHitFractionArray, int32_t* raysIdx, int32_t numRays,
		int32_t taskCap, int32_t raysIdxOutCap0, int32_t raysIdxOutCap1, int32_t* raysIdxOut0, int32_t* numRaysOut0,
		int32_t* raysIdxOut1, int32_t* numRaysOut1, typename Tree::Node* child0Out, typename Tree::Node* child1Out);
	// Casts numRays rays through tree, filtered by filter, each into its command's collector.
	static void castRayFiltering(const Tree* tree, const hkAabbTreeData::Ray* rays,
		hkpWorldRayCastCommand* commandArray, const hkpCollisionFilter* filter, hkCpuWorldRayCastCollector* collectors,
		hkpFixedBufferRayHitCollector* fixedBufferCollectors, int32_t numRays);
};
// The generic bodies are in hkAabbTreeRaycastUtil.inl, instantiated once in hkAabbTreeRaycastUtil.cpp.
extern template class hkAabbTreeRayCastUtil<hkAabbTreeData::hkAabbTreeAabb, uint16_t>;
extern template class hkAabbTreeRayCastUtil<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>;

// One entry of castRayFiltering's task stack: a node still to visit, the node to skip to past it, and the rays still
// active there (the last m_numActiveRays indices of lane m_activeRayLane). The PDB's constructor (Node's, clearing
// m_root.m_parentIdx) is inline at each use in the exe, so src declares none.
template <typename BOUNDING_VOLUME, typename IDX> struct Task {
	typename hkAabbTree<BOUNDING_VOLUME, IDX>::Node m_root;
	const typename hkAabbTree<BOUNDING_VOLUME, IDX>::Node* m_skipNode;
	int32_t m_activeRayLane;
	int32_t m_numActiveRays;
};

using hkAabbTreeTask16 = Task<hkAabbTreeData::hkAabbTreeAabb16, uint16_t>;

using hkAabbTreeTask = Task<hkAabbTreeData::hkAabbTreeAabb, uint16_t>;
