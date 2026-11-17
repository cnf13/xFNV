#pragma once

#include "Reconstruction.hpp"

// The ray cast jobs' result slot helper over a command type and an output type (the SDK's inline
// getNextFreeResult). Statics only: sizeof 1 in Havok's release hkpcollide.pdb; the game PDB has getNextFreeResult for
// <hkpShapeRayCastCommand, hkpWorldRayCastOutput> and <hkpWorldRayCastCommand, hkpWorldRayCastOutput>.
template <typename COMMAND, typename OUTPUT> class hkpRayCastJobUtil {};
