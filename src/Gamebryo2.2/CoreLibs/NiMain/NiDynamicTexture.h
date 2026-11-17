#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Reconstruction.hpp"

// A texture whose pixels the CPU writes; the renderer keeps its own copy in RendererData. Only the members called so
// far; the rest is not reconstructed yet.
class NiDynamicTexture : public NiTexture {
public:
	// The tiled flag, which the exe reads inline (NiDynamicTexture.inl's accessor).
	bool GetTiled() const { return m_bTiled; }

	uint32_t m_uiWidth;
	uint32_t m_uiHeight;
	bool m_bTiled;
};
