#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"

#include <stdint.h>

// A place to avoid that comes from a navmesh triangle (a PathingAvoidNode with the triangle's index).
class NavMeshStaticAvoidNode : public PathingAvoidNode {
public:
	NavMeshStaticAvoidNode();
	NavMeshStaticAvoidNode(const NiPoint3& aCenter, uint16_t ausTriangle, float afRadius, float afCost);

	uint16_t usTriangle;
};
