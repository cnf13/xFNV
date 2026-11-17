#include "BSCore/BSThread.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/MemoryManager.h"

void BSSleep(uint32_t auiMSec) {
	Sleep(auiMSec);
}

BSThread::BSThread()
	: m_ThreadHandle(nullptr), m_ParentHandle(nullptr), m_ThreadID(0), m_ParentID(0), bThreadIsActive(false) {}

BSThread::~BSThread() {
	Close();
}

uint32_t BSThread::ThreadProc() {
	return 0;
}

// The stack size is not applied (BSSystemThread): every size the game asks for is at most the default's 1 MB reserve.
void BSThread::Initialize(StackSize aeStackSize, const char* apName, bool bSuspend) {
	if (!m_ThreadHandle) {
		// The exe stores GetCurrentThread()'s pseudo-handle here; nothing reads it.
		m_ParentHandle = nullptr;
		m_ParentID = GetCurrentThreadId();
		m_ThreadHandle = new BSSystemThread(&BSThread::Exit, this, bSuspend);
		m_ThreadID = m_ThreadHandle->Id();
		EnterCriticalSection(&CriticalSection.CriticalSection);
		SetThreadPriority(-1);
		bThreadIsActive = true;
		m_ThreadHandle->Resume();
		LeaveCriticalSection(&CriticalSection.CriticalSection);
	}
}

// Updates the thread priority only when a thread exists.
void BSThread::SetThreadPriority(int32_t aiPriority) {
	if (m_ThreadHandle) {
		m_ThreadHandle->SetPriority(aiPriority);
	}
}

// The PC body ignores the Xbox-only name argument.
void BSThread::SetName(const char* apName) {
	static_cast<void>(apName);
}

// The PC body ignores the Xbox-only processor argument.
void BSThread::SetThreadProcessor(uint32_t aiThread) {
	static_cast<void>(aiThread);
}

bool BSThread::QIsThreadActive() {
	return bThreadIsActive;
}

void BSThread::Close() {
	if (m_ThreadHandle) {
		bThreadIsActive = 0;
		EnterCriticalSection(&CriticalSection.CriticalSection);
		delete m_ThreadHandle;
		m_ThreadHandle = nullptr;
		LeaveCriticalSection(&CriticalSection.CriticalSection);
	}
}

// Dispatches the Win32 thread entry to the subclass's virtual thread procedure: apThread is the BSThread Initialize
// passes to CreateThread.
uint32_t BSThread::Exit(void* apThread) {
	return static_cast<BSThread*>(apThread)->ThreadProc();
}
