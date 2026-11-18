#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <xmmintrin.h>

class hkaAnimation;
class hkaAnimationBinding;
class hkaSkeleton;
class hkQsTransform;

// Static helpers on skeletons and poses (pose space conversions, blending, constraints, bone lookup). It has no
// instances and no data.
class hkaSkeletonUtils {
public:
    // Havok SDK: the world space pose of aiNumBones bones; apParentIndices holds each bone's
    // parent (-1 for a root), apWorldFromModel the transform the model sits in and apPoseLocal the local space pose.
    static void transformLocalPoseToWorldPose(
        int32_t numTransforms,
        const int16_t* parentIndices,
        const hkQsTransform& worldFromModel,
        const hkQsTransform* poseLocal,
        hkQsTransform* poseWorldOut
    );
    // Havok SDK: the local space pose of aiNumBones bones from their world space pose apPoseWorld, the model sitting
    // in arWorldFromModel.
    static void transformWorldPoseToLocalPose(
        int32_t numTransforms,
        const int16_t* parentIndices,
        const hkQsTransform& worldFromModel,
        const hkQsTransform* poseWorld,
        hkQsTransform* poseLocalOut
    );
    // Havok SDK: the local space pose of aiNumBones bones from their model space pose apPoseModel.
    static void transformModelPoseToLocalPose(
        int32_t numTransforms,
        const int16_t* parentIndices,
        const hkQsTransform* poseModel,
        hkQsTransform* poseLocalOut
    );
    // Havok SDK: sets out[i] for each bone i of skeleton: whether it lies in startBone's
    // subtree; out[startBone] itself is includeStart.
    static void markDescendants(const hkaSkeleton* skeleton, int32_t startBone, bool* out, bool includeStart);
    // Havok SDK: blends numBones transforms of poseA towards poseB by each bone's own weight into poseOut.
    static void blendPoses(
        uint32_t numBones,
        const hkQsTransform* poseA,
        const hkQsTransform* poseB,
        const float* weights,
        hkQsTransform* poseOut
    );
    // Havok SDK: the same with one weight for every bone.
    static void blendPoses(
        uint32_t numBones,
        const hkQsTransform* poseA,
        const hkQsTransform* poseB,
        float weight,
        hkQsTransform* poseOut
    );
    // Havok SDK: blends only the numBones bones listed in bones, each at its own index of the poses.
    static void blendPartialPoses(
        uint32_t numBones,
        int16_t* bones,
        const hkQsTransform* poseA,
        const hkQsTransform* poseB,
        float weight,
        hkQsTransform* poseOut
    );
    // Havok SDK: appends to bonesOut every bone below startBone (bones are ordered parents first), and startBone
    // itself first when includeStart is set.
    static void
    getDescendants(const hkaSkeleton& skeleton, int16_t startBone, hkArray<int16_t>& bonesOut, bool includeStart);
    // Havok SDK: appends to leavesOut every bone that is no bone's parent.
    static void getLeaves(const hkaSkeleton& skeleton, hkArray<int16_t>& leavesOut);
    // Appends to bonesOut the chain of bones from startBone down to endBone, each bone after the parent it walked from
    // (so parents come first), and returns whether endBone's chain reached startBone. The hkBool comes back through
    // apFoundOut, the first argument. It walks m_parentIndices from endBone and inserts each bone at index 0.
    static hkBool
    getBoneChain(const hkaSkeleton& skeleton, int16_t startBone, int16_t endBone, hkArray<int16_t>& bonesOut);
    // Havok SDK: puts the model space pose of every bone whose m_lockTranslation is set back
    // in its parent's space (pose[i] = pose[parent] * referencePose[i]); roots are left alone.
    static void enforceSkeletonConstraintsModelSpace(const hkaSkeleton& skeleton, hkQsTransform* poseModelInOut);
    // Havok SDK: the same for the bones whose m_lockTranslation is set, taking their local
    // space transform from apConstraintsLocal instead of the skeleton's reference pose.
    static void enforcePoseConstraintsModelSpace(
        const hkaSkeleton& skeleton,
        const hkQsTransform* constraintsLocal,
        hkQsTransform* poseModelInOut
    );
    static void
    transformLocalPoseToModelPose(int32_t a1, const int16_t* a2, const hkQsTransform* a3, hkQsTransform* a4);
    static void getModelSpaceScale(
        const hkaSkeleton& skeleton,
        const hkQsTransform* poseLocal,
        int32_t boneId,
        hkVector4& scaleOut
    );
    static int16_t
    findBoneWithName(const hkaSkeleton& skeleton, const char* name, int32_t (*compare)(const char*, const char*));
    static void transformLocalBoneToModelBone(
        int32_t boneIndex,
        const int16_t* parentIndices,
        const hkQsTransform* poseLocal,
        hkQsTransform& boneModelOut
    );
    // Havok SDK signature (hkaAnimation.lib, tied by bytes).
    static void lockTranslations(hkaSkeleton& skeleton, hkBool exceptRoots);
    // Havok SDK signature (hkaAnimation.lib, tied by bytes).
    static void unlockTranslations(hkaSkeleton& skeleton);
    static hkBool isBindingOk(const hkaSkeleton& skeleton, const hkaAnimationBinding& binding);
    // Havok SDK: the world space pose of numTransforms bones from their model space pose, the model sitting in
    // worldFromModel.
    static void transformModelPoseToWorldPose(
        int32_t numTransforms,
        const hkQsTransform& worldFromModel,
        const hkQsTransform* poseModel,
        hkQsTransform* poseWorldOut
    );
    // Havok SDK: the model space pose of numTransforms bones from their world space pose.
    static void transformWorldPoseToModelPose(
        int32_t numTransforms,
        const hkQsTransform& worldFromModel,
        const hkQsTransform* poseWorld,
        hkQsTransform* poseModelOut
    );
    // Havok SDK: the translation of every bone whose m_lockTranslation is set taken from the skeleton's reference
    // pose.
    static void enforceSkeletonConstraintsLocalSpace(const hkaSkeleton& skeleton, hkQsTransform* poseInOut);
    // Havok SDK: the same, taking the translations from constraintsLocal.
    static void enforcePoseConstraintsLocalSpace(
        const hkaSkeleton& skeleton,
        const hkQsTransform* constraintsLocal,
        hkQsTransform* poseInOut
    );
    // Fills bindingOut's track-to-bone indices by matching each annotation track's name to a bone's name with compare
    // (0 on a match); HK_FAILURE with a warning when the binding is not sized, a track or bone has no name, or a track
    // matches no bone.
    static hkResult bindByName(
        const hkaSkeleton& skeleton,
        const hkaAnimation& anim,
        int32_t (*compare)(const char*, const char*),
        hkaAnimationBinding& bindingOut
    );
};
