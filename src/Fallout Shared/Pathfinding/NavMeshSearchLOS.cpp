#include "Fallout Shared/Pathfinding/NavMeshSearchLOS.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLOSGridMap.h"
#include "Fallout Shared/Pathfinding/PathingLOSMap.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequestLOS.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

NavMeshSearchLOS::NavMeshSearchLOS()
	: pCurrentRequest(nullptr), spLOSMap(nullptr), bHasGoalNode(false), cMinLOSScore(-3), pFirstIgnoredGoal(nullptr),
	  pLastValidNode(nullptr), pClosestNode(nullptr), fClosestNodeDistanceSquared(3.4028235e+38f) {}

NavMeshSearchLOS::~NavMeshSearchLOS() {}

// The exe's cleanup-only C++ EH frame is left out.
bool NavMeshSearchLOS::GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
	BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) {
	NiPoint3 v20;
	NiPoint3 result;
	NiPoint3 v23;
	DoorLock doorLock;
	AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pnode;
	NavMesh* potherMesh;
	int32_t iotherEdge;
	uint16_t usotherTri;
	NiPoint3 aNodeCenter;
	apNode->tNodeData.GetCenter(&aNodeCenter);
	for (int32_t i = 0; i < 3; ++i) {
		if (apNode->tNodeData.pNavMesh->GetMatchingEdge(apNode->tNodeData.sTriangle, i, &potherMesh, &usotherTri,
				&iotherEdge)) {
			EdgeExtraInfo* pextra = apNode->tNodeData.pNavMesh->GetEdgeExtraInfo(apNode->tNodeData.sTriangle, i);
			if (!pextra || pextra->eType != EIT_LEDGE_UP) {
				if (potherMesh == apNode->tNodeData.pNavMesh ||
					spLOSMap.m_pObject->IsNavMeshInMap(potherMesh->iFormID)) {
					NiPoint3* Center;
					NiPoint3* a2a;
					double Range;
					double v18;
					NiPoint3 vec;
					float fdoorMultiplier = 1.0f;
					NavMeshTriangle* Triangle = potherMesh->GetTriangle(usotherTri);
					if (Triangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
						if (pCurrentRequest->bCantOpenDoors) {
							continue;
						}
						uint32_t doorID = potherMesh->GetClosedDoorInfoForTriangle(usotherTri)->DoorFormID;
						TESObjectREFR* pdoor = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(doorID));
						if (pdoor) {
							bool bmustPick;
							doorLock.~DoorLock();
							::new (&doorLock) DoorLock(pdoor);
							const ActorLockData* lockData = &pCurrentRequest->LockData;
							if (!doorLock.CanActorUnlockDoorLock(lockData, pdoor, bmustPick)) {
								continue;
							}
							if (static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())
									->TESObjectDOOR::IsMinimalUse()) {
								fdoorMultiplier = 100.0f;
							}
						}
					}
					if (pCurrentRequest->fSearchCenterRadius == 0.0 ||
						(a2a = ::new (&result) NiPoint3(pCurrentRequest->GetSearchCenter()),
							(Center = ::new (&v23) NiPoint3(potherMesh->GetCenter(usotherTri)),
								(::new (&vec) NiPoint3(*Center - *a2a),
									(v18 = vec.SqrLength(),
										(Range = pCurrentRequest->fSearchCenterRadius,
											!(pCurrentRequest->fSearchCenterRadius * Range < v18))))))) {
						NavMeshTriangle* v7;
						double fActorRadius = pCurrentRequest->fActorRadius;
						double fLargeActorRadius = fPathingLargeActorRadius.Float();
						if (!(fLargeActorRadius < fActorRadius) ||
							(v7 = potherMesh->GetTriangle(usotherTri), !v7->GetFlag(16))) {
							NiPoint3 nodeCenter = potherMesh->GetCenter(usotherTri);
							float fcost = (aNodeCenter - nodeCenter).XYLength();
							float fnodescore = 0.0f;
							if (pCurrentRequest->GetAvoidNodeArray()) {
								PathingAvoidNodeArray* AvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
								fnodescore = AvoidNodeArray->GetCostForPoint(nodeCenter);
							}
							if (bHasGoalNode) {
								NiPoint3 deltaToGoal = aNodeCenter - *GoalNode.GetCenter(&v20);
								NiPoint3 deltaToNode = aNodeCenter - nodeCenter;
								deltaToGoal.Unitize();
								deltaToNode.Unitize();
								double v9 = deltaToGoal.Dot(deltaToNode);
								fcost = (1.0 - v9) * fcost;
								fcost = BSMAX(fcost, 0.0f);
							}
							fcost = (apNode->fCost + fcost) * (fnodescore + 1.0);
							fcost = fcost * fdoorMultiplier;
							float a1 = fcost;
							uint32_t KeyForNode = GetKeyForNode(potherMesh, usotherTri);
							int32_t v15 = AStarSearch_GetOrCreateAStarNode_6A7E00(this, KeyForNode, a1, &pnode);
							if (v15) {
								if (v15 != 1) {
									continue;
								}
							} else {
								SetNodeData(pnode, potherMesh, usotherTri);
							}
							pnode->ParentLink.pNavMesh = apNode->tNodeData.pNavMesh;
							pnode->ParentLink.sTriangle = apNode->tNodeData.sTriangle;
							pnode->ParentLink.iEdgeIndex = i;
							apRetNodes->Add(pnode);
						}
					}
				}
			}
		}
	}
	return 1;
}

bool NavMeshSearchLOS::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
	NiPoint3 v12;
	NiPoint3 triangleCenter;
	uint16_t sTriangle = apNode->tNodeData.sTriangle;
	uint32_t CastingType = apNode->tNodeData.pNavMesh->iFormID;
	char cscore = spLOSMap.m_pObject->GetAt(CastingType, sTriangle);
	if (cscore >= 0) {
		if (cscore == 127) {
			if (!pLastValidNode &&
				((&apNode->GetNodeData())->sTriangle != StartNode.sTriangle ||
					(&apNode->GetNodeData())->pNavMesh != StartNode.pNavMesh)) {
				NiPoint3* Center = (&apNode->GetNodeData())->GetCenter(&v12);
				NiPoint3 nodeVector = spLOSMap.m_pObject->GetCenter() - *Center;
				float fnodeDistanceSquared = nodeVector.SqrLength();
				if (fClosestNodeDistanceSquared > static_cast<double>(fnodeDistanceSquared)) {
					pClosestNode = apNode;
					fClosestNodeDistanceSquared = fnodeDistanceSquared;
				}
			}
		} else {
			pLastValidNode = apNode;
		}
		return 0;
	} else {
		// GetIgnoreLocations: the PDB's const getter.
		const BSSimpleArray<NiPoint3, 1024>* ignoreLocations = &pCurrentRequest->IgnoreLocations;
		for (uint32_t i = 0;; ++i) {
			uint32_t FormFlags = ignoreLocations->iSize;
			if (i >= FormFlags) {
				return 1;
			}
			apNode->tNodeData.GetCenter(&triangleCenter);
			NiPoint3 vec = triangleCenter - ignoreLocations->GetAt(static_cast<int32_t>(i));
			// The exe keeps the squared distance as a double (fstp qword).
			double fdistanceSquared = vec.SqrXYLength();
			double fradius = pCurrentRequest->fIgnoreLocationRadius;
			if (pCurrentRequest->fIgnoreLocationRadius * fradius > fdistanceSquared) {
				break;
			}
		}
		if (!pFirstIgnoredGoal) {
			pFirstIgnoredGoal = apNode;
		}
		return 0;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
bool NavMeshSearchLOS::BuildNodePath(const PathingRequestLOS* apRequest, const NavMeshSearchNode& aStartNode,
	BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
	BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray) {
	NiPoint3 kGoalCenter;
	NiPoint3 kStartCenter;
	NiPoint3 kGridCenter;
	float fheuristic;
	uint16_t usgoalNodeTri;
	pCurrentRequest = apRequest;
	StartNode = aStartNode;
	if (!apRequest->spLOSGridMap.m_pObject) {
		return false;
	}
	PathingLOSMap* pmemory = static_cast<PathingLOSMap*>(NiMemObject::operator new(sizeof(PathingLOSMap)));
	PathingLOSMap* pnewMap;
	if (pmemory) {
		float fradius = apRequest->spLOSGridMap.m_pObject->fRadius;
		NiPoint3 kCenter = *apRequest->spLOSGridMap.m_pObject->GetLocation(&kGridCenter);
		pnewMap = ::new (pmemory) PathingLOSMap(kCenter, fradius);
	} else {
		pnewMap = nullptr;
	}
	spLOSMap = pnewMap;
	apRequest->spLOSGridMap.m_pObject->SetLOSMapFromSeenTriangles(spLOSMap);
	pFirstIgnoredGoal = nullptr;
	// The exe reads the origin's location and drops it.
	apRequest->GetOrigin().GetLocation();
	StartNode.pNavMesh = aStartNode.pNavMesh;
	StartNode.sTriangle = aStartNode.sTriangle;
	NiPoint3 center = StartNode.pNavMesh->GetCenter(StartNode.sTriangle);
	StartNode.CenterX = center.x;
	StartNode.CenterY = center.y;
	StartNode.CenterZ = center.z;
	if (bHasGoalNode) {
		// The goal's center is built before the start's.
		const NiPoint3* pgoalCenter = GoalNode.GetCenter(&kGoalCenter);
		fheuristic = (*StartNode.GetCenter(&kStartCenter) - *pgoalCenter).XYLength();
	} else {
		fheuristic = 0.0f;
	}
	AddStartNode(GetKeyForNode(StartNode.pNavMesh, StartNode.sTriangle), StartNode, fheuristic);
	FNVD_CONSTRUCTED_LOCAL(NavMeshPtr, spgoalNodeMesh, &NavMeshPtr::Destruct);
	bHasGoalNode = false;
	FNVD_CONSTRUCTED_LOCAL(PathingLocation, goalLoc, &PathingLocation::Destruct, pCurrentRequest->Destination);
	if (goalLoc.ResolveNavMeshInfo(0) && goalLoc.GetNavMeshAndTriangle(spgoalNodeMesh, usgoalNodeTri)) {
		bHasGoalNode = true;
		GoalNode.pNavMesh = spgoalNodeMesh.m_pObject;
		GoalNode.sTriangle = usgoalNodeTri;
		NiPoint3 goalNodeCenter = GoalNode.pNavMesh->GetCenter(GoalNode.sTriangle);
		GoalNode.CenterX = goalNodeCenter.x;
		GoalNode.CenterY = goalNodeCenter.y;
		GoalNode.CenterZ = goalNodeCenter.z;
	}
	cMinLOSScore = BSMIN(spLOSMap.m_pObject->iMinScore + 1, -1);
	// goalLoc and then spgoalNodeMesh are destroyed after Search, as the exe does on each return path.
	if (Search(apReturnNodeArray, apReturnLinkArray, 0)) {
		return true;
	}
	return false;
}

void NavMeshSearchLOS::SetNodeData(AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode, NavMesh* apNavMesh,
	uint16_t aiTriangle) {
	NiPoint3 kGoalCenter;
	apNode->tNodeData.pNavMesh = apNavMesh;
	apNode->tNodeData.sTriangle = aiTriangle;
	NiPoint3 center = apNavMesh->GetCenter(aiTriangle);
	apNode->tNodeData.CenterX = center.x;
	apNode->tNodeData.CenterY = center.y;
	apNode->tNodeData.CenterZ = center.z;
	if (bHasGoalNode) {
		apNode->fHeuristic = (center - *GoalNode.GetCenter(&kGoalCenter)).XYLength();
	} else {
		apNode->fHeuristic = 0.0f;
	}
}

uint32_t NavMeshSearchLOS::GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const {
	return (uint32_t)(aiTri | (((uint16_t)apNavMesh->TESForm::GetFormID()) << 16));
}

char PathingLOSMap::GetAt(uint32_t aNavMeshID, uint16_t ausTriangle) {
	uint32_t a2;
	if (!NavMeshToOffsetMap.GetAt(aNavMeshID, a2)) {
		return 127;
	}
	if (a2 + ausTriangle < iTotalTriangleCount) {
		return pTriangleScores[a2 + ausTriangle];
	}
	return 127;
}

NiPoint3 PathingLOSMap::GetCenter() {
	return Center;
}

NiPoint3 PathingRequestLOS::GetSearchCenter() const {
	return SearchCenter;
}
