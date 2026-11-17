#pragma once

#include "BSCore/BSSemaphore.h"
#include "BSSystem/BSTaskletManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSWin32TaskletData;
class BSWin32TaskletManager;

// IDA name; not in the PDB. The PC tasklet manager, the counterpart of the PDB's BSXenonTaskletManager: it owns up to
// six worker threads and the per-thread queues of tasklet data. Every member name below is IDA's.
class BSWin32TaskletManager : public BSTaskletManager {
public:
	BSWin32TaskletManager(uint32_t auiNumThreads);
	~BSWin32TaskletManager() override;

	void ShutdownAllTasks() override;
	bool IsTaskletThread(uint32_t) override;
	bool CreateTaskGroupData(BSTaskletGroupData*&) override;
	bool OpenTaskGroupData(BSTaskletGroupData*) override;
	bool AttachTaskData(BSTaskletGroupData*, BSTaskletData*, bool) override;
	bool CloseTaskGroupData(BSTaskletGroupData*) override;
	void DestroyTaskGroupData(BSTaskletGroupData*) override;

	void ProcessQueue();
	char QueueTaskData(BSWin32TaskletData* arData);
	void Threads();

	// The PDB's BSXenonTaskletManager::ThreadInitProc(apThis): the init thread's start routine; waits on
	// InstanceSema, then resumes the workers. PDB: BSXenonTaskletManager* apThis; the exe passes it as CreateThread's
	// start routine (stdcall, ret 4) with the manager as its parameter.
	static uint32_t ThreadInitProc(void* apThis);
	// The PDB's BSXenonTaskletManager::ThreadProc(apThis): the worker threads' start routine. PDB: void* (*)(
	// BSXenonTaskletManager*); the exe passes it as CreateThread's start routine (stdcall, ret 4) with the manager as
	// its parameter.
	static uint32_t ThreadProc(void* apThis);

	// IDA: HANDLE hInitThreadHandle; the exe's thread handle, here the standard-C++ thread in its slot.
	BSSystemThread* hInitThreadHandle;
	uint32_t uiNumThreads;
	// IDA: UInt32 uiNumThreadsCreated.
	uint32_t uiNumThreadsCreated;
	// IDA: HANDLE[6] hThreadHandles; the exe's thread handles, here the standard-C++ threads in their slots.
	BSSystemThread* hThreadHandles[6];
	uint32_t uiThreadIDs[6];
	bool bTerminated;
	uint8_t padding51[3];
	_RTL_CRITICAL_SECTION kQueueLock;
	BSWin32TaskletData* pQueueHead[64];
	BSWin32TaskletData** ppQueueTail[64];
	BSSemaphore kStartSemaphore;
	char cThreadNames[0x600];
};
inline constexpr fnvd::Vtable BSWin32TaskletManager_vftable{};
