#pragma once

#include "Reconstruction.hpp"

// The static data manager of the BSMain library: InitSDM runs the other BSMain classes' tag setup (BSAttachPoints,
// BSBound, BSXFlags ...) once. Everything in it is static.
class BSMain {
public:
	// The exe uses no this (cdecl, ret 0, 0xC5D4E0); the PDB's BSMain is a namespace of free functions.
	static void InitSDM();
	// The exe uses no this (cdecl, ret 0, 0xC5D520).
	static void KillSDM();
};
