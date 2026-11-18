#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperUtils.h"

#include "Havok/Source/Animation/Animation/Animation/SplineCompressed/hkaSplineCompressedAnimation.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapper.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <bit>
#include <cmath>
#include <xmmintrin.h>

// The scale arithmetic is kept in extended precision from the lengths to each stored scale, so it is written as one
// double expression per scale.
void hkaSkeletonMapperUtils::ExtractedMotionParams::computeExtractedMotionTransforms(
    hkaSkeletonMapperData::MappingType type,
    const hkQsTransform* poseAMS,
    int32_t numBonesA,
    const hkQsTransform* poseBMS,
    int32_t numBonesB,
    hkQsTransform& aMotionOut,
    hkQsTransform& bMotionOut
) {
    aMotionOut.m_translation.setZero4();
    aMotionOut.m_rotation.m_vec.m_quad = hkQuadReal0001;
    aMotionOut.m_scale.setAll(1.0f);
    bMotionOut.m_translation.setZero4();
    bMotionOut.m_rotation.m_vec.m_quad = hkQuadReal0001;
    bMotionOut.m_scale.setAll(1.0f);
    float fScaleA;
    float fScaleB;
    if (m_mode.m_storage == HK_USE_BOUNDING_BOX_DIAGONAL || m_mode.m_storage == HK_USE_BONE_LENGTHS) {
        __m128 lengthA;
        __m128 lengthB;
        if (m_mode.m_storage == HK_USE_BOUNDING_BOX_DIAGONAL) {
            hkVector4 minA;
            hkVector4 maxA;
            hkVector4 minB;
            hkVector4 maxB;
            getPoseMinMax(poseAMS, numBonesA, minA, maxA);
            getPoseMinMax(poseBMS, numBonesB, minB, maxB);
            __m128 diagonalA = _mm_sub_ps(maxA.m_quad, minA.m_quad);
            diagonalA = _mm_mul_ps(diagonalA, diagonalA);
            __m128 diagonalB = _mm_sub_ps(maxB.m_quad, minB.m_quad);
            diagonalB = _mm_mul_ps(diagonalB, diagonalB);
            lengthA = _mm_sqrt_ss(_mm_add_ss(
                _mm_shuffle_ps(diagonalA, diagonalA, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(diagonalA, diagonalA, 0x55), diagonalA)
            ));
            lengthB = _mm_sqrt_ss(_mm_add_ss(
                _mm_shuffle_ps(diagonalB, diagonalB, 0xAA),
                _mm_add_ss(_mm_shuffle_ps(diagonalB, diagonalB, 0x55), diagonalB)
            ));
        }
        else {
            __m128 boneA = poseAMS[m_extractedMotionBoneA].m_translation.m_quad;
            boneA = _mm_mul_ps(boneA, boneA);
            lengthA = _mm_sqrt_ss(
                _mm_add_ss(_mm_shuffle_ps(boneA, boneA, 0xAA), _mm_add_ss(_mm_shuffle_ps(boneA, boneA, 0x55), boneA))
            );
            __m128 boneB = poseBMS[m_extractedMotionBoneB].m_translation.m_quad;
            boneB = _mm_mul_ps(boneB, boneB);
            lengthB = _mm_sqrt_ss(
                _mm_add_ss(_mm_shuffle_ps(boneB, boneB, 0xAA), _mm_add_ss(_mm_shuffle_ps(boneB, boneB, 0x55), boneB))
            );
        }
        const float fLengthA = _mm_cvtss_f32(lengthA);
        const float fLengthB = _mm_cvtss_f32(lengthB);
        fScaleA = static_cast<double>(fLengthB) / fLengthA * m_extractedMotionFactorB / m_extractedMotionFactorA;
        fScaleB = static_cast<double>(fLengthA) / fLengthB * m_extractedMotionFactorA / m_extractedMotionFactorB;
    }
    else {
        fScaleA = m_extractedMotionFactorB / m_extractedMotionFactorA;
        fScaleB = m_extractedMotionFactorA / m_extractedMotionFactorB;
    }
    aMotionOut.m_scale.setAll(fScaleA);
    bMotionOut.m_scale.setAll(fScaleB);
}

// The SDK's getPoseMinMax: the componentwise minimum and maximum of the pose's translations, starting from the
// largest and smallest floats.
void hkaSkeletonMapperUtils::ExtractedMotionParams::getPoseMinMax(
    const hkQsTransform* poseModelSpace,
    int32_t numBones,
    hkVector4& minOut,
    hkVector4& maxOut
) {
    minOut.setAll(rdata_104DA48);
    maxOut.setAll(rdata_104E148);
    for (int32_t i = 0; i < numBones; ++i) {
        minOut.m_quad = _mm_min_ps(minOut.m_quad, poseModelSpace[i].m_translation.m_quad);
        maxOut.m_quad = _mm_max_ps(maxOut.m_quad, poseModelSpace[i].m_translation.m_quad);
    }
}

// For a ragdoll mapping, mapOut = inverse(a) * b; for a retargeting mapping, the scale is the ratio of the bones'
// lengths (defaultScale when either is within tolerance), the rotation b * a^-1 and the translation b - scale * a. Each
// warning goes through its own hkErrStream over a shared 512-byte buffer.
void hkaSkeletonMapperUtils::computeMappingTransform(
    hkaSkeletonMapperData::MappingType type,
    const hkQsTransform& a,
    const hkQsTransform& b,
    hkQsTransform& mapOut,
    float tolerance,
    float defaultScale
) {
    alignas(16) char buf[512];
    if (type == hkaSkeletonMapperData::HK_RAGDOLL_MAPPING) {
        hkQuaternion v6;
        hkVector4 v16;
        hkQuaternion q0_;
        v6.m_vec.m_quad = a.m_rotation.m_vec.m_quad;
        __m128 v7 = _mm_mul_ps(a.m_translation.m_quad, v6.m_vec.m_quad);
        __m128 v8 = _mm_shuffle_ps(v7, v7, 170);
        v8[0] = v8[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
        __m128 v9 = _mm_shuffle_ps(v6.m_vec.m_quad, v6.m_vec.m_quad, 255);
        __m128 v10 = _mm_add_ps(
            _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(a.m_translation.m_quad, a.m_translation.m_quad, 201),
                        _mm_shuffle_ps(v6.m_vec.m_quad, v6.m_vec.m_quad, 210)
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(a.m_translation.m_quad, a.m_translation.m_quad, 210),
                        _mm_shuffle_ps(v6.m_vec.m_quad, v6.m_vec.m_quad, 201)
                    )
                ),
                v9
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v9, v9), hkQuadRealMinusHalf), a.m_translation.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), v6.m_vec.m_quad)
            )
        );
        __m128 v11 = _mm_xor_ps(_mm_add_ps(v10, v10), hkQuadRealNegateMask);
        __m128 m_quad = _mm_xor_ps(v6.m_vec.m_quad, rdata_10C4C00);
        __m128 v13 = _mm_shuffle_ps(a.m_scale.m_quad, _mm_unpackhi_ps(a.m_scale.m_quad, hkQuadReal1111), 196);
        __m128 v14 = _mm_rcp_ps(v13);
        __m128 v15 = _mm_and_ps(_mm_sub_ps(_mm_add_ps(v14, v14), _mm_mul_ps(_mm_mul_ps(v14, v13), v14)), rdata_10C4C10);
        v16.m_quad = b.m_translation.m_quad;
        __m128 v17 = _mm_mul_ps(v16.m_quad, m_quad);
        __m128 v18 = _mm_shuffle_ps(v17, v17, 170);
        v18[0] = v18[0] + (_mm_shuffle_ps(v17, v17, 85)[0] + v17[0]);
        __m128 v19 = _mm_shuffle_ps(m_quad, m_quad, 255);
        q0_.m_vec.m_quad = m_quad;
        __m128 v20 = _mm_add_ps(
            _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v16.m_quad, v16.m_quad, 210), _mm_shuffle_ps(m_quad, m_quad, 201)),
                    _mm_mul_ps(_mm_shuffle_ps(v16.m_quad, v16.m_quad, 201), _mm_shuffle_ps(m_quad, m_quad, 210))
                ),
                v19
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v19, v19), hkQuadRealMinusHalf), v16.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v18, v18, 0), m_quad)
            )
        );
        mapOut.m_translation.m_quad = _mm_add_ps(_mm_add_ps(v20, v20), v11);
        mapOut.m_rotation.setMul(q0_, b.m_rotation);
        mapOut.m_scale.m_quad = _mm_mul_ps(b.m_scale.m_quad, v15);
    }
    else if (type == hkaSkeletonMapperData::HK_RETARGETING_MAPPING) {
        if (tolerance < 0.0f) {
            alignas(16) hkErrStream kErrStream(buf, 512);
            kErrStream.operator<<("Tolerance must be positive");
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA39C3u), buf,
                ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 846
            );
        }
        __m128 lengthSqA = _mm_mul_ps(a.m_translation.m_quad, a.m_translation.m_quad);
        const float fLengthA = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
            _mm_shuffle_ps(lengthSqA, lengthSqA, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(lengthSqA, lengthSqA, 0x55), lengthSqA)
        )));
        __m128 lengthSqB = _mm_mul_ps(b.m_translation.m_quad, b.m_translation.m_quad);
        const float fLengthB = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
            _mm_shuffle_ps(lengthSqB, lengthSqB, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(lengthSqB, lengthSqB, 0x55), lengthSqB)
        )));
        const bool bZeroA = fLengthA <= tolerance;
        const bool bZeroB = fLengthB <= tolerance;
        float fScale;
        if (bZeroA || bZeroB) {
            if (bZeroA != bZeroB) {
                alignas(16) hkErrStream kErrStream(buf, 512);
                kErrStream.operator<<("Attempt to map a zero length bone to a non-zero length bone.  This may result "
                                      "in poor accuracy.");
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA2B94u), buf,
                    ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 860
                );
            }
            fScale = defaultScale;
        }
        else {
            fScale = fLengthB / fLengthA;
        }
        mapOut.m_scale.setAll(fScale);
        mapOut.m_rotation.setMulInverse(b.m_rotation, a.m_rotation);
        // The scale is negated and stored as a float before it is splatted.
        const float fNegScale = -fScale;
        mapOut.m_translation.m_quad = _mm_add_ps(
            _mm_mul_ps(_mm_shuffle_ps(_mm_set_ss(fNegScale), _mm_set_ss(fNegScale), 0), a.m_translation.m_quad),
            b.m_translation.m_quad
        );
    }
    else {
        alignas(16) hkErrStream kErrStream(buf, 512);
        kErrStream.operator<<("Unsupported mapping type");
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAEF91u), buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp",
            876
        );
    }
    if (!mapOut.isOk().m_bool) {
        alignas(16) hkErrStream kErrStream(buf, 512);
        kErrStream.operator<<("Mapping is invalid");
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA10FCu), buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp",
            881
        );
    }
}

// A simple or chain mapping is copied into its array member by member, a chain mapping through ChainMapping::operator=.
// The bone names are written with hkOstream::operator<<(const hkStringPtr&).
void hkaSkeletonMapperUtils::createMapping(
    const hkaSkeletonMapperUtils::Params& originalParams,
    hkaSkeletonMapperData& originalAToB,
    hkaSkeletonMapperData& originalBToA
) {
    char buf[512];
    hkError::s_instance->sectionBegin(0x54E4323E, "hkaSkeletonMapperUtils::createMapping");
    hkaSkeletonMapperUtils::Params params(originalParams);
    hkaSkeletonMapperData* pAToB;
    hkaSkeletonMapperData* pBToA;
    // A ragdoll mapping is built from the skeleton with fewer bones: the skeletons, the user mappings and the
    // outputs swap.
    if (params.m_mappingType.m_storage == hkaSkeletonMapperData::HK_RAGDOLL_MAPPING &&
        originalParams.m_skeletonA->m_bones.m_size > originalParams.m_skeletonB->m_bones.m_size) {
        params.m_skeletonA = originalParams.m_skeletonB;
        params.m_skeletonB = originalParams.m_skeletonA;
        {
            // EXE-INLINED none: hkArray<UserMapping>::operator=
            hkArray<UserMapping>& kDst = params.m_userMappingsAtoB;
            const hkArray<UserMapping>& kSrc = originalParams.m_userMappingsBtoA;
            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kDst);
            const int32_t iNewSize = kSrc.m_size;
            const int32_t iCopied = iNewSize > kDst.m_size ? kDst.m_size : iNewSize;
            kDst._reserve(alloc, iNewSize);
            for (int32_t i = 0; i < iCopied; ++i) {
                kDst.m_data[i] = kSrc.m_data[i];
            }
            for (int32_t i = iCopied; i < iNewSize; ++i) {
                kDst.m_data[i] = kSrc.m_data[i];
            }
            kDst.m_size = iNewSize;
        }
        {
            // EXE-INLINED none: hkArray<UserMapping>::operator=
            hkArray<UserMapping>& kDst = params.m_userMappingsBtoA;
            const hkArray<UserMapping>& kSrc = originalParams.m_userMappingsAtoB;
            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kDst);
            const int32_t iNewSize = kSrc.m_size;
            const int32_t iCopied = iNewSize > kDst.m_size ? kDst.m_size : iNewSize;
            kDst._reserve(alloc, iNewSize);
            for (int32_t i = 0; i < iCopied; ++i) {
                kDst.m_data[i] = kSrc.m_data[i];
            }
            for (int32_t i = iCopied; i < iNewSize; ++i) {
                kDst.m_data[i] = kSrc.m_data[i];
            }
            kDst.m_size = iNewSize;
        }
        pAToB = &originalBToA;
        pBToA = &originalAToB;
    }
    else {
        pAToB = &originalAToB;
        pBToA = &originalBToA;
    }
    {
        const int32_t numBonesA = params.m_skeletonA->m_bones.m_size;
        const int32_t numBonesB = params.m_skeletonB->m_bones.m_size;
        hkLocalArray<hkQsTransform> modelPoseA(numBonesA);
        modelPoseA.setSizeUnchecked(numBonesA);
        hkaSkeletonUtils::transformLocalPoseToModelPose(
            numBonesA, params.m_skeletonA->m_parentIndices.m_data, params.m_skeletonA->m_referencePose.m_data,
            modelPoseA.m_data
        );
        hkLocalArray<hkQsTransform> modelPoseB(numBonesB);
        modelPoseB.setSizeUnchecked(numBonesB);
        hkaSkeletonUtils::transformLocalPoseToModelPose(
            numBonesB, params.m_skeletonB->m_parentIndices.m_data, params.m_skeletonB->m_referencePose.m_data,
            modelPoseB.m_data
        );
        const hkaSkeletonMapperData::MappingType type =
            static_cast<hkaSkeletonMapperData::MappingType>(params.m_mappingType.m_storage);
        // A retargeting mapping compares the local reference poses, a ragdoll mapping the model space ones.
        const hkQsTransform* poseA = type != hkaSkeletonMapperData::HK_RAGDOLL_MAPPING
            ? params.m_skeletonA->m_referencePose.m_data
            : modelPoseA.m_data;
        const hkQsTransform* poseB = type != hkaSkeletonMapperData::HK_RAGDOLL_MAPPING
            ? params.m_skeletonB->m_referencePose.m_data
            : modelPoseB.m_data;
        // Per bone, the bone of the other skeleton it is mapped to: -1 unmapped, -2 inside a chain.
        hkLocalArray<int16_t> bonesAMapped(numBonesA);
        {
            // EXE-INLINED none: hkArray<hkInt16>::setSize(numBonesA, -1)
            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&bonesAMapped);
            bonesAMapped._reserve(alloc, numBonesA);
            for (int32_t i = bonesAMapped.m_size; i < numBonesA; ++i) {
                bonesAMapped.m_data[i] = -1;
            }
            bonesAMapped.m_size = numBonesA;
        }
        hkLocalArray<int16_t> bonesBMapped(numBonesB);
        {
            // EXE-INLINED none: hkArray<hkInt16>::setSize(numBonesB, -1)
            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&bonesBMapped);
            bonesBMapped._reserve(alloc, numBonesB);
            for (int32_t i = bonesBMapped.m_size; i < numBonesB; ++i) {
                bonesBMapped.m_data[i] = -1;
            }
            bonesBMapped.m_size = numBonesB;
        }
        pAToB->m_mappingType.m_storage = params.m_mappingType.m_storage;
        pBToA->m_mappingType.m_storage = params.m_mappingType.m_storage;
        // The SDK's hkRefPtr assignments: a reference on the new skeleton, then the old one's released.
        {
            const hkaSkeleton* pSkeleton = params.m_skeletonA;
            if (pSkeleton) {
                pSkeleton->addReference();
            }
            if (pAToB->m_skeletonA) {
                pAToB->m_skeletonA->removeReference();
            }
            pAToB->m_skeletonA = const_cast<hkaSkeleton*>(pSkeleton);
        }
        {
            const hkaSkeleton* pSkeleton = params.m_skeletonB;
            if (pSkeleton) {
                pSkeleton->addReference();
            }
            if (pAToB->m_skeletonB) {
                pAToB->m_skeletonB->removeReference();
            }
            pAToB->m_skeletonB = const_cast<hkaSkeleton*>(pSkeleton);
        }
        {
            const hkaSkeleton* pSkeleton = params.m_skeletonB;
            if (pSkeleton) {
                pSkeleton->addReference();
            }
            if (pBToA->m_skeletonA) {
                pBToA->m_skeletonA->removeReference();
            }
            pBToA->m_skeletonA = const_cast<hkaSkeleton*>(pSkeleton);
        }
        {
            const hkaSkeleton* pSkeleton = params.m_skeletonA;
            if (pSkeleton) {
                pSkeleton->addReference();
            }
            if (pBToA->m_skeletonB) {
                pBToA->m_skeletonB->removeReference();
            }
            pBToA->m_skeletonB = const_cast<hkaSkeleton*>(pSkeleton);
        }

        // The user's mappings from A to B.
        for (int32_t m = 0; m < params.m_userMappingsAtoB.m_size; ++m) {
            const UserMapping& kUser = params.m_userMappingsAtoB.m_data[m];
            const int16_t boneA =
                hkaSkeletonUtils::findBoneWithName(*params.m_skeletonA, kUser.m_boneIn, params.m_compareNames);
            const int16_t boneB =
                hkaSkeletonUtils::findBoneWithName(*params.m_skeletonB, kUser.m_boneOut, params.m_compareNames);
            if (boneA == -1 || boneB == -1) {
                hkErrStream kStream(buf, 512);
                kStream << "Couldn't find bones for mapping (" << kUser.m_boneIn << " - " << kUser.m_boneOut << ")";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C64u), buf,
                    ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 127
                );
                continue;
            }
            hkaSkeletonMapperData::SimpleMapping kMapping;
            kMapping.m_aFromBTransform.m_translation.setZero4();
            kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad = hkQuadReal0001;
            kMapping.m_aFromBTransform.m_scale.setAll(1.0f);
            kMapping.m_boneA = boneA;
            kMapping.m_boneB = boneB;
            computeMappingTransform(
                type, poseA, boneA, poseB, boneB, kMapping.m_aFromBTransform, params.m_positionMatchTolerance,
                params.m_rotationMatchTolerance
            );
            {
                // EXE-INLINED none: hkArray<SimpleMapping>::pushBack
                hkArray<hkaSkeletonMapperData::SimpleMapping>& kMappings = pAToB->m_simpleMappings;
                hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kMappings);
                if (kMappings.m_size == kMappings.getCapacity()) {
                    hkArrayUtil::_reserveMore(alloc, &kMappings, sizeof(hkaSkeletonMapperData::SimpleMapping));
                }
                hkaSkeletonMapperData::SimpleMapping* pNew = kMappings.m_data + kMappings.m_size;
                if (pNew) {
                    pNew->m_boneA = kMapping.m_boneA;
                    pNew->m_boneB = kMapping.m_boneB;
                    pNew->m_aFromBTransform.m_translation.m_quad = kMapping.m_aFromBTransform.m_translation.m_quad;
                    pNew->m_aFromBTransform.m_rotation.m_vec.m_quad =
                        kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad;
                    pNew->m_aFromBTransform.m_scale.m_quad = kMapping.m_aFromBTransform.m_scale.m_quad;
                }
                kMappings.m_size++;
            }
            bonesBMapped.m_data[boneB] = boneA;
        }

        // The user's mappings from B to A.
        for (int32_t m = 0; m < params.m_userMappingsBtoA.m_size; ++m) {
            const UserMapping& kUser = params.m_userMappingsBtoA.m_data[m];
            const int16_t boneB =
                hkaSkeletonUtils::findBoneWithName(*params.m_skeletonB, kUser.m_boneIn, params.m_compareNames);
            const int16_t boneA =
                hkaSkeletonUtils::findBoneWithName(*params.m_skeletonA, kUser.m_boneOut, params.m_compareNames);
            if (boneB == -1 || boneA == -1) {
                hkErrStream kStream(buf, 512);
                kStream << "Couldn't find bones for mapping (" << kUser.m_boneIn << " - " << kUser.m_boneOut << ")";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C64u), buf,
                    ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 158
                );
                continue;
            }
            hkaSkeletonMapperData::SimpleMapping kMapping;
            kMapping.m_aFromBTransform.m_translation.setZero4();
            kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad = hkQuadReal0001;
            kMapping.m_aFromBTransform.m_scale.setAll(1.0f);
            kMapping.m_boneA = boneB;
            kMapping.m_boneB = boneA;
            computeMappingTransform(type, poseB, boneB, poseA, boneA, kMapping.m_aFromBTransform, 1e-05f, 1.0f);
            {
                // EXE-INLINED none: hkArray<SimpleMapping>::pushBack
                hkArray<hkaSkeletonMapperData::SimpleMapping>& kMappings = pBToA->m_simpleMappings;
                hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kMappings);
                if (kMappings.m_size == kMappings.getCapacity()) {
                    hkArrayUtil::_reserveMore(alloc, &kMappings, sizeof(hkaSkeletonMapperData::SimpleMapping));
                }
                hkaSkeletonMapperData::SimpleMapping* pNew = kMappings.m_data + kMappings.m_size;
                if (pNew) {
                    pNew->m_boneA = kMapping.m_boneA;
                    pNew->m_boneB = kMapping.m_boneB;
                    pNew->m_aFromBTransform.m_translation.m_quad = kMapping.m_aFromBTransform.m_translation.m_quad;
                    pNew->m_aFromBTransform.m_rotation.m_vec.m_quad =
                        kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad;
                    pNew->m_aFromBTransform.m_scale.m_quad = kMapping.m_aFromBTransform.m_scale.m_quad;
                }
                kMappings.m_size++;
            }
            bonesAMapped.m_data[boneA] = boneB;
        }

        if (type != hkaSkeletonMapperData::HK_RAGDOLL_MAPPING) {
            // Retargeting: the user's chains come in pairs, A's chain then B's.
            const int32_t numChains = params.m_userChains.m_size / 2;
            for (int32_t c = 0; c < numChains; ++c) {
                const UserChain& kChainA = params.m_userChains.m_data[2 * c];
                const UserChain& kChainB = params.m_userChains.m_data[2 * c + 1];
                const int16_t startA =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonA, kChainA.m_start, params.m_compareNames);
                const int16_t endA =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonA, kChainA.m_end, params.m_compareNames);
                const int16_t startB =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonB, kChainB.m_start, params.m_compareNames);
                const int16_t endB =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonB, kChainB.m_end, params.m_compareNames);
                if (startA < 0 || endA < 0) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChainA.m_start << " - " << kChainA.m_end
                            << "). Start or end bone not found.";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAA938u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 325
                    );
                    continue;
                }
                if (startB < 0 || endB < 0) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChainB.m_start << " - " << kChainB.m_end
                            << "). Start or end bone not found.";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBAA938u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 331
                    );
                    continue;
                }
                hkArray<int16_t> chainA;
                hkArray<int16_t> chainB;
                hkBool foundA = hkaSkeletonUtils::getBoneChain(*params.m_skeletonA, startA, endA, chainA);
                hkBool foundB = hkaSkeletonUtils::getBoneChain(*params.m_skeletonB, startB, endB, chainB);
                if (!foundA.m_bool) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChainA.m_start << " - " << kChainA.m_end << ")";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C63u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 343
                    );
                }
                else if (!foundB.m_bool) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChainB.m_start << " - " << kChainB.m_end << ")";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C63u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 349
                    );
                }
                else if (chainA.m_size < 2) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChainA.m_start << " - " << kChainA.m_end << ")";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1D64u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 359
                    );
                }
                else if (chainB.m_size < 2) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChainB.m_start << " - " << kChainB.m_end << ")";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1D64u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 366
                    );
                }
                else {
                    for (int32_t i = 0; i < chainA.m_size; ++i) {
                        if (bonesAMapped.m_data[chainA.m_data[i]] != -1) {
                            hkErrStream kStream(buf, 512);
                            kStream << "Couldn't make a chain (" << kChainA.m_start << " - " << kChainA.m_end
                                    << ")  Perhaps a bone is mapped twice?";
                            hkError::s_instance->message(
                                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1D65u), buf,
                                ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 376
                            );
                        }
                    }
                    for (int32_t i = 0; i < chainB.m_size; ++i) {
                        if (bonesBMapped.m_data[chainB.m_data[i]] != -1) {
                            hkErrStream kStream(buf, 512);
                            kStream << "Couldn't make a chain (" << kChainB.m_start << " - " << kChainB.m_end
                                    << ")  Perhaps a bone is mapped twice?";
                            hkError::s_instance->message(
                                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1D66u), buf,
                                ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 385
                            );
                        }
                    }
                    {
                        hkErrStream kStream(buf, 512);
                        kStream << "Creating chains: " << "( " << kChainA.m_start << ", " << kChainA.m_end << " )   ( "
                                << kChainB.m_start << ", " << kChainB.m_end << " )";
                        hkError::s_instance->message(
                            hkError::MESSAGE_REPORT, -1, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 397
                        );
                    }
                    hkaSkeletonMapperData::ChainMapping kChainAToB;
                    kChainAToB.~ChainMapping();
                    ::new (&kChainAToB) hkaSkeletonMapperData::ChainMapping;
                    hkaSkeletonMapperData::ChainMapping kChainBToA;
                    kChainBToA.~ChainMapping();
                    ::new (&kChainBToA) hkaSkeletonMapperData::ChainMapping;
                    computeChainMapping(
                        type, params.m_skeletonA, poseA, startA, endA, params.m_skeletonB, poseB, startB, endB,
                        kChainAToB
                    );
                    computeChainMapping(
                        type, params.m_skeletonB, poseB, startB, endB, params.m_skeletonA, poseA, startA, endA,
                        kChainBToA
                    );
                    {
                        // EXE-INLINED none: hkArray<ChainMapping>::pushBack
                        hkArray<hkaSkeletonMapperData::ChainMapping>& kChains = pBToA->m_chainMappings;
                        hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kChains);
                        if (kChains.m_size == kChains.getCapacity()) {
                            hkArrayUtil::_reserveMore(alloc, &kChains, sizeof(hkaSkeletonMapperData::ChainMapping));
                        }
                        hkaSkeletonMapperData::ChainMapping* pNew = kChains.m_data + kChains.m_size;
                        if (pNew) {
                            *pNew = kChainBToA;
                        }
                        kChains.m_size++;
                    }
                    {
                        // EXE-INLINED none: hkArray<ChainMapping>::pushBack
                        hkArray<hkaSkeletonMapperData::ChainMapping>& kChains = pAToB->m_chainMappings;
                        hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kChains);
                        if (kChains.m_size == kChains.getCapacity()) {
                            hkArrayUtil::_reserveMore(alloc, &kChains, sizeof(hkaSkeletonMapperData::ChainMapping));
                        }
                        hkaSkeletonMapperData::ChainMapping* pNew = kChains.m_data + kChains.m_size;
                        if (pNew) {
                            *pNew = kChainAToB;
                        }
                        kChains.m_size++;
                    }
                    for (int32_t i = 0; i < chainA.m_size; ++i) {
                        bonesAMapped.m_data[chainA.m_data[i]] = -2;
                    }
                    for (int32_t i = 0; i < chainB.m_size; ++i) {
                        bonesBMapped.m_data[chainB.m_data[i]] = -2;
                    }
                }
            }
        }
        else {
            // Ragdoll: each user chain names its start and end bones in both skeletons; a bone missing from one is
            // taken from the other's mapping.
            for (int32_t c = 0; c < params.m_userChains.m_size; ++c) {
                const UserChain& kChain = params.m_userChains.m_data[c];
                int16_t startA =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonA, kChain.m_start, params.m_compareNames);
                int16_t endA =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonA, kChain.m_end, params.m_compareNames);
                int16_t startB =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonB, kChain.m_start, params.m_compareNames);
                int16_t endB =
                    hkaSkeletonUtils::findBoneWithName(*params.m_skeletonB, kChain.m_end, params.m_compareNames);
                if (startA < 0 && startB < 0) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChain.m_start << " - " << kChain.m_end
                            << "). Start bone not found in either skeleton.";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, 0x38521C64, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 185
                    );
                    continue;
                }
                if (endA < 0 && endB < 0) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChain.m_start << " - " << kChain.m_end
                            << "). End bone not found in either skeleton.";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, 0x38521C64, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 191
                    );
                    continue;
                }
                if (startA < 0) {
                    startA = bonesBMapped.m_data[startB];
                }
                if (endA < 0) {
                    endA = bonesBMapped.m_data[endB];
                }
                if (startB < 0) {
                    startB = bonesAMapped.m_data[startA];
                }
                if (endB < 0) {
                    endB = bonesAMapped.m_data[endA];
                }
                hkArray<int16_t> chainA;
                hkArray<int16_t> chainB;
                hkBool foundA = hkaSkeletonUtils::getBoneChain(*params.m_skeletonA, startA, endA, chainA);
                hkBool foundB = hkaSkeletonUtils::getBoneChain(*params.m_skeletonB, startB, endB, chainB);
                if (!foundA.m_bool || !foundB.m_bool) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChain.m_start << " - " << kChain.m_end << ")";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C63u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 210
                    );
                }
                else if (chainA.m_size < 2 || chainB.m_size < 2) {
                    hkErrStream kStream(buf, 512);
                    kStream << "Couldn't make a chain (" << kChain.m_start << " - " << kChain.m_end << ")";
                    hkError::s_instance->message(
                        hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C64u), buf,
                        ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 220
                    );
                }
                else {
                    for (int32_t i = 1; i < chainA.m_size - 1; ++i) {
                        if (bonesAMapped.m_data[chainA.m_data[i]] != -1) {
                            hkErrStream kStream(buf, 512);
                            kStream << "Couldn't make a chain (" << kChain.m_start << " - " << kChain.m_end << ")";
                            hkError::s_instance->message(
                                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C65u), buf,
                                ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 230
                            );
                        }
                    }
                    for (int32_t i = 1; i < chainB.m_size - 1; ++i) {
                        if (bonesBMapped.m_data[chainB.m_data[i]] != -1) {
                            hkErrStream kStream(buf, 512);
                            kStream << "Couldn't make a chain (" << kChain.m_start << " - " << kChain.m_end << ")";
                            hkError::s_instance->message(
                                hkError::MESSAGE_WARNING, static_cast<int32_t>(0xABBA1C66u), buf,
                                ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 239
                            );
                        }
                    }
                    {
                        hkErrStream kStream(buf, 512);
                        kStream << "Creating chain " << params.m_skeletonA->m_bones.m_data[startA].m_name << " - "
                                << params.m_skeletonA->m_bones.m_data[endA].m_name;
                        hkError::s_instance->message(
                            hkError::MESSAGE_REPORT, -1, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 249
                        );
                    }
                    hkaSkeletonMapperData::ChainMapping kChainMapping;
                    kChainMapping.m_startBoneA = startA;
                    kChainMapping.m_endBoneA = endA;
                    kChainMapping.m_startBoneB = startB;
                    kChainMapping.m_endBoneB = endB;
                    kChainMapping.m_endAFromBTransform.setMulInverseMul(
                        modelPoseA.m_data[endA], modelPoseB.m_data[endB]
                    );
                    kChainMapping.m_startAFromBTransform.setMulInverseMul(
                        modelPoseA.m_data[startA], modelPoseB.m_data[startB]
                    );
                    {
                        // EXE-INLINED none: hkArray<ChainMapping>::pushBack
                        hkArray<hkaSkeletonMapperData::ChainMapping>& kChains = pAToB->m_chainMappings;
                        hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kChains);
                        if (kChains.m_size == kChains.getCapacity()) {
                            hkArrayUtil::_reserveMore(alloc, &kChains, sizeof(hkaSkeletonMapperData::ChainMapping));
                        }
                        hkaSkeletonMapperData::ChainMapping* pNew = kChains.m_data + kChains.m_size;
                        if (pNew) {
                            *pNew = kChainMapping;
                        }
                        kChains.m_size++;
                    }
                    kChainMapping.m_startBoneA = startB;
                    kChainMapping.m_endBoneA = endB;
                    kChainMapping.m_startBoneB = startA;
                    kChainMapping.m_endBoneB = endA;
                    kChainMapping.m_endAFromBTransform.setMulInverseMul(
                        modelPoseB.m_data[endB], modelPoseA.m_data[endA]
                    );
                    kChainMapping.m_startAFromBTransform.setMulInverseMul(
                        modelPoseB.m_data[startB], modelPoseA.m_data[startA]
                    );
                    {
                        // EXE-INLINED none: hkArray<ChainMapping>::pushBack
                        hkArray<hkaSkeletonMapperData::ChainMapping>& kChains = pBToA->m_chainMappings;
                        hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kChains);
                        if (kChains.m_size == kChains.getCapacity()) {
                            hkArrayUtil::_reserveMore(alloc, &kChains, sizeof(hkaSkeletonMapperData::ChainMapping));
                        }
                        hkaSkeletonMapperData::ChainMapping* pNew = kChains.m_data + kChains.m_size;
                        if (pNew) {
                            *pNew = kChainMapping;
                        }
                        kChains.m_size++;
                    }
                    for (int32_t i = 1; i < chainA.m_size - 1; ++i) {
                        bonesAMapped.m_data[chainA.m_data[i]] = -2;
                    }
                    for (int32_t i = 1; i < chainB.m_size - 1; ++i) {
                        bonesBMapped.m_data[chainB.m_data[i]] = -2;
                    }
                }
            }
        }

        if (params.m_autodetectSimple.m_bool) {
            // A candidate pair of bones, A's and B's.
            struct BonePair {
                HK_DECLARE_POD_TYPE();

                int32_t m_boneA;
                int32_t m_boneB;
            };
            hkArray<BonePair> pairs;
            // Bones match by name, or for a ragdoll mapping with a position tolerance and no compare function by
            // position.
            int32_t (*compareNames)(const char*, const char*) = params.m_compareNames;
            if ((!compareNames) &&
                (type != hkaSkeletonMapperData::HK_RAGDOLL_MAPPING || !(params.m_positionMatchTolerance > 0.0f))) {
                compareNames = &hkString::strCasecmp;
            }

            const float fToleranceSq = params.m_positionMatchTolerance * params.m_positionMatchTolerance;
            for (int32_t a = 0; a < numBonesA; ++a) {
                if (compareNames) {
                    for (int32_t b = 0; b < numBonesB; ++b) {
                        if (compareNames(
                                params.m_skeletonA->m_bones.m_data[a].m_name.cString(),
                                params.m_skeletonB->m_bones.m_data[b].m_name.cString()
                            ) == 0) {
                            // EXE-INLINED none: hkArray<BonePair>::pushBack
                            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&pairs);
                            if (pairs.m_size == pairs.getCapacity()) {
                                hkArrayUtil::_reserveMore(alloc, &pairs, sizeof(BonePair));
                            }
                            BonePair& kPair = pairs.m_data[pairs.m_size++];
                            kPair.m_boneA = a;
                            kPair.m_boneB = b;
                        }
                    }
                }
                else {
                    int32_t closest = -1;
                    float fClosestDistSq = 100000.0f;
                    for (int32_t b = 0; b < numBonesB; ++b) {
                        __m128 diff = _mm_sub_ps(
                            modelPoseA.m_data[a].m_translation.m_quad, modelPoseB.m_data[b].m_translation.m_quad
                        );
                        diff = _mm_mul_ps(diff, diff);
                        const float fDistSq = _mm_cvtss_f32(_mm_add_ss(
                            _mm_shuffle_ps(diff, diff, 0xAA), _mm_add_ss(_mm_shuffle_ps(diff, diff, 0x55), diff)
                        ));
                        if (fClosestDistSq > fDistSq) {
                            fClosestDistSq = fDistSq;
                            closest = b;
                        }
                    }
                    if (fClosestDistSq < fToleranceSq) {
                        // EXE-INLINED none: hkArray<BonePair>::pushBack
                        hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&pairs);
                        if (pairs.m_size == pairs.getCapacity()) {
                            hkArrayUtil::_reserveMore(alloc, &pairs, sizeof(BonePair));
                        }
                        BonePair& kPair = pairs.m_data[pairs.m_size++];
                        kPair.m_boneA = a;
                        kPair.m_boneB = closest;
                    }
                }
            }
            for (int32_t p = 0; p < pairs.m_size; ++p) {
                const int16_t boneA = static_cast<int16_t>(pairs.m_data[p].m_boneA);
                const int16_t boneB = static_cast<int16_t>(pairs.m_data[p].m_boneB);
                if (bonesAMapped.m_data[boneA] != -1 || bonesBMapped.m_data[boneB] != -1) {
                    continue;
                }
                hkaSkeletonMapperData::SimpleMapping kMapping;
                kMapping.m_aFromBTransform.m_translation.setZero4();
                kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad = hkQuadReal0001;
                kMapping.m_aFromBTransform.m_scale.setAll(1.0f);
                kMapping.m_boneA = boneA;
                kMapping.m_boneB = boneB;
                computeMappingTransform(
                    type, poseA, boneA, poseB, boneB, kMapping.m_aFromBTransform, params.m_positionMatchTolerance, 1.0f
                );
                {
                    // EXE-INLINED none: hkArray<SimpleMapping>::pushBack
                    hkArray<hkaSkeletonMapperData::SimpleMapping>& kMappings = pAToB->m_simpleMappings;
                    hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kMappings);
                    if (kMappings.m_size == kMappings.getCapacity()) {
                        hkArrayUtil::_reserveMore(alloc, &kMappings, sizeof(hkaSkeletonMapperData::SimpleMapping));
                    }
                    hkaSkeletonMapperData::SimpleMapping* pNew = kMappings.m_data + kMappings.m_size;
                    if (pNew) {
                        pNew->m_boneA = kMapping.m_boneA;
                        pNew->m_boneB = kMapping.m_boneB;
                        pNew->m_aFromBTransform.m_translation.m_quad = kMapping.m_aFromBTransform.m_translation.m_quad;
                        pNew->m_aFromBTransform.m_rotation.m_vec.m_quad =
                            kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad;
                        pNew->m_aFromBTransform.m_scale.m_quad = kMapping.m_aFromBTransform.m_scale.m_quad;
                    }
                    kMappings.m_size++;
                }
                kMapping.m_aFromBTransform.m_translation.setZero4();
                kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad = hkQuadReal0001;
                kMapping.m_aFromBTransform.m_scale.setAll(1.0f);
                kMapping.m_boneA = boneB;
                kMapping.m_boneB = boneA;
                computeMappingTransform(
                    type, poseB, boneB, poseA, boneA, kMapping.m_aFromBTransform, params.m_positionMatchTolerance, 1.0f
                );
                {
                    // EXE-INLINED none: hkArray<SimpleMapping>::pushBack
                    hkArray<hkaSkeletonMapperData::SimpleMapping>& kMappings = pBToA->m_simpleMappings;
                    hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kMappings);
                    if (kMappings.m_size == kMappings.getCapacity()) {
                        hkArrayUtil::_reserveMore(alloc, &kMappings, sizeof(hkaSkeletonMapperData::SimpleMapping));
                    }
                    hkaSkeletonMapperData::SimpleMapping* pNew = kMappings.m_data + kMappings.m_size;
                    if (pNew) {
                        pNew->m_boneA = kMapping.m_boneA;
                        pNew->m_boneB = kMapping.m_boneB;
                        pNew->m_aFromBTransform.m_translation.m_quad = kMapping.m_aFromBTransform.m_translation.m_quad;
                        pNew->m_aFromBTransform.m_rotation.m_vec.m_quad =
                            kMapping.m_aFromBTransform.m_rotation.m_vec.m_quad;
                        pNew->m_aFromBTransform.m_scale.m_quad = kMapping.m_aFromBTransform.m_scale.m_quad;
                    }
                    kMappings.m_size++;
                }
                bonesAMapped.m_data[boneA] = boneB;
                bonesBMapped.m_data[boneB] = boneA;
                hkErrStream kStream(buf, 512);
                kStream << "Matching " << params.m_skeletonA->m_bones.m_data[boneA].m_name;
                hkError::s_instance->message(
                    hkError::MESSAGE_REPORT, -1, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 561
                );
            }
        }

        if (params.m_autodetectChains.m_bool) {
            // A mapped bone of A whose nearest mapped ancestor is a few bones up makes a chain with it, when the
            // matching bones of B make one too.
            for (int32_t a = 0; a < numBonesA; ++a) {
                const int16_t b = bonesAMapped.m_data[a];
                if (b < 0) {
                    continue;
                }
                const int16_t* parentIndices = params.m_skeletonA->m_parentIndices.m_data;
                int16_t parentA = parentIndices[a];
                if (parentA == -1) {
                    continue;
                }
                while (bonesAMapped.m_data[parentA] == -1) {
                    parentA = parentIndices[parentA];
                    if (parentA == -1) {
                        break;
                    }
                }
                if (parentA == -1) {
                    continue;
                }
                const int16_t parentB = bonesAMapped.m_data[parentA];
                if (parentB == -2) {
                    continue;
                }
                hkArray<int16_t> chainA;
                hkArray<int16_t> chainB;
                hkBool foundA =
                    hkaSkeletonUtils::getBoneChain(*params.m_skeletonA, parentA, static_cast<int16_t>(a), chainA);
                hkBool foundB = hkaSkeletonUtils::getBoneChain(*params.m_skeletonB, parentB, b, chainB);
                if (foundA.m_bool && foundB.m_bool && (chainA.m_size != 2 || chainB.m_size != 2)) {
                    bool bFree = true;
                    for (int32_t i = 1; i < chainB.m_size - 1; ++i) {
                        if (bonesBMapped.m_data[chainB.m_data[i]] != -1) {
                            bFree = false;
                        }
                    }
                    if (bFree) {
                        {
                            hkErrStream kStream(buf, 512);
                            kStream << "Creating chain " << params.m_skeletonA->m_bones.m_data[parentA].m_name << " - "
                                    << params.m_skeletonA->m_bones.m_data[a].m_name;
                            hkError::s_instance->message(
                                hkError::MESSAGE_REPORT, -1, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 637
                            );
                        }
                        hkaSkeletonMapperData::ChainMapping kChainMapping;
                        kChainMapping.~ChainMapping();
                        ::new (&kChainMapping) hkaSkeletonMapperData::ChainMapping;
                        kChainMapping.m_startBoneA = parentA;
                        kChainMapping.m_endBoneA = static_cast<int16_t>(a);
                        kChainMapping.m_startBoneB = parentB;
                        kChainMapping.m_endBoneB = b;
                        kChainMapping.m_endAFromBTransform.setMulInverseMul(modelPoseA.m_data[a], modelPoseB.m_data[b]);
                        kChainMapping.m_startAFromBTransform.setMulInverseMul(
                            modelPoseA.m_data[parentA], modelPoseB.m_data[parentB]
                        );
                        {
                            // EXE-INLINED none: hkArray<ChainMapping>::pushBack
                            hkArray<hkaSkeletonMapperData::ChainMapping>& kChains = pAToB->m_chainMappings;
                            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kChains);
                            if (kChains.m_size == kChains.getCapacity()) {
                                hkArrayUtil::_reserveMore(alloc, &kChains, sizeof(hkaSkeletonMapperData::ChainMapping));
                            }
                            hkaSkeletonMapperData::ChainMapping* pNew = kChains.m_data + kChains.m_size;
                            if (pNew) {
                                *pNew = kChainMapping;
                            }
                            kChains.m_size++;
                        }
                        kChainMapping.~ChainMapping();
                        ::new (&kChainMapping) hkaSkeletonMapperData::ChainMapping;
                        kChainMapping.m_startBoneA = parentB;
                        kChainMapping.m_endBoneA = b;
                        kChainMapping.m_startBoneB = parentA;
                        kChainMapping.m_endBoneB = static_cast<int16_t>(a);
                        kChainMapping.m_endAFromBTransform.setMulInverseMul(modelPoseB.m_data[b], modelPoseA.m_data[a]);
                        kChainMapping.m_startAFromBTransform.setMulInverseMul(
                            modelPoseB.m_data[parentB], modelPoseA.m_data[parentA]
                        );
                        {
                            // EXE-INLINED none: hkArray<ChainMapping>::pushBack
                            hkArray<hkaSkeletonMapperData::ChainMapping>& kChains = pBToA->m_chainMappings;
                            hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kChains);
                            if (kChains.m_size == kChains.getCapacity()) {
                                hkArrayUtil::_reserveMore(alloc, &kChains, sizeof(hkaSkeletonMapperData::ChainMapping));
                            }
                            hkaSkeletonMapperData::ChainMapping* pNew = kChains.m_data + kChains.m_size;
                            if (pNew) {
                                *pNew = kChainMapping;
                            }
                            kChains.m_size++;
                        }
                        for (int32_t i = 1; i < chainA.m_size - 1; ++i) {
                            bonesAMapped.m_data[chainA.m_data[i]] = -2;
                        }
                        for (int32_t i = 1; i < chainB.m_size - 1; ++i) {
                            bonesBMapped.m_data[chainB.m_data[i]] = -2;
                        }
                    }
                }
            }
        }

        // The bones left unmapped keep their local transforms.
        for (int32_t b = 0; b < numBonesB; ++b) {
            if (bonesBMapped.m_data[b] == -1) {
                // EXE-INLINED none: hkArray<hkInt16>::pushBack
                hkArray<uint16_t>& kUnmapped = pAToB->m_unmappedBones;
                hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kUnmapped);
                if (kUnmapped.m_size == kUnmapped.getCapacity()) {
                    hkArrayUtil::_reserveMore(alloc, &kUnmapped, sizeof(uint16_t));
                }
                kUnmapped.m_data[kUnmapped.m_size++] = static_cast<uint16_t>(b);
            }
        }
        for (int32_t a = 0; a < numBonesA; ++a) {
            if (bonesAMapped.m_data[a] == -1) {
                // EXE-INLINED none: hkArray<hkInt16>::pushBack
                hkArray<uint16_t>& kUnmapped = pBToA->m_unmappedBones;
                hkMemoryAllocator& alloc = hkContainerHeapAllocator().get(&kUnmapped);
                if (kUnmapped.m_size == kUnmapped.getCapacity()) {
                    hkArrayUtil::_reserveMore(alloc, &kUnmapped, sizeof(uint16_t));
                }
                kUnmapped.m_data[kUnmapped.m_size++] = static_cast<uint16_t>(a);
            }
        }
        pAToB->m_keepUnmappedLocal.m_bool = true;
        pBToA->m_keepUnmappedLocal.m_bool = true;

        originalAToB.m_extractedMotionMapping.m_translation.setZero4();
        originalAToB.m_extractedMotionMapping.m_rotation.m_vec.m_quad = hkQuadReal0001;
        originalAToB.m_extractedMotionMapping.m_scale.setAll(1.0f);
        originalBToA.m_extractedMotionMapping.m_translation.setZero4();
        originalBToA.m_extractedMotionMapping.m_rotation.m_vec.m_quad = hkQuadReal0001;
        originalBToA.m_extractedMotionMapping.m_scale.setAll(1.0f);
        if (type == hkaSkeletonMapperData::HK_RETARGETING_MAPPING) {
            params.m_motionParams.computeExtractedMotionTransforms(
                hkaSkeletonMapperData::HK_RETARGETING_MAPPING, modelPoseA.m_data, numBonesA, modelPoseB.m_data,
                numBonesB, originalAToB.m_extractedMotionMapping, originalBToA.m_extractedMotionMapping
            );
        }
        hkError::s_instance->sectionEnd();
    }
    params.Free();
}

// The SDK's computeChainMapping: the start bones' mapping transform, then the end transform: the shortest rotation
// from B's chain direction to A's (kept in the translation), the inverse of A's chain length as the scale, and the end
// bones' relative rotation. Each direction is normalized with the SDK's inline rsqrt and one Newton step, zero when
// the length is zero.
void hkaSkeletonMapperUtils::computeChainMapping(
    hkaSkeletonMapperData::MappingType type,
    const hkaSkeleton* skeletonA,
    const hkQsTransform* poseA,
    int16_t startA,
    int16_t endA,
    const hkaSkeleton* skeletonB,
    const hkQsTransform* poseB,
    int16_t startB,
    int16_t endB,
    hkaSkeletonMapperData::ChainMapping& mapAFromBOut
) {
    mapAFromBOut.m_startBoneA = startA;
    mapAFromBOut.m_endBoneA = endA;
    mapAFromBOut.m_startBoneB = startB;
    mapAFromBOut.m_endBoneB = endB;
    computeMappingTransform(type, poseA, startA, poseB, startB, mapAFromBOut.m_startAFromBTransform, 1e-05f, 1.0f);
    float fLengthA;
    hkVector4 to;
    computeChainLengthAndPosition(skeletonA, poseA, startA, endA, fLengthA, to);
    float fLengthB;
    hkVector4 from;
    computeChainLengthAndPosition(skeletonB, poseB, startB, endB, fLengthB, from);
    const __m128 half = _mm_set_ss(0.5f);
    const __m128 three = _mm_set_ss(3.0f);
    const __m128 zero = _mm_set_ss(0.0f);
    __m128 toSq = _mm_mul_ps(to.m_quad, to.m_quad);
    __m128 toLengthSq =
        _mm_add_ss(_mm_shuffle_ps(toSq, toSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(toSq, toSq, 0x55), toSq));
    __m128 toInvLength = _mm_rsqrt_ss(toLengthSq);
    __m128 toFactor = _mm_mul_ss(
        _mm_mul_ss(half, toInvLength), _mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(toLengthSq, toInvLength), toInvLength))
    );
    toFactor = _mm_andnot_ps(_mm_cmpeq_ss(toLengthSq, zero), toFactor);
    to.m_quad = _mm_mul_ps(_mm_shuffle_ps(toFactor, toFactor, 0), to.m_quad);
    __m128 fromSq = _mm_mul_ps(from.m_quad, from.m_quad);
    __m128 fromLengthSq =
        _mm_add_ss(_mm_shuffle_ps(fromSq, fromSq, 0xAA), _mm_add_ss(_mm_shuffle_ps(fromSq, fromSq, 0x55), fromSq));
    __m128 fromInvLength = _mm_rsqrt_ss(fromLengthSq);
    __m128 fromFactor = _mm_mul_ss(
        _mm_mul_ss(half, fromInvLength),
        _mm_sub_ss(three, _mm_mul_ss(_mm_mul_ss(fromLengthSq, fromInvLength), fromInvLength))
    );
    fromFactor = _mm_andnot_ps(_mm_cmpeq_ss(fromLengthSq, zero), fromFactor);
    from.m_quad = _mm_mul_ps(_mm_shuffle_ps(fromFactor, fromFactor, 0), from.m_quad);
    hkQuaternion rotation;
    rotation.setShortestRotation(from, to);
    mapAFromBOut.m_endAFromBTransform.m_translation.m_quad = rotation.m_vec.m_quad;
    const float fInvLengthA = 1.0f / fLengthA;
    mapAFromBOut.m_endAFromBTransform.m_scale.setAll(fInvLengthA);
    mapAFromBOut.m_endAFromBTransform.m_rotation.setInverseMul(poseA[endA].m_rotation, poseB[endB].m_rotation);
}

// The SDK's computeChainLengthAndPosition: walks from end up the parents to start, taking the position into each
// bone's parent space and summing the bone lengths (the translations' lengths).
void hkaSkeletonMapperUtils::computeChainLengthAndPosition(
    const hkaSkeleton* skeleton,
    const hkQsTransform* pose,
    int16_t start,
    int16_t end,
    float& lengthOut,
    hkVector4& posOut
) {
    lengthOut = 0.0f;
    posOut.setZero4();
    for (int16_t bone = end; bone > start; bone = skeleton->m_parentIndices.m_data[bone]) {
        const hkQsTransform& boneTransform = pose[bone];
        hkVector4 position = posOut;
        posOut.setRotatedDir(boneTransform.m_rotation, position);
        posOut.setAdd4(posOut, boneTransform.m_translation);
        __m128 translationSq = _mm_mul_ps(boneTransform.m_translation.m_quad, boneTransform.m_translation.m_quad);
        const float fBoneLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
            _mm_shuffle_ps(translationSq, translationSq, 0xAA),
            _mm_add_ss(_mm_shuffle_ps(translationSq, translationSq, 0x55), translationSq)
        )));
        lengthOut = lengthOut + fBoneLength;
    }
}

const hkaSkeletonMapperData::SimpleMapping*
hkaSkeletonMapperUtils::findSimpleMapping(int16_t boneA, const hkaSkeletonMapperData& mapperdata) {
    int32_t m_size = mapperdata.m_simpleMappings.m_size;
    int32_t v3 = 0;
    if (m_size <= 0) {
        return nullptr;
    }
    for (hkaSkeletonMapperData::SimpleMapping* i = mapperdata.m_simpleMappings.m_data; i->m_boneA != boneA; ++i) {
        if (++v3 >= m_size) {
            return nullptr;
        }
    }
    return &mapperdata.m_simpleMappings.m_data[v3];
}

bool hkaSkeletonMapperUtils::checkRotation(
    const hkaSkeletonMapperUtils::Params& params,
    int16_t boneA,
    int16_t boneB,
    const hkQsTransform& aToBTransform,
    const hkQsTransform& bToATransform
) {
    if (params.m_mappingType.m_storage != 1) {
        return true;
    }
    float a1a = std::fabs(aToBTransform.m_rotation.getAngle());
    float a1 = std::fabs(bToATransform.m_rotation.getAngle());
    double v7 = a1a;
    if (!(a1 < static_cast<double>(a1a))) {
        v7 = a1;
    }
    float a1b = v7;
    return params.m_rotationMatchTolerance >= static_cast<double>(a1b);
}

// Havok SDK: the first simple mapping whose bone A lies in skeleton A gives the two root bones; every bone after a
// root (by index) gets its translation locked, the roots and the bones before them unlocked. HK_FAILURE with a
// warning when no mapping gives a root or its bone B lies outside skeleton B.
hkResult hkaSkeletonMapperUtils::lockTranslationsAutomatically(const hkaSkeletonMapperData& mapperData) {
    char buf[512];
    hkaSkeleton* pSkeletonA = mapperData.m_skeletonA;
    hkaSkeleton* pSkeletonB = mapperData.m_skeletonB;
    const int32_t numBonesB = pSkeletonB->m_bones.m_size;
    int32_t rootA = pSkeletonA->m_bones.m_size;
    int32_t rootB = numBonesB;
    for (int32_t i = 0; i < mapperData.m_simpleMappings.m_size; ++i) {
        const hkaSkeletonMapperData::SimpleMapping& rSimple = mapperData.m_simpleMappings.m_data[i];
        if (rSimple.m_boneA < rootA) {
            rootB = rSimple.m_boneB;
            if (rootB >= numBonesB) {
                hkErrStream kStream(buf, 512);
                kStream << "Invalid mapping - cannot lock translation based on it";
                hkError::s_instance->message(
                    hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba782a), buf,
                    ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 767
                );
                return HK_FAILURE;
            }
            rootA = rSimple.m_boneA;
            break;
        }
    }
    if (rootA == pSkeletonA->m_bones.m_size) {
        hkErrStream kStream(buf, 512);
        kStream << "Invalid mapping - cannot lock translations based on it";
        hkError::s_instance->message(
            hkError::MESSAGE_WARNING, static_cast<int32_t>(0xabba782a), buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp",
            776
        );
        return HK_FAILURE;
    }

    hkError::s_instance->sectionBegin(static_cast<int32_t>(0x87e5654e), "lockTranslationsAutomatically");
    {
        hkErrStream kStream(buf, 512);
        kStream << "Locking translations in skeleton " << pSkeletonA->m_name << " from bone "
                << pSkeletonA->m_bones.m_data[rootA].m_name;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 784);
    }
    for (int32_t i = 0; i < pSkeletonA->m_bones.m_size; ++i) {
        pSkeletonA->m_bones.m_data[i].m_lockTranslation.m_bool = i > rootA;
    }
    {
        hkErrStream kStream(buf, 512);
        kStream << "Locking translations in skeleton " << pSkeletonB->m_name << " from bone "
                << pSkeletonB->m_bones.m_data[rootB].m_name;
        hkError::s_instance->message(hkError::MESSAGE_REPORT, -1, buf, ".\\Mapper\\hkaSkeletonMapperUtils.cpp", 793);
    }
    for (int32_t i = 0; i < pSkeletonB->m_bones.m_size; ++i) {
        pSkeletonB->m_bones.m_data[i].m_lockTranslation.m_bool = i > rootB;
    }
    hkError::s_instance->sectionEnd();
    return HK_SUCCESS;
}

// Havok SDK: scales the translations of the mapper's transforms: each simple mapping's and each chain mapping's
// start and end transforms.
void hkaSkeletonMapperUtils::scaleMapping(hkaSkeletonMapper& mapper, float scale) {
    hkaSkeletonMapperData& rMapping = mapper.m_mapping;
    for (int32_t i = 0; i < rMapping.m_simpleMappings.m_size; ++i) {
        hkVector4& rTranslation = rMapping.m_simpleMappings.m_data[i].m_aFromBTransform.m_translation;
        rTranslation.m_quad = _mm_mul_ps(_mm_set1_ps(scale), rTranslation.m_quad);
    }
    for (int32_t i = 0; i < rMapping.m_chainMappings.m_size; ++i) {
        hkaSkeletonMapperData::ChainMapping& rChain = rMapping.m_chainMappings.m_data[i];
        rChain.m_startAFromBTransform.m_translation.m_quad =
            _mm_mul_ps(_mm_set1_ps(scale), rChain.m_startAFromBTransform.m_translation.m_quad);
        rChain.m_endAFromBTransform.m_translation.m_quad =
            _mm_mul_ps(_mm_set1_ps(scale), rChain.m_endAFromBTransform.m_translation.m_quad);
    }
}

void hkaSkeletonMapperUtils::computeMappingTransform(
    hkaSkeletonMapperData::MappingType type,
    const hkQsTransform* poseA,
    int16_t boneA,
    const hkQsTransform* poseB,
    int16_t boneB,
    hkQsTransform& mapOut,
    float tolerance,
    float defaultScale
) {
    hkaSkeletonMapperUtils::computeMappingTransform(type, poseA[boneA], poseB[boneB], mapOut, tolerance, defaultScale);
}

hkaSkeletonMapperData::ChainMapping::ChainMapping() : m_startBoneA(0), m_endBoneA(0), m_startBoneB(0), m_endBoneB(0) {
    m_startAFromBTransform.m_translation.setZero4();
    m_startAFromBTransform.m_rotation.m_vec.m_quad = hkQuadReal0001;
    m_startAFromBTransform.m_scale.setAll(1.0f);
    m_endAFromBTransform.m_translation.setZero4();
    m_endAFromBTransform.m_rotation.m_vec.m_quad = hkQuadReal0001;
    m_endAFromBTransform.m_scale.setAll(1.0f);
}

const hkClass& hkaSkeletonMapperData::ChainMapping::staticClass() {
    return hkaSkeletonMapperDataChainMappingClass;
}
