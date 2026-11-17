#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>
#include <xmmintrin.h>

class hkaSkeleton;

uint8_t* returnTrue(uint8_t* a1, int32_t a2);

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
	// The model-space pose, synchronized first, for writing; every bone is marked locally dirty.
	hkArray<hkQsTransform>& accessSyncedPoseModelSpace();
	// The model-space pose, synchronized first (the SDK's const member: its pose arrays are mutable there).
	const hkArray<hkQsTransform>& getSyncedPoseModelSpace() const;
	// The local-space pose, synchronized first (const as in the SDK).
	const hkArray<hkQsTransform>& getSyncedPoseLocalSpace() const;
	// Whether no bone is dirty in both spaces, and no bone is dirty in a space marked in sync.
	hkBool* checkPoseValidity(hkBool* apResult) const;
	void setToReferencePose();
	hkQsTransform& accessBoneLocalSpace(int32_t a1);
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
	uint8_t* checkPoseTransformsValidity(uint8_t* a1);
	void getModelSpaceAabb(__m128* a1);
	// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc): sizes the pose arrays to m_skeleton's bones and
	// float slots, then sets the given pose in its space.
	void init(hkaPose::PoseSpace space, const hkaSkeleton* skeleton, const hkArrayBase<hkQsTransform>& pose);

	// hkaPose's class allocator delete: the block goes back to Havok's heap.
	static void operator delete(void* apMemory);

	const hkaSkeleton* m_skeleton;
	hkArray<hkQsTransform> m_localPose;
	hkArray<hkQsTransform> m_modelPose;
	hkArray<uint32_t> m_boneFlags;
	hkBool m_modelInSync;
	hkBool m_localInSync;
	uint8_t padding2A[2];
	hkArray<float> m_floatSlotValues;
};
