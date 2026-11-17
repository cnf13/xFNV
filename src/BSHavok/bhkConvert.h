#pragma once

#include "Reconstruction.hpp"

// bhkConvert (BSHavok\bhkConvert.h in the PDB's line tables): the length scales between game units and Havok's.
// Each is a static of every file that includes the header, as in the original: the exe computes the copies at
// startup, or stores them as data where its compiler folded the value.
namespace bhkConvert {

// The PDB's const float of every file that includes this header (the same 504 modules as fHk2BSScaleSC), in .rdata.
static const float fHkScaleSC = 10.0f;
// The exe's 8-byte constant over a 4-byte 10.0 (0xF35270).
static const float fHk2BSScaleSC = 69.99125671386719 / 10.0f;
// The file's own fHk2BSScaleSC inverted.
static const float fBS2HkScaleSC = 1.0f / fHk2BSScaleSC;

} // namespace bhkConvert
