#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

// Checks the animation key code: 0 with *apSimulationPossible set when it is valid, 3 with it set when it expires
// within ten days, 1 (invalid) or 2 (expired) with it clear and an error message.
int32_t hkaProcessFlyingColors(uint8_t* flyingColors);

// The SDK's header static (hkaProcessFlyingColors.h; release hkaInternal.lib has a file-local one-byte .bss
// `_HK_flyingcolors_animation` in hkaExpandReal.obj and in SignedQuaternion.obj): each file's key-code result,
// passed to hkaProcessFlyingColors. The SDK header is not in resources/havok: the type is the .obj's one byte, as
// hkaProcessFlyingColors's uint8_t* parameter takes it.
static uint8_t HK_flyingcolors_animation = 0;
