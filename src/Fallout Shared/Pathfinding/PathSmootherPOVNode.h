#pragma once

#include "Fallout Shared/Pathfinding/AngleSinCosArc.h"

class PathSmootherPOV;

// A node of the path smoother's A* search: a point of view, the direction (none, counter clockwise, clockwise) the path
// goes around it and the radius it was actually passed at.
class PathSmootherPOVNode {
public:
	PathSmootherPOV* pPOV;
	ANGLE_DIRECTION Direction;
	float fActualRadius;
};
