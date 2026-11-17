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
#include "Reconstruction.hpp"

#include <stdint.h>

class ActorPathingMessageQueue;
class InventoryChanges;
class NavMeshEdgeLocation;
class PathManagerImpl;
class PathingLocation;
class PathingRequest;
class PathingSolution;
class PathingTaskData;
struct RayCastPath;
struct RayCastPathRequest;

// The result of a ray cast path request. Values from the Xbox PDB.
enum RAYCASTPATH_RESULT : int32_t {
	RCP_NONE = 0,
	RCP_SUCCESS = 1,
	RCP_FAILED = 2,
};

// The pathfinding front end: hands path requests to its implementation and the pathing thread. Only the members the
// reconstructed functions use are declared.
class PathManager {
public:
	PathManager();
	~PathManager();

	// pImpl.
	PathManagerImpl* QImpl() { return pImpl; }
	// Closes the implementation, deletes it and clears pImpl, so the singleton's destructor at exit finds nothing to
	// delete (thiscall, 0x6EB980; WinMain's shutdown).
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
	// Queues aspRequest for the pathing thread, answering through aspMessageQueue (thiscall, ret 0xC, 0x6EB9D0).
	// Both smart pointers come by value: the caller copies them into its argument slots and the exe releases them
	// where it returns (0x45CEC0 on [ebp+8], 0x6EBF90 on [ebp+0xC]), as C++ destroys the parameters.
	bool BuildPath(NiPointer<PathingRequest> aspRequest, NiPointer<ActorPathingMessageQueue> aspMessageQueue,
		bool abQueue);
	RAYCASTPATH_RESULT GetRayCastPath(PathingRequest* apRequest, PathingLocation& aStartLocation,
		PathingLocation& aGoalLocation, BSSimpleArray<PathingLocation, 1024>& aRetPath);
	// Inferred type: return: RAYCASTPATH_RESULT: returned from PathBuilder::BuildPathToValidNavMeshTriangle at
	// 0x6cb137; aEdges: BSSimpleArray<NavMeshEdgeLocation, 1024>* from 0x6cb137 (parameter 3 of
	// PathBuilder::BuildPathToValidNavMeshTriangle); aGoalLocation: PathingLocation* from 0x6cb137 (parameter 0 of
	// PathBuilder::BuildPathToValidNavMeshTriangle); aRetPath: BSSimpleArray<PathingLocation, 1024>* from 0x6cb137
	// (parameter 1 of PathBuilder::BuildPathToValidNavMeshTriangle)
	RAYCASTPATH_RESULT GetRayCastPathToNavmesh(PathingRequest* apRequest,
		const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges, PathingLocation* aGoalLocation,
		BSSimpleArray<PathingLocation, 1024>& aRetPath, bool a6);
	// Draws or hides the path node aspNode: passes both on to the implementation's DrawPath
	// (0x6EB200) (thiscall, ret 8, 0x6EBBD0; the PDB's NiPointer<NiNode>& overload).
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
	RAYCASTPATH_RESULT GetRayCastPath(PathingLocation& aStartLocation, PathingLocation& aGoalLocation,
		BSSimpleArray<PathingLocation, 1024>& aRetPath);
	RAYCASTPATH_RESULT GetRayCastPathToNavmesh(const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges,
		PathingLocation& aGoalLocation, BSSimpleArray<PathingLocation, 1024>& aRetPath, bool abStart);
	// Takes the request and the actor's message queue (thiscall, ret 8, 0x6E9DB0). The PDB
	// passes the queue as a pointer; the exe takes the smart pointer by value and releases it (0x6EBF90 on
	// [ebp+0xC]).
	void Setup(const NiPointer<PathingRequest>& aspRequest, NiPointer<ActorPathingMessageQueue> aspMessageQueue);

	// The exe's deleting destructor (0x6E9C90) frees with ourFree.
	static void operator delete(void* apMemory);

	NiPointer<PathingRequest> spRequest;
	NiPointer<PathingSolution> spSolution;
	// The PC lock occupies 0x20 bytes; the Xbox PDB lock occupies eight.
	BSSpinLock RayCastLock;
	BSSimpleArray<RayCastPathRequest*, 1024> RayCastRequests;
	BSSimpleArray<RayCastPath*, 1024> RayCastPaths;
	NiPointer<ActorPathingMessageQueue> spMessageQueue;
	bool bWaitingForRayCasts;
	uint8_t padding65[3];
	// The PC type supplies no matched PDB members for this trailing span.
	uint8_t padding68[0x18];
};

// A message from an actor to the pathing thread: its kind and the task the message is about.
class PathingMessage {
public:
	PathingMessage();
	PathingMessage(PATHING_MESSAGE_TYPE aType, PathingTaskData* apTaskData);

	// The PDB's operator=: copies msgType and pTaskData, returns this (thiscall, ret 4, 0x6EAA00). The exe's copy
	// constructor is the same function (BSTFreeListElem<PathingMessage>::ConstructElem 0x6ECE00 calls it after
	// placement new).
	PathingMessage& operator=(const PathingMessage& aMsg);

	// The exe's deleting destructor (0x7B3FA0) frees with ourFree.
	static void operator delete(void* apMemory);

	PATHING_MESSAGE_TYPE msgType;
	PathingTaskData* pTaskData;
};

// The PC uses BSTaskletGroup as a base where the PDB names a PathingTaskletGroup member.
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
	RAYCASTPATH_RESULT GetRayCastPathToNavmesh(PathingRequest* apRequest,
		const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEdges, PathingLocation& aGoalLocation,
		BSSimpleArray<PathingLocation, 1024>& aRetPath, bool abStart);
	// The ray cast path between two locations for apRequest; 2 when the request is unknown.
	RAYCASTPATH_RESULT GetRayCastPath(PathingRequest* apRequest, PathingLocation& aStartLocation,
		PathingLocation& aGoalLocation, BSSimpleArray<PathingLocation, 1024>& aRetPath);
	// Drops the current requests' pointers into apInventory.
	void ClearInventoryPointersFromCurrentRequests(InventoryChanges* apInventory);
	// Hands aspRequest to the pathing tasklets, answering through aspMessageQueue (thiscall, ret 8, 0x6EAC70). Both
	// smart pointers come by value; the exe releases them where it returns (0x6EADAF, 0x6EADBE), as C++ destroys the
	// parameters.
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

	// The exe's deleting destructor (0x6EB890) frees with ourFree.
	static void operator delete(void* apMemory);

	BSTStaticFreeList<PathingMessage, 256> MessagePool;
	// PDB name (IDA kTaskletsCompleteQueue); SendMessageTo 0x6EB7F0 pushes into it.
	BSTCommonLLMessageQueue<PathingMessage> MessageQueue;
	PATH_MANAGER_STATE eState;
	uint32_t iPauseRetryCount;
	BSReadWriteLock SpinLock;
	bool bCancelPaths;
	uint8_t paddingC35[3];
	// PC critical sections are 0x18 bytes rather than the Xbox PDB's 0x28 bytes.
	BSCriticalSection UpdateCriticalSection;
	BSTCommonStaticMessageQueue<NiPointer<NiAVObject>, 256> PathsToDraw;
	BSTCommonStaticMessageQueue<NiPointer<NiAVObject>, 256> PathsToRemove;
	// 8 bytes IDA leaves untyped.
	uint8_t padding1478[8];
	// PDB name (IDA kRequestToTaskletDataMap): the destructor 0x6EAB30 destroys it first. The Xbox map and its
	// following BuiltPaths/FailedPaths do not match the PC's.
	LockFreeMap<PathingRequest*, PathingTaskData*> RequestToTaskletDataMap;
};

inline constexpr fnvd::Vtable PathingTaskData_vftable{};

// The game settings PathManager.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bBackgroundPathing;
