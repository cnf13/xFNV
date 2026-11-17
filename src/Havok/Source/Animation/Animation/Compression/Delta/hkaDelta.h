#pragma once

#include <cstdint>

// Replaces every sample but the first with its difference from the sample before it, walking
// the array backwards. rawData holds nValues floats.
void hkDeltaTransform(float* rawData, uint32_t nValues);

// The delta coding of a compressed animation's scalar tracks: a track's samples are stored as the differences
// between consecutive samples.
