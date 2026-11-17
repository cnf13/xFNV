#pragma once

class NiTexture;

// Keeps the previous texture bindings for the instance renderer's texture stages.
class BSInstanceRendering {
public:
	BSInstanceRendering();
	~BSInstanceRendering();

	NiTexture** pPrevTextures;
};
