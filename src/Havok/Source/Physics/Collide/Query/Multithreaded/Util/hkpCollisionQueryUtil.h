#pragma once

#include "Reconstruction.hpp"

// The shape ray cast job's result slot helper (the SDK's inline getNextFreeResult). Statics only: sizeof 1 in Havok's
// release hkpcollide.pdb; the game PDB has its getNextFreeResult.
class hkpShapeRayCastJobUtil {};
