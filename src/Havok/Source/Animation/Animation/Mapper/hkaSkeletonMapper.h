#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperData.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

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

	hkaSkeletonMapper(hkaSkeletonMapperData* a1);
	// The SDK's finish constructor: the loaded mapper keeps its fields; the mapping data is finished.
	hkaSkeletonMapper(hkFinishLoadedObjectFlag flag) : hkReferencedObject(flag), m_mapping(flag) {}
	~hkaSkeletonMapper() override;

	// Havok SDK: the bone-to-track maps of both skeletons for an animation binding.
	void initializeBindingMaps(const hkaAnimationBinding* binding,
		hkArray<int16_t, hkContainerHeapAllocator>& srcBoneToTrackIndicesOut,
		hkArray<int16_t, hkContainerHeapAllocator>& dstBoneToTrackIndicesOut,
		hkArray<int16_t, hkContainerHeapAllocator>& dstTrackToBoneIndicesOut);

	static void operator delete(void* apMemory) { hkReferencedObject::deleteObject(apMemory); }
	// The PDB's hkaSkeletonMapper::mapPose (HK_CALL, so cdecl with the mapper on the stack): maps the model space
	// pose of skeleton A into the model space pose of skeleton B, the bones of the simple and chain mappings first,
	// then the unmapped bones, and finally enforces skeleton B's locked bones against arSource's pose.
	static void mapPose(const hkaSkeletonMapper* apThis, const hkQsTransform* poseAModelSpace,
		const hkQsTransform* originalPoseBLocalSpace, hkQsTransform* poseBModelSpace, ConstraintSource source);
	// The PDB's mapPose over poses (cdecl, the mapper on the stack, ret 0, 0xCE4360): maps
	// poseAIn's model space pose into poseBInOut through poseBInOut's local space pose.
	static void mapPose(const hkaSkeletonMapper* apThis, const hkaPose& poseAIn, hkaPose& poseBInOut,
		ConstraintSource source);
	// apOut[j] = apA[j] * apB[i] (hkQsTransform::setMul) for each of
	// the aiNumTransforms transforms of apB: j is apIndices[i] (skipped when negative), or i when apIndices is null.
	// Inferred type: aiNumTransforms: int32_t: signed uses from 0xce2e2f
	static void multiplyPoses(const hkQsTransform* apA, const hkQsTransform* apB, const int16_t* apIndices,
		int32_t aiNumTransforms, hkQsTransform* apOut);
	// The exe's five-byte jump thunk to multiplyPoses (cdecl, ret 0, 0xCE2F30: `jmp 0xCE2D10`), which
	// mapPoseLocalSpace_0 (0xCE3090) calls. Not named in the PDB or IDA; name chosen here.
	static void multiplyPosesThunk(const hkQsTransform* apA, const hkQsTransform* apB, const int16_t* apIndices,
		int32_t aiNumTransforms, hkQsTransform* apOut);
	// The PDB's mapPoseLocalSpace with the bone-to-track maps (cdecl, the mapper on the stack, ret 0, 0xCE3090): maps
	// the local space pose of skeleton A into skeleton B's; with both maps given, each bone's transform is at its
	// track's index; additive poses are combined with the reference poses first and made additive again after.
	static void mapPoseLocalSpace(const hkaSkeletonMapper* apThis, const hkQsTransform* poseALocalSpace,
		hkQsTransform* poseBLocalSpaceInOut, const int16_t* boneToTrackIndicesA, const int16_t* boneToTrackIndicesB,
		bool additive);
	// The PDB's mapPoseLocalSpace without the bone-to-track maps (cdecl, the mapper on the
	// stack, ret 0, 0xCE4330): forwards to the overload above with no maps.
	static void mapPoseLocalSpace(const hkaSkeletonMapper* apThis, const hkQsTransform* poseALocalSpace,
		hkQsTransform* poseBLocalSpaceInOut, bool additive);
	// Havok SDK: deltaMotionOut = m_extractedMotionMapping times deltaMotionIn (setMulWithScaling).
	// The SDK's const member mapExtractedMotion(const hkQsTransform&, hkQsTransform&) const; the exe (0xCE2CF0, cdecl,
	// ret 0) reads the mapper as its first stack argument.
	static void mapExtractedMotion(const hkaSkeletonMapper* apThis, const hkQsTransform& deltaMotionIn,
		hkQsTransform& deltaMotionOut);
	// Havok SDK: additivePoseOut[j] = combinedPose[j] times the inverse of referencePose[i] (multiplyPoses on a
	// stack buffer of the inverses; cdecl, ret 0, 0xCE2F40).
	static void additivePoseFromCombinedPoseAndReferencePose(const hkQsTransform* combinedPose,
		const hkQsTransform* referencePose, const int16_t* boneToTrackIndices, int32_t numBones,
		hkQsTransform* additivePoseOut);
	static const hkClass& staticClass();
	static bool checkTransform(hkaSkeletonMapperData::MappingType type, const hkQsTransform& q);
	// Havok SDK signature (hkaAnimation.lib, tied by string).
	static void checkMapping(const hkaSkeletonMapperData& m_mapping);
	// Havok SDK: the local space mapping's checks, warnings only (cdecl, ret 0, 0xCE43E0; the warnings name
	// ".\Mapper\hkaSkeletonMapper.cpp", lines 551 to 629).
	static void checkMappingLocalSpace(const hkaSkeletonMapperData& m_mapping);
	// Havok SDK signature (hkaAnimation.lib, tied by reloc).
	static void setMulWithScaling(hkaSkeletonMapperData::MappingType type, hkQsTransform& dst, const hkQsTransform& map,
		const hkQsTransform& src);

	hkaSkeletonMapperData m_mapping;
};

inline constexpr fnvd::Vtable hkaSkeletonMapper_vftable{};
