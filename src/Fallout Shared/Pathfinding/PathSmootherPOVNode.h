#pragma once

#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"
#include "Reconstruction.hpp"

class PathSmootherPOV;

// A node of the path smoother's A* search: a point of view, the direction (the PDB's ANGLE_DIRECTION: none, counter
// clockwise, clockwise) the path goes around it and the radius it was actually passed at. The PDB's source module is
// PathSmootherPOV.cpp.
class PathSmootherPOVNode {
public:
	PathSmootherPOV* pPOV;
	ANGLE_DIRECTION Direction;
	float fActualRadius;
};
