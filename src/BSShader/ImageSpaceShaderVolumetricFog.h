#pragma once

#include "BSShader/ImageSpaceShader.h"

class ImageSpaceShaderVolumetricFog : public ImageSpaceShader {
public:
	ImageSpaceShaderVolumetricFog();
	~ImageSpaceShaderVolumetricFog() override;

	void ReloadShaders() override;
	void SetState() override;
	// The ImageSpaceEffect IsActive override: active while texture slot 2 holds a texture.
	bool IsActive() override;

	BSRenderedTexture* GetTexture();
	// Returns the rendered texture of texture slot 2 to the texture manager. Its callers are this shader's destructor
	// and RenderVolumetricFog, and it reads pTextures.m_pBase.
	void ReturnVolumetricFogTexture();
};
