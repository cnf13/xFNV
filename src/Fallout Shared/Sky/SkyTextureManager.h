#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiTexture;

// Loads the sky's (and the screen splatters') textures through the IO manager. Every method is static.
class SkyTextureManager {
public:
	// IDA's RequestTexture. Queues a SkyTask that loads apFilename (standardized as a
	// texture path) into *apspTexture (cdecl, ret 0, 0x63FB70).
	static void LoadTextureInBackground(NiPointer<NiTexture>* apspTexture, const char* apFilename);
};
