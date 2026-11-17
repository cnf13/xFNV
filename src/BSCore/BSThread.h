#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSystemObjects.h"
#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"

// Sleeps the calling thread for auiMSec milliseconds.
void BSSleep(uint32_t auiMSec);

// A Win32 thread with its own critical section; subclasses supply ThreadProc.
class BSThread {
public:
	// The stack size Initialize gives the thread.
	enum StackSize : uint32_t {
		INHERIT_MAIN = 0,
		SMALL_STACK_SIZE = 0x4000,
		MEDIUM_STACK_SIZE = 0x8000,
		LARGE_STACK_SIZE = 0x10000,
		MAINTHREAD_STACK_SIZE = 0x20000,
	};

	BSThread();
	virtual ~BSThread();

	virtual uint32_t ThreadProc();

	// Starts the Win32 thread when no handle exists.
	void Initialize(StackSize aeStackSize, const char* apName, bool bSuspend);
	void SetThreadPriority(int32_t aiPriority);
	// The PC build ignores this Xbox-only naming operation.
	void SetName(const char* apName);
	// The PC build ignores this Xbox-only processor selection.
	void SetThreadProcessor(uint32_t aiThread);
	// bThreadIsActive; the exe's shared one-byte getter 0x8905F0 (thiscall, ret 0).
	bool QIsThreadActive();
	void Close();

	static uint32_t Exit(void* apThread);
	static void operator delete(void* apMemory) { MemoryManager_Singleton.Deallocate(apMemory); }

	BSCriticalSection CriticalSection;
	BSSystemThread* m_ThreadHandle;
	void* m_ParentHandle;
	uint32_t m_ThreadID;
	uint32_t m_ParentID;
	bool bThreadIsActive;
};
inline constexpr fnvd::Vtable BSThread_vftable{};
