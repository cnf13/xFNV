#pragma once

#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"

// An AI worker thread with its own Havok memory router and stack buffer: it runs the task function it was given.
class AILinearTaskThread : public BSLinearTaskThread {
public:
    AILinearTaskThread(void (*apfnTaskFunc)());
    ~AILinearTaskThread() override;

    void OnStartup() override;
    void OnShutdown() override;
    void RunLinearTasks() override;

    // The task the thread runs each frame; it is called with no arguments.
    void (*pfnTaskFunc)();
    hkMemoryRouter ThreadMemory;
    void* pThreadStackBuffer;
};
