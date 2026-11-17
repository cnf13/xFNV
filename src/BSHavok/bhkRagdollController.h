#pragma once

#include "BSCore/BSPrecisionTimer.h"
#include "BSHavok/bhkPoseArray.h"
#include "BSHavok/bhkRagdollShareData.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Havok/Source/Animation/Animation/Ik/FootPlacement/hkaFootPlacementIkSolver.h"
#include "BSCore/BSSimpleArray.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiQuaternion;
struct RagdollFeedbackData;
class bhkRigidBody;
class bhkShapePhantom;
struct hkCapsuleShapeCinfo;
class hkaPose;
class hkaPoseMatchingUtility;
class hkaRagdollInstance;
class hkaRagdollRigidBodyController;
class hkaSkeleton;
class hkpCachingShapePhantomRay;
class hkpConstraintInstance;
class hkpRigidBody;

void* Reallocate(void* a1, uint32_t Size);
// IDA name; not in the PDB. Appends apBody's Havok body to arBones and the Havok constraint of each bhkConstraint
// on its action/constraint list to arConstraints (cdecl; bhkRagdollController::FindBones, its one caller, passes its
// own two arrays).
void AddRagdollData(bhkRigidBody* apBody, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBones,
	hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints);
NiTransform& HK2NI(NiTransform& arNiTran, const hkQsTransform& arhkTran);
void FreeRagdollRigidBodyController(void* a1);
void FreePoseMatchingUtility(void* a1);

// The look IK's shared per-creature state: the head bone's skeleton and the head's forward direction in parent and
// in local space. The 12 bytes after the pointer are hkVector4's alignment gap, as in the PDB. IDA declares three
// floats fUnk4..fUnkC there, which none of the functions that use the record touches: InitLookIK (0xC7DE60) stores
// only the pointer, InitBoneParams (0xC79340) reads the two vectors. Only the layout is declared.
struct bhkLookIKData {
	~bhkLookIKData();

	hkaSkeleton* pHeadSkel;
	hkVector4 FwdParentMS;
	hkVector4 FwdLS;
};

// Drives an actor's ragdoll from its animation; the PDB's base is hkaRaycastInterface (vtable 0x10C4DDC: the
// deleting destructor 0xC7DE30, the base's filtered castRay 0xCD8810, castRay 0xC77460). Only what the
// reconstructed code uses is declared.
class bhkRagdollController : public hkaRaycastInterface {
public:
	// The arms the grab IK drives.
	enum GRAB_IK_ARM : int32_t {
		LEFT_ARM = 0,
		RIGHT_ARM = 1,
		NUM_ARMS = 2,
	};

	// How the ragdoll's rigid bodies follow the animation (eRBUpdateMethod).
	enum RAGDOLL_UPDATE_TYPE : int32_t {
		NONE = 0,
		WARP = 1,
		APPLY_VELOCITY = 2,
		DRIVE_CONSTRAINTS = 3,
	};

	// The look-at IK's bones.
	enum LookIKBone : int32_t {
		EYE_BONE = 0,
		HEAD_BONE = 1,
		NUM_LOOKIK_BONES = 2,
	};

	// The skeleton whose debug lines are drawn: the keyframed pose, the ragdoll's or the final one.
	enum DEBUG_SKEL : int32_t {
		SKEL_KEYFRAMED = 0,
		SKEL_RAGDOLL = 1,
		SKEL_FINAL = 2,
	};

	// The grab IK of one hand: the node it reaches for, its blend and the previous reach in world space.
	struct GrabIKInfo {
		NiAVObject* pTarget;
		float fIntrpGain;
		float fReachWeight;
		bool bEnabled;
		bool bEaseOutGrabIK;
		uint8_t padding0E[2];
		hkVector4 PrevReachLocWS;
		hkVector4 PrevReachNormWS;
	};

	// One hand's grab IK chain: the ragdoll bones the solver drives.
	struct GrabIKSetup {
		int16_t usShoulderIdx;
		int16_t usElbowIdx;
		int16_t usHandIdx;
	};

	// The look-at IK of one bone (eye or head): its pose and bone indexes, the forward directions, the previous
	// rotation and the dead zone.
	struct LookIKBoneParams {
		hkaPose* pPose;
		int16_t ssBoneIdx;
		int16_t ssParentIdx;
		float fLookAtMaxAngle;
		uint8_t padding0C[4];
		hkVector4 FwdLS;
		hkVector4 FwdParentMS;
		hkQuaternion PrevRotLS;
		bool bLimitBoneRot;
		bool bActive;
		uint8_t padding42[2];
		float fDeadHeading;
		float fDeadPitch;
		uint8_t padding4C[4];
	};

	// One of a foot IK leg's two ray casts: the ray's ends, the surface normal it hit and the hit fraction.
	struct FootRayCastResults {
		hkVector4 From;
		hkVector4 To;
		hkVector4 Norm;
		float fFraction;
	};

	// The foot IK of one leg: its placement solver, the solver's error, the leg's ray casts, the ankle's local
	// transform and the ragdoll's foot bone.
	struct FootIkLegInfo {
		hkaFootPlacementIkSolver* pSolver;
		float fResultError;
		FootRayCastResults CastResults[2];
		hkQsTransform AnkleLS;
		int16_t RagdollFootIdx;
	};

	// A bone and its local transform (IDA names the members pBone and kTransform).
	struct BoneTransf {
		NiAVObject* pObj;
		NiTransform LocalTransform;
	};

	// One bone of the skeleton the ragdoll is built from: the scene graph node's name, its local transform and the
	// index of its parent bone (the PDB's members; sizeof 0x50, the 0xE bytes after parentIndice being hkQsTransform's
	// 16-byte alignment tail). CreateSGSkeletonRec (0xC7B4F0) allocates 0x60 and aligns the pointer up to 16.
	struct bhkSkeletonBone {
		const char* name;
		hkQsTransform transform;
		int16_t parentIndice;
	};

	// The motors of the ragdoll's constraints (the PDB puts this class in bhkRagdollController.cpp).
	class bhkRagdollPoweredConstraintController {
	public:
		// Turns on the motor of every constraint the ragdoll's bone-to-rigid-body map names.
		static void startMotors(hkaRagdollInstance* ragdoll);
		static void stopMotors(hkaRagdollInstance* ragdoll);
		static void driveToPose(hkaRagdollInstance* ragdoll, const hkQsTransform* desiredPose);
	};

	// IDA: bhkRagdollController::kDeathPoses. The exe stores NiTMap<const char*, NiPointer<bhkPoseArray>>'s vtable
	// (0x10C4C84) into it.
	using PoseMap_t = NiTMap<const char*, NiPointer<bhkPoseArray>>;
	// IDA: bhkRagdollController::kShareDataMap. The exe stores NiTMap<const char*,
	// NiPointer<bhkRagdollShareData>>'s vtable (0x10C4CC4) into it.
	using SkeletonMap_t = NiTMap<const char*, NiPointer<bhkRagdollShareData>>;

	bhkRagdollController(NiAVObject* apObject, NiAVObject* apPose, const char* apScaledSkeletonName);
	// Slot 0 of the engine's vtable is the deleting destructor; `delete` calls it with flag 1.
	~bhkRagdollController() override;

	// Casts a foot IK ray for the current leg (IDA names it castRayAlt; thiscall, the hkBool
	// returned through a hidden pointer, ret 0x14, slot 2, 0xC77460).
	hkBool castRay(const hkVector4& arFromWS, const hkVector4& arToWS, float& arHitFractionOut,
		hkVector4& arNormalWSOut) override;

	// The PDB's ForceHeadTrack(): makes the ragdoll follow the head tracking (thiscall, ret 0,
	// 0xC748B0).
	void ForceHeadTrack();
	// Switches the look inverse kinematics on or off.
	void SetLookIKActive(bool abActive);
	// Whether the ragdoll animation initialised (the bool at +0x42; thiscall, ret 0,
	// 0x87EA20).
	bool QRagdollAnimInit();
	// Moves the ragdoll's rigid bodies to the current 3D pose.
	void SyncRagdollTo3D();
	// The inverse scale matrix at +0x60.
	NiMatrix3& QScaleInv() { return ScaleInv; }
	// Sets whether the look-at IK uses the head pose (the bool at +0x190); the exe's shared
	// one-byte setter 0x4E4660 (thiscall, ret 4).
	void SetUseHeadPose(bool abUseHead);
	// Sets up the look-at IK from the node apNodeName.
	bool InitLookIK(const char* apNodeName);
	// The look-at IK's maximum angle for aeBone, in radians.
	void SetLookAtMaxAngle(LookIKBone aeBone, float afMaxAngle);
	// The look-at IK's dead zone for aeBone.
	void SetLookIKDeadZone(LookIKBone aeBone, float afHeadingMax, float afPitchMax);
	// Enables the look-at IK when it is set up.
	void SetLookIKEnable(bool abEnable);
	// Whether the look-at IK is enabled.
	bool QLookIKEnable();
	// Sets whether the feet align to the terrain (the bool at +0x240); the exe's shared
	// one-byte setter 0x5C6B40 (thiscall, ret 4).
	void SetAlignFeetToTerrain(bool abAlign);
	// Sets whether the foot IK casts with the phantom (the bool at +0x224; thiscall, ret 4,
	// 0x87EA60).
	void SetUsePhantomFootIKCasts(bool abUsePhantom);
	// Sets up the foot IK from the bones named in arboneNameA.
	void InitFootIK(BSSimpleArray<const char*>& arboneNameA);
	// Enables the foot IK when it is set up.
	void SetFootIKEnable(bool abEnable);
	// Whether the foot IK is enabled.
	bool QFootIKEnable();
	// Enables the grab IK when it is set up.
	void SetGrabIKEnable(bool abEnable);
	// Whether the grab IK is enabled.
	bool QGrabIKEnable();
	// Sets the ragdoll's collision group (the dword at +0x5C; thiscall, ret 4, 0x87EA40).
	void SetCollisionGroup(uint32_t aiGroup);
	// Loads the poses of apFileName for pose matching on the given bones.
	void InitPoseMatching(const char* apFileName, int16_t asRootIndex, int16_t asBone2Index, int16_t asBone3Index);
	// Copies the ragdoll feedback settings.
	void SetFeedbackData(RagdollFeedbackData& arFeedbackData);
	// Enables pose matching when poses are loaded.
	void SetPoseMatchEnable(bool abEnable);
	// Stores apPhantom's Havok body.
	void SetPhantom(bhkShapePhantom* apPhantom);
	// Turns the ragdoll feedback on or off, or restarts its timer when it is already on.
	void SetRagdollFeedbackActive(int32_t abActive);
	// Stops the animation driving the ragdoll.
	void DisableRagdollAnim(bool abUpdateConstraints);
	bool GetRagdollAnimEnable();
	void ReinitRBControllerOnUpdate(bool abReinit);
	bool QFootIKActive();
	bool QRagdollFeedbackEnable();
	bool QRagdollFeedbackActive();
	uint32_t QFeedbackImpulseWeak();
	uint32_t QFeedbackImpulseStrong();
	hkpRigidBody* GetRigidBodyHead();
	void EnableRagdollFeedback(bool abEnable);
	void ClearVertError();
	void SetEaseOutLookAtIK(bool abEnable);
	bool QRagdollAnimEaseOut();
	uint8_t GetEaseOutLookAtIK();
	// Points the ragdoll's look IK at apTarget, or releases it when abEnable is false.
	void SetLookAtTarget(NiPoint3* apTarget, bool abEnable);
	void SetMaxEyePitch(float afAngle);
	uint8_t GetLookIKActive();
	void GetEyeAngles(float& arHeading, float& arPitch);
	void UseFootDownCast(bool abCastDown);
	void SetUseFootEndCast(bool abCast);
	bhkWorld* GetWorld() const;
	void SetFootIKActive(bool abActive);
	// Poses the ragdoll from the animation: the IK passes, pose matching, the rigid bodies and the feedback.
	void DoRagdollAnim(bool abUpdateBones);

protected:
	// Reads the scene graph's pose into the local-space pose.
	void GetSGPoseLocalSpace();
	// Writes the local-space pose back to the scene graph.
	void SetSGPoseLocalSpace();
	// Drives the ragdoll's rigid bodies, from apRagdollPoseLS when it is not null.
	void UpdateRagdollRigidBodies(const hkQsTransform* apRagdollPoseLS);
	// Picks the best pose to match into apRagdollDrivePoseLS and sets arbUseDrivePose.
	void DoPoseMatching(const hkQsTransform& arWorldFromModel, hkQsTransform* apRagdollDrivePoseLS,
		bool& arbUseDrivePose);

public:
	// Updates the debug lines of skeleton aeSkeleton.
	void UpdateDebugLines(DEBUG_SKEL aeSkeleton);
	// Places the feet on the ground.
	void DoFootIK(hkQsTransform& arWorldFromModel);
	// Moves the hands to their grab targets.
	void DoGrabIK(hkQsTransform& arWorldFromModel);
	// Turns the eyes and head to the look-at target.
	void DoLookAtIK(hkQsTransform& arWorldFromModel);
	// Applies the ragdoll feedback.
	void DoRagdollFeedback();
	void ShowDebugLines(bool a1, bool a2, bool a3);
	void SetRagdollAnimEaseOut(bool abEaseOut);
	void SetHeadOffset(const NiQuaternion& arRot);
	hkCapsuleShapeCinfo* GetCapsuleBoneShapeInfo(NiAVObject* apBone);
	bool InitGrabIK();
	float LimitBoneRot(bhkRagdollController::LookIKBone aeBone, const hkQuaternion& arIKBoneRotLS);
	bool IsPoweredRagdoll();
	void DampenDriveGains();
	void ResetDriveGains();
	bool Leg3DSelectiveUpdate();
	void AdjustCurrentTarget();
	bool BoneTrack(bhkRagdollController::LookIKBone aeBone, hkQsTransform& arWorldFromModel);
	bool InitBoneParams(hkaPose* apPose, const char* apNodeName, bhkRagdollController::LookIKBone aeBone);
	bool CheckMappersForWarnings();
	void EnableRagdollAnim();
	// Builds the shared high skeleton from the scene graph's bones and the scene-graph pose.
	void CreateSGSkeleton();
	void Init(NiAVObject* apObject, const char* apScaledSkeletonName);
	bool GetEyeOffset(NiPoint3& arOffset);
	void ApplyBoneTransforms();
	bool ExtractBipedSetupData(hkaFootPlacementIkSolver::Setup& arSetupData,
		BSSimpleArray<const char*, 1024>& arboneNameA);
	// Adds the ragdoll's constraints to its world, or removes them; only the low byte of abAdd is read.
	void UpdateConstraintsWorld(int32_t abAdd);
	void GetRBForBones(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBoneRBA);
	void SetGrabIKEaseOut(bhkRagdollController::GRAB_IK_ARM arArm, bool abEnable);
	void SetGrabIKEnabled(bhkRagdollController::GRAB_IK_ARM arArm, bool abEnable);
	void SetGrabIKIntrpGain(bhkRagdollController::GRAB_IK_ARM arArm, float afGain);
	void SetGrabIKTarget(bhkRagdollController::GRAB_IK_ARM arArm, NiAVObject* apTarget);
	// Builds the skeleton bones of apObj and the nodes below it.
	void CreateSGSkeletonRec(NiAVObject* apObj, BSSimpleArray<bhkRagdollController::bhkSkeletonBone*, 1024>& arBoneA,
		NiTMap<NiAVObject*, int16_t>& arboneMap);

	// Sets the ragdoll animation flag (cdecl, ret 0, 0x456C90).
	// The exe passes the setting byte xor 1, not 0/1; bool would normalise it.
	static void SetRagdollAnim(bool abEnable);
	// The bGrabIK setting's byte as stored.
	static bool QGrabIK();
	// Stores the byte abEnable in the bGrabIK setting as given (cdecl, ret 0, 0x5BA2F0). The
	// PDB types it bool; the exe's callers pass the setting byte xor 1, which a bool parameter cannot carry.
	static void SetGrabIK(bool abEnable);
	// The bRagdollAnim setting's byte as stored.
	static bool QRagdollAnim();
	// The bLookIK setting's byte as stored.
	static bool QLookIK();
	// Stores the byte abEnable in the bLookIK setting as given (cdecl, ret 0, 0x5BA4D0). The
	// PDB types it bool; the exe's caller passes the setting byte xor 1, which a bool parameter cannot carry.
	static void SetLookIK(bool abEnable);
	// The class's operator new: 16-byte aligned memory.
	static void* operator new(size_t blocksize);
	// Frees the 16-aligned block operator new made: the alignment padding byte sits in front of the object.
	static void operator delete(void* apMemory);
	static void ResetFootCastTimer();
	static float QMaxTrackingDistance();
	static float QMinTrackingDistance();
	static float QCameraDistance();
	static bool QFootIK();
	static void SetFootIK(bool abEnable);
	// Puts each bone's node back to its pose in arPoseA (translation and rotation), updates the skeleton at time 0,
	// gives its bodies aePrevMotion unless that is MOTION_INVALID or MOTION_KEYFRAMED, and sets the use-velocity flag
	// of its collision objects when abPrevUseVel is set.
	static void ResetPose(NiNode* apSkeleton, BSSimpleArray<BoneTransf, 1024>& arPoseA, bool abPrevUseVel,
		hkpMotion::MotionType aePrevMotion);
	// For apPose and each node under it, its namesake under apSkeleton: records it with its local transform in
	// arPoseA, then gives it apPose's local rotation and translation (the parent's for "Bip01 NonAccum").
	static void CopyPoseTo3DRec(NiNode* apSkeleton, NiAVObject* apPose, BSSimpleArray<BoneTransf, 1024>& arPoseA);
	// Copies apPose onto apSkeleton (CopyPoseTo3DRec) and updates it; first records in arbPrevUseVel whether the
	// skeleton's collision object used velocity (and clears it under the skeleton) and in arePrevMotion its body's
	// motion type (MOTION_FIXED without a body), keyframing the skeleton unless it was keyframed.
	static void CopyPoseTo3D(NiNode* apSkeleton, NiNode* apPose, BSSimpleArray<BoneTransf, 1024>& arPoseA,
		bool& arbPrevUseVel, hkpMotion::MotionType& arePrevMotion);
	// Collects the blended rigid bodies under apObject and their constraints.
	static void FindBones(NiAVObject* apObject, hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
		hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints);

	bool bQueueStopPoweredMotors;
	uint8_t padding005[3];
	float fCurrentMatchError;
	RAGDOLL_UPDATE_TYPE eRBUpdateMethod;
	hkQsTransform WorldFromModel;
	bool bFullyDynamic;
	bool bEnableRagdollAnim;
	bool bInitRagdollAnim;
	bool bEaseOutRagdollAnim;
	// The PDB's bQueuedWarp and bBoneUpdate (xbox 0x44/0x45, no shift up to pRagdollInstance); the constructor
	// 0xC7F060 clears bQueuedWarp (0xC7F3B3 mov byte ptr [esi+0x44], bl).
	bool bQueuedWarp;
	bool bBoneUpdate;
	uint8_t padding046[0x2];
	hkaRagdollInstance* pRagdollInstance;
	hkaRagdollRigidBodyController* pRagdollRBController;
	bool bReinitRBController;
	uint8_t padding051[3];
	// The head's pose the look-at IK starts from.
	hkaPose* pHeadPose;
	NiAVObject* pRagdollRoot;
	uint32_t iGroup;
	NiMatrix3 ScaleInv;
	NiPointer<NiNode> spSkeletonDebug;
	hkaPose* pSGPose;
	// The PDB's bUsingMappers (xbox 0x98; the members from spSkeletonDebug to bActiveLookIK sit 0xC below the PDB's,
	// as spSkeletonDebug 0x84 against 0x90 shows). The constructor 0xC7F060 clears it (0xC7F313 mov byte ptr
	// [esi+0x8C], bl).
	bool bUsingMappers;
	uint8_t padding08D[0x3];
	hkpCachingShapePhantomRay* pControllerPhantom;
	// The animation's pose in local space (the PDB's member; IDA names it kTransforms and types it hkArray_UInt32)
	// and the scene graph's bones (the PDB's SGBonesA; IDA's kBones, BSSimpleArray_NiAVObject).
	hkArray<hkQsTransform> AnimPoseLS;
	BSSimpleArray<NiAVObject*> SGBonesA;
	bool bInitLookIK;
	bool bEnableLookIK;
	bool bEaseOutLookIK;
	bool bActiveLookIK;
	// The PDB's bApplyHeadOffset (xbox 0xC0, 0x10 above the exe's 0xB4 like the vectors after it); the constructor
	// 0xC7F060 clears it (0xC7F1E8 mov byte ptr [esi+0xB4], bl).
	bool bApplyHeadOffset;
	uint8_t padding0B5[0xB];
	// IDA's struct puts the members from here on 8 bytes lower; the exe aligns the Havok vectors to 16 bytes, which
	// moves CurrentHeadRotOffset to 0xC0 (SetHeadOffset 0x8BD5E0, SetLookAtTarget 0x8A3B70 read 0xC0 and 0xD0) and
	// BoneParamA to 0xF0 (SetLookAtMaxAngle 0x607810 writes 0xF8 + 0x50 * bone).
	hkQuaternion CurrentHeadRotOffset;
	hkVector4 CurrentTargetWS;
	hkVector4 EyeOffset;
	LookIKBoneParams BoneParamA[NUM_LOOKIK_BONES];
	// The exe's bUseHeadPose (the PDB's name; xbox 0x1A0, 0x10 above the exe's 0x190): SetUseHeadPose (0x4E4660)
	// stores it (0x4E466D mov byte ptr [eax+0x190], cl). src's bodies name it bTargetOutsideHeadLimit; renaming it
	// changes what every body naming either member reads, so it waits for the bodies' re-emit.
	bool bTargetOutsideHeadLimit;
	uint8_t padding191[3];
	LookIKBone eCurrentBone;
	// The PDB's bTargetOutsideHeadLimit (xbox 0x1A8, 0x10 above the exe's 0x198); the constructor 0xC7F060 clears it
	// (0xC7F3B9 mov byte ptr [esi+0x198], bl). Its name is on the member at 0x190 until that member's rename.
	uint8_t padding198[0x4];
	float fEyeHeading;
	float fEyePitch;
	// The PDB's bTargetBehind and bHeadSwitchSides (xbox 0x1B4/0x1B5, 0x10 above the exe's); the constructor 0xC7F060
	// clears both (0xC7F20E/0xC7F214 mov byte ptr [esi+0x1A4]/[esi+0x1A5], bl).
	bool bTargetBehind;
	bool bHeadSwitchSides;
	uint8_t padding1A6[0x2];
	float fMaxEyePitchAngle;
	uint8_t padding1AC[0x4];
	GrabIKInfo GrabIKState[2];
	GrabIKSetup GrabSetup[2];
	// The PDB's members from bInitGrabIK to bActivePoseMatch sit 0x10 above the exe's (xbox 0x22C..0x266):
	// SetGrabIKEnable (0x5BA310) sets 0x21D from 0x21C, SetFootIKEnable (0x5BA130) 0x220 from 0x21F,
	// SetUsePhantomFootIKCasts (0x87EA60) stores 0x224, SetAlignFeetToTerrain (0x5C6B40) 0x240, ClearVertError
	// (0x888AA0) zeroes 0x23C and SetPoseMatchEnable (0x87E9D0) sets 0x255 from 0x254. The foot IK's leg array
	// (0x22C: data, size, capacity), the ease-out frame count (0x228) and the cast indices (0x238/0x23A) are
	// DoFootIK's (0xC76350) and castRay's (0xC77460).
	bool bInitGrabIK;
	bool bEnableGrabIK;
	bool bActiveGrabIK;
	bool bInitFootIK;
	bool bEnableFootIK;
	bool bEaseOutFootIK;
	bool bActiveFootIK;
	bool bResetSolverWeight;
	bool bUsePhantomFootIKCasts;
	int32_t iCurrFootEase;
	hkArray<FootIkLegInfo> LegInfoA;
	uint16_t usCurrentCastIdx;
	uint16_t usCurrentFootCastIdx;
	float fVertFootIkError;
	bool bAlignFeetToTerrain;
	bool bFootEndCast;
	bool bUseCurrAnkleTransf;
	bool bDownCastFoot;
	// The constructor 0xC7F060 zeroes the distance (0xC7F325 fst dword ptr [esi+0x244]) and pMatchUtil
	// (0xC7F2C5), clears PoseAStr (0xC7F103) and spPoseA (0xC7F109); the destructor 0xC7D900 frees pMatchUtil
	// (blockFree, 48 bytes), releases PoseAStr (NiGlobalStringTable::ReleaseHandle) and spPoseA (its reference).
	float fDownCastDistance;
	hkaPoseMatchingUtility* pMatchUtil;
	NiFixedString PoseAStr;
	NiPointer<bhkPoseArray> spPoseA;
	bool bInitPoseMatch;
	bool bEnablePoseMatch;
	bool bActivePoseMatch;
	// The pose matching's state: the PDB's members from iCurrentMatchPoseIdx to iEditorForcePoseIdx (xbox
	// 0x268..0x298, 0x10 above the exe's). DoPoseMatching (0xC7BA50) reads the timer at 0x260 (0xC7BAE5 sub eax,
	// [esi+0x260]), the root location at 0x270 (0xC7BA7C fld dword ptr [esi+0x270]), the displacement maximum at
	// 0x27C and the time at 0x280, and tests bDisablePoseMatchOnDisplace (0xC7BB03) and iEditorForcePoseIdx
	// (0xC7BB1F cmp dword ptr [esi+0x288], -1).
	uint8_t padding257[0x1];
	int32_t iCurrentMatchPoseIdx;
	uint8_t padding25C[0x4];
	BSPrecisionTimer PoseMatchTimer;
	NiPoint3 PoseMatchRootLoc;
	float fPoseMatchDisplacementMax;
	float fPoseMatchTime;
	bool bDisablePoseMatchOnDisplace;
	uint8_t padding285[0x3];
	int32_t iEditorForcePoseIdx;
	bool bInitRagdollFeedback;
	bool bEnableRagdollFeedback;
	bool bActiveRagdollFeedback;
	bool bEaseOutFeedback;
	float fCurrFeedbackInterp;
	uint32_t uiFeedbackTimeoutCount;
	uint32_t uiFeedbackImpulseStrong;
	uint32_t uiFeedbackImpulseWeak;
	// The PDB's SkeletonShareName (a NiFixedString), spRagdollSharedData (a reference-counted bhkRagdollShareData)
	// and FeedbackTimer (xbox 0x2B0/0x2B4/0x2B8, 0x10 above the exe's): the constructor 0xC7F060 clears the name
	// (0xC7F12D), the shared data (0xC7F133) and the timer's four words (0xC7F13B..0xC7F14B), and the destructor
	// 0xC7D900 releases the name (NiGlobalStringTable::ReleaseHandle) and the shared data (its reference count).
	NiFixedString SkeletonShareName;
	NiPointer<bhkRagdollShareData> spRagdollSharedData;
	BSPrecisionTimer FeedbackTimer;
	int32_t nHeadBone;
	uint8_t padding2BC[0x4];

	static SettingT<INISettingCollection> bGrabIK;
	// The ini settings (the exe reads each one's value byte at +4).
	static SettingT<INISettingCollection> bRagdollAnim;
	static SettingT<INISettingCollection> bFootIK;
	static inline float fFootCastTimer;
	static SettingT<INISettingCollection> bLookIK;
	static SettingT<INISettingCollection> bRagdollFeedback;
	static SettingT<INISettingCollection> bPoseMatching;
	static SettingT<INISettingCollection> fCameraDist;
	static SettingT<INISettingCollection> fMaxTrackingDist;
	static SettingT<INISettingCollection> fMinTrackingDist;
	static SettingT<INISettingCollection> fDriveGain;
	static inline float fFeedbackTimeMS = 3000.0f;
	static SettingT<INISettingCollection> fPelvisOffsetDamping;
	static SettingT<INISettingCollection> fVertErrorGain;
	static SettingT<INISettingCollection> iNumFramesFootEaseOut;
	static SettingT<INISettingCollection> fControllerTetherLen;
	static SettingT<INISettingCollection> fEaseAngleShutOff;
	static SettingT<INISettingCollection> fHierarchyGain;
	static SettingT<INISettingCollection> fVelocityDamping;
	static SettingT<INISettingCollection> fAccelerationGain;
	static SettingT<INISettingCollection> fVelocityGain;
	static SettingT<INISettingCollection> fPositionGain;
	static SettingT<INISettingCollection> fPositionMaxLinearVelocity;
	static SettingT<INISettingCollection> fPositionMaxAngularVelocity;
	static SettingT<INISettingCollection> fSnapGain;
	static SettingT<INISettingCollection> fSnapMaxLinearVelocity;
	static SettingT<INISettingCollection> fSnapMaxAngularVelocity;
	static SettingT<INISettingCollection> fSnapMaxLinearDistance;
	static SettingT<INISettingCollection> fSnapMaxAngularDistance;
	static SettingT<INISettingCollection> bFootPlacementOn;
	static SettingT<INISettingCollection> fMaxFootCastMilliSec;
	static SettingT<INISettingCollection> uiFeedbackTimeout;
	static SettingT<INISettingCollection> fRagdollFeedback;
	static SettingT<INISettingCollection> bRigidBodyController;
	static PoseMap_t PoseMap;
	static SkeletonMap_t SkeletonMap;
};

// IDA's hkArray of the foot IK legs (the PDB's hkArray<bhkRagdollController::FootIkLegInfo>).
using hkArray_bhkRagdollController_FootIkLegInfo = hkArray<bhkRagdollController::FootIkLegInfo>;

inline float fMaxTrackLen = 5.0f;
inline constexpr fnvd::Vtable bhkRagdollController_vftable{};
inline float f0_69999999 = 0.7f;

// INISettingCollection: 0xfbd10f calls 0x44f440.
extern SettingT<INISettingCollection> gs_fAngleMax_LookIK;

// INISettingCollection: 0xfbd136 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fAngleMaxEase_LookIK;

// INISettingCollection: 0xfbd19f calls 0x44f440.
extern SettingT<INISettingCollection> gs_fOnOffGain_FootIK;

// INISettingCollection: 0xfbd1cf calls 0x44f440.
extern SettingT<INISettingCollection> gs_fGroundAscendingGain_FootIK;

// INISettingCollection: 0xfbd1ff calls 0x44f440.
extern SettingT<INISettingCollection> gs_fGroundDescendingGain_FootIK;

// INISettingCollection: 0xfbd22f calls 0x44f440.
extern SettingT<INISettingCollection> gs_fFootRaisedGain_FootIK;

// INISettingCollection: 0xfbd256 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fFootPlantedGain_FootIK;

// INISettingCollection: 0xfbd2af calls 0x44f440.
extern SettingT<INISettingCollection> gs_fOriginalGroundHeightMS_FootIK;

// INISettingCollection: 0xfbd2df calls 0x44f440.
extern SettingT<INISettingCollection> gs_fAnkleOffset_FootIK;

// INISettingCollection: 0xfbd30f calls 0x44f440.
extern SettingT<INISettingCollection> gs_fPelvisUpDownBias_FootIK;

// INISettingCollection: 0xfbd3ef calls 0x44f440.
extern SettingT<INISettingCollection> gs_fMaxStepVertError_FootIK;
