#pragma once

#include "Reconstruction.hpp"

// The kd-tree's vector helpers (inline statics: ray-box tests, projected entry to vector and box conversions). No
// data: sizeof 1 in Havok's release hkinternal.pdb; the game PDB has its convertProjectedEntryToHkVector4 and
// getAabbFromProjectedEntry.
class hkKdTreeMath {};
