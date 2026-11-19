#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"

#include "Havok/Source/Animation/Animation/Animation/hkaAnimation.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnimationBinding.h"
#include "Havok/Source/Animation/Animation/Animation/hkaAnnotationTrack.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperUtils.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/System/Io/OStream/hkOStream.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "SDK/MSVC/string.h"

#include <xmmintrin.h>

// Interpolates a (t = 0) to b (t = 1) into out.
static void setInterpolate(const hkQsTransform& a, const hkQsTransform& b, const hkSimdReal& t, hkQsTransform& out);

// Interpolates in the order translation, rotation, scale. The rotation goes from a's towards b's, or towards b's
// negation when their dot product is negative, then is normalized; one too short to normalize becomes the identity.
static void setInterpolate(const hkQsTransform& a, const hkQsTransform& b, const hkSimdReal& t, hkQsTransform& out) {
    out.m_translation.setInterpolate4(a.m_translation, b.m_translation, t);
    const hkQuadReal x2 = _mm_mul_ps(a.m_rotation.m_vec.m_quad, b.m_rotation.m_vec.m_quad);
    const hkQuadReal sum0 = _mm_add_ps(_mm_shuffle_ps(x2, x2, _MM_SHUFFLE(1, 0, 3, 2)), x2);
    const float dot = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum0, sum0, _MM_SHUFFLE(2, 3, 0, 1)), sum0));
    hkVector4 bRotation;
    if (dot < 0.0f) {
        bRotation.m_quad = _mm_mul_ps(_mm_set1_ps(-1.0f), b.m_rotation.m_vec.m_quad);
    }
    else {
        bRotation.m_quad = b.m_rotation.m_vec.m_quad;
    }
    out.m_rotation.m_vec.setInterpolate4(a.m_rotation.m_vec, bRotation, t);
    out.m_scale.setInterpolate4(a.m_scale, b.m_scale, t);
    const hkQuadReal r2 = _mm_mul_ps(out.m_rotation.m_vec.m_quad, out.m_rotation.m_vec.m_quad);
    const hkQuadReal sum1 = _mm_add_ps(_mm_shuffle_ps(r2, r2, _MM_SHUFFLE(1, 0, 3, 2)), r2);
    const float lengthSquared = _mm_cvtss_f32(_mm_add_ps(_mm_shuffle_ps(sum1, sum1, _MM_SHUFFLE(2, 3, 0, 1)), sum1));
    if (lengthSquared < 1.1920929e-07f) {
        out.m_rotation.setIdentity();
    }
    else {
        out.m_rotation.normalize();
    }
}

// Each bone's world transform is its parent's (worldFromModel for a root) times its local one.
void hkaSkeletonUtils::transformLocalPoseToWorldPose(
    int32_t numTransforms,
    const int16_t* parentIndices,
    const hkQsTransform& worldFromModel,
    const hkQsTransform* poseLocal,
    hkQsTransform* poseWorldOut
) {
    for (int32_t i = 0; i < numTransforms; ++i) {
        const int32_t iParent = parentIndices[i];
        const hkQsTransform& rParent = iParent == -1 ? worldFromModel : poseWorldOut[iParent];
        const hkQsTransform& rLocal = poseLocal[i];
        hkQsTransform& rWorld = poseWorldOut[i];
        // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
        hkVector4 kRotated;
        kRotated.setRotatedDir(rParent.m_rotation, rLocal.m_translation);
        rWorld.m_translation.m_quad = _mm_add_ps(kRotated.m_quad, rParent.m_translation.m_quad);
        rWorld.m_rotation.setMul(rParent.m_rotation, rLocal.m_rotation);
        rWorld.m_scale.m_quad = _mm_mul_ps(rLocal.m_scale.m_quad, rParent.m_scale.m_quad);
    }
}

// Each bone's local transform is the inverse of its parent's world transform (worldFromModel for a root) times its own.
void hkaSkeletonUtils::transformWorldPoseToLocalPose(
    int32_t numTransforms,
    const int16_t* parentIndices,
    const hkQsTransform& worldFromModel,
    const hkQsTransform* poseWorld,
    hkQsTransform* poseLocalOut
) {
    for (int32_t i = 0; i < numTransforms; ++i) {
        const int32_t iParent = parentIndices[i];
        const hkQsTransform& rParent = iParent == -1 ? worldFromModel : poseWorld[iParent];
        // EXE-INLINED none: hkQsTransform::setInverse
        hkVector4 kInvTranslation;
        kInvTranslation.setRotatedInverseDir(rParent.m_rotation, rParent.m_translation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&kInvTranslation.m_quad)[0],
            _mm_xor_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&kInvTranslation.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<float*>(&hkQuadRealNegateMask)[0])
            )
        );
        hkQuaternion kInvRotation;
        _mm_store_ps(
            &reinterpret_cast<float*>(&kInvRotation.m_vec.m_quad)[0],
            _mm_xor_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_rotation.m_vec.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<float*>(&rdata_10C4C00)[0])
            )
        );
        // EXE-INLINED none: hkVector4::setReciprocal3 (x, y, z, 1 through rcpps and one Newton step, then masked)
        const __m128 kScale = _mm_load_ps(&reinterpret_cast<const float*>(&rParent.m_scale.m_quad)[0]);
        const __m128 kScaleXYZ1 = _mm_shuffle_ps(
            kScale, _mm_unpackhi_ps(kScale, _mm_load_ps(&reinterpret_cast<float*>(&hkQuadReal1111)[0])), 0xC4
        );
        const __m128 kRcp = _mm_rcp_ps(kScaleXYZ1);
        const __m128 kInvScale = _mm_and_ps(
            _mm_sub_ps(_mm_add_ps(kRcp, kRcp), _mm_mul_ps(_mm_mul_ps(kRcp, kScaleXYZ1), kRcp)),
            _mm_load_ps(&reinterpret_cast<float*>(&rdata_10C4C10)[0])
        );

        const hkQsTransform& rWorld = poseWorld[i];
        hkQsTransform& rLocal = poseLocalOut[i];
        // EXE-INLINED none: hkQsTransform::setMul (translation and scale; the rotation is the out-of-line call)
        hkVector4 kRotated;
        kRotated.setRotatedDir(kInvRotation, rWorld.m_translation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rLocal.m_translation.m_quad)[0],
            _mm_add_ps(
                _mm_load_ps(&reinterpret_cast<float*>(&kRotated.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<float*>(&kInvTranslation.m_quad)[0])
            )
        );
        rLocal.m_rotation.setMul(kInvRotation, rWorld.m_rotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rLocal.m_scale.m_quad)[0],
            _mm_mul_ps(_mm_load_ps(&reinterpret_cast<const float*>(&rWorld.m_scale.m_quad)[0]), kInvScale)
        );
    }
}

// The model space pose is a world space pose with the model at the identity.
void hkaSkeletonUtils::transformModelPoseToLocalPose(
    int32_t numTransforms,
    const int16_t* parentIndices,
    const hkQsTransform* poseModel,
    hkQsTransform* poseLocalOut
) {
    transformWorldPoseToLocalPose(numTransforms, parentIndices, hkQsTransformIdentity, poseModel, poseLocalOut);
}

void hkaSkeletonUtils::markDescendants(const hkaSkeleton* skeleton, int32_t startBone, bool* out, bool includeStart) {
    if (startBone > 0) {
        std::memset(out, 0, static_cast<size_t>(startBone));
    }
    int32_t v4 = startBone + 1;
    out[startBone] = true;
    if (startBone + 1 >= skeleton->m_bones.m_size) {
        out[startBone] = includeStart;
    }
    else {
        do {
            int16_t v5 = skeleton->m_parentIndices.m_data[v4];
            bool v6 = v5 >= 0 && out[v5];
            out[v4++] = v6;
        }
        while (v4 < skeleton->m_bones.m_size);
        out[startBone] = includeStart;
    }
}

void hkaSkeletonUtils::blendPoses(
    uint32_t numBones,
    const hkQsTransform* poseA,
    const hkQsTransform* poseB,
    const float* weights,
    hkQsTransform* poseOut
) {
    for (uint32_t i = 0; i < numBones; ++i) {
        hkSimdReal weight;
        weight.~hkSimdReal();
        ::new (&weight) hkSimdReal(weights[i]);
        setInterpolate(poseA[i], poseB[i], weight, poseOut[i]);
    }
}

// Builds the hkSimdReal before testing numBones.
void hkaSkeletonUtils::blendPoses(
    uint32_t numBones,
    const hkQsTransform* poseA,
    const hkQsTransform* poseB,
    float weight,
    hkQsTransform* poseOut
) {
    hkSimdReal simdWeight;
    simdWeight.~hkSimdReal();
    ::new (&simdWeight) hkSimdReal(weight);
    for (uint32_t i = 0; i < numBones; ++i) {
        setInterpolate(poseA[i], poseB[i], simdWeight, poseOut[i]);
    }
}

void hkaSkeletonUtils::blendPartialPoses(
    uint32_t numBones,
    int16_t* bones,
    const hkQsTransform* poseA,
    const hkQsTransform* poseB,
    float weight,
    hkQsTransform* poseOut
) {
    hkSimdReal simdWeight;
    simdWeight.~hkSimdReal();
    ::new (&simdWeight) hkSimdReal(weight);
    for (uint32_t i = 0; i < numBones; ++i) {
        const int32_t bone = bones[i];
        setInterpolate(poseA[bone], poseB[bone], simdWeight, poseOut[bone]);
    }
}

// A bone is a descendant when its parent is startBone or a descendant: one pass from startBone + 1 marks them in a
// stack-allocated hkLocalArray<hkBool> of one flag per bone.
void hkaSkeletonUtils::getDescendants(
    const hkaSkeleton& skeleton,
    int16_t startBone,
    hkArray<int16_t>& bonesOut,
    bool includeStart
) {
    const int32_t numBones = skeleton.m_bones.m_size;
    hkLocalArray<hkBool> included(numBones);

    // EXE-INLINED none: hkArray<hkBool>::setSize(numBones, false)
    const int32_t size = skeleton.m_bones.m_size;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t capacity = static_cast<int32_t>(included.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK);
    if (capacity < size) {
        int32_t request = 2 * capacity;
        if (size >= request) {
            request = size;
        }
        hkArrayUtil::_reserve(pHeap, &included, request, sizeof(hkBool));
    }
    const int32_t numNew = size - included.m_size;
    if (numNew > 0) {
        std::memset(included.m_data + included.m_size, 0, numNew);
    }
    included.m_size = static_cast<uint32_t>(size);

    if (includeStart) {
        bonesOut.pushBack(startBone);
    }
    included.m_data[startBone].m_bool = true;
    for (int16_t b = static_cast<int16_t>(startBone + 1); b < skeleton.m_bones.m_size; ++b) {
        const int16_t parent = skeleton.m_parentIndices.m_data[b];
        if (parent != -1 && included.m_data[parent].m_bool) {
            included.m_data[b].m_bool = true;
            bonesOut.pushBack(b);
        }
    }
}

// Marks every bone that some bone names as its parent, in a stack-allocated hkLocalArray<hkBool>, then appends the
// unmarked ones.
void hkaSkeletonUtils::getLeaves(const hkaSkeleton& skeleton, hkArray<int16_t>& leavesOut) {
    const int32_t numBones = skeleton.m_bones.m_size;
    hkLocalArray<hkBool> isParent(numBones);

    // EXE-INLINED none: hkArray<hkBool>::setSize(numBones, false)
    const int32_t size = skeleton.m_bones.m_size;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    const int32_t capacity = static_cast<int32_t>(isParent.m_capacityAndFlags & hkArrayBase<hkBool>::CAPACITY_MASK);
    if (capacity < size) {
        int32_t request = 2 * capacity;
        if (size >= request) {
            request = size;
        }
        hkArrayUtil::_reserve(pHeap, &isParent, request, sizeof(hkBool));
    }
    const int32_t numNew = size - isParent.m_size;
    if (numNew > 0) {
        std::memset(isParent.m_data + isParent.m_size, 0, numNew);
    }
    isParent.m_size = static_cast<uint32_t>(size);

    for (int32_t i = 0; i < skeleton.m_parentIndices.m_size; ++i) {
        const int16_t parent = skeleton.m_parentIndices.m_data[i];
        if (parent != -1) {
            isParent.m_data[parent].m_bool = true;
        }
    }
    for (int16_t b = 0; b < skeleton.m_bones.m_size; ++b) {
        if (!isParent.m_data[b].m_bool) {
            leavesOut.pushBack(b);
        }
    }
}

// Walks from endBone up its parents until startBone, each bone inserted at index 0, then startBone itself; found when
// the walk did not run off a root. The current bone is kept in endBone and inserted from there.
hkBool hkaSkeletonUtils::getBoneChain(
    const hkaSkeleton& skeleton,
    int16_t startBone,
    int16_t endBone,
    hkArray<int16_t, hkContainerHeapAllocator>& bonesOut
) {
    while (endBone > 0) {
        if (endBone == startBone) {
            break;
        }
        bonesOut._insertAt<int16_t>(hkMemoryRouter::getInstance().heap(), 0, endBone);
        endBone = skeleton.m_parentIndices.m_data[endBone];
    }
    bonesOut._insertAt<int16_t>(hkMemoryRouter::getInstance().heap(), 0, startBone);
    return endBone >= 0;
}

void hkaSkeletonUtils::enforceSkeletonConstraintsModelSpace(
    const hkaSkeleton& skeleton,
    hkQsTransform* poseModelInOut
) {
    int32_t v2 = 0;
    int32_t m_size = skeleton.m_bones.m_size;
    if (m_size > 0) {
        int32_t v3 = 0;
        do {
            int16_t v4 = skeleton.m_parentIndices.m_data[v2];
            if (skeleton.m_bones.m_data[v2].m_lockTranslation.m_bool) {
                __m128 m_quad;
                hkQsTransform* v5 = &skeleton.m_referencePose.m_data[v3];
                if (v4 == -1) {
                    m_quad = v5->m_translation.m_quad;
                }
                else {
                    hkQuaternion v7;
                    int32_t v6 = v4;
                    v7.m_vec.m_quad = poseModelInOut[v6].m_rotation.m_vec.m_quad;
                    __m128 v8 = _mm_mul_ps(v5->m_translation.m_quad, v7.m_vec.m_quad);
                    __m128 v9 = _mm_shuffle_ps(v8, v8, 170);
                    v9[0] = v9[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]);
                    __m128 v10 = _mm_shuffle_ps(v7.m_vec.m_quad, v7.m_vec.m_quad, 255);
                    __m128 v11 = _mm_add_ps(
                        _mm_mul_ps(
                            _mm_sub_ps(
                                _mm_mul_ps(
                                    _mm_shuffle_ps(v5->m_translation.m_quad, v5->m_translation.m_quad, 210),
                                    _mm_shuffle_ps(v7.m_vec.m_quad, v7.m_vec.m_quad, 201)
                                ),
                                _mm_mul_ps(
                                    _mm_shuffle_ps(v5->m_translation.m_quad, v5->m_translation.m_quad, 201),
                                    _mm_shuffle_ps(v7.m_vec.m_quad, v7.m_vec.m_quad, 210)
                                )
                            ),
                            v10
                        ),
                        _mm_add_ps(
                            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v10, v10), hkQuadRealMinusHalf), v5->m_translation.m_quad),
                            _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), v7.m_vec.m_quad)
                        )
                    );
                    poseModelInOut[v3].m_translation.m_quad = _mm_add_ps(v11, v11);
                    m_quad =
                        _mm_add_ps(poseModelInOut[v3].m_translation.m_quad, poseModelInOut[v6].m_translation.m_quad);
                }
                poseModelInOut[v3].m_translation.m_quad = m_quad;
            }
            ++v2;
            ++v3;
        }
        while (v2 < m_size);
    }
}

// The SDK's loop: a bone whose translation is locked takes its constraint's local translation, put in model space by
// its parent's model transform (rotated, then translated); a root takes it as it is.
void hkaSkeletonUtils::enforcePoseConstraintsModelSpace(
    const hkaSkeleton& skeleton,
    const hkQsTransform* constraintsLocal,
    hkQsTransform* poseModelInOut
) {
    const int32_t numBones = skeleton.m_bones.m_size;
    for (int32_t i = 0; i < numBones; ++i) {
        const int16_t parent = skeleton.m_parentIndices.m_data[i];
        if (skeleton.m_bones.m_data[i].m_lockTranslation.m_bool) {
            const hkQsTransform& rConstraint = constraintsLocal[i];
            __m128 translation;
            if (parent == -1) {
                translation = rConstraint.m_translation.m_quad;
            }
            else {
                const hkQsTransform& rParent = poseModelInOut[parent];
                // EXE-INLINED none: hkVector4::setRotatedDir
                hkVector4 kRotated;
                kRotated.setRotatedDir(rParent.m_rotation, rConstraint.m_translation);
                translation = _mm_add_ps(kRotated.m_quad, rParent.m_translation.m_quad);
            }
            poseModelInOut[i].m_translation.m_quad = translation;
        }
    }
}

void hkaSkeletonUtils::transformLocalPoseToModelPose(
    int32_t a1,
    const int16_t* a2,
    const hkQsTransform* a3,
    hkQsTransform* a4
) {
    hkaSkeletonUtils::transformLocalPoseToWorldPose(a1, a2, hkQsTransformIdentity, a3, a4);
}

void hkaSkeletonUtils::getModelSpaceScale(
    const hkaSkeleton& skeleton,
    const hkQsTransform* poseLocal,
    int32_t boneId,
    hkVector4& scaleOut
) {
    int32_t v4 = boneId;
    __m128 m_quad = hkQuadReal1111;
    if (boneId == -1) {
        scaleOut.m_quad = hkQuadReal1111;
    }
    else {
        hkVector4 v7;
        do {
            int32_t v6 = v4;
            v4 = skeleton.m_parentIndices.m_data[v4];
            v7.m_quad = _mm_mul_ps(poseLocal[v6].m_scale.m_quad, m_quad);
            m_quad = v7.m_quad;
        }
        while (v4 != -1);
        scaleOut.m_quad = v7.m_quad;
    }
}

// Linear search by name, using hkString::strCasecmp when no compare is given. The index counts as an hkInt16.
int16_t hkaSkeletonUtils::findBoneWithName(
    const hkaSkeleton& skeleton,
    const char* name,
    int32_t (*compare)(const char*, const char*)
) {
    const int32_t numBones = skeleton.m_bones.m_size;
    if (!compare) {
        compare = &hkString::strCasecmp;
    }
    for (int16_t i = 0; i < numBones; ++i) {
        if (compare(name, skeleton.m_bones.m_data[i].m_name.cString()) == 0) {
            return i;
        }
    }
    return -1;
}

void hkaSkeletonUtils::transformLocalBoneToModelBone(
    int32_t boneIndex,
    const int16_t* parentIndices,
    const hkQsTransform* poseLocal,
    hkQsTransform& boneModelOut
) {
    boneModelOut = poseLocal[boneIndex];
    for (int32_t i = parentIndices[boneIndex]; i >= 0; i = parentIndices[i]) {
        const hkQsTransform* v5 = &poseLocal[i];
        __m128 m_quad = v5->m_rotation.m_vec.m_quad;
        __m128 v7 = _mm_mul_ps(boneModelOut.m_translation.m_quad, m_quad);
        __m128 v8 = _mm_shuffle_ps(v7, v7, 170);
        v8[0] = v8[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
        __m128 v9 = _mm_shuffle_ps(m_quad, m_quad, 255);
        __m128 v10 = _mm_add_ps(
            _mm_mul_ps(
                _mm_sub_ps(
                    _mm_mul_ps(
                        _mm_shuffle_ps(boneModelOut.m_translation.m_quad, boneModelOut.m_translation.m_quad, 210),
                        _mm_shuffle_ps(m_quad, m_quad, 201)
                    ),
                    _mm_mul_ps(
                        _mm_shuffle_ps(boneModelOut.m_translation.m_quad, boneModelOut.m_translation.m_quad, 201),
                        _mm_shuffle_ps(m_quad, m_quad, 210)
                    )
                ),
                v9
            ),
            _mm_add_ps(
                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v9, v9), hkQuadRealMinusHalf), boneModelOut.m_translation.m_quad),
                _mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), m_quad)
            )
        );
        boneModelOut.m_translation.m_quad = _mm_add_ps(_mm_add_ps(v10, v10), v5->m_translation.m_quad);
        boneModelOut.m_rotation.setMul(v5->m_rotation, boneModelOut.m_rotation);
        boneModelOut.m_scale.m_quad = _mm_mul_ps(v5->m_scale.m_quad, boneModelOut.m_scale.m_quad);
    }
}

void hkaSkeletonUtils::lockTranslations(hkaSkeleton& skeleton, hkBool exceptRoots) {
    int32_t m_size = skeleton.m_bones.m_size;
    for (int32_t i = 0; i < m_size; ++i) {
        if (skeleton.m_parentIndices.m_data[i] > 0 || !exceptRoots.m_bool) {
            skeleton.m_bones.m_data[i].m_lockTranslation.m_bool = 1;
        }
    }
}

void hkaSkeletonUtils::unlockTranslations(hkaSkeleton& skeleton) {
    int32_t m_size = skeleton.m_bones.m_size;
    for (int32_t i = 0; i < m_size; ++i) {
        skeleton.m_bones.m_data[i].m_lockTranslation.m_bool = 0;
    }
}

hkBool hkaSkeletonUtils::isBindingOk(const hkaSkeleton& skeleton, const hkaAnimationBinding& binding) {
    int32_t m_size = binding.m_transformTrackToBoneIndices.m_size;
    int32_t v4 = 0;
    if (m_size <= 0) {
    LABEL_6:;
        return 1;
    }

    int16_t* m_data = binding.m_transformTrackToBoneIndices.m_data;
    while (*m_data >= 0 && *m_data < skeleton.m_bones.m_size) {
        ++v4;
        ++m_data;
        if (v4 >= m_size) {
            goto LABEL_6;
        }
    }
    return 0;
}

// Each bone's world transform is worldFromModel times its model space one.
void hkaSkeletonUtils::transformModelPoseToWorldPose(
    int32_t numTransforms,
    const hkQsTransform& worldFromModel,
    const hkQsTransform* poseModel,
    hkQsTransform* poseWorldOut
) {
    for (int32_t i = 0; i < numTransforms; ++i) {
        poseWorldOut[i].setMul(worldFromModel, poseModel[i]);
    }
}

// Each bone's model transform is the inverse of worldFromModel times its world space one. The inverse is deliberately
// computed again in every iteration, reloading worldFromModel each time, as the game does.
void hkaSkeletonUtils::transformWorldPoseToModelPose(
    int32_t numTransforms,
    const hkQsTransform& worldFromModel,
    const hkQsTransform* poseWorld,
    hkQsTransform* poseModelOut
) {
    for (int32_t i = 0; i < numTransforms; ++i) {
        hkQsTransform modelFromWorld;
        modelFromWorld.setInverse(worldFromModel);
        poseModelOut[i].setMul(modelFromWorld, poseWorld[i]);
    }
}

// Havok SDK: a bone whose translation is locked takes its translation from the reference pose.
void hkaSkeletonUtils::enforceSkeletonConstraintsLocalSpace(const hkaSkeleton& skeleton, hkQsTransform* poseInOut) {
    const int32_t numBones = skeleton.m_bones.m_size;
    for (int32_t i = 0; i < numBones; ++i) {
        if (skeleton.m_bones.m_data[i].m_lockTranslation.m_bool) {
            poseInOut[i].m_translation = skeleton.m_referencePose.m_data[i].m_translation;
        }
    }
}

// Havok SDK: a bone whose translation is locked takes its translation from constraintsLocal.
void hkaSkeletonUtils::enforcePoseConstraintsLocalSpace(
    const hkaSkeleton& skeleton,
    const hkQsTransform* constraintsLocal,
    hkQsTransform* poseInOut
) {
    const int32_t numBones = skeleton.m_bones.m_size;
    for (int32_t i = 0; i < numBones; ++i) {
        if (skeleton.m_bones.m_data[i].m_lockTranslation.m_bool) {
            poseInOut[i].m_translation = constraintsLocal[i].m_translation;
        }
    }
}

// Each transform track is bound to the bone whose name compare matches the track's annotation track name (the last
// match wins). The bone's name is passed to compare first. Each failure is a warning (HK_WARN) and HK_FAILURE.
hkResult hkaSkeletonUtils::bindByName(
    const hkaSkeleton& skeleton,
    const hkaAnimation& anim,
    int32_t (*compare)(const char*, const char*),
    hkaAnimationBinding& bindingOut
) {
    char buf[512];
    const int32_t numBindings = bindingOut.m_transformTrackToBoneIndices.m_size;
    if (numBindings == 0 || numBindings != anim.m_numberOfTransformTracks) {
        hkErrStream kStream(buf, 512);
        kStream << "Binding is not initiialized";
        hkError::s_instance->message(hkError::MESSAGE_WARNING, 0x654E3435, buf, ".\\Rig\\hkaSkeletonUtils.cpp", 439);
        return HK_FAILURE;
    }
    if (anim.m_annotationTracks.m_size != anim.m_numberOfTransformTracks) {
        hkErrStream kStream(buf, 512);
        kStream << "No track names present in animation";
        hkError::s_instance->message(hkError::MESSAGE_WARNING, 0x654E3437, buf, ".\\Rig\\hkaSkeletonUtils.cpp", 446);
        return HK_FAILURE;
    }
    const int32_t numBones = skeleton.m_bones.m_size;
    for (int32_t i = 0; i < numBones; ++i) {
        if (skeleton.m_bones.m_data[i].m_name.cString() == nullptr) {
            hkErrStream kStream(buf, 512);
            kStream << "No bone name for bone " << i;
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0x654E3437, buf, ".\\Rig\\hkaSkeletonUtils.cpp", 456
            );
            return HK_FAILURE;
        }
    }
    for (int32_t t = 0; t < anim.m_numberOfTransformTracks; ++t) {
        const char* trackName = anim.m_annotationTracks.m_data[t].m_trackName.cString();
        bool found = false;
        for (int16_t b = 0; b < skeleton.m_bones.m_size; ++b) {
            if (compare(skeleton.m_bones.m_data[b].m_name.cString(), trackName) == 0) {
                found = true;
                bindingOut.m_transformTrackToBoneIndices.m_data[t] = b;
            }
        }
        if (!found) {
            hkErrStream kStream(buf, 512);
            kStream << "No match for track " << trackName;
            hkError::s_instance->message(
                hkError::MESSAGE_WARNING, 0x654E3438, buf, ".\\Rig\\hkaSkeletonUtils.cpp", 478
            );
            return HK_FAILURE;
        }
    }
    return HK_SUCCESS;
}
