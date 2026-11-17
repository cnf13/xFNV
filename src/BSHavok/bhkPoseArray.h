#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/ehvec.h"

class hkaSkeleton;
class hkQsTransform;
class NiFixedString;
class NiStream;

// The poses a ragdoll's pose matching compares against: the skeleton's bone names, and per pose one transform per
// bone.
class bhkPoseArray : public NiObject {
public:
	// One pose: the bone transforms it holds, one per bone of the skeleton it was built for.
	class bhkPose {
	public:
		// Writes one bone transform.
		static void SaveBoneTransf(NiStream* apStream, hkQsTransform* apBoneTransf);

		uint32_t uiNumBones;
		hkQsTransform* BoneTransfA;
	};

	bhkPoseArray();

	const NiRTTI* GetRTTI() const override;
	// The bone names and the poses with their bone counts, read from the stream; charged to the Havok context.
	void LoadBinary(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	virtual ~bhkPoseArray();

	// Whether every bone of apSkel has its name in this array.
	bool BoneMatch(const hkaSkeleton* apSkel);
	// Copies the pose at uiPoseIdx into apPoseTransfA, one transform per bone of apSkeleton.
	void CreatePoseForRagdoll(hkQsTransform* apPoseTransfA, const hkaSkeleton* apSkeleton, uint32_t uiPoseIdx);

	// An empty pose array: the base object, this class's vtable, no bones and no poses.
	static NiObject* CreateObject();

	uint32_t uiNumBones;
	NiFixedString* BoneNameA;
	uint32_t uiNumPose;
	bhkPose** PoseA;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(bhkPoseArray);
