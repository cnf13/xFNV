#include "Fallout Shared/Pathfinding/NavMeshSearchSLPoint.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestSafeStraightLine.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

NavMeshSearchSLPoint::NavMeshSearchSLPoint() {}

NavMeshSearchSLPoint::~NavMeshSearchSLPoint() {}

bool NavMeshSearchSLPoint::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
	NiPoint3 triCenter;
	NiPoint3 circumCenter;
	NavMesh* pmesh = apNode->tNodeData.pNavMesh;
	uint16_t itriIndex = apNode->tNodeData.sTriangle;
	NiPoint3 targetLoc = TargetLocation.GetLocation();
	NiPoint3 nodeToTargetVec = targetLoc;
	pmesh->GetCircumCenter2D(itriIndex, circumCenter);
	nodeToTargetVec.operator-=(circumCenter);
	float fdistanceSqr = nodeToTargetVec.SqrXYLength();
	NavMeshTriangle* Triangle = pmesh->GetTriangle(itriIndex);
	uint16_t Vert = Triangle->GetVert(0);
	NiPoint3 circumCenterToVert0Vec = pmesh->GetVertex(Vert)->Location;
	circumCenterToVert0Vec.operator-=(circumCenter);
	float ftriangleRadius = circumCenterToVert0Vec.Length();
	float fradii = fMaxValidRadius + ftriangleRadius;
	float fradiiSqr = fradii * fradii;
	if (fradiiSqr >= static_cast<double>(fdistanceSqr)) {
		NiPoint3 validPoint;
		float fminCheckRadii = fMinValidRadius - ftriangleRadius;
		if (fminCheckRadii > 0.0 && fdistanceSqr < fminCheckRadii * fminCheckRadii) {
			return 0;
		}
		apNode->tNodeData.GetCenter(&triCenter);
		if (CheckPointOnGoalCircle(triCenter, targetLoc, pmesh, validPoint)) {
			GoalLocation = validPoint;
			return 1;
		}
		if (fActorRadius < static_cast<double>(ftriangleRadius)) {
			// The exe keeps the three vertex pointers side by side and indexes them ([ebp+eax*4-0xA0]).
			const NavMeshVertex* apVerts[3];
			pmesh->GetTriangleVertices(itriIndex, apVerts[0], apVerts[1], apVerts[2]);
			NiPoint3 targetToVertVec = apVerts[0]->Location;
			targetToVertVec.operator-=(targetLoc);
			float fmaxDotLen = targetToVertVec.Dot(RightUnitVector);
			float fminDotLen = fmaxDotLen;
			for (int32_t j = 1; j < 3; ++j) {
				targetToVertVec = apVerts[j]->Location;
				targetToVertVec.operator-=(targetLoc);
				float fdotLen = targetToVertVec.Dot(RightUnitVector);
				if (!(fmaxDotLen < static_cast<double>(fdotLen))) {
					if (fminDotLen > static_cast<double>(fdotLen)) {
						fminDotLen = fdotLen;
					}
				} else {
					fmaxDotLen = fdotLen;
				}
			}
			NiPoint3 translatedStartTestPoint = TestStartPoint + RightUnitVector * fminDotLen;
			NiPoint3 translatedEndTestPoint = TestStartPoint + RightUnitVector * fmaxDotLen;
			NiPoint3 translatedTestVec = translatedEndTestPoint;
			translatedTestVec.operator-=(translatedStartTestPoint);
			float fvecLen = translatedTestVec.UnitizeGetLength();
			float fsamples = fvecLen / fActorRadius;
			float fcount = std::ceilf(fsamples);
			int32_t icount = static_cast<int32_t>(fcount);
			float fsampleDistance = fvecLen / fcount;
			float fsamplePoint = fsampleDistance * 0.5;
			NiPoint3 testPoint;
			for (int32_t i = 0; i < icount; ++i) {
				testPoint = translatedTestVec * fsamplePoint;
				testPoint.operator+=(translatedStartTestPoint);
				if (CheckPointOnGoalCircle(testPoint, targetLoc, pmesh, validPoint)) {
					GoalLocation = validPoint;
					return 1;
				}
				fsamplePoint = fsamplePoint + fsampleDistance;
			}
		}
	}
	return 0;
}

void NavMeshSearchSLPoint::GetGoalLocation(NiPoint3& aOut) const {
	aOut = GoalLocation;
}

// The exe's cleanup-only C++ EH frame is left out.
bool NavMeshSearchSLPoint::FindStraightLinePath(PathingRequestSafeStraightLine* apRequest,
	NavMeshSearchNode& aStartNode, NavMeshSearchNode& aGoalNode) {
	using links_t = BSScrapArray<NavMeshSearchLink, 1024>;
	FNVD_INLINED_LOCAL(links_t, links, std::nullopt, std::nullopt);
	TargetLocation.operator=(*&apRequest->Destination);
	fTargetRadius = apRequest->fTargetRadius;
	double LastFadePercent = apRequest->fMaxDistance;
	fMaxValidRadius = apRequest->fTargetRadius + LastFadePercent;
	double MinDistance = apRequest->fMinDistance;
	fMinValidRadius = apRequest->fTargetRadius + MinDistance;
	fActorRadius = apRequest->fActorRadius;
	using nodes_t = BSScrapArray<NavMeshSearchNode, 1024>;
	FNVD_CONSTRUCTED_LOCAL(nodes_t, nodes, &nodes_t::Destruct, 32u, 0u, static_cast<ScrapHeap*>(0));
	::new (&links) BSScrapArray<NavMeshSearchLink, 1024>(32u, 0u, static_cast<ScrapHeap*>(0));
	bool v8 = BuildNodePath(apRequest, aStartNode, aGoalNode, &nodes, &links, 0);
	links.~BSScrapArray<NavMeshSearchLink, 1024>();
	return v8;
}

bool NavMeshSearchSLPoint::BuildPathToSafeStraightLine(PathingRequestSafeStraightLine* apRequest,
	NavMeshSearchNode& aStartNode, NavMeshSearchNode& aGoalNode, BSSimpleArray<NavMeshSearchNode, 1024>* apNodes,
	BSSimpleArray<NavMeshSearchLink, 1024>* apLinks) {
	PathingLocation* LocalTransform = static_cast<PathingLocation*>(&apRequest->GetDestination());
	TargetLocation.operator=(*LocalTransform);
	fTargetRadius = apRequest->GetTargetRadius();
	double LastFadePercent = apRequest->GetMaxDistance();
	fMaxValidRadius = apRequest->GetTargetRadius() + LastFadePercent;
	double MinDistance = apRequest->GetMinDistance();
	fMinValidRadius = apRequest->GetTargetRadius() + MinDistance;
	fActorRadius = apRequest->GetActorRadius();
	return BuildNodePath(apRequest, aStartNode, aGoalNode, apNodes, apLinks, 0);
}

// The exe's cleanup-only C++ EH frame is left out.
bool NavMeshSearchSLPoint::CheckPointOnGoalCircle(NiPoint3& aCheck, NiPoint3& aTarget, NavMesh* apMesh,
	NiPoint3& aOutValidPoint) {
	NavMesh* v11;
	uint16_t v12;
	NiPoint3 v21;
	NiPoint2 v24;
	NiPoint3 a2;
	bool IsPointInsideTriangle;
	uint16_t arTriangle[3];
	FNVD_INLINED_LOCAL(NavMeshPtr, v34, std::nullopt, std::nullopt);
	float arZDiff;
	FNVD_INLINED_LOCAL(PathingLocation, v36, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, aGoalLocation, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, apMemory, std::nullopt, std::nullopt);
	NiPoint3 arLocatiom;
	FNVD_INLINED_LOCAL(PathingLocation, v40, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, v41, std::nullopt, std::nullopt);
	NiPoint3 pt;
	FNVD_INLINED_LOCAL(PathingLocation, apLocation, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(PathingLocation, aStartLocation, std::nullopt, std::nullopt);
	NiPoint3 v47 = aCheck;
	v47.operator-=(aTarget);
	float v49 = v47.Length();
	if (v49 < 9.999999747378752e-05) {
		v47 = aCheck;
		v47.x = v47.x + 10.0;
		v47.operator-=(aTarget);
		v49 = v47.Length();
	}
	float v42 = fMaxValidRadius / v49;
	float v45 = (fMaxValidRadius + fActorRadius) / v49;
	arLocatiom = v47 * v42;
	TestStartPoint = v47 * v45;
	NiPoint3 aLocation = aTarget;
	arLocatiom.operator+=(aTarget);
	TestStartPoint.operator+=(aTarget);
	RightUnitVector = v47.UnitCross(NiPoint3_UNIT_Z);
	pt = RightUnitVector * fActorRadius;
	NavMeshInfo* NavMeshInfo_ = apMesh->GetNavMeshInfo();
	::new (&aStartLocation) PathingLocation(&TestStartPoint, NavMeshInfo_);
	::new (&aGoalLocation) PathingLocation(aLocation, TargetLocation);
	if (!Pathing::CheckLineOfSight(aStartLocation, aGoalLocation, static_cast<PathingLocation*>(0),
			static_cast<Pathing::CheckLineOfSightFilter*>(0))) {
		aGoalLocation.~PathingLocation();
		aStartLocation.~PathingLocation();
		return 0;
	}
	::new (&apMemory) PathingLocation(TestStartPoint - pt, aStartLocation);
	::new (&apLocation) PathingLocation(aLocation - pt, aGoalLocation);
	if (!Pathing::CheckLineOfSight(apMemory, apLocation, static_cast<PathingLocation*>(0),
			static_cast<Pathing::CheckLineOfSightFilter*>(0))) {
		apLocation.~PathingLocation();
		apMemory.~PathingLocation();
		aGoalLocation.~PathingLocation();
		aStartLocation.~PathingLocation();
		return 0;
	}
	::new (&v41) PathingLocation(TestStartPoint + pt, aStartLocation);
	::new (&v40) PathingLocation(aLocation + pt, aGoalLocation);
	if (!Pathing::CheckLineOfSight(v41, v40, static_cast<PathingLocation*>(0),
			static_cast<Pathing::CheckLineOfSightFilter*>(0))) {
		v40.~PathingLocation();
		v41.~PathingLocation();
		apLocation.~PathingLocation();
		apMemory.~PathingLocation();
		aGoalLocation.~PathingLocation();
		aStartLocation.~PathingLocation();
		return 0;
	}
	::new (&v36) PathingLocation(arLocatiom, aStartLocation);
	::new (&v34) NavMeshPtr;
	if (!v36.GetNavMeshAndTriangle(v34, arTriangle[0])) {
		goto LABEL_15;
	}
	v12 = arTriangle[0];
	v11 = v34.m_pObject;
	IsPointInsideTriangle = v11->IsPointInsideTriangle(v12, arLocatiom, arZDiff, 180.0f, 180.0f);
	if (arZDiff < 0.0 && aLocation.z - TestStartPoint.z > 0.0) {
		float v13;
		float v22;
		float v23;
		float v26;
		float v27;
		float v28;
		float v29;
		a2 = aTarget;
		a2.operator-=(arLocatiom);
		v29 = a2.Length();
		v27 = v29 - fTargetRadius - fActorRadius;
		v13 = v27 / v29;
		a2.operator*=(v13);
		v21 = arLocatiom + a2;
		::new (&v24) NiPoint2(a2.x, a2.y);
		v26 = v24.Unitize();
		v23 = v27 * v27;
		v22 = v21.z - (arLocatiom.z + arZDiff);
		v22 = v22 * v22;
		if (v23 < static_cast<double>(v22) || (v28 = v23 - v22, (v28 = NiSqrt(v28), v26 <= static_cast<double>(v28)))) {
		LABEL_15:;
			FNVD_DESTRUCT(NiPointer<NavMesh>)(&v34);
			v36.~PathingLocation();
			v40.~PathingLocation();
			v41.~PathingLocation();
			apLocation.~PathingLocation();
			apMemory.~PathingLocation();
			aGoalLocation.~PathingLocation();
			aStartLocation.~PathingLocation();
			return 0;
		}
		float v25 = v26 - v28;
		v24.operator*=(v25);
		arLocatiom.x = arLocatiom.x + v24.x;
		arLocatiom.y = arLocatiom.y + v24.y;
	}
	arLocatiom.z = arLocatiom.z + arZDiff;
	aOutValidPoint = arLocatiom;
	FNVD_DESTRUCT(NiPointer<NavMesh>)(&v34);
	v36.~PathingLocation();
	v40.~PathingLocation();
	v41.~PathingLocation();
	apLocation.~PathingLocation();
	apMemory.~PathingLocation();
	aGoalLocation.~PathingLocation();
	aStartLocation.~PathingLocation();
	return 1;
}

NiPoint2& NiPoint2::operator*=(float fScalar) {
	x = x * fScalar;
	y = y * fScalar;
	return *this;
}
