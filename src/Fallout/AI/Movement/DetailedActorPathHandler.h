#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout/AI/Movement/ActorPathHandler.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class Actor;
class ActorMover;

// The path handler that follows a detailed path node by node (PDB: DetailedActorPathHandler : ActorPathHandler).
// The PC offsets match the PDB's up to 0xE3 (the ctor 0x9DFF40); the debug-geometry members after it are not on PC.
class DetailedActorPathHandler : public ActorPathHandler {
public:
	enum PATH_HANDLER_DOOR_STATE : int32_t {
		PATH_HANDLER_DOOR_STATE_NONE = 0,
		PATH_HANDLER_DOOR_STATE_WAITING_FOR_DOOR_TO_OPEN = 1,
		PATH_HANDLER_DOOR_STATE_FAILED = 2,
	};

	// The handler's state (IDA's enum; UNK_0 and UNK_7 are IDA's names for values the PDB does not name).
	enum PathingState : int32_t {
		UNK_0 = 0,
		DAP_INITIALIZING = 1,
		DAP_MOVING = 2,
		DAP_MOVING_WHILE_WAITING = 3,
		DAP_WAITING_FOR_UPDATED_PATH = 4,
		DAP_GOAL_REACHED = 5,
		DAP_ROTATING_WITHOUT_PATH = 6,
		UNK_7 = 7,
	};

	// The actor avoidance state (eAvoidState; values from the PDB, numbered as IDA's ActorAvoidState).
	enum PATH_AVOID_STATE : int32_t {
		PATH_AVOID_INVALID = 0,
		PATH_AVOID_OK = 1,
		PATH_AVOID_WAITING = 2,
		PATH_AVOID_REPATH = 3,
		PATH_AVOID_REPATH_IGNORE = 4,
		PATH_AVOID_FAILED = 5,
	};

	// The walk-to-run blend step (eBlendRunning; enumerators from the PDB, in declaration order).
	enum PATH_RUNNING_BLEND : int32_t {
		PATH_RUNNING_BLEND_INVALID = 0,
		PATH_RUNNING_BLEND_WALK_TO_RUN = 1,
		PATH_RUNNING_BLEND_FINISH_BLEND = 2,
		PATH_RUNNING_BLEND_DONE = 3,
	};

	// What UpdateActorAvoidance decided (PDB names and values).
	enum PATH_ACTOR_AVOID_RETURN : int32_t {
		PATH_ACTOR_AVOID_RETURN_OK = 0,
		PATH_ACTOR_AVOID_RETURN_WAIT = 1,
		PATH_ACTOR_AVOID_RETURN_REPATH = 2,
		PATH_ACTOR_AVOID_RETURN_FAIL = 3,
	};

	struct ActorAvoidanceData;

	// An actor the path has to avoid and how well that actor detects the follower.
	struct ActorAvoidanceData {
		ActorAvoidanceData();
		ActorAvoidanceData(Actor* a1, float a2);

		// IDA name; not in the PDB. The exe's Add passes the address of its const reference, and Copy only reads it.
		DetailedActorPathHandler::ActorAvoidanceData* Copy(const DetailedActorPathHandler::ActorAvoidanceData* a2);

		Actor* pActor;
		float fDetection;
	};

	DetailedActorPathHandler(Actor* apActor, ActorMover* apMover);
	DetailedActorPathHandler(Actor* apActor, ActorMover* apMover, NiPointer<PathingSolution>& apPath,
		PathingRequest* apRequest);
	~DetailedActorPathHandler() override;

	// Except GetPercentTraveled and GetDistanceRemaining: the overrides forward to the exe.
	ActorPathHandler::PATH_HANDLER_RESULT Init(float afPercentTraveled, float afDeltaT) override;
	bool InitRotationOnly(float afTargetAngle, bool abForceTurn) override;
	ActorPathHandler::PATH_HANDLER_RESULT Update(float afDeltaT) override;
	ActorPathHandler::PATH_HANDLER_RESULT NoPathUpdate(float afDeltaT) override;
	ActorPathHandler::PATH_HANDLER_RESULT UpdateWhilePlayerWaits(float afDeltaT) override;
	float GetDistanceTraveled() override;
	float GetPercentTraveled() override;
	NiPoint3 GetNextNodeLocation() override;
	bool GetNodeLocation(uint32_t aiIndex, NiPoint3& arLocation) override;
	float GetDistanceRemaining() override;
	NiPoint3 ComputeApproximateLocation(float afDeltaT) override;
	uint32_t GetCurrentNodeIndex() override;
	bool IsDetailedPathHandler() override;
	void SetWaitForUpdatedPath(PathingRequest* apRequest) override;
	bool IsWaitingForAvoidance() override;
	bool IsMovementBlocked() override;
	bool ComputePathPassesThroughPlayersArea() override;
	uint32_t GetMoveMode() override;
	bool GetMoveDelta(NiPoint3& aDelta) override;
	bool GetNextTetherPoint(NiPoint3& arPoint) override;
	void ForceMoveMode(uint32_t aiMoveMode) override;
	void HandlePreviousPathTransition(ActorPathHandler* apOldHander) override;
	void HandleDeletedReference(TESObjectREFR* apRef) override;
	void PrintCurrentStatus(IDebugText* apDebugText, DEBUGTEXT_LOCATION aLoc, DEBUGTEXT_JUSTIFY aJust) override;
	void ClearMovement() override;
	bool WarpToNextNode() override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;

	// Whether the door aDoorID lies on the path.
	bool IsDoorInPath(uint32_t aDoorID);
	// Starts turning towards afGoalAngle.
	bool InitRotationMode(float afGoalAngle, bool abForceTurn, bool abPathTransition);
	// Turns the actor's looking angle towards afLookingAngle, at most afDeltaT * 75 degrees (in radians) this frame.
	void UpdateLooking(float afDeltaT, float afLookingAngle);
	void ComputeSimpleMoveDelta(float afDeltaT);
	void ResetStaticAvoidance();
	void ResetActorAvoidance();
	// Whether the actor ends the path turning to face the request's goal angle: only on the last virtual node, and
	// not while sitting or sleeping. Returns whether the rotation mode started.
	bool CheckForEndRotate();
	void ResetMoveModeAndDelta();
	// Finds the actors in the way this frame and decides whether to wait for them (WAIT), to repath around them with
	// an avoid node array (REPATH), or to give up on an occupied goal (FAIL); OK when nothing changes.
	PATH_ACTOR_AVOID_RETURN UpdateActorAvoidance(float afDeltaT);
	// Sets the tether distances from the request's actor radius; always returns false.
	bool ComputeMinTetherDistance();
	// Steps the door state: opens a closed door on the path the mover detected and waits until it is open.
	DetailedActorPathHandler::PATH_HANDLER_DOOR_STATE HandleDoorOpening();
	float UpdateRotation(float afDeltaT);
	bool ComputeMoveMode(float afDeltaT);
	void RotateMoveDelta(float afDeltaT);
	void UpdateTetherPosition(float afDist);
	void UpdateTether(float afDeltaT);
	void ComputeHeadingAndLooking(float& arfHeading, float& arfLooking);
	float UpdateHeading(float fdeltaT, float& afSlowDown);
	bool UpdateStaticAvoidance(float afDeltaT);
	void FindPathStartingLocation(float afPercentTraveled);
	ActorPathHandler::PATH_HANDLER_RESULT CheckGoalReached();

	// Fills aRetCandidates with the actors apActor may have to avoid moving along aMoveDelta
	// in aMoveMode for apHandler's request.
	static void BuildAvoidanceList(Actor* apActor, PathingRequest* aspRequest, uint32_t aMoveMode,
		const NiPoint3& aMoveDelta, DetailedActorPathHandler* apHandler,
		BSSimpleArray<DetailedActorPathHandler::ActorAvoidanceData, 1024>& aRetCandidates);
	static Actor* GetFollowLeader(Actor* apActor);
	static void UpdateAvoidNodesForPath(Actor* apActor, PathingRequest* apRequest, DetailedActorPathHandler* apHandler);
	static bool UpdateAvoidanceCandidateList();
	static bool CheckDetection(Actor* apActor, DetailedActorPathHandler* apHandler, Actor* apOtherActor,
		const NiPoint3& aLocation, const NiPoint3& aVelocityDir, float afDistanceCheck, float afAvoidAngleCone,
		float& afOutDetection, bool abCheckOnly);

	float fTetherMinMove;
	float fTetherDistFromActor;
	NiPoint3 MoveDelta;
	NiPoint3 BlockedTargetPoint;
	NiPoint3 TetherTargetPoint;
	NiPoint3 LastLocation;
	NiPoint3 LastTetherPoint;
	// The actors being avoided. The exe stores Actor* here (UpdateActorAvoidance's AddHead of its BSSimpleArray<Actor*>
	// items, and its loop reading each item as an Actor).
	BSSimpleList<Actor*> ActorAvoidList;
	uint32_t MoveMode;
	uint32_t iCurrentNodeIndex;
	float fRotateGoal;
	float fPathLength;
	float fCurrentSplineT;
	float fActorAvoidanceTimer;
	float fAvoidanceDetectionValue;
	int32_t iUnblockDirection;
	float fAlternateFacingAngle;
	float fBlockedStartAngle;
	float fBlockTime;
	float fTimeInAnim;
	float fRunWalkTime;
	float fMinRotateTime;
	float fWaitTime;
	float fLastFrameMinMoveDelta;
	NiPoint3 LastFrameMinMoveDelta;
	// The squared radius of the goal: Init sets it to BSMAX(PathingRequest::GetTargetRadius squared, 0.1), 1600 for
	// an incomplete path and 6400 when the last node is a teleport door.
	float fGoalRadiusSqr;
	// How long the actor has been falling (bhkCharacterController::GetFallTimeElapsed): Update resets the move mode
	// once a fall passes 0.4 s, and returns 4 when it is over 0.65 s.
	float fFallTime;
	float fDistanceTraveled;
	float fIgnoreLookAtPointTimer;
	// The last rotation applied; loaded from save version 0x12 on.
	float fLastRotationApplied;
	bool bIgnoreInitialRotation;
	bool bIgnoreInitialRotationKeepActorHeading;
	uint8_t padding0C2[2];
	// The path parameter the initial rotation is ignored up to (Update compares fCurrentSplineT against it).
	float fIgnoreInitialRotationTetherParam;
	PathingState eState;
	PATH_RUNNING_BLEND eBlendRunning;
	PATH_AVOID_STATE eAvoidState;
	// The door state HandleDoorOpening steps.
	PATH_HANDLER_DOOR_STATE eDoorState;
	TESObjectREFR* pCurrentDoor;
	bool bRotateMode;
	bool bRotateCCW;
	bool bMovementBlocked;
	// Whether the path's last node is a transition node (PathingNode::IsTransitionNode), which ends the update path.
	bool bEndUpdatePath;
	bool bWaitingForUpdate;
	bool bIgnoreLookAtPoint;
	bool bDeltaComputedThisFrame;
	uint8_t padding0E3[1];

	// Cosines of the actor avoidance cone half-angles, set by Pathing::Init.
	static inline float fActorAvoidConeAngle = 0.70710677f;
	static inline float fWiderActorAvoidConeAngle = 0.5f;
	static inline float fRunningActorAvoidConeAngle = 0.8660254f;
	static inline uint32_t uiAvoidanceCandidatesMaxSize;
	static inline int32_t uiAvoidanceCandidatesSize;
	static inline int32_t uiAvoidanceCandidatesTimestamp;
	// IDA: DetailedActorPathHandler::kAvoidanceCandidates.
	static NiTPrimitiveArray<Actor*> AvoidanceCandidates;
	static inline float fInnerGoalRadiusSqr = 25.0f;
};

// The line-of-sight test for a path through a closed door: crossing is allowed only between triangles of the door
// DoorID (any door when it is 0). Packed to byte alignment (vtable 0x1092484).
#pragma pack(push, 1)
class DoorInPathFilter : public Pathing::CheckLineOfSightFilter {
public:
	// The hybrid's implicit constructor, for the locals constructed later in place.
	DoorInPathFilter() = default;
	// Constructs the filter in apMemory for door aiDoorID.
	DoorInPathFilter(uint32_t aiDoorID);

	bool CanCrossEdge(FastNavMeshEdgeHandle hEdge, FastNavMeshEdgeHandle hMatchingEdge) override;

	uint32_t DoorID;
	bool bEnterTriWithDoor;
	bool bLeaveTriWithDoor;
	bool bEntirePathInDoor;
};

inline constexpr fnvd::Vtable DetailedActorPathHandler_vftable{};
inline constexpr fnvd::Vtable DoorInPathFilter_vftable{};

// The game settings DetailedActorPathHandler.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fRotationSpeedIncrease;
extern SettingT<INISettingCollection> fLODActorAvoidanceDistance;

// INISettingCollection: 0xfb0fe2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fAICombatTurnSpeedScale_Pathfinding;

// INISettingCollection: 0xfb0fb2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fAITurnSpeedScale_Pathfinding;

// INISettingCollection: 0xfb1042 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fCreatureCombatTurnSpeedScale_Pathfinding;

// INISettingCollection: 0xfb0da2 calls 0x44f440.
extern SettingT<INISettingCollection> fAvoidanceMinWaitTime;

// INISettingCollection: 0xfb0f22 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fRepathingWaitDistance_Pathfinding;

// INISettingCollection: 0xfb0f82 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fAvoidanceAvoidAllRadius_Pathfinding;

// INISettingCollection: 0xfb0e32 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fAvoidanceTimeCheck_Pathfinding;

// INISettingCollection: 0xfb0d0a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bUseActorAvoidance_Pathfinding;

// INISettingCollection: 0xfb1012 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fCreatureTurnSpeedScale_Pathfinding;

#pragma pack(pop)
