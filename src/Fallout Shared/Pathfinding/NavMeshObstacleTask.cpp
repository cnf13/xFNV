#include "Fallout Shared/Pathfinding/NavMeshObstacleTask.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
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
        MEM_CONTEXT_NAVMESH, true,
        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Pathfinding\\NavMeshObstacleTask.cpp", 357
    );
    NavMeshObstacleManager* Instance = NavMeshObstacleManager::GetInstance();
    BSPerformanceTimer* PerformanceTimer = Instance->GetPerformanceTimer();
    if (pInfo->GetNavMesh(spSrcMesh)) {
        NavMesh* v11;
        v11 = new NavMesh;
        spNewNavMesh = NavMeshPtr::MakeNavMeshPtr(v11);
        spSrcMesh.m_pObject->MakeCopyForObstacle(spNewNavMesh);
        SortOperations();
        for (int32_t a2 = 0;; ++a2) {
            uint32_t FormFlags = Operations.iSize;
            if (a2 >= FormFlags) {
                break;
            }
            switch ((&Operations.GetAt(a2))->eType) {
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
                    if (!spNewNavMesh.m_pObject->AddObstacle((&Operations.GetAt(a2))->spObstacle, PortalSwaps)) {
                        goto LABEL_13;
                    }
                    goto LABEL_17;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
                    if (!spNewNavMesh.m_pObject->RemoveObstacle((&Operations.GetAt(a2))->spObstacle, PortalSwaps)) {
                        goto LABEL_13;
                    }
                    goto LABEL_17;
                }
                case OBSTACLE_TASK_TYPE_ADD_DOOR: {
                    ObstacleData* pDoorData = (&Operations.GetAt(a2))->spObstacle.m_pObject;
                    // The door's ID is its parent array's form ID.
                    if (spNewNavMesh.m_pObject->AddClosedDoor(
                            (&Operations.GetAt(a2))->spObstacle.m_pObject->spParentArray->iFormID, pDoorData
                        )) {
                        goto LABEL_17;
                    }
                    goto LABEL_13;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
                    if (spNewNavMesh.m_pObject->RemoveClosedDoor(
                            (&Operations.GetAt(a2))->spObstacle.m_pObject->spParentArray->iFormID
                        )) {
                    LABEL_17:;
                        (&Operations.GetAt(a2))->eResult = OBSTACLE_TASK_RESULT_SUCCESS;
                    }
                    else {
                    LABEL_13:;
                        (&Operations.GetAt(a2))->eResult = OBSTACLE_TASK_RESULT_FAILLURE;
                    }
                    break;
                }
                default: {
                    continue;
                }
            }
        }
        for (uint32_t auiIndex = 0;; ++auiIndex) {
            uint32_t v4 = PortalSwaps.PotentialParents.iSize;
            if (auiIndex >= v4) {
                break;
            }
            (&PortalSwaps.PotentialParents.GetAt(static_cast<int32_t>(auiIndex)))->pNavMeshInfo = pInfo;
        }
        for (uint32_t i = 0;; ++i) {
            uint32_t v5 = PortalSwaps.PotentialChildren.iSize;
            if (i >= v5) {
                break;
            }
            (&PortalSwaps.PotentialChildren.GetAt(static_cast<int32_t>(i)))->pNavMeshInfo = pInfo;
        }
    }
    else {
        for (ObstacleTaskNavMeshOperation& rOperation : Operations) {
            rOperation.eResult = OBSTACLE_TASK_RESULT_FAILLURE;
        }
    }
    NavMeshObstacleManager* v6 = NavMeshObstacleManager::GetInstance();
    v6->AddProcessedTask(this);
    NavMeshObstacleManager* v7 = NavMeshObstacleManager::GetInstance();
    BSPerformanceTimer* v8 = v7->GetPerformanceTimer();
}

bool ObstacleTaskData::ReplaceNavMesh() {
    if (!spSrcMesh.operator->()) {
        return false;
    }
    if (!spNewNavMesh.operator->()) {
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
        // SWEEP-INLINE 0x42FA20
        pCell->GetNavMeshArray()->AddNavMesh(spNewNavMesh);
        // SWEEP-INLINE 0x42FA20
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
    bool result;
    switch (aeType) {
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
            for (uint32_t auiIndex = 0;; ++auiIndex) {
                if (auiIndex >= Operations.iSize) {
                    goto LABEL_35;
                }
                if ((&Operations.GetAt(static_cast<int32_t>(auiIndex)))->spObstacle == apObstacle &&
                    ((&Operations.GetAt(static_cast<int32_t>(auiIndex)))->eType & 48) == 16) {
                    break;
                }
            }
            result = false;
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
            char v11 = 1;
            for (int32_t a2 = 0; a2 < Operations.iSize; ++a2) {
                if ((&Operations.GetAt(a2))->spObstacle == apObstacle) {
                    if (((&Operations.GetAt(a2))->eType & 48) == 16) {
                        InitializationFlow(
                            "Removing previous operation ADD for Task on Navmesh 0x%08x (srcptr = 0x%08x)",
                            pInfo->NavMeshID, spSrcMesh.m_pObject
                        );
                        Operations.Remove(a2--, true);
                    }
                    else {
                        v11 = 0;
                    }
                }
            }
            if (v11) {
                goto LABEL_35;
            }
            result = false;
            break;
        }
        case OBSTACLE_TASK_TYPE_ADD_DOOR: {
            for (uint32_t i = 0;; ++i) {
                if (i >= Operations.iSize) {
                    goto LABEL_35;
                }
                if ((&Operations.GetAt(static_cast<int32_t>(i)))->spObstacle == apObstacle &&
                    (&Operations.GetAt(static_cast<int32_t>(i)))->eType == OBSTACLE_TASK_TYPE_ADD_DOOR) {
                    break;
                }
            }
            result = false;
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
            for (const ObstacleTaskNavMeshOperation& rOperation : Operations) {
                if (rOperation.spObstacle == apObstacle && rOperation.eType == OBSTACLE_TASK_TYPE_REMOVE_DOOR) {
                    return false;
                }
            }
        LABEL_35:;
            NavMeshPtr spmesh;
            if (pInfo->GetNavMesh(spmesh)) {
                ObstacleTaskNavMeshOperation apMemory(&apObstacle, aeType, &spmesh);
                ObstacleTaskNavMeshOperation* aItem = &apMemory;
                Operations.Add(*aItem);
                result = true;
            }
            else {
                result = false;
            }
            break;
        }
        default: {
            result = false;
            break;
        }
    }
    return result;
}

void ObstacleTaskData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

int32_t ObstacleTaskData::SortOperationsFunction(const void* apOp1, const void* apOp2) {
    int32_t result;
    const ObstacleTaskNavMeshOperation* pOp1 = static_cast<const ObstacleTaskNavMeshOperation*>(apOp1);
    const ObstacleTaskNavMeshOperation* pOp2 = static_cast<const ObstacleTaskNavMeshOperation*>(apOp2);
    switch (pOp1->eType) {
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
        case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
            switch (pOp2->eType) {
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
                    // Two adds sort by their obstacles' reference form IDs.
                    result = static_cast<int32_t>(
                        pOp1->spObstacle.m_pObject->spParentArray->iFormID -
                        pOp2->spObstacle.m_pObject->spParentArray->iFormID
                    );
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
                    result = 1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_ADD_DOOR: {
                    result = -1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
                    result = 1;
                    break;
                }
                default: {
                    goto LABEL_18;
                }
            }
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
        case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
            switch (pOp2->eType) {
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE:
                case OBSTACLE_TASK_TYPE_ADD_OBSTACLE_FOR_UPDATE: {
                    result = -1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE:
                case OBSTACLE_TASK_TYPE_REMOVE_OBSTACLE_FOR_UPDATE: {
                    NavMeshPtr spnavMesh(pOp1->spNavMesh);
                    if (spnavMesh.operator NavMesh*()) {
                        uint32_t iindex1 = spnavMesh.m_pObject->GetUndoDataIndexForObstacle(pOp1->spObstacle);
                        int32_t v4 = spnavMesh.m_pObject->GetUndoDataIndexForObstacle(pOp2->spObstacle) - iindex1;
                        result = v4;
                    }
                    else {
                        result = 0;
                    }
                    break;
                }
                case OBSTACLE_TASK_TYPE_ADD_DOOR: {
                    result = -1;
                    break;
                }
                case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
                    result = 1;
                    break;
                }
                default: {
                    goto LABEL_18;
                }
            }
            break;
        }
        case OBSTACLE_TASK_TYPE_ADD_DOOR: {
            result = pOp2->eType != OBSTACLE_TASK_TYPE_ADD_DOOR;
            break;
        }
        case OBSTACLE_TASK_TYPE_REMOVE_DOOR: {
            if (pOp2->eType == OBSTACLE_TASK_TYPE_REMOVE_DOOR) {
                result = 0;
            }
            else {
                result = -1;
            }
            break;
        }
        default: {
        LABEL_18:;
            result = 0;
            break;
        }
    }
    return result;
}

BSTaskletData::BSTaskletData() : bYielding(false) {}

BSTaskletData::~BSTaskletData() {}

// The default task always runs.
bool BSTaskletData::OnStartup() {
    return true;
}

void BSTaskletData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// The tasklets' timer (+0x1A1).
BSPerformanceTimer* NavMeshObstacleManager::GetPerformanceTimer() {
    return &TaskletsPerformaceTimer;
}

ObstacleTaskNavMeshOperation::~ObstacleTaskNavMeshOperation() {}

void ObstacleTaskNavMeshOperation::operator delete(void* apMemory) {
    ourFree(apMemory);
}
