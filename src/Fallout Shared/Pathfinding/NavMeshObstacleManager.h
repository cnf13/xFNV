#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSPerformanceTimer.h"
#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSTaskletGroup.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/ModelLoader.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityActivationListener.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityListener.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct NavMeshEdgeHandle;
class NavMeshInfo;
class NavMeshPtr;
struct NavMeshTriHandle;
class NiTriShape;
class ObstacleData;
class ObstacleTaskData;
class ReferenceObstacleArray;
class TESObjectCELL;

// Tells the obstacle manager when an obstacle's rigid body goes to sleep or wakes up.
class bhkObstacleDeactivationListener : public hkpEntityActivationListener {
public:
	bhkObstacleDeactivationListener();
	~bhkObstacleDeactivationListener() override;

	void entityDeactivatedCallback(hkpEntity* entity) override;
	void entityActivatedCallback(hkpEntity* entity) override;

	static void operator delete(void* apMemory);
};

// Tells the obstacle manager when an obstacle's rigid body leaves the world.
class bhkObstacleRemovalListener : public hkpEntityListener {
public:
	bhkObstacleRemovalListener();
	~bhkObstacleRemovalListener() override;

	void entityRemovedCallback(hkpEntity* entity) override;

	static void operator delete(void* apMemory);
};

// The obstacles of one reference: one per collidable rigid body in its 3D.
class ReferenceObstacleArray : public NiRefObject {
public:
	ReferenceObstacleArray();
	~ReferenceObstacleArray() override;

	uint32_t iFormID;
	BSSimpleArray<NiPointer<ObstacleData>> Obstacles;
};

NiSmartPointer(ReferenceObstacleArray);

// One rigid body that cuts the navmesh: its oriented box (Center, Orientation, BoxMin/BoxMax in the box's frame) and
// its world-space bounds.
class ObstacleData : public NiRefObject {
public:
	ObstacleData();
	~ObstacleData() override;

	ReferenceObstacleArray* spParentArray;
	NiPointer<bhkRigidBody> pRigidBody;
	NiPoint3 Center;
	NiMatrix3 Orientation;
	NiPoint3 BoxMin;
	NiPoint3 BoxMax;
	NiPoint3 aabbMin;
	NiPoint3 aabbMax;
	uint32_t iLastUpdateTime;
	bool bActive;
	BSSimpleArray<NavMeshInfo*> Navmeshes;
	NiPointer<NiTriShape> sp3DNode;
};

NiSmartPointer(ObstacleData);

// Keeps the navmesh obstacles cut by references and doors up to date. Only the members the reconstructed functions
// use are declared; the padding holds the PDB's other members.
class NavMeshObstacleManager {
public:
	using ReferenceObstacleArrayMap = NiTMap<uint32_t, NiPointer<ReferenceObstacleArray>>;

	enum OBSTACLE_MANAGER_BACKGROUND_STATE : int32_t {
		OBSTACLE_MANAGER_BACKGROUND_STATE_PROCESSING_OBSTACLES = 0,
		OBSTACLE_MANAGER_BACKGROUND_STATE_WAITING_FOR_PATH_MANAGER_PAUSE = 1,
	};

	NavMeshObstacleManager();
	~NavMeshObstacleManager();

	// Removes apRigidBody from the list of obstacles to update.
	void RemoveObstacleFromUpdateList(bhkRigidBody* apRigidBody);
	// Adds apRigidBody to the list of obstacles to update.
	void AddObstacleToUpdateList(bhkRigidBody* apRigidBody);
	// IDA name; not in the PDB. Removes the obstacles of the reference that owns apRigidBody's obstacle (thiscall,
	// ret 4, 0x6C1CB0; the removal listener passes the rigid body).
	void OnObstacleLost3D(bhkRigidBody* apRigidBody);
	// The per-frame update.
	void Update();
	// Creates the tasklet group and the rigid-body listeners; false when the group cannot be created or opened. IDA
	// calls it CreateListeners; it is the PDB's Initialize (WinMain calls it once).
	bool Initialize();
	// Add the obstacle of apRef.
	void AddObstacleForReference(TESObjectREFR* apRef);
	void RemoveObstacleForReference(TESObjectREFR* apRef);
	void OnDoorClose(TESObjectREFR* apDoorRef);
	// Starts following the opened door apDoorRef.
	void OnDoorOpen(TESObjectREFR* apDoorRef);
	// Forgets the door apDoorRef.
	void OnDoorRemove(TESObjectREFR* apDoorRef);
	void QueueUpdateAllObstacle();
	void ForceUpdateAllObstacles();
	BSPerformanceTimer* GetPerformanceTimer();
	void AddActivationListener(bhkRigidBody* apRigidBody);
	void EnableNavMesh(uint32_t aiNavMeshID);
	void DisableNavMesh(uint32_t aiNavMeshID);
	void AddBoundingBox3DNode(const NiPointer<ObstacleData>& aBox);

protected:
	// Stops listening to apRigidBody's activation.
	void RemoveActivationListener(bhkRigidBody* apRigidBody);
	uint32_t GetProcessedTaskCount();
	uint32_t GetBackgroundTaskCount();
	void QueueTasks();
	bool SwapNavmeshes();
	bool DeleteCurrentTasks();
	// IDA calls it AddTask.
	ObstacleTaskData* GetTaskForNavMesh(NavMeshInfo* apNavMesh);
	bool PushTask(ObstacleTaskData* apTaskData);
	void ProcessNewReferences();
	void ProcessRemovedReferences();
	// Enables and clears the queued navmesh IDs.
	bool ProcessEnabledNavMeshes();
	// Disables and clears the queued navmesh IDs.
	bool ProcessDisabledNavMeshes();
	void ProcessAllObstacles();
	// Defined in NavMeshObstacleManager.cpp, unregistered (CHK-STUBOUT there); tied here, where the protected member
	// is accessible.
	void ProcessMovingReferences();
	// Re-cuts spObstacle into its navmeshes.
	void UpdateObstacle(NiPointer<ObstacleData>& spObstacle);
	// Rebuilds the obstacles of the known reference apRef.
	void UpdateObstaclesForReference(uint32_t aiRefID, TESObjectREFR* apRef);
	// Builds and cuts the obstacles of apRef.
	void ReallyAddObstaclesForReference(TESObjectREFR* apRef);
	// Drops the obstacles of the reference aiFormID.
	void ReallyRemoveObstaclesForReference(uint32_t aiFormID);
	// Defined in NavMeshObstacleManager.cpp, unregistered (CHK-STUBOUT there); tied here, where the protected member
	// is accessible.
	bool ReconnectPortals();
	// Joins edge aEdge1 of aspNavMesh1 and edge aEdge2 of aspNavMesh2 with portals both ways;
	// always true.
	bool CreatePortal(NavMeshPtr& aspNavMesh1, const NavMeshEdgeHandle& aEdge1, NavMeshPtr& aspNavMesh2,
		const NavMeshEdgeHandle& aEdge2);
	// Points edge aEdge1 of aspNavMesh1 at triangle aTri2; always true.
	bool CreateOneWayPortal(NavMeshPtr& aspNavMesh1, const NavMeshEdgeHandle& aEdge1, const NavMeshTriHandle& aTri2);
	// Drops the obstacles of the doors queued for removal.
	bool ProcessRemovedDoors();
	// Builds the obstacles of the doors queued for adding.
	bool ProcessNewDoors();

public:
	// Records a task in the processed-task array under the task lock.
	void AddProcessedTask(ObstacleTaskData* apTaskData);
	void Release();
	void ClearAllObstacles();
	void ProcessActivatedObstacles();
	void ProcessDeactivatedObstacles();
	void DrawObstacles(bool abDraw);
	void CreateBoundingBox3DNode(const NiPointer<ObstacleData>& aBox, NiPointer<NiTriShape>& aspTriShape);
	void RemoveBoundingBox3DNode(const NiPointer<ObstacleData>& aBox);

	// The singleton, created on first use.
	static NavMeshObstacleManager* GetInstance();
	static void UpdateObstacleActiveStatus(NiPointer<ObstacleData>& aspObstacle);
	static bool GetNavMeshListForObstacle(const NiPointer<ObstacleData>& apData,
		BSSimpleArray<NavMeshPtr, 1024>* apReturnNavMeshArray);

protected:
	// Fills apRetObstacle from the first collidable obstacle body under ap3D; false when
	// there is none.
	static bool FindFirstObstacleFor3D(NiAVObject* ap3D, ObstacleData* apRetObstacle);
	static bool AddObstaclesFor3DRecursive(NiAVObject* ap3D, NiPointer<ReferenceObstacleArray>& apRetObstacleArray);
	static bool UpdateObstacleData(ObstacleData* apObstacle);
	static bool GetObstacleDataForReference(TESObjectREFR* apRef,
		NiPointer<ReferenceObstacleArray>& apRetObstacleArray);
	static bool UpdateObstacleDataForReference(TESObjectREFR* apRef,
		NiPointer<ReferenceObstacleArray>& apRetObstacleArray);

public:
	static bool GetCellListForObstacle(const NiPointer<ObstacleData>& apData,
		BSSimpleArray<TESObjectCELL*, 1024>* apReturnCellArray);
	static bool IsPointInBox(const NiPoint3& aPoint, const NiPointer<ObstacleData>& aBox, float afMaxHeightAbove,
		float afMaxHeightBelow);

	BSCriticalSection UpdateCriticalSection;
	bool bUpdateAllObstacles;
	// The obstacles by reference form ID.
	ReferenceObstacleArrayMap FormIDMap;
	BSSimpleList<TESObjectREFR*> QueuedListOfRefsToAdd;
	// Form IDs.
	BSSimpleList<uint32_t> QueuedListOfRefsToRemove;
	// Obstacles to re-cut while they move.
	BSSimpleList<NiPointer<ObstacleData>> ObstaclesUpdateList;
	// IDA: UInt32 gap44[7].
	uint8_t padding44[0x1C];
	// Rigid bodies queued to join or leave ObstaclesUpdateList, from any thread.
	LockFreeQueue<bhkRigidBody*> ObstaclesToAddToUpdateList;
	LockFreeQueue<bhkRigidBody*> ObstaclesToRemoveFromUpdateList;
	// The obstacle of each rigid body.
	NiTMap<bhkRigidBody*, NiPointer<ObstacleData>> RigidBodyToObstacleMap;
	BSSimpleList<TESObjectREFR*> QueuedListOfDoorsToAdd;
	// Form IDs.
	BSSimpleList<uint32_t> QueuedListOfDoorsToRemove;
	// The open and closed doors' obstacles by form ID.
	ReferenceObstacleArrayMap OpenDoorMap;
	ReferenceObstacleArrayMap ClosedDoorMap;
	bhkObstacleDeactivationListener* pListener;
	bhkObstacleRemovalListener* pRemoveListener;
	BSTaskletGroup ObstacleTaskletGroup;
	// The pending task of each navmesh, by navmesh ID.
	NiTMap<uint32_t, ObstacleTaskData*> CurrentNavMeshTaskMap;
	BSSimpleArray<ObstacleTaskData*> BackgroundTasks;
	BSSimpleArray<ObstacleTaskData*> ProcessedTasks;
	uint8_t padding15C[0x4];
	BSSpinLock ProcessedTaskLock;
	// QueuedListOfNavmeshesToEnable and QueuedListOfNavmeshesToDisable.
	BSSimpleList<uint32_t> QueuedListOfNavmeshesToEnable;
	BSSimpleList<uint32_t> QueuedListOfNavmeshesToDisable;
	OBSTACLE_MANAGER_BACKGROUND_STATE eState;
	// Seconds until the processed navmeshes are swapped in.
	float fTimeToNextSwap;
	// 0x198 whether the obstacles' boxes are built and drawn (set by DrawObstacles)
	bool bDrawObstacles;
	uint8_t padding199[3];
	// 0x19C the node the obstacles' boxes are attached to (IDA: spNavmeshDebugRoot)
	NiPointer<NiNode> spObstacleRootNode;
	BSPerformanceTimer MainThreadPerformaceTimer;
	BSPerformanceTimer TaskletsPerformaceTimer;
};

inline constexpr fnvd::Vtable bhkObstacleDeactivationListener_vftable{};
inline constexpr fnvd::Vtable bhkObstacleRemovalListener_vftable{};
inline constexpr fnvd::Vtable ReferenceObstacleArray_vftable{};
inline constexpr fnvd::Vtable ObstacleData_vftable{};

// The game settings NavMeshObstacleManager.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bCutDoors;
extern SettingT<INISettingCollection> fObstacleUpdateDeltaWhenUnknown;
extern SettingT<INISettingCollection> bBackgroundNavmeshUpdate;
extern SettingT<INISettingCollection> fObstacleUpdateDeltaWhenMoving;
