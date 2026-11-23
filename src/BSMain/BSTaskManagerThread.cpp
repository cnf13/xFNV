#include "BSMain/BSTaskManagerThread.h"

#include "BSMain/BSTaskManager.h"
#include "Fallout Shared/BSTaskThread.h"

BSTaskManagerThread::BSTaskManagerThread(BSTaskManager<int64_t>* apOwner, uint32_t aiProcessor)
  : BSTaskThread(aiProcessor, "BSTaskManagerThread"), pOwner(apOwner) {}

BSTaskManagerThread::~BSTaskManagerThread() {}
