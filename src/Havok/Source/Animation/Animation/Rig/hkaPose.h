#pragma once

#include "Havok/Source/Animation/Animation/Rig/hkaSkeleton.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stddef.h>

#include <stdint.h>
#include <xmmintrin.h>

uint8_t* returnTrue(uint8_t* result, int32_t a2);

// A skeleton's pose in local (parent-relative) and model space, kept in sync on demand, with per-bone dirty flags and
// the float slot values.
class hkaPose {
public:
    enum PoseSpace : int32_t {
        MODEL_SPACE = 0x0,
        LOCAL_SPACE = 0x1,
    };

    enum PropagateOrNot : int32_t {
        DONT_PROPAGATE = 0x0,
        PROPAGATE = 0x1,
    };

    enum : int32_t {
        F_BONE_LOCAL_DIRTY = 0x1,
        F_BONE_MODEL_DIRTY = 0x2,
        F_BONE_INTERNAL_FLAG1 = 0x4,
        F_BONE_INTERNAL_FLAG2 = 0x8,
        F_BONE_INTERNAL_FLAG3 = 0x10,
    };

    // The default constructor leaves the members to the callers to fill.
    hkaPose() = default;
    // Sizes the pose arrays to the skeleton's bones and float slots, the float slot values zeroed.
    hkaPose(const hkaSkeleton* skeleton);
    hkaPose(hkaPose::PoseSpace space, const hkaSkeleton* skeleton, const hkArrayBase<hkQsTransform>& pose);
    hkaPose(hkaPose::PoseSpace space, const hkaSkeleton* skeleton, const hkQsTransform* pose, int32_t numBones);
    // The members' destructors free the four arrays.
    ~hkaPose();

    // Accesses the model-space pose without synchronizing it.
    hkArray<hkQsTransform>& accessUnsyncedPoseModelSpace();
    // Synchronizes the model-space pose.
    void syncModelSpace();
    // Synchronizes the local-space pose.
    void syncLocalSpace();
    // The model-space transform of bone boneIdx, built from its parent's and cached; called where the bone's
    // F_BONE_MODEL_DIRTY flag is set.
    const hkQsTransform& calculateBoneModelSpace(int32_t theBoneIdx) const;
    // The model-space transform of bone boneIdx for writing, synchronized first; its
    // descendants are marked to follow it (PROPAGATE) or to keep their model space.
    hkQsTransform& accessBoneModelSpace(int32_t boneIdx, PropagateOrNot propagateOrNot);
    const hkQsTransform& getBoneModelSpace(int32_t boneIdx) const;
    // The local transform of bone boneIdx, rebuilt from its model transform and its parent's when it is dirty.
    const hkQsTransform& getBoneLocalSpace(int32_t boneIdx) const;
    void setBoneLocalSpace(int32_t boneIdx, const hkQsTransform& boneLocal);
    void setBoneModelSpace(int32_t boneIdx, const hkQsTransform& boneModel, PropagateOrNot propagateOrNot);
    // The descendants of boneIdx get their local transforms and are marked model-dirty, so that they follow it.
    void makeAllChildrenLocalSpace(int32_t boneIdx) const;
    // The children of boneIdx get their model transforms and are marked local-dirty, so that they keep them.
    void makeFirstChildrenModelSpace(int32_t boneIdx) const;
    // The model-space pose, synchronized first, for writing; every bone is marked locally dirty.
    hkArray<hkQsTransform>& accessSyncedPoseModelSpace();
    // The model-space pose, synchronized first (the SDK's const member: its pose arrays are mutable there).
    const hkArray<hkQsTransform>& getSyncedPoseModelSpace() const;
    // The local-space pose, synchronized first (const as in the SDK).
    const hkArray<hkQsTransform>& getSyncedPoseLocalSpace() const;
    // Whether no bone is dirty in both spaces, and no bone is dirty in a space marked in sync.
    hkBool checkPoseValidity() const;
    void setToReferencePose();
    hkQsTransform& accessBoneLocalSpace(int32_t boneIndex);
    // The translation-locked bones take the reference pose's translation in model space (from their parent's model
    // transform); their descendants follow.
    void enforceSkeletonConstraintsModelSpace();
    hkArray<hkQsTransform, hkContainerHeapAllocator>& accessSyncedPoseLocalSpace();
    // The same in local space.
    void enforceSkeletonConstraintsLocalSpace();
    int32_t setNonInitializedFlags();
    // Copies the local space pose from poseLocal and flags every bone model-dirty. Returns the bone count.
    int32_t setPoseLocalSpace(const hkArrayBase<hkQsTransform>& poseLocal);
    // Havok SDK: setPoseModelSpace, the same for the model space pose, every bone flagged local-dirty.
    int32_t setPoseModelSpace(const hkArrayBase<hkQsTransform>& poseModel);
    uint8_t* checkPoseTransformsValidity(uint8_t* result);
    void getModelSpaceAabb(__m128* aabbOut);
    // Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc): sizes the pose arrays to m_skeleton's bones and
    // float slots, then sets the given pose in its space.
    void init(hkaPose::PoseSpace space, const hkaSkeleton* skeleton, const hkArrayBase<hkQsTransform>& pose);

    // hkaPose's class allocator: the block comes from Havok's heap and goes back to it.
    static void* operator new(size_t auiSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(auiSize));
    }
    static void operator delete(void* apMemory);

    const hkaSkeleton* m_skeleton;
    mutable hkArray<hkQsTransform> m_localPose;
    mutable hkArray<hkQsTransform> m_modelPose;
    mutable hkArray<uint32_t> m_boneFlags;
    mutable hkBool m_modelInSync;
    mutable hkBool m_localInSync;
    hkArray<float> m_floatSlotValues;
};

inline void hkaPose::operator delete(void* apMemory) {
    if (apMemory) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, sizeof(hkaPose));
    }
}

inline hkaPose::hkaPose(const hkaSkeleton* skeleton)
  : m_skeleton(skeleton), m_modelInSync(false), m_localInSync(false) {
    const int32_t numBones = m_skeleton->m_bones.m_size;
    m_modelPose.setSize(numBones);
    m_localPose.setSize(numBones);
    m_boneFlags.setSize(numBones);
    m_floatSlotValues.reserveExactly((m_skeleton->m_floatSlots.m_size + 3) & ~3);
    m_floatSlotValues.setSize(m_skeleton->m_floatSlots.m_size, 0.0f);
}

inline const hkQsTransform& hkaPose::getBoneModelSpace(int32_t boneIdx) const {
    if (!(m_boneFlags.m_data[boneIdx] & F_BONE_MODEL_DIRTY)) {
        return m_modelPose.m_data[boneIdx];
    }
    return calculateBoneModelSpace(boneIdx);
}

// The model transform's address is taken before the parent's model transform is computed, the local transform's
// after.
inline const hkQsTransform& hkaPose::getBoneLocalSpace(int32_t boneIdx) const {
    if (!(m_boneFlags.m_data[boneIdx] & F_BONE_LOCAL_DIRTY)) {
        return m_localPose.m_data[boneIdx];
    }
    const int16_t parentIdx = m_skeleton->m_parentIndices.m_data[boneIdx];
    const hkQsTransform& modelFromBone = m_modelPose.m_data[boneIdx];
    if (parentIdx != -1) {
        const hkQsTransform& modelFromParent = getBoneModelSpace(parentIdx);
        m_localPose.m_data[boneIdx].setMulInverseMul(modelFromParent, modelFromBone);
    }
    else {
        m_localPose.m_data[boneIdx] = modelFromBone;
    }
    m_boneFlags.m_data[boneIdx] &= ~static_cast<uint32_t>(F_BONE_LOCAL_DIRTY);
    return m_localPose.m_data[boneIdx];
}

// The bone count is read once.
inline void hkaPose::makeAllChildrenLocalSpace(int32_t boneIdx) const {
    const int32_t numBones = m_skeleton->m_bones.m_size;
    m_boneFlags.m_data[boneIdx] |= F_BONE_INTERNAL_FLAG1;
    for (int32_t i = boneIdx + 1; i < numBones; ++i) {
        const int16_t parentId = m_skeleton->m_parentIndices.m_data[i];
        if (parentId != -1 && (m_boneFlags.m_data[parentId] & F_BONE_INTERNAL_FLAG1)) {
            getBoneLocalSpace(i);
            m_boneFlags.m_data[i] |= F_BONE_INTERNAL_FLAG1;
            m_modelInSync = false;
        }
    }
    for (int32_t i = boneIdx + 1; i < numBones; ++i) {
        uint32_t& ruiFlags = m_boneFlags.m_data[i];
        if (ruiFlags & F_BONE_INTERNAL_FLAG1) {
            ruiFlags = (ruiFlags & ~static_cast<uint32_t>(F_BONE_INTERNAL_FLAG1)) | F_BONE_MODEL_DIRTY;
        }
    }
}

inline void hkaPose::makeFirstChildrenModelSpace(int32_t boneIdx) const {
    const int32_t numBones = m_skeleton->m_bones.m_size;
    for (int32_t i = boneIdx + 1; i < numBones; ++i) {
        if (m_skeleton->m_parentIndices.m_data[i] == boneIdx) {
            getBoneModelSpace(i);
            m_boneFlags.m_data[i] = F_BONE_LOCAL_DIRTY;
            m_localInSync = false;
        }
    }
}

inline void hkaPose::setBoneLocalSpace(int32_t boneIdx, const hkQsTransform& boneLocal) {
    makeAllChildrenLocalSpace(boneIdx);
    m_localPose.m_data[boneIdx] = boneLocal;
    m_boneFlags.m_data[boneIdx] = F_BONE_MODEL_DIRTY;
    m_modelInSync = false;
}

inline void hkaPose::setBoneModelSpace(int32_t boneIdx, const hkQsTransform& boneModel, PropagateOrNot propagateOrNot) {
    if (!propagateOrNot) {
        makeFirstChildrenModelSpace(boneIdx);
    }
    else {
        makeAllChildrenLocalSpace(boneIdx);
    }
    m_modelPose.m_data[boneIdx] = boneModel;
    m_boneFlags.m_data[boneIdx] = F_BONE_LOCAL_DIRTY;
    m_localInSync = false;
}
