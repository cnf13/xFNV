#include "BSCore/BSSystemObjects.h"

#include "SDK/Windows/winbase.h"
#include "platform/System.h"

#include <chrono>

namespace {
// What ExitCurrent throws: caught at the thread's start, so the thread ends with its code.
struct ExitRequest {
	uint32_t uiExitCode;
};

// BSSystemEvent::WaitAny's waiters sleep on one process-wide condition, which Set wakes while any of them waits
// (the count spares every other Set the lock). Lock order: kAnyLock, then an event's own lock.
std::mutex kAnyLock;
std::condition_variable kAnySignal;
std::atomic<int32_t> iAnyWaiters{0};

std::chrono::milliseconds Milliseconds(uint32_t auiMilliseconds) {
	return std::chrono::milliseconds(auiMilliseconds);
}

}

BSSystemSemaphore::BSSystemSemaphore(int32_t aiInitialCount, int32_t aiMaxCount)
	: kSemaphore(aiInitialCount), iCount(aiInitialCount), iMaxCount(aiMaxCount) {}

bool BSSystemSemaphore::Release(int32_t aiCount) {
	int32_t iOld = iCount.load();
	do {
		if (aiCount <= 0 || aiCount > iMaxCount - iOld) {
			return false;
		}
	} while (!iCount.compare_exchange_weak(iOld, iOld + aiCount));
	kSemaphore.release(aiCount);
	return true;
}

uint32_t BSSystemSemaphore::Wait(uint32_t auiMilliseconds) {
	if (auiMilliseconds == BSSystemWait::INFINITE_TIME) {
		kSemaphore.acquire();
	} else if (!kSemaphore.try_acquire_for(Milliseconds(auiMilliseconds))) {
		BSSystemThread::StopIfSuspended();
		return BSSystemWait::TIMED_OUT;
	}
	--iCount;
	BSSystemThread::StopIfSuspended();
	return BSSystemWait::SIGNALED;
}

BSSystemEvent::BSSystemEvent(bool abManualReset, bool abInitialState)
	: bSignaled(abInitialState), bManualReset(abManualReset) {}

void BSSystemEvent::Set() {
	{
		std::lock_guard kGuard(kLock);
		bSignaled = true;
		if (bManualReset) {
			kSignal.notify_all();
		} else {
			kSignal.notify_one();
		}
	}
	// A WaitAny waiter that saw the event unset is already asleep on kAnySignal, or still holds kAnyLock.
	if (iAnyWaiters.load() != 0) {
		std::lock_guard kGuard(kAnyLock);
		kAnySignal.notify_all();
	}
}

void BSSystemEvent::Reset() {
	std::lock_guard kGuard(kLock);
	bSignaled = false;
}

uint32_t BSSystemEvent::Wait(uint32_t auiMilliseconds) {
	uint32_t uiResult = BSSystemWait::SIGNALED;
	{
		std::unique_lock kGuard(kLock);
		auto IsSignaled = [this] { return bSignaled; };
		if (auiMilliseconds == BSSystemWait::INFINITE_TIME) {
			kSignal.wait(kGuard, IsSignaled);
		} else if (!kSignal.wait_for(kGuard, Milliseconds(auiMilliseconds), IsSignaled)) {
			uiResult = BSSystemWait::TIMED_OUT;
		}
		if (uiResult == BSSystemWait::SIGNALED && !bManualReset) {
			bSignaled = false;
		}
	}
	BSSystemThread::StopIfSuspended();
	return uiResult;
}

bool BSSystemEvent::TryTake() {
	std::lock_guard kGuard(kLock);
	if (!bSignaled) {
		return false;
	}
	if (!bManualReset) {
		bSignaled = false;
	}
	return true;
}

uint32_t BSSystemEvent::WaitAny(BSSystemEvent& arFirst, BSSystemEvent& arSecond, uint32_t auiMilliseconds) {
	uint32_t uiResult = BSSystemWait::TIMED_OUT;
	++iAnyWaiters;
	{
		std::unique_lock kGuard(kAnyLock);
		auto Take = [&] {
			if (arFirst.TryTake()) {
				uiResult = 0;
			} else if (arSecond.TryTake()) {
				uiResult = 1;
			}
			return uiResult != BSSystemWait::TIMED_OUT;
		};
		if (auiMilliseconds == BSSystemWait::INFINITE_TIME) {
			kAnySignal.wait(kGuard, Take);
		} else {
			kAnySignal.wait_for(kGuard, Milliseconds(auiMilliseconds), Take);
		}
	}
	--iAnyWaiters;
	BSSystemThread::StopIfSuspended();
	return uiResult;
}

BSSystemThread::BSSystemThread(Procedure apProcedure, void* apArgument, bool abSuspended)
	: spState(std::make_shared<State>()), uiId(0) {
	spState->bStarted = !abSuspended;
	kThread = std::thread(Run, spState, apProcedure, apArgument);
	std::unique_lock kGuard(spState->kLock);
	spState->kChanged.wait(kGuard, [this] { return spState->bIdKnown; });
	uiId = spState->uiId;
}

// CloseHandle: a finished thread is joined; a running one runs on, and a suspended one stays at its gate, as a Win32
// thread closed while suspended stays suspended.
BSSystemThread::~BSSystemThread() {
	std::unique_lock kGuard(spState->kLock);
	bool bFinished = spState->bFinished;
	kGuard.unlock();
	if (bFinished) {
		kThread.join();
	} else {
		kThread.detach();
	}
}

void BSSystemThread::Resume() {
	std::lock_guard kGuard(spState->kLock);
	spState->bStarted = true;
	spState->bSuspended = false;
	spState->kChanged.notify_all();
}

uint32_t BSSystemThread::Suspend() {
	std::lock_guard kGuard(spState->kLock);
	uint32_t uiPrevious = spState->bSuspended ? 1 : 0;
	spState->bSuspended = true;
	return uiPrevious;
}

uint32_t BSSystemThread::ExitCode() const {
	std::lock_guard kGuard(spState->kLock);
	return spState->bFinished ? spState->uiExitCode : STILL_ACTIVE;
}

uint32_t BSSystemThread::Wait(uint32_t auiMilliseconds) {
	std::unique_lock kGuard(spState->kLock);
	auto IsFinished = [this] { return spState->bFinished; };
	if (auiMilliseconds == BSSystemWait::INFINITE_TIME) {
		spState->kChanged.wait(kGuard, IsFinished);
	} else if (!spState->kChanged.wait_for(kGuard, Milliseconds(auiMilliseconds), IsFinished)) {
		return BSSystemWait::TIMED_OUT;
	}
	return BSSystemWait::SIGNALED;
}

bool BSSystemThread::SetPriority(int32_t aiPriority) {
	return platform::SetThreadPriority(kThread, aiPriority);
}

void BSSystemThread::SetName(const char* apName) {
	platform::SetThreadName(kThread, apName);
}

void BSSystemThread::ExitCurrent(uint32_t auiExitCode) {
	throw ExitRequest{auiExitCode};
}

void BSSystemThread::StopIfSuspended() {
	State* pState = Current();
	if (pState) {
		std::unique_lock kGuard(pState->kLock);
		pState->kChanged.wait(kGuard, [pState] { return !pState->bSuspended; });
	}
}

BSSystemThread::State*& BSSystemThread::Current() {
	thread_local State* tpState = nullptr;
	return tpState;
}

void BSSystemThread::Run(std::shared_ptr<State> aspState, Procedure apProcedure, void* apArgument) {
	Current() = aspState.get();
	{
		std::unique_lock kGuard(aspState->kLock);
		aspState->uiId = GetCurrentThreadId();
		aspState->bIdKnown = true;
		aspState->kChanged.notify_all();
		aspState->kChanged.wait(kGuard, [&] { return aspState->bStarted; });
	}
	uint32_t uiExitCode;
	try {
		uiExitCode = apProcedure(apArgument);
	} catch (const ExitRequest& arRequest) {
		uiExitCode = arRequest.uiExitCode;
	}
	std::lock_guard kGuard(aspState->kLock);
	aspState->uiExitCode = uiExitCode;
	aspState->bFinished = true;
	aspState->kChanged.notify_all();
}
