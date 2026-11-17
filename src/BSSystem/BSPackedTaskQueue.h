#pragma once

#include "BSSystem/BSTMessageQueue.h"
#include "Reconstruction.hpp"

typedef void (*UnpackFunc)(BSPackedTask* apTask);

// A queue of packed tasks (BSPackedTask) with the function that unpacks and runs them.
class BSPackedTaskQueue {
public:
	// Defined in Main.cpp.
	BSPackedTaskQueue(ScrapHeap* apScrapHeap, UnpackFunc apUnpackFunc);

	void ThreadBeginInput();
	void ThreadEndInput();
	bool IsEmpty();
	bool Process();
	bool AddTask(BSPackedTask* apMessage);

	// The PDB's BSTCommonScrapHeapMessageQueue<BSPackedTask>; IDA's instance is declared in BSTMessageQueue.h.
	BSTCommonScrapHeapMessageQueue Queue;
	UnpackFunc pUnpackFunc;
	bool bProcessing;
	uint8_t padding25[3];
};
