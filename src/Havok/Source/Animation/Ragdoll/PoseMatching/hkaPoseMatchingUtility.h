#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

#include <cstddef>
#include <stdint.h>

class hkaAnimationBinding;
class hkQsTransform;

// Takes the twist about direction out of rotationInOut. findBestCandidatePoseIndex and addCandidatePose pass
// m_rotationAxis.
void hkRemoveTwist(const hkVector4& direction, hkQuaternion& rotationInOut);

// Matches the pose of a reference character to the rotations the animation is playing, by scoring the candidate
// rotations of one bone against the character's other bones.
class hkaPoseMatchingUtility {
public:
    // A candidate pose: the reference rotation computeOrientation found for it, with the animation and the time it
    // was taken from. findBestCandidatePoseIndex strides 0x20 bytes reading m_reference.
    struct CandidatePoseInfo {
        hkQuaternion m_reference;
        const hkaAnimationBinding* m_binding;
        float m_time;
    };

    hkaPoseMatchingUtility(int16_t rootBoneIdx, int16_t otherBoneIdx, int16_t anotherBoneIdx, const hkVector4& rotAxis);

    // The class allocator: blocks of the Havok heap.
    static void* operator new(size_t aSize) noexcept {
        return hkMemoryRouter::getInstance().m_heap->blockAlloc(static_cast<int32_t>(aSize));
    }
    static void operator delete(void* apMemory, size_t aSize) {
        hkMemoryRouter::getInstance().m_heap->blockFree(apMemory, static_cast<int32_t>(aSize));
    }

    int32_t addCandidatePose(const hkQsTransform* poseModelSpace, const hkaAnimationBinding* binding, float time);
    void computeOrientation(const hkQsTransform* poseModelSpace, hkQuaternion& frameOut);
    int32_t findBestCandidatePoseIndex(const hkQsTransform* poseModelSpace, float& error);
    void computeReferenceFrame(
        const hkQsTransform* animPoseModelSpace,
        const hkQsTransform* ragdollPoseWorldSpace,
        hkQsTransform& animWorldFromModel,
        hkQsTransform& ragdollWorldFromModel
    );

    hkArray<CandidatePoseInfo> m_candidatePoses;
    hkVector4 m_rotationAxis;
    int16_t m_rootBoneIdx;
    int16_t m_otherBoneIdx;
    int16_t m_anotherBoneIdx;
};
