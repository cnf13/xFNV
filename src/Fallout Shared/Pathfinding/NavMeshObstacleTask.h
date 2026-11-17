#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NavMeshInfo;

// The edges a task's new navmesh still has to reconnect: the edges cut out of the new navmesh, and the source navmesh's
// edges that led to other navmeshes.
struct TaskPortalSwap {
    TaskPortalSwap();
    ~TaskPortalSwap();

    BSSimpleArray<NavMeshEdgeHandle> PotentialChildren;
    BSSimpleArray<NavMeshEdgeHandle> PotentialParents;
};

// The background work that cuts the obstacles into one navmesh. Only some of its members are declared.
class ObstacleTaskData : public BSWin32TaskletData {
public:
    ObstacleTaskData(NavMeshInfo* apNavMesh);
    ~ObstacleTaskData() override;

    void Process() override;

    // Swaps the processed navmesh in for the source one when a portal changed or an operation has no result yet:
    // the cell's navmesh array, the form ID and the navmesh info move to the new navmesh; true when it did.
    bool ReplaceNavMesh();
    void SortOperations();
    bool AddOperation(const NiPointer<ObstacleData>& apObstacle, OBSTACLE_TASK_TYPE aeType);

    static void operator delete(void* apMemory);
    static int32_t SortOperationsFunction(const void* apOp1, const void* apOp2);

    NavMeshInfo* pInfo;
    // The navmesh the task cuts, and the copy it builds.
    NavMeshPtr spSrcMesh;
    NavMeshPtr spNewNavMesh;
    BSSimpleArray<ObstacleTaskNavMeshOperation> Operations;
    TaskPortalSwap PortalSwaps;
    bool bPortalModified;
};
