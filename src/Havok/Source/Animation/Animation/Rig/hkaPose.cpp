#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <xmmintrin.h>

uint8_t* returnTrue(uint8_t* a1, int32_t a2) {
    *a1 = (uint8_t)1;
    return a1;
}

hkaPose::hkaPose(hkaPose::PoseSpace space, const hkaSkeleton* skeleton, const hkArrayBase<hkQsTransform>& pose)
  : m_skeleton(skeleton) {
    init(space, skeleton, pose);
}

hkaPose::hkaPose(hkaPose::PoseSpace space, const hkaSkeleton* skeleton, const hkQsTransform* pose, int32_t numBones)
  : m_skeleton(skeleton) {
    // The caller's pose wrapped as a non-deallocating array: init only reads it.
    hkArrayBase<hkQsTransform> kPose(const_cast<hkQsTransform*>(pose), numBones, numBones);
    init(space, skeleton, kPose);
}

hkaPose::~hkaPose() {}

hkArray<hkQsTransform, hkContainerHeapAllocator>& hkaPose::accessUnsyncedPoseModelSpace() {
    int32_t m_size = m_skeleton->m_bones.m_size;
    for (int32_t i = 0; i < m_size; ++i) {
        m_boneFlags.m_data[i] = 1;
    }
    m_localInSync.m_bool = 0;
    m_modelInSync.m_bool = 1;
    return m_modelPose;
}

// Each bone whose model transform is dirty is rebuilt from its parent's (parents come first) or, for a root, copied
// from its local transform.
void hkaPose::syncModelSpace() {
    if (!m_modelInSync.m_bool) {
        const int32_t iNumBones = m_skeleton->m_bones.m_size;
        for (int32_t i = 0; i < iNumBones; ++i) {
            if (m_boneFlags.m_data[i] & F_BONE_MODEL_DIRTY) {
                const int16_t sParent = m_skeleton->m_parentIndices.m_data[i];
                const hkQsTransform& kLocal = m_localPose.m_data[i];
                if (sParent == -1) {
                    hkQsTransform& kModel = m_modelPose.m_data[i];
                    kModel.m_translation.m_quad = kLocal.m_translation.m_quad;
                    kModel.m_rotation.m_vec.m_quad = kLocal.m_rotation.m_vec.m_quad;
                    kModel.m_scale.m_quad = kLocal.m_scale.m_quad;
                }
                else {
                    hkQsTransform* pModelPose = m_modelPose.m_data;
                    const hkQsTransform& kParent = pModelPose[sParent];
                    hkQsTransform& kModel = pModelPose[i];
                    hkVector4 kTranslation;
                    kTranslation.setRotatedDir(kParent.m_rotation, kLocal.m_translation);
                    kModel.m_translation.m_quad = _mm_add_ps(kTranslation.m_quad, kParent.m_translation.m_quad);
                    kModel.m_rotation.setMul(kParent.m_rotation, kLocal.m_rotation);
                    kModel.m_scale.m_quad = _mm_mul_ps(kParent.m_scale.m_quad, kLocal.m_scale.m_quad);
                }
                m_boneFlags.m_data[i] &= ~F_BONE_MODEL_DIRTY;
            }
        }
        m_modelInSync.m_bool = 1;
    }
}

void hkaPose::syncLocalSpace() {
    hkaPose* this_1 = this;
    if (!m_localInSync.m_bool) {
        int32_t m_size = m_skeleton->m_bones.m_size;
        int32_t m_size_2 = 0;
        int32_t m_size_1 = m_size;
        int32_t m_size_3 = 0;
        if (m_size > 0) {
            int32_t v4 = 0;
            do {
                uint32_t* m_data = this_1->m_boneFlags.begin();
                if ((m_data[m_size_2] & 1) != 0) {
                    int16_t boneIdx = this_1->m_skeleton->m_parentIndices.m_data[m_size_2];
                    hkQsTransform* m_data_1 = this_1->m_modelPose.begin();
                    if (boneIdx == -1) {
                        hkVector4* p_m_translation = &this_1->m_localPose.m_data[v4].m_translation;
                        *p_m_translation = m_data_1[v4].m_translation;
                        p_m_translation[1] = m_data_1[v4].m_rotation.m_vec;
                        p_m_translation[2] = m_data_1[v4].m_scale;
                    }
                    else {
                        const hkQsTransform* v8;
                        hkQuaternion q0_;
                        if ((m_data[boneIdx] & 2) != 0) {
                            v8 = &this_1->calculateBoneModelSpace(boneIdx);
                        }
                        else {
                            v8 = &m_data_1[boneIdx];
                        }
                        __m128 m_quad = v8->m_rotation.m_vec.m_quad;
                        __m128 v10 = _mm_mul_ps(v8->m_translation.m_quad, m_quad);
                        __m128 v11 = _mm_shuffle_ps(v10, v10, 170);
                        v11[0] = v11[0] + (_mm_shuffle_ps(v10, v10, 85)[0] + v10[0]);
                        __m128 v12 = _mm_shuffle_ps(m_quad, m_quad, 255);
                        __m128 v13 = _mm_add_ps(
                            _mm_mul_ps(
                                _mm_sub_ps(
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(v8->m_translation.m_quad, v8->m_translation.m_quad, 201),
                                        _mm_shuffle_ps(m_quad, m_quad, 210)
                                    ),
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(v8->m_translation.m_quad, v8->m_translation.m_quad, 210),
                                        _mm_shuffle_ps(m_quad, m_quad, 201)
                                    )
                                ),
                                v12
                            ),
                            _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_add_ps(_mm_mul_ps(v12, v12), hkQuadRealMinusHalf), v8->m_translation.m_quad
                                ),
                                _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), m_quad)
                            )
                        );
                        hkQsTransform* m_data_2 = this_1->m_localPose.begin();
                        __m128 v15 = _mm_xor_ps(_mm_add_ps(v13, v13), hkQuadRealNegateMask);
                        __m128 m_quad_2 = _mm_xor_ps(m_quad, rdata_10C4C00);
                        __m128 v17 = _mm_shuffle_ps(
                            v8->m_scale.m_quad, _mm_unpackhi_ps(v8->m_scale.m_quad, hkQuadReal1111), 196
                        );
                        __m128 v18 = _mm_rcp_ps(v17);
                        __m128 v19 = _mm_and_ps(
                            _mm_sub_ps(_mm_add_ps(v18, v18), _mm_mul_ps(_mm_mul_ps(v18, v17), v18)), rdata_10C4C10
                        );
                        __m128 m_quad_1 = m_data_1[v4].m_translation.m_quad;
                        __m128 v31 = v19;
                        __m128 v21 = _mm_mul_ps(m_quad_1, m_quad_2);
                        __m128 v22 = _mm_shuffle_ps(v21, v21, 170);
                        v22[0] = v22[0] + (_mm_shuffle_ps(v21, v21, 85)[0] + v21[0]);
                        __m128 v23 = _mm_shuffle_ps(m_quad_2, m_quad_2, 255);
                        hkQsTransform* v24 = &m_data_2[v4];
                        q0_.m_vec.m_quad = m_quad_2;
                        __m128 v25 = _mm_add_ps(
                            _mm_mul_ps(
                                _mm_sub_ps(
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(m_quad_1, m_quad_1, 210), _mm_shuffle_ps(m_quad_2, m_quad_2, 201)
                                    ),
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(m_quad_1, m_quad_1, 201), _mm_shuffle_ps(m_quad_2, m_quad_2, 210)
                                    )
                                ),
                                v23
                            ),
                            _mm_add_ps(
                                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v23, v23), hkQuadRealMinusHalf), m_quad_1),
                                _mm_mul_ps(_mm_shuffle_ps(v22, v22, 0), m_quad_2)
                            )
                        );
                        v24->m_translation.m_quad = _mm_add_ps(_mm_add_ps(v25, v25), v15);
                        v24->m_rotation.setMul(q0_, m_data_1[v4].m_rotation);
                        m_size_2 = m_size_3;
                        v24->m_scale.m_quad = _mm_mul_ps(m_data_1[v4].m_scale.m_quad, v31);
                        this_1 = this;
                    }
                    this_1->m_boneFlags.m_data[m_size_2] &= ~1u;
                    m_size = m_size_1;
                }
                ++m_size_2;
                ++v4;
                m_size_3 = m_size_2;
            }
            while (m_size_2 < m_size);
        }
        this_1->m_localInSync.m_bool = 1;
    }
}

// Walks up from boneIdx to the first ancestor whose model transform is clean (a dirty root is copied from its local
// transform), marking the bones on the way (F_BONE_INTERNAL_FLAG2); then rebuilds each marked bone from its parent's,
// parents first.
const hkQsTransform& hkaPose::calculateBoneModelSpace(int32_t theBoneIdx) const {
    int32_t iBone = theBoneIdx;
    if (m_boneFlags.m_data[iBone] & F_BONE_MODEL_DIRTY) {
        while (true) {
            const int16_t sParent = m_skeleton->m_parentIndices.m_data[iBone];
            if (sParent == -1) {
                const hkQsTransform& kLocal = m_localPose.m_data[iBone];
                hkQsTransform& kModel = m_modelPose.m_data[iBone];
                kModel.m_translation.m_quad = kLocal.m_translation.m_quad;
                kModel.m_rotation.m_vec.m_quad = kLocal.m_rotation.m_vec.m_quad;
                kModel.m_scale.m_quad = kLocal.m_scale.m_quad;
                m_boneFlags.m_data[iBone] &= ~F_BONE_MODEL_DIRTY;
                break;
            }
            m_boneFlags.m_data[iBone] |= F_BONE_INTERNAL_FLAG2;
            iBone = sParent;
            if (!(m_boneFlags.m_data[iBone] & F_BONE_MODEL_DIRTY)) {
                break;
            }
        }
    }
    for (int32_t i = iBone + 1; i <= theBoneIdx; ++i) {
        if (m_boneFlags.m_data[i] & F_BONE_INTERNAL_FLAG2) {
            const int16_t sParent = m_skeleton->m_parentIndices.m_data[i];
            hkQsTransform* pModelPose = m_modelPose.m_data;
            const hkQsTransform& kLocal = m_localPose.m_data[i];
            const hkQsTransform& kParent = pModelPose[sParent];
            hkQsTransform& kModel = pModelPose[i];
            hkVector4 kTranslation;
            kTranslation.setRotatedDir(kParent.m_rotation, kLocal.m_translation);
            kModel.m_translation.m_quad = _mm_add_ps(kTranslation.m_quad, kParent.m_translation.m_quad);
            kModel.m_rotation.setMul(kParent.m_rotation, kLocal.m_rotation);
            kModel.m_scale.m_quad = _mm_mul_ps(kParent.m_scale.m_quad, kLocal.m_scale.m_quad);
            m_boneFlags.m_data[i] &= ~(F_BONE_MODEL_DIRTY | F_BONE_INTERNAL_FLAG2);
        }
    }
    return m_modelPose.m_data[theBoneIdx];
}

hkQsTransform& hkaPose::accessBoneModelSpace(int32_t boneIdx, hkaPose::PropagateOrNot propagateOrNot) {
    int32_t v4;
    if (propagateOrNot) {
        int32_t m_size = m_skeleton->m_bones.m_size;
        int32_t v8 = boneIdx;
        m_boneFlags.m_data[boneIdx] |= 4u;
        int32_t v9 = boneIdx + 1;
        int32_t v39 = m_size;
        int32_t v38 = boneIdx + 1;
        if (boneIdx + 1 < m_size) {
            int32_t v36 = v9;
            do {
                int16_t v10 = m_skeleton->m_parentIndices.m_data[v9];
                if (v10 != -1) {
                    uint32_t* m_data = m_boneFlags.begin();
                    if ((m_data[v10] & 4) != 0) {
                        if ((m_data[v9] & 1) != 0) {
                            const hkQsTransform* v13;
                            hkQuaternion result;
                            hkQsTransform* v12 = m_modelPose.begin();
                            if ((m_data[v10] & 2) != 0) {
                                v13 = &calculateBoneModelSpace(v10);
                            }
                            else {
                                v13 = &v12[v10];
                            }
                            __m128 m_quad = v13->m_rotation.m_vec.m_quad;
                            __m128 v15 = _mm_mul_ps(v13->m_translation.m_quad, m_quad);
                            __m128 v16 = _mm_shuffle_ps(v15, v15, 170);
                            v16[0] = v16[0] + (_mm_shuffle_ps(v15, v15, 85)[0] + v15[0]);
                            __m128 v17 = _mm_shuffle_ps(m_quad, m_quad, 255);
                            __m128 v18 = _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_sub_ps(
                                        _mm_mul_ps(
                                            _mm_shuffle_ps(v13->m_translation.m_quad, v13->m_translation.m_quad, 201),
                                            _mm_shuffle_ps(m_quad, m_quad, 210)
                                        ),
                                        _mm_mul_ps(
                                            _mm_shuffle_ps(v13->m_translation.m_quad, v13->m_translation.m_quad, 210),
                                            _mm_shuffle_ps(m_quad, m_quad, 201)
                                        )
                                    ),
                                    v17
                                ),
                                _mm_add_ps(
                                    _mm_mul_ps(
                                        _mm_add_ps(_mm_mul_ps(v17, v17), hkQuadRealMinusHalf), v13->m_translation.m_quad
                                    ),
                                    _mm_mul_ps(_mm_shuffle_ps(v16, v16, 0), m_quad)
                                )
                            );
                            hkQsTransform* v19 = m_localPose.begin();
                            __m128 v20 = _mm_xor_ps(_mm_add_ps(v18, v18), hkQuadRealNegateMask);
                            __m128 v21 = _mm_xor_ps(m_quad, rdata_10C4C00);
                            __m128 v22 = _mm_shuffle_ps(
                                v13->m_scale.m_quad, _mm_unpackhi_ps(v13->m_scale.m_quad, hkQuadReal1111), 196
                            );
                            __m128 v23 = _mm_rcp_ps(v22);
                            __m128 v24 = _mm_and_ps(
                                _mm_sub_ps(_mm_add_ps(v23, v23), _mm_mul_ps(_mm_mul_ps(v23, v22), v23)), rdata_10C4C10
                            );
                            __m128 v25 = v12[v36].m_translation.m_quad;
                            __m128 v41 = v24;
                            __m128 v26 = _mm_mul_ps(v25, v21);
                            __m128 v27 = _mm_shuffle_ps(v26, v26, 170);
                            v27[0] = v27[0] + (_mm_shuffle_ps(v26, v26, 85)[0] + v26[0]);
                            hkQsTransform* v28 = &v19[v36];
                            result.m_vec.m_quad = v21;
                            __m128 v29 = _mm_shuffle_ps(v21, v21, 255);
                            __m128 v30 = _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_sub_ps(
                                        _mm_mul_ps(_mm_shuffle_ps(v25, v25, 210), _mm_shuffle_ps(v21, v21, 201)),
                                        _mm_mul_ps(_mm_shuffle_ps(v25, v25, 201), _mm_shuffle_ps(v21, v21, 210))
                                    ),
                                    v29
                                ),
                                _mm_add_ps(
                                    _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v29, v29), hkQuadRealMinusHalf), v25),
                                    _mm_mul_ps(_mm_shuffle_ps(v27, v27, 0), v21)
                                )
                            );
                            v28->m_translation.m_quad = _mm_add_ps(_mm_add_ps(v30, v30), v20);
                            v28->m_rotation.setMul(result, v12[v36].m_rotation);
                            v9 = v38;
                            v28->m_scale.m_quad = _mm_mul_ps(v12[v36].m_scale.m_quad, v41);
                            m_boneFlags.m_data[v38] &= ~1u;
                            m_size = v39;
                        }
                        m_boneFlags.m_data[v9] |= 4u;
                        m_modelInSync.m_bool = 0;
                    }
                }
                ++v36;
                v38 = ++v9;
            }
            while (v9 < m_size);
            v8 = boneIdx;
        }
        for (int32_t i = v8 + 1; i < m_size; ++i) {
            uint32_t* v32 = m_boneFlags.begin();
            uint32_t v33 = v32[i];
            uint32_t* v34 = &v32[i];
            if ((v33 & 4) != 0) {
                *v34 = (v33 & 0xFFFFFFF9u) | 2;
            }
        }
        v4 = boneIdx;
    }
    else {
        v4 = boneIdx;
        int32_t v5 = m_skeleton->m_bones.m_size;
        int32_t v6 = boneIdx + 1;
        for (int32_t j = v5; v6 < v5; ++v6) {
            if (m_skeleton->m_parentIndices.m_data[v6] == boneIdx) {
                if ((m_boneFlags.m_data[v6] & 2) != 0) {
                    calculateBoneModelSpace(v6);
                    v5 = j;
                }
                m_boneFlags.m_data[v6] = 1;
                m_localInSync.m_bool = 0;
            }
        }
    }
    if ((m_boneFlags.m_data[v4] & 2) != 0) {
        calculateBoneModelSpace(v4);
    }
    m_boneFlags.m_data[v4] = 1;
    m_localInSync.m_bool = 0;
    return m_modelPose.m_data[v4];
}

hkArray<hkQsTransform>& hkaPose::accessSyncedPoseModelSpace() {
    syncModelSpace();
    return accessUnsyncedPoseModelSpace();
}

// syncModelSpace and syncLocalSpace are not const members, so these const getters call them through a non-const this.
const hkArray<hkQsTransform>& hkaPose::getSyncedPoseModelSpace() const {
    const_cast<hkaPose*>(this)->syncModelSpace();
    return m_modelPose;
}

const hkArray<hkQsTransform>& hkaPose::getSyncedPoseLocalSpace() const {
    const_cast<hkaPose*>(this)->syncLocalSpace();
    return m_localPose;
}

// The bone count comes from the skeleton, read once; each loop walks the bone flags directly.
hkBool hkaPose::checkPoseValidity() const {
    const int32_t iNumBones = m_skeleton->m_bones.m_size;
    for (int32_t i = 0; i < iNumBones; ++i) {
        const uint32_t uiFlags = m_boneFlags.m_data[i];
        if ((uiFlags & F_BONE_LOCAL_DIRTY) && (uiFlags & F_BONE_MODEL_DIRTY)) {
            return 0;
        }
    }
    if (m_localInSync.m_bool) {
        for (int32_t i = 0; i < iNumBones; ++i) {
            if (m_boneFlags.m_data[i] & F_BONE_LOCAL_DIRTY) {
                return 0;
            }
        }
    }
    if (m_modelInSync.m_bool) {
        for (int32_t i = 0; i < iNumBones; ++i) {
            if (m_boneFlags.m_data[i] & F_BONE_MODEL_DIRTY) {
                return 0;
            }
        }
    }
    return 1;
}

void hkaPose::setToReferencePose() {
    hkString::memCpy(
        m_localPose.begin(), this->m_skeleton->m_referencePose.begin(),
        sizeof(hkQsTransform) * this->m_skeleton->m_bones.m_size
    );
    for (int32_t i = 0; i < this->m_skeleton->m_bones.m_size; ++i) {
        m_boneFlags.m_data[i] = 2;
    }
    m_modelInSync.m_bool = 0;
    m_localInSync.m_bool = 1;
    const hkaSkeleton* m_skeleton = this->m_skeleton;
    int32_t v4 = 0;
    if (this->m_skeleton->m_floatSlots.m_size > 0) {
        float* m_data = m_floatSlotValues.begin();
        do {
            *m_data = 0.0f;
            ++v4;
            ++m_data;
        }
        while (v4 < m_skeleton->m_floatSlots.m_size);
    }
}

hkQsTransform& hkaPose::accessBoneLocalSpace(int32_t a1) {
    hkQuaternion q0_;
    __m128 v60;
    int32_t v2 = a1;
    int32_t m_size = m_skeleton->m_bones.m_size;
    m_boneFlags.m_data[a1] |= 4u;
    int32_t m_size_2 = a1 + 1;
    int32_t m_size_1 = m_size;
    int32_t m_size_4 = a1 + 1;
    if (a1 + 1 < m_size) {
        int32_t m_size_3 = m_size_2;
        do {
            int16_t boneIdx = m_skeleton->m_parentIndices.m_data[m_size_2];
            if (boneIdx != -1) {
                uint32_t* m_data = m_boneFlags.begin();
                if ((m_data[boneIdx] & 4) != 0) {
                    if ((m_data[m_size_2] & 1) != 0) {
                        const hkQsTransform* v9;
                        hkQsTransform* m_data_1 = m_modelPose.begin();
                        if ((m_data[boneIdx] & 2) != 0) {
                            v9 = &calculateBoneModelSpace(boneIdx);
                        }
                        else {
                            v9 = &m_data_1[boneIdx];
                        }
                        __m128 m_quad = v9->m_rotation.m_vec.m_quad;
                        __m128 v11 = _mm_mul_ps(v9->m_translation.m_quad, m_quad);
                        __m128 v12 = _mm_shuffle_ps(v11, v11, 170);
                        v12[0] = v12[0] + (_mm_shuffle_ps(v11, v11, 85)[0] + v11[0]);
                        __m128 v13 = _mm_shuffle_ps(m_quad, m_quad, 255);
                        __m128 v14 = _mm_add_ps(
                            _mm_mul_ps(
                                _mm_sub_ps(
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(v9->m_translation.m_quad, v9->m_translation.m_quad, 201),
                                        _mm_shuffle_ps(m_quad, m_quad, 210)
                                    ),
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(v9->m_translation.m_quad, v9->m_translation.m_quad, 210),
                                        _mm_shuffle_ps(m_quad, m_quad, 201)
                                    )
                                ),
                                v13
                            ),
                            _mm_add_ps(
                                _mm_mul_ps(
                                    _mm_add_ps(_mm_mul_ps(v13, v13), hkQuadRealMinusHalf), v9->m_translation.m_quad
                                ),
                                _mm_mul_ps(_mm_shuffle_ps(v12, v12, 0), m_quad)
                            )
                        );
                        hkQsTransform* m_data_2 = m_localPose.begin();
                        __m128 v16 = _mm_xor_ps(_mm_add_ps(v14, v14), hkQuadRealNegateMask);
                        __m128 m_quad_2 = _mm_xor_ps(m_quad, rdata_10C4C00);
                        __m128 v18 = _mm_shuffle_ps(
                            v9->m_scale.m_quad, _mm_unpackhi_ps(v9->m_scale.m_quad, hkQuadReal1111), 196
                        );
                        __m128 v19 = _mm_rcp_ps(v18);
                        __m128 v20 = _mm_and_ps(
                            _mm_sub_ps(_mm_add_ps(v19, v19), _mm_mul_ps(_mm_mul_ps(v19, v18), v19)), rdata_10C4C10
                        );
                        __m128 m_quad_1 = m_data_1[m_size_3].m_translation.m_quad;
                        v60 = v20;
                        __m128 v22 = _mm_mul_ps(m_quad_2, m_quad_1);
                        __m128 v23 = _mm_shuffle_ps(v22, v22, 170);
                        v23[0] = v23[0] + (_mm_shuffle_ps(v22, v22, 85)[0] + v22[0]);
                        hkQsTransform* v24 = &m_data_2[m_size_3];
                        q0_.m_vec.m_quad = m_quad_2;
                        __m128 v25 = _mm_shuffle_ps(m_quad_2, m_quad_2, 255);
                        __m128 v26 = _mm_add_ps(
                            _mm_mul_ps(
                                _mm_sub_ps(
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(m_quad_1, m_quad_1, 210), _mm_shuffle_ps(m_quad_2, m_quad_2, 201)
                                    ),
                                    _mm_mul_ps(
                                        _mm_shuffle_ps(m_quad_1, m_quad_1, 201), _mm_shuffle_ps(m_quad_2, m_quad_2, 210)
                                    )
                                ),
                                v25
                            ),
                            _mm_add_ps(
                                _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v25, v25), hkQuadRealMinusHalf), m_quad_1),
                                _mm_mul_ps(_mm_shuffle_ps(v23, v23, 0), m_quad_2)
                            )
                        );
                        v24->m_translation.m_quad = _mm_add_ps(_mm_add_ps(v26, v26), v16);
                        v24->m_rotation.setMul(q0_, m_data_1[m_size_3].m_rotation);
                        m_size_2 = m_size_4;
                        v24->m_scale.m_quad = _mm_mul_ps(m_data_1[m_size_3].m_scale.m_quad, v60);
                        m_boneFlags.m_data[m_size_4] &= ~1u;
                    }
                    m_boneFlags.m_data[m_size_2] |= 4u;
                    m_modelInSync.m_bool = 0;
                }
                m_size = m_size_1;
            }
            ++m_size_3;
            m_size_4 = ++m_size_2;
        }
        while (m_size_2 < m_size);
        v2 = a1;
    }
    for (int32_t i = v2 + 1; i < m_size; ++i) {
        uint32_t* m_data_3 = m_boneFlags.begin();
        uint32_t v29 = m_data_3[i];
        uint32_t* v30 = &m_data_3[i];
        if ((v29 & 4) != 0) {
            *v30 = (v29 & ~(F_BONE_MODEL_DIRTY | F_BONE_INTERNAL_FLAG1)) | F_BONE_MODEL_DIRTY;
        }
    }
    uint32_t* m_data_4 = m_boneFlags.begin();
    if ((m_data_4[v2] & 1) != 0) {
        int32_t v32 = a1;
        int16_t boneIdx_1 = m_skeleton->m_parentIndices.m_data[a1];
        hkQsTransform* m_data_5 = m_modelPose.begin();
        uint32_t v35 = a1;
        if (boneIdx_1 == -1) {
            hkQsTransform* v53 = &m_localPose.m_data[v35];
            v53->m_translation = m_data_5[v35].m_translation;
            v53->m_rotation = m_data_5[v35].m_rotation;
            v53->m_scale = m_data_5[v35].m_scale;
        }
        else {
            const hkQsTransform* v36;
            if ((m_data_4[boneIdx_1] & 2) != 0) {
                v36 = &calculateBoneModelSpace(boneIdx_1);
            }
            else {
                v36 = &m_data_5[boneIdx_1];
            }
            __m128 m_quad_3 = v36->m_rotation.m_vec.m_quad;
            __m128 v38 = _mm_mul_ps(v36->m_translation.m_quad, m_quad_3);
            __m128 v39 = _mm_shuffle_ps(v38, v38, 170);
            v39[0] = v39[0] + (_mm_shuffle_ps(v38, v38, 85)[0] + v38[0]);
            __m128 v40 = _mm_shuffle_ps(m_quad_3, m_quad_3, 255);
            __m128 v41 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(
                            _mm_shuffle_ps(v36->m_translation.m_quad, v36->m_translation.m_quad, 201),
                            _mm_shuffle_ps(m_quad_3, m_quad_3, 210)
                        ),
                        _mm_mul_ps(
                            _mm_shuffle_ps(v36->m_translation.m_quad, v36->m_translation.m_quad, 210),
                            _mm_shuffle_ps(m_quad_3, m_quad_3, 201)
                        )
                    ),
                    v40
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v40, v40), hkQuadRealMinusHalf), v36->m_translation.m_quad),
                    _mm_mul_ps(_mm_shuffle_ps(v39, v39, 0), m_quad_3)
                )
            );
            hkQsTransform* m_data_6 = m_localPose.begin();
            __m128 v43 = _mm_xor_ps(_mm_add_ps(v41, v41), hkQuadRealNegateMask);
            __m128 m_quad_5 = _mm_xor_ps(m_quad_3, rdata_10C4C00);
            __m128 v45 = _mm_shuffle_ps(v36->m_scale.m_quad, _mm_unpackhi_ps(v36->m_scale.m_quad, hkQuadReal1111), 196);
            __m128 v46 = _mm_rcp_ps(v45);
            __m128 v47 =
                _mm_and_ps(_mm_sub_ps(_mm_add_ps(v46, v46), _mm_mul_ps(_mm_mul_ps(v46, v45), v46)), rdata_10C4C10);
            __m128 m_quad_4 = m_data_5[v35].m_translation.m_quad;
            v60 = v47;
            __m128 v49 = _mm_mul_ps(m_quad_5, m_quad_4);
            __m128 v50 = _mm_shuffle_ps(v49, v49, 170);
            v50[0] = v50[0] + (_mm_shuffle_ps(v49, v49, 85)[0] + v49[0]);
            __m128* p_m_quad = &m_data_6[v35].m_translation.m_quad;
            q0_.m_vec.m_quad = m_quad_5;
            __m128 v51 = _mm_shuffle_ps(m_quad_5, m_quad_5, 255);
            __m128 v52 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(m_quad_5, m_quad_5, 201), _mm_shuffle_ps(m_quad_4, m_quad_4, 210)),
                        _mm_mul_ps(_mm_shuffle_ps(m_quad_5, m_quad_5, 210), _mm_shuffle_ps(m_quad_4, m_quad_4, 201))
                    ),
                    v51
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v51, v51), hkQuadRealMinusHalf), m_quad_4),
                    _mm_mul_ps(_mm_shuffle_ps(v50, v50, 0), m_quad_5)
                )
            );
            m_data_6[v35].m_translation.m_quad = _mm_add_ps(_mm_add_ps(v52, v52), v43);
            m_data_6[v35].m_rotation.setMul(q0_, m_data_5[v35].m_rotation);
            v32 = a1;
            p_m_quad[2] = _mm_mul_ps(m_data_5[v35].m_scale.m_quad, v60);
        }
        m_boneFlags.m_data[v32] &= ~1u;
        v2 = a1;
    }
    m_boneFlags.m_data[v2] = 2;
    m_modelInSync.m_bool = 0;
    return m_localPose.m_data[v2];
}

void hkaPose::enforceSkeletonConstraintsModelSpace() {
    int32_t m_size = m_skeleton->m_bones.m_size;
    syncModelSpace();
    int32_t v3 = 0;
    if (m_size > 0) {
        int32_t v4 = 0;
        do {
            int16_t v5 = m_skeleton->m_parentIndices.m_data[v3];
            if (m_skeleton->m_bones.m_data[v3].m_lockTranslation.m_bool) {
                hkQsTransform* v6 = &m_skeleton->m_referencePose.m_data[v4];
                if (v5 == -1) {
                    m_modelPose.m_data[v4].m_translation = v6->m_translation;
                }
                else {
                    hkQsTransform* m_data = m_modelPose.begin();
                    int32_t v8 = v5;
                    __m128 m_quad = m_data[v8].m_rotation.m_vec.m_quad;
                    __m128 v10 = _mm_mul_ps(v6->m_translation.m_quad, m_quad);
                    __m128 v11 = _mm_shuffle_ps(v10, v10, 170);
                    v11[0] = v11[0] + (_mm_shuffle_ps(v10, v10, 85)[0] + v10[0]);
                    __m128 v12 = _mm_shuffle_ps(m_quad, m_quad, 255);
                    __m128 v13 = _mm_add_ps(
                        _mm_mul_ps(
                            _mm_sub_ps(
                                _mm_mul_ps(
                                    _mm_shuffle_ps(v6->m_translation.m_quad, v6->m_translation.m_quad, 210),
                                    _mm_shuffle_ps(m_quad, m_quad, 201)
                                ),
                                _mm_mul_ps(
                                    _mm_shuffle_ps(v6->m_translation.m_quad, v6->m_translation.m_quad, 201),
                                    _mm_shuffle_ps(m_quad, m_quad, 210)
                                )
                            ),
                            v12
                        ),
                        _mm_add_ps(
                            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v12, v12), hkQuadRealMinusHalf), v6->m_translation.m_quad),
                            _mm_mul_ps(_mm_shuffle_ps(v11, v11, 0), m_quad)
                        )
                    );
                    m_data[v4].m_translation.m_quad = _mm_add_ps(v13, v13);
                    m_modelPose.m_data[v4].m_translation.m_quad =
                        _mm_add_ps(m_data[v8].m_translation.m_quad, m_modelPose.m_data[v4].m_translation.m_quad);
                }
                m_boneFlags.m_data[v3] |= 1u;
                m_boneFlags.m_data[v3] |= 4u;
            }
            else {
                if (v5 != -1) {
                    uint32_t* v14 = m_boneFlags.begin();
                    if ((v14[v5] & 4) != 0) {
                        v14[v3] |= 1u;
                    }
                }
            }
            ++v3;
            ++v4;
        }
        while (v3 < m_size);
    }
    int32_t v15 = m_boneFlags.m_size;
    for (int32_t i = 0; i < v15; ++i) {
        m_boneFlags.m_data[i] &= 0xFFFFFFE3u;
    }
}

hkArray<hkQsTransform, hkContainerHeapAllocator>& hkaPose::accessSyncedPoseLocalSpace() {
    syncLocalSpace();
    int32_t m_size = m_skeleton->m_bones.m_size;
    for (int32_t i = 0; i < m_size; ++i) {
        m_boneFlags.m_data[i] = 2;
    }
    m_modelInSync.m_bool = 0;
    m_localInSync.m_bool = 1;
    return m_localPose;
}

void hkaPose::enforceSkeletonConstraintsLocalSpace() {
    int32_t m_size = this->m_skeleton->m_bones.m_size;
    syncLocalSpace();
    int32_t v3 = 0;
    if (m_size > 0) {
        int32_t v4 = 0;
        do {
            uint32_t* m_data;
            const hkaSkeleton* m_skeleton = this->m_skeleton;
            if (this->m_skeleton->m_bones.m_data[v3].m_lockTranslation.m_bool) {
                m_localPose.m_data[v4].m_translation = m_skeleton->m_referencePose.m_data[v4].m_translation;
                m_data = m_boneFlags.begin();
            }
            else {
                int16_t v7 = m_skeleton->m_parentIndices.m_data[v3];
                if (v7 == -1 || (m_boneFlags.m_data[v7] & 4) == 0) {
                    goto LABEL_9;
                }
                m_data = m_boneFlags.begin();
            }
            m_data[v3] |= 2u;
            m_boneFlags.m_data[v3] |= 4u;
        LABEL_9:;
            ++v3;
            ++v4;
        }
        while (v3 < m_size);
    }
    int32_t v8 = m_boneFlags.m_size;
    for (int32_t i = 0; i < v8; ++i) {
        m_boneFlags.m_data[i] &= 0xFFFFFFE3u;
    }
}

int32_t hkaPose::setNonInitializedFlags() {
    int32_t result;
    int32_t m_size = m_skeleton->m_bones.m_size;
    for (result = 0; result < m_size; ++result) {
        m_boneFlags.m_data[result] = 3;
    }
    m_localInSync.m_bool = 0;
    m_modelInSync.m_bool = 0;
    return result;
}

int32_t hkaPose::setPoseLocalSpace(const hkArrayBase<hkQsTransform>& poseLocal) {
    int32_t result;
    int32_t m_size = m_skeleton->m_bones.m_size;
    hkString::memCpy(
        static_cast<hkArrayBase<hkQsTransform>&>(m_localPose).begin(), poseLocal.m_data, sizeof(hkQsTransform) * m_size
    );
    for (result = 0; result < m_size; ++result) {
        m_boneFlags.m_data[result] = 2;
    }
    m_localInSync.m_bool = 1;
    m_modelInSync.m_bool = 0;
    return result;
}

int32_t hkaPose::setPoseModelSpace(const hkArrayBase<hkQsTransform>& poseModel) {
    int32_t result;
    int32_t m_size = m_skeleton->m_bones.m_size;
    hkString::memCpy(
        static_cast<hkArrayBase<hkQsTransform>&>(m_modelPose).begin(), poseModel.m_data, sizeof(hkQsTransform) * m_size
    );
    for (result = 0; result < m_size; ++result) {
        m_boneFlags.m_data[result] = 1;
    }
    m_localInSync.m_bool = 0;
    m_modelInSync.m_bool = 1;
    return result;
}

uint8_t* hkaPose::checkPoseTransformsValidity(uint8_t* a1) {
    int32_t m_size = m_skeleton->m_bones.m_size;
    int32_t v4 = 0;
    if (m_size <= 0) {
    LABEL_10:;
        *a1 = 1;
        return a1;
    }

    int32_t v5 = 0;
    while (true) {
        uint32_t v6 = m_boneFlags.m_data[v4];
        if ((v6 & 2) != 0 && (v6 & 1) != 0) {
            break;
        }
        m_boneFlags.m_data[v4];
        if ((m_boneFlags.m_data[v4] & 2) == 0 && !m_modelPose.m_data[v5].isOk().m_bool) {
            break;
        }
        if ((m_boneFlags.m_data[v4] & 1) == 0 && !m_localPose.m_data[v5].isOk().m_bool) {
            break;
        }
        ++v4;
        ++v5;
        if (v4 >= m_size) {
            goto LABEL_10;
        }
    }
    *a1 = 0;
    return a1;
}

void hkaPose::getModelSpaceAabb(__m128* a1) {
    *a1 = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
    a1[1] = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
    int32_t m_size = m_skeleton->m_bones.m_size;
    syncModelSpace();
    if (m_size > 0) {
        int32_t v4 = 0;
        int32_t v5 = m_size;
        do {
            hkQsTransform* m_data = m_modelPose.begin();
            __m128 v7 = a1[1];
            *a1 = _mm_min_ps(*a1, m_data[v4].m_translation.m_quad);
            __m128 m_quad = m_data[v4++].m_translation.m_quad;
            --v5;
            a1[1] = _mm_max_ps(v7, m_quad);
        }
        while (v5);
    }
}

// Sizes the model pose, local pose and bone flags to m_skeleton's bones (the skeleton argument is not read), sets the
// pose in its space, and reserves the float slot values to a multiple of 4 and sizes them to the skeleton's float
// slots, the new ones 0.
void hkaPose::init(hkaPose::PoseSpace space, const hkaSkeleton* skeleton, const hkArrayBase<hkQsTransform>& pose) {
    const int32_t numBones = m_skeleton->m_bones.m_size;
    m_modelPose.setSize(numBones);
    m_localPose.setSize(numBones);
    m_boneFlags.setSize(numBones);
    if (space == LOCAL_SPACE) {
        setPoseLocalSpace(pose);
    }
    else {
        setPoseModelSpace(pose);
    }
    {
        const int32_t reserved = (m_skeleton->m_floatSlots.m_size + 3) & ~3;
        // EXE-INLINED none: hkArray<float>::reserve
        hkMemoryAllocator& rHeap = hkMemoryRouter::getInstance().heap();
        if (m_floatSlotValues.getCapacity() < reserved) {
            hkArrayUtil::_reserve(rHeap, &m_floatSlotValues, reserved, sizeof(float));
        }
    }
    m_floatSlotValues.setSize(m_skeleton->m_floatSlots.m_size, 0.0f);
}
