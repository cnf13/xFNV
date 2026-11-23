#pragma once

#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

class hkaAnimationBinding;
class hkaPose;

// Maps poses between two skeletons by its mapping data. Its only virtual of its own is the destructor.
class hkaSkeletonMapper : public hkReferencedObject {
public:
    // Which pose the mapping enforces the skeleton's locked bones against after mapping.
    enum ConstraintSource {
        NO_CONSTRAINTS = 0x0,
        REFERENCE_POSE = 0x1,
        CURRENT_POSE = 0x2,
    };

    hkaSkeletonMapper(hkaSkeletonMapperData* mapping);
    // The SDK's finish constructor: the loaded mapper keeps its fields; the mapping data is finished.
    hkaSkeletonMapper(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag), m_mapping(flag) {}
    ~hkaSkeletonMapper() override;

    // Havok SDK: the bone-to-track maps of both skeletons for an animation binding.
    void initializeBindingMaps(
        const hkaAnimationBinding* binding,
        hkArray<int16_t, hkContainerHeapAllocator>& srcBoneToTrackIndicesOut,
        hkArray<int16_t, hkContainerHeapAllocator>& dstBoneToTrackIndicesOut,
        hkArray<int16_t, hkContainerHeapAllocator>& dstTrackToBoneIndicesOut
    );

    static void operator delete(void* apMemory) {
        hkReferencedObject::deleteObject(apMemory);
    }
    // Maps the model space pose of skeleton A into the model space pose of skeleton B, the bones of the simple and
    // chain mappings first, then the unmapped bones, and finally enforces skeleton B's locked bones against source's
    // pose.
    static void mapPose(
        const hkaSkeletonMapper* apThis,
        const hkQsTransform* poseAModelSpace,
        const hkQsTransform* originalPoseBLocalSpace,
        hkQsTransform* poseBModelSpace,
        ConstraintSource source
    );
    // mapPose over poses: maps poseAIn's model space pose into poseBInOut through poseBInOut's local space pose.
    static void
    mapPose(const hkaSkeletonMapper* apThis, const hkaPose& poseAIn, hkaPose& poseBInOut, ConstraintSource source);
    // apOut[j] = apA[j] * apB[i] (hkQsTransform::setMul) for each of the aiNumTransforms transforms of apB: j is
    // apIndices[i] (skipped when negative), or i when apIndices is null.
    static void multiplyPoses(
        const hkQsTransform* apA,
        const hkQsTransform* apB,
        const int16_t* apIndices,
        int32_t aiNumTransforms,
        hkQsTransform* apOut
    );
    // Jump thunk to multiplyPoses, which mapPoseLocalSpace calls.
    static void multiplyPosesThunk(
        const hkQsTransform* apA,
        const hkQsTransform* apB,
        const int16_t* apIndices,
        int32_t aiNumTransforms,
        hkQsTransform* apOut
    );
    // mapPoseLocalSpace with the bone-to-track maps: maps the local space pose of skeleton A into skeleton B's; with
    // both maps given, each bone's transform is at its track's index; additive poses are combined with the reference
    // poses first and made additive again after.
    static void mapPoseLocalSpace(
        const hkaSkeletonMapper* apThis,
        const hkQsTransform* poseALocalSpace,
        hkQsTransform* poseBLocalSpaceInOut,
        const int16_t* boneToTrackIndicesA,
        const int16_t* boneToTrackIndicesB,
        bool additive
    );
    // mapPoseLocalSpace without the bone-to-track maps: forwards to the overload above with no maps.
    static void mapPoseLocalSpace(
        const hkaSkeletonMapper* apThis,
        const hkQsTransform* poseALocalSpace,
        hkQsTransform* poseBLocalSpaceInOut,
        bool additive
    );
    // deltaMotionOut = m_extractedMotionMapping times deltaMotionIn (setMulWithScaling). Takes the mapper as apThis
    // instead of being a const member function.
    static void mapExtractedMotion(
        const hkaSkeletonMapper* apThis,
        const hkQsTransform& deltaMotionIn,
        hkQsTransform& deltaMotionOut
    );
    // additivePoseOut[j] = combinedPose[j] times the inverse of referencePose[i] (multiplyPoses on a stack buffer of
    // the inverses).
    static void additivePoseFromCombinedPoseAndReferencePose(
        const hkQsTransform* combinedPose,
        const hkQsTransform* referencePose,
        const int16_t* boneToTrackIndices,
        int32_t numBones,
        hkQsTransform* additivePoseOut
    );
    static const hkClass& staticClass();
    static bool checkTransform(hkaSkeletonMapperData::MappingType type, const hkQsTransform& q);
    // Havok SDK signature (hkaAnimation.lib, tied by string).
    static void checkMapping(const hkaSkeletonMapperData& m_mapping);
    // The local space mapping's checks; these only issue warnings.
    static void checkMappingLocalSpace(const hkaSkeletonMapperData& m_mapping);
    // Havok SDK signature (hkaAnimation.lib, tied by reloc).
    static void setMulWithScaling(
        hkaSkeletonMapperData::MappingType type,
        hkQsTransform& dst,
        const hkQsTransform& map,
        const hkQsTransform& src
    );

    hkaSkeletonMapperData m_mapping;
};
