#pragma once

#include "Havok/Source/Common/Base/Types/hkResult.h"

// The interface the visual debugger's clients use to register, select, create and delete processes. Not a
// reference counted object. hkResult is spelled int32_t.
class hkProcessHandler {
public:
    // The commands the client sends to a handler (values from the SDK).
    enum Command : int32_t {
        HK_REGISTER_PROCESS = 0xC0,
        HK_SELECT_PROCESS = 0xC1,
        HK_CREATE_PROCESS = 0xC2,
        HK_DELETE_PROCESS = 0xC3,
        HK_PAUSE_WORLD_STEP = 0xC4,
        HK_UNPAUSE_WORLD_STEP = 0xC5,
    };

    virtual ~hkProcessHandler();

    virtual hkResult registerProcess(const char* name, int32_t id) = 0;
    virtual int32_t getProcessId(const char* name) = 0;
    virtual const char* getProcessName(int32_t id) = 0;
    virtual hkResult selectProcess(int32_t id) = 0;
    virtual hkResult createProcess(int32_t id) = 0;
    virtual hkResult deleteProcess(int32_t id) = 0;

    // DeletingDestruct's free (ourFree).
    static void operator delete(void* apMemory);
};
