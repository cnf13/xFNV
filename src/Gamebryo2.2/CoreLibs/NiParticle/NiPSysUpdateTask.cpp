#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysUpdateTask.h"

#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParallelUpdateTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"

NiRTTI NiPSysUpdateTask::ms_RTTI("NiPSysUpdateTask", &NiTask::ms_RTTI);
NiCriticalSection NiPSysUpdateTask::ms_kPoolCriticalSection;
NiTObjectPool<NiPSysUpdateTask> NiPSysUpdateTask::ms_kPool;

// A real new[]: clang gives the class a vector deleting destructor only where its new[] is seen, and _SDMShutdown's
// delete[] goes through it.
template <> NiPSysUpdateTask* NiTNewInterface<NiPSysUpdateTask>::Allocate(uint32_t uiNumElements) {
	return new NiPSysUpdateTask[uiNumElements];
}

using NiTPool_lNiPSysUpdateTask_cNiTNewInterface_lNiPSysUpdateTask =
	NiTPool<NiPSysUpdateTask, NiTNewInterface<NiPSysUpdateTask>>;

NiPSysUpdateTask::NiPSysUpdateTask() : m_spSystem(nullptr), m_fTime(0.0f) {}

NiPSysUpdateTask::~NiPSysUpdateTask() {}

const NiRTTI* NiPSysUpdateTask::GetRTTI() const {
	return &NiPSysUpdateTask::ms_RTTI;
}

// Updates the system unless the task manager is shutting down, then returns the task to the pool.
void NiPSysUpdateTask::DoTask() {
	NiParticleSystem* pkSystem = m_spSystem.m_pObject;
	if (pkSystem && !NiParallelUpdateTaskManager::bEnd) {
		pkSystem->Do_UpdateSystem(m_fTime);
		m_spSystem = nullptr;
	}
	NiTask* pkTask = this;
	pkTask->Clear();
}

bool NiPSysUpdateTask::Clear() {
	m_spSystem = nullptr;
	ms_kPoolCriticalSection.Lock();
	ms_kPool.ReleaseObject(this);
	ms_kPoolCriticalSection.Unlock();
	return true;
}

void NiPSysUpdateTask::Init(NiParticleSystem* pkSystem, float fTime) {
	m_spSystem = pkSystem;
	m_fTime = fTime;
}

NiPSysUpdateTask* NiPSysUpdateTask::GetFreeObject() {
	EnterCriticalSection(&NiPSysUpdateTask::ms_kPoolCriticalSection.m_kCriticalSection);
	if (!NiPSysUpdateTask::ms_kPool.m_kFreeObjects.m_uiUsed) {
		NiPSysUpdateTask::ms_kPool.CreateNewObjects(NiPSysUpdateTask::ms_kPool.m_uiCurrentSize);
		NiPSysUpdateTask::ms_kPool.m_uiCurrentSize *= 2;
	}
	NiPSysUpdateTask* v0 = *NiPSysUpdateTask::ms_kPool.m_kFreeObjects.m_pBase;
	*NiPSysUpdateTask::ms_kPool.m_kFreeObjects.m_pBase =
		NiPSysUpdateTask::ms_kPool.m_kFreeObjects.m_pBase[--NiPSysUpdateTask::ms_kPool.m_kFreeObjects.m_uiUsed];
	LeaveCriticalSection(&NiPSysUpdateTask::ms_kPoolCriticalSection.m_kCriticalSection);
	return v0;
}

// As NiGeomMorpherUpdateTask::_SDMShutdown: the pool's free objects are purged, the first container is destroyed
// directly and the next ones through the container's deleting destructor; the container pointer is left as it was.
void NiPSysUpdateTask::_SDMShutdown() {
	NiTPoolContainer<NiPSysUpdateTask, NiTNewInterface<NiPSysUpdateTask>>* pkContainers = ms_kPool.m_pkContainers;
	ms_kPool.m_kFreeObjects.m_uiUsed = 0;
	if (pkContainers) {
		NiPSysUpdateTask* pkObjectArray = pkContainers->m_pkObjectArray;
		if (pkObjectArray) {
			if (reinterpret_cast<size_t*>(pkObjectArray)[-1]) {
				delete[] pkObjectArray;
			} else {
				MemoryManager_Singleton.Deallocate(reinterpret_cast<size_t*>(pkObjectArray) - 1);
			}
		}
		if (pkContainers->m_pkNext) {
			delete pkContainers->m_pkNext;
		}
		MemoryManager_Singleton.Deallocate(pkContainers);
	}
	ms_kPool.m_uiCurrentSize = ms_kPool.m_uiInitialSize;
}
