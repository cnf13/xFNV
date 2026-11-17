#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

// The operation ObstacleTaskData::AddOperation queues: its switch dispatches on these values, and it tells an add from
// a remove by masking an operation's eType with 0x30 (an add is 0x10, a remove 0x20).
enum OBSTACLE_TASK_TYPE : int32_t {
	OBSTACLE_TASK_TYPE_ADD_OBSTACLE = 0x10,
	OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE = 0x11,
	OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE = 0x20,
	OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE = 0x21,
	OBSTACLE_TASK_TYPE_ADD_DOOR = 0x30,
	OBSTACLE_TASK_TYPE_REMOVE_DOOR = 0x40,
};

// How an obstacle task step ended.
enum OBSTACLE_TASK_RESULT : int32_t {
	OBSTACLE_TASK_RESULT_SUCCESS = 0,
	OBSTACLE_TASK_RESULT_FAILLURE = 1,
};

// One step of an obstacle task: an obstacle's object, what to do with it and the navmesh it applies to.
class ObstacleTaskNavMeshOperation {
public:
	ObstacleTaskNavMeshOperation();
	ObstacleTaskNavMeshOperation(const NiPointer<ObstacleData>* a1, OBSTACLE_TASK_TYPE a2, NavMeshPtr* a3);
	~ObstacleTaskNavMeshOperation();

	ObstacleTaskNavMeshOperation& operator=(const ObstacleTaskNavMeshOperation& aOperation);

	// Frees with ourFree.
	static void operator delete(void* apMemory);

	NiPointer<ObstacleData> spObstacle;
	OBSTACLE_TASK_TYPE eType;
	OBSTACLE_TASK_RESULT eResult;
	NavMeshPtr spNavMesh;
};
