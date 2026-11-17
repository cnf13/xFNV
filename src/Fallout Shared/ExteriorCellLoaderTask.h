#pragma once

#include "BSMain/IOManager.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct CELL_LOAD_DATA;
class ExteriorCellLoader;

// The background load of one exterior cell, queued by the ExteriorCellLoader at critical priority. The PDB's source
// module is ExteriorCellLoader.cpp.
class ExteriorCellLoaderTask : public IOTask {
public:
	ExteriorCellLoaderTask(ExteriorCellLoader* apLoader, CELL_LOAD_DATA* apCellData);
	~ExteriorCellLoaderTask() override;

	void Run() override;
	void Finish() override;
	void PostProcess() override;
	void Cancel(BS_TASK_STATE aePreviousState, void* apTaskOwner) override;
	bool GetDescription(char* apDescription, uint32_t aiBufferSize) override;

	static void operator delete(void* apMemory);

	ExteriorCellLoader* pLoader;
	CELL_LOAD_DATA* pCellData;
};

inline constexpr fnvd::Vtable ExteriorCellLoaderTask_vftable{};
