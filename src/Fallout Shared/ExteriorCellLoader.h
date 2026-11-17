#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/ExteriorCellLoaderTask.h"
#include "Fallout Shared/ModelLoader.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

struct CELL_LOAD_DATA;
class TESObjectCELL;
class TESWorldSpace;

// Loads exterior cells in the background. Only the members src calls are declared.
class ExteriorCellLoader {
public:
	ExteriorCellLoader();
	~ExteriorCellLoader();

	// Cancels every queued exterior cell load.
	void CancelAllCellLoads() { ClearData(); }
	// Loads apData's exterior cell data.
	void LoadCellData(CELL_LOAD_DATA* apCellData);
	// Removes the queued load of the cell at the requested coordinates (thiscall, ret 8, 0x528560). The exe reads
	// only the low word of each coordinate; no caller reads eax after it.
	void RemoveCell(int32_t aiCellX, int32_t aiCellY);
	// Attaches apCell after background loading.
	void AttachLoadedCell(TESObjectCELL* apCell);
	// The number of queued cell loads.
	uint32_t QueuedCellCount();
	void ClearData();
	bool CellIsQueued(int32_t aiCellX, int32_t aiCellY);
	void PostProcessCompletedTasks();
	uint32_t TryCancelTasks();
	void WaitForTasks();
	void QueueCellLoad(TESWorldSpace* apWorld, int32_t aiCellX, int32_t aiCellY);
	CELL_LOAD_DATA* PreLoadCellData(TESWorldSpace* apWorld, int32_t aiCellX, int32_t aiCellY);

	// Deletes the loader and clears pExteriorCellLoader.
	static void Release();
	static void Initialize();
	static void operator delete(void* apMemory);

	LockFreeMap<uint32_t, NiPointer<ExteriorCellLoaderTask>> QueuedCellMap;
};

inline ExteriorCellLoader* pExteriorCellLoader;

// The game settings ExteriorCellLoader.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bBackgroundCellLoads;
