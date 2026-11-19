#include "Havok/Source/Physics/Collide/Agent3/CollectionCollection3/hkpCollectionCollectionAgent3.h"

#include "Havok/Source/Common/Base/Algorithm/Collide/1AxisSweep/hk1AxisSweep.h"
#include "Havok/Source/Common/Base/Algorithm/Sort/hkSort.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabbUtil.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent/ContactMgr/hkpContactMgr.h"
#include "Havok/Source/Physics/Collide/Agent/hkpCollisionQualityInfo.h"
#include "Havok/Source/Physics/Collide/Agent/hkpProcessCollisionInput.h"
#include "Havok/Source/Physics/Collide/Agent3/BvTree3/hkpBvTreeAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/List3/hkpListAgent3.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nMachine.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/1n/hkpAgent1nTrack.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3ProcessInput.h"
#include "Havok/Source/Physics/Collide/Agent3/hkpAgent3RootMotion.h"
#include "Havok/Source/Physics/Collide/Dispatch/hkpCollisionDispatcher.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/hkpBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeBuffer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintOwner.h"
#include "xfnv/Compiler.h"

#include <emmintrin.h>
#include <xmmintrin.h>

// The key of a quantised hit list entry is kept in the four bytes after the lowest corner, where an hkAabbUint32 holds
// its expansion bytes.
void hkpCollectionCollectionAgent3::setHitListKey(hkAabbUint32& arAabb, uint32_t auiKey) {
    arAabb.m_expansionMin[0] = static_cast<uint8_t>(auiKey);
    arAabb.m_expansionMin[1] = static_cast<uint8_t>(auiKey >> 8);
    arAabb.m_expansionMin[2] = static_cast<uint8_t>(auiKey >> 16);
    arAabb.m_expansionShift = static_cast<uint8_t>(auiKey >> 24);
}

// Packs a float box into the input's quantised AABB space: add the low bit offset to the lowest corner and the high one
// to the highest, scale, clamp to the largest quantised value and to zero, then round each lane to an integer. Each
// lane is rounded to nearest, even on a tie.
void hkpCollectionCollectionAgent3::quantiseAabb(
    const hkpCollisionInput& arInput,
    const hkAabb& arAabb,
    hkAabbUint32& arOut
) {
    const __m128 kOffset = _mm_load_ps(&reinterpret_cast<const float*>(&arInput.m_aabb32Info.m_bitOffsetLow.m_quad)[0]);
    const __m128 kOffsetHigh =
        _mm_load_ps(&reinterpret_cast<const float*>(&arInput.m_aabb32Info.m_bitOffsetHigh.m_quad)[0]);
    const __m128 kScale = _mm_load_ps(&reinterpret_cast<const float*>(&arInput.m_aabb32Info.m_bitScale.m_quad)[0]);
    const __m128 kMax = _mm_load_ps(&reinterpret_cast<float*>(&hkVector4Util::hkAabbUint32MaxVal)[0]);
    const __m128 kZero = _mm_load_ps(&reinterpret_cast<float*>(&hkVector4Zero.m_quad)[0]);
    const __m128 kMin = _mm_max_ps(
        _mm_min_ps(
            _mm_mul_ps(
                _mm_add_ps(kOffset, _mm_load_ps(&reinterpret_cast<const float*>(&arAabb.m_min.m_quad)[0])), kScale
            ),
            kMax
        ),
        kZero
    );
    const __m128 kMaxQuantised = _mm_max_ps(
        _mm_min_ps(
            _mm_mul_ps(
                _mm_add_ps(kOffsetHigh, _mm_load_ps(&reinterpret_cast<const float*>(&arAabb.m_max.m_quad)[0])), kScale
            ),
            kMax
        ),
        kZero
    );
    // The packed lanes are stored and converted one at a time: three for the lowest corner, four for the highest (its
    // fourth lane lands in the expansion bytes).
    alignas(16) float afMin[4];
    alignas(16) float afMax[4];
    _mm_store_ps(afMin, kMin);
    _mm_store_ps(afMax, kMaxQuantised);
    int32_t aiMin[3];
    int32_t aiMax[4];
    for (int32_t i = 0; i < 3; ++i) {
        xfnv::RoundToInt(aiMin[i], afMin[i]);
    }
    for (int32_t i = 0; i < 4; ++i) {
        xfnv::RoundToInt(aiMax[i], afMax[i]);
    }
    arOut.m_min[0] = static_cast<uint32_t>(aiMin[0]);
    arOut.m_min[1] = static_cast<uint32_t>(aiMin[1]);
    arOut.m_min[2] = static_cast<uint32_t>(aiMin[2]);
    arOut.m_max[0] = static_cast<uint32_t>(aiMax[0]);
    arOut.m_max[1] = static_cast<uint32_t>(aiMax[1]);
    arOut.m_max[2] = static_cast<uint32_t>(aiMax[2]);
    // The fourth lane lands in the expansion bytes, as the game does.
    arOut.m_expansionMax[0] = static_cast<uint8_t>(aiMax[3]);
    arOut.m_expansionMax[1] = static_cast<uint8_t>(aiMax[3] >> 8);
    arOut.m_expansionMax[2] = static_cast<uint8_t>(aiMax[3] >> 16);
    arOut.m_shapeKeyByte = static_cast<uint8_t>(aiMax[3] >> 24);
}

// Grows a child shape's box by the root body's motion over the step: the box is turned into the root's space by the
// inverse transform and back by the forward one, its corners are sampled along the step, and it is grown by the half
// extent, scaled by the root's turn.
void hkpCollectionCollectionAgent3::growAabbByRootMotion(const hkpAgent3RootMotion& arMotion, hkAabb& arAabb) {
    const __m128 kHalf = _mm_set1_ps(0.5f);
    const __m128 kMin = _mm_load_ps(&reinterpret_cast<float*>(&arAabb.m_min.m_quad)[0]);
    const __m128 kMax = _mm_load_ps(&reinterpret_cast<float*>(&arAabb.m_max.m_quad)[0]);
    const __m128 kHalfExtent = _mm_mul_ps(_mm_sub_ps(kMax, kMin), kHalf);
    const __m128 kMid = _mm_add_ps(
        _mm_mul_ps(_mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111_0)[0]), kHalf), kMax),
        _mm_mul_ps(kHalf, kMin)
    );

    const hkTransform& arToWorld = arMotion.m_inverse;
    const hkTransform& arToLocal = arMotion.m_transform;
    const __m128 kInvCol0 = _mm_load_ps(&reinterpret_cast<const float*>(&arToWorld.m_rotation.m_col0.m_quad)[0]);
    const __m128 kInvCol1 = _mm_load_ps(&reinterpret_cast<const float*>(&arToWorld.m_rotation.m_col1.m_quad)[0]);
    const __m128 kInvCol2 = _mm_load_ps(&reinterpret_cast<const float*>(&arToWorld.m_rotation.m_col2.m_quad)[0]);
    const __m128 kInvTrans = _mm_load_ps(&reinterpret_cast<const float*>(&arToWorld.m_translation.m_quad)[0]);
    const __m128 kInRoot = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kMid, kMid, 0x55), kInvCol1),
            _mm_mul_ps(_mm_shuffle_ps(kMid, kMid, 0x00), kInvCol0)
        ),
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kMid, kMid, 0xAA), kInvCol2), kInvTrans)
    );
    const __m128 kCol0 = _mm_load_ps(&reinterpret_cast<const float*>(&arToLocal.m_rotation.m_col0.m_quad)[0]);
    const __m128 kCol1 = _mm_load_ps(&reinterpret_cast<const float*>(&arToLocal.m_rotation.m_col1.m_quad)[0]);
    const __m128 kCol2 = _mm_load_ps(&reinterpret_cast<const float*>(&arToLocal.m_rotation.m_col2.m_quad)[0]);
    const __m128 kTrans = _mm_load_ps(&reinterpret_cast<const float*>(&arToLocal.m_translation.m_quad)[0]);
    const __m128 kInChild = _mm_add_ps(
        _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(kInRoot, kInRoot, 0x55), kCol1),
            _mm_mul_ps(_mm_shuffle_ps(kInRoot, kInRoot, 0x00), kCol0)
        ),
        _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kInRoot, kInRoot, 0xAA), kCol2), kTrans)
    );
    __m128 kLo = _mm_min_ps(kMid, kInChild);
    __m128 kHi = _mm_max_ps(kMid, kInChild);
    const __m128 kLocal = _mm_load_ps(
        &reinterpret_cast<const float*>(&arMotion.m_motionState->m_sweptTransform.m_centerOfMassLocal.m_quad)[0]
    );
    if (arMotion.m_numSamples > 0) {
        // Samples from the box's centre in the root's space, not the one turned back.
        const __m128 kDelta = _mm_sub_ps(kInRoot, kLocal);
        for (int32_t i = 0; i < arMotion.m_numSamples; ++i) {
            const hkTransform& arSample = arMotion.m_samples[i];
            const __m128 kSampleCol0 =
                _mm_load_ps(&reinterpret_cast<const float*>(&arSample.m_rotation.m_col0.m_quad)[0]);
            const __m128 kSampleCol1 =
                _mm_load_ps(&reinterpret_cast<const float*>(&arSample.m_rotation.m_col1.m_quad)[0]);
            const __m128 kSampleCol2 =
                _mm_load_ps(&reinterpret_cast<const float*>(&arSample.m_rotation.m_col2.m_quad)[0]);
            const __m128 kSampleTrans = _mm_load_ps(&reinterpret_cast<const float*>(&arSample.m_translation.m_quad)[0]);
            const __m128 kPoint =
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kSampleTrans, kSampleTrans, 0xFF), kDelta), kLocal);
            const __m128 kCorner = _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(kPoint, kPoint, 0x55), kSampleCol1),
                    _mm_mul_ps(_mm_shuffle_ps(kPoint, kPoint, 0x00), kSampleCol0)
                ),
                _mm_add_ps(_mm_mul_ps(_mm_shuffle_ps(kPoint, kPoint, 0xAA), kSampleCol2), kSampleTrans)
            );
            kLo = _mm_min_ps(kLo, kCorner);
            kHi = _mm_max_ps(kHi, kCorner);
        }
    }
    const __m128 kSquared = _mm_mul_ps(kHalfExtent, kHalfExtent);
    // The square root of (y + x) + z.
    const float fLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
        _mm_add_ss(_mm_shuffle_ps(kSquared, kSquared, 0x55), kSquared), _mm_shuffle_ps(kSquared, kSquared, 0xAA)
    )));
    const __m128 kLength = _mm_set1_ps(fLength);
    const __m128 kAngle = _mm_set1_ps(XFNV_M128_F32(arMotion.m_motionState->m_deltaAngle.m_quad, 3));
    const __m128 kGrow = _mm_min_ps(kLength, _mm_add_ps(_mm_mul_ps(kAngle, kLength), kHalfExtent));
    kLo = _mm_sub_ps(kLo, kGrow);
    kHi = _mm_add_ps(kHi, kGrow);
    _mm_store_ps(&reinterpret_cast<float*>(&arAabb.m_min.m_quad)[0], kLo);
    _mm_store_ps(&reinterpret_cast<float*>(&arAabb.m_max.m_quad)[0], kHi);
}

// One quantised AABB per shape key of the container: the child shape's box, grown by the root body's motion when the
// input runs continuous physics, then packed. It has no caller.
void hkpCollectionCollectionAgent3::calcAabbsOfKeys(
    const hkpAgent3ProcessInput& arInput,
    const hkpShapeContainer* apShapeContainer,
    const hkpCdBody& arBodyA,
    const uint32_t* apuiKeys,
    int32_t aiNumKeys,
    hkAabbUint32* apAabbsOut
) {
    hkpAgent3RootMotion kRootMotion;
    const int32_t bContinuous = arInput.m_input->m_collisionQualityInfo->m_useContinuousPhysics.m_storage;
    if (bContinuous) {
        const hkpCdBody* pRoot = &arBodyA;
        for (const hkpCdBody* pParent = arBodyA.m_parent; pParent != nullptr; pParent = pParent->m_parent) {
            pRoot = pParent;
        }
        hkpAgent3CalcRootMotion(pRoot->m_motion.pState, kRootMotion);
    }
    for (int32_t i = 0; i < aiNumKeys; ++i) {
        alignas(16) hkAabb kAabb;
        alignas(16) char acShapeBuffer[512];
        const uint32_t uiKey = apuiKeys[i];
        const hkpShape* pChild = apShapeContainer->getChildShape(uiKey, acShapeBuffer);
        pChild->getAabbImpl(*arBodyA.m_motion.pMotion, arInput.m_input->m_tolerance, kAabb);
        if (bContinuous) {
            growAabbByRootMotion(kRootMotion, kAabb);
        }
        quantiseAabb(*arInput.m_input, kAabb, apAabbsOut[i]);
        setHitListKey(apAabbsOut[i], uiKey);
    }
}

// The quantised child AABBs a root collidable caches in its bounding volume data (expanded and sorted by the sweep axis
// in the continuous case), or, for a root collidable with valid bounding volume data and an override body, its one box;
// otherwise one box per child shape of the container the body's shape exposes, grown by the root body's motion in the
// continuous case, and sorted.
int32_t hkpCollectionCollectionAgent3::calcAabbsOfCollidable(
    const hkpCollidable* apCollidable,
    const hkpCdBody* apOverrideBody,
    const hkpShapeContainer* apShapeContainer,
    const hkpProcessCollisionInput* apInput,
    hkAabbUint32* apAabbsOut
) {
    const bool bContinuous = apInput->m_collisionQualityInfo->m_useContinuousPhysics.m_storage != 0;
    const hkpCollidable::BoundingVolumeData& arVolumeData = apCollidable->m_boundingVolumeData;
    int32_t iNumAabbs = 0;
    bool bCached = false;
    if (apCollidable->m_parent == nullptr) {
        if (arVolumeData.m_childShapeAabbs != nullptr && arVolumeData.m_min[0] <= arVolumeData.m_max[0]) {
            if (arVolumeData.m_numChildShapeAabbs == 0) {
                return 0;
            }
            for (int32_t i = 0; i < arVolumeData.m_numChildShapeAabbs; ++i) {
                const hkAabbUint32& arChild = arVolumeData.m_childShapeAabbs[i];
                if (bContinuous) {
                    const uint8_t ucShift = arChild.m_expansionShift;
                    apAabbsOut[i].m_min[0] = arChild.m_min[0] - (arChild.m_expansionMin[0] << ucShift);
                    apAabbsOut[i].m_min[1] = arChild.m_min[1] - (arChild.m_expansionMin[1] << ucShift);
                    apAabbsOut[i].m_min[2] = arChild.m_min[2] - (arChild.m_expansionMin[2] << ucShift);
                    apAabbsOut[i].m_max[0] = arChild.m_max[0] + (arChild.m_expansionMax[0] << ucShift);
                    apAabbsOut[i].m_max[1] = arChild.m_max[1] + (arChild.m_expansionMax[1] << ucShift);
                    apAabbsOut[i].m_max[2] = arChild.m_max[2] + (arChild.m_expansionMax[2] << ucShift);
                }
                else {
                    hkString::memCpy16<32>(&apAabbsOut[i], &arChild);
                }
                ++iNumAabbs;
                setHitListKey(apAabbsOut[i], arVolumeData.m_childShapeKeys[i]);
            }
            if (!bContinuous) {
                return iNumAabbs;
            }
            bCached = true;
        }
        else if (arVolumeData.m_min[0] <= arVolumeData.m_max[0] && apOverrideBody != nullptr) {
            if (bContinuous) {
                const uint8_t ucShift = arVolumeData.m_expansionShift;
                apAabbsOut[0].m_min[0] = arVolumeData.m_min[0] - (arVolumeData.m_expansionMin[0] << ucShift);
                apAabbsOut[0].m_min[1] = arVolumeData.m_min[1] - (arVolumeData.m_expansionMin[1] << ucShift);
                apAabbsOut[0].m_min[2] = arVolumeData.m_min[2] - (arVolumeData.m_expansionMin[2] << ucShift);
                apAabbsOut[0].m_max[0] = arVolumeData.m_max[0] + (arVolumeData.m_expansionMax[0] << ucShift);
                apAabbsOut[0].m_max[1] = arVolumeData.m_max[1] + (arVolumeData.m_expansionMax[1] << ucShift);
                apAabbsOut[0].m_max[2] = arVolumeData.m_max[2] + (arVolumeData.m_expansionMax[2] << ucShift);
            }
            else {
                // The bounding volume data starts with the box, laid out as an hkAabbUint32.
                hkString::memCpy16<32>(&apAabbsOut[0], &arVolumeData);
            }
            setHitListKey(apAabbsOut[0], 0);
            return 1;
        }
    }
    if (!bCached) {
        hkpAgent3RootMotion kRootMotion;
        if (bContinuous) {
            const hkpCdBody* pRoot = apCollidable;
            for (const hkpCdBody* pParent = apCollidable->m_parent; pParent != nullptr; pParent = pParent->m_parent) {
                pRoot = pParent;
            }
            hkpAgent3CalcRootMotion(pRoot->m_motion.pState, kRootMotion);
        }
        alignas(16) hkAabb kAabb;
        alignas(16) char acShapeBuffer[512];
        for (uint32_t uiKey = apShapeContainer->getFirstKey(); uiKey != 0xFFFFFFFF;
             uiKey = apShapeContainer->getNextKey(uiKey)) {
            const hkpShape* pChild = apShapeContainer->getChildShape(uiKey, acShapeBuffer);
            pChild->getAabbImpl(*apCollidable->m_motion.pMotion, apInput->m_tolerance, kAabb);
            if (bContinuous) {
                growAabbByRootMotion(kRootMotion, kAabb);
            }
            quantiseAabb(*apInput, kAabb, apAabbsOut[iNumAabbs]);
            setHitListKey(apAabbsOut[iNumAabbs], uiKey);
            ++iNumAabbs;
        }
    }
    if (iNumAabbs > 1) {
        hkAlgorithm::quickSortRecursive(apAabbsOut, 0, iNumAabbs - 1, AabbUint32Less{0});
    }
    return iNumAabbs;
}

// When either body is not a bounding volume tree, A's children are boxed one by one; otherwise one body's AABB is
// queried against the other's tree (A's against B's when B's tree holds the list with fewer enabled children, when only
// A caches its child boxes, or when B is not the smaller), and the hits are boxed. The other body's children are boxed
// too, the pairs of overlapping boxes are found along x and sorted, and they run through the Nm machine, or with an
// override body through the 1n machine on B's keys. The box lists, the pair list and the key list live on the Havok
// stack; the stack is untyped memory, so each list is cast out of it and back. Each box list ends in four end markers.
hkpAgentData* hkpCollectionCollectionAgent3::process(
    const hkpAgent3ProcessInput& input,
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    hkVector4* separatingNormal,
    hkpProcessCollisionOutput& output
) {
    HK_TIMER_BEGIN_LIST("CollColl3", "Init");

    const hkpCdBody* pBodyA = input.m_bodyA;
    const hkpCdBody* pBodyB = input.m_bodyB;
    const hkpCollisionDispatcher* pDispatcher = input.m_input->m_dispatcher;
    const bool bANotTree =
        (pDispatcher->m_hasAlternateType[pBodyA->m_shape->m_type.m_storage] & (1u << HK_SHAPE_BV_TREE)) == 0;
    const bool bBNotTree =
        (pDispatcher->m_hasAlternateType[pBodyB->m_shape->m_type.m_storage] & (1u << HK_SHAPE_BV_TREE)) == 0;

    // Whether A's AABB is queried against B's tree (else B's against A's).
    bool bQueryTreeB;
    if (bANotTree || bBNotTree) {
        bQueryTreeB = bANotTree;
    }
    else {
        // A root body's collidable may cache its child boxes in its bounding volume data.
        const bool bACached = pBodyA->m_parent == nullptr &&
            static_cast<const hkpCollidable*>(pBodyA)->m_boundingVolumeData.m_childShapeAabbs != nullptr;
        const bool bBCached = pBodyB->m_parent == nullptr &&
            static_cast<const hkpCollidable*>(pBodyB)->m_boundingVolumeData.m_childShapeAabbs != nullptr;
        if (bACached != bBCached) {
            bQueryTreeB = bACached;
        }
        else {
            // Each call gets its own stack buffer, which the callee never reads.
            alignas(16) char acTreeBufferA[HK_SHAPE_BUFFER_SIZE];
            alignas(16) char acTreeBufferB[HK_SHAPE_BUFFER_SIZE];
            const hkpShapeCollection* pTreeChildA =
                hkBvTreeAgent3::getShapeCollectionIfBvTreeSupportsAabbQueries(pBodyA, acTreeBufferA);
            const hkpShapeCollection* pTreeChildB =
                hkBvTreeAgent3::getShapeCollectionIfBvTreeSupportsAabbQueries(input.m_bodyB, acTreeBufferB);
            const bool bAList = pTreeChildA != nullptr && pTreeChildA->m_type.m_storage == HK_SHAPE_LIST;
            const bool bBList = pTreeChildB != nullptr && pTreeChildB->m_type.m_storage == HK_SHAPE_LIST;
            if (bAList && bBList) {
                const hkpListShape* pListA = static_cast<const hkpListShape*>(pTreeChildA);
                const hkpListShape* pListB = static_cast<const hkpListShape*>(pTreeChildB);
                bQueryTreeB = pListA->m_childInfo.getSize() - pListA->m_numDisabledChildren <=
                    pListB->m_childInfo.getSize() - pListB->m_numDisabledChildren;
            }
            else if (bAList || bBList) {
                bQueryTreeB = bAList;
            }
            else {
                bQueryTreeB =
                    input.m_bodyB->m_motion.pState->m_objectRadius >= input.m_bodyA->m_motion.pState->m_objectRadius;
            }
        }
    }

    // Each getShapeContainerFrom call gets its own shape buffer, which the callee never reads.
    alignas(16) char acContainerBufferA[HK_SHAPE_BUFFER_SIZE];
    alignas(16) char acContainerBufferB[HK_SHAPE_BUFFER_SIZE];
    const hkpShapeContainer* pContainerA = hkBvTreeAgent3::getShapeContainerFrom(input.m_bodyA, acContainerBufferA);
    const hkpShapeContainer* pContainerB = hkBvTreeAgent3::getShapeContainerFrom(input.m_bodyB, acContainerBufferB);

    hkShapeKeyPair* pHitList = hkAllocateStack<hkShapeKeyPair>(HK_MAX_NUM_HITS_PER_AABB_QUERY, nullptr);

    // The body whose boxes are the first list, and the other body with its container.
    const hkpCdBody* pBody1;
    hk1AxisSweep::AabbInt* pAabbs1;
    int32_t iNum1;
    const hkpCdBody* pBody2;
    const hkpShapeContainer* pContainer2;
    if (bANotTree && bBNotTree) {
        int32_t iNumChildren = pContainerA->getNumChildShapes();
        const hkpCdBody* pA = input.m_bodyA;
        if (pA->m_parent == nullptr) {
            const hkpCollidable::BoundingVolumeData& rData =
                static_cast<const hkpCollidable*>(pA)->m_boundingVolumeData;
            if (rData.m_childShapeAabbs != nullptr && rData.m_min[0] <= rData.m_max[0] &&
                iNumChildren <= rData.m_numChildShapeAabbs) {
                iNumChildren = rData.m_numChildShapeAabbs;
            }
        }
        pAabbs1 = hkAllocateStack<hk1AxisSweep::AabbInt>(iNumChildren + 4, nullptr);
        iNum1 = calcAabbsOfCollidable(
            static_cast<const hkpCollidable*>(input.m_bodyA), input.m_overrideBodyA, pContainerA, input.m_input, pAabbs1
        );
        pBody1 = input.m_bodyA;
        pBody2 = input.m_bodyB;
        pContainer2 = pContainerB;
    }
    else {
        HK_TIMER_SPLIT_LIST("queryTree");
        alignas(16) uint32_t auiHits[HK_MAX_NUM_HITS_PER_AABB_QUERY];
        const hkpShapeContainer* pContainer1;
        if (bQueryTreeB) {
            hkTransform bTa;
            bTa.setInverse(input.m_aTb);
            iNum1 = hkBvTreeAgent3::calcAabbAndQueryTree(
                *input.m_bodyA, *input.m_bodyB, bTa, input.m_linearTimInfo, *input.m_input, nullptr, auiHits,
                HK_MAX_NUM_HITS_PER_AABB_QUERY
            );
            pBody1 = input.m_bodyB;
            pContainer1 = pContainerB;
            pBody2 = input.m_bodyA;
            pContainer2 = pContainerA;
        }
        else {
            // setNeg4, done by xor'ing with the sign mask.
            hkVector4 kNegLinearTimInfo;
            kNegLinearTimInfo.m_quad = _mm_xor_ps(input.m_linearTimInfo.m_quad, rdata_10C4C00);
            iNum1 = hkBvTreeAgent3::calcAabbAndQueryTree(
                *input.m_bodyB, *input.m_bodyA, input.m_aTb, kNegLinearTimInfo, *input.m_input, nullptr, auiHits,
                HK_MAX_NUM_HITS_PER_AABB_QUERY
            );
            pBody1 = input.m_bodyA;
            pContainer1 = pContainerA;
            pBody2 = input.m_bodyB;
            pContainer2 = pContainerB;
        }
        pAabbs1 = hkAllocateStack<hk1AxisSweep::AabbInt>(iNum1 + 4, nullptr);

        HK_TIMER_SPLIT_LIST("ConvertHitlist");
        const bool bContinuous = input.m_input->m_collisionQualityInfo->m_useContinuousPhysics.m_storage != 0;
        hkpAgent3RootMotion kRootMotion;
        if (bContinuous) {
            const hkpCdBody* pRoot = pBody1;
            for (const hkpCdBody* pParent = pBody1->m_parent; pParent != nullptr; pParent = pParent->m_parent) {
                pRoot = pParent;
            }
            hkpAgent3CalcRootMotion(pRoot->m_motion.pState, kRootMotion);
        }
        alignas(16) hkAabb kAabb;
        alignas(16) char acShapeBuffer[HK_SHAPE_BUFFER_SIZE];
        for (int32_t i = 0; i < iNum1; ++i) {
            const uint32_t uiKey = auiHits[i];
            const hkpShape* pChild = pContainer1->getChildShape(uiKey, acShapeBuffer);
            pChild->getAabbImpl(*pBody1->m_motion.pMotion, input.m_input->m_tolerance, kAabb);
            if (bContinuous) {
                growAabbByRootMotion(kRootMotion, kAabb);
            }
            quantiseAabb(*input.m_input, kAabb, pAabbs1[i]);
            setHitListKey(pAabbs1[i], uiKey);
        }

        HK_TIMER_SPLIT_LIST("SortHitList");
        if (iNum1 > 1) {
            hkAlgorithm::quickSortRecursive<hkAabbUint32, AabbUint32Less>(pAabbs1, 0, iNum1 - 1, AabbUint32Less{0});
        }
    }

    hk1AxisSweep::AabbInt* pAabbs2;
    int32_t iNum2;
    if (iNum1 != 0) {
        int32_t iNumChildren = pContainer2->getNumChildShapes();
        if (pBody2->m_parent == nullptr) {
            const hkpCollidable::BoundingVolumeData& rData =
                static_cast<const hkpCollidable*>(pBody2)->m_boundingVolumeData;
            if (rData.m_childShapeAabbs != nullptr && rData.m_min[0] <= rData.m_max[0] &&
                iNumChildren <= rData.m_numChildShapeAabbs) {
                iNumChildren = rData.m_numChildShapeAabbs;
            }
        }
        pAabbs2 = hkAllocateStack<hk1AxisSweep::AabbInt>(iNumChildren + 4, nullptr);
        iNum2 = calcAabbsOfCollidable(
            static_cast<const hkpCollidable*>(pBody2), nullptr, pContainer2, input.m_input, pAabbs2
        );
    }
    else {
        iNum2 = 0;
        pAabbs2 = hkAllocateStack<hk1AxisSweep::AabbInt>(4, nullptr);
    }

    // The end markers: four boxes past each list whose lowest x is the largest value.
    for (int32_t i = iNum1; i < iNum1 + 4; ++i) {
        pAabbs1[i].m_min[0] = 0xFFFFFFFF;
    }
    for (int32_t i = iNum2; i < iNum2 + 4; ++i) {
        pAabbs2[i].m_min[0] = 0xFFFFFFFF;
    }

    // HK_TIMER_SPLIT_LIST("1-Axis") written out.
    hkMonitorStream& rAxisStream = hkMonitorStream::getInstance();
    if (rAxisStream.memoryAvailable()) {
        hkMonitorStream::TimerCommand* h = reinterpret_cast<hkMonitorStream::TimerCommand*>(rAxisStream.getEnd());
        h->m_commandAndMonitor = "St1-Axis";
        h->setTime();
        rAxisStream.setEnd(reinterpret_cast<char*>(h + 1));
    }
    hkPadSpu<int32_t> numPairsSkipped;
    numPairsSkipped = 0;
    // The pairs are written as hk1AxisSweep's key pairs into the same untyped stack block.
    hkKeyPair* pPairsOut = reinterpret_cast<hkKeyPair*>(pHitList);
    int32_t iNumHits;
    if (pBody1 == input.m_bodyA) {
        iNumHits = hk1AxisSweep::collide(
            pAabbs1, iNum1, pAabbs2, iNum2, pPairsOut, HK_MAX_NUM_HITS_PER_AABB_QUERY - 1, numPairsSkipped
        );
    }
    else {
        iNumHits = hk1AxisSweep::collide(
            pAabbs2, iNum2, pAabbs1, iNum1, pPairsOut, HK_MAX_NUM_HITS_PER_AABB_QUERY - 1, numPairsSkipped
        );
    }
    hkDeallocateStack(pAabbs2);
    hkDeallocateStack(pAabbs1);

    HK_TIMER_SPLIT_LIST("SortHits");
    HK_MONITOR_ADD_VALUE("NumHits", static_cast<float>(iNumHits), HK_MONITOR_TYPE_INT);
    if (iNumHits > 1) {
        // The comparator argument is the stream's address with its low byte cleared (only the low half of it on x64),
        // as the game does.
        hkAlgorithm::quickSortRecursive(
            pHitList, 0, iNumHits - 1, static_cast<int32_t>(reinterpret_cast<uintptr_t>(&rAxisStream) & 0xFFFFFF00u)
        );
    }
    pHitList[iNumHits].m_shapeKeyA = HK_INVALID_SHAPE_KEY;
    pHitList[iNumHits].m_shapeKeyB = HK_INVALID_SHAPE_KEY;
    // EXE-INLINED none: hkShrinkAllocatedStack<hkShapeKeyPair>
    if (iNumHits + 1 < HK_MAX_NUM_HITS_PER_AABB_QUERY) {
        hkMemoryRouter::getInstance().m_stack.m_next.m_storage = reinterpret_cast<char*>(pHitList) +
            (((iNumHits + 1) * sizeof(hkShapeKeyPair) + 127) & ~static_cast<size_t>(127));
    }

    HK_TIMER_SPLIT_LIST("Narrow");
    if (input.m_overrideBodyA == nullptr) {
        hkAgentNmMachine_Process(
            *static_cast<hkpAgent1nTrack*>(agentData), input, pContainerA, pContainerB, pHitList, output
        );
    }
    else {
        uint32_t* puiKeys = hkAllocateStack<uint32_t>(iNumHits + 1, nullptr);
        // B's keys, the end marker's included.
        for (int32_t i = 0; i <= iNumHits; ++i) {
            puiKeys[i] = pHitList[i].m_shapeKeyB;
        }
        hkpAgent3ProcessInput kInput = input;
        kInput.m_bodyA = input.m_overrideBodyA;
        kInput.m_overrideBodyA = nullptr;
        hkAgent1nMachine_Process(*static_cast<hkpAgent1nTrack*>(agentData), kInput, pContainerB, puiKeys, output);
        hkDeallocateStack(puiKeys);
    }
    hkDeallocateStack(pHitList);
    HK_TIMER_END_LIST();
    return static_cast<char*>(agentData) + ((sizeof(hkpAgent1nTrack) + 15) & ~static_cast<size_t>(15));
}

// The SDK's initAgentFunc: the list agent's create and destroy, this agent's process and table functions.
void hkpCollectionCollectionAgent3::initAgentFunc(hkpCollisionDispatcher::Agent3Funcs& f) {
    f.m_createFunc = &hkListAgent3::create;
    f.m_processFunc = &hkpCollectionCollectionAgent3::process;
    f.m_sepNormalFunc = nullptr;
    f.m_cleanupFunc = nullptr;
    f.m_removePointFunc = nullptr;
    f.m_commitPotentialFunc = nullptr;
    f.m_createZombieFunc = nullptr;
    f.m_updateFilterFunc = &hkpCollectionCollectionAgent3::updateFilter;
    f.m_calcStatisticsFunc = &hkpCollectionCollectionAgent3::calcStatistics;
    f.m_invalidateTimFunc = &hkpCollectionCollectionAgent3::invalidateTim;
    f.m_warpTimeFunc = &hkpCollectionCollectionAgent3::warpTime;
    f.m_destroyFunc = &hkListAgent3::destroy;
    f.m_isPredictive.m_bool = 1;
}

void hkpCollectionCollectionAgent3::updateFilter(
    hkpAgentEntry* entry,
    hkpAgentData* agentData,
    const hkpCdBody& bodyA,
    const hkpCdBody& bodyB,
    const hkpCollisionInput& input,
    hkpContactMgr* mgr,
    hkpConstraintOwner& constraintOwner
) {
    hkpAgent1nMachine_VisitorInput vin;
    const hkpShape* m_shape = bodyA.m_shape;
    vin.m_bodyA = &bodyA;
    vin.m_input = &input;
    const hkpShape* m_shape_1 = bodyB.m_shape;
    vin.m_collectionBodyB = &bodyB;
    vin.m_constraintOwner = &constraintOwner;
    vin.m_contactMgr = mgr;
    vin.m_containerShapeA = m_shape->getContainer();
    vin.m_containerShapeB = m_shape_1->getContainer();
    hkAgentNmMachine_UpdateShapeCollectionFilter(*static_cast<hkpAgent1nTrack*>(agentData), vin);
}

// The Agent3Funcs constructor clears the flags and the five optional functions from m_updateFilterFunc on;
// initAgentFunc fills the rest. registerAgent3 copies the whole of f, so the padding bytes after m_reusePreviousEntry,
// never written here, land in the dispatcher's table, as the game does.
void hkpCollectionCollectionAgent3::registerAgent3(hkpCollisionDispatcher* dispatcher) {
    alignas(16) hkpCollisionDispatcher::Agent3Funcs f;
    f.m_isPredictive.m_bool = 0;
    f.m_ignoreSymmetricVersion.m_bool = 0;
    f.m_reusePreviousEntry.m_bool = 0;
    f.m_updateFilterFunc = nullptr;
    f.m_calcStatisticsFunc = nullptr;
    f.m_invalidateTimFunc = nullptr;
    f.m_warpTimeFunc = nullptr;
    f.m_sepNormalFunc = nullptr;
    hkpCollectionCollectionAgent3::initAgentFunc(f);
    dispatcher->registerAgent3(f, HK_SHAPE_BV_TREE, HK_SHAPE_BV_TREE);
    f.m_ignoreSymmetricVersion.m_bool = 1;
    f.m_reusePreviousEntry.m_bool = 1;
    dispatcher->registerAgent3(f, HK_SHAPE_COLLECTION, HK_SHAPE_BV_TREE);
    dispatcher->registerAgent3(f, HK_SHAPE_BV_TREE, HK_SHAPE_COLLECTION);
    dispatcher->registerAgent3(f, HK_SHAPE_COLLECTION, HK_SHAPE_COLLECTION);
    dispatcher->m_midphaseAgent3Registered.m_bool = 1;
    g_agentRegistered = 1;
}

// The left index climbs past the pairs below the middle one and the right index drops past those above it; the
// recursion takes the left part, the loop the right one.
int32_t hkAlgorithm::quickSortRecursive(hkShapeKeyPair* apArr, int32_t aiLow, int32_t aiHigh, int32_t aiLess) {
    hkAlgorithm::less<hkShapeKeyPair> cmpLess;
    int32_t d = aiLow;
    for (;;) {
        int32_t i = d;
        int32_t j = aiHigh;
        const hkShapeKeyPair str = apArr[(d + j) >> 1];
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
                hkShapeKeyPair tmp = apArr[j];
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
