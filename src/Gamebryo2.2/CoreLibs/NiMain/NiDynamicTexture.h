#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"

// A texture whose pixels the CPU writes; the renderer keeps its own copy in RendererData. Only some of its members are
// declared.
class NiDynamicTexture : public NiTexture {
public:
	bool GetTiled() const { return m_bTiled; }

	uint32_t m_uiWidth;
	uint32_t m_uiHeight;
	bool m_bTiled;
};
