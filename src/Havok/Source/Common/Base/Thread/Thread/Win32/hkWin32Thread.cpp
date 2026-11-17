#include "Havok/Source/Common/Base/Thread/Thread/Win32/hkWin32Thread.h"

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "platform/System.h"
#include "SDK/Windows/winbase.h"

#include <atomic>
#include <future>
#include <memory>
#include <thread>

// The running flag is shared with the thread, so a thread closed while it runs (detached, as CloseHandle leaves a Win32
// thread running) never touches freed memory.
struct hkThreadHandle {
	std::thread kThread;
	std::shared_ptr<std::atomic<bool>> spRunning;
};

hkThread::hkThread() : m_thread(nullptr) {}

hkThread::~hkThread() {
	stopThread();
}

// CloseHandle: a finished thread is joined, a running one runs on detached.
void hkThread::stopThread() {
	if (m_thread) {
		if (m_thread->spRunning->load()) {
			m_thread->kThread.detach();
		} else {
			m_thread->kThread.join();
		}
		delete m_thread;
		m_thread = nullptr;
	}
}

// The platform layer names the thread (Win32: the debugger's thread-naming exception 0x406D1388, raised only with a
// debugger attached). The stack size is not applied (std::thread has none). The id is GetCurrentThreadId's inside the
// thread, as CreateThread reports it; a thread that cannot start throws (std::system_error).
hkResult hkThread::startThread(void* (*func)(void*), void* arg, const char* name, int32_t stackSize) {
	auto spRunning = std::make_shared<std::atomic<bool>>(true);
	std::promise<uint32_t> kId;
	std::future<uint32_t> kIdReady = kId.get_future();
	m_thread = new hkThreadHandle{std::thread([func, arg, spRunning, kId = std::move(kId)]() mutable {
									  kId.set_value(GetCurrentThreadId());
									  func(arg);
									  spRunning->store(false);
								  }),
		spRunning};
	uint32_t dwThreadId = kIdReady.get();
	m_threadId = dwThreadId;
	platform::SetThreadName(m_thread->kThread, name);
	return HK_SUCCESS;
}

void* hkThread::getHandle() {
	return m_thread;
}

// Running until the thread's function has returned.
hkThread::Status hkThread::getStatus() {
	if (!m_thread) {
		return THREAD_NOT_STARTED;
	}
	return m_thread->spRunning->load() ? THREAD_RUNNING : THREAD_TERMINATED;
}
