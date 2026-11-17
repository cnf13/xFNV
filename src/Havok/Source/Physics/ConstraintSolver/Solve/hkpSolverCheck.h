#pragma once

#include <stdint.h>

// Checks the physics key code: 0 with *apSimulationPossible set when it is valid, 3 with it set when it expires within
// ten days, 1 (invalid) or 2 (expired) with it clear and an error message.
int32_t hkpProcessFlyingColors(uint8_t* flyingColors);
