#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapper.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnimationBinding.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4Util.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/Base/Monitor/hkMonitorStream.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"

#include <xmmintrin.h>

hkaSkeletonMapper::hkaSkeletonMapper(hkaSkeletonMapperData* a1) : m_mapping() {
    m_mapping = *a1;
}

// The mapping data goes with its member.
hkaSkeletonMapper::~hkaSkeletonMapper() {}

// Builds skeleton A's bone-to-track map from the binding, then skeleton B's tracks: the B bone of each simple mapping
// whose A bone is animated, and the B chain of each chain mapping whose A chain is wholly animated; B's bone-to-track
// map from those.
void hkaSkeletonMapper::initializeBindingMaps(
    const hkaAnimationBinding* binding,
    hkArray<int16_t, hkContainerHeapAllocator>& srcBoneToTrackIndicesOut,
    hkArray<int16_t, hkContainerHeapAllocator>& dstBoneToTrackIndicesOut,
    hkArray<int16_t, hkContainerHeapAllocator>& dstTrackToBoneIndicesOut
) {
    const hkaSkeleton* pSkeletonA = m_mapping.m_skeletonA;
    const hkaSkeleton* pSkeletonB = m_mapping.m_skeletonB;

    srcBoneToTrackIndicesOut.clear();
    {
        const int32_t numBones = pSkeletonA->m_bones.m_size;
        // EXE-INLINED none: hkArray<hkInt16>::setSize(int, const hkInt16&)
        hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
        const int32_t capacity = srcBoneToTrackIndicesOut.getCapacity();
        if (capacity < numBones) {
            hkArrayUtil::_reserve(
                rHeap, &srcBoneToTrackIndicesOut, numBones < 2 * capacity ? 2 * capacity : numBones, sizeof(int16_t)
            );
        }
        for (int32_t i = srcBoneToTrackIndicesOut.m_size; i < numBones; ++i) {
            srcBoneToTrackIndicesOut.m_data[i] = -1;
        }
        srcBoneToTrackIndicesOut.m_size = numBones;
    }
    for (int16_t t = 0; t < binding->m_transformTrackToBoneIndices.m_size; ++t) {
        srcBoneToTrackIndicesOut.m_data[binding->m_transformTrackToBoneIndices.m_data[t]] = t;
    }

    dstTrackToBoneIndicesOut.clear();
    dstBoneToTrackIndicesOut.clear();
    {
        const int32_t numBones = pSkeletonB->m_bones.m_size;
        // EXE-INLINED none: hkArray<hkInt16>::setSize(int, const hkInt16&)
        hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
        const int32_t capacity = dstBoneToTrackIndicesOut.getCapacity();
        if (capacity < numBones) {
            hkArrayUtil::_reserve(
                rHeap, &dstBoneToTrackIndicesOut, numBones < 2 * capacity ? 2 * capacity : numBones, sizeof(int16_t)
            );
        }
        for (int32_t i = dstBoneToTrackIndicesOut.m_size; i < numBones; ++i) {
            dstBoneToTrackIndicesOut.m_data[i] = -1;
        }
        dstBoneToTrackIndicesOut.m_size = numBones;
    }

    for (int16_t s = 0; s < m_mapping.m_simpleMappings.m_size; ++s) {
        const hkaSkeletonMapperData::SimpleMapping& rSimple = m_mapping.m_simpleMappings.m_data[s];
        if (srcBoneToTrackIndicesOut.m_data[rSimple.m_boneA] >= 0) {
            // EXE-INLINED none: hkArray<hkInt16>::pushBack
            const int16_t boneB = rSimple.m_boneB;
            hkMemoryAllocator& alloc = hkMemoryRouter::getInstance().heap();
            if (dstTrackToBoneIndicesOut.m_size == dstTrackToBoneIndicesOut.getCapacity()) {
                hkArrayUtil::_reserveMore(alloc, &dstTrackToBoneIndicesOut, sizeof(int16_t));
            }
            dstTrackToBoneIndicesOut.m_data[dstTrackToBoneIndicesOut.m_size] = boneB;
            dstTrackToBoneIndicesOut.m_size++;
        }
    }

    for (int16_t c = 0; c < m_mapping.m_chainMappings.m_size; ++c) {
        const hkaSkeletonMapperData::ChainMapping& rChain = m_mapping.m_chainMappings.m_data[c];
        // EXE-INLINED none: hkArray<hkInt16>::hkArray
        XFNV_INLINED_LOCAL(hkArray<int16_t>, bonesA, std::nullopt, std::nullopt);
        bonesA.m_data = nullptr;
        bonesA.m_size = 0;
        bonesA.m_capacityAndFlags = hkArrayBase<int16_t>::DONT_DEALLOCATE_FLAG;
        // EXE-INLINED none: hkArray<hkInt16>::hkArray
        XFNV_INLINED_LOCAL(hkArray<int16_t>, bonesB, std::nullopt, std::nullopt);
        bonesB.m_data = nullptr;
        bonesB.m_size = 0;
        bonesB.m_capacityAndFlags = hkArrayBase<int16_t>::DONT_DEALLOCATE_FLAG;
        hkBool foundA;
        hkBool foundB;
        if (hkaSkeletonUtils::getBoneChain(&foundA, *pSkeletonA, rChain.m_startBoneA, rChain.m_endBoneA, bonesA)
                ->m_bool &&
            hkaSkeletonUtils::getBoneChain(&foundB, *pSkeletonB, rChain.m_startBoneB, rChain.m_endBoneB, bonesB)
                ->m_bool) {
            bool allAnimated = true;
            for (int32_t i = 0; i < bonesA.m_size; ++i) {
                if (srcBoneToTrackIndicesOut.m_data[bonesA.m_data[i]] < 0) {
                    allAnimated = false;
                    break;
                }
            }
            if (allAnimated) {
                for (int32_t i = 0; i < bonesB.m_size; ++i) {
                    // EXE-INLINED none: hkArray<hkInt16>::pushBack
                    hkMemoryAllocator& alloc = hkMemoryRouter::getInstance().heap();
                    if (dstTrackToBoneIndicesOut.m_size == dstTrackToBoneIndicesOut.getCapacity()) {
                        hkArrayUtil::_reserveMore(alloc, &dstTrackToBoneIndicesOut, sizeof(int16_t));
                    }
                    dstTrackToBoneIndicesOut.m_data[dstTrackToBoneIndicesOut.m_size] = bonesB.m_data[i];
                    dstTrackToBoneIndicesOut.m_size++;
                }
            }
        }
        // EXE-INLINED none: hkArray<hkInt16>::~hkArray
        bonesB.clearAndDeallocate();
        // EXE-INLINED none: hkArray<hkInt16>::~hkArray
        bonesA.clearAndDeallocate();
    }

    {
        const int32_t numBones = pSkeletonB->m_bones.m_size;
        // EXE-INLINED none: hkArray<hkInt16>::setSize(int, const hkInt16&)
        hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
        const int32_t capacity = dstBoneToTrackIndicesOut.getCapacity();
        if (capacity < numBones) {
            hkArrayUtil::_reserve(
                rHeap, &dstBoneToTrackIndicesOut, numBones < 2 * capacity ? 2 * capacity : numBones, sizeof(int16_t)
            );
        }
        for (int32_t i = dstBoneToTrackIndicesOut.m_size; i < numBones; ++i) {
            dstBoneToTrackIndicesOut.m_data[i] = -1;
        }
        dstBoneToTrackIndicesOut.m_size = numBones;
    }
    for (int16_t t = 0; t < dstTrackToBoneIndicesOut.m_size; ++t) {
        dstBoneToTrackIndicesOut.m_data[dstTrackToBoneIndicesOut.m_data[t]] = t;
    }
}

// Each simple mapping writes poseB[m_boneB] = poseA[m_boneA] * m_aFromBTransform; each chain mapping maps its start
// bone the same way, collects skeleton B's bone chain from m_startBoneB to m_endBoneB, walks the chain's local
// transforms up into one translation, rotates the start bone by the shortest rotation from that translation's
// direction to the end transform's, and composes the rest of the chain from it. The bones with no mapping are then
// either copied from the original local pose (roots) or composed from their parent's mapped transform.
void hkaSkeletonMapper::mapPose(
    const hkaSkeletonMapper* apThis,
    const hkQsTransform* poseAModelSpace,
    const hkQsTransform* originalPoseBLocalSpace,
    hkQsTransform* poseBModelSpace,
    ConstraintSource source
) {
    HK_TIMER_BEGIN_LIST("MapPoseModelSpace", "init");
    HK_TIMER_SPLIT_LIST("simpleMappings");

    const hkaSkeletonMapperData& rMapping = apThis->m_mapping;
    for (int32_t i = 0; i < rMapping.m_simpleMappings.m_size; ++i) {
        const hkaSkeletonMapperData::SimpleMapping& rSimple = rMapping.m_simpleMappings.m_data[i];
        const hkQsTransform& rA = poseAModelSpace[rSimple.m_boneA];
        hkQsTransform& rB = poseBModelSpace[rSimple.m_boneB];
        _mm_prefetch(reinterpret_cast<const char*>(&rMapping.m_simpleMappings.m_data[i + 2]), _MM_HINT_NTA);
        // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
        hkVector4 kRotated;
        kRotated.setRotatedDir(rA.m_rotation, rSimple.m_aFromBTransform.m_translation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rB.m_translation.m_quad)[0],
            _mm_add_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&kRotated.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&rA.m_translation.m_quad)[0])
            )
        );
        rB.m_rotation.setMul(rA.m_rotation, rSimple.m_aFromBTransform.m_rotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rB.m_scale.m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&rA.m_scale.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&rSimple.m_aFromBTransform.m_scale.m_quad)[0])
            )
        );
        _mm_prefetch(reinterpret_cast<const char*>(&poseAModelSpace[rSimple.m_boneA + 2].m_scale), _MM_HINT_NTA);
        _mm_prefetch(reinterpret_cast<const char*>(&poseBModelSpace[rSimple.m_boneB + 2].m_scale), _MM_HINT_NTA);
    }

    HK_TIMER_SPLIT_LIST("chainMappings");

    for (int32_t c = 0; c < rMapping.m_chainMappings.m_size; ++c) {
        const hkaSkeletonMapperData::ChainMapping& rChain = rMapping.m_chainMappings.m_data[c];
        const int16_t iStartBoneA = rChain.m_startBoneA;
        const int16_t iStartBoneB = rChain.m_startBoneB;
        const int16_t iEndBoneB = rChain.m_endBoneB;
        const hkQsTransform& rStartA = poseAModelSpace[iStartBoneA];
        hkQsTransform& rStartB = poseBModelSpace[iStartBoneB];
        // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
        hkVector4 kRotated;
        kRotated.setRotatedDir(rStartA.m_rotation, rChain.m_startAFromBTransform.m_translation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rStartB.m_translation.m_quad)[0],
            _mm_add_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&kRotated.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&rStartA.m_translation.m_quad)[0])
            )
        );
        rStartB.m_rotation.setMul(rStartA.m_rotation, rChain.m_startAFromBTransform.m_rotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rStartB.m_scale.m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&rStartA.m_scale.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&rChain.m_startAFromBTransform.m_scale.m_quad)[0])
            )
        );

        // Skeleton B's bone chain, in a stack-allocated hkLocalArray of the start bone's address space.
        const int32_t iNumBones = static_cast<int32_t>(iEndBoneB) - static_cast<int32_t>(iStartBoneB) + 1;
        XFNV_INLINED_LOCAL(hkLocalArray<int16_t>, kBones, std::nullopt, std::nullopt);
        kBones.m_data = nullptr;
        kBones.m_size = 0;
        kBones.m_capacityAndFlags = hkArrayBase<int16_t>::DONT_DEALLOCATE_FLAG;
        // EXE-INLINED none: hkAllocateStack<int16_t>
        hkStackMemory& stack = hkMemoryRouter::getInstance().m_stack;
        char* pBlock = stack.m_next.m_storage;
        stack.m_next.m_storage = pBlock + ((2 * iNumBones + 127) & ~127);
        kBones.m_data = reinterpret_cast<int16_t*>(pBlock);
        kBones.m_localMemory = reinterpret_cast<int16_t*>(pBlock);
        kBones.m_capacityAndFlags = static_cast<uint32_t>(iNumBones) | hkArrayBase<int16_t>::DONT_DEALLOCATE_FLAG;
        hkBool kFound;
        hkaSkeletonUtils::getBoneChain(&kFound, *rMapping.m_skeletonB, iStartBoneB, iEndBoneB, kBones);

        // The end bone's local translation, walked up the chain to the bone after the start bone.
        hkVector4 kAccumulated;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kAccumulated.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&originalPoseBLocalSpace[iEndBoneB].m_translation.m_quad)[0])
        );
        for (int32_t i = kBones.m_size - 2; i >= 1; --i) {
            const hkQsTransform& rLocal = originalPoseBLocalSpace[kBones.m_data[i]];
            hkVector4 kScaled;
            _mm_store_ps(
                &reinterpret_cast<float*>(&kScaled.m_quad)[0],
                _mm_mul_ps(
                    _mm_load_ps(&reinterpret_cast<const float*>(&rLocal.m_scale.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<float*>(&kAccumulated.m_quad)[0])
                )
            );
            hkVector4 kRotatedUp;
            kRotatedUp.setRotatedDir(rLocal.m_rotation, kScaled);
            _mm_store_ps(
                &reinterpret_cast<float*>(&kAccumulated.m_quad)[0],
                _mm_add_ps(
                    _mm_load_ps(&reinterpret_cast<float*>(&kRotatedUp.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<const float*>(&rLocal.m_translation.m_quad)[0])
                )
            );
        }

        // The chain's start transform in B space and its end transform in A space.
        hkVector4 kStartInB;
        {
            hkVector4 kScaled;
            _mm_store_ps(
                &reinterpret_cast<float*>(&kScaled.m_quad)[0],
                _mm_mul_ps(
                    _mm_load_ps(&reinterpret_cast<float*>(&rStartB.m_scale.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<float*>(&kAccumulated.m_quad)[0])
                )
            );
            kStartInB.setRotatedDir(rStartB.m_rotation, kScaled);
        }
        const hkQsTransform& rEndA = poseAModelSpace[rChain.m_endBoneA];
        hkVector4 kRotatedEnd;
        kRotatedEnd.setRotatedDir(rEndA.m_rotation, rChain.m_endAFromBTransform.m_translation);
        hkVector4 kEndInA;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kEndInA.m_quad)[0],
            _mm_add_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&kRotatedEnd.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&rEndA.m_translation.m_quad)[0])
            )
        );
        // kEndRotation is computed but never read; the call is kept deliberately, as the game does.
        hkQuaternion kEndRotation;
        kEndRotation.setMul(rEndA.m_rotation, rChain.m_endAFromBTransform.m_rotation);

        // Adds the start bone's model space translation to twice the chain's start translation and then subtracts it
        // again; the two steps are kept deliberately, as the game does, since rounding makes them differ from the plain
        // sum.
        const __m128 kTranslationB = _mm_load_ps(&reinterpret_cast<float*>(&rStartB.m_translation.m_quad)[0]);
        hkVector4 kDirection;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kDirection.m_quad)[0],
            _mm_sub_ps(
                _mm_add_ps(
                    _mm_add_ps(
                        _mm_load_ps(&reinterpret_cast<float*>(&kStartInB.m_quad)[0]),
                        _mm_load_ps(&reinterpret_cast<float*>(&kStartInB.m_quad)[0])
                    ),
                    kTranslationB
                ),
                kTranslationB
            )
        );
        hkVector4 kUp;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kUp.m_quad)[0],
            _mm_sub_ps(_mm_load_ps(&reinterpret_cast<float*>(&kEndInA.m_quad)[0]), kTranslationB)
        );
        hkVector4Util::normalizeVectors2(kDirection, kUp);
        hkQuaternion kDirectionRotation;
        kDirectionRotation.setShortestRotation(kDirection, kUp);
        hkQuaternion kStartRotation;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kStartRotation.m_vec.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&rStartB.m_rotation.m_vec.m_quad)[0])
        );
        hkQuaternion kTwisted;
        kTwisted.setMul(kDirectionRotation, kStartRotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rStartB.m_rotation.m_vec.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<float*>(&kTwisted.m_vec.m_quad)[0])
        );

        for (int32_t i = 1; i < kBones.m_size - 1; ++i) {
            const hkQsTransform& rParent = poseBModelSpace[kBones.m_data[i - 1]];
            const hkQsTransform& rLocal = originalPoseBLocalSpace[kBones.m_data[i]];
            hkQsTransform& rBone = poseBModelSpace[kBones.m_data[i]];
            // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
            hkVector4 kRotatedBone;
            kRotatedBone.setRotatedDir(rParent.m_rotation, rLocal.m_translation);
            _mm_store_ps(
                &reinterpret_cast<float*>(&rBone.m_translation.m_quad)[0],
                _mm_add_ps(
                    _mm_load_ps(&reinterpret_cast<float*>(&kRotatedBone.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_translation.m_quad)[0])
                )
            );
            rBone.m_rotation.setMul(rParent.m_rotation, rLocal.m_rotation);
            _mm_store_ps(
                &reinterpret_cast<float*>(&rBone.m_scale.m_quad)[0],
                _mm_mul_ps(
                    _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_scale.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<const float*>(&rLocal.m_scale.m_quad)[0])
                )
            );
        }

        kBones.~hkLocalArray<int16_t>();
    }

    HK_TIMER_SPLIT_LIST("unmappedBones");

    if (rMapping.m_keepUnmappedLocal.m_bool) {
        for (int32_t j = 0; j < rMapping.m_unmappedBones.m_size; ++j) {
            const int32_t iBone = rMapping.m_unmappedBones.m_data[j];
            const int16_t iParent = rMapping.m_skeletonB->m_parentIndices.m_data[iBone];
            if (iParent == -1) {
                _mm_store_ps(
                    &reinterpret_cast<float*>(&poseBModelSpace[iBone].m_translation.m_quad)[0],
                    _mm_load_ps(
                        &reinterpret_cast<const float*>(&originalPoseBLocalSpace[iBone].m_translation.m_quad)[0]
                    )
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&poseBModelSpace[iBone].m_rotation.m_vec.m_quad)[0],
                    _mm_load_ps(
                        &reinterpret_cast<const float*>(&originalPoseBLocalSpace[iBone].m_rotation.m_vec.m_quad)[0]
                    )
                );
                _mm_store_ps(
                    &reinterpret_cast<float*>(&poseBModelSpace[iBone].m_scale.m_quad)[0],
                    _mm_load_ps(&reinterpret_cast<const float*>(&originalPoseBLocalSpace[iBone].m_scale.m_quad)[0])
                );
            }
            else {
                const hkQsTransform& rParent = poseBModelSpace[iParent];
                const hkQsTransform& rLocal = originalPoseBLocalSpace[iBone];
                hkQsTransform& rBone = poseBModelSpace[iBone];
                // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line
                // call)
                hkVector4 kRotatedBone;
                kRotatedBone.setRotatedDir(rParent.m_rotation, rLocal.m_translation);
                _mm_store_ps(
                    &reinterpret_cast<float*>(&rBone.m_translation.m_quad)[0],
                    _mm_add_ps(
                        _mm_load_ps(&reinterpret_cast<float*>(&kRotatedBone.m_quad)[0]),
                        _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_translation.m_quad)[0])
                    )
                );
                rBone.m_rotation.setMul(rParent.m_rotation, rLocal.m_rotation);
                _mm_store_ps(
                    &reinterpret_cast<float*>(&rBone.m_scale.m_quad)[0],
                    _mm_mul_ps(
                        _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_scale.m_quad)[0]),
                        _mm_load_ps(&reinterpret_cast<const float*>(&rLocal.m_scale.m_quad)[0])
                    )
                );
            }
        }
    }

    HK_TIMER_SPLIT_LIST("enforceConstraints");

    if (source == REFERENCE_POSE) {
        hkaSkeletonUtils::enforceSkeletonConstraintsModelSpace(*rMapping.m_skeletonB, poseBModelSpace);
    }
    else if (source == CURRENT_POSE) {
        hkaSkeletonUtils::enforcePoseConstraintsModelSpace(
            *rMapping.m_skeletonB, originalPoseBLocalSpace, poseBModelSpace
        );
    }

    HK_TIMER_END_LIST();
}

// mapPose over poses: a ragdoll mapping maps poseAIn's synced model space pose into poseBInOut's model space pose, from
// poseBInOut's synced local space one; a retargeting mapping maps the local space poses.
void hkaSkeletonMapper::mapPose(
    const hkaSkeletonMapper* apThis,
    const hkaPose& poseAIn,
    hkaPose& poseBInOut,
    ConstraintSource source
) {
    const int32_t type = apThis->m_mapping.m_mappingType.m_storage;
    if (type == hkaSkeletonMapperData::HK_RAGDOLL_MAPPING) {
        const hkQsTransform* poseAModelSpace = poseAIn.getSyncedPoseModelSpace().m_data;
        const hkQsTransform* originalPoseBLocalSpace = poseBInOut.getSyncedPoseLocalSpace().m_data;
        hkQsTransform* poseBModelSpace = poseBInOut.accessSyncedPoseModelSpace().m_data;
        mapPose(apThis, poseAModelSpace, originalPoseBLocalSpace, poseBModelSpace, source);
    }
    else if (type == hkaSkeletonMapperData::HK_RETARGETING_MAPPING) {
        const hkQsTransform* poseALocalSpace = poseAIn.getSyncedPoseLocalSpace().m_data;
        hkQsTransform* poseBLocalSpace = poseBInOut.accessSyncedPoseLocalSpace().m_data;
        mapPoseLocalSpace(apThis, poseALocalSpace, poseBLocalSpace, false);
    }
}

// With an index array, apA[j] is copied to a local before apOut[j] is written, because apOut may be apA.
void hkaSkeletonMapper::multiplyPoses(
    const hkQsTransform* apA,
    const hkQsTransform* apB,
    const int16_t* apIndices,
    int32_t aiNumTransforms,
    hkQsTransform* apOut
) {
    if (!apIndices) {
        for (int32_t i = 0; i < aiNumTransforms; ++i) {
            // EXE-INLINED none: hkQsTransform::setMul
            hkVector4 kRotated;
            kRotated.setRotatedDir(apA[i].m_rotation, apB[i].m_translation);
            _mm_store_ps(
                &reinterpret_cast<float*>(&apOut[i].m_translation.m_quad)[0],
                _mm_add_ps(
                    _mm_load_ps(&reinterpret_cast<float*>(&kRotated.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<const float*>(&apA[i].m_translation.m_quad)[0])
                )
            );
            apOut[i].m_rotation.setMul(apA[i].m_rotation, apB[i].m_rotation);
            _mm_store_ps(
                &reinterpret_cast<float*>(&apOut[i].m_scale.m_quad)[0],
                _mm_mul_ps(
                    _mm_load_ps(&reinterpret_cast<const float*>(&apB[i].m_scale.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<const float*>(&apA[i].m_scale.m_quad)[0])
                )
            );
        }
        return;
    }
    for (int32_t i = 0; i < aiNumTransforms; ++i) {
        const int32_t j = apIndices[i];
        if (j < 0) {
            continue;
        }
        hkQsTransform kA;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kA.m_translation.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&apA[j].m_translation.m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&kA.m_rotation.m_vec.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&apA[j].m_rotation.m_vec.m_quad)[0])
        );
        _mm_store_ps(
            &reinterpret_cast<float*>(&kA.m_scale.m_quad)[0],
            _mm_load_ps(&reinterpret_cast<const float*>(&apA[j].m_scale.m_quad)[0])
        );
        // EXE-INLINED none: hkQsTransform::setMul
        hkVector4 kRotated;
        kRotated.setRotatedDir(kA.m_rotation, apB[i].m_translation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&apOut[j].m_translation.m_quad)[0],
            _mm_add_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&kRotated.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<float*>(&kA.m_translation.m_quad)[0])
            )
        );
        apOut[j].m_rotation.setMul(kA.m_rotation, apB[i].m_rotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&apOut[j].m_scale.m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&apB[i].m_scale.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<float*>(&kA.m_scale.m_quad)[0])
            )
        );
    }
}

void hkaSkeletonMapper::multiplyPosesThunk(
    const hkQsTransform* apA,
    const hkQsTransform* apB,
    const int16_t* apIndices,
    int32_t aiNumTransforms,
    hkQsTransform* apOut
) {
    hkaSkeletonMapper::multiplyPoses(apA, apB, apIndices, aiNumTransforms, apOut);
}

void hkaSkeletonMapper::mapPoseLocalSpace(
    const hkaSkeletonMapper* apThis,
    const hkQsTransform* poseALocalSpace,
    hkQsTransform* poseBLocalSpaceInOut,
    const int16_t* boneToTrackIndicesA,
    const int16_t* boneToTrackIndicesB,
    bool additive
) {
    HK_TIMER_BEGIN_LIST("MapPoseLocalSpace", "init");
    HK_TIMER_SPLIT_LIST("simpleMappings");

    const bool useMaps = boneToTrackIndicesA != nullptr && boneToTrackIndicesB != nullptr;
    const hkaSkeletonMapperData& rMapping = apThis->m_mapping;
    const int32_t numBonesA = rMapping.m_skeletonA->m_bones.m_size;
    // EXE-INLINED none: hkLocalBuffer<hkQsTransform>. The stack allocator's block is untyped memory, cast to the
    // transforms it holds.
    hkStackMemory& rStack = hkMemoryRouter::getInstance().m_stack;
    char* pBlock = rStack.m_next.m_storage;
    rStack.m_next.m_storage = pBlock + ((static_cast<int32_t>(sizeof(hkQsTransform)) * numBonesA + 127) & ~127);
    const hkQsTransform* poseA;
    if (additive) {
        hkQsTransform* pCombined = reinterpret_cast<hkQsTransform*>(pBlock);
        multiplyPosesThunk(
            poseALocalSpace, rMapping.m_skeletonA->m_referencePose.m_data, boneToTrackIndicesA,
            rMapping.m_skeletonA->m_bones.m_size, pCombined
        );
        poseA = pCombined;
    }
    else {
        poseA = poseALocalSpace;
    }

    const int32_t numSimple = rMapping.m_simpleMappings.m_size;
    for (int32_t i = 0; i < numSimple; ++i) {
        const hkaSkeletonMapperData::SimpleMapping& rSimple = rMapping.m_simpleMappings.m_data[i];
        int32_t boneB = rSimple.m_boneB;
        // A prefetch takes the address as untyped bytes.
        _mm_prefetch(reinterpret_cast<const char*>(&rMapping.m_simpleMappings.m_data[i + 2]), _MM_HINT_NTA);
        int32_t boneA = rSimple.m_boneA;
        if (useMaps) {
            boneA = boneToTrackIndicesA[boneA];
            if (boneA < 0) {
                continue;
            }
            boneB = boneToTrackIndicesB[boneB];
        }
        const hkQsTransform& rA = poseA[boneA];
        hkQsTransform& rB = poseBLocalSpaceInOut[boneB];
        setMulWithScaling(rMapping.m_mappingType, rB, rSimple.m_aFromBTransform, rA);
        _mm_prefetch(reinterpret_cast<const char*>(&(&rA)[2].m_scale), _MM_HINT_NTA);
        _mm_prefetch(reinterpret_cast<const char*>(&(&rB)[2].m_scale), _MM_HINT_NTA);
    }

    HK_TIMER_SPLIT_LIST("chainMappings");

    for (int32_t c = 0; c < rMapping.m_chainMappings.m_size; ++c) {
        const hkaSkeletonMapperData::ChainMapping& rChain = rMapping.m_chainMappings.m_data[c];

        // Skeleton A's chain: the end bone's position in the start bone's space and the chain's length.
        __m128 kLengthA = _mm_set_ss(0.0f);
        hkVector4 kDirectionB;
        kDirectionB.m_quad = _mm_setzero_ps();
        hkVector4 kDirectionA;
        kDirectionA.m_quad = _mm_setzero_ps();
        bool chainOk = true;
        int16_t boneA = rChain.m_endBoneA;
        if (boneA > rChain.m_startBoneA) {
            hkVector4 kPosition;
            kPosition.m_quad = _mm_setzero_ps();
            do {
                const int16_t index = useMaps ? boneToTrackIndicesA[boneA] : boneA;
                if (index < 0) {
                    chainOk = false;
                    break;
                }
                const hkQsTransform& rLocal = poseA[index];
                hkVector4 kRotated;
                kRotated.setRotatedDir(rLocal.m_rotation, kPosition);
                kPosition.m_quad = _mm_add_ps(kRotated.m_quad, rLocal.m_translation.m_quad);
                kLengthA = _mm_add_ss(kLengthA, rLocal.m_translation.length3().getQuad());
                boneA = rMapping.m_skeletonA->m_parentIndices.m_data[boneA];
            }
            while (boneA > rChain.m_startBoneA);
            kDirectionA = kPosition;
        }
        if (useMaps && (!chainOk || boneToTrackIndicesA[rChain.m_startBoneA] < 0)) {
            continue;
        }

        // Skeleton B's chain, its translations scaled to A's chain length.
        const __m128 kScale = _mm_mul_ps(
            _mm_shuffle_ps(kLengthA, kLengthA, _MM_SHUFFLE(0, 0, 0, 0)), rChain.m_endAFromBTransform.m_scale.m_quad
        );
        for (int16_t boneB = rChain.m_endBoneB; boneB > rChain.m_startBoneB;
             boneB = rMapping.m_skeletonB->m_parentIndices.m_data[boneB]) {
            const int16_t index = useMaps ? boneToTrackIndicesB[boneB] : boneB;
            hkQsTransform& rLocal = poseBLocalSpaceInOut[index];
            rLocal.m_translation.m_quad = _mm_mul_ps(kScale, rLocal.m_translation.m_quad);
            hkVector4 kRotated;
            kRotated.setRotatedDir(rLocal.m_rotation, kDirectionB);
            kDirectionB.m_quad = _mm_add_ps(kRotated.m_quad, rLocal.m_translation.m_quad);
        }

        int32_t startBoneA = rChain.m_startBoneA;
        int32_t endBoneA = rChain.m_endBoneA;
        int32_t startBoneB = rChain.m_startBoneB;
        int32_t endBoneB = rChain.m_endBoneB;
        if (useMaps) {
            startBoneA = boneToTrackIndicesA[startBoneA];
            endBoneA = boneToTrackIndicesA[endBoneA];
            endBoneB = boneToTrackIndicesB[endBoneB];
            startBoneB = boneToTrackIndicesB[startBoneB];
        }
        hkQsTransform& rStartB = poseBLocalSpaceInOut[startBoneB];
        setMulWithScaling(rMapping.m_mappingType, rStartB, rChain.m_startAFromBTransform, poseA[startBoneA]);

        // The start bone turned so that B's chain points where A's does.
        hkVector4Util::normalizeVectors2(kDirectionA, kDirectionB);
        hkQuaternion kTurn;
        kTurn.setShortestRotation(kDirectionB, kDirectionA);
        // Reads the end transform's translation here, not its rotation, as the game does.
        hkQuaternion kEnd;
        kEnd.m_vec = rChain.m_endAFromBTransform.m_translation;
        hkQuaternion kTurnInB;
        kTurnInB.setMulInverse(kTurn, kEnd);
        rStartB.m_rotation.mul(kTurnInB);
        poseBLocalSpaceInOut[endBoneB].m_rotation.setMul(
            rChain.m_endAFromBTransform.m_rotation, poseA[endBoneA].m_rotation
        );
    }

    if (additive) {
        const hkaSkeleton* pSkeletonB = rMapping.m_skeletonB;
        additivePoseFromCombinedPoseAndReferencePose(
            poseBLocalSpaceInOut, pSkeletonB->m_referencePose.m_data, boneToTrackIndicesB, pSkeletonB->m_bones.m_size,
            poseBLocalSpaceInOut
        );
    }

    HK_TIMER_END_LIST();
    // EXE-INLINED none: hkLocalBuffer<hkQsTransform>::~hkLocalBuffer
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBlock;
}

// mapPoseLocalSpace with no bone-to-track maps.
void hkaSkeletonMapper::mapPoseLocalSpace(
    const hkaSkeletonMapper* apThis,
    const hkQsTransform* poseALocalSpace,
    hkQsTransform* poseBLocalSpaceInOut,
    bool additive
) {
    mapPoseLocalSpace(apThis, poseALocalSpace, poseBLocalSpaceInOut, nullptr, nullptr, additive);
}

// Havok SDK: the extracted motion mapped by the mapping's own transform.
void hkaSkeletonMapper::mapExtractedMotion(
    const hkaSkeletonMapper* apThis,
    const hkQsTransform& deltaMotionIn,
    hkQsTransform& deltaMotionOut
) {
    setMulWithScaling(
        apThis->m_mapping.m_mappingType, deltaMotionOut, apThis->m_mapping.m_extractedMotionMapping, deltaMotionIn
    );
}

// Computes the inverses of the reference pose in a stack buffer (hkLocalBuffer, using hkQsTransform::setInverse), then
// multiplies each combined transform by its bone's inverse (multiplyPoses).
void hkaSkeletonMapper::additivePoseFromCombinedPoseAndReferencePose(
    const hkQsTransform* combinedPose,
    const hkQsTransform* referencePose,
    const int16_t* boneToTrackIndices,
    int32_t numBones,
    hkQsTransform* additivePoseOut
) {
    // EXE-INLINED none: hkLocalBuffer<hkQsTransform>. The stack allocator's block is untyped memory, cast to the
    // transforms it holds.
    hkStackMemory& rStack = hkMemoryRouter::getInstance().m_stack;
    char* pBlock = rStack.m_next.m_storage;
    rStack.m_next.m_storage = pBlock + ((static_cast<int32_t>(sizeof(hkQsTransform)) * numBones + 127) & ~127);
    hkQsTransform* pInverses = reinterpret_cast<hkQsTransform*>(pBlock);
    for (int32_t i = 0; i < numBones; ++i) {
        // EXE-INLINED none: hkQsTransform::setInverse, with hkVector4::setRotatedInverseDir (quaternion) written in
        // SSE as the exe computes it (0xCE2F80..0xCE2FF5): the header's lane-by-lane float version runs on the x87
        // and rounds denormal lanes twice, where mulps/addps round once.
        const hkQsTransform& rRef = referencePose[i];
        hkQsTransform& rInv = pInverses[i];
        const hkQuadReal q = rRef.m_rotation.m_vec.m_quad;
        const hkQuadReal v = rRef.m_translation.m_quad;
        const hkQuadReal p = _mm_mul_ps(v, q);
        const hkQuadReal dot = _mm_add_ss(
            _mm_shuffle_ps(p, p, _MM_SHUFFLE(2, 2, 2, 2)), _mm_add_ss(_mm_shuffle_ps(p, p, _MM_SHUFFLE(1, 1, 1, 1)), p)
        );
        const hkQuadReal cross = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 0, 2, 1)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 1, 0, 2))),
            _mm_mul_ps(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 1, 0, 2)), _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 0, 2, 1)))
        );
        const hkQuadReal w = _mm_shuffle_ps(q, q, _MM_SHUFFLE(3, 3, 3, 3));
        const hkQuadReal scaled = _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(w, w), hkQuadRealMinusHalf), v),
            _mm_mul_ps(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(0, 0, 0, 0)), q)
        );
        const hkQuadReal total = _mm_add_ps(_mm_mul_ps(cross, w), scaled);
        rInv.m_translation.m_quad = _mm_add_ps(total, total);
        rInv.m_translation.setNeg4(rInv.m_translation);
        rInv.m_rotation.setInverse(rRef.m_rotation);
        hkVector4 invScale;
        invScale.setReciprocal3(rRef.m_scale);
        rInv.m_scale.setXYZ0(invScale);
    }
    multiplyPoses(combinedPose, pInverses, boneToTrackIndices, numBones, additivePoseOut);
    // EXE-INLINED none: hkLocalBuffer<hkQsTransform>::~hkLocalBuffer
    hkMemoryRouter::getInstance().m_stack.m_next.m_storage = pBlock;
}

bool hkaSkeletonMapper::checkTransform(hkaSkeletonMapperData::MappingType type, const hkQsTransform& q) {
    hkBool v4;
    return q.m_rotation.isOk(&v4)->m_bool && (::new (&v4) hkBool(q.m_translation.isOk3()))->m_bool &&
        (::new (&v4) hkBool(q.m_scale.isOk3()))->m_bool != 0;
}

// For a ragdoll mapping, warns of each simple mapping onto a translation-locked bone of B whose transform moves it (a
// translation longer than 0.01), and of a skeleton B with no translation-locked bone.
void hkaSkeletonMapper::checkMapping(const hkaSkeletonMapperData& m_mapping) {
    XFNV_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
    alignas(16) char buf[512];
    if (m_mapping.m_mappingType.m_storage == hkaSkeletonMapperData::HK_RAGDOLL_MAPPING) {
        for (int32_t i = 0; i < m_mapping.m_simpleMappings.m_size; ++i) {
            const hkaSkeletonMapperData::SimpleMapping& rSimple = m_mapping.m_simpleMappings.m_data[i];
            const hkaBone& rBoneA = m_mapping.m_skeletonA->m_bones.m_data[rSimple.m_boneA];
            const hkaBone& rBoneB = m_mapping.m_skeletonB->m_bones.m_data[rSimple.m_boneB];
            if (rBoneB.m_lockTranslation.m_bool) {
                const float error = _mm_cvtss_f32(rSimple.m_aFromBTransform.m_translation.length3().getQuad());
                if (error > 0.01f) {
                    ::new (&kStream) hkErrStream(buf, 512);
                    kStream << "Bones A:" << rBoneA.m_name << " and B:" << rBoneB.m_name
                            << " are not fully aligned (error :" << error << ").";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabbaefbf), buf,
                        ".\\Mapper\\hkaSkeletonMapper.cpp", 499
                    );
                    XFNV_DESTRUCT(hkOstream)(&kStream);
                }
            }
        }
    }
    if (m_mapping.m_mappingType.m_storage == hkaSkeletonMapperData::HK_RAGDOLL_MAPPING) {
        const hkaSkeleton* pSkeletonB = m_mapping.m_skeletonB;
        for (int32_t i = 0; i < pSkeletonB->m_bones.m_size; ++i) {
            if (pSkeletonB->m_bones.m_data[i].m_lockTranslation.m_bool) {
                return;
            }
        }
        ::new (&kStream) hkErrStream(buf, 512);
        kStream << "None of the bones in skeleton have translations locked - did you forget to lock translations ?.";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba77a5), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 521
        );
        XFNV_DESTRUCT(hkOstream)(&kStream);
    }
}

// Warns of each improper mapping transform, mapped bone outside its skeleton, empty chain, and bone of skeleton B
// mapped more than once (counted per bone in a stack array of skeleton B's size).
void hkaSkeletonMapper::checkMappingLocalSpace(const hkaSkeletonMapperData& m_mapping) {
    XFNV_INLINED_LOCAL(hkErrStream, kStream, std::nullopt, std::nullopt);
    char buf[512];
    const int32_t numBonesB = m_mapping.m_skeletonB->m_bones.m_size;
    // EXE-INLINED none: hkLocalArray<int>::hkLocalArray. The stack allocator's block is untyped memory, cast to the
    // counts it holds.
    XFNV_INLINED_LOCAL(hkLocalArray<int32_t>, boneCounts, std::nullopt, std::nullopt);
    hkStackMemory& rStack = hkMemoryRouter::getInstance().m_stack;
    char* pBlock = rStack.m_next.m_storage;
    rStack.m_next.m_storage = pBlock + ((static_cast<int32_t>(sizeof(int32_t)) * numBonesB + 127) & ~127);
    boneCounts.m_data = reinterpret_cast<int32_t*>(pBlock);
    boneCounts.m_localMemory = boneCounts.m_data;
    boneCounts.m_capacityAndFlags = numBonesB | hkArrayBase<int32_t>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<int>::setSize(int, const int&)
    for (int32_t i = 0; i < numBonesB; ++i) {
        boneCounts.m_data[i] = 0;
    }
    boneCounts.m_size = numBonesB;

    for (int32_t i = 0; i < m_mapping.m_simpleMappings.m_size; ++i) {
        const hkaSkeletonMapperData::SimpleMapping& rSimple = m_mapping.m_simpleMappings.m_data[i];
        if (!checkTransform(m_mapping.m_mappingType, rSimple.m_aFromBTransform)) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Improper mapping transform found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabbabac1), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 551
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rSimple.m_boneA < 0 || rSimple.m_boneA >= m_mapping.m_skeletonA->m_bones.m_size) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid mapped bone found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba3f01), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 556
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rSimple.m_boneB < 0 || rSimple.m_boneB >= m_mapping.m_skeletonB->m_bones.m_size) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid mapped bone found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabbaeab2), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 561
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        ++boneCounts.m_data[rSimple.m_boneB];
        if (boneCounts.m_data[rSimple.m_boneB] > 1) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Improperly mapped bone found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba819f), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 569
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
    }

    for (int32_t c = 0; c < m_mapping.m_chainMappings.m_size; ++c) {
        const hkaSkeletonMapperData::ChainMapping& rChain = m_mapping.m_chainMappings.m_data[c];
        if (!checkTransform(m_mapping.m_mappingType, rChain.m_startAFromBTransform)) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Improper mapping transform found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabbabac1), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 582
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (!checkTransform(m_mapping.m_mappingType, rChain.m_endAFromBTransform)) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Improper mapping transform found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabbabac1), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 587
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rChain.m_endBoneA <= rChain.m_startBoneA) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid chain found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba819f), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 592
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rChain.m_endBoneB <= rChain.m_startBoneB) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid chain found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba819f), buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 597
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rChain.m_endBoneA < 0 || rChain.m_endBoneA >= m_mapping.m_skeletonA->m_bones.m_size) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid bone found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0x17626ee1, buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 602
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rChain.m_startBoneA < 0 || rChain.m_startBoneA >= m_mapping.m_skeletonA->m_bones.m_size) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid bone found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0x6073fdc, buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 607
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rChain.m_endBoneB < 0 || rChain.m_endBoneB >= m_mapping.m_skeletonB->m_bones.m_size) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid bone found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0x19a18bfd, buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 612
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        if (rChain.m_startBoneB < 0 || rChain.m_startBoneB >= m_mapping.m_skeletonB->m_bones.m_size) {
            ::new (&kStream) hkErrStream(buf, 512);
            kStream << "Invalid bone found";
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0xfb695b1, buf, ".\\Mapper\\hkaSkeletonMapper.cpp", 617
            );
            XFNV_DESTRUCT(hkOstream)(&kStream);
        }
        for (int32_t bone = rChain.m_endBoneB; bone >= rChain.m_startBoneB;
             bone = m_mapping.m_skeletonB->m_parentIndices.m_data[bone]) {
            ++boneCounts.m_data[bone];
            if (boneCounts.m_data[bone] > 1) {
                ::new (&kStream) hkErrStream(buf, 512);
                kStream << "Improperly mapped bone found. Bone \"" << m_mapping.m_skeletonB->m_bones.m_data[bone].m_name
                        << "\" has been mapped more than once.";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba819f), buf, ".\\Mapper\\hkaSkeletonMapper.cpp",
                    629
                );
                XFNV_DESTRUCT(hkOstream)(&kStream);
            }
        }
    }
    // EXE-INLINED none: hkLocalArray<int>::~hkLocalArray
    boneCounts.~hkLocalArray<int32_t>();
}

void hkaSkeletonMapper::setMulWithScaling(
    hkaSkeletonMapperData::MappingType type,
    hkQsTransform& dst,
    const hkQsTransform& map,
    const hkQsTransform& src
) {
    dst.m_translation.m_quad =
        _mm_add_ps(_mm_mul_ps(map.m_scale.m_quad, src.m_translation.m_quad), map.m_translation.m_quad);
    dst.m_rotation.setMul(map.m_rotation, src.m_rotation);
    dst.m_scale = src.m_scale;
}

hkaSkeletonMapperData::ChainMapping&
hkaSkeletonMapperData::ChainMapping::operator=(const hkaSkeletonMapperData::ChainMapping& other) {
    m_startBoneA = other.m_startBoneA;
    m_endBoneA = other.m_endBoneA;
    m_startBoneB = other.m_startBoneB;
    m_endBoneB = other.m_endBoneB;
    // EXE-INLINED 0xC6CD30
    m_startAFromBTransform.m_translation = other.m_startAFromBTransform.m_translation;
    m_startAFromBTransform.m_rotation.m_vec = other.m_startAFromBTransform.m_rotation.m_vec;
    m_startAFromBTransform.m_scale = other.m_startAFromBTransform.m_scale;
    // EXE-INLINED 0xC6CD30
    m_endAFromBTransform.m_translation = other.m_endAFromBTransform.m_translation;
    m_endAFromBTransform.m_rotation.m_vec = other.m_endAFromBTransform.m_rotation.m_vec;
    m_endAFromBTransform.m_scale = other.m_endAFromBTransform.m_scale;
    return *this;
}
