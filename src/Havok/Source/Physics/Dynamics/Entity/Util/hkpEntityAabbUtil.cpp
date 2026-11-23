#include "Havok/Source/Physics/Dynamics/Entity/Util/hkpEntityAabbUtil.h"

#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Math/hkMath.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

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
        }
        while (i <= j);
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
    for (i = 0; i < numEntities; p_m_boundingVolumeData->m_max[0] = 0) {
        p_m_boundingVolumeData = &entityBatch[i++]->m_collidable.m_boundingVolumeData;
        p_m_boundingVolumeData->m_min[0] = 1;
    }
}

// For each entity: its shape's aabb, grown by the motion over the step (the SDK's hkAabbUtil::sweepAabb), quantised
// into the collidable's bounding volume data with the grown box stored as expansion bytes. A shape with room for
// child shape aabbs (m_capacityChildShapeAabbs) gets one quantised box per child shape key instead, each grown
// along the root's sampled motion (hkAabbUtil::sweepOffsetAabb on initOffsetAabbInput's transforms); the union of the
// boxes is the collidable's own, and the child boxes and keys are stored sorted by their lower x.
void hkpEntityAabbUtil::entityBatchRecalcAabb(
    const hkpCollisionInput* collisionInput,
    hkpEntity* const* entityBatch,
    int32_t numEntities
) {
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

            hkAabbUint32* childAabbs = hkAllocateStack<hkAabbUint32>(numChildShapes, nullptr);
            uint32_t* childKeys = hkAllocateStack<uint32_t>(numChildShapes, nullptr);

            hkAabbUtil::OffsetAabbInput rootMotion;
            hkAabbUtil::initOffsetAabbInput(&motionState, rootMotion);

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

                hkAabb sweptAabb;
                hkAabbUtil::sweepOffsetAabb(rootMotion, childAabb, sweptAabb);

                compoundAabb.m_min.setMin4(compoundAabb.m_min, childAabb.m_min);
                compoundAabb.m_max.setMax4(compoundAabb.m_max, childAabb.m_max);
                compoundExpandedAabb.m_min.setMin4(compoundExpandedAabb.m_min, sweptAabb.m_min);
                compoundExpandedAabb.m_max.setMax4(compoundExpandedAabb.m_max, sweptAabb.m_max);

                hkVector4Util::convertAabbToUint32(
                    childAabb,
                    info.m_bitOffsetLow,
                    info.m_bitOffsetHigh,
                    info.m_bitScale,
                    *aabbOut
                );
                hkAabbUint32 expandedAabbInt;
                hkVector4Util::convertAabbToUint32(
                    sweptAabb,
                    info.m_bitOffsetLow,
                    info.m_bitOffsetHigh,
                    info.m_bitScale,
                    expandedAabbInt
                );
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
            hkVector4Util::convertAabbToUint32(
                compoundAabb,
                info.m_bitOffsetLow,
                info.m_bitOffsetHigh,
                info.m_bitScale,
                bvdAabb
            );
            hkAabbUint32 compoundExpandedInt;
            hkVector4Util::convertAabbToUint32(
                compoundExpandedAabb,
                info.m_bitOffsetLow,
                info.m_bitOffsetHigh,
                info.m_bitScale,
                compoundExpandedInt
            );
            hkVector4Util::compressExpandedAabbUint32(compoundExpandedInt, bvdAabb);

            if (numChildShapes != 0) {
                hkValueIndexPair* pairs = hkAllocateStack<hkValueIndexPair>(numChildShapes, nullptr);
                if (numChildShapes > 1) {
                    for (int32_t k = 0; k < numChildShapes; k++) {
                        pairs[k].m_index = k;
                        pairs[k].m_value = childAabbs[k].m_min[0];
                    }
                    hkAlgorithm::quickSortRecursive(pairs, 0, numChildShapes - 1, static_cast<uint8_t>(0));
                }
                else if (numChildShapes == 1) {
                    pairs[0].m_index = 0;
                }
                hkAabbUint32* sortedAabbs = bvd.m_childShapeAabbs;
                uint32_t* sortedKeys = bvd.m_childShapeKeys;
                for (int32_t k = 0; k < numChildShapes; k++) {
                    sortedAabbs[k] = childAabbs[pairs[k].m_index];
                    sortedKeys[k] = childKeys[pairs[k].m_index];
                }
                hkDeallocateStack(pairs);
            }
            hkDeallocateStack(childKeys);
            hkDeallocateStack(childAabbs);
        }
        else {
            hkAabb aabb;
            shape->getAabbImpl(motionState.m_transform, halfTolerance, aabb);

            hkAabb sweptAabb;
            hkAabbUtil::sweepAabb(&motionState, halfTolerance, aabb, sweptAabb);

            hkAabbUint32& bvdAabb = reinterpret_cast<hkAabbUint32&>(bvd);
            hkVector4Util::convertAabbToUint32(
                aabb,
                info.m_bitOffsetLow,
                info.m_bitOffsetHigh,
                info.m_bitScale,
                bvdAabb
            );
            hkAabbUint32 expandedAabbInt;
            hkVector4Util::convertAabbToUint32(
                sweptAabb,
                info.m_bitOffsetLow,
                info.m_bitOffsetHigh,
                info.m_bitScale,
                expandedAabbInt
            );
            hkVector4Util::compressExpandedAabbUint32(expandedAabbInt, bvdAabb);
        }
    }
}
