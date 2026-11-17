#pragma once

#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Reconstruction.hpp"

class hkpRigidBody;

// Static helpers that drive a ragdoll's rigid bodies to a key-framed pose through velocities. It has no instances;
// only its nested types carry data.
class hkaKeyFrameHierarchyUtility {
public:
	// The previous frame's state of one rigid body.
	struct WorkElem {
		hkVector4 m_prevPosition;
		hkQuaternion m_prevRotation;
		hkVector4 m_prevLinearVelocity;
		hkVector4 m_prevAngularVelocity;
	};

	// The gains and limits that drive a rigid body towards its key-framed pose.
	struct ControlData {
		ControlData();

		hkBool* isValid(hkBool* apResult);

		// Havok SDK signature (hkaRagdoll.lib, tied by target).
		static const hkClass& staticClass();

		float m_hierarchyGain;
		float m_velocityDamping;
		float m_accelerationGain;
		float m_velocityGain;
		float m_positionGain;
		float m_positionMaxLinearVelocity;
		float m_positionMaxAngularVelocity;
		float m_snapGain;
		float m_snapMaxLinearVelocity;
		float m_snapMaxAngularVelocity;
		float m_snapMaxLinearDistance;
		float m_snapMaxAngularDistance;
	};

	// One skeleton's key-framed pose, as applyKeyFrame takes it: the world transform of the root, the desired
	// local-space poses of the bones and the previous frame's state of each rigid body. The exe's applyKeyFrame
	// (0xD6B8E0) reads the two pointers at +0x30 and +0x34, after the 0x30-byte m_worldFromRoot.
	struct KeyFrameData {
		hkQsTransform m_worldFromRoot;
		const hkQsTransform* m_desiredPoseLocalSpace;
		WorkElem* m_internalReferencePose;
	};

	// The rigid bodies of a ragdoll with their parents, control data indices and per-bone weights.
	struct BodyData {
		int32_t m_numRigidBodies;
		hkpRigidBody** m_rigidBodies;
		const int16_t* m_parentIndices;
		int32_t* m_controlDataIndices;
		const float* m_boneWeights;
	};

	// The per-body result of a key-frame step (hkaRagdollRigidBodyController::driveToPose 0xCDB7C0 passes an array
	// of these to applyKeyFrame 0xD6B8E0).
	struct Output {
		float m_stressSquared;
	};

	// Seeds workElemsOut with each rigid body's current state.
	static void initialize(const BodyData& bodies, WorkElem* internalReferencePose);
	static void applyKeyFrame(float m_deltaTime, const hkaKeyFrameHierarchyUtility::KeyFrameData& pose,
		const hkaKeyFrameHierarchyUtility::BodyData& bodydata,
		const hkaKeyFrameHierarchyUtility::ControlData* controlPalette, hkaKeyFrameHierarchyUtility::Output* output);
	// The reflection class hkaKeyFrameHierarchyUtilityClass.
	static const hkClass& staticClass();
};

// Not named in the PDB or IDA; name chosen here. A helper of applyKeyFrame (0xD6B8E0), its only caller, beside it in
// the exe.
// Inferred type: a1: float: used as one at 0xd6b367
// r_esi: the hkVector4 hkaKeyFrameHierarchyUtility::applyKeyFrame (0xD6B8E0) passes in esi, read and written with
// movaps at 0xD6B330.
void clampVectorLength(float a1, hkVector4* r_esi);
// Not named in the PDB or IDA; name chosen here. A helper of applyKeyFrame (0xD6B8E0), its only caller, beside it in
// the exe.
// Inferred type: return: double: used as one at 0xd6be3e; a1: float: used as one at 0xd6b3cf; a2: float: used as one at
// 0xd6b41e; r_esi: the hkVector4 applyKeyFrame (0xD6B8E0) passes in esi, read and written with movaps.
double clampSnapVelocity(float a1, float a2, hkVector4* r_esi);
// Not named in the PDB or IDA; name chosen here. applyKeyFrame's (0xD6B8E0) first step: for
// each rigid body, its bone's model transform (its parent's, from apOut, or worldFromRoot for a root, times
// desiredPoseLocalSpace[i]) blended towards the body's current one by the control data's m_hierarchyGain (times
// the bone weight), and the body's desired transform from it. The exe takes desiredPoseLocalSpace in ecx and the
// rest on the stack, caller-popped (usercall, ret 0, 0xD6B510).
// Inferred type: bodyData: hkaKeyFrameHierarchyUtility::BodyData* from 0xd6b91f (parameter 2 of
// hkaKeyFrameHierarchyUtility::applyKeyFrame), its members used at 0xd6b520; controlPalette:
// hkaKeyFrameHierarchyUtility::ControlData* from 0xd6b91f (parameter 3 of hkaKeyFrameHierarchyUtility::applyKeyFrame),
// its members used at 0xd6b510
void calcComPoseUsingHierarchy(const hkQsTransform* desiredPoseLocalSpace,
	const hkaKeyFrameHierarchyUtility::BodyData& bodyData,
	const hkaKeyFrameHierarchyUtility::ControlData* controlPalette, const hkQsTransform& worldFromRoot,
	hkQsTransform* apOut);

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkaKeyFrameHierarchyUtilityControlDataTypeInfo;
extern hkTypeInfo hkaKeyFrameHierarchyUtilityTypeInfo;
extern hkClass hkaKeyFrameHierarchyUtilityControlDataClass;
extern hkClass hkaKeyFrameHierarchyUtilityClass;
