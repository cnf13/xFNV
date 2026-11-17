#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class NiD3DShaderProgramCreator;

// The PC renderer's static shutdown interface; it has no instance data.
// The PDB has no source path for this PC-only type; the renderer family determines its location.
class NiDX9RendererSDM {
public:
	// The first instance registers the library's init and shutdown.
	NiDX9RendererSDM();

	static void Shutdown();
	static void Init();

	static inline char ms_bInitialized;
	static inline int32_t siCounter;
};

// IDA's name is a type's; the address is added to it.
inline NiD3DShaderProgramCreator* NiD3DShaderProgramCreatorObj_126FA54;
