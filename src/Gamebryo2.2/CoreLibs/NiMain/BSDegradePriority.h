#pragma once

#include "Reconstruction.hpp"

// A texture's degrade state (the PDB's; NiTexture::QState returns it as a whole dword, 0x43C467).
enum PRIORITY_STATE : int32_t {
	PS_NONE = 0,
	PS_DEGRADING = 1,
	PS_UPGRADING = 2,
	PS_DEGRADED = 3,
	PS_UPGRADED = 4,
};

// Bethesda's texture degrade priority: the low, high and current priority and the degrade state.
class BSDegradePriority {
public:
	// cState, through the exe's shared one-byte getter (thiscall, 0x43C470, zero-extended into eax).
	PRIORITY_STATE QState();

	uint8_t cPriorityL;
	uint8_t cPriorityH;
	uint8_t cPriorityC;
	// A PRIORITY_STATE, stored as a byte.
	uint8_t cState;
};
