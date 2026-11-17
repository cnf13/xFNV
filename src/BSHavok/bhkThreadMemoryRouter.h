#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSHavok/HavokThreadMemoryStruct.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// Sets up and tears down a thread's Havok memory router.
class bhkThreadMemoryRouter {
public:
    // Creates apData's memory router and stack buffer when it has none, and installs them
    // for the calling thread.
    static void Initialize(HavokThreadMemoryStruct* apData);
    // Releases apData's memory router and stack buffer.
    static void Quit(HavokThreadMemoryStruct* apData);

    // Guards the routers' creation across threads.
    static BSCriticalSection kCriticalSection;
};
