#pragma once

#include "BSShader/ImageSpaceEffect.h"
#include "Reconstruction.hpp"

class ImageSpaceManager;

// The local-map image-space effect. It adds no fields to ImageSpaceEffect.
class ImageSpaceEffectMap : public ImageSpaceEffect {
public:
	ImageSpaceEffectMap();

	// The destructor (slot 0, deleting destructor 0xBAA220) is the implicit one.
	void Setup(ImageSpaceManager* apManager, ImageSpaceEffectParam* apParam) override;
	// Whether its one effect is active.
	bool IsActive() override;
	// The pixel constant from the second texture's size.
	bool UpdateParams(ImageSpaceEffectParam* apParam) override;
	~ImageSpaceEffectMap() override;

	// Set while the local map renders for saving to disk (the larger map textures).
	static inline bool bSaveMode;
};
inline constexpr fnvd::Vtable ImageSpaceEffectMap_vftable{};
