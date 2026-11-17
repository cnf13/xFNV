#pragma once

#include <stdint.h>

class NiD3DShaderProgramCreator;

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

inline NiD3DShaderProgramCreator* NiD3DShaderProgramCreatorObj_126FA54;
