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
    // Starts func(arg) on a new thread named name; HK_SUCCESS once it runs. stackSize is not applied.
    hkResult startThread(void* (*func)(void*), void* arg, const char* name, int32_t stackSize);
    void* getHandle();
    hkThread::Status getStatus();

    // The thread HANDLE.
    hkThreadHandle* m_thread;
    // There is no member between the handle and m_threadId; hkThread declares only these two.
    uint64_t m_threadId;
};
