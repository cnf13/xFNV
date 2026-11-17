#pragma once

#include "Reconstruction.hpp"

class BSSimpleArray_void;

class BSParallelTask {
public:
	// IDA name; not in the PDB. Called cdecl with pArray and the item's index (BSParallelTaskThread::IteratorFunc
	// 0xC44D90: call through [task+0], add esp, 8).
	void (*pCallBack)(BSSimpleArray_void* apArray, uint32_t auiIndex);
	BSSimpleArray_void* pArray;
	uint32_t uiSize;
	uint32_t uiPosition;
};
