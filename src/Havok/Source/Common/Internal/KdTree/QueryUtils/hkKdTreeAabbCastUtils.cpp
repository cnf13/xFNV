#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeAabbCastUtils.h"

#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Internal/KdTree/QueryUtils/hkKdTreeCastCollector.h"

#include <xmmintrin.h>

// The function-local constant of hkGetOneOver65K: 1 / 65535 in every lane. A projected entry's 16-bit bound times the
// tree's extent times this is its offset from the tree's minimum.
static hkQuadReal x{1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f, 1.5259022e-05f};

__m128 hkGetOneOver65K() {
	return _mm_load_ps(&reinterpret_cast<float*>(&x)[0]);
}

// The direction is normalized with a reciprocal square root and one Newton step ((0.5f * r) * (3.0f - (len2 * r) * r),
// 0 for a zero length); the ray's length is len2 times that. Its inverse is a reciprocal with one Newton step ((r + r)
// - (r * d) * r) on the direction with its zero lanes set to 1.0 (hkQuadReal1111) and w 1.0, masked to xyz, then zero
// where the direction is zero. The node stack takes (stack size * 16 + 0x7F) & ~0x7F bytes of the router's stack memory
// (the tree's maximum depth, or DEFAULT_STACK_SIZE when that is not positive), given back before collector->hasHit. An
// inner node's planes are its 16-bit splits / 65535.0 times the tree's extent plus its minimum, each step stored as a
// float. Along an axis the cast does not move, a child is visited when the start lies within its plane widened by the
// half extent and 9.999999747378752e-05 (NaN visits neither). Otherwise the planes give the children's entry and exit
// along the cast, ordered by the direction's sign, and the far child is pushed unless the cast leaves the near one
// before tNear or enters the far one after tMax. tMax is the cast's tFar capped by the collector's hit fraction times
// the ray's length, read after each push and each pop (NaN keeps tFar); a popped node is visited only while tNear is
// below tMax. A leaf's entries are tested four lanes at a time, with the 16-bit words converted directly to floats
// (exact). Along an axis the cast does not move the slab is -3.40282e38f to 3.40282e38f and the start must lie inside
// the box. addMidPhaseHit's id is the entry's words at +6 and +0xE.
hkBool* hkKdTreeAabbCastUtils::castAabb(hkBool* arResult, const hkKdTree* root, hkCastAabbInput& castInput,
	hkKdTreeCastCollector* collector, float aabbCheckTolerance) {
	const hkKdTreeProjectedEntry* pEntries = root->m_projectedEntries.m_data;
	alignas(16) float afTreeMin[4];
	alignas(16) float afExtent[4];
	_mm_store_ps(afTreeMin, _mm_load_ps(&reinterpret_cast<const float*>(&root->m_aabb.m_min.m_quad)[0]));
	const __m128 kExtent = _mm_sub_ps(_mm_load_ps(&reinterpret_cast<const float*>(&root->m_aabb.m_max.m_quad)[0]),
		_mm_load_ps(&reinterpret_cast<const float*>(&root->m_aabb.m_min.m_quad)[0]));
	_mm_store_ps(afExtent, kExtent);
	int32_t iStackSize = root->m_maxDepth;
	if (iStackSize <= 0) {
		iStackSize = DEFAULT_STACK_SIZE;
	}
	const hkKdTreeNode* pNode = root->m_nodes.m_data;

	// EXE-INLINED none: hkAllocateStack<hkKdTreeAabbCastUtils::NodeState>
	hkMemoryRouter& kRouter = hkMemoryRouter::getInstance();
	// EXE-INLINED 0x458B50
	char* pStackMemory = kRouter.m_stack.m_next.m_storage;
	kRouter.m_stack.m_next.m_storage = pStackMemory + (((iStackSize << 4) + 0x7F) & ~0x7F);
	// The stack allocator's bytes are untyped memory, used here as the node stack.
	NodeState* const pStackBase = reinterpret_cast<NodeState*>(pStackMemory);
	NodeState* pTop = pStackBase;

	alignas(16) float afFrom[4];
	alignas(16) float afHalfExtents[4];
	const __m128 kFrom = _mm_load_ps(&reinterpret_cast<float*>(&castInput.m_from.m_quad)[0]);
	_mm_store_ps(afFrom, kFrom);
	const __m128 kDir = _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&castInput.m_to.m_quad)[0]), kFrom);
	const __m128 kDir2 = _mm_mul_ps(kDir, kDir);
	const __m128 kLen2 =
		_mm_add_ss(_mm_shuffle_ps(kDir2, kDir2, 0xAA), _mm_add_ss(_mm_shuffle_ps(kDir2, kDir2, 0x55), kDir2));
	// EXE-INLINED none: hkVector4::normalize3 (the SDK's sqrtInverse)
	const __m128 kR = _mm_rsqrt_ss(kLen2);
	const __m128 kLenZero = _mm_cmpeq_ss(kLen2, _mm_setzero_ps());
	const __m128 kNewton = _mm_mul_ss(_mm_mul_ss(_mm_set_ss(0.5f), kR),
		_mm_sub_ss(_mm_set_ss(3.0f), _mm_mul_ss(_mm_mul_ss(kLen2, kR), kR)));
	const __m128 kInvLength = _mm_andnot_ps(kLenZero, kNewton);
	const __m128 kDirN = _mm_mul_ps(_mm_shuffle_ps(kInvLength, kInvLength, 0x00), kDir);
	const float fRayLength = _mm_cvtss_f32(_mm_mul_ss(kLen2, kInvLength));
	const __m128 kHalfExtents = _mm_load_ps(&reinterpret_cast<float*>(&castInput.m_halfExtents.m_quad)[0]);
	_mm_store_ps(afHalfExtents, kHalfExtents);
	const __m128 kExpand = _mm_add_ps(_mm_set1_ps(aabbCheckTolerance), kHalfExtents);
	float fTNear = 0.0f;
	float fTFar = fRayLength;

	const __m128 kZero = _mm_setzero_ps();
	const __m128 kOne = _mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111)[0]);
	const __m128 kDirZero = _mm_cmpeq_ps(kZero, kDirN);
	const __m128 kSafeDir = _mm_or_ps(_mm_andnot_ps(kDirZero, kDirN), _mm_and_ps(kDirZero, kOne));
	const __m128 kDivisor = _mm_shuffle_ps(kSafeDir, _mm_unpackhi_ps(kSafeDir, kOne), 0xC4);
	const __m128 kRcp = _mm_rcp_ps(kDivisor);
	const __m128 kInverse = _mm_and_ps(_mm_sub_ps(_mm_add_ps(kRcp, kRcp), _mm_mul_ps(_mm_mul_ps(kRcp, kDivisor), kRcp)),
		_mm_load_ps(&reinterpret_cast<float*>(&rdata_10C4C10)[0]));
	const __m128 kInvDir = _mm_or_ps(_mm_andnot_ps(kDirZero, kInverse), _mm_and_ps(kDirZero, kZero));
	alignas(16) float afInvDir[4];
	_mm_store_ps(afInvDir, kInvDir);

	const float fEarlyOut = collector->getHitFraction() * fRayLength;
	float fTMax = (fEarlyOut < fRayLength) ? fEarlyOut : fRayLength;

	if ((_mm_movemask_ps(kDirZero) & 7) != 7) {
		const int32_t iDirZeroBits = _mm_movemask_ps(kDirZero);
		const int32_t iDirPositiveBits = _mm_movemask_ps(_mm_cmplt_ps(kZero, kDirN));
		for (;;) {
			// Descend through the inner nodes.
			bool bVisitLeaf = true;
			while (pNode->m_data.m_node.m_type_left_axis < 0) {
				const int32_t iHeader = pNode->m_data.m_node.m_type_left_axis;
				// The node stores its left child as a byte offset from itself, which is added to the node's address.
				const hkKdTreeNode* pLeft = reinterpret_cast<const hkKdTreeNode*>(reinterpret_cast<const char*>(pNode) +
					(iHeader & 0x7FFFFFFC));
				const hkKdTreeNode* pRight = pLeft + 1;
				const int32_t iAxis = iHeader & 3;
				const uint32_t uiAxisBit = axisMaskBits_10C92E0[iAxis];
				const float fLow = static_cast<int32_t>(pNode->m_data.m_node.m_splitMin) / 65535.0 * afExtent[iAxis] +
					afTreeMin[iAxis];
				if ((iDirZeroBits & uiAxisBit) == uiAxisBit) {
					const float fLowHalf = fLow - afHalfExtents[iAxis];
					const float fHigh =
						static_cast<int32_t>(pNode->m_data.m_node.m_splitMax) / 65535.0 * afExtent[iAxis] +
						afTreeMin[iAxis];
					const float fHighHalf = fHigh + afHalfExtents[iAxis];
					const float fHighTest = fHighHalf + 9.999999747378752e-05;
					const bool bLeft = fHighTest >= afFrom[iAxis];
					const float fLowTest = fLowHalf - 9.999999747378752e-05;
					const bool bRight = fLowTest <= afFrom[iAxis];
					if (bLeft) {
						if (bRight) {
							// EXE-INLINED none: hkKdTreeAabbCastUtils::NodeState::set
							pTop->node = pRight;
							pTop->tNear = fTNear;
							pTop->tFar = fTFar;
							++pTop;
						}
						pNode = pLeft;
					} else if (bRight) {
						pNode = pRight;
					} else {
						bVisitLeaf = false;
						break;
					}
				} else {
					const float fLowHalf = fLow - afHalfExtents[iAxis];
					const float fTLow = (fLowHalf - afFrom[iAxis]) * afInvDir[iAxis];
					const float fHigh =
						static_cast<int32_t>(pNode->m_data.m_node.m_splitMax) / 65535.0 * afExtent[iAxis] +
						afTreeMin[iAxis];
					const float fHighHalf = fHigh + afHalfExtents[iAxis];
					const float fTHigh = (fHighHalf - afFrom[iAxis]) * afInvDir[iAxis];
					const hkKdTreeNode* pNear;
					const hkKdTreeNode* pFar;
					float fTEnterFar;
					float fTLeaveNear;
					if ((iDirPositiveBits & uiAxisBit) == uiAxisBit) {
						pNear = pLeft;
						pFar = pRight;
						fTEnterFar = fTLow;
						fTLeaveNear = fTHigh;
					} else {
						pNear = pRight;
						pFar = pLeft;
						fTEnterFar = fTHigh;
						fTLeaveNear = fTLow;
					}
					if (fTLeaveNear < fTNear) {
						pNode = pFar;
					} else if (fTMax < fTEnterFar) {
						pNode = pNear;
					} else {
						const float fFarNear = (fTNear < fTEnterFar) ? fTEnterFar : fTNear;
						// EXE-INLINED none: hkKdTreeAabbCastUtils::NodeState::set
						pTop->node = pFar;
						pTop->tNear = fFarNear;
						pTop->tFar = fTFar;
						++pTop;
						pNode = pNear;
						if (fTLeaveNear < fTFar) {
							fTFar = fTLeaveNear;
						}
						const float fHitDistance = collector->getHitFraction() * fRayLength;
						fTMax = (fTFar > fHitDistance) ? fHitDistance : fTFar;
					}
				}
			}

			const int32_t iHeader = pNode->m_data.m_leaf.m_type_primId;
			const int32_t iNumPrimitives = static_cast<int32_t>(pNode->m_data.m_leaf.m_numPrimitives);
			if (bVisitLeaf && (iHeader ^ 0x7FFFFFFF) != 0 && iNumPrimitives > 0) {
				const __m128 kPositive = _mm_cmple_ps(kZero, kInvDir);
				const __m128 kParallel = _mm_cmpeq_ps(kZero, kInvDir);
				// -3.40282e38f and 3.40282e38f (0xFF7FFFEE, 0x7F7FFFEE) where the cast does not move.
				const __m128 kParallelNear = _mm_and_ps(kParallel, _mm_set1_ps(-0x1.ffffdcp+127f));
				const __m128 kParallelFar = _mm_and_ps(kParallel, _mm_set1_ps(0x1.ffffdcp+127f));
				const __m128 kTNear = _mm_set1_ps(fTNear);
				const __m128 kTMax = _mm_set1_ps(fTMax);
				const hkKdTreeProjectedEntry* pEntry = pEntries + (iHeader & 0x7FFFFFFF);
				int32_t iLeft = iNumPrimitives;
				do {
					// EXE-INLINED 0xDF2740
					const __m128 kScale = _mm_mul_ps(kExtent, _mm_load_ps(&reinterpret_cast<float*>(&x)[0]));
					const __m128 kMinWords =
						_mm_set_ps(static_cast<float>(pEntry->m_primIdHigh), static_cast<float>(pEntry->m_min[2]),
							static_cast<float>(pEntry->m_min[1]), static_cast<float>(pEntry->m_min[0]));
					const __m128 kMaxWords =
						_mm_set_ps(static_cast<float>(pEntry->m_primIdLow), static_cast<float>(pEntry->m_max[2]),
							static_cast<float>(pEntry->m_max[1]), static_cast<float>(pEntry->m_max[0]));
					const __m128 kTreeMin = _mm_load_ps(afTreeMin);
					const __m128 kBoxMin = _mm_sub_ps(_mm_add_ps(_mm_mul_ps(kMinWords, kScale), kTreeMin), kExpand);
					const __m128 kBoxMax = _mm_add_ps(_mm_add_ps(_mm_mul_ps(kMaxWords, kScale), kTreeMin), kExpand);
					const __m128 kTMin = _mm_mul_ps(_mm_sub_ps(kBoxMin, kFrom), kInvDir);
					const __m128 kTMaxPlane = _mm_mul_ps(_mm_sub_ps(kBoxMax, kFrom), kInvDir);
					const __m128 kEnter =
						_mm_or_ps(_mm_andnot_ps(kParallel,
									  _mm_or_ps(_mm_andnot_ps(kPositive, kTMaxPlane), _mm_and_ps(kPositive, kTMin))),
							kParallelNear);
					const __m128 kLeave =
						_mm_or_ps(_mm_andnot_ps(kParallel,
									  _mm_or_ps(_mm_and_ps(kPositive, kTMaxPlane), _mm_andnot_ps(kPositive, kTMin))),
							kParallelFar);
					const __m128 kLeaveMin = _mm_min_ps(kTMax,
						_mm_min_ps(_mm_min_ps(_mm_shuffle_ps(kLeave, kLeave, 0x00),
									   _mm_shuffle_ps(kLeave, kLeave, 0x55)),
							_mm_shuffle_ps(kLeave, kLeave, 0xAA)));
					const __m128 kEnterMax = _mm_max_ps(kTNear,
						_mm_max_ps(_mm_max_ps(_mm_shuffle_ps(kEnter, kEnter, 0x00),
									   _mm_shuffle_ps(kEnter, kEnter, 0x55)),
							_mm_shuffle_ps(kEnter, kEnter, 0xAA)));
					const __m128 kInside =
						_mm_and_ps(_mm_and_ps(_mm_cmple_ps(kBoxMin, kFrom), _mm_cmple_ps(kFrom, kBoxMax)), kParallel);
					const __m128 kHit =
						_mm_or_ps(_mm_andnot_ps(kParallel, _mm_cmplt_ps(kEnterMax, kLeaveMin)), kInside);
					if ((_mm_movemask_ps(kHit) & 7) == 7) {
						const hkPrimitiveId uiPrimitiveId = pEntry->getPrimitiveId();
						hkBool kAdded;
						collector->addMidPhaseHit(&kAdded, uiPrimitiveId);
					}
					++pEntry;
				} while (--iLeft);
			}

			// Pop the next subtree the cast still reaches before tMax.
			bool bFound = false;
			while (pTop != pStackBase) {
				--pTop;
				fTFar = pTop->tFar;
				pNode = pTop->node;
				fTNear = pTop->tNear;
				const float fHitDistance = collector->getHitFraction() * fRayLength;
				fTMax = (fTFar > fHitDistance) ? fHitDistance : fTFar;
				if (!(fTNear >= fTMax)) {
					bFound = true;
					break;
				}
			}
			if (!bFound) {
				break;
			}
		}
	}

	// EXE-INLINED none: hkDeallocateStack<hkKdTreeAabbCastUtils::NodeState>
	hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pStackMemory;
	collector->hasHit(arResult);
	return arResult;
}
