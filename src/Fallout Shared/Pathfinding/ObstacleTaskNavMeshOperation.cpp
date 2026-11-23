#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleTask.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

ObstacleTaskNavMeshOperation::ObstacleTaskNavMeshOperation() {}

ObstacleTaskNavMeshOperation::ObstacleTaskNavMeshOperation(
    const NiPointer<ObstacleData>* apObstacle,
    OBSTACLE_TASK_TYPE aeType,
    NavMeshPtr* apNavMesh
)
  : spObstacle(*apObstacle), eType(aeType), spNavMesh(*apNavMesh) {}

ObstacleTaskNavMeshOperation& ObstacleTaskNavMeshOperation::operator=(const ObstacleTaskNavMeshOperation& aOperation) {
    spObstacle = aOperation.spObstacle;
    eType = aOperation.eType;
    eResult = aOperation.eResult;
    spNavMesh = aOperation.spNavMesh;
    return *this;
}
