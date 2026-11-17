#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkKdTree;
class hkKdTreeBundleCastCollector;
class hkKdTreeCastCollector;
struct hkKdTreeNode;

// hkKdTreeMath.inl's file-static hkGetOneOver65K (the PDB lists it in this file and two others), the out-of-line copy
// defined in hkKdTreeAabbQueryUtils.cpp: returns 1 / 65535 in every lane in xmm0 (`movaps xmm0, [0x11B3810]; ret`;
// cdecl, ret 0, 0xD1B280). The address names it apart from the copies at 0xDF2740 and 0xD98620.
hkQuadReal hkGetOneOver65K_D1B280();
// Not named in the PDB or IDA; name chosen here. isSameSignDiff (0xD98630) on apA and apB under aiMask, its hkBool
// returned through apResult (cdecl, ret 0, 0xD99540; the database has no caller).
hkBool* isSameSignDiffWrapper(hkBool* apResult, const hkVector4* apA, const hkVector4* apB, int32_t aiMask);
// Not named in the PDB or IDA; name chosen here. Whether, under the lane mask x3, each of the
// three vectors x2[i] - x1[i] has all its masked lanes of one sign (all below, all equal to or all above 0), into
// the hkBool x0 (cdecl, ret 0, 0xD98630; the exe returns x0).
hkBool* isSameSignDiff(hkBool* x0, const hkVector4* x1, const hkVector4* x2, int32_t x3);

// Ray queries over an hkKdTree.
class hkKdTreeUtils {
public:
	// The node stack's depth where the tree has none (m_maxDepth 0 or less).
	enum {
		DEFAULT_STACK_SIZE = 128
	};

	// A bundle of four rays: its three vectors hold the four rays' x, y and z, one ray per lane (the SDK's
	// hkKdTreeUtils::RayBundle, sizeof 48 in hkinternal.pdb).
	struct RayBundle {
		hkVector4 m_vec[3];
	};

	// An entry of the iterative ray cast's node stack: the node and the ray's parameter range through it. The SDK's
	// HK_ALIGN16 on node pads it to 16 bytes (hkinternal.pdb: sizeof 16, members at 0, 4, 8, 0xC).
	struct NodeState {
		alignas(16) const hkKdTreeNode* node;
		float tNear;
		float tFar;
		uint32_t m_padding;
	};

	// An entry of the iterative ray bundle cast's node stack: the node, each ray's parameter range through it and
	// which rays are still active (hkinternal.pdb: members at 0, 0x10, 0x20, 0x30; sizeof 64, the exe's 16-byte
	// hkVector4Comparison filling the last vector).
	struct BundleState {
		hkPadSpu<const hkKdTreeNode*> node;
		hkVector4 tNear;
		hkVector4 tFar;
		hkVector4Comparison activeMask;
	};

	// Casts the ray from from to to through tree, reporting each leaf it reaches to collector; the exe returns the
	// hkBool through a hidden pointer (cdecl, 0xD98CA0). The PDB's parameter names for it are garbled (A, orig,
	// projectedEntries, offset); these are the SDK's.
	static hkBool* castRayIterative(hkBool* arResult, const hkKdTree* tree, const hkVector4& from, const hkVector4& to,
		hkKdTreeCastCollector* collector);

	// Casts the four rays of a bundle, each from its lane of fromBundle to its lane of toBundle, front to back through
	// tree, the rays rayMask selects, reporting each projected entry whose box an active ray reaches before its hit
	// fraction to collector. The SDK's hkBool return comes back through the hidden result pointer arResult;
	// always true.
	static hkBool* castRayBundleIterative(hkBool* arResult, const hkKdTree* tree, const RayBundle& fromBundle,
		const RayBundle& toBundle, hkKdTreeBundleCastCollector* collector, hkVector4Comparison::Mask rayMask);
	// Havok SDK signature (hkInternal.lib, tied by bytes).
	// Havok SDK: result through the hidden result pointer apResult.
	static hkBool* castRayMultiTree(hkBool* apResult, int32_t numTrees, hkKdTree* const* trees, const hkVector4& start,
		const hkVector4& end, hkKdTreeCastCollector* collector);
};
