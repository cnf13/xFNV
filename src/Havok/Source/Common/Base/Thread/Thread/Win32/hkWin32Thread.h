#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

// The standard-C++ thread hkThread keeps in its handle's slot
// (hkWin32Thread.cpp).
struct hkThreadHandle;

// Havok's Win32 thread: the thread (the exe's handle) and its id.
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
	// Starts func(arg) on a new thread with stackSize and names it for the debugger; 0 on success, 1 on failure
	// (thiscall, ret 0x10, 0xD87940). func is the SDK's cdecl StartFunc: the exe passes it to CreateThread as it is.
	// The exe raises the MSVC thread-naming exception 0x406D1388 inside a __try whose __except filter swallows
	// exactly that code; src cannot express SEH __try/__except (the standalone build is x64 Linux), and without it
	// the raise would be unhandled. The exe reuses the arg stack slot as the thread id out-parameter.
	hkResult startThread(void* (*func)(void*), void* arg, const char* name, int32_t stackSize);
	void* getHandle();
	hkThread::Status getStatus();

	// The exe's thread HANDLE.
	hkThreadHandle* m_thread;
	// The 4 bytes at +0x4 are the alignment hole before the 8-byte-aligned id: neither the PDB nor IDA has a member
	// between the handle and m_threadId, and the SDK's hkThread declares only these two.
	uint64_t m_threadId;
};
