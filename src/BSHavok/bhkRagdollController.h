#pragma once

#include "BSCore/BSPrecisionTimer.h"
#include "BSCore/BSSimpleArray.h"
#include "BSHavok/bhkCachingShapePhantom.h"
#include "BSHavok/bhkPoseArray.h"
#include "BSHavok/bhkRagdollShareData.h"
#include "BSHavok/bhkShapePhantom.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Havok/Source/Animation/Animation/Ik/FootPlacement/hkaFootPlacementIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/QsTransform/hkQsTransform.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class bhkRigidBody;
class bhkShapePhantom;
class hkaPose;
class hkaPoseMatchingUtility;
class hkaRagdollInstance;
class hkaRagdollRigidBodyController;
class hkaSkeleton;
struct hkCapsuleShapeCinfo;
class hkpConstraintInstance;
class hkpRigidBody;
class NiQuaternion;

void* Reallocate(void* apMemory, uint32_t Size);
// Appends apBody's Havok body to arBones and the Havok constraint of each bhkConstraint on its action/constraint list
// to arConstraints. bhkRagdollController::FindBones, its one caller, passes its own two arrays.
void AddRagdollData(
    bhkRigidBody* apBody,
    hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBones,
    hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints
);
NiTransform& HK2NI(NiTransform& arNiTran, const hkQsTransform& arhkTran);

// The ragdoll's animation feedback settings and the bones it drives dynamically.
struct RagdollFeedbackData {
    // The feedback gains and limits (RAFD).
    struct RagdollFeedbackSystemData {
        RagdollFeedbackSystemData();

        // Swaps every field read from a big-endian file.
        void Endian();

        float fKeyDynIntrp;
        float fHeirGain;
        float fPosGain;
        float fVelGain;
        float fAccGain;
        float fSnapGain;
        float fVelDamp;
        float fSnapMaxLinVel;
        float fSnapMaxAngVel;
        float fSnapMaxLinDist;
        float fSnapMaxAngDist;
        float fPosMaxLinVel;
        float fPosMaxAngVel;
        uint32_t uiImpulseMult1;
        uint32_t uiImpulseMult2;
    };

    // Built through Init.
    RagdollFeedbackData();
    ~RagdollFeedbackData();

    void Init();
    bool Compare(RagdollFeedbackData* apOther);
    void Copy(const RagdollFeedbackData& arData);

    bool bEnabled;
    RagdollFeedbackSystemData SystemData;
    // The game builds and destroys it as BSSimpleArray<short, 1024>.
    BSSimpleArray<int16_t> DynamicBoneIdxA;
};

// The look IK's shared per-creature state: the head bone's skeleton and the head's forward direction in parent and in
// local space. The 12 bytes after the pointer are hkVector4's alignment gap, which none of the functions that use the
// record touches: InitLookIK stores only the pointer, InitBoneParams reads the two vectors. Only the layout is
// declared.
struct bhkLookIKData {
    bhkLookIKData() : pHeadSkel(nullptr) {}
    ~bhkLookIKData();

    hkaSkeleton* pHeadSkel;
    hkVector4 FwdParentMS;
    hkVector4 FwdLS;
};

// Drives an actor's ragdoll from its animation. Only part of the class is declared.
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
        float fDeadHeading;
        float fDeadPitch;
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
        FootIkLegInfo() : pSolver(nullptr), fResultError(0.0f), RagdollFootIdx(-1) {}

        hkaFootPlacementIkSolver* pSolver;
        float fResultError;
        FootRayCastResults CastResults[2];
        hkQsTransform AnkleLS;
        int16_t RagdollFootIdx;
    };

    // A bone and its local transform.
    struct BoneTransf {
        NiAVObject* pObj;
        NiTransform LocalTransform;
    };

    // One bone of the skeleton the ragdoll is built from: the scene graph node's name, its local transform and the
    // index of its parent bone.
    struct bhkSkeletonBone {
        const char* name;
        hkQsTransform transform;
        int16_t parentIndice;
    };

    // The motors of the ragdoll's constraints.
    class bhkRagdollPoweredConstraintController {
    public:
        // Turns on the motor of every constraint the ragdoll's bone-to-rigid-body map names.
        static void startMotors(hkaRagdollInstance* ragdoll);
        static void stopMotors(hkaRagdollInstance* ragdoll);
        static void driveToPose(hkaRagdollInstance* ragdoll, const hkQsTransform* desiredPose);
    };

    using PoseMap_t = NiTMap<const char*, NiPointer<bhkPoseArray>>;
    using SkeletonMap_t = NiTMap<const char*, NiPointer<bhkRagdollShareData>>;

    bhkRagdollController(NiAVObject* apObject, NiAVObject* apPose, const char* apScaledSkeletonName);
    // Slot 0 of the engine's vtable is the deleting destructor; `delete` calls it with flag 1.
    ~bhkRagdollController() override;

    // Casts a foot IK ray for the current leg.
    hkBool castRay(
        const hkVector4& arFromWS,
        const hkVector4& arToWS,
        float& arHitFractionOut,
        hkVector4& arNormalWSOut
    ) override;

    // Makes the ragdoll follow the head tracking.
    void ForceHeadTrack();
    // Switches the look inverse kinematics on or off.
    void SetLookIKActive(bool abActive);
    // Whether the ragdoll animation initialised.
    bool QRagdollAnimInit();
    // Moves the ragdoll's rigid bodies to the current 3D pose.
    void SyncRagdollTo3D();
    // The inverse scale matrix at +0x60.
    NiMatrix3& QScaleInv() {
        return ScaleInv;
    }
    // Sets whether the look-at IK uses the head pose.
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
    void SetAlignFeetToTerrain(bool abAlign);
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
    static inline float fPoseErrThreshold = 0.7f;

    // Reads the scene graph's pose into the local-space pose.
    void GetSGPoseLocalSpace();
    // Writes the local-space pose back to the scene graph.
    void SetSGPoseLocalSpace();
    // Drives the ragdoll's rigid bodies, from apRagdollPoseLS when it is not null.
    void UpdateRagdollRigidBodies(const hkQsTransform* apRagdollPoseLS);
    // Picks the best pose to match into apRagdollDrivePoseLS and sets arbUseDrivePose.
    void
    DoPoseMatching(const hkQsTransform& arWorldFromModel, hkQsTransform* apRagdollDrivePoseLS, bool& arbUseDrivePose);

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
    void ShowDebugLines(bool abKeyframed, bool abRagdoll, bool abFinal);
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
    bool
    ExtractBipedSetupData(hkaFootPlacementIkSolver::Setup& arSetupData, BSSimpleArray<const char*, 1024>& arboneNameA);
    // Adds the ragdoll's constraints to its world, or removes them; only the low byte of abAdd is read.
    void UpdateConstraintsWorld(int32_t abAdd);
    void GetRBForBones(hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arBoneRBA);
    void SetGrabIKEaseOut(bhkRagdollController::GRAB_IK_ARM arArm, bool abEnable);
    void SetGrabIKEnabled(bhkRagdollController::GRAB_IK_ARM arArm, bool abEnable);
    void SetGrabIKIntrpGain(bhkRagdollController::GRAB_IK_ARM arArm, float afGain);
    void SetGrabIKTarget(bhkRagdollController::GRAB_IK_ARM arArm, NiAVObject* apTarget);
    // Builds the skeleton bones of apObj and the nodes below it.
    void CreateSGSkeletonRec(
        NiAVObject* apObj,
        BSSimpleArray<bhkRagdollController::bhkSkeletonBone*, 1024>& arBoneA,
        NiTMap<NiAVObject*, int16_t>& arboneMap
    );

    // Sets the ragdoll animation flag. Callers pass the setting byte xor 1, not 0/1; bool would normalise it.
    static void SetRagdollAnim(bool abEnable);
    // The bGrabIK setting's byte as stored.
    static bool QGrabIK();
    // Stores abEnable in the bGrabIK setting as given.
    static void SetGrabIK(bool abEnable);
    // The bRagdollAnim setting's byte as stored.
    static bool QRagdollAnim();
    // The bLookIK setting's byte as stored.
    static bool QLookIK();
    // Stores abEnable in the bLookIK setting as given.
    static void SetLookIK(bool abEnable);
    // The game's heap (MemoryManager).
    static void* operator new(size_t blocksize);
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
    static void ResetPose(
        NiNode* apSkeleton,
        BSSimpleArray<BoneTransf, 1024>& arPoseA,
        bool abPrevUseVel,
        hkpMotion::MotionType aePrevMotion
    );
    // For apPose and each node under it, its namesake under apSkeleton: records it with its local transform in
    // arPoseA, then gives it apPose's local rotation and translation (the parent's for "Bip01 NonAccum").
    static void CopyPoseTo3DRec(NiNode* apSkeleton, NiAVObject* apPose, BSSimpleArray<BoneTransf, 1024>& arPoseA);
    // Copies apPose onto apSkeleton (CopyPoseTo3DRec) and updates it; first records in arbPrevUseVel whether the
    // skeleton's collision object used velocity (and clears it under the skeleton) and in arePrevMotion its body's
    // motion type (MOTION_FIXED without a body), keyframing the skeleton unless it was keyframed.
    static void CopyPoseTo3D(
        NiNode* apSkeleton,
        NiNode* apPose,
        BSSimpleArray<BoneTransf, 1024>& arPoseA,
        bool& arbPrevUseVel,
        hkpMotion::MotionType& arePrevMotion
    );
    // Collects the blended rigid bodies under apObject and their constraints.
    static void FindBones(
        NiAVObject* apObject,
        hkArray<hkpRigidBody*, hkContainerHeapAllocator>& arRigidBodies,
        hkArray<hkpConstraintInstance*, hkContainerHeapAllocator>& arConstraints
    );

    bool bQueueStopPoweredMotors;
    float fCurrentMatchError;
    RAGDOLL_UPDATE_TYPE eRBUpdateMethod;
    hkQsTransform WorldFromModel;
    bool bFullyDynamic;
    bool bEnableRagdollAnim;
    bool bInitRagdollAnim;
    bool bEaseOutRagdollAnim;
    // The constructor clears bQueuedWarp.
    bool bQueuedWarp;
    bool bBoneUpdate;
    hkaRagdollInstance* pRagdollInstance;
    hkaRagdollRigidBodyController* pRagdollRBController;
    bool bReinitRBController;
    // The head's pose the look-at IK starts from.
    hkaPose* pHeadPose;
    NiAVObject* pRagdollRoot;
    uint32_t iGroup;
    NiMatrix3 ScaleInv;
    NiPointer<NiNode> spSkeletonDebug;
    hkaPose* pSGPose;
    bool bUsingMappers;
    hkpCachingShapePhantomRay* pControllerPhantom;
    // The animation's pose in local space and the scene graph's bones.
    hkArray<hkQsTransform> AnimPoseLS;
    BSSimpleArray<NiAVObject*> SGBonesA;
    bool bInitLookIK;
    bool bEnableLookIK;
    bool bEaseOutLookIK;
    bool bActiveLookIK;
    // The constructor clears it.
    bool bApplyHeadOffset;
    uint8_t padding0B5[0xB];
    hkQuaternion CurrentHeadRotOffset;
    hkVector4 CurrentTargetWS;
    hkVector4 EyeOffset;
    LookIKBoneParams BoneParamA[NUM_LOOKIK_BONES];
    bool bUseHeadPose;
    uint8_t padding191[3];
    LookIKBone eCurrentBone;
    // Cleared by the constructor and not otherwise used.
    bool bTargetOutsideHeadLimit;
    float fEyeHeading;
    float fEyePitch;
    // The constructor clears both.
    bool bTargetBehind;
    bool bHeadSwitchSides;
    float fMaxEyePitchAngle;
    uint8_t padding1AC[0x4];
    GrabIKInfo GrabIKState[2];
    GrabIKSetup GrabSetup[2];
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
    // The constructor zeroes fDownCastDistance and pMatchUtil and clears PoseAStr and spPoseA; the destructor frees
    // pMatchUtil, releases PoseAStr and releases its reference to spPoseA.
    float fDownCastDistance;
    hkaPoseMatchingUtility* pMatchUtil;
    NiFixedString PoseAStr;
    NiPointer<bhkPoseArray> spPoseA;
    bool bInitPoseMatch;
    bool bEnablePoseMatch;
    bool bActivePoseMatch;
    // The pose matching's state, from iCurrentMatchPoseIdx to iEditorForcePoseIdx. DoPoseMatching reads the timer, the
    // root location, the displacement maximum and the time, and tests bDisablePoseMatchOnDisplace and
    // iEditorForcePoseIdx.
    int32_t iCurrentMatchPoseIdx;
    uint8_t padding25C[0x4];
    BSPrecisionTimer PoseMatchTimer;
    NiPoint3 PoseMatchRootLoc;
    float fPoseMatchDisplacementMax;
    float fPoseMatchTime;
    bool bDisablePoseMatchOnDisplace;
    int32_t iEditorForcePoseIdx;
    bool bInitRagdollFeedback;
    bool bEnableRagdollFeedback;
    bool bActiveRagdollFeedback;
    bool bEaseOutFeedback;
    float fCurrFeedbackInterp;
    uint32_t uiFeedbackTimeoutCount;
    uint32_t uiFeedbackImpulseStrong;
    uint32_t uiFeedbackImpulseWeak;
    // The constructor clears SkeletonShareName, spRagdollSharedData and FeedbackTimer, and the destructor releases the
    // name and the shared data's reference count.
    NiFixedString SkeletonShareName;
    NiPointer<bhkRagdollShareData> spRagdollSharedData;
    BSPrecisionTimer FeedbackTimer;
    int32_t nHeadBone;

    static SettingT<INISettingCollection> bGrabIK;
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

protected:
    static SettingT<INISettingCollection> fAngleMax;
    static SettingT<INISettingCollection> fAngleMaxEase;
    static SettingT<INISettingCollection> fOnOffGain;
    static SettingT<INISettingCollection> fGroundAscendingGain;
    static SettingT<INISettingCollection> fGroundDescendingGain;
    static SettingT<INISettingCollection> fFootRaisedGain;
    static SettingT<INISettingCollection> fFootPlantedGain;
    static SettingT<INISettingCollection> fOriginalGroundHeightMS;
    static SettingT<INISettingCollection> fAnkleOffset;
    static SettingT<INISettingCollection> fPelvisUpDownBias;
    static SettingT<INISettingCollection> fMaxStepVertError;
};

using hkArray_bhkRagdollController_FootIkLegInfo = hkArray<bhkRagdollController::FootIkLegInfo>;

inline float fMaxTrackLen = 5.0f;

inline bool bhkRagdollController::QFootIKActive() {
    return bActiveFootIK;
}

inline bool bhkRagdollController::QFootIK() {
    return bhkRagdollController::bFootIK.Bool();
}

inline void bhkRagdollController::SetRagdollAnim(bool abEnable) {
    bhkRagdollController::bRagdollAnim.Bool() = abEnable;
}

inline void bhkRagdollController::SetLookAtMaxAngle(bhkRagdollController::LookIKBone aeBone, float afMaxAngle) {
    BoneParamA[aeBone].fLookAtMaxAngle = afMaxAngle;
}

inline bool bhkRagdollController::QRagdollAnim() {
    return bhkRagdollController::bRagdollAnim.Bool();
}

inline void bhkRagdollController::SetLookIKEnable(bool abEnable) {
    bEnableLookIK = bInitLookIK && abEnable;
}

inline void bhkRagdollController::SetFootIKEnable(bool abEnable) {
    bEnableFootIK = bInitFootIK && abEnable;
}

inline void bhkRagdollController::SetGrabIKEnable(bool abEnable) {
    bEnableGrabIK = bInitGrabIK && abEnable;
}

inline bool bhkRagdollController::QGrabIK() {
    return bhkRagdollController::bGrabIK.Bool();
}

inline void bhkRagdollController::SetGrabIK(bool abEnable) {
    bhkRagdollController::bGrabIK.Bool() = abEnable;
}

inline void bhkRagdollController::SetLookIK(bool abEnable) {
    bhkRagdollController::bLookIK.Bool() = abEnable;
}

inline void bhkRagdollController::SetFootIK(bool abEnable) {
    bhkRagdollController::bFootIK.Bool() = abEnable;
}

inline void bhkRagdollController::SetUsePhantomFootIKCasts(bool abUsePhantom) {
    bUsePhantomFootIKCasts = abUsePhantom;
}

inline void bhkRagdollController::SetPoseMatchEnable(bool abEnable) {
    bEnablePoseMatch = bInitPoseMatch && abEnable;
}

inline void bhkRagdollController::SetPhantom(bhkShapePhantom* apPhantom) {
    pControllerPhantom = static_cast<hkpCachingShapePhantomRay*>(**apPhantom);
}

inline bool bhkRagdollController::QRagdollFeedbackEnable() {
    return bEnableRagdollFeedback;
}

inline uint32_t bhkRagdollController::QFeedbackImpulseWeak() {
    return uiFeedbackImpulseWeak;
}

inline uint32_t bhkRagdollController::QFeedbackImpulseStrong() {
    return uiFeedbackImpulseStrong;
}

inline void bhkRagdollController::SetRagdollAnimEaseOut(bool abEaseOut) {
    bEaseOutFootIK = abEaseOut;
    GrabIKState[1].bEaseOutGrabIK = abEaseOut;
    GrabIKState[0].bEaseOutGrabIK = abEaseOut;
    bEaseOutLookIK = abEaseOut;
    bEaseOutRagdollAnim = abEaseOut;
}

inline void bhkRagdollController::SetHeadOffset(const NiQuaternion& arRot) {
    NI2HKMIGRATION_QUATERNION(CurrentHeadRotOffset, arRot);
}

inline void bhkRagdollController::SetGrabIKEaseOut(bhkRagdollController::GRAB_IK_ARM arArm, bool abEnable) {
    GrabIKState[arArm].bEaseOutGrabIK = abEnable;
}

inline void bhkRagdollController::ReinitRBControllerOnUpdate(bool abReinit) {
    bReinitRBController = abReinit;
}

inline bool bhkRagdollController::QRagdollFeedbackActive() {
    return bActiveRagdollFeedback;
}

inline RagdollFeedbackData::RagdollFeedbackSystemData::RagdollFeedbackSystemData()
  : fKeyDynIntrp(0.9f), fHeirGain(0.8f), fPosGain(0.4f), fVelGain(0.2f), fAccGain(0.1f), fSnapGain(0.3f),
    fVelDamp(0.0f), fSnapMaxLinVel(50.0f), fSnapMaxAngVel(50.0f), fSnapMaxLinDist(25.0f), fSnapMaxAngDist(25.0f),
    fPosMaxLinVel(50.0f), fPosMaxAngVel(50.0f), uiImpulseMult1(10000), uiImpulseMult2(30000) {}

inline void RagdollFeedbackData::RagdollFeedbackSystemData::Endian() {
    BSEndian(fKeyDynIntrp, false);
    BSEndian(fHeirGain, false);
    BSEndian(fPosGain, false);
    BSEndian(fVelGain, false);
    BSEndian(fAccGain, false);
    BSEndian(fSnapGain, false);
    BSEndian(fVelDamp, false);
    BSEndian(fSnapMaxLinVel, false);
    BSEndian(fSnapMaxAngVel, false);
    BSEndian(fSnapMaxLinDist, false);
    BSEndian(fSnapMaxAngDist, false);
    BSEndian(fPosMaxLinVel, false);
    BSEndian(fPosMaxAngVel, false);
    BSEndian(uiImpulseMult1, false);
    BSEndian(uiImpulseMult2, false);
}

inline RagdollFeedbackData::RagdollFeedbackData() : bEnabled(false) {}
