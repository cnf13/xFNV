#pragma once

#include "Reconstruction.hpp"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <semaphore>
#include <thread>

// The kernel objects the exe creates through KERNEL32 (CreateSemaphoreA, CreateEventA, CreateThread), in standard
// C++. A class holding one keeps it behind a pointer in the HANDLE's slot, so its layout stays the exe's. The waits
// return the Win32 values the exe's callers test.
namespace BSSystemWait {
// WaitForSingleObject's results and its infinite timeout.
inline constexpr uint32_t SIGNALED = 0;
inline constexpr uint32_t TIMED_OUT = 0x102;
inline constexpr uint32_t INFINITE_TIME = 0xFFFFFFFF;
}

// A Win32 semaphore: the count stays between 0 and the maximum given
// at creation, and a release that would pass the maximum adds nothing and fails, as ReleaseSemaphore does.
class BSSystemSemaphore {
public:
	BSSystemSemaphore(int32_t aiInitialCount, int32_t aiMaxCount);

	// ReleaseSemaphore(handle, aiCount, nullptr).
	bool Release(int32_t aiCount = 1);
	// WaitForSingleObject(handle, auiMilliseconds): SIGNALED with one count taken, or TIMED_OUT.
	uint32_t Wait(uint32_t auiMilliseconds = BSSystemWait::INFINITE_TIME);

private:
	std::counting_semaphore<INT32_MAX> kSemaphore;
	// The count released and not yet taken, kept beside kSemaphore's own to hold the maximum.
	std::atomic<int32_t> iCount;
	const int32_t iMaxCount;
};

// A Win32 event: a manual-reset one stays signaled until Reset and
// releases every waiter; an auto-reset one releases one waiter and resets itself.
class BSSystemEvent {
public:
	BSSystemEvent(bool abManualReset, bool abInitialState);

	// SetEvent.
	void Set();
	// ResetEvent.
	void Reset();
	// WaitForSingleObject(handle, auiMilliseconds): SIGNALED or TIMED_OUT.
	uint32_t Wait(uint32_t auiMilliseconds = BSSystemWait::INFINITE_TIME);

private:
	// Takes the signal when set (resetting an auto-reset event); false when not signaled.
	bool TryTake();

public:
	// WaitForMultipleObjects(2, {first, second}, FALSE, auiMilliseconds): 0 when arFirst is signaled (looked at
	// first, as Win32 takes the lowest index), 1 when arSecond is, else TIMED_OUT. The auto-reset event it returns
	// for is reset.
	static uint32_t WaitAny(BSSystemEvent& arFirst, BSSystemEvent& arSecond,
		uint32_t auiMilliseconds = BSSystemWait::INFINITE_TIME);

private:
	std::mutex kLock;
	std::condition_variable kSignal;
	bool bSignaled;
	const bool bManualReset;
};

// A thread as CreateThread starts one: a thread created suspended
// waits at a start gate until Resume (ResumeThread), and the procedure's return value (or ExitCurrent's code) is
// kept as its exit code. Destroying it is CloseHandle: the thread runs on, detached, if it has not finished.
// The stack size the exe asks for is not applied (std::thread has none); every caller asks for the default or less.
// SuspendThread has no standard form (plan section 3, a shutdown-only departure): Suspend stops the thread at its next
// return from a BSSystemSemaphore or BSSystemEvent wait, not at once.
class BSSystemThread {
public:
	using Procedure = uint32_t (*)(void*);
	// GetExitCodeThread's value while the thread runs.
	static constexpr uint32_t STILL_ACTIVE = 259;

private:
	// What the thread and its owner share; the thread holds it too, so a detached thread outlives its owner safely.
	struct State {
		std::mutex kLock;
		std::condition_variable kChanged;
		bool bIdKnown = false;
		bool bStarted = false;
		bool bFinished = false;
		bool bSuspended = false;
		uint32_t uiExitCode = STILL_ACTIVE;
		uint32_t uiId = 0;
	};

public:
	// CreateThread(nullptr, 0, apProcedure, apArgument, abSuspended ? CREATE_SUSPENDED : 0, &id). The id is the one
	// GetCurrentThreadId gives inside the thread, known when the constructor returns.
	BSSystemThread(Procedure apProcedure, void* apArgument, bool abSuspended);
	BSSystemThread(const BSSystemThread&) = delete;
	~BSSystemThread();

	BSSystemThread& operator=(const BSSystemThread&) = delete;

	// ResumeThread: opens the start gate and lifts a Suspend.
	void Resume();
	// SuspendThread: the thread stops at its next wait (see above) until Resume. Returns the previous suspend count,
	// 0 or 1.
	uint32_t Suspend();
	// GetExitCodeThread: STILL_ACTIVE until the procedure has returned.
	uint32_t ExitCode() const;
	// WaitForSingleObject on the thread: SIGNALED once it has finished, else TIMED_OUT after auiMilliseconds.
	uint32_t Wait(uint32_t auiMilliseconds = BSSystemWait::INFINITE_TIME);
	uint32_t Id() const { return uiId; }
	// SetThreadPriority(handle, aiPriority), with the Win32 priority values the exe passes (platform layer).
	bool SetPriority(int32_t aiPriority);
	// The debugger's name of the thread (the exe's SetThreadName with the thread's id; platform layer).
	void SetName(const char* apName);

	// ExitThread(auiExitCode) from inside a thread this class started: unwinds to the thread's start and ends it
	// with that exit code.
	[[noreturn]] static void ExitCurrent(uint32_t auiExitCode);
	// Called by the semaphore's and the event's waits: holds a thread this class started while it is suspended.
	static void StopIfSuspended();

private:
	// The state of the thread this class started that is running the caller, else null.
	static State*& Current();
	static void Run(std::shared_ptr<State> aspState, Procedure apProcedure, void* apArgument);

	std::shared_ptr<State> spState;
	uint32_t uiId;
	std::thread kThread;
};
