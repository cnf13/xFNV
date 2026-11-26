#include "Fallout Shared/Pathfinding/NavMeshObstacleTask.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSSystem/BSTaskletData.h"
#include "BSSystem/Win32/BSWin32TaskletData.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshArray.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/ObstacleTaskNavMeshOperation.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

TaskPortalSwap::TaskPortalSwap() {}

ObstacleTaskData::ObstacleTaskData(NavMeshInfo* apNavMesh) : pInfo(apNavMesh), bPortalModified(false) {}

ObstacleTaskData::~ObstacleTaskData() {}

void ObstacleTaskData::Process() {
    AutoMemContext tempMemContext(
        MEM_CONTEXT_NAVMESH,
        true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMeshObstacleTask.cpp",
        357
    );
    NavMeshObstacleManager* pManager = NavMeshObstacleManager::GetInstance();
    if (pInfo->GetNavMesh(spSrcMesh)) {
        NavMesh* pNavMesh = new NavMesh;
        spNewNavMesh = NavMeshPtr::MakeNavMeshPtr(pNavMesh);
        spSrcMesh->MakeCopyForObstacle(spNewNavMesh);
        SortOperations();
        for (int32_t i = 0; i < Operations.iSize; ++i) {
            bool bSucceeded;
            switch (Operations.GetAt(i).eType) {
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
                    bSucceeded = spNewNavMesh->AddObstacle(Operations.GetAt(i).spObstacle, PortalSwaps);
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
                    bSucceeded = spNewNavMesh->RemoveObstacle(Operations.GetAt(i).spObstacle, PortalSwaps);
                    break;
                }
                case OBSTACLE_TASK_TYPE_ADD_DOOR: {
                    ObstacleData* pDoorData = Operations.GetAt(i).spObstacle.m_pObject;
                    // The door's ID is its parent array's form ID.
                    bSucceeded = spNewNavMesh->AddClosedDoor(
                        Operations.GetAt(i).spObstacle->spParentArray->iFormID,
                        pDoorData
                    );
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
                    bSucceeded = spNewNavMesh->RemoveClosedDoor(Operations.GetAt(i).spObstacle->spParentArray->iFormID);
                    break;
                }
                default: {
                    continue;
                }
            }
            if (bSucceeded) {
                Operations.GetAt(i).eResult = OBSTACLE_TASK_RESULT_SUCCESS;
            }
            else {
                Operations.GetAt(i).eResult = OBSTACLE_TASK_RESULT_FAILLURE;
            }
        }
        for (uint32_t i = 0; i < PortalSwaps.PotentialParents.iSize; ++i) {
            PortalSwaps.PotentialParents.GetAt(static_cast<int32_t>(i)).pNavMeshInfo = pInfo;
        }
        for (uint32_t i = 0; i < PortalSwaps.PotentialChildren.iSize; ++i) {
            PortalSwaps.PotentialChildren.GetAt(static_cast<int32_t>(i)).pNavMeshInfo = pInfo;
        }
    }
    else {
        for (ObstacleTaskNavMeshOperation& rOperation : Operations) {
            rOperation.eResult = OBSTACLE_TASK_RESULT_FAILLURE;
        }
    }
    pManager = NavMeshObstacleManager::GetInstance();
    pManager->AddProcessedTask(this);
    NavMeshObstacleManager::GetInstance();
}

bool ObstacleTaskData::ReplaceNavMesh() {
    if (!spSrcMesh) {
        return false;
    }
    if (!spNewNavMesh) {
        return false;
    }
    bool bReplace = false;
    if (bPortalModified) {
        bReplace = true;
    }
    else {
        for (const ObstacleTaskNavMeshOperation& rOperation : Operations) {
            if (rOperation.eResult == 0) {
                bReplace = true;
                break;
            }
        }
    }
    if (!bReplace) {
        return false;
    }
    uint32_t uiFormID = spSrcMesh->GetFormID();
    TESObjectCELL* pCell = spSrcMesh->GetParentCell();
    if (pCell) {
        pCell->GetNavMeshArray()->AddNavMesh(spNewNavMesh);
        pCell->GetNavMeshArray()->RemoveNavMesh(spSrcMesh);
    }
    spSrcMesh->ForceSetFormID(0);
    spSrcMesh->SetNavMeshInfo(nullptr);
    spNewNavMesh->ForceSetFormID(uiFormID);
    spNewNavMesh->SetNavMeshInfo(pInfo);
    pInfo->SetNavMesh(static_cast<NavMesh*>(spNewNavMesh));
    return true;
}

void ObstacleTaskData::SortOperations() {
    Operations.Sort(&ObstacleTaskData::SortOperationsFunction);
}

bool ObstacleTaskData::AddOperation(const NiPointer<ObstacleData>& apObstacle, OBSTACLE_TASK_TYPE aeType) {
    switch (aeType) {
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
            for (uint32_t i = 0; i < Operations.iSize; ++i) {
                if (Operations.GetAt(static_cast<int32_t>(i)).spObstacle == apObstacle &&
                    (Operations.GetAt(static_cast<int32_t>(i)).eType & 48) == 16) {
                    return false;
                }
            }
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
            char cAllowed = 1;
            for (int32_t i = 0; i < Operations.iSize; ++i) {
                if (Operations.GetAt(i).spObstacle == apObstacle) {
                    if ((Operations.GetAt(i).eType & 48) == 16) {
                        InitializationFlow(
                            "Removing previous operation ADD for Task on Navmesh 0x%08x (srcptr = 0x%08x)",
                            pInfo->NavMeshID,
                            spSrcMesh.m_pObject
                        );
                        Operations.Remove(i--, true);
                    }
                    else {
                        cAllowed = 0;
                    }
                }
            }
            if (!cAllowed) {
                return false;
            }
            break;
        }
        case OBSTACLE_TASK_TYPE_ADD_DOOR: {
            for (uint32_t i = 0; i < Operations.iSize; ++i) {
                if (Operations.GetAt(static_cast<int32_t>(i)).spObstacle == apObstacle &&
                    Operations.GetAt(static_cast<int32_t>(i)).eType == OBSTACLE_TASK_TYPE_ADD_DOOR) {
                    return false;
                }
            }
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
            for (const ObstacleTaskNavMeshOperation& rOperation : Operations) {
                if (rOperation.spObstacle == apObstacle && rOperation.eType == OBSTACLE_TASK_TYPE_REMOVE_DOOR) {
                    return false;
                }
            }
            break;
        }
        default: {
            return false;
        }
    }
    bool bResult;
    NavMeshPtr spmesh;
    if (pInfo->GetNavMesh(spmesh)) {
        ObstacleTaskNavMeshOperation kOperation(&apObstacle, aeType, &spmesh);
        Operations.Add(kOperation);
        bResult = true;
    }
    else {
        bResult = false;
    }
    return bResult;
}

void ObstacleTaskData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t ObstacleTaskData::SortOperationsFunction(const void* apOp1, const void* apOp2) {
    int32_t iResult;
    const ObstacleTaskNavMeshOperation* pOp1 = static_cast<const ObstacleTaskNavMeshOperation*>(apOp1);
    const ObstacleTaskNavMeshOperation* pOp2 = static_cast<const ObstacleTaskNavMeshOperation*>(apOp2);
    switch (pOp1->eType) {
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
            switch (pOp2->eType) {
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
                    // Two adds sort by their obstacles' reference form IDs.
                    iResult = static_cast<int32_t>(
                        pOp1->spObstacle->spParentArray->iFormID - pOp2->spObstacle->spParentArray->iFormID
                    );
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
                    iResult = 1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_ADD_DOOR: {
                    iResult = -1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
                    iResult = 1;
                    break;
                }
                default: {
                    iResult = 0;
                    break;
                }
            }
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
            switch (pOp2->eType) {
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
                    iResult = -1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
                    NavMeshPtr spnavMesh(pOp1->spNavMesh);
                    if (spnavMesh) {
                        uint32_t iindex1 = spnavMesh->GetUndoDataIndexForObstacle(pOp1->spObstacle);
                        iResult = spnavMesh->GetUndoDataIndexForObstacle(pOp2->spObstacle) - iindex1;
                    }
                    else {
                        iResult = 0;
                    }
                    break;
                }
                case OBSTACLE_TASK_TYPE_ADD_DOOR: {
                    iResult = -1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
                    iResult = 1;
                    break;
                }
                default: {
                    iResult = 0;
                    break;
                }
            }
            break;
        }
        case OBSTACLE_TASK_TYPE_ADD_DOOR: {
            iResult = pOp2->eType != OBSTACLE_TASK_TYPE_ADD_DOOR;
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
            iResult = pOp2->eType == OBSTACLE_TASK_TYPE_REMOVE_DOOR ? 0 : -1;
            break;
        }
        default: {
            iResult = 0;
            break;
        }
    }
    return iResult;
}

// The tasklets' timer (+0x1A1).
BSPerformanceTimer* NavMeshObstacleManager::GetPerformanceTimer() {
    return &TaskletsPerformaceTimer;
}

ObstacleTaskNavMeshOperation::~ObstacleTaskNavMeshOperation() {}

void ObstacleTaskNavMeshOperation::operator delete(void* apMemory) {
    ourFree(apMemory);
}
