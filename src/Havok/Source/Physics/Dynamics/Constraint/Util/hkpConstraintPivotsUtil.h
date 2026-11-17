#pragma once

#include "Reconstruction.hpp"

// Reads and sets the pivots of a constraint's data (getPivotA, constraintSupportsPivotGetSet, findClosestShapeKey and
// the rest). Statics only: sizeof 1 in Havok's release hkpdynamics.pdb; the game PDB has seven of its functions.
class hkpConstraintPivotsUtil {};
