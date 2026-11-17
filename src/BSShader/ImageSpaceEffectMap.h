#pragma once

#include "BSShader/ImageSpaceEffect.h"

class ImageSpaceManager;

// The local-map image-space effect. It adds no fields to ImageSpaceEffect.
class ImageSpaceEffectMap : public ImageSpaceEffect {
public:
	ImageSpaceEffectMap();

	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	// Whether its one effect is active.
	bool IsActive() override;
	// The pixel constant from the second texture's size.
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;
	~ImageSpaceEffectMap() override;

	// Set while the local map renders for saving to disk (the larger map textures).
	static inline bool bSaveMode;
};
