#include "Fallout Shared/TempEffects/BSTECreateTask.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"

NiRTTI BSTECreateTask::ms_RTTI("BSTECreateTask", &NiTask::ms_RTTI);

NiCriticalSection BSTECreateTask::ms_kPoolCriticalSection;
NiTObjectPool<BSTECreateTask> BSTECreateTask::ms_kPool(8);

BSTECreateTask::~BSTECreateTask() {}
