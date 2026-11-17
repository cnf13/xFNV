#include "Fallout Shared/TempEffects/BSTECreateTask.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"

NiRTTI BSTECreateTask::ms_RTTI("BSTECreateTask", &NiTask::ms_RTTI);
NiCriticalSection BSTECreateTask::ms_kPoolCriticalSection;
NiTObjectPool<BSTECreateTask> BSTECreateTask::ms_kPool(8);

NiTask::~NiTask() {}

// A task can always run.
bool NiTask::CanExecute() {
	return true;
}

bool NiTask::Clear() {
	return (bool)((uint8_t)((((int32_t)m_eStatus) == 4) || (((int32_t)m_eStatus) == 3)));
}

NiCriticalSection::NiCriticalSection() {
	InitializeCriticalSection(&m_kCriticalSection);
}

NiCriticalSection::~NiCriticalSection() {
	DeleteCriticalSection(&m_kCriticalSection);
}
