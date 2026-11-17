#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeUtils.h"

#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkIntVector.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"
#include "Havok/Source/Common/Internal/KdTree/hkKdTree.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeBundleCastCollector.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"

#include <bit>
#include <emmintrin.h>
#include <xmmintrin.h>

// This file's copy of hkKdTreeMath.inl's static x of hkGetOneOver65K (its function-local constant, no guard): 1 / 65535
// in every lane. A projected entry's 16-bit bound times the tree's extent times this is its offset from the tree's
// minimum.
static hkQuadReal x{1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f};

// isSameSignDiff's test, its hkBool returned by value through apResult.
hkBool* isSameSignDiffWrapper(hkBool* apResult, const hkVector4* apA, const hkVector4* apB, int32_t aiMask) {
	isSameSignDiff(apResult, apA, apB, aiMask);
	return apResult;
}

// The result is set first, then replaced by whether every difference has its x3 lanes of one sign.
hkBool* isSameSignDiff(hkBool* x0, const hkVector4* x1, const hkVector4* x2, int32_t x3) {
	x0->m_bool = true;
	bool bOneSign = true;
	for (int32_t i = 0; i < 3; ++i) {
		__m128 diff = _mm_sub_ps(x2[i].m_quad, x1[i].m_quad);
		bool bLanes = (x3 & _mm_movemask_ps(_mm_cmplt_ps(diff, _mm_setzero_ps()))) == x3 ||
			(x3 & _mm_movemask_ps(_mm_cmpeq_ps(diff, _mm_setzero_ps()))) == x3 ||
			(x3 & _mm_movemask_ps(_mm_cmplt_ps(_mm_setzero_ps(), diff))) == x3;
		bOneSign = bOneSign && bLanes;
	}
	x0->m_bool = bOneSign;
	return x0;
}

// Walks the ray from from to to down tree front to back with a stack of the far children still to visit (on the
// thread's Havok stack), reporting each projected entry of a leaf whose box the ray reaches before the collector's hit
// fraction; the collector's hasHit is the result. Along an axis the ray does not move, a split is tested against
// from's coordinate instead. The exe keeps tNear, tFar and the 1e16, -FLT_MAX, FLT_MAX constants on the x87 stack;
// tNear is always the float last stored in nodeTNear. Each x87 compare is written so that a NaN takes the exe's
// branch.
hkBool* hkKdTreeUtils::castRayIterative(hkBool* arResult, const hkKdTree* tree, const hkVector4& from,
	const hkVector4& to, hkKdTreeCastCollector* collector) {
	__m128 offset = tree->m_aabb.m_min.m_quad;
	const hkKdTreeProjectedEntry* projectedEntries = tree->m_projectedEntries.m_data;
	const hkKdTreeNode* node = tree->m_nodes.m_data;
	int32_t maxDepth = tree->m_maxDepth;
	__m128 scale = _mm_sub_ps(tree->m_aabb.m_max.m_quad, tree->m_aabb.m_min.m_quad);
	if (maxDepth <= 0) {
		maxDepth = DEFAULT_STACK_SIZE;
	}
	// An inner node's second word holds its two splits, a leaf's its primitive count.
	int32_t type = node->m_data.m_leaf.m_type_primId;
	int32_t secondWord = static_cast<int32_t>(node->m_data.m_leaf.m_numPrimitives);
	// The node stack, from the thread's stack memory, rounded up to 128 bytes (hkStackMemory's inlined allocation).
	hkMemoryRouter* router = &hkMemoryRouter::getInstance();
	float nodeTNear = 0.0f;
	char* stackStorage = router->m_stack.m_next.m_storage;
	router->m_stack.m_next.m_storage += (static_cast<int32_t>(sizeof(NodeState)) * maxDepth + 127) & ~127;
	NodeState* const stackBottom = static_cast<NodeState*>(static_cast<void*>(stackStorage));
	NodeState* stackTop = stackBottom;

	// The ray's length, by the refined reciprocal square root (hkVector4::length3), and its unit direction.
	__m128 dir = _mm_sub_ps(to.m_quad, from.m_quad);
	__m128 dirSq = _mm_mul_ps(dir, dir);
	__m128 lengthSq = _mm_shuffle_ps(dirSq, dirSq, 170);
	lengthSq[0] = lengthSq[0] + static_cast<float>(_mm_shuffle_ps(dirSq, dirSq, 85)[0] + dirSq[0]);
	float rsqrt = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(lengthSq[0])));
	__m128 lengthSqIsZero = _mm_cmpeq_ss(lengthSq, _mm_setzero_ps());
	float newtonStep = 3.0f - static_cast<float>(static_cast<float>(lengthSq[0] * rsqrt) * rsqrt);
	__m128 invLength = _mm_setzero_ps();
	invLength[0] = static_cast<float>(0.5f * rsqrt) * newtonStep;
	invLength = _mm_andnot_ps(lengthSqIsZero, invLength);
	invLength = _mm_shuffle_ps(invLength, invLength, 0);
	float rayLength = static_cast<float>(_mm_shuffle_ps(dirSq, dirSq, 170)[0] +
						  static_cast<float>(_mm_shuffle_ps(dirSq, dirSq, 85)[0] + dirSq[0])) *
		invLength[0];
	float nodeTFar = rayLength;
	__m128 origin = from.m_quad;
	__m128 unitDir = _mm_mul_ps(invLength, dir);
	const __m128 zero = _mm_setzero_ps();
	__m128 dirIsZero = _mm_cmpeq_ps(_mm_setzero_ps(), unitDir);
	// The exe stores 1/65535 (0x37800080) into all four lanes from one x87 load.
	__m128 oneOver65K = _mm_set1_ps(1.5259022e-05f);
	// The reciprocal of the direction (rcpps and one Newton step), 0 in w and along the axes it does not move on.
	__m128 dirOr1 = _mm_or_ps(_mm_andnot_ps(dirIsZero, unitDir), _mm_and_ps(dirIsZero, hkQuadReal1111));
	__m128 dirW1 = _mm_shuffle_ps(dirOr1, _mm_unpackhi_ps(dirOr1, hkQuadReal1111), 196);
	__m128 rcp = _mm_rcp_ps(dirW1);
	__m128 invDir = _mm_or_ps(_mm_andnot_ps(dirIsZero,
								  _mm_and_ps(_mm_sub_ps(_mm_add_ps(rcp, rcp), _mm_mul_ps(_mm_mul_ps(rcp, dirW1), rcp)),
									  rdata_10C4C10)),
		_mm_and_ps(dirIsZero, _mm_setzero_ps()));
	// A split's quantised value times splitScale plus splitBias is the ray parameter where it crosses the split.
	__m128 splitScale = _mm_mul_ps(_mm_mul_ps(invDir, scale), oneOver65K);
	__m128 splitBias = _mm_mul_ps(_mm_sub_ps(offset, origin), invDir);
	float hitFraction = collector->getHitFraction() * rayLength;
	float tMax = rayLength;
	if (hitFraction < rayLength) {
		tMax = hitFraction;
	}
	float tNear;
	float tFar;
	if ((_mm_movemask_ps(dirIsZero) & 7) == 7) {
		goto done;
	}
	tNear = nodeTNear;
	tFar = rayLength;
traverse:
	while (type < 0) {
		int32_t axis;
		uint32_t axisBit;
		const hkKdTreeNode* nearChild;
		int32_t dirIsZeroMask = _mm_movemask_ps(_mm_cmpeq_ps(zero, unitDir));
		while (true) {
			// The left child sits the byte offset in bits 2 to 30 past the node (the SDK's hkKdTreeNode::getLeft):
			// the exe adds it to the node's address as a byte count.
			node = reinterpret_cast<const hkKdTreeNode*>(reinterpret_cast<uintptr_t>(node) +
				static_cast<uint32_t>(node->m_data.m_node.m_type_left_axis & 0x7FFFFFFC));
			axis = type & 3;
			axisBit = axisMaskBits_10C92E0[axis];
			if ((axisBit & dirIsZeroMask) != axisBit) {
				break;
			}
			// The ray does not move along the split axis: from's coordinate decides which children it is in.
			float splitMax =
				static_cast<double>(static_cast<uint16_t>(secondWord >> 16)) / 65535.0 * scale[axis] + offset[axis];
			float splitMaxPlus = splitMax + 9.999999747378752e-05;
			float splitMin =
				static_cast<double>(static_cast<uint16_t>(secondWord)) / 65535.0 * scale[axis] + offset[axis];
			float splitMinMinus = splitMin - 9.999999747378752e-05;
			if (!(splitMaxPlus >= origin[axis])) {
				if (!(splitMinMinus <= origin[axis])) {
					goto popStack;
				}
				++node;
			} else if (splitMinMinus <= origin[axis]) {
				stackTop->tNear = tNear;
				stackTop->node = node + 1;
				stackTop->tFar = tFar;
				++stackTop;
			}
			type = node->m_data.m_leaf.m_type_primId;
			secondWord = static_cast<int32_t>(node->m_data.m_leaf.m_numPrimitives);
			if (type >= 0) {
				goto leaf;
			}
		}
		// The ray parameters where it crosses the two splits, near child first along the ray's direction.
		float tSplitNear = splitScale[axis] * static_cast<double>(static_cast<uint16_t>(secondWord)) + splitBias[axis];
		float tSplitFar =
			static_cast<double>(static_cast<uint16_t>(secondWord >> 16)) * splitScale[axis] + splitBias[axis];
		if ((axisBit & _mm_movemask_ps(_mm_cmplt_ps(zero, unitDir))) == axisBit) {
			nearChild = node++;
		} else {
			nearChild = node + 1;
			float tSwap = tSplitNear;
			tSplitNear = tSplitFar;
			tSplitFar = tSwap;
		}
		if (tSplitFar < tNear) {
			// Only the far child: node already points to it.
		} else if (!(tMax < tSplitNear)) {
			// Both children: the far one goes on the stack from where the ray enters it, the near one is next.
			float farTNear = tSplitNear;
			if (!(tNear < tSplitNear)) {
				farTNear = tNear;
			}
			stackTop->node = node;
			stackTop->tNear = farTNear;
			stackTop->tFar = tFar;
			++stackTop;
			node = nearChild;
			if (tSplitFar < tFar) {
				nodeTFar = tSplitFar;
			}
			float collectorMax = collector->getHitFraction() * rayLength;
			tMax = nodeTFar;
			if (nodeTFar > collectorMax) {
				tMax = collectorMax;
			}
			tNear = nodeTNear;
			tFar = nodeTFar;
		} else {
			// Only the near child.
			node = nearChild;
		}
		type = node->m_data.m_leaf.m_type_primId;
		secondWord = static_cast<int32_t>(node->m_data.m_leaf.m_numPrimitives);
	}
leaf:
	if (type != 0x7FFFFFFF && secondWord > 0) {
		// Tests the ray's slab interval against each projected entry's dequantised box, all lanes at once.
		__m128 invDirIsPositive = _mm_cmple_ps(zero, invDir);
		__m128 invDirIsZero = _mm_cmpeq_ps(zero, invDir);
		__m128 highWordScale = _mm_set1_ps(1e+16f);
		__m128 nearWhereZero = _mm_and_ps(invDirIsZero, _mm_set1_ps(-3.40282e+38f));
		__m128 farWhereZero = _mm_and_ps(invDirIsZero, _mm_set1_ps(3.40282e+38f));
		__m128 tNearSplat = _mm_set1_ps(nodeTNear);
		__m128 tMaxSplat = _mm_set1_ps(tMax);
		const __m128i zeroWords = _mm_setzero_si128();
		const hkKdTreeProjectedEntry* entry = &projectedEntries[type & 0x7FFFFFFF];
		int32_t numLeft = secondWord;
		do {
			__m128 entryScale = _mm_mul_ps(scale, x);
			hkPrimitiveId primitiveId = entry->getPrimitiveId();
			__m128i words =
				_mm_set_epi16(static_cast<int16_t>(entry->m_primIdLow), static_cast<int16_t>(entry->m_max[2]),
					static_cast<int16_t>(entry->m_max[1]), static_cast<int16_t>(entry->m_max[0]),
					static_cast<int16_t>(entry->m_primIdHigh), static_cast<int16_t>(entry->m_min[2]),
					static_cast<int16_t>(entry->m_min[1]), static_cast<int16_t>(entry->m_min[0]));
			__m128i highWords = _mm_unpackhi_epi16(words, zeroWords);
			__m128i lowWords = _mm_unpacklo_epi16(words, zeroWords);
			__m128 boxMin =
				_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(lowWords, 16)),
													 highWordScale),
										  _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(lowWords, 16), 16))),
							   entryScale),
					offset);
			__m128 boxMax =
				_mm_add_ps(_mm_mul_ps(_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(highWords, 16)),
													 highWordScale),
										  _mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(highWords, 16), 16))),
							   entryScale),
					offset);
			__m128 tMinSlab = _mm_mul_ps(_mm_sub_ps(boxMin, origin), invDir);
			__m128 tMaxSlab = _mm_mul_ps(_mm_sub_ps(boxMax, origin), invDir);
			__m128 tEnter = _mm_or_ps(_mm_andnot_ps(invDirIsZero,
										  _mm_or_ps(_mm_andnot_ps(invDirIsPositive, tMaxSlab),
											  _mm_and_ps(invDirIsPositive, tMinSlab))),
				nearWhereZero);
			__m128 tLeave = _mm_or_ps(_mm_andnot_ps(invDirIsZero,
										  _mm_or_ps(_mm_and_ps(invDirIsPositive, tMaxSlab),
											  _mm_andnot_ps(invDirIsPositive, tMinSlab))),
				farWhereZero);
			__m128 enter = _mm_max_ps(tNearSplat,
				_mm_max_ps(_mm_max_ps(_mm_shuffle_ps(tEnter, tEnter, 0), _mm_shuffle_ps(tEnter, tEnter, 85)),
					_mm_shuffle_ps(tEnter, tEnter, 170)));
			__m128 leave = _mm_min_ps(tMaxSplat,
				_mm_min_ps(_mm_min_ps(_mm_shuffle_ps(tLeave, tLeave, 0), _mm_shuffle_ps(tLeave, tLeave, 85)),
					_mm_shuffle_ps(tLeave, tLeave, 170)));
			__m128 inside =
				_mm_and_ps(_mm_and_ps(_mm_cmple_ps(origin, boxMax), _mm_cmple_ps(boxMin, origin)), invDirIsZero);
			if ((_mm_movemask_ps(_mm_or_ps(_mm_andnot_ps(invDirIsZero, _mm_cmplt_ps(enter, leave)), inside)) & 7) ==
				7) {
				hkBool hitResult;
				collector->addMidPhaseHit(&hitResult, primitiveId);
			}
			++entry;
		} while (--numLeft);
	}
popStack:
	while (stackTop != stackBottom) {
		--stackTop;
		node = stackTop->node;
		nodeTFar = stackTop->tFar;
		type = node->m_data.m_leaf.m_type_primId;
		secondWord = static_cast<int32_t>(node->m_data.m_leaf.m_numPrimitives);
		nodeTNear = stackTop->tNear;
		float collectorMax = collector->getHitFraction() * rayLength;
		tMax = nodeTFar;
		if (nodeTFar > collectorMax) {
			tMax = collectorMax;
		}
		if (!(nodeTNear >= tMax)) {
			tNear = nodeTNear;
			tFar = nodeTFar;
			goto traverse;
		}
	}
done:
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = stackStorage;
	collector->hasHit(arResult);
	return arResult;
}

// The node stack lives in the thread's hkStackMemory (maxDepth entries, the block rounded up to 128 bytes) and is given
// back at the end. The exe's division of a split by 65535.0 is x87 (fild, fdiv by the double, fmul and fadd of the
// floats, fstp), the same double expression here.
hkBool* hkKdTreeUtils::castRayBundleIterative(hkBool* arResult, const hkKdTree* tree, const RayBundle& fromBundle,
	const RayBundle& toBundle, hkKdTreeBundleCastCollector* collector, hkVector4Comparison::Mask rayMask) {
	const hkKdTreeNode* node = tree->m_nodes.m_data;
	const hkKdTreeProjectedEntry* projectedEntries = tree->m_projectedEntries.m_data;
	hkVector4 offset = tree->m_aabb.m_min;
	hkVector4 scale;
	scale.m_quad = _mm_sub_ps(tree->m_aabb.m_max.m_quad, tree->m_aabb.m_min.m_quad);
	int32_t maxDepth = tree->m_maxDepth;
	if (maxDepth <= 0) {
		maxDepth = DEFAULT_STACK_SIZE;
	}
	// A node's two dwords: an inner node's left offset and axis, and its two 16-bit splits (min in the low word); a
	// leaf's primitive id and count. The exe reads the second dword once and tests it signed (jle).
	int32_t typePrimId = node->m_data.m_leaf.m_type_primId;
	int32_t numPrimitives = static_cast<int32_t>(node->m_data.m_leaf.m_numPrimitives);

	hkMemoryRouter& router = hkMemoryRouter::getInstance();
	char* stackBase = router.m_stack.m_next.m_storage;
	router.m_stack.m_next.m_storage = stackBase + ((maxDepth * static_cast<int32_t>(sizeof(BundleState)) + 127) & ~127);
	// The SDK's hkAllocateStack: the stack memory is raw bytes, which the exe uses as the stack's entries.
	BundleState* const stackStart = reinterpret_cast<BundleState*>(stackBase);
	BundleState* stack = stackStart;

	const hkVector4 zero = hkVector4::getZero();
	const hkQuadReal d0 = _mm_sub_ps(toBundle.m_vec[0].m_quad, fromBundle.m_vec[0].m_quad);
	const hkQuadReal d1 = _mm_sub_ps(toBundle.m_vec[1].m_quad, fromBundle.m_vec[1].m_quad);
	const hkQuadReal d2 = _mm_sub_ps(toBundle.m_vec[2].m_quad, fromBundle.m_vec[2].m_quad);
	const hkQuadReal lengthSquared = _mm_add_ps(_mm_add_ps(_mm_mul_ps(d0, d0), _mm_mul_ps(d1, d1)), _mm_mul_ps(d2, d2));
	// One Newton step on rsqrtps (3 and 0.5 in every lane), then one on rcpps: each ray's length.
	const hkQuadReal r = _mm_rsqrt_ps(lengthSquared);
	const hkQuadReal invLength = _mm_mul_ps(_mm_sub_ps(hkQuadReal3333, _mm_mul_ps(_mm_mul_ps(r, lengthSquared), r)),
		_mm_mul_ps(r, hkQuadRealHalf));
	const hkQuadReal e = _mm_rcp_ps(invLength);
	hkVector4 rayLength;
	rayLength.m_quad = _mm_sub_ps(_mm_add_ps(e, e), _mm_mul_ps(_mm_mul_ps(e, invLength), e));
	hkVector4Comparison activeMask;
	activeMask.m_mask = hkVector4Comparison_s_maskFromBits[rayMask];
	hkVector4 dir[3];
	dir[0].m_quad = _mm_mul_ps(invLength, d0);
	dir[1].m_quad = _mm_mul_ps(d1, invLength);
	dir[2].m_quad = _mm_mul_ps(d2, invLength);

	// Per axis: each ray's inverse direction (0 where the direction is 0), the ray parameter one unit of a 16-bit split
	// moves (scaleStep) and where the ray meets the tree's minimum (originT).
	const hkQuadReal oneOver65K = _mm_set1_ps(1.5259022e-05f);
	hkVector4 scaleStep[3];
	RayBundle invDir;
	hkVector4 originT[3];
	for (int32_t i = 0; i < 3; ++i) {
		const hkQuadReal d = dir[i].m_quad;
		const hkQuadReal isZero = _mm_cmpeq_ps(zero.m_quad, d);
		const hkQuadReal safe = _mm_or_ps(_mm_andnot_ps(isZero, d), _mm_and_ps(isZero, hkQuadReal1111));
		const hkQuadReal re = _mm_rcp_ps(safe);
		const hkQuadReal inv =
			_mm_or_ps(_mm_andnot_ps(isZero, _mm_sub_ps(_mm_add_ps(re, re), _mm_mul_ps(_mm_mul_ps(re, safe), re))),
				_mm_and_ps(isZero, zero.m_quad));
		scaleStep[i].m_quad = _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(scale(i)), inv), oneOver65K);
		invDir.m_vec[i].m_quad = inv;
		originT[i].m_quad = _mm_mul_ps(_mm_sub_ps(_mm_set1_ps(offset(i)), fromBundle.m_vec[i].m_quad), inv);
	}

	// The collector's hit fractions of the rays (1 until it reports some), the active rays' parameter range in the
	// current node, and tMax: the far end clipped to the hit fractions.
	hkVector4 fractions;
	fractions.m_quad = _mm_set1_ps(1.0f);
	hkQuadReal tNear = _mm_setzero_ps();
	hkQuadReal tFar = rayLength.m_quad;
	hkQuadReal tMax = _mm_min_ps(_mm_mul_ps(fractions.m_quad, rayLength.m_quad), tFar);

	for (;;) {
		while (typePrimId < 0) {
			// The SDK's hkKdTreeNode::getLeft and getRight: the exe adds the left offset to the node's address in
			// bytes (hkUlong arithmetic, not an array index).
			const hkKdTreeNode* left = reinterpret_cast<const hkKdTreeNode*>(reinterpret_cast<uintptr_t>(node) +
				(node->m_data.m_node.m_type_left_axis & 0x7FFFFFFC));
			const hkKdTreeNode* right = left + 1;
			const int32_t axis = typePrimId & 3;
			if ((_mm_movemask_ps(_mm_cmpeq_ps(dir[axis].m_quad, zero.m_quad)) & _mm_movemask_ps(activeMask.m_mask)) !=
				0) {
				// Some active ray runs parallel to the split plane: each ray goes to the sides its origin lies in, with
				// 1e-5 of slack either way.
				const float splitMin = static_cast<float>(static_cast<double>(static_cast<uint16_t>(numPrimitives)) /
						65535.0 * scale(axis) +
					offset(axis));
				const float splitMax =
					static_cast<float>(static_cast<double>(static_cast<uint16_t>(static_cast<uint32_t>(numPrimitives) >>
										   16)) /
							65535.0 * scale(axis) +
						offset(axis));
				const hkQuadReal eps = _mm_set1_ps(1e-5f);
				const hkQuadReal origin = fromBundle.m_vec[axis].m_quad;
				hkVector4Comparison leftMask;
				leftMask.m_mask =
					_mm_and_ps(_mm_cmple_ps(origin, _mm_add_ps(_mm_set1_ps(splitMax), eps)), activeMask.m_mask);
				hkVector4Comparison rightMask;
				rightMask.m_mask =
					_mm_and_ps(_mm_cmple_ps(_mm_sub_ps(_mm_set1_ps(splitMin), eps), origin), activeMask.m_mask);
				if (_mm_movemask_ps(leftMask.m_mask)) {
					node = left;
					if (_mm_movemask_ps(rightMask.m_mask)) {
						// The exe stores the node's dword inline.
						stack->node.m_storage = right;
						stack->tNear.m_quad = tNear;
						stack->tFar.m_quad = tFar;
						stack->activeMask = rightMask;
						++stack;
						activeMask = leftMask;
					}
				} else {
					if (!_mm_movemask_ps(rightMask.m_mask)) {
						goto popStack;
					}
					node = right;
				}
			} else {
				// Front to back: the near child is the left one where an active ray points up the axis. tFarStart is
				// where the rays enter the far child, tNearEnd where they leave the near one.
				const hkQuadReal tSplitMin =
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(static_cast<float>(static_cast<uint16_t>(numPrimitives))),
								   scaleStep[axis].m_quad),
						originT[axis].m_quad);
				const hkQuadReal tSplitMax =
					_mm_add_ps(_mm_mul_ps(_mm_set1_ps(static_cast<float>(static_cast<
											  uint16_t>(static_cast<uint32_t>(numPrimitives) >> 16))),
								   scaleStep[axis].m_quad),
						originT[axis].m_quad);
				const hkQuadReal dirPositive =
					_mm_and_ps(_mm_cmple_ps(zero.m_quad, dir[axis].m_quad), activeMask.m_mask);
				const hkKdTreeNode* nearChild;
				const hkKdTreeNode* farChild;
				if (_mm_movemask_ps(dirPositive)) {
					nearChild = left;
					farChild = right;
				} else {
					nearChild = right;
					farChild = left;
				}
				const hkQuadReal tFarStart =
					_mm_or_ps(_mm_andnot_ps(dirPositive, tSplitMax), _mm_and_ps(dirPositive, tSplitMin));
				const hkQuadReal tNearEnd =
					_mm_or_ps(_mm_and_ps(dirPositive, tSplitMax), _mm_andnot_ps(dirPositive, tSplitMin));
				hkVector4Comparison nearMask;
				nearMask.m_mask = _mm_and_ps(_mm_cmple_ps(tNear, tNearEnd), activeMask.m_mask);
				hkVector4Comparison farMask;
				farMask.m_mask = _mm_and_ps(_mm_cmple_ps(tFarStart, tMax), activeMask.m_mask);
				if (!_mm_movemask_ps(nearMask.m_mask)) {
					node = farChild;
				} else if (!_mm_movemask_ps(farMask.m_mask)) {
					node = nearChild;
				} else {
					// The exe stores the node's dword inline.
					stack->node.m_storage = farChild;
					stack->tNear.m_quad = _mm_max_ps(tNear, tFarStart);
					stack->tFar.m_quad = tFar;
					stack->activeMask = farMask;
					++stack;
					node = nearChild;
					activeMask = nearMask;
					tFar = _mm_min_ps(tNearEnd, tFar);
					tMax = _mm_min_ps(_mm_mul_ps(fractions.m_quad, rayLength.m_quad), tFar);
				}
			}
			typePrimId = node->m_data.m_leaf.m_type_primId;
			numPrimitives = static_cast<int32_t>(node->m_data.m_leaf.m_numPrimitives);
		}

		// A leaf (0x7FFFFFFF: an empty one): each projected entry's box against the active rays.
		if (typePrimId != 0x7FFFFFFF && numPrimitives > 0) {
			// The SDK's hkIntVector::convertToF32fromU32 (high word times this, plus the low word); the exe's constant
			// is 1e16 (its words' high halves are 0 here, so the product is 0).
			const hkQuadReal highWordScale = _mm_set1_ps(1e16f);
			// The SDK's hkGetProjectedEntry: getPrimitiveId's index.
			const hkKdTreeProjectedEntry* entry = projectedEntries + (typePrimId & 0x7FFFFFFF);
			for (int32_t n = numPrimitives; n != 0; --n, ++entry) {
				// The SDK's hkKdTreeProjectedEntry::getPrimitiveId: m_primIdHigh is the low half.
				const hkPrimitiveId primitiveId = entry->getPrimitiveId();

				// EXE-INLINED none: hkKdTreeMath::getAabbVecsFromProjectedEntry
				const hkQuadReal entryScale = _mm_mul_ps(scale.m_quad, x);
				// EXE-INLINED none: hkKdTreeMath::convertProjectedEntryToHkVector4
				// The SDK and the exe read the 16-aligned entry's raw bytes as four integers (an hkIntVector). Where the
				// id halves are 32 bits the entry is not those eight words: they are gathered from its fields (the id
				// words' lanes are not used).
				hkIntVector b;
				if constexpr (sizeof(hkHalfPointer) == sizeof(uint16_t)) {
					b = *reinterpret_cast<const hkIntVector*>(entry);
				} else {
					b.m_quad =
						_mm_set_epi16(static_cast<int16_t>(entry->m_primIdLow), static_cast<int16_t>(entry->m_max[2]),
							static_cast<int16_t>(entry->m_max[1]), static_cast<int16_t>(entry->m_max[0]),
							static_cast<int16_t>(entry->m_primIdHigh), static_cast<int16_t>(entry->m_min[2]),
							static_cast<int16_t>(entry->m_min[1]), static_cast<int16_t>(entry->m_min[0]));
				}
				const hkQuadUint zeroInt = _mm_setzero_si128();
				const hkQuadUint minUint = _mm_unpacklo_epi16(b.m_quad, zeroInt);
				const hkQuadUint maxUint = _mm_unpackhi_epi16(b.m_quad, zeroInt);
				const hkQuadReal minF =
					_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(minUint, 16)), highWordScale),
						_mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(minUint, 16), 16)));
				const hkQuadReal maxF =
					_mm_add_ps(_mm_mul_ps(_mm_cvtepi32_ps(_mm_srli_epi32(maxUint, 16)), highWordScale),
						_mm_cvtepi32_ps(_mm_srli_epi32(_mm_slli_epi32(maxUint, 16), 16)));
				const hkQuadReal aabbMin = _mm_add_ps(_mm_mul_ps(minF, entryScale), offset.m_quad);
				const hkQuadReal aabbMax = _mm_add_ps(_mm_mul_ps(maxF, entryScale), offset.m_quad);
				hkVector4 aabbVecs[6];
				aabbVecs[0].m_quad = _mm_shuffle_ps(aabbMin, aabbMin, 0x00);
				aabbVecs[1].m_quad = _mm_shuffle_ps(aabbMax, aabbMax, 0x00);
				aabbVecs[2].m_quad = _mm_shuffle_ps(aabbMin, aabbMin, 0x55);
				aabbVecs[3].m_quad = _mm_shuffle_ps(aabbMax, aabbMax, 0x55);
				aabbVecs[4].m_quad = _mm_shuffle_ps(aabbMin, aabbMin, 0xAA);
				aabbVecs[5].m_quad = _mm_shuffle_ps(aabbMax, aabbMax, 0xAA);

				// EXE-INLINED none: hkKdTreeMath::rayBundleAabbCheck
				hkQuadReal checkNear = tNear;
				hkQuadReal checkFar = tMax;
				hkQuadReal flatOrigOK = hkVector4Comparison_s_maskFromBits[hkVector4Comparison::MASK_XYZW];
				for (int32_t i = 0; i < 3; ++i) {
					const hkQuadReal boxMin = aabbVecs[2 * i].m_quad;
					const hkQuadReal boxMax = aabbVecs[2 * i + 1].m_quad;
					const hkQuadReal orig = fromBundle.m_vec[i].m_quad;
					const hkQuadReal inv = invDir.m_vec[i].m_quad;
					const hkQuadReal dNearTemp = _mm_mul_ps(_mm_sub_ps(boxMin, orig), inv);
					const hkQuadReal dFarTemp = _mm_mul_ps(_mm_sub_ps(boxMax, orig), inv);
					const hkQuadReal checkZero = _mm_setzero_ps();
					const hkQuadReal dirGeZero = _mm_cmple_ps(checkZero, inv);
					const hkQuadReal dirEqZero = _mm_cmpeq_ps(checkZero, inv);
					const hkQuadReal inRange = _mm_and_ps(_mm_cmple_ps(orig, boxMax), _mm_cmple_ps(boxMin, orig));
					flatOrigOK =
						_mm_and_ps(_mm_or_ps(_mm_and_ps(inRange, dirEqZero), _mm_andnot_ps(dirEqZero, flatOrigOK)),
							flatOrigOK);
					const hkQuadReal dNear =
						_mm_or_ps(_mm_and_ps(dirGeZero, dNearTemp), _mm_andnot_ps(dirGeZero, dFarTemp));
					const hkQuadReal dFar =
						_mm_or_ps(_mm_and_ps(dirGeZero, dFarTemp), _mm_andnot_ps(dirGeZero, dNearTemp));
					checkNear = _mm_max_ps(_mm_or_ps(_mm_andnot_ps(dirEqZero, dNear), _mm_and_ps(checkNear, dirEqZero)),
						checkNear);
					checkFar = _mm_min_ps(_mm_or_ps(_mm_andnot_ps(dirEqZero, dFar), _mm_and_ps(checkFar, dirEqZero)),
						checkFar);
				}
				hkVector4Comparison hitMask;
				hitMask.m_mask =
					_mm_and_ps(_mm_and_ps(_mm_cmplt_ps(checkNear, checkFar), flatOrigOK), activeMask.m_mask);
				if (_mm_movemask_ps(hitMask.m_mask)) {
					collector->addMidPhaseHit(primitiveId, hitMask);
					collector->getHitFractions(fractions);
				}
			}
		}

	popStack:
		// The next pushed node some active ray still reaches before its hit fraction.
		for (;;) {
			if (stack == stackStart) {
				hkMemoryRouter::getInstance().m_stack.m_next.m_storage = stackBase;
				arResult->m_bool = true;
				return arResult;
			}
			--stack;
			node = stack->node.m_storage;
			tNear = stack->tNear.m_quad;
			tFar = stack->tFar.m_quad;
			typePrimId = node->m_data.m_leaf.m_type_primId;
			numPrimitives = static_cast<int32_t>(node->m_data.m_leaf.m_numPrimitives);
			tMax = _mm_min_ps(_mm_mul_ps(fractions.m_quad, rayLength.m_quad), tFar);
			activeMask.m_mask = _mm_and_ps(_mm_cmplt_ps(tNear, tMax), stack->activeMask.m_mask);
			if (_mm_movemask_ps(activeMask.m_mask)) {
				break;
			}
		}
	}
}

// The SDK's: whether a ray cast hits any of the non-empty trees, each cast in turn into collector.
hkBool* hkKdTreeUtils::castRayMultiTree(hkBool* apResult, int32_t numTrees, hkKdTree* const* trees,
	const hkVector4& start, const hkVector4& end, hkKdTreeCastCollector* collector) {
	apResult->m_bool = false;
	for (int32_t i = 0; i < numTrees; ++i) {
		const hkKdTree* pTree = trees[i];
		if (pTree && pTree->m_nodes.m_size) {
			hkBool castHit;
			hkKdTreeUtils::castRayIterative(&castHit, pTree, start, end, collector);
			apResult->m_bool = apResult->m_bool || castHit.m_bool;
		}
	}
	return apResult;
}
