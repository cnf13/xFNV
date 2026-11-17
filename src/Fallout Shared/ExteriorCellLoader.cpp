#include "Fallout Shared/ExteriorCellLoader.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSAutoWater.h"
#include "Fallout Shared/CELL_LOAD_DATA.h"
#include "Fallout Shared/ExteriorCellLoaderTask.h"
#include "Fallout Shared/LockFreeMap.h"
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
			apCellData->pCell = pDataHandler->NewCell(0, apCellData->iX, apCellData->iY, apCellData->pWorld);
		}
		apCellData->pCell->Load3D();
		TESObjectLAND* pland = apCellData->pCell->GetLand();
		if (pland) {
			pland->LoadVertices(1);
			bool ShowLandBorders = TES::pInstance->GetShowLandBorders();
			pland->DrawLandBorder(ShowLandBorders);
		}
		if (!apCellData->pCell->bAutoWaterLoaded) {
			NiPoint2 extents;
			TESObjectLAND* pland_2 = apCellData->pCell->GetLand();
			extents = pland_2->GetMinMaxLandHeight();
			double WaterHeight_0 = apCellData->pCell->GetWaterHeight();
			apCellData->pCell->SetHasWater(extents.x < WaterHeight_0);
			if (TESWaterSystem::bUseWater.Bool() && apCellData->pCell->HasWater() &&
				!apCellData->pCell->bAutoWaterLoaded) {
				BGSAutoWater::GeneratePlaceableWaterForCell(apCellData->pCell);
			}
			apCellData->pCell->SetAutoWaterLoaded(1);
		}
		apCellData->bLoaded = 1;
	}
}

// Each coordinate is truncated to its low 16 bits and zero-extended for GetCoord.
void ExteriorCellLoader::RemoveCell(int32_t aiCellX, int32_t aiCellY) {
	int32_t iCoord = TESObjectCELL::GetCoord(static_cast<uint16_t>(aiCellX), static_cast<uint16_t>(aiCellY));
	LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>* pMap = &QueuedCellMap;
	pMap->RemoveAt(iCoord);
}

void ExteriorCellLoader::AttachLoadedCell(TESObjectCELL* apCell) {
	if (apCell) {
		if (apCell->Get3D()) {
			TES::pInstance->AddToBuffer(apCell);
		}
	}
}

uint32_t ExteriorCellLoader::QueuedCellCount() {
	LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>* pMap = &QueuedCellMap;
	return pMap->GetCount();
}

void ExteriorCellLoader::ClearData() {
	uint32_t ikey;
	FNVD_INLINED_LOCAL(NiPointer<ExteriorCellLoaderTask>, sptask, std::nullopt, std::nullopt);
	using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
	iterator_t iterator;
	while (!iterator.GetFinished()) {
		ikey = 0;
		::new (&sptask) NiPointer<QueuedFile>(static_cast<QueuedFile*>(0));
		if (QueuedCellMap.GetNext(iterator, ikey, sptask, true)) {
			ExteriorCellLoaderTask* v1 = sptask.m_pObject;
			pIOManager->CancelTask(v1, 0);
		}
		sptask.~NiPointer<ExteriorCellLoaderTask>();
	}
}

bool ExteriorCellLoader::CellIsQueued(int32_t aiCellX, int32_t aiCellY) {
	FNVD_INLINED_LOCAL(NiPointer<ExteriorCellLoaderTask>, sptask, std::nullopt, std::nullopt);
	uint32_t ikey = TESObjectCELL::GetCoord(static_cast<uint16_t>(aiCellX), static_cast<uint16_t>(aiCellY));
	::new (&sptask) NiPointer<QueuedFile>(static_cast<QueuedFile*>(0));
	bool v5 = QueuedCellMap.GetAt(ikey, sptask);
	// This destructor releases through BSTask's DecRefCount.
	sptask.~NiPointer<ExteriorCellLoaderTask>();
	return v5;
}

void ExteriorCellLoader::PostProcessCompletedTasks() {
	uint32_t ikey;
	FNVD_INLINED_LOCAL(NiPointer<ExteriorCellLoaderTask>, sptask, std::nullopt, std::nullopt);
	using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
	iterator_t iterator;
	while (!iterator.GetFinished()) {
		ikey = 0;
		::new (&sptask) NiPointer<QueuedFile>(static_cast<QueuedFile*>(0));
		if (QueuedCellMap.GetNext(iterator, ikey, sptask, true) && sptask.m_pObject->IsCompleted()) {
			ExteriorCellLoaderTask* pTask = sptask.m_pObject;
			pTask->PostProcess();
		}
		sptask.~NiPointer<ExteriorCellLoaderTask>();
	}
}

uint32_t ExteriorCellLoader::TryCancelTasks() {
	uint32_t ikey;
	FNVD_INLINED_LOCAL(NiPointer<ExteriorCellLoaderTask>, sptask, std::nullopt, std::nullopt);
	using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
	uint32_t iuncanceledTasks = 0;
	iterator_t iterator;
	while (!iterator.GetFinished()) {
		ikey = 0;
		::new (&sptask) NiPointer<QueuedFile>(static_cast<QueuedFile*>(0));
		if (QueuedCellMap.GetNext(iterator, ikey, sptask, true)) {
			ExteriorCellLoaderTask* v2 = sptask.m_pObject;
			if (!pIOManager->TryCancelTask(v2, 0)) {
				++iuncanceledTasks;
			}
		}
		sptask.~NiPointer<ExteriorCellLoaderTask>();
	}
	return iuncanceledTasks;
}

void ExteriorCellLoader::WaitForTasks() {
	uint32_t ikey;
	FNVD_INLINED_LOCAL(NiPointer<ExteriorCellLoaderTask>, sptask, std::nullopt, std::nullopt);
	using iterator_t = LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>>::LockFreeMapIterator;
	iterator_t iterator;
	while (!iterator.GetFinished()) {
		ikey = 0;
		::new (&sptask) NiPointer<QueuedFile>(static_cast<QueuedFile*>(0));
		if (QueuedCellMap.GetNext(iterator, ikey, sptask, true)) {
			ExteriorCellLoaderTask* v1 = sptask.m_pObject;
			pIOManager->WaitForTask(v1);
			if (sptask.m_pObject->IsCompleted()) {
				ExteriorCellLoaderTask* pTask = sptask.m_pObject;
				pTask->PostProcess();
			}
		}
		sptask.~NiPointer<ExteriorCellLoaderTask>();
	}
}

void ExteriorCellLoader::QueueCellLoad(TESWorldSpace* apWorld, int32_t aiCellX, int32_t aiCellY) {
	FNVD_INLINED_LOCAL(NiPointer<ExteriorCellLoaderTask>, sptask, std::nullopt, std::nullopt);
	if (apWorld) {
		TESObjectCELL* pcell = apWorld->GetCellFromCellCoord(aiCellX, aiCellY);
		if (pcell && TES::pInstance->IsBuffered(pcell)) {
			TES::pInstance->AddToBuffer(pcell);
		} else {
			if (!CellIsQueued(aiCellX, aiCellY)) {
				CELL_LOAD_DATA* pcelldata = PreLoadCellData(apWorld, aiCellX, aiCellY);
				if (pcelldata) {
					void* v5;
					ExteriorCellLoaderTask* v7 =
						static_cast<ExteriorCellLoaderTask*>(ourMalloc(sizeof(ExteriorCellLoaderTask)));
					if (v7) {
						v5 = ::new (v7) ExteriorCellLoaderTask(this, pcelldata);
					} else {
						v5 = 0;
					}
					::new (&sptask) NiPointer<QueuedFile>(static_cast<QueuedFile*>(v5));
					uint32_t ikey =
						TESObjectCELL::GetCoord(static_cast<uint16_t>(aiCellX), static_cast<uint16_t>(aiCellY));
					QueuedCellMap.SetAt(ikey, sptask, 1);
					ExteriorCellLoaderTask* v4 = sptask.m_pObject;
					pIOManager->AddTask(v4);
					sptask.~NiPointer<ExteriorCellLoaderTask>();
				}
			}
		}
	}
}

CELL_LOAD_DATA* ExteriorCellLoader::PreLoadCellData(TESWorldSpace* apWorld, int32_t aiCellX, int32_t aiCellY) {
	CELL_LOAD_DATA* pcell = 0;
	if (apWorld) {
		pcell = static_cast<CELL_LOAD_DATA*>(ourMalloc(sizeof(CELL_LOAD_DATA)));
		pcell->pWorld = apWorld;
		pcell->iX = aiCellX;
		pcell->iY = aiCellY;
		pcell->bLoaded = 0;
		pcell->pCell = 0;
	}
	return pcell;
}

// The copy is tested for null again before it is deleted, as the game does.
void ExteriorCellLoader::Release() {
	if (pExteriorCellLoader) {
		ExteriorCellLoader* pLoader = pExteriorCellLoader;
		if (pLoader)
			delete pLoader;
		pExteriorCellLoader = nullptr;
	}
}

void ExteriorCellLoader::Initialize() {
	ExteriorCellLoader* v0;
	void* v1;
	if (!pExteriorCellLoader) {
		v1 = ourMalloc(sizeof(ExteriorCellLoader));
		if (v1) {
			v0 = ::new (v1) ExteriorCellLoader;
		} else {
			v0 = nullptr;
		}
		pExteriorCellLoader = v0;
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
	pCellData->pCell->QueueReferences(1);
}

void ExteriorCellLoaderTask::Finish() {
	IOTask::AddToPostProcessQueue();
}

void ExteriorCellLoaderTask::PostProcess() {
	if (!IsCancelled()) {
		if (pCellData->pCell) {
			if (pCellData->bLoaded) {
				pLoader->AttachLoadedCell(pCellData->pCell);
			}
		}
	}
	pCellData->bLoaded = 0;
	pLoader->RemoveCell(pCellData->iX, pCellData->iY);
}

void ExteriorCellLoaderTask::Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) {
	if (pCellData->pCell) {
		if (TES::pInstance->IsCellLoaded(pCellData->pCell, 0)) {
			pDataHandler->UnloadCell(pCellData->pCell);
		}
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
