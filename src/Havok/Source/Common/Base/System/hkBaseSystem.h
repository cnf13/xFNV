#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Object/hkSingleton.h"
#include "Havok/Source/Common/Base/System/Error/hkError.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkHardwareInfo;

void showHavokBuild();
// The SDK's hardware query: m_numThreads is the processor count, at most 4.
void hkGetHardwareInfo(hkHardwareInfo& info);
// Havok SDK signature (hkBase.lib, tied by data).
hkReferencedObject* hkDummySingletoncreate();

// Havok's base system: static init/quit of the library and its per-thread state (init, initSingletons, initThread,
// isInitialized, quit, quitSingletons, quitThread). It has no instances and no data; its state lives in globals.
class hkBaseSystem {
public:
    // Initialises Havok on the main thread with apMemoryRouter, reporting errors through
    // apErrorReportFunction with apErrorReportObject. Returns an hkResult, spelled int32_t.
    static hkResult
    init(hkMemoryRouter* memoryRouter, hkErrorReportFunction errorReportFunction, void* errorReportObject);
    // Sets up the calling thread: its memory router, then its monitor stream. Returns HK_SUCCESS (0), an hkResult
    // spelled as int32_t.
    static hkResult initThread(hkMemoryRouter* memoryRouter);
    // Frees the calling thread's monitor stream and detaches its memory router. Returns HK_SUCCESS (0).
    static hkResult quitThread();
    // Creates every registered singleton not created yet. The ones whose create function returns null are unlinked
    // and retried, the last failed one first, until every one exists; each is linked back in as it succeeds.
    static void initSingletons();
    // Releases every registered singleton, the list's tail first, and clears its instance pointer.
    static void quitSingletons();
    // Quits the library once initialised: unlocks references, drops the singletons and the
    // two global referenced objects, quits the calling thread. Returns HK_SUCCESS (0).
    static hkResult quit();
    // Whether hkBaseSystem::init has run: writes the flag into apResult and returns it.
    static hkBool* isInitialized(hkBool* apResult);
};

// A singleton of hkBaseSystem.cpp that holds nothing: a bare hkReferencedObject with its
// own vtable, which its create function allocates as 8 bytes.
class hkDummySingleton : public hkReferencedObject, public hkSingleton<hkDummySingleton> {
public:
    // Empty: hkBaseSystem::init calls it through the vtable so that the linker keeps the class.
    virtual void forceLinkage() {}

    // hkSingleton<hkDummySingleton>::s_instance.
    static inline hkDummySingleton* s_instance;
};

// Whether hkBaseSystem::init has run (hkBaseSystem::isInitialized reads it).
inline hkBool hkBaseSystemIsInitialized{false};
using s_platformNetQuit_t = void (*)();
extern s_platformNetQuit_t s_platformNetQuit;
