#pragma once

#include <cstdint>

class NiInitOptions;

// Runs the registered libraries' static data init and shutdown functions. Only some of its members are declared.
class NiStaticDataManager {
public:
    typedef void (*InitFunction)();
    typedef void (*ShutdownFunction)();
    // The tables hold 16 entries.
    using InitFunctionArray = InitFunction[16];
    using ShutdownFunctionArray = ShutdownFunction[16];

    // Sets the root library's init and shutdown functions.
    static void SetRootLibrary(InitFunction pfnInit, ShutdownFunction pfnShutdown);
    // Appends a library's init and shutdown functions; runs the init at once when Init has already run.
    static void AddLibrary(InitFunction pfnInit, ShutdownFunction pfnShutdown);
    // Runs the system, root and added libraries' init functions; makes default init options when none are given.
    static void Init(const NiInitOptions* pkOptions);
    // Runs the shutdown functions and frees the init options Init made.
    static void Shutdown();

    static inline InitFunction ms_pfnRootInitFunction{};
    static inline ShutdownFunction ms_pfnRootShutdownFunction{};
    static inline InitFunctionArray ms_apfnInitFunctions{};
    static inline ShutdownFunctionArray ms_apfnShutdownFunctions{};
    static inline uint32_t ms_uiNumLibraries;
    static inline bool ms_bInitialized;
    static inline bool ms_bAutoCreatedInitOptions;
    static inline const NiInitOptions* ms_pkInitOptions{};
};
