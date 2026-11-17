#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSSystem/BSTMessageQueue.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"
#include "Fallout/AI/Movement/ActorPathHandler.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"

class Actor;
class ActorPathingMessageQueue;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class NavMeshInfo;
class PathingAvoidNodeArray;
class PathingSolution;
class TESObjectCELL;
class TESObjectREFR;
class TESWorldSpace;

using MessagePool_t = BSTStaticFreeList<ActorPathingMessage, 256>;

using BSSimpleArray_PathingNode_1024 = BSSimpleArray<PathingNode, 1024>;
using BSSimpleArray_VirtualPathingNode_1024 = BSSimpleArray<VirtualPathingNode, 1024>;

// Overrides only the destructor of NiRefObject.
class PathingSolution : public NiRefObject {
public:
    PathingSolution();
    ~PathingSolution() override;

    void ClearCurrentDetailedPath();
    uint8_t IsIncomplete();
    // The last of the current detailed path nodes, or null for none.
    PathingNode* GetLastNode();
    // The virtual node at auiIndex.
    VirtualPathingNode* GetVirtualNode(uint32_t aiIndex);
    // The node at auiIndex.
    PathingNode* GetNode(uint32_t aiIndex);
    // Whether the path has virtual nodes and, when a virtual range is loaded, detailed nodes.
    bool IsValid();
    // Empties the node and door arrays (freeing their memory) and resets the loaded range and the incomplete flag.
    PathingSolution* Clear();
    uint32_t GetNodeCount();
    // The length of the spline from node aiNodeIndex to the next.
    float ComputeSplineLength(int32_t aiNodeIndex);
    // The sum of the straight distances between consecutive nodes.
    float ComputeDetailedPathLength();
    // The point at spline parameter afCurrentSplineParameter: node index plus the fraction along the next spline.
    void ComputeLocationFromParameter(float afCurrentSplineParameter, NiPoint3& aOutLoc);
    // The path length from the first node to spline parameter afParameter.
    float ComputeDistanceToParameter(float afParameter);
    // The node a path percentage falls after, written to aiOutNodeIndex, plus the spline parameter within it.
    float GetSplineParametersFromPercentage(float afPathPercent, float afPathLength, uint32_t& aiOutNodeIndex);
    // The point afDistanceTraveled along the splines from parameter afCurrentParameter; the last node's location past
    // the end.
    NiPoint3 ComputeApproxLocationForDistance(float afCurrentParameter, float afDistanceTraveled);
    uint32_t GetVirtualNodeCount();
    // The count BuildPathToNavMeshSearchStart compares a virtual node index against.
    uint32_t GetCount();
    // Appends a detailed path node at aLocation with tangent aTangent.
    uint32_t AddPathingNode(const PathingLocation& aLocation, const NiPoint3& aTangent);
    // Appends a virtual node at apLocation; returns its index.
    uint32_t AddVirtualPathingNode(const PathingLocation& apLocation);
    // Appends a virtual node at aLocation through the door apDoor and, for a door, adds the door's form ID to DoorIDs;
    // returns the node's index.
    uint32_t AddVirtualPathingNode(const PathingLocation& aLocation, TESObjectREFR* apDoor);
    void SetIncomplete(bool abIncomplete);
    // Removes the detailed path node at aiIndex.
    void RemoveNode(uint32_t aiIndex);
    // Removes the last detailed path node.
    void RemoveLastNode();
    // The first detailed path node, or null for none.
    PathingNode* GetFirstNode();
    // The last virtual node.
    VirtualPathingNode* GetLastVirtualNode();
    // Whether the door aDoorID is on the path.
    bool IsDoorInPath(uint32_t aDoorID);
    // Copies the loaded virtual node range, the incomplete flag and both node arrays to apDest.
    void CopyTo(PathingSolution* apDest);
    bool FindPointOnPath(
        float afCurrentParameter,
        float afDist,
        bool abForward,
        float* apNewParameter,
        NiPoint3* apNewPoint
    );
    uint32_t AddVirtualPathingNode_3(const NavMeshInfo* apNavMeshInfo);
    int32_t AddNode_Door(const NavMeshInfo* apNavMeshInfo, NiPoint3* apLocation, TESObjectREFR* apDoor);
    float ComputeVirtualPathLength();
    void Save(BGSSaveFormBuffer* a2);
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    uint32_t AddVirtualPathingNode_0(const NiPoint3& aLocation, const NavMeshInfo* apNavMeshInfo);
    uint32_t
    AddVirtualPathingNode_006D1520(const NiPoint3& aLocation, TESObjectCELL* apCell, TESWorldSpace* apWorldSpace);

    // NiRefObject (vtable and reference count) fills 0x0-0x8.
    BSSimpleArray<VirtualPathingNode, 1024> VirtualPathingNodes;
    int32_t iFirstLoadedVirtualNodeIndex;
    int32_t iLastLoadedVirtualNodeIndex;
    BSSimpleArray<PathingNode, 1024> CurrentPathingNodes;
    BSSimpleArray<uint32_t, 1024> DoorIDs;
    uint8_t bIncompletePath;
};

// The reference-counted queue of pathing messages (ActorPathingMessage) an actor's mover receives from the pathing
// thread; its smart pointer releases it through the NiRefObject base.
class ActorPathingMessageQueue : public BSTCommonLLMessageQueue<ActorPathingMessage>, public NiRefObject {
public:
    ActorPathingMessageQueue();
    ~ActorPathingMessageQueue();

    // Uses NiMemObject's operator delete; declared here because BSTMessageQueue's makes the inherited one ambiguous.
    static void operator delete(void* apMemory, size_t astSize) {
        NiMemObject::operator delete(apMemory, astSize);
    }

    static MessagePool_t MessagePool;
};

NiSmartPointer(ActorPathingMessageQueue);

// Where an ActorMover is in carrying out a move.
enum ACTOR_MOVER_STATE : int32_t {
    ACTOR_MOVER_STATE_INVALID = 0x0,
    ACTOR_MOVER_STATE_INIT = 0x1,
    ACTOR_MOVER_STATE_WAITING = 0x2,
    ACTOR_MOVER_STATE_MOVING = 0x3,
    ACTOR_MOVER_STATE_ROTATING = 0x4,
    ACTOR_MOVER_STATE_LAST_UPDATE = 0x5,
    ACTOR_MOVER_STATE_FINISHED = 0x6,
    ACTOR_MOVER_STATE_FAILED = 0x7,
    ACTOR_MOVER_STATE_FAILED_PROCESS_UPDATE = 0x8,
};

// Moves one actor along a path: asks for paths, follows them and tracks the move mode and the point to face.
class ActorMover {
public:
    enum END_OF_PATH_CHECK_RESULT : int32_t {
        END_OF_PATH_OK = 0,
        END_OF_PATH_TELEPORT = 1,
        END_OF_PATH_FAILED = 2,
    };

    // The move mode bits (GetMoveMode, uiPreferredMoveMode, Actor::SetMoveMode).
    enum MovementFlags : int32_t {
        MF_FORWARD = 0x1,
        MF_BACKWARD = 0x2,
        MF_LEFT = 0x4,
        MF_RIGHT = 0x8,
        MF_TURNLEFT = 0x10,
        MF_TURNRIGHT = 0x20,
        MF_NONCONTROLLER = 0x40,
        MF_WALKING = 0x100,
        MF_RUNNING = 0x200,
        MF_SNEAKING = 0x400,
        MF_SWIMMING = 0x800,
        MF_JUMP = 0x1000,
        MF_FLYING = 0x2000,
        MF_FALL = 0x4000,
        MF_SLIDE = 0x8000,
    };

    ActorMover(Actor* apActor);
    virtual ~ActorMover();

    virtual void SetMoveModePreference(uint32_t auiPreferredMoveMode);
    virtual void ClearMoveModePreference(uint32_t auiPreferredMoveMode);
    virtual void ForceMoveMode(uint32_t aiMoveMode);
    virtual void ClearForcedMoveMode();
    virtual void Update(float fdeltaT);
    virtual void UpdateTurnTimer(float) {}
    virtual uint32_t GetPreferredMoveMode() {
        return uiPreferredMoveMode;
    }
    virtual uint32_t GetMoveMode();
    // copies the move delta of this frame into aDelta; false when there is none
    virtual bool GetMoveDelta(NiPoint3& aDelta);
    virtual void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    virtual void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    virtual void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    virtual void FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Resets the mover's path state for a load; the buffer is unused.
    virtual void Revert(BGSLoadFormBuffer* apLoadGameBuffer);

    // Where the actor will be afTime seconds ahead, written to apOut.
    NiPoint3* GetAnticipatedLocation(NiPoint3* apOut, float afTime);
    // Informs the path handler that apReference was removed.
    void RemoveReference(TESObjectREFR* apReference);
    // Gets the current path goal.
    bool GetCurrentGoal(PathingLocation* apRetLocation);
    // The path handler's current node index, or 0 without a handler.
    uint32_t GetCurrentNodeIndex();
    // Gets the path location at auiNodeIndex.
    bool GetLocationAt(uint32_t auiNodeIndex, NiPoint3* apLocation);
    // Sets the point the actor should look at.
    void SetLookAtTarget(NiPoint3 aPoint);
    // Finishes the mover: clears the request, path, path handler and forced move mode.
    void ClearCurrentData();
    bool IsActorInLoadedArea();
    bool IsPathingComplete();
    PathingLocation* GetFailedPathDestination();
    bool IsPathValid();
    void SetPathingDoorReference(TESObjectREFR* apDoor);
    void FinishMoving(bool abClearHandler);
    // Stops now; a stop queued for the next update is finished here as well.
    void ForceStopMoving();
    // Empty function, called at the end of ForceStopMoving.
    void DebugOnEvent() {}
    bool IsPathing();
    bool IsWaitingForAvoidance();
    bool BackGroundBuildPath(const PathingRequest* apRequest);
    void ResumeMoving();
    bool IsRotating();
    // Ends the current move.
    void StopMoving();
    void DoStopMoving();
    void SetPathingFailed(bool abValue);
    // Clears the outstanding path request and conditionally clears the waiting flag.
    void ClearDataForNewPathingRequest(bool abClearWaitingFlag);
    void OnMoveToLow();
    // Adjusts the mover for an actor entering the high process.
    void OnMoveToHigh();
    // Asks for a path to aGoalLocation that ends facing afAngleToFace.
    bool SetGoalAndAngle(
        const NiPoint3& aGoalLocation,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldSpace,
        float afAngleToFace,
        float afTargetRadius,
        PathingAvoidNodeArray* apAvoidNodes
    );
    // Asks for a path to where apTargetRef is, ending facing that reference's angle.
    bool SetGoalAndAngle(TESObjectREFR* apTargetRef, float afTargetRadius, PathingAvoidNodeArray* apAvoidNodes);
    // Starts a path search for a copy of apRequest.
    bool SetGoal(const PathingRequest* apRequest);
    // Asks for a path to aGoalLocation.
    bool SetGoal(
        const NiPoint3& aGoalLocation,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldSpace,
        float afTargetRadius,
        PathingAvoidNodeArray* apAvoidNodes
    );
    // Stops the actor facing a point.
    void ClearLookAtTarget();
    bool HasLookAtTarget() const;
    bool RequestRotateActor(float afTargetAngle, bool abForceTurn);
    // Turns the actor to face akTargetPos; the position is passed by value.
    bool RequestRotateActor(NiPoint3 akTargetPos, bool abForceTurn);
    // Returns the whole byte; bool would assume 0/1.
    uint8_t DidPathingFail();
    // Returns bWaitingForPath's whole byte.
    uint8_t IsWaitingOnPath();
    // Returns bStopMovementNextUpdate's whole byte.
    uint8_t IsStoppingMovement();
    // Returns spCurrentRequest.
    PathingRequest* GetCurrentRequest();
    // Whether the current path crosses the area around the player.
    bool ComputePathPassesThroughPlayersArea();
    uint32_t GetPreferredMoveModeType() const {
        return uiPreferredMoveMode & 0xFF00;
    }
    void SetInDialog(bool abVal);
    bool IsMovementBlocked() const;
    float GetCurrentPercent() const;
    float GetDistanceTraveled() const;
    float GetDistanceRemaining() const;
    bool IsPathIncomplete() const;
    bool MoveActorToLocation(const NiPoint3& aNewLoc);
    ActorMover::END_OF_PATH_CHECK_RESULT EndOfCurrentPathCheck();
    bool GetLookAtPoint(NiPoint3& apPoint) const;
    bool SetGoal(TESObjectREFR* apReference, float afTargetRadius, PathingAvoidNodeArray* apAvoidNodes);
    void DoStartMoving(float afDelta);
    void UpdateMovement(float fdeltaT);
    void HandleProcessLevelChange();
    void SetCurrentFrameMoveModeAndDelta();
    NiPoint3* GetNextNodeLocation(NiPoint3* apResult);
    bool GetCurrentGoal_009DE2B0(NiPoint3* apRetLocation);
    void WarpToNextNode();
    void HandleMovementResult(ActorPathHandler::PATH_HANDLER_RESULT aeResult);

    // Fills apRequest in for apActor and the goal.
    static void BuildRequest(
        Actor* apActor,
        PathingRequest* apRequest,
        const NiPoint3& aGoalLocation,
        TESObjectCELL* apCell,
        TESWorldSpace* apWorldSpace,
        float afTargetRadius,
        PathingAvoidNodeArray* apAvoidNodes
    );
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // The point the actor is turned towards while bFaceTargetPoint is set.
    NiPoint3 LookAtPoint;
    NiPoint3 CachedMoveDelta;
    NiPointer<PathingRequest> spCurrentRequest;
    NiPointer<PathingSolution> spCurrentPath;
    ActorPathHandler* pCurrentPathHandler;
    Actor* pActor;
    uint32_t DetectionDoorID;
    NiPointer<ActorPathingMessageQueue> spMessageQueue;

    // The preferred move mode type sits in bits 8-15.
    uint32_t uiPreferredMoveMode;
    uint32_t uiForcedMoveMode;
    uint32_t iCachedLastValidMoveMode;
    uint16_t iPreviousNode;
    uint16_t iNextNode;
    PathingLocation FailedPathDestination;
    ACTOR_MOVER_STATE eState;
    // Tested as a whole byte; bool would assume 0/1.
    uint8_t bPathingFailed;
    // Tested as a whole byte; bool would assume 0/1.
    uint8_t bForceMoveMode;
    bool bStopMovementNextUpdate;
    bool bFaceTargetPoint;
    // Tested as a whole byte; bool would assume 0/1.
    uint8_t bWaitingForPath;
    bool bActorChangedProcessLevel;
    bool bInDialog;
    // Tested as a whole byte; bool would assume 0/1.
    uint8_t bHaveValidDeltaThisFrame;
    bool bStopCurrentMoveAnim;
    uint8_t padding79[3];
    uint32_t iLastFailureTimeStamp;
    uint32_t iFirstFailureTimeStamp;
    // The compare is signed.
    int32_t iContinuedFailureCount;
};
