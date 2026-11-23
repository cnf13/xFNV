#pragma once

#include <stdint.h>

// The PC renderer's static shutdown interface; it has no instance data.
class NiDX9RendererSDM {
public:
    // The first instance registers the library's init and shutdown.
    NiDX9RendererSDM();

    static void Shutdown();
    static void Init();

    static inline char ms_bInitialized;
    static inline int32_t siCounter;
};
