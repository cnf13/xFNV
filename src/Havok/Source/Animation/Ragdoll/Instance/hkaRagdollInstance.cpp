#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperClass.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix3.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Physics/MotionState/hkMotionState.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstanceClass.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

#include <bit>
#include <xmmintrin.h>

// The class record's member table.
using hkaRagdollInstanceClassMemberArray = const hkClassMember[4];
static hkaRagdollInstanceClassMemberArray hkaRagdollInstanceClass_Members{
    {"rigidBodies",
     &hkpRigidBodyClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     0,
     0x8,
     nullptr},
    {"constraints",
     &hkpConstraintInstanceClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     0,
     0x14,
     nullptr},
    {"boneToRigidBodyMap",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_INT32},
     0,
     0,
     0x20,
     nullptr},
    {"skeleton",
     &hkaSkeletonClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x2C,
     nullptr}
};
hkClass hkaRagdollInstanceClass(
    "hkaRagdollInstance",
    &hkReferencedObjectClass,
    48,
    nullptr,
    0,
    nullptr,
    0,
    hkaRagdollInstanceClass_Members,
    4,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkaRagdollInstanceTypeInfo = {
    "hkaRagdollInstance",          finishLoadedObjecthkaRagdollInstance, cleanupLoadedObjecthkaRagdollInstance,
    getVtablehkaRagdollInstance(), &typeid(hkaRagdollInstance),          sizeof(hkaRagdollInstance)
};

hkaRagdollInstance::hkaRagdollInstance(
    const hkArrayBase<hkpRigidBody*>& rigidBodies,
    const hkArrayBase<hkpConstraintInstance*>& constraints,
    const hkaSkeleton* skeleton,
    const hkArrayBase<int32_t>& boneToRigidBody
)
  : m_skeleton(nullptr) {
    commonInit(rigidBodies, constraints, skeleton);
    m_boneToRigidBodyMap.copyFromArray(
        hkContainerHeapAllocator().get(&m_boneToRigidBodyMap), boneToRigidBody, hkTypeIsPod()
    );
}

hkaRagdollInstance::hkaRagdollInstance(
    const hkArrayBase<hkpRigidBody*>& rigidBodies,
    const hkArrayBase<hkpConstraintInstance*>& constraints,
    const hkaSkeleton* skeleton
)
  : m_skeleton(nullptr) {
    commonInit(rigidBodies, constraints, skeleton);
    // The identity bone-to-body map.
    m_boneToRigidBodyMap.setSize(skeleton->m_bones.getSize());
    for (int32_t iBone = 0; iBone < skeleton->m_bones.getSize(); ++iBone) {
        m_boneToRigidBodyMap[iBone] = iBone;
    }
}

// The references the instance holds; the arrays free themselves.
hkaRagdollInstance::~hkaRagdollInstance() {
    for (int32_t i = 0; i < m_rigidBodies.getSize(); ++i) {
        m_rigidBodies[i]->removeReference();
    }
    for (int32_t i = 0; i < m_constraints.getSize(); ++i) {
        m_constraints[i]->removeReference();
    }
    if (m_skeleton) {
        m_skeleton->removeReference();
    }
    m_skeleton = nullptr;
}

// The bodies and constraints are cloned into two stack-allocated hkLocalArrays sized to the originals; constraint c
// joins bone c + 1's body to its parent's. The new instance holds its own references, so the clones' references are
// dropped afterwards.
hkaRagdollInstance* hkaRagdollInstance::clone(hkpConstraintInstance::CloningMode mode) const {
    const int32_t numBodies = m_rigidBodies.m_size;
    const int32_t numConstraints = m_constraints.m_size;

    hkLocalArray<hkpRigidBody*> newRigidBodies(numBodies);
    newRigidBodies.setSize(numBodies);

    hkLocalArray<hkpConstraintInstance*> newConstraints(numConstraints);
    newConstraints.setSize(numConstraints);

    for (int32_t b = 0; b < numBodies; ++b) {
        const hkpRigidBody* pBody = m_rigidBodies.m_data[b];
        newRigidBodies.m_data[b] = pBody->clone();
    }
    for (int32_t c = 0; c < numConstraints; ++c) {
        hkpRigidBody* pChild = newRigidBodies.m_data[c + 1];
        hkpRigidBody* pParent = newRigidBodies.m_data[m_skeleton->m_parentIndices.m_data[c + 1]];
        newConstraints.m_data[c] = m_constraints.m_data[c]->clone(pChild, pParent, mode);
    }

    hkMemoryRouter& router = hkMemoryRouter::getInstance();
    void* pMemory = router.m_heap->blockAlloc(static_cast<int32_t>(sizeof(hkaRagdollInstance)));
    static_cast<hkaRagdollInstance*>(pMemory)->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkaRagdollInstance));
    hkaRagdollInstance* pInstance =
        ::new (pMemory) hkaRagdollInstance(newRigidBodies, newConstraints, m_skeleton, m_boneToRigidBodyMap);

    for (int32_t b = 0; b < numBodies; ++b) {
        newRigidBodies.m_data[b]->removeReference();
    }
    for (int32_t c = 0; c < numConstraints; ++c) {
        newConstraints.m_data[c]->removeReference();
    }

    return pInstance;
}

// Havok SDK: a bone with a rigid body takes the body's transform approximated at afTime and its model-space scale;
// any other bone is its reference pose in its parent's frame (the identity for a root), found recursively.
void hkaRagdollInstance::getApproxWorldFromBoneTransformAt(
    int32_t i,
    float time,
    hkQsTransform& worldFromBoneOut
) const {
    hkpRigidBody* pRigidBody;
    const int32_t iRigidBody = m_boneToRigidBodyMap.m_data[i];
    if (iRigidBody >= 0 && (pRigidBody = m_rigidBodies.m_data[iRigidBody]) != nullptr) {
        hkTransform kTransform;
        pRigidBody->m_motion.approxTransformAt(time, kTransform);
        worldFromBoneOut.setFromTransformNoScale(kTransform);
        hkaSkeletonUtils::getModelSpaceScale(
            *m_skeleton, m_skeleton->m_referencePose.begin(), i, worldFromBoneOut.m_scale
        );
    }
    else {
        hkQsTransform kWorldFromParent;
        const int32_t iParent = m_skeleton->m_parentIndices.m_data[i];
        if (iParent < 0) {
            kWorldFromParent.m_translation.m_quad = _mm_setzero_ps();
            kWorldFromParent.m_rotation.setIdentity();
            kWorldFromParent.m_scale.m_quad = _mm_set1_ps(1.0f);
        }
        else {
            getApproxWorldFromBoneTransformAt(iParent, time, kWorldFromParent);
        }
        worldFromBoneOut.setMul(kWorldFromParent, m_skeleton->m_referencePose.m_data[i]);
    }
}

// Havok SDK: the rigid body mapped to bone i, null when none is.
hkpRigidBody* hkaRagdollInstance::getRigidBodyOfBone(int32_t i) const {
    int32_t v2 = m_boneToRigidBodyMap.m_data[i];
    if (v2 >= 0) {
        return m_rigidBodies.m_data[v2];
    }

    return nullptr;
}

void hkaRagdollInstance::getPoseWorldSpace(hkQsTransform* poseOut) const {
    getPoseWorldSpace(poseOut, hkQsTransformIdentity, nullptr);
}

void hkaRagdollInstance::getApproxPoseWorldSpaceAt(float time, hkQsTransform* poseWorldSpaceOut) const {
    int32_t m_size = m_skeleton->m_bones.m_size;
    for (int32_t i = 0; i < m_size; ++poseWorldSpaceOut) {
        getApproxWorldFromBoneTransformAt(i++, time, *poseWorldSpaceOut);
    }
}

void hkaRagdollInstance::getApproxCurrentPoseWorldSpace(hkQsTransform* poseWorldSpaceOut) const {
    // The root rigid body's world: tests the body and then its world.
    hkpRigidBody* pRoot = m_rigidBodies.m_data[0];
    hkpWorld* pWorld;
    if (pRoot && (pWorld = pRoot->m_world) != nullptr) {
        getApproxPoseWorldSpaceAt(pWorld->getCurrentTime(), poseWorldSpaceOut);
    }
    else {
        getPoseWorldSpace(poseWorldSpaceOut);
    }
}

void hkaRagdollInstance::setPoseModelSpace(const hkQsTransform* poseIn, const hkQsTransform& worldFromModel) {
    hkaRagdollInstance* pThis = this;
    int32_t v3 = 0;
    hkaRagdollInstance* v14 = pThis;
    int32_t position = pThis->m_skeleton->m_bones.m_size;
    if (position > 0) {
        do {
            int32_t v5 = pThis->m_boneToRigidBodyMap.m_data[v3];
            if (v5 >= 0) {
                hkpRigidBody* v6 = pThis->m_rigidBodies.m_data[v5];
                if (v6) {
                    hkQuaternion v7;
                    hkVector4 v16;
                    hkQuaternion rotation_4[2];
                    v7.m_vec.m_quad = worldFromModel.m_rotation.m_vec.m_quad;
                    __m128 v8 = _mm_mul_ps(poseIn->m_translation.m_quad, v7.m_vec.m_quad);
                    __m128 v9 = _mm_shuffle_ps(v8, v8, 170);
                    v9[0] = v9[0] + (_mm_shuffle_ps(v8, v8, 85)[0] + v8[0]);
                    __m128 v10 = _mm_shuffle_ps(v7.m_vec.m_quad, v7.m_vec.m_quad, 255);
                    __m128 v11 = _mm_add_ps(
                        _mm_mul_ps(
                            _mm_sub_ps(
                                _mm_mul_ps(
                                    _mm_shuffle_ps(poseIn->m_translation.m_quad, poseIn->m_translation.m_quad, 210),
                                    _mm_shuffle_ps(v7.m_vec.m_quad, v7.m_vec.m_quad, 201)
                                ),
                                _mm_mul_ps(
                                    _mm_shuffle_ps(poseIn->m_translation.m_quad, poseIn->m_translation.m_quad, 201),
                                    _mm_shuffle_ps(v7.m_vec.m_quad, v7.m_vec.m_quad, 210)
                                )
                            ),
                            v10
                        ),
                        _mm_add_ps(
                            _mm_mul_ps(
                                _mm_add_ps(_mm_mul_ps(v10, v10), hkQuadRealMinusHalf), poseIn->m_translation.m_quad
                            ),
                            _mm_mul_ps(_mm_shuffle_ps(v9, v9, 0), v7.m_vec.m_quad)
                        )
                    );
                    v16.m_quad = _mm_add_ps(_mm_add_ps(v11, v11), worldFromModel.m_translation.m_quad);
                    rotation_4->setMul(worldFromModel.m_rotation, poseIn->m_rotation);
                    rotation_4[1].m_vec.m_quad = _mm_mul_ps(worldFromModel.m_scale.m_quad, poseIn->m_scale.m_quad);
                    rotation_4->normalize();
                    v6->setPositionAndRotation(v16, *rotation_4);
                    __m128 m_quad = hkVector4Zero.m_quad;
                    __m128 v13 = _mm_shuffle_ps(
                        _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)), _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)), 0
                    );
                    if ((_mm_movemask_ps(_mm_cmple_ps(
                             _mm_and_ps(
                                 _mm_sub_ps(hkVector4Zero.m_quad, v6->m_motion.m_linearVelocity.m_quad),
                                 hkVector4_setAbs4_absMask
                             ),
                             v13
                         )) &
                         15) != 15) {
                        v6->activate();
                        v6->m_motion.setLinearVelocity(hkVector4Zero);
                        m_quad = hkVector4Zero.m_quad;
                    }
                    if ((_mm_movemask_ps(_mm_cmple_ps(
                             _mm_and_ps(
                                 _mm_sub_ps(m_quad, v6->m_motion.m_angularVelocity.m_quad), hkVector4_setAbs4_absMask
                             ),
                             v13
                         )) &
                         15) != 15) {
                        v6->activate();
                        v6->m_motion.setAngularVelocity(hkVector4Zero);
                    }
                    pThis = v14;
                }
            }
            ++v3;
            ++poseIn;
        }
        while (v3 < position);
    }
}

// Per bone with a rigid body: both world transforms (worldFromModel * pose), the body moved to pose B's, and each
// velocity set (activate, then the motion's linear and angular velocity setters) only when some lane differs from the
// motion's current one by more than 0.001f. The angular velocity is quatDif's axis times its angle times invTimestep
// when the imaginary part's length is above 1.1920929e-07f (NaN gives zero), else zero.
void hkaRagdollInstance::setPoseAndVelocitiesModelSpace(
    const hkQsTransform* poseModelSpaceA,
    const hkQsTransform& worldFromModelA,
    const hkQsTransform* poseModelSpaceB,
    const hkQsTransform& worldFromModelB,
    float timestep
) {
    const float invTimestep = 1.0f / timestep;
    const int32_t iNumBones = m_skeleton->m_bones.m_size;
    for (int32_t i = 0; i < iNumBones; i++) {
        const int32_t iRigidBody = m_boneToRigidBodyMap.m_data[i];
        if (iRigidBody < 0) {
            continue;
        }
        hkpRigidBody* pRigidBody = m_rigidBodies.m_data[iRigidBody];
        if (!pRigidBody) {
            continue;
        }
        alignas(16) hkQsTransform rbTransformA;
        rbTransformA.m_translation.setRotatedDir(worldFromModelA.m_rotation, poseModelSpaceA[i].m_translation);
        rbTransformA.m_translation.add4(worldFromModelA.m_translation);
        rbTransformA.m_rotation.setMul(worldFromModelA.m_rotation, poseModelSpaceA[i].m_rotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rbTransformA.m_scale.m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&worldFromModelA.m_scale.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&poseModelSpaceA[i].m_scale.m_quad)[0])
            )
        );
        rbTransformA.m_rotation.normalize();

        alignas(16) hkQsTransform rbTransformB;
        rbTransformB.m_translation.setRotatedDir(worldFromModelB.m_rotation, poseModelSpaceB[i].m_translation);
        rbTransformB.m_translation.add4(worldFromModelB.m_translation);
        rbTransformB.m_rotation.setMul(worldFromModelB.m_rotation, poseModelSpaceB[i].m_rotation);
        _mm_store_ps(
            &reinterpret_cast<float*>(&rbTransformB.m_scale.m_quad)[0],
            _mm_mul_ps(
                _mm_load_ps(&reinterpret_cast<const float*>(&poseModelSpaceB[i].m_scale.m_quad)[0]),
                _mm_load_ps(&reinterpret_cast<const float*>(&worldFromModelB.m_scale.m_quad)[0])
            )
        );
        rbTransformB.m_rotation.normalize();

        pRigidBody->setPositionAndRotation(rbTransformB.m_translation, rbTransformB.m_rotation);

        hkpMotion* pMotion = &pRigidBody->m_motion;
        alignas(16) hkVector4 linearVelocity;
        _mm_store_ps(
            &reinterpret_cast<float*>(&linearVelocity.m_quad)[0],
            _mm_mul_ps(
                _mm_set1_ps(invTimestep),
                _mm_sub_ps(
                    _mm_load_ps(&reinterpret_cast<float*>(&rbTransformB.m_translation.m_quad)[0]),
                    _mm_load_ps(&reinterpret_cast<float*>(&rbTransformA.m_translation.m_quad)[0])
                )
            )
        );
        if (!linearVelocity.equals4(pRigidBody->m_motion.m_linearVelocity, 0.001f)) {
            // EXE-INLINED 0x5616D0
            pRigidBody->activate();
            pMotion->setLinearVelocity(linearVelocity);
        }

        alignas(16) hkQuaternion quatDif;
        quatDif.setMulInverse(rbTransformB.m_rotation, rbTransformA.m_rotation);
        quatDif.normalize();
        const __m128 kImag = _mm_load_ps(&reinterpret_cast<float*>(&quatDif.m_vec.m_quad)[0]);
        const __m128 kSquares = _mm_mul_ps(kImag, kImag);
        const float fImagLength = _mm_cvtss_f32(_mm_sqrt_ss(_mm_add_ss(
            _mm_shuffle_ps(kSquares, kSquares, 0xAA), _mm_add_ss(_mm_shuffle_ps(kSquares, kSquares, 0x55), kSquares)
        )));
        alignas(16) hkVector4 angularVelocity;
        if (1.1920929e-07f < fImagLength) {
            quatDif.getAxis(angularVelocity);
            const float fAngle = static_cast<float>(quatDif.getAngle() * invTimestep);
            _mm_store_ps(
                &reinterpret_cast<float*>(&angularVelocity.m_quad)[0],
                _mm_mul_ps(_mm_set1_ps(fAngle), _mm_load_ps(&reinterpret_cast<float*>(&angularVelocity.m_quad)[0]))
            );
        }
        else {
            _mm_store_ps(&reinterpret_cast<float*>(&angularVelocity.m_quad)[0], _mm_setzero_ps());
        }
        if (!angularVelocity.equals4(pRigidBody->m_motion.m_angularVelocity, 0.001f)) {
            // EXE-INLINED 0x561800
            pRigidBody->activate();
            pMotion->setAngularVelocity(angularVelocity);
        }
    }
}

void hkaRagdollInstance::getPoseModelSpace(hkQsTransform* poseOut, const hkQsTransform& a2) const {
    hkQuaternion v6;
    hkQuaternion result;
    hkQsTransform* v3 = poseOut;
    getPoseWorldSpace(poseOut);
    int32_t m_size = m_skeleton->m_bones.m_size;
    v6.m_vec.m_quad = a2.m_rotation.m_vec.m_quad;
    __m128 v7 = _mm_mul_ps(a2.m_translation.m_quad, v6.m_vec.m_quad);
    __m128 v8 = _mm_shuffle_ps(v7, v7, 170);
    v8[0] = v8[0] + (_mm_shuffle_ps(v7, v7, 85)[0] + v7[0]);
    __m128 v9 = _mm_shuffle_ps(v6.m_vec.m_quad, v6.m_vec.m_quad, 255);
    __m128 v10 = _mm_add_ps(
        _mm_mul_ps(
            _mm_sub_ps(
                _mm_mul_ps(
                    _mm_shuffle_ps(a2.m_translation.m_quad, a2.m_translation.m_quad, 201),
                    _mm_shuffle_ps(v6.m_vec.m_quad, v6.m_vec.m_quad, 210)
                ),
                _mm_mul_ps(
                    _mm_shuffle_ps(a2.m_translation.m_quad, a2.m_translation.m_quad, 210),
                    _mm_shuffle_ps(v6.m_vec.m_quad, v6.m_vec.m_quad, 201)
                )
            ),
            v9
        ),
        _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(v9, v9), hkQuadRealMinusHalf), a2.m_translation.m_quad),
            _mm_mul_ps(_mm_shuffle_ps(v8, v8, 0), v6.m_vec.m_quad)
        )
    );
    __m128 v11 = _mm_xor_ps(_mm_add_ps(v10, v10), hkQuadRealNegateMask);
    __m128 m_quad = _mm_xor_ps(v6.m_vec.m_quad, rdata_10C4C00);
    __m128 v13 = _mm_shuffle_ps(a2.m_scale.m_quad, _mm_unpackhi_ps(a2.m_scale.m_quad, hkQuadReal1111), 196);
    __m128 v14 = _mm_rcp_ps(v13);
    __m128 v24 = v11;
    result.m_vec.m_quad = m_quad;
    __m128 v26 = _mm_and_ps(_mm_sub_ps(_mm_add_ps(v14, v14), _mm_mul_ps(_mm_mul_ps(v14, v13), v14)), rdata_10C4C10);
    if (m_size > 0) {
        __m128 v15 = _mm_shuffle_ps(m_quad, m_quad, 255);
        __m128 v22 = _mm_shuffle_ps(m_quad, m_quad, 210);
        __m128 v16 = _mm_shuffle_ps(m_quad, m_quad, 201);
        __m128 v20 = v15;
        __m128 v21 = v16;
        __m128 v23 = _mm_mul_ps(v15, v15);
        while (true) {
            __m128 v17 = _mm_mul_ps(v3->m_translation.m_quad, m_quad);
            __m128 v18 = _mm_shuffle_ps(v17, v17, 170);
            v18[0] = v18[0] + (_mm_shuffle_ps(v17, v17, 85)[0] + v17[0]);
            __m128 v19 = _mm_add_ps(
                _mm_mul_ps(
                    _mm_sub_ps(
                        _mm_mul_ps(_mm_shuffle_ps(v3->m_translation.m_quad, v3->m_translation.m_quad, 210), v16),
                        _mm_mul_ps(_mm_shuffle_ps(v3->m_translation.m_quad, v3->m_translation.m_quad, 201), v22)
                    ),
                    v15
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(v18, v18, 0), m_quad),
                    _mm_mul_ps(_mm_add_ps(v23, hkQuadRealMinusHalf), v3->m_translation.m_quad)
                )
            );
            v3->m_translation.m_quad = _mm_add_ps(_mm_add_ps(v19, v19), v11);
            v3->m_rotation.setMul(result, v3->m_rotation);
            v3->m_scale.m_quad = _mm_mul_ps(v26, v3->m_scale.m_quad);
            ++v3;
            if (!--m_size) {
                break;
            }
            v15 = v20;
            m_quad = result.m_vec.m_quad;
            v11 = v24;
            v16 = v21;
        }
    }
}

// Per bone, a mapped rigid body's world transform with the bone's model-space scale, else the parent's world transform
// (worldFromModel for a root) times the local pose (the given one, else the reference pose).
void hkaRagdollInstance::getPoseWorldSpace(
    hkQsTransform* poseOut,
    const hkQsTransform& worldFromModel,
    const hkQsTransform* localSpacePoseForUnmappedBonesOptional
) const {
    const hkQsTransform* localPose = localSpacePoseForUnmappedBonesOptional;
    if (!localPose) {
        localPose = m_skeleton->m_referencePose.m_data;
    }
    for (int32_t boneId = 0; boneId < m_skeleton->m_bones.m_size; ++boneId) {
        // EXE-INLINED 0xC76220
        const int32_t rbIndex = m_boneToRigidBodyMap.m_data[boneId];
        const hkpRigidBody* rb = rbIndex >= 0 ? m_rigidBodies.m_data[rbIndex] : nullptr;
        if (rb) {
            poseOut[boneId].setFromTransformNoScale(rb->m_motion.m_motionState.m_transform);
            hkaSkeletonUtils::getModelSpaceScale(
                *m_skeleton, m_skeleton->m_referencePose.m_data, boneId, poseOut[boneId].m_scale
            );
        }
        else {
            const int32_t parentId = m_skeleton->m_parentIndices.m_data[boneId];
            alignas(16) const hkQsTransform worldFromParent = parentId < 0 ? worldFromModel : poseOut[parentId];
            poseOut[boneId].setMul(worldFromParent, localPose[boneId]);
        }
    }
}

// The skeleton's reference is swapped, both arrays are copied and a reference is added to every body and constraint
// there is.
void hkaRagdollInstance::commonInit(
    const hkArrayBase<hkpRigidBody*>& rigidBodies,
    const hkArrayBase<hkpConstraintInstance*>& constraints,
    const hkaSkeleton* skeleton
) {
    if (skeleton) {
        skeleton->addReference();
    }
    if (m_skeleton) {
        m_skeleton->removeReference();
    }
    m_skeleton = skeleton;
    // EXE-INLINED none: hkArray<hkpRigidBody*>::operator=
    m_rigidBodies.copyFromArray(hkContainerHeapAllocator().get(&m_rigidBodies), rigidBodies, hkTypeIsPod());
    // EXE-INLINED none: hkArray<hkpConstraintInstance*>::operator=
    m_constraints.copyFromArray(hkContainerHeapAllocator().get(&m_constraints), constraints, hkTypeIsPod());
    for (hkpRigidBody* pRigidBody : m_rigidBodies) {
        if (pRigidBody) {
            pRigidBody->addReference();
        }
    }
    for (hkpConstraintInstance* pConstraint : m_constraints) {
        if (pConstraint) {
            pConstraint->addReference();
        }
    }
}

void hkaRagdollInstance::setPoseModelSpaceZeroVelocities(hkQuaternion* position) {
    int32_t v3 = 0;
    int32_t m_size = m_skeleton->m_bones.m_size;
    if (m_size > 0) {
        hkQuaternion* v4 = position;
        hkQuaternion* v9 = position;
        do {
            int32_t v5 = m_boneToRigidBodyMap.m_data[v3];
            if (v5 >= 0) {
                hkpRigidBody* v6 = m_rigidBodies.m_data[v5];
                if (v6) {
                    v6->setPositionAndRotation(v4->m_vec, *(v4 + 1));
                    __m128 m_quad = hkVector4Zero.m_quad;
                    __m128 v8 = _mm_shuffle_ps(
                        _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)), _mm_set_ss(std::bit_cast<float>(0x3A83126Fu)), 0
                    );
                    if ((_mm_movemask_ps(_mm_cmple_ps(
                             _mm_and_ps(
                                 _mm_sub_ps(hkVector4Zero.m_quad, v6->m_motion.m_linearVelocity.m_quad),
                                 hkVector4_setAbs4_absMask
                             ),
                             v8
                         )) &
                         15) != 15) {
                        v6->activate();
                        v6->m_motion.setLinearVelocity(hkVector4Zero);
                        m_quad = hkVector4Zero.m_quad;
                    }
                    if ((_mm_movemask_ps(_mm_cmple_ps(
                             _mm_and_ps(
                                 _mm_sub_ps(m_quad, v6->m_motion.m_angularVelocity.m_quad), hkVector4_setAbs4_absMask
                             ),
                             v8
                         )) &
                         15) != 15) {
                        v6->activate();
                        v6->m_motion.setAngularVelocity(hkVector4Zero);
                    }
                }
            }
            ++v3;
            v4 = v9 + 3;
            v9 += 3;
        }
        while (v3 < m_size);
    }
}

// Havok SDK: fails when the first body is in a world already; adds the bodies as one batch, then the constraints,
// then rechecks each body's entity-entity pairs when asked.
hkResult hkaRagdollInstance::addToWorld(hkpWorld* world, hkBool updateFilter) const {
    hkpRigidBody** ppRigidBodies = m_rigidBodies.m_data;
    if (*ppRigidBodies && (*ppRigidBodies)->m_world) {
        return HK_FAILURE;
    }
    // The rigid body array is handed to the entity batch call unchanged: an hkpRigidBody is an hkpEntity and both
    // arrays hold pointers, so the same address is passed. The untyped local is that hand-over.
    void* pBatch = ppRigidBodies;
    world->addEntityBatch(
        static_cast<hkpEntity* const*>(pBatch), m_rigidBodies.m_size, HK_ENTITY_ACTIVATION_DO_ACTIVATE
    );
    for (int32_t i = 0; i < m_constraints.m_size; ++i) {
        world->addConstraint(m_constraints.m_data[i]);
    }
    if (updateFilter.m_bool) {
        const int32_t iNumRigidBodies = m_rigidBodies.m_size;
        for (int32_t j = 0; j < iNumRigidBodies; ++j) {
            world->updateCollisionFilterOnEntity(
                m_rigidBodies.m_data[j], HK_UPDATE_FILTER_ON_ENTITY_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY,
                HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS
            );
        }
    }
    return HK_SUCCESS;
}

// Havok SDK: fails when the first body is in no world; removes the constraints, then the bodies as one batch.
hkResult hkaRagdollInstance::removeFromWorld() const {
    hkpRigidBody* pFirstRigidBody = *m_rigidBodies.m_data;
    if (!pFirstRigidBody) {
        return HK_FAILURE;
    }
    hkpWorld* pWorld = pFirstRigidBody->m_world;
    if (!pWorld) {
        return HK_FAILURE;
    }
    for (int32_t i = 0; i < m_constraints.m_size; ++i) {
        pWorld->removeConstraint(m_constraints.m_data[i]);
    }
    // The rigid body array handed over unchanged, as in addToWorld.
    void* pBatch = m_rigidBodies.m_data;
    pWorld->removeEntityBatch(static_cast<hkpEntity* const*>(pBatch), m_rigidBodies.m_size);
    return HK_SUCCESS;
}

// A bone with a rigid body takes the body's transform and its model-space scale; any other bone is its reference pose
// in its parent's frame (the identity for a root), found recursively. setMul's setRotatedDir is written out with the
// dot product summed z + (y + x), followed by a call to hkQuaternion::setMul, since a different summation order rounds
// a denormal differently under 24-bit precision.
void hkaRagdollInstance::getWorldFromBoneTransform(int32_t i, hkQsTransform& worldFromBoneOut) const {
    hkpRigidBody* pRigidBody;
    const int32_t iRigidBody = m_boneToRigidBodyMap.m_data[i];
    if (iRigidBody >= 0 && (pRigidBody = m_rigidBodies.m_data[iRigidBody]) != nullptr) {
        worldFromBoneOut.setFromTransformNoScale(pRigidBody->m_motion.m_motionState.m_transform);
        hkaSkeletonUtils::getModelSpaceScale(
            *m_skeleton, m_skeleton->m_referencePose.begin(), i, worldFromBoneOut.m_scale
        );
    }
    else {
        hkQsTransform worldFromParent;
        const int32_t parentId = m_skeleton->m_parentIndices.m_data[i];
        if (parentId < 0) {
            worldFromParent.m_translation.m_quad = _mm_setzero_ps();
            worldFromParent.m_rotation.setIdentity();
            worldFromParent.m_scale.m_quad = _mm_set1_ps(1.0f);
        }
        else {
            getWorldFromBoneTransform(parentId, worldFromParent);
        }
        const hkQsTransform& boneFromRef = m_skeleton->m_referencePose.m_data[i];
        const hkQuadReal q = worldFromParent.m_rotation.m_vec.m_quad;
        const hkQuadReal v = boneFromRef.m_translation.m_quad;
        const hkQuadReal p = _mm_mul_ps(v, q);
        const hkQuadReal dot = _mm_add_ss(_mm_shuffle_ps(p, p, 0xAA), _mm_add_ss(_mm_shuffle_ps(p, p, 0x55), p));
        const hkQuadReal w = _mm_shuffle_ps(q, q, 0xFF);
        const hkQuadReal cross = _mm_sub_ps(
            _mm_mul_ps(_mm_shuffle_ps(v, v, 0xD2), _mm_shuffle_ps(q, q, 0xC9)),
            _mm_mul_ps(_mm_shuffle_ps(v, v, 0xC9), _mm_shuffle_ps(q, q, 0xD2))
        );
        const hkQuadReal scaled = _mm_add_ps(
            _mm_mul_ps(_mm_add_ps(_mm_mul_ps(w, w), hkQuadRealMinusHalf), v), _mm_mul_ps(_mm_shuffle_ps(dot, dot, 0), q)
        );
        const hkQuadReal rotated = _mm_add_ps(_mm_mul_ps(cross, w), scaled);
        worldFromBoneOut.m_translation.m_quad =
            _mm_add_ps(_mm_add_ps(rotated, rotated), worldFromParent.m_translation.m_quad);
        worldFromBoneOut.m_rotation.setMul(worldFromParent.m_rotation, boneFromRef.m_rotation);
        worldFromBoneOut.m_scale.m_quad = _mm_mul_ps(boneFromRef.m_scale.m_quad, worldFromParent.m_scale.m_quad);
    }
}

const hkClass& hkaRagdollInstance::staticClass() {
    return hkaRagdollInstanceClass;
}
