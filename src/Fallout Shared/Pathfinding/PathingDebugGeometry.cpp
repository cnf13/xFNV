#include "BSMain/BSSpline.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingSpline.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

PathingSplineKey PathingNode::GetSplineKey() {
	PathingSplineKey result;
	NiPoint3 resulta;
	PathingSplineKey apMemory;
	apMemory.Location = (resulta = Location.GetLocation());
	apMemory.Tangent = Tangent;
	result.operator=(apMemory);
	return result;
}

PathingSplineKey::PathingSplineKey() : Location(NiPoint3_ZERO), Tangent(NiPoint3_ZERO) {}

PathingSplineKey& PathingSplineKey::operator=(const PathingSplineKey& aKey) {
	Location = aKey.Location;
	Tangent = aKey.Tangent;
	return *this;
}

NiPoint3 PathingSpline::InterpolateNoZ(const PathingSplineKey& aKey1, const PathingSplineKey& aKey2) {
	NiPoint3 result;
	NiPoint3 retVec;
	retVec.x = Spline.Interpolate<float>(aKey1.Location.x, aKey2.Location.x, aKey1.Tangent.x, aKey2.Tangent.x);
	retVec.y = Spline.Interpolate<float>(aKey1.Location.y, aKey2.Location.y, aKey1.Tangent.y, aKey2.Tangent.y);
	float arEndTangent = 0.0f;
	float arStartTangent = 0.0f;
	retVec.z = Spline.Interpolate<float>(aKey1.Location.z, aKey2.Location.z, arStartTangent, arEndTangent);
	result = retVec;
	return result;
}

NiPoint3 PathingSpline::Interpolate(const PathingSplineKey& aKey1, const PathingSplineKey& aKey2) {
	NiPoint3 result;
	Key1.operator=(aKey1);
	Key2.operator=(aKey2);
	result = InterpolateNoZ(aKey1, aKey2);
	return result;
}

PathingNode* PathingSolution::GetFirstNode() {
	if (CurrentPathingNodes.IsEmpty())
		return nullptr;
	return &CurrentPathingNodes.GetFirst();
}
