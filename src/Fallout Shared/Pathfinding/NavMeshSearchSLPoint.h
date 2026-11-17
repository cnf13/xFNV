#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class NavMesh;
class PathingRequestSafeStraightLine;

// The navmesh search for a straight-line point (FindStraightLinePath, CheckPointOnGoalCircle): the target location,
// the goal and the radii it is tested against. The PDB's lists of tested locations (TestedStartLocations ..
// TestedProjectedTriangle) have no PC fields.
class NavMeshSearchSLPoint : public NavMeshSearch {
public:
	NavMeshSearchSLPoint();
	~NavMeshSearchSLPoint() override;

	bool IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) override;

	void GetGoalLocation(NiPoint3& aOut) const;
	bool FindStraightLinePath(PathingRequestSafeStraightLine* apRequest, NavMeshSearchNode& aStartNode,
		NavMeshSearchNode& aGoalNode);
	bool BuildPathToSafeStraightLine(PathingRequestSafeStraightLine* apRequest, NavMeshSearchNode& aStartNode,
		NavMeshSearchNode& aGoalNode, BSSimpleArray<NavMeshSearchNode, 1024>* apNodes,
		BSSimpleArray<NavMeshSearchLink, 1024>* apLinks);
	bool CheckPointOnGoalCircle(NiPoint3& aCheck, NiPoint3& aTarget, NavMesh* apMesh, NiPoint3& aOutValidPoint);

	PathingLocation TargetLocation;
	NiPoint3 GoalLocation;
	NiPoint3 RightUnitVector;
	NiPoint3 TestStartPoint;
	float fMinValidRadius;
	float fMaxValidRadius;
	float fTargetRadius;
	float fActorRadius;
	float fZDelta;
};
inline constexpr fnvd::Vtable NavMeshSearchSLPoint_vftable{};
