#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"

// The standard-C++ thread hkThread keeps in its handle's slot
// (hkWin32Thread.cpp).
struct hkThreadHandle;

// Havok's Win32 thread: the thread handle and its id.
class hkThread {
public:
	enum : int32_t {
		HK_THREAD_DEFAULT_STACKSIZE = 0x0,
	};

	enum Status : int32_t {
		THREAD_TERMINATED = 0x0,
		THREAD_RUNNING = 0x1,
		THREAD_NOT_STARTED = 0x2,
	};

	hkThread();
	~hkThread();

	// Closes the thread handle and clears it.
	void stopThread();
	// Starts func(arg) on a new thread with stackSize; 0 on success, 1 on failure. func is passed to CreateThread as it
	// is. The thread-naming exception 0x406D1388 is not raised, because it needs a SEH __try/__except that swallows
	// exactly that code, which cannot be expressed on x64 Linux, and without it the raise would be unhandled.
	hkResult startThread(void* (*func)(void*), void* arg, const char* name, int32_t stackSize);
	void* getHandle();
	hkThread::Status getStatus();

	// The thread HANDLE.
	hkThreadHandle* m_thread;
	// There is no member between the handle and m_threadId; hkThread declares only these two.
	uint64_t m_threadId;
};
