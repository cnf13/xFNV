#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Reconstruction.hpp"

// The static data manager of the particle library: it registers the stream loaders of every particle class at
// start-up and removes them at shut-down. Everything in it is static.
class NiParticleSDM {
public:
	NiParticleSDM();

	// Registers the stream loaders of the particle classes once.
	static void Init();
	// Unregisters the stream loaders Init registered.
	static void Shutdown();

	// Whether the loaders are registered.
	static inline bool ms_bInitialized;
	// How many NiParticleSDM objects were constructed (IDA name; not in the PDB).
	static inline uint32_t siCounter;
};

// The library's static object (NiParticle.h in Gamebryo's source): one per file that includes this header, as in the
// original; its constructor runs NiParticleSDM's Init the first time.
static NiParticleSDM NiParticleSDMObject{};
