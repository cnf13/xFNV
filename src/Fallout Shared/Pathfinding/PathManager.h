#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSReadWriteLock.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSpinLock.h"
#include "BSCore/BSTFreeList.h"
#include "BSCore/BSTStaticFreeList.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSTMessageQueue.h"
#include "BSSystem/BSTaskletGroup.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/Pathfinding/ActorPathingMessage.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class ActorPathingMessageQueue;
class InventoryChanges;
class NavMeshEdgeLocation;
class PathingLocation;
class PathingRequest;
class PathingSolution;
class PathingTaskData;
class PathManagerImpl;
struct RayCastPath;
struct RayCastPathRequest;

// The result of a ray cast path request.
enum RAYCASTPATH_RESULT : int32_t {
    RCP_NONE = 0,
    RCP_SUCCESS = 1,
    RCP_FAILED = 2,
};

// The pathfinding front end: hands path requests to its implementation and the pathing thread. Only some of its members
// are declared.
class PathManager {
public:
    PathManager();
    ~PathManager();

    // pImpl.
    PathManagerImpl* QImpl() {
        return pImpl;
    }
    // Closes the implementation, deletes it and clears pImpl, so the singleton's destructor at exit finds nothing to
    // delete. Called from WinMain's shutdown.
    void Release();
    void ClearInventoryPointersFromCurrentRequests(InventoryChanges* apInventory);
    // Steps the pathing implementation on the calling thread.
    void Update();
    void Pause();
    // Whether the pathing implementation is paused.
    bool QPaused();
    // Lets the pathing implementation run again.
    void Resume();
    bool QIdle();
    void CancelCurrentPathsAndPause();
    // Queues aspRequest for the pathing thread, answering through aspMessageQueue. Both smart pointers come by value
    // and are released when the function returns.
    bool
    BuildPath(NiPointer<PathingRequest> aspRequest, NiPointer<ActorPathingMessageQueue> aspMessageQueue, bool abQueue);
    RAYCASTPATH_RESULT GetRayCastPath(
        PathingRequest* apRequest,
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath
    );
    RAYCASTPATH_RESULT GetRayCastPathToNavmesh(
        PathingRequest* apRequest,
        const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
        PathingLocation* aGoalLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath,
        bool abStart
    );
    // Draws or hides the path node aspNode: passes both on to the implementation's DrawPath.
    bool DrawPath(NiPointer<NiNode>& aspNode, bool abDraw);
    bool Initialize();

    // The singleton, created on first use.
    static PathManager* QInstance();

    PathManagerImpl* pImpl;
    BSPerformanceTimer PerformanceTimer;
};

class PathingTaskData : public BSWin32TaskletData {
public:
    PathingTaskData(const NiPointer<PathingRequest>& aspRequest, NiPointer<ActorPathingMessageQueue> aspMessageQueue);
    ~PathingTaskData() override;

    void Process() override;

    // Runs the pending ray cast requests whose paths have no result yet, until more than ten ray casts are made;
    // returns the number of ray casts.
    int32_t ComputeRayCasts();
    uint8_t GetWaitingForRayCasts();
    bool AllRayCastPathComputed();
    RAYCASTPATH_RESULT GetRayCastPath(
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath
    );
    RAYCASTPATH_RESULT GetRayCastPathToNavmesh(
        const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
        PathingLocation& aGoalLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath,
        bool abStart
    );
    // Takes the request and the actor's message queue. The queue smart pointer is taken by value and released.
    void Setup(const NiPointer<PathingRequest>& aspRequest, NiPointer<ActorPathingMessageQueue> aspMessageQueue);

    // Frees with ourFree.
    static void operator delete(void* apMemory);

    NiPointer<PathingRequest> spRequest;
    NiPointer<PathingSolution> spSolution;
    BSSpinLock RayCastLock;
    BSSimpleArray<RayCastPathRequest*, 1024> RayCastRequests;
    BSSimpleArray<RayCastPath*, 1024> RayCastPaths;
    NiPointer<ActorPathingMessageQueue> spMessageQueue;
    bool bWaitingForRayCasts;
    uint8_t padding65[3];
    uint8_t padding68[0x18];
};

// A message from an actor to the pathing thread: its kind and the task the message is about.
class PathingMessage {
public:
    PathingMessage();
    PathingMessage(PATHING_MESSAGE_TYPE aType, PathingTaskData* apTaskData);

    // Copies msgType and pTaskData and returns this.
    PathingMessage& operator=(const PathingMessage& aMsg);

    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);

    PATHING_MESSAGE_TYPE msgType;
    PathingTaskData* pTaskData;
};

class PathManagerImpl : public BSTaskletGroup {
public:
    // The values eState holds.
    enum PATH_MANAGER_STATE : int32_t {
        PATH_MANAGER_STATE_IDLE = 0,
        PATH_MANAGER_STATE_WAITING_FOR_PAUSE = 1,
        PATH_MANAGER_STATE_PAUSED = 2,
        PATH_MANAGER_STATE_WAITING_FOR_RESUME = 3,
    };

    PathManagerImpl();
    ~PathManagerImpl();

    // Draws or hides the path node aspNode.
    bool DrawPath(NiPointer<NiNode>& aspNode, bool abDraw);
    // The ray cast path to the navmesh for apRequest; 2 when the request is unknown.
    RAYCASTPATH_RESULT GetRayCastPathToNavmesh(
        PathingRequest* apRequest,
        const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
        PathingLocation& aGoalLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath,
        bool abStart
    );
    // The ray cast path between two locations for apRequest; 2 when the request is unknown.
    RAYCASTPATH_RESULT GetRayCastPath(
        PathingRequest* apRequest,
        PathingLocation& aStartLocation,
        PathingLocation& aGoalLocation,
        BSSimpleArray<PathingLocation, 1024>& aRetPath
    );
    // Drops the current requests' pointers into apInventory.
    void ClearInventoryPointersFromCurrentRequests(InventoryChanges* apInventory);
    // Hands aspRequest to the pathing tasklets, answering through aspMessageQueue. Both smart pointers come by value
    // and are released when the function returns.
    bool BuildPath(NiPointer<PathingRequest> aspRequest, NiPointer<ActorPathingMessageQueue> aspMessageQueue);
    void Update();
    // Asks the tasklets to pause: an idle manager waits for the pause, one waiting to resume stays paused.
    void Pause();
    bool QPaused();
    bool QIdle();
    // Undoes Pause: a manager waiting for the pause goes idle again, a paused one waits to resume.
    void Resume();
    // Closes the tasklet group and hands it back to the tasklet manager.
    void Release();
    void CancelCurrentPathsAndPause();
    bool Initialize();
    bool QCancelPaths() const;
    void TaskUnlock();
    // Takes SpinLock for reading without waiting; false when it is held for writing.
    bool TaskTryLock();
    void SendMessageTo(const PathingMessage& aMsg);

    // The deleting destructor frees with ourFree.
    static void operator delete(void* apMemory);

    BSTStaticFreeList<PathingMessage, 256> MessagePool;
    // SendMessageTo pushes into it.
    BSTCommonLLMessageQueue<PathingMessage> MessageQueue;
    PATH_MANAGER_STATE eState;
    uint32_t iPauseRetryCount;
    BSReadWriteLock SpinLock;
    bool bCancelPaths;
    BSCriticalSection UpdateCriticalSection;
    BSTCommonStaticMessageQueue<NiPointer<NiAVObject>, 256> PathsToDraw;
    BSTCommonStaticMessageQueue<NiPointer<NiAVObject>, 256> PathsToRemove;
    uint8_t padding1478[8];
    // The destructor destroys this map first.
    LockFreeMap<PathingRequest*, PathingTaskData*> RequestToTaskletDataMap;
};

extern SettingT<INISettingCollection> bBackgroundPathing;
