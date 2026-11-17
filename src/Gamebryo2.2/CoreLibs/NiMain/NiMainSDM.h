#pragma once

#include <stdint.h>

void FreeMemoryBlocks();

// The static data manager of the main library: Init sets up every NiMain class's static data once, Shutdown tears it
// down. Everything in it is static.
class NiMainSDM {
public:
	NiMainSDM();

	static void Init();
	static void Shutdown();

	// Whether Init has run (tested and set by Init).
	static inline bool ms_bInitialized;
};

inline int32_t siCounter;
inline void* apMem;
inline int32_t uiMemBlockCount;
