#include "Havok/Source/Animation/Ragdoll/Penetration/hkaDetectRagdollPenetration.h"

#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Animation/Animation/Rig/hkaBone.h"
#include "Havok/Source/Animation/Animation/Rig/hkaPose.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Animation/Animation/Rig/hkaSkeletonUtils.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/Geometry/Aabb/hkAabb.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"

#include <xmmintrin.h>

hkaDetectRagdollPenetration::BonePenetration::BonePenetration() : m_boneBeginIndex(0), m_boneEndIndex(1) {
    m_penetrationPoint.setZero4();
    m_penetrationNormal.setZero4();
}

// The penetrations free themselves.
hkaDetectRagdollPenetration::Output::~Output() {}

hkaDetectRagdollPenetration::hkaDetectRagdollPenetration(hkaDetectRagdollPenetration::Setup* a1)
  : m_setup(*a1), m_numBones(a1->m_ragdollSkeleton->m_bones.getSize()) {
    // Every bone starts out penetrating.
    m_boneStates.setSize(m_numBones);
    for (int32_t i = 0; i < m_numBones; ++i) {
        m_boneStates[i] = HK_YESP;
    }
    m_ragdollIsOut.m_bool = false;
}

// The bone states free themselves.
hkaDetectRagdollPenetration::~hkaDetectRagdollPenetration() {}

void hkaDetectRagdollPenetration::resetBoneStates() {
    // Havok's getSize() is an int, so the compare is signed.
    for (int32_t i = 0; i < m_boneStates.m_size; i++) {
        m_boneStates.m_data[i] = HK_YESP;
    }
    m_ragdollIsOut.m_bool = false;
}

// The box and the penetration a ray finds share one 16-aligned storage area, since their lifetimes do not overlap. The
// penetration's padding after its two bone indices still holds the box's m_min y, z and w, and the copy into arOutput
// carries them along, as the game does. The union is that storage.
hkBool hkaDetectRagdollPenetration::detectPenetration(
    const hkaPose& arRagdollPoseIn,
    hkaDetectRagdollPenetration::Output& arOutput
) {
    union Slot {
        Slot() {}

        hkAabb aabb;
        BonePenetration pen;
    };
    arOutput.m_penetrations.m_size = 0;
    if (!m_ragdollIsOut.m_bool) {
        Slot slot;
        slot.aabb.m_min.m_quad = _mm_shuffle_ps(_mm_set_ss(rdata_104DA48), _mm_set_ss(rdata_104DA48), 0);
        slot.aabb.m_max.m_quad = _mm_shuffle_ps(_mm_set_ss(rdata_104E148), _mm_set_ss(rdata_104E148), 0);
        for (int16_t boneIdx = 1; boneIdx < m_numBones; ++boneIdx) {
            const int16_t parentIdx = m_setup.m_ragdollSkeleton->m_parentIndices.m_data[boneIdx];
            if (m_boneStates.m_data[parentIdx]) {
                // hkaPose::getBoneModelSpace, inline; the parent's translation is copied before the bone's is computed.
                const hkQsTransform* pParent;
                if ((arRagdollPoseIn.m_boneFlags.m_data[parentIdx] & hkaPose::F_BONE_MODEL_DIRTY) != 0) {
                    pParent = &arRagdollPoseIn.calculateBoneModelSpace(parentIdx);
                }
                else {
                    pParent = &arRagdollPoseIn.m_modelPose.m_data[parentIdx];
                }
                hkVector4 parentPos = pParent->m_translation;
                const hkQsTransform* pBone;
                if ((arRagdollPoseIn.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) != 0) {
                    pBone = &arRagdollPoseIn.calculateBoneModelSpace(boneIdx);
                }
                else {
                    pBone = &arRagdollPoseIn.m_modelPose.m_data[boneIdx];
                }
                slot.aabb.includePoint(parentPos);
                slot.aabb.includePoint(pBone->m_translation);
            }
        }
        if (slot.aabb.isValid().m_bool) {
            m_setup.m_ragdollPhantom->setAabb(slot.aabb);
        }
        for (int16_t boneIdx = 1; boneIdx < m_numBones; ++boneIdx) {
            const int16_t parentIdx = m_setup.m_ragdollSkeleton->m_parentIndices.m_data[boneIdx];
            if (m_boneStates.m_data[parentIdx]) {
                hkVector4 arToWS_;
                hkVector4 arHitNormalWS_;
                hkVector4 arHitPointWS_;
                hkVector4 arFromWS_;
                const hkQsTransform* pParent;
                if ((arRagdollPoseIn.m_boneFlags.m_data[parentIdx] & hkaPose::F_BONE_MODEL_DIRTY) != 0) {
                    pParent = &arRagdollPoseIn.calculateBoneModelSpace(parentIdx);
                }
                else {
                    pParent = &arRagdollPoseIn.m_modelPose.m_data[parentIdx];
                }
                arFromWS_ = pParent->m_translation;
                const hkQsTransform* pBone;
                if ((arRagdollPoseIn.m_boneFlags.m_data[boneIdx] & hkaPose::F_BONE_MODEL_DIRTY) != 0) {
                    pBone = &arRagdollPoseIn.calculateBoneModelSpace(boneIdx);
                }
                else {
                    pBone = &arRagdollPoseIn.m_modelPose.m_data[boneIdx];
                }
                arToWS_ = pBone->m_translation;
                hkBool apResult = m_setup.m_raycastInterface->castRay(
                    m_setup.m_ragdollPhantom, arFromWS_, arToWS_, arHitPointWS_, arHitNormalWS_
                );
                if (apResult.m_bool) {
                    slot.pen.~BonePenetration();
                    ::new (&slot.pen) BonePenetration;
                    slot.pen.m_penetrationPoint = arHitPointWS_;
                    slot.pen.m_boneBeginIndex = parentIdx;
                    slot.pen.m_boneEndIndex = boneIdx;
                    slot.pen.m_penetrationNormal = arHitNormalWS_;
                    // hkArray::pushBack, inline.
                    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
                    if (arOutput.m_penetrations.m_size ==
                        (arOutput.m_penetrations.m_capacityAndFlags & hkArrayBase<BonePenetration>::CAPACITY_MASK)) {
                        hkArrayUtil::_reserveMore(*Instance->m_heap, &arOutput.m_penetrations, sizeof(BonePenetration));
                    }
                    BonePenetration* pSlot = &arOutput.m_penetrations.m_data[arOutput.m_penetrations.m_size];
                    if (pSlot) {
                        *pSlot = slot.pen;
                    }
                    ++arOutput.m_penetrations.m_size;
                }
            }
        }
    }
    for (int32_t i = 0; i < m_boneStates.m_size; ++i) {
        m_boneStates.m_data[i] = hkaDetectRagdollPenetration::HK_NOP;
    }
    if (arOutput.m_penetrations.m_size) {
        for (int32_t i = 0; i < arOutput.m_penetrations.m_size; ++i) {
            const int16_t boneBeginIdx = arOutput.m_penetrations.m_data[i].m_boneBeginIndex;
            if (m_boneStates.m_data[boneBeginIdx] != hkaDetectRagdollPenetration::HK_YESP_DESCENDANT) {
                m_boneStates.m_data[boneBeginIdx] = hkaDetectRagdollPenetration::HK_YESP;
            }
            hkArray<int16_t> bonesOut;
            hkaSkeletonUtils::getDescendants(*m_setup.m_ragdollSkeleton, boneBeginIdx, bonesOut, false);
            for (int32_t j = 0; j < bonesOut.m_size; ++j) {
                if (m_boneStates.m_data[bonesOut.m_data[j]] != hkaDetectRagdollPenetration::HK_YESP_DESCENDANT) {
                    m_boneStates.m_data[bonesOut.m_data[j]] = hkaDetectRagdollPenetration::HK_YESP_DESCENDANT;
                }
            }
        }
        return 1;
    }

    m_ragdollIsOut.m_bool = 1;
    return 0;
}
