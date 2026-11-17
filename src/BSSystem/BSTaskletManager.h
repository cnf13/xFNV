#pragma once

#include "BSCore/BSSemaphore.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSTasklet;
class BSTaskletData;
class BSTaskletGroup;
class BSTaskletGroupData;
class BSTaskletManagerCallback;

// Runs tasklet groups on the tasklet threads. Only the members src calls are declared.
class BSTaskletManager {
public:
	using ExitCallbackArray = BSTaskletManagerCallback* [16];
	using StartupCallbackArray = BSTaskletManagerCallback* [16];

	// Values from the Xbox PDB.
	enum : int32_t {
		MAX_CALLBACK = 0x10,
	};

	BSTaskletManager();
	virtual ~BSTaskletManager();

	virtual void SetTaskletThreadPriority(uint32_t, int32_t) {}
	virtual void ShutdownAllTasks() = 0;
	virtual bool IsTaskletThread(uint32_t) = 0;
	virtual bool CreateTaskGroupData(BSTaskletGroupData*&) = 0;
	virtual bool OpenTaskGroupData(BSTaskletGroupData*) = 0;
	virtual bool AttachTaskData(BSTaskletGroupData*, BSTaskletData*, bool) = 0;
	virtual bool CloseTaskGroupData(BSTaskletGroupData*) = 0;
	virtual void DestroyTaskGroupData(BSTaskletGroupData*) = 0;
	virtual void InstanceAvailable() {}

	// Creates a task group into aResult; false on failure.
	bool CreateTaskGroup(BSTaskletGroup& aResult);
	// Frees aGroup's group data.
	void DestroyTaskGroup(BSTaskletGroup& aGroup);
	// Opens aGroup for new tasks; false on failure.
	bool OpenTaskGroup(BSTaskletGroup& aGroup);
	// Adds aTask to aGroup; abLastTask closes the group.
	bool AttachTask(BSTaskletGroup& aGroup, BSTasklet& aTask, bool abLastTask);
	// Closes aGroup and starts its tasks; false when the manager is unavailable or the group has no data.
	bool CloseTaskGroup(BSTaskletGroup& aGroup);
	void OnStartup();
	void OnExit();
	void SignalInstanceAvailable();

	// The singleton, created on first use.
	static BSTaskletManager& QInstance();
	static bool RegisterOnExitCallback(BSTaskletManagerCallback* apCallback);
	static bool RegisterOnStartupCallback(BSTaskletManagerCallback* apCallback);
	// The exe's deleting destructor frees through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);

	bool bAvailable;
	bool bSignaled;
	uint8_t padding06[2];
	// The PDB's BSSemaphore InstanceSema (IDA splits it into iCount, hSemaphore and lMaxCount); the constructor
	// creates its Win32 semaphore with CreateSemaphoreA (0xB02720).
	BSSemaphore InstanceSema;

	static inline int32_t siNumExit;
	static inline ExitCallbackArray spOnExit{};
	static inline int32_t siNumStartup;
	static inline StartupCallbackArray spOnStartup{};
};

// A callback the tasklet manager runs at startup and exit.
class BSTaskletManagerCallback {
public:
	// Values from the Xbox PDB.
	enum Phase : int32_t {
		TMCB_STARTUP = 0x0,
		TMCB_EXIT = 0x1,
	};

	explicit BSTaskletManagerCallback(BSTaskletManagerCallback::Phase aePhase);
	virtual ~BSTaskletManagerCallback();

	virtual void operator()() = 0;

	// The exe's deleting destructors free through MemoryManager::Deallocate.
	static void operator delete(void* apMemory);
};

inline constexpr fnvd::Vtable BSTaskletManager_vftable{};
inline constexpr fnvd::Vtable BSTaskletManagerCallback_vftable{};
