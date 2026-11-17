#include "BSMain/BSTaskManagerThread.h"

#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/BSTaskThread.h"

BSTaskManagerThread::BSTaskManagerThread(BSTaskManager<int64_t>* apOwner, uint32_t aiProcessor)
	: BSTaskThread(aiProcessor, "BSTaskManagerThread"), pOwner(apOwner) {}

// The exe has no destructor of its own here: its vtable's slot 0 is BSTaskThread's (0xC3F130).
BSTaskManagerThread::~BSTaskManagerThread() {}
