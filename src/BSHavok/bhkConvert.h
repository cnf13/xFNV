#pragma once

// The length scales between game units and Havok's. Each is a static of every file that includes the header, as in the
// original.
namespace bhkConvert {

static const float fHkScaleSC = 10.0f;
// An 8-byte constant divided by a 4-byte 10.0.
static const float fHk2BSScaleSC = 69.99125671386719 / 10.0f;
// The file's own fHk2BSScaleSC inverted.
static const float fBS2HkScaleSC = 1.0f / fHk2BSScaleSC;

} // namespace bhkConvert
