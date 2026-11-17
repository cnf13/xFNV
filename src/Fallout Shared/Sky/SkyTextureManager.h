#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiTexture;

// Loads the sky's (and the screen splatters') textures through the IO manager. Every method is static.
class SkyTextureManager {
public:
    // Queues a SkyTask that loads apFilename (standardized as a texture path) into *apspTexture.
    static void LoadTextureInBackground(NiPointer<NiTexture>* apspTexture, const char* apFilename);
};
