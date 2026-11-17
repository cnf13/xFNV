#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3RootMotion.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

#include <xmmintrin.h>

// The left index climbs past the pairs below the middle one and the right index drops past those above it; the
// recursion takes the left part, the loop the right one.
int32_t hkAlgorithm::quickSortRecursive(hkValueIndexPair* apArr, int32_t aiLow, int32_t aiHigh, uint8_t aiLess) {
	hkAlgorithm::less<hkValueIndexPair> cmpLess;
	int32_t d = aiLow;
	for (;;) {
		int32_t i = d;
		int32_t j = aiHigh;
		const hkValueIndexPair str = apArr[(d + j) >> 1];
		do {
			while (cmpLess(apArr[i], str)) {
				i++;
			}
			while (cmpLess(str, apArr[j])) {
				j--;
			}
			if (j < i) {
				break;
			}
			if (j != i) {
				hkValueIndexPair tmp = apArr[j];
				apArr[j] = apArr[i];
				apArr[i] = tmp;
			}
			--j;
			++i;
		} while (i <= j);
		if (d < j) {
			j = hkAlgorithm::quickSortRecursive(apArr, d, j, aiLess);
		}
		if (i >= aiHigh) {
			return j;
		}
		d = i;
	}
}

void hkpEntityAabbUtil::entityBatchInvalidateAabb(hkpEntity* const* entityBatch, int32_t numEntities) {
	int32_t i;
	hkpCollidable::BoundingVolumeData* p_m_boundingVolumeData;
	for (i = 0; i < numEntities; p_m_boundingVolumeData->m_max[0] = (uint32_t)0) {
		p_m_boundingVolumeData = &entityBatch[i++]->m_collidable.m_boundingVolumeData;
		p_m_boundingVolumeData->m_min[0] = (uint32_t)1;
	}
}

// For each entity: its shape's aabb, grown by the motion over the step (the SDK's hkAabbUtil::sweepAabb), quantised
// into the collidable's bounding volume data with the grown box stored as expansion bytes. A shape with room for
// child shape aabbs (m_capacityChildShapeAabbs) gets one quantised box per child shape key instead, each grown
// along the root's sampled motion (hkAabbUtil::sweepOffsetAabb on hkpAgent3CalcRootMotion's samples); the union of
// the boxes is the collidable's own, and the child boxes and keys are stored sorted by their lower x.
void hkpEntityAabbUtil::entityBatchRecalcAabb(const hkpCollisionInput* collisionInput, hkpEntity* const* entityBatch,
	int32_t numEntities) {
	// The 0.5 is a double; the product is stored as a float.
	const float halfTolerance = collisionInput->m_tolerance * 0.5;
	for (int32_t i = 0; i < numEntities; i++) {
		hkpEntity* entity = entityBatch[i];
		hkpCollidable::BoundingVolumeData& bvd = entity->m_collidable.m_boundingVolumeData;
		const hkpShape* shape = entity->m_collidable.m_shape;
		const hkMotionState& motionState = entity->m_motion.m_motionState;
		const hkpCollisionInput::Aabb32Info& info = collisionInput->m_aabb32Info;
		if (bvd.m_capacityChildShapeAabbs != 0) {
			const hkpShapeContainer* container = shape->getContainer();
			const int32_t numChildShapes = container->getNumChildShapes();

			// EXE-INLINED none: hkAllocateStack<hkAabbUint32>
			hkStackMemory& aabbStack = hkMemoryRouter::getInstance().m_stack;
			hkAabbUint32* childAabbs = reinterpret_cast<hkAabbUint32*>(aabbStack.m_next.m_storage);
			aabbStack.m_next.m_storage = reinterpret_cast<char*>(childAabbs) +
				((numChildShapes * static_cast<int32_t>(sizeof(hkAabbUint32)) + 127) & ~127);
			// EXE-INLINED none: hkAllocateStack<hkpShapeKey>
			hkStackMemory& keyStack = hkMemoryRouter::getInstance().m_stack;
			uint32_t* childKeys = reinterpret_cast<uint32_t*>(keyStack.m_next.m_storage);
			keyStack.m_next.m_storage = reinterpret_cast<char*>(childKeys) +
				((numChildShapes * static_cast<int32_t>(sizeof(uint32_t)) + 127) & ~127);

			hkpAgent3RootMotion rootMotion;
			hkpAgent3CalcRootMotion(&motionState, rootMotion);

			// The union of the child boxes, and of the grown ones, both from the centre of mass.
			hkAabb compoundAabb;
			compoundAabb.m_min = motionState.m_sweptTransform.m_centerOfMass1;
			compoundAabb.m_max = motionState.m_sweptTransform.m_centerOfMass1;
			hkAabb compoundExpandedAabb = compoundAabb;

			hkAabbUint32* aabbOut = childAabbs;
			uint32_t* keyOut = childKeys;
			for (uint32_t key = container->getFirstKey(); key != HK_INVALID_SHAPE_KEY;
				key = container->getNextKey(key)) {
				alignas(16) char shapeBuffer[512];
				const hkpShape* childShape = container->getChildShape(key, shapeBuffer);
				hkAabb childAabb;
				childShape->getAabbImpl(motionState.m_transform, halfTolerance, childAabb);

				// EXE-INLINED none: hkAabbUtil::sweepOffsetAabb
				const __m128 half = _mm_set1_ps(0.5f);
				const __m128 inMin = childAabb.m_min.m_quad;
				const __m128 inMax = childAabb.m_max.m_quad;
				__m128 halfSize = _mm_mul_ps(_mm_sub_ps(inMax, inMin), half);
				const __m128 center =
					_mm_add_ps(_mm_mul_ps(_mm_sub_ps(hkQuadReal1111_0, half), inMax), _mm_mul_ps(half, inMin));
				const hkTransform& endInv = rootMotion.m_inverse;
				const __m128 arm =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(center, center, 0x55),
											  endInv.m_rotation.m_col1.m_quad),
								   _mm_mul_ps(_mm_shuffle_ps(center, center, 0x00), endInv.m_rotation.m_col0.m_quad)),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(center, center, 0xAA), endInv.m_rotation.m_col2.m_quad),
							endInv.m_translation.m_quad));
				const hkTransform& start = rootMotion.m_transform;
				const __m128 startPoint =
					_mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(arm, arm, 0x55), start.m_rotation.m_col1.m_quad),
								   _mm_mul_ps(_mm_shuffle_ps(arm, arm, 0x00), start.m_rotation.m_col0.m_quad)),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(arm, arm, 0xAA), start.m_rotation.m_col2.m_quad),
							start.m_translation.m_quad));
				__m128 sweptMin = _mm_min_ps(center, startPoint);
				__m128 sweptMax = _mm_max_ps(center, startPoint);
				const __m128 centerOfMassLocal = rootMotion.m_motionState->m_sweptTransform.m_centerOfMassLocal.m_quad;
				for (int32_t s = 0; s < rootMotion.m_numSamples; s++) {
					const hkTransform& sample = rootMotion.m_samples[s];
					const __m128 trans = sample.m_translation.m_quad;
					const __m128 extendedArm =
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(trans, trans, 0xFF), _mm_sub_ps(arm, centerOfMassLocal)),
							centerOfMassLocal);
					const __m128 p = _mm_add_ps(_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(extendedArm, extendedArm, 0x55),
															   sample.m_rotation.m_col1.m_quad),
													_mm_mul_ps(_mm_shuffle_ps(extendedArm, extendedArm, 0x00),
														sample.m_rotation.m_col0.m_quad)),
						_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(extendedArm, extendedArm, 0xAA),
									   sample.m_rotation.m_col2.m_quad),
							trans));
					sweptMin = _mm_min_ps(sweptMin, p);
					sweptMax = _mm_max_ps(sweptMax, p);
				}
				// Square root of z + (y + x), summed in that order; only lane 0 of the _mm_add_ps results is used.
				const __m128 squared = _mm_mul_ps(halfSize, halfSize);
				const __m128 length = _mm_sqrt_ss(_mm_add_ps(_mm_shuffle_ps(squared, squared, 0xAA),
					_mm_add_ps(_mm_shuffle_ps(squared, squared, 0x55), squared)));
				const __m128 radius = _mm_shuffle_ps(length, length, 0x00);
				const __m128 deltaAngle = rootMotion.m_motionState->m_deltaAngle.m_quad;
				halfSize = _mm_min_ps(radius,
					_mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(deltaAngle, deltaAngle, 0xFF), radius), halfSize));
				hkAabb sweptAabb;
				sweptAabb.m_min.m_quad = _mm_min_ps(inMin, _mm_sub_ps(sweptMin, halfSize));
				sweptAabb.m_max.m_quad = _mm_max_ps(inMax, _mm_add_ps(sweptMax, halfSize));

				compoundAabb.m_min.setMin4(compoundAabb.m_min, childAabb.m_min);
				compoundAabb.m_max.setMax4(compoundAabb.m_max, childAabb.m_max);
				compoundExpandedAabb.m_min.setMin4(compoundExpandedAabb.m_min, sweptAabb.m_min);
				compoundExpandedAabb.m_max.setMax4(compoundExpandedAabb.m_max, sweptAabb.m_max);

				hkVector4Util::convertAabbToUint32(childAabb, info.m_bitOffsetLow, info.m_bitOffsetHigh,
					info.m_bitScale, *aabbOut);
				hkAabbUint32 expandedAabbInt;
				hkVector4Util::convertAabbToUint32(sweptAabb, info.m_bitOffsetLow, info.m_bitOffsetHigh,
					info.m_bitScale, expandedAabbInt);
				hkVector4Util::compressExpandedAabbUint32(expandedAabbInt, *aabbOut);
				aabbOut->m_shapeKeyByte = static_cast<uint8_t>(key);
				*keyOut = key;
				aabbOut++;
				keyOut++;
			}
			bvd.m_numChildShapeAabbs = static_cast<uint16_t>(keyOut - childKeys);

			// The collidable's own box. The SDK casts its bounding volume data, whose first 0x20 bytes are an
			// hkAabbUint32's.
			hkAabbUint32& bvdAabb = reinterpret_cast<hkAabbUint32&>(bvd);
			hkVector4Util::convertAabbToUint32(compoundAabb, info.m_bitOffsetLow, info.m_bitOffsetHigh, info.m_bitScale,
				bvdAabb);
			hkAabbUint32 compoundExpandedInt;
			hkVector4Util::convertAabbToUint32(compoundExpandedAabb, info.m_bitOffsetLow, info.m_bitOffsetHigh,
				info.m_bitScale, compoundExpandedInt);
			hkVector4Util::compressExpandedAabbUint32(compoundExpandedInt, bvdAabb);

			if (numChildShapes != 0) {
				// EXE-INLINED none: hkAllocateStack<hkValueIndexPair>
				hkStackMemory& pairStack = hkMemoryRouter::getInstance().m_stack;
				hkValueIndexPair* pairs = reinterpret_cast<hkValueIndexPair*>(pairStack.m_next.m_storage);
				pairStack.m_next.m_storage = reinterpret_cast<char*>(pairs) +
					((numChildShapes * static_cast<int32_t>(sizeof(hkValueIndexPair)) + 127) & ~127);
				if (numChildShapes > 1) {
					for (int32_t k = 0; k < numChildShapes; k++) {
						pairs[k].m_index = k;
						pairs[k].m_value = childAabbs[k].m_min[0];
					}
					hkAlgorithm::quickSortRecursive(pairs, 0, numChildShapes - 1, uint8_t(0));
				} else if (numChildShapes == 1) {
					pairs[0].m_index = 0;
				}
				hkAabbUint32* sortedAabbs = bvd.m_childShapeAabbs;
				uint32_t* sortedKeys = bvd.m_childShapeKeys;
				for (int32_t k = 0; k < numChildShapes; k++) {
					sortedAabbs[k] = childAabbs[pairs[k].m_index];
					sortedKeys[k] = childKeys[pairs[k].m_index];
				}
				// EXE-INLINED 0x5E0B60
				hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(pairs);
			}
			// EXE-INLINED 0x5E0B60
			hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(childKeys);
			// EXE-INLINED 0x5E0B60
			hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(childAabbs);
		} else {
			hkAabb aabb;
			shape->getAabbImpl(motionState.m_transform, halfTolerance, aabb);

			// EXE-INLINED none: hkAabbUtil::sweepAabb
			hkAabb sweptAabb;
			const float rotationalRadius = motionState.m_deltaAngle(3) * motionState.m_objectRadius;
			hkVector4 rotationalGain;
			rotationalGain.setAll(rotationalRadius);
			sweptAabb.m_min.setSub4(aabb.m_min, rotationalGain);
			sweptAabb.m_max.setAdd4(aabb.m_max, rotationalGain);
			const float objectRadius = motionState.m_objectRadius + halfTolerance;
			hkVector4 radius4;
			radius4.setAll(objectRadius);
			hkVector4 maxR;
			maxR.setAdd4(motionState.m_sweptTransform.m_centerOfMass1, radius4);
			hkVector4 minR;
			minR.setSub4(motionState.m_sweptTransform.m_centerOfMass1, radius4);
			sweptAabb.m_min.setMax4(sweptAabb.m_min, minR);
			sweptAabb.m_min.setMin4(sweptAabb.m_min, aabb.m_min);
			sweptAabb.m_max.setMin4(sweptAabb.m_max, maxR);
			sweptAabb.m_max.setMax4(sweptAabb.m_max, aabb.m_max);
			hkVector4 invPath;
			invPath.setSub4(motionState.m_sweptTransform.m_centerOfMass0, motionState.m_sweptTransform.m_centerOfMass1);
			hkVector4 zero;
			zero.setZero4();
			hkVector4 minPath;
			minPath.setMin4(zero, invPath);
			hkVector4 maxPath;
			maxPath.setMax4(zero, invPath);
			sweptAabb.m_min.add4(minPath);
			sweptAabb.m_max.add4(maxPath);

			hkAabbUint32& bvdAabb = reinterpret_cast<hkAabbUint32&>(bvd);
			hkVector4Util::convertAabbToUint32(aabb, info.m_bitOffsetLow, info.m_bitOffsetHigh, info.m_bitScale,
				bvdAabb);
			hkAabbUint32 expandedAabbInt;
			hkVector4Util::convertAabbToUint32(sweptAabb, info.m_bitOffsetLow, info.m_bitOffsetHigh, info.m_bitScale,
				expandedAabbInt);
			hkVector4Util::compressExpandedAabbUint32(expandedAabbInt, bvdAabb);
		}
	}
}
