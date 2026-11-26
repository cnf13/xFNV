#include "Fallout Shared/ExteriorCellLoader.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSMain/ThreadSafeStructures.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/BGSAutoWater.h"
#include "Fallout Shared/CELL_LOAD_DATA.h"
#include "Fallout Shared/ExteriorCellLoaderTask.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESLand.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// The game settings this file defines, in their initialization order.
SettingT<INISettingCollection> bBackgroundCellLoads("bBackgroundCellLoads:BackgroundLoad", true);

// The map of the exterior cell loader's pending tasks.
using ExteriorCellMap = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>;

using ExteriorCellMapIterator = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;

ExteriorCellLoader::ExteriorCellLoader() : QueuedCellMap(2u, 37u, 12u) {}

ExteriorCellLoader::~ExteriorCellLoader() {
    ClearData();
}

void ExteriorCellLoader::LoadCellData(CELL_LOAD_DATA* apCellData) {
    if (apCellData) {
        if (!apCellData->pCell) {
            apCellData->pCell = apCellData->pWorld->LoadCell(apCellData->iX, apCellData->iY);
        }
        if (!apCellData->pCell) {
            apCellData->pCell = pDataHandler->NewCell(nullptr, apCellData->iX, apCellData->iY, apCellData->pWorld);
        }
        apCellData->pCell->Load3D();
        TESObjectLAND* pLand = apCellData->pCell->GetLand();
        if (pLand) {
            pLand->LoadVertices(true);
            bool bShowLandBorders = TES::pInstance->GetShowLandBorders();
            pLand->DrawLandBorder(bShowLandBorders);
        }
        if (!apCellData->pCell->bAutoWaterLoaded) {
            NiPoint2 kExtents;
            pLand = apCellData->pCell->GetLand();
            kExtents = pLand->GetMinMaxLandHeight();
            double dWaterHeight = apCellData->pCell->GetWaterHeight();
            apCellData->pCell->SetHasWater(kExtents.x < dWaterHeight);
            if (TESWaterSystem::bUseWater.Bool() &&
                apCellData->pCell->HasWater() &&
                !apCellData->pCell->bAutoWaterLoaded) {
                BGSAutoWater::GeneratePlaceableWaterForCell(apCellData->pCell);
            }
            apCellData->pCell->SetAutoWaterLoaded(true);
        }
        apCellData->bLoaded = true;
    }
}

// Each coordinate is truncated to its low 16 bits and zero-extended for GetCoord.
void ExteriorCellLoader::RemoveCell(int32_t aiCellX, int32_t aiCellY) {
    int32_t iCoord = TESObjectCELL::GetCoord(static_cast<uint16_t>(aiCellX), static_cast<uint16_t>(aiCellY));
    LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>* pMap = &QueuedCellMap;
    pMap->RemoveAt(iCoord);
}

void ExteriorCellLoader::AttachLoadedCell(TESObjectCELL* apCell) {
    if (apCell && apCell->Get3D()) {
        TES::pInstance->AddToBuffer(apCell);
    }
}

uint32_t ExteriorCellLoader::QueuedCellCount() {
    LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>* pMap = &QueuedCellMap;
    return pMap->GetCount();
}

void ExteriorCellLoader::ClearData() {
    uint32_t uiKey;
    using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
    iterator_t iterator;
    while (!iterator.GetFinished()) {
        uiKey = 0;
        NiPointer<ExteriorCellLoaderTask> spTask;
        if (QueuedCellMap.GetNext(iterator, uiKey, spTask, true)) {
            pIOManager->CancelTask(spTask.m_pObject, nullptr);
        }
    }
}

bool ExteriorCellLoader::CellIsQueued(int32_t aiCellX, int32_t aiCellY) {
    uint32_t uiKey = TESObjectCELL::GetCoord(static_cast<uint16_t>(aiCellX), static_cast<uint16_t>(aiCellY));
    NiPointer<ExteriorCellLoaderTask> spTask;
    return QueuedCellMap.GetAt(uiKey, spTask);
}

void ExteriorCellLoader::PostProcessCompletedTasks() {
    uint32_t uiKey;
    using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
    iterator_t iterator;
    while (!iterator.GetFinished()) {
        uiKey = 0;
        NiPointer<ExteriorCellLoaderTask> spTask;
        if (QueuedCellMap.GetNext(iterator, uiKey, spTask, true) && spTask->IsCompleted()) {
            spTask->PostProcess();
        }
    }
}

uint32_t ExteriorCellLoader::TryCancelTasks() {
    uint32_t uiKey;
    using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
    uint32_t uiUncanceledTasks = 0;
    iterator_t iterator;
    while (!iterator.GetFinished()) {
        uiKey = 0;
        NiPointer<ExteriorCellLoaderTask> spTask;
        if (QueuedCellMap.GetNext(iterator, uiKey, spTask, true)) {
            if (!pIOManager->TryCancelTask(spTask.m_pObject, nullptr)) {
                ++uiUncanceledTasks;
            }
        }
    }
    return uiUncanceledTasks;
}

void ExteriorCellLoader::WaitForTasks() {
    uint32_t uiKey;
    using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
    iterator_t iterator;
    while (!iterator.GetFinished()) {
        uiKey = 0;
        NiPointer<ExteriorCellLoaderTask> spTask;
        if (QueuedCellMap.GetNext(iterator, uiKey, spTask, true)) {
            pIOManager->WaitForTask(spTask.m_pObject);
            if (spTask->IsCompleted()) {
                spTask->PostProcess();
            }
        }
    }
}

void ExteriorCellLoader::QueueCellLoad(TESWorldSpace* apWorld, int32_t aiCellX, int32_t aiCellY) {
    if (apWorld) {
        TESObjectCELL* pCell = apWorld->GetCellFromCellCoord(aiCellX, aiCellY);
        if (pCell && TES::pInstance->IsBuffered(pCell)) {
            TES::pInstance->AddToBuffer(pCell);
        }
        else if (!CellIsQueued(aiCellX, aiCellY)) {
            CELL_LOAD_DATA* pCellData = PreLoadCellData(apWorld, aiCellX, aiCellY);
            if (pCellData) {
                ExteriorCellLoaderTask* pTask = new ExteriorCellLoaderTask(this, pCellData);
                NiPointer<ExteriorCellLoaderTask> spTask(pTask);
                uint32_t uiKey = TESObjectCELL::GetCoord(
                    static_cast<uint16_t>(aiCellX),
                    static_cast<uint16_t>(aiCellY)
                );
                QueuedCellMap.SetAt(uiKey, spTask, true);
                pIOManager->AddTask(spTask.m_pObject);
            }
        }
    }
}

CELL_LOAD_DATA* ExteriorCellLoader::PreLoadCellData(TESWorldSpace* apWorld, int32_t aiCellX, int32_t aiCellY) {
    CELL_LOAD_DATA* pCell = nullptr;
    if (apWorld) {
        pCell = static_cast<CELL_LOAD_DATA*>(ourMalloc(sizeof(CELL_LOAD_DATA)));
        pCell->pWorld = apWorld;
        pCell->iX = aiCellX;
        pCell->iY = aiCellY;
        pCell->bLoaded = false;
        pCell->pCell = nullptr;
    }
    return pCell;
}

// The copy is tested for null again before it is deleted.
void ExteriorCellLoader::Release() {
    if (pExteriorCellLoader) {
        ExteriorCellLoader* pLoader = pExteriorCellLoader;
        if (pLoader) {
            delete pLoader;
        }
        pExteriorCellLoader = nullptr;
    }
}

void ExteriorCellLoader::Initialize() {
    if (!pExteriorCellLoader) {
        pExteriorCellLoader = new ExteriorCellLoader;
    }
}

void ExteriorCellLoader::operator delete(void* apMemory) {
    ourFree(apMemory);
}

ExteriorCellLoaderTask::ExteriorCellLoaderTask(ExteriorCellLoader* apLoader, CELL_LOAD_DATA* apCellData)
  : IOTask(IO_TASK_PRIORITY_CRITICAL), pLoader(apLoader), pCellData(apCellData) {}

// The cell data the task owns is a raw ourMalloc block.
ExteriorCellLoaderTask::~ExteriorCellLoaderTask() {
    ourFree(pCellData);
}

void ExteriorCellLoaderTask::Run() {
    pLoader->LoadCellData(pCellData);
    pCellData->pCell->QueueReferences(true);
}

void ExteriorCellLoaderTask::Finish() {
    IOTask::AddToPostProcessQueue();
}

void ExteriorCellLoaderTask::PostProcess() {
    if (!IsCancelled() && pCellData->pCell && pCellData->bLoaded) {
        pLoader->AttachLoadedCell(pCellData->pCell);
    }

    pCellData->bLoaded = false;
    pLoader->RemoveCell(pCellData->iX, pCellData->iY);
}

void ExteriorCellLoaderTask::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
    if (pCellData->pCell && TES::pInstance->IsCellLoaded(pCellData->pCell, false)) {
        pDataHandler->UnloadCell(pCellData->pCell);
    }

    pLoader->RemoveCell(pCellData->iX, pCellData->iY);
}

bool ExteriorCellLoaderTask::GetDescription(char* apDescription, uint32_t aiBufferSize) {
    // "ExteriorCellLoaderTask for cell ( %i, %i )"
    BSsprintf(apDescription, aiBufferSize, "ExteriorCellLoaderTask for cell ( %i, %i )", pCellData->iX, pCellData->iY);
    return true;
}

void ExteriorCellLoaderTask::operator delete(void* apMemory) {
    ourFree(apMemory);
}
