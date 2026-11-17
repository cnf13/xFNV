#include "Fallout Shared/Pathfinding/PathBuilder.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSSystem/BSTaskletManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshEdgeLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshInfoSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshMath.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/NavMeshSearch.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingNode.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Pathfinding/PathingSearchRayCast.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathSmoother.h"
#include "Fallout Shared/Pathfinding/PathSmootherPOVSearch.h"
#include "Fallout Shared/Pathfinding/TeleportDoorSearch.h"
#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/Pathfinding/VirtualPathingNode.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

#include <cmath>
#include <new>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bUseStraightLineCheckFirst("bUseStraightLineCheckFirst:Pathfinding", true);
SettingT<INISettingCollection> bUseObstacleAvoidance("bUseObstacleAvoidance:Pathfinding", true);
SettingT<INISettingCollection> bUsePathSmoothing("bUsePathSmoothing:Pathfinding", true);
SettingT<INISettingCollection> bCreateDebugInfo("bCreateDebugInfo:Pathfinding", false);
SettingT<INISettingCollection> bRebuildPathIfSmootherFailed("bRebuildPathIfSmootherFailed:Pathfinding", true);
SettingT<INISettingCollection> bUseOldPathSmoothing("bUseOldPathSmoothing:Pathfinding", false);
SettingT<INISettingCollection> bUseRayCasts("bUseRayCasts:Pathfinding", true);
SettingT<INISettingCollection> bUseAlternateSmoothingForPrime("bUseAlternateSmoothingForPrime:Pathfinding", true);

using VirtualNodeSegmentArray = BSSimpleArray<PathBuilder::VirtualNodeSegment, 1024>;
using VirtualNodeSegmentScrapArray = BSScrapArray<PathBuilder::VirtualNodeSegment, 1024>;

bool IsEqual(uint8_t* a1, uint8_t* a2) {
	return (bool)((uint8_t)((*a1) == (*a2)));
}

RayCastPathRequest::RayCastPathRequest() {}

RayCastPathRequest::~RayCastPathRequest() {}

void RayCastPathRequest::operator delete(void* apMemory) {
	ourFree(apMemory);
}

CheckLineOfSightFilterWater::CheckLineOfSightFilterWater() {}

bool CheckLineOfSightFilterWater::CanCrossEdge(FastNavMeshEdgeHandle hEdge, FastNavMeshEdgeHandle hMatchingEdge) {
	NavMeshTriangle* pTriangle = hMatchingEdge.pNavMesh->GetTriangle(hMatchingEdge.usTriangle);
	if (pTriangle->IsWater()) {
		return false;
	}
	if (pTriangle->IsNoLargeCreature() && bIsLargeCreature) {
		return false;
	}
	if (pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_AVOID_NODE)) {
		return false;
	}
	if (pTriangle->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
		return false;
	}
	return true;
}

PathBuilder::FindTriangleForLocationWaterFilter::FindTriangleForLocationWaterFilter() {}

bool PathBuilder::FindTriangleForLocationWaterFilter::IsValidTri(const NiPoint3& aLocation, const NavMesh* apNavMesh,
	uint16_t aiTri, const NiPoint3& aPointOnTri) {
	return true;
}

// The request and the solution are stored without taking a reference and the destructor never releases them, so the raw
// pointers are written directly to keep that balance.
PathBuilder::PathBuilder(PathingRequest* apRequest, PathingSolution* apSolution)
	: pScrapHeap(MemoryManager::Instance().GetThreadScrapHeap()), bPathUpdate(false) {
	pRequest.m_pObject = apRequest;
	pSolution.m_pObject = apSolution;
}

PathBuilder::~PathBuilder() {
	pSolution.m_pObject = nullptr;
	pRequest.m_pObject = nullptr;
}

Pathing::UPDATE_DETAILED_PATH_RESULT PathBuilder::UpdateDetailedPath(uint32_t aiCurrentVirtualNodeIndex,
	const NiPoint3& aCurrentActorLocation) {
	int32_t arFirstLoadedNodeIndex;
	int32_t arLastLoadedNodeIndex;
	arFirstLoadedNodeIndex = -1;
	arLastLoadedNodeIndex = -1;
	if (FindLoadedNodeIndices(aiCurrentVirtualNodeIndex, &arFirstLoadedNodeIndex, &arLastLoadedNodeIndex)) {
		if (pSolution.m_pObject->GetNodeCount()) {
			int32_t ShaderPropertyType = pSolution.m_pObject->iLastLoadedVirtualNodeIndex;
			if (ShaderPropertyType >= arLastLoadedNodeIndex) {
				return Pathing::NO_CHANGE;
			}
			pSolution.m_pObject->iFirstLoadedVirtualNodeIndex = aiCurrentVirtualNodeIndex;
			VirtualPathingNode* VirtualNode = pSolution.m_pObject->GetVirtualNode(aiCurrentVirtualNodeIndex);
			PathingLocation* ActorPackageThatIsRunning = static_cast<PathingLocation*>(&VirtualNode->GetLocation());
			PathingLocation arLocation(aCurrentActorLocation, *ActorPackageThatIsRunning);
			VirtualPathingNode* v7 = pSolution.m_pObject->GetVirtualNode(aiCurrentVirtualNodeIndex);
			v7->SetLocation(arLocation);
			uint32_t aiIndex = pSolution.m_pObject->GetNodeCount() - 1;
			pSolution.m_pObject->RemoveLastNode();
			pSolution.m_pObject->iLastLoadedVirtualNodeIndex = arLastLoadedNodeIndex;
			bPathUpdate = 1;
			if (!BuildDetailedPathForLoadedVirtualNodes(0) || pSolution.m_pObject->IsIncomplete()) {
				return Pathing::FAILED;
			}
			pSolution.m_pObject->RemoveNode(aiIndex);
		} else {
			if (!BuildDetailedPath(0, aiCurrentVirtualNodeIndex)) {
				return Pathing::FAILED;
			}
		}
		return Pathing::CREATED_DETAILED_PATH;
	} else {
		if (pSolution.m_pObject->GetNodeCount()) {
			pSolution.m_pObject->ClearCurrentDetailedPath();
			return Pathing::CLEARED_DETAILED_PATH;
		} else {
			return Pathing::NO_CHANGE;
		}
	}
}

Pathing::UPDATE_DETAILED_PATH_RESULT PathBuilder::UpdatePathMoveToLow() {
	pSolution.m_pObject->ClearCurrentDetailedPath();
	return Pathing::CLEARED_DETAILED_PATH;
}

Pathing::UPDATE_DETAILED_PATH_RESULT PathBuilder::UpdatePathMoveToHigh(uint32_t aiCurrentVirtualNodeIndex) {
	int32_t inewFirstLoadedNodeIndex;
	int32_t inewLastLoadedNodeIndex;
	inewFirstLoadedNodeIndex = -1;
	inewLastLoadedNodeIndex = -1;
	if (!FindLoadedNodeIndices(aiCurrentVirtualNodeIndex, &inewFirstLoadedNodeIndex, &inewLastLoadedNodeIndex)) {
		return Pathing::NO_CHANGE;
	}
	for (uint32_t i = 0; i < pSolution.m_pObject->GetVirtualNodeCount(); ++i) {
		VirtualPathingNode* VirtualNode = pSolution.m_pObject->GetVirtualNode(i);
		(&VirtualNode->Location)->ClearTriangleData();
	}
	for (int32_t i_2 = inewFirstLoadedNodeIndex; i_2 <= inewLastLoadedNodeIndex; ++i_2) {
		VirtualPathingNode* v4 = pSolution.m_pObject->GetVirtualNode(i_2);
		v4->UpdateNode();
	}
	if (BuildDetailedPath(0, aiCurrentVirtualNodeIndex)) {
		return Pathing::CREATED_DETAILED_PATH;
	} else {
		return Pathing::FAILED;
	}
}

// BuildDetailedPath is passed a null actor local and 0 as the first high-level node, as the game does.
bool PathBuilder::BuildPath() {
	Actor* pActor = nullptr;
	pRequest->GetOrigin().ClearTriangleData();
	pRequest->GetDestination().ClearTriangleData();
	if (!pSolution->GetVirtualNodeCount()) {
		const NavMeshInfo* pStartMesh = nullptr;
		const NavMeshInfo* pGoalMesh = nullptr;
		if (pRequest->GetOrigin().ResolveNavMeshInfo(&WaterFilter))
			pStartMesh = pRequest->GetOrigin().GetBestNavMeshInfo();
		if (pRequest->GetDestination().ResolveNavMeshInfo(&WaterFilter))
			pGoalMesh = pRequest->GetDestination().GetBestNavMeshInfo();
		if (!BuildHighLevelPath(pStartMesh, pGoalMesh) && !pSolution->GetVirtualNodeCount())
			return false;
	} else {
		pRequest->GetOrigin().ResolveNavMeshInfo(&WaterFilter);
		pRequest->GetDestination().ResolveNavMeshInfo(&WaterFilter);
	}
	return BuildDetailedPath(pActor, 0);
}

RAYCASTPATH_RESULT PathBuilder::BuildPathFromNavMeshSearchGoal(uint32_t aiLastVirtualNodeIndex,
	BSSimpleArray<PathingLocation, 1024>& aRetPath, BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges) {
	VirtualPathingNode* pLastNode = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex);
	if (aiLastVirtualNodeIndex != pSolution.m_pObject->GetVirtualNodeCount() - 1) {
		VirtualPathingNode* pNextNode = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex + 1);
		// The index compare is signed.
		if (static_cast<int32_t>(aiLastVirtualNodeIndex) >= pSolution.m_pObject->iLastLoadedVirtualNodeIndex) {
			if (!pLastNode->IsTransitionNode()) {
				PathingLocation exitLoc;
				if (FindExitPoint(pLastNode, pNextNode, exitLoc)) {
					pLastNode->Location.operator=(exitLoc);
					aRetPath.Add(exitLoc);
					return RCP_SUCCESS;
				}
			}
		} else {
			if (pNextNode->HasActualLocation()) {
				PathingLocation exitLoc;
				if (FindExitPoint(pLastNode, pNextNode, exitLoc)) {
					pLastNode->Location.operator=(exitLoc);
					aRetPath.Add(exitLoc);
					return RCP_SUCCESS;
				}
			}
		}
	}
	if (!pLastNode->HasActualLocation()) {
		return RCP_FAILED;
	}
	PathingLocation* pLastNodeLoc = &pLastNode->Location;
	if (!pLastNodeLoc->ResolveNavMeshInfo(0)) {
		return RCP_FAILED;
	}
	if (pLastNodeLoc->HasNavMeshData()) {
		aRetPath.Add(*pLastNodeLoc);
		return RCP_SUCCESS;
	}
	Pathing::FIND_CLOSEST_EDGES_RESULT eClosestEdges = Pathing::FindClosestEdges(*pLastNodeLoc, 10u, aRetEdges);
	if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_ONTRI) {
		aRetPath.Add(*pLastNodeLoc);
		return RCP_SUCCESS;
	}
	if (eClosestEdges != Pathing::FIND_CLOSEST_EDGES_OK) {
		return RCP_FAILED;
	}
	for (uint32_t uiIndex = 0; uiIndex < aRetEdges.iSize; ++uiIndex) {
		NiPoint3 deltaToEdge = pLastNodeLoc->GetLocation() - aRetEdges.GetAt(static_cast<int32_t>(uiIndex)).Location;
		// The squared distance and the radius are kept as doubles, and the radius is squared in double precision.
		double dSqrDistance = deltaToEdge.SqrXYLength();
		double dRadius = pRequest.m_pObject->fTargetRadius;
		if (pRequest.m_pObject->fTargetRadius * dRadius > dSqrDistance && deltaToEdge.z > -64.0 &&
			deltaToEdge.z < 180.0) {
			uint16_t usTriangle = aRetEdges.GetAt(static_cast<int32_t>(uiIndex)).usTriangle;
			NavMeshInfo* pNavMeshInfo = aRetEdges.GetAt(static_cast<int32_t>(uiIndex)).pNavMeshInfo;
			NavMeshEdgeLocation* pEdge = &aRetEdges.GetAt(static_cast<int32_t>(uiIndex));
			PathingLocation retPathLoc(pEdge->Location, pNavMeshInfo, usTriangle);
			aRetPath.Add(retPathLoc);
			return RCP_SUCCESS;
		}
	}
	return BuildPathToValidNavMeshTriangle(*pLastNodeLoc, aRetPath, 0, aRetEdges);
}

RAYCASTPATH_RESULT PathBuilder::BuildStraightPathUsingRayCasts(PathingLocation& aStartLocation,
	PathingLocation& aGoalLocation, BSSimpleArray<PathingLocation, 1024>& aRetPath) {
	if (!bUseRayCasts.Bool()) {
		return RCP_FAILED;
	}
	BSTaskletManager* pTaskletManager = &BSTaskletManager::QInstance();
	uint32_t v4 = BSGetCurrentThreadID();
	if (pTaskletManager->IsTaskletThread(v4)) {
		PathingRequest* m_pObject = pRequest.m_pObject;
		PathManager* v5 = PathManager::QInstance();
		return v5->GetRayCastPath(m_pObject, aStartLocation, aGoalLocation, aRetPath);
	} else {
		RayCastPathRequest aRequest;
		aRequest.StartLocation.operator=(aStartLocation);
		aRequest.GoalLocation.operator=(aGoalLocation);
		aRequest.eType = RCRT_NOMESH;
		aRequest.pInitialPathRequest = pRequest.m_pObject;
		RayCastPath v10;
		PathingSearchRayCast apMemory;
		if (apMemory.BuildPath(aRequest, v10) == RCP_SUCCESS) {
			v10.Nodes.CopyTo(&aRetPath);
			return RCP_SUCCESS;
		} else {
			return RCP_FAILED;
		}
	}
}

bool PathBuilder::BuildNavMeshPathForVirtualNodes(uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex) {
	using LocationArray = BSSimpleArray<PathingLocation, 1024>;
	using EdgeArray = BSSimpleArray<NavMeshEdgeLocation, 1024>;
	EdgeArray startEdges;
	LocationArray startNodes;
	RAYCASTPATH_RESULT efoundPathToStart =
		BuildPathToNavMeshSearchStart(aiFirstVirtualNodeIndex, startNodes, startEdges);
	EdgeArray endEdges;
	LocationArray endNodes;
	RAYCASTPATH_RESULT efoundPathToGoal = BuildPathFromNavMeshSearchGoal(aiLastVirtualNodeIndex, endNodes, endEdges);
	if (efoundPathToStart == RCP_NONE || efoundPathToGoal == RCP_NONE) {
		return false;
	}
	float fdistFromStartToClosestEdge = 0.0f;
	if (!startEdges.IsEmpty()) {
		NiPoint3 closestStartEdgeCenter = startEdges.GetAt(0).Location;
		VirtualPathingNode* pFirstNode = pSolution.m_pObject->GetVirtualNode(aiFirstVirtualNodeIndex);
		fdistFromStartToClosestEdge = (closestStartEdgeCenter - pFirstNode->Location.GetLocation()).XYLength();
	}
	float fdistFromGoalToClosestEdge = 0.0f;
	if (!endEdges.IsEmpty()) {
		NiPoint3 closestGoalEdgeCenter = endEdges.GetAt(0).Location;
		VirtualPathingNode* pLastNode = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex);
		fdistFromGoalToClosestEdge = (closestGoalEdgeCenter - pLastNode->Location.GetLocation()).XYLength();
	}
	// Fetches the last node's location before the first node.
	const NiPoint3& lastLocation = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex)->Location.GetLocation();
	VirtualPathingNode* pFirstNode = pSolution.m_pObject->GetVirtualNode(aiFirstVirtualNodeIndex);
	float fdistFromStartToGoal = (pFirstNode->Location.GetLocation() - lastLocation).XYLength();
	bool btryRayCastPath = fdistFromStartToGoal < (fdistFromStartToClosestEdge + fdistFromGoalToClosestEdge) * 2.0;
	if ((efoundPathToStart == RCP_FAILED || efoundPathToGoal == RCP_FAILED) && fdistFromStartToGoal < 512.0) {
		btryRayCastPath = true;
	}
	if (btryRayCastPath) {
		LocationArray rayCastPath;
		PathingLocation* pGoalLoc = &pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex)->Location;
		VirtualPathingNode* pStartNode = pSolution.m_pObject->GetVirtualNode(aiFirstVirtualNodeIndex);
		RAYCASTPATH_RESULT res = BuildStraightPathUsingRayCasts(pStartNode->Location, *pGoalLoc, rayCastPath);
		if (res == RCP_NONE) {
			return false;
		}
		if (res == RCP_SUCCESS) {
			for (uint32_t i = 0; i < rayCastPath.iSize; ++i) {
				pSolution.m_pObject->AddPathingNode(rayCastPath.GetAt(static_cast<int32_t>(i)), NiPoint3_ZERO);
			}
			VirtualPathingNode* plastVirtualNode = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex);
			if (plastVirtualNode->IsTransitionNode()) {
				TESObjectREFR* pDoorRef = plastVirtualNode->pDoorRef;
				PathingNode* pLastPathNode = pSolution.m_pObject->GetLastNode();
				pLastPathNode->SetTeleportDoor(pDoorRef);
			} else {
				if (plastVirtualNode->Location.pWorldSpace) {
					if (pSolution.m_pObject->GetVirtualNodeCount() > aiLastVirtualNodeIndex + 1) {
						TESWorldSpace* pNextWorldSpace =
							pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex + 1)->Location.pWorldSpace;
						if (pNextWorldSpace == plastVirtualNode->Location.pWorldSpace) {
							PathingLocation* pNextLoc =
								&pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex + 1)->Location;
							pSolution.m_pObject->AddPathingNode(*pNextLoc, NiPoint3_ZERO);
							PathingNode* pLastPathNode = pSolution.m_pObject->GetLastNode();
							pLastPathNode->SetTransitionFlag(1);
						}
					}
				}
			}
			return true;
		}
	}
	if (efoundPathToStart == RCP_FAILED) {
	LABEL_62:;
		return false;
	}
	if (efoundPathToGoal == RCP_FAILED) {
		if (endEdges.IsEmpty()) {
			goto LABEL_62;
		}
		if (!CheckClosestEdgeToGoal(aiLastVirtualNodeIndex, endEdges, endNodes)) {
			pSolution.m_pObject->SetIncomplete(1);
		}
		PathingLocation endLoc(endEdges.GetFirst().Location, endEdges.GetFirst().pNavMeshInfo,
			endEdges.GetFirst().usTriangle);
		endNodes.Add(endLoc);
	}
	for (uint32_t i_2 = 0; i_2 < startNodes.iSize - 1; ++i_2) {
		pSolution.m_pObject->AddPathingNode(startNodes.GetAt(static_cast<int32_t>(i_2)), NiPoint3_ZERO);
	}
	PathingLocation* pGoalTail = static_cast<PathingLocation*>(endNodes.GetTail());
	PathingLocation* pStartTail = static_cast<PathingLocation*>(startNodes.GetTail());
	PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT res_2 =
		AddNodesFromNavMesh(*pStartTail, *pGoalTail, aiFirstVirtualNodeIndex, aiLastVirtualNodeIndex);
	if (res_2) {
		if (res_2 == PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_INCOMPLETE) {
			pSolution.m_pObject->SetIncomplete(1);
			return false;
		}
		if (res_2 != PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED) {
			goto LABEL_62;
		}
		if (!(fdistFromStartToGoal < 512.0)) {
			goto LABEL_62;
		}
		double dActorRadius = pRequest.m_pObject->fActorRadius;
		if (!(fPathingLargeActorRadius.Float() > dActorRadius)) {
			goto LABEL_62;
		}
		PathingLocation* pGoalLoc = &pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex)->Location;
		VirtualPathingNode* pStartNode = pSolution.m_pObject->GetVirtualNode(aiFirstVirtualNodeIndex);
		if (Pathing::CheckLineOfSight(pStartNode->Location, *pGoalLoc, 0, 0)) {
			goto LABEL_62;
		}
		LocationArray rayCastPath_2;
		PathingLocation* pGoalLoc_2 = &pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex)->Location;
		VirtualPathingNode* pStartNode_2 = pSolution.m_pObject->GetVirtualNode(aiFirstVirtualNodeIndex);
		RAYCASTPATH_RESULT res_3 = BuildStraightPathUsingRayCasts(pStartNode_2->Location, *pGoalLoc_2, rayCastPath_2);
		if (res_3 == RCP_SUCCESS) {
			pSolution.m_pObject->SetIncomplete(0);
			for (uint32_t i_3 = 0; i_3 < rayCastPath_2.iSize; ++i_3) {
				pSolution.m_pObject->AddPathingNode(rayCastPath_2.GetAt(static_cast<int32_t>(i_3)), NiPoint3_ZERO);
			}
			bool bComplete = !pSolution.m_pObject->IsIncomplete();
			return bComplete;
		}
		return false;
	}
	if (!endNodes.IsEmpty()) {
		for (int32_t i_4 = endNodes.iSize - 1; i_4 >= 0; --i_4) {
			pSolution.m_pObject->AddPathingNode(endNodes.GetAt(i_4), NiPoint3_ZERO);
		}
	}
	VirtualPathingNode* plastVirtualNode_2 = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex);
	if (plastVirtualNode_2->IsTransitionNode()) {
		TESObjectREFR* pDoorRef = plastVirtualNode_2->pDoorRef;
		PathingNode* pLastPathNode = pSolution.m_pObject->GetLastNode();
		pLastPathNode->SetTeleportDoor(pDoorRef);
	} else {
		if (plastVirtualNode_2->Location.pWorldSpace) {
			if (pSolution.m_pObject->GetVirtualNodeCount() > aiLastVirtualNodeIndex + 1) {
				TESWorldSpace* pNextWorldSpace =
					pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex + 1)->Location.pWorldSpace;
				if (pNextWorldSpace == plastVirtualNode_2->Location.pWorldSpace) {
					PathingLocation* pNextLoc =
						&pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex + 1)->Location;
					pSolution.m_pObject->AddPathingNode(*pNextLoc, NiPoint3_ZERO);
					PathingNode* pLastPathNode = pSolution.m_pObject->GetLastNode();
					pLastPathNode->SetTransitionFlag(1);
				}
			}
		}
	}
	bool bComplete = !pSolution.m_pObject->IsIncomplete();
	return bComplete;
}

PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT PathBuilder::AddNodesFromNavMesh(PathingLocation& aStartLocation,
	PathingLocation& aGoalLocation, uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex) {
	NiPoint3 v24;
	NiPoint3 v26;
	NiPoint3 result;
	NiPoint3 v31;
	PathingNode* plastNode;
	NiPoint3 delta;
	PathingLocation lastLoc;
	uint32_t icurrentNodeCount;
	using nodes_t = BSSimpleArray<NavMeshSearchNode, 1024>;
	FNVD_INLINED_LOCAL(nodes_t, nodes, std::nullopt, std::nullopt);
	// The nodes are read through the const GetAt.
	const nodes_t& rconstNodes = nodes;
	PathingLocation smoothingGoalLocation;
	using BadNodes_t = BSSimpleArray<NavMeshSearchLink, 1024>;
	FNVD_INLINED_LOCAL(BadNodes_t, links, std::nullopt, std::nullopt);
	PathingLocation smoothingStartLocation;
	int32_t pass;
	if (bUseStraightLineCheckFirst.Bool() &&
		(!pRequest.m_pObject->GetAvoidNodeArray() &&
			(!pRequest.m_pObject->bFaceTargetAtGoal &&
				(!pRequest.m_pObject->GetTryToUseCurvedPath() &&
					TryBuildStraightPath(aStartLocation, aGoalLocation))))) {
		return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_OK;
	}
	BadNodes_t BadNodes(32u, 0);
	bool bAddNodesFromNavMeshIncomplete = 0;
	int32_t imaxPassCount = pRequest.m_pObject->iRetryCount;
	BSClamp<int32_t>(imaxPassCount, 1, 10);
	DetailedPathInfos.Clear(0);
	DetailedPathMeshes.Clear(0);
	for (uint32_t i = 0;; ++i) {
		uint32_t VirtualNodeCount = pSolution.m_pObject->GetVirtualNodeCount();
		if (i >= VirtualNodeCount) {
			break;
		}
		NavMeshPtr spmesh;
		VirtualPathingNode* VirtualNode = pSolution.m_pObject->GetVirtualNode(i);
		if ((&VirtualNode->Location)->GetNavMesh(spmesh)) {
			DetailedPathMeshes.Add(spmesh);
			NavMeshInfo* a2 = spmesh.m_pObject->GetNavMeshInfo();
			DetailedPathInfos.Add(a2);
		}
	}
	for (pass = 0;; ++pass) {
		NavMeshInfo* NavMeshInfo_;
		uint16_t a1;
		NiPoint3* a1a;
		VirtualPathingNode* plastVirtualNode;
		float ftargetRadius;
		NavMeshSearchNode* lastNode;
		if (pass >= imaxPassCount) {
			goto LABEL_73;
		}
		if (pass == 1) {
			using loadedNavmeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
			loadedNavmeshes_t loadedNavmeshes;
			Pathing::GetAllLoadedNavmeshes(&loadedNavmeshes);
			for (uint32_t i_2 = 0;; ++i_2) {
				uint32_t FormFlags = loadedNavmeshes.iSize;
				if (i_2 >= FormFlags) {
					break;
				}
				if (DetailedPathMeshes.Find(loadedNavmeshes.GetAt(static_cast<int32_t>(i_2)), 0,
						&BSSimpleArray<NavMeshPtr, 1024>::DefaultEqualFunctionByAddress) == -1) {
					DetailedPathMeshes.Add(loadedNavmeshes.GetAt(static_cast<int32_t>(i_2)));
					NavMesh* v9 = loadedNavmeshes.GetAt(static_cast<int32_t>(i_2)).m_pObject;
					NavMeshInfo* aItem = v9->GetNavMeshInfo();
					DetailedPathInfos.Add(aItem);
				}
			}
		}
		::new (&nodes) BSSimpleArray<NavMeshSearchNode, 1024>(32u, 0);
		::new (&links) BSSimpleArray<NavMeshSearchLink, 1024>(32u, 0);
		smoothingStartLocation.~PathingLocation();
		::new (&smoothingStartLocation) PathingLocation(aStartLocation);
		smoothingGoalLocation.~PathingLocation();
		::new (&smoothingGoalLocation) PathingLocation(aGoalLocation);
		if (RunNavMeshSearch_006CD670(aStartLocation, aGoalLocation, &BadNodes, nodes, links, aiFirstVirtualNodeIndex,
				aiLastVirtualNodeIndex)) {
			break;
		}
		if (nodes.IsEmpty() && pass >= 1) {
			PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT v32 = PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
			links.~BSSimpleArray<NavMeshSearchLink, 1024>();
			nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
			return v32;
		}
		if (nodes.IsEmpty()) {
			goto LABEL_27;
		}
		lastNode = &nodes.GetLast();
		a1 = lastNode->sTriangle;
		NavMeshInfo_ = lastNode->pNavMesh->GetNavMeshInfo();
		FNVD_RECONSTRUCT(PathingLocation)(&lastLoc, *lastNode->GetCenter(&v31), NavMeshInfo_, a1);
		if (pSolution.m_pObject->iLastLoadedVirtualNodeIndex != aiLastVirtualNodeIndex) {
			goto LABEL_41;
		}
		plastVirtualNode = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex);
		a1a = ::new (&result) NiPoint3((&plastVirtualNode->Location)->GetLocation());
		::new (&delta) NiPoint3(lastLoc.GetLocation() - *a1a);
		if (plastVirtualNode->IsTransitionNode()) {
			ftargetRadius = 80.0f;
		} else {
			ftargetRadius = pRequest.m_pObject->fTargetRadius;
		}
		if (ftargetRadius * ftargetRadius > delta.SqrXYLength() && std::fabs(delta.z) < 180.0) {
			if (!pRequest.m_pObject->bAllowIncompletePath) {
				PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT v28 = PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
				links.~BSSimpleArray<NavMeshSearchLink, 1024>();
				nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
				return v28;
			}
			if (!plastVirtualNode->IsTransitionNode()) {
				bAddNodesFromNavMeshIncomplete = 1;
				pSolution.m_pObject->SetIncomplete(1);
			}
			goto LABEL_42;
		}
		if (pass) {
		LABEL_41:;
			bAddNodesFromNavMeshIncomplete = 1;
			pSolution.m_pObject->SetIncomplete(1);
		LABEL_42:;
			if (!pRequest.m_pObject->bAllowIncompletePath) {
				PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT v27 = PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
				links.~BSSimpleArray<NavMeshSearchLink, 1024>();
				nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
				return v27;
			}
			smoothingGoalLocation.operator=(lastLoc);
			break;
		}
		links.~BSSimpleArray<NavMeshSearchLink, 1024>();
		nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
	LABEL_13:;
		;
	}
	icurrentNodeCount = pSolution.m_pObject->GetNodeCount();
	if (RunPathSmoother(smoothingStartLocation, smoothingGoalLocation, BadNodes, nodes, links)) {
		goto LABEL_72;
	}
	if (pass == imaxPassCount - 1 && pRequest.m_pObject->GetIgnoreSmoothingIfFails()) {
		bAddNodesFromNavMeshIncomplete = 1;
		pSolution.m_pObject->SetIncomplete(1);
		for (uint32_t i_3 = 0;; ++i_3) {
			uint32_t v13 = nodes.iSize;
			if (i_3 >= v13) {
				break;
			}
			NavMeshInfo* a1b = rconstNodes.GetAt(static_cast<int32_t>(i_3)).pNavMesh->GetNavMeshInfo();
			NiPoint3* v14 = rconstNodes.GetAt(static_cast<int32_t>(i_3)).GetCenter(&v26);
			PathingLocation loc(v14, a1b);
			pSolution.m_pObject->AddPathingNode(loc, NiPoint3_ZERO);
		}
	LABEL_72:;
		links.~BSSimpleArray<NavMeshSearchLink, 1024>();
		nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
	LABEL_73:;
		if (bAddNodesFromNavMeshIncomplete) {
			return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_INCOMPLETE;
		} else {
			return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_OK;
		}
	}
	plastNode = pSolution.m_pObject->GetLastNode();
	if (plastNode) {
		if (pSolution.m_pObject->iLastLoadedVirtualNodeIndex == aiLastVirtualNodeIndex) {
			float ftargetRadius_2;
			VirtualPathingNode* plastVirtualNode_2 = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex);
			NiPoint3* a1c = ::new (&v24) NiPoint3((&plastVirtualNode_2->Location)->GetLocation());
			NiPoint3 delta_2 = (&plastNode->Location)->GetLocation() - *a1c;
			if (plastVirtualNode_2->IsTransitionNode()) {
				ftargetRadius_2 = 80.0f;
			} else {
				ftargetRadius_2 = pRequest.m_pObject->fTargetRadius;
			}
			if (ftargetRadius_2 * ftargetRadius_2 > delta_2.SqrXYLength() && std::fabs(delta_2.z) < 180.0) {
				goto LABEL_72;
			}
		}
	}
	if (!BadNodes.IsEmpty() && pass != imaxPassCount - 1) {
		if (pass < imaxPassCount - 1) {
			while (1) {
				uint32_t NodeCount = pSolution.m_pObject->GetNodeCount();
				if (NodeCount == icurrentNodeCount) {
					break;
				}
				pSolution.m_pObject->RemoveLastNode();
			}
		}
	LABEL_27:;
		links.~BSSimpleArray<NavMeshSearchLink, 1024>();
		nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
		goto LABEL_13;
	}
	if (plastNode) {
		bAddNodesFromNavMeshIncomplete = 1;
		pSolution.m_pObject->SetIncomplete(1);
		if (pRequest.m_pObject->bAllowIncompletePath) {
			links.~BSSimpleArray<NavMeshSearchLink, 1024>();
			nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
			return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_INCOMPLETE;
		}
		while (1) {
			uint32_t v17 = pSolution.m_pObject->GetNodeCount();
			if (v17 == icurrentNodeCount) {
				break;
			}
			pSolution.m_pObject->RemoveLastNode();
		}
	}
	links.~BSSimpleArray<NavMeshSearchLink, 1024>();
	nodes.~BSSimpleArray<NavMeshSearchNode, 1024>();
	return PathBuilder::ADD_NODES_FROM_NAVMESH_RESULT_FAILED;
}

// The setting's value is unused: Setting::Bool is called for a debug report that is compiled out, as the game does.
bool PathBuilder::BuildHighLevelPath(const NavMeshInfo* apStartMesh, const NavMeshInfo* apGoalMesh) {
	bool bFound = false;
	if (!apStartMesh || !apGoalMesh) {
		bCreateDebugInfo.Bool();
	} else if (apStartMesh == apGoalMesh) {
		pSolution.m_pObject->AddVirtualPathingNode(pRequest.m_pObject->GetOrigin());
		pSolution.m_pObject->AddVirtualPathingNode(pRequest.m_pObject->GetDestination());
		bFound = true;
	} else if (BuildNavMeshInfoPath(apStartMesh, apGoalMesh)) {
		bFound = true;
	}
	if (bFound) {
		return true;
	}
	if (pRequest.m_pObject->GetOrigin().GetWorldspaceSimple()) {
		if (pRequest.m_pObject->GetOrigin().GetWorldspaceSimple() ==
				pRequest.m_pObject->GetDestination().GetWorldspaceSimple() &&
			pRequest.m_pObject->GetOrigin().GetBestNavMeshInfo() &&
			pRequest.m_pObject->GetDestination().GetBestNavMeshInfo()) {
			return false;
		}
	} else if (pRequest.m_pObject->GetOrigin().GetCell() == pRequest.m_pObject->GetDestination().GetCell() &&
		pRequest.m_pObject->GetOrigin().GetBestNavMeshInfo() &&
		pRequest.m_pObject->GetDestination().GetBestNavMeshInfo()) {
		return false;
	}
	pSolution.m_pObject->VirtualPathingNodes.Clear(false);
	return BuildTeleportDoorPath();
}

bool PathBuilder::BuildNavMeshInfoPath(const NavMeshInfo* apStartMesh, const NavMeshInfo* apGoalMesh) {
	NiPoint3 result;
	NiPoint3 markerLoc_2;
	NiPoint3 markerLoc;
	using infos_t = BSScrapArray<const NavMeshInfo*, 1024>;
	using portals_t = BSScrapArray<TESObjectREFR*, 1024>;
	NavMeshInfoSearch highSearch;
	infos_t infos(20u, 0, pScrapHeap);
	portals_t portals(20u, 0, pScrapHeap);
	ActorLockData* plockData = 0;
	if (!pRequest.m_pObject->bCantOpenDoors) {
		plockData = &pRequest.m_pObject->LockData;
	}
	Pathing::DOOR_LOCK_DISPOSITION doorLockDisp =
		static_cast<Pathing::DOOR_LOCK_DISPOSITION>(pRequest.m_pObject->GetIgnoreLocks());
	NiPoint3* Location_ = ::new (&result) NiPoint3((&pRequest.m_pObject->Destination)->GetLocation());
	bool binfoSearchSucceeded = highSearch.Pathfind(plockData, (&pRequest.m_pObject->Origin)->GetLocation(),
		apStartMesh, *Location_, apGoalMesh, &infos, &portals, doorLockDisp);
	if (infos.iSize > 1) {
		pSolution.m_pObject->AddVirtualPathingNode(*&pRequest.m_pObject->Origin);
		if (portals.GetAt(0)) {
			TESObjectREFR* pTelDoor = portals.GetAt(0);
			markerLoc = pTelDoor->GetLinkedDoorTeleportPosition();
			pSolution.m_pObject->AddNode_Door(infos.GetAt(0), &markerLoc, pTelDoor);
			NiPoint3 teleportLoc = *&pTelDoor->GetTeleport()->position;
			pSolution.m_pObject->AddVirtualPathingNode_0(teleportLoc, infos.GetAt(1));
		}
		for (uint32_t i = 1;; ++i) {
			uint32_t FormFlags = infos.iSize;
			if (i >= FormFlags - 1) {
				break;
			}
			if (portals.GetAt(static_cast<int32_t>(i))) {
				TESObjectREFR* pTelDoor_2 = portals.GetAt(static_cast<int32_t>(i));
				if (pTelDoor_2) {
					markerLoc_2 = pTelDoor_2->GetLinkedDoorTeleportPosition();
					pSolution.m_pObject->AddNode_Door(infos.GetAt(static_cast<int32_t>(i)), &markerLoc_2, pTelDoor_2);
					NiPoint3 teleportLoc_2 = *&pTelDoor_2->GetTeleport()->position;
					pSolution.m_pObject->AddVirtualPathingNode_0(teleportLoc_2,
						infos.GetAt(static_cast<int32_t>(i + 1)));
				}
			} else {
				pSolution.m_pObject->AddVirtualPathingNode_3(infos.GetAt(static_cast<int32_t>(i)));
			}
		}
		pSolution.m_pObject->AddVirtualPathingNode(*&pRequest.m_pObject->Destination);
	} else {
		pSolution.m_pObject->AddVirtualPathingNode(*&pRequest.m_pObject->Origin);
		pSolution.m_pObject->AddVirtualPathingNode(*&pRequest.m_pObject->Destination);
	}
	return binfoSearchSucceeded;
}

bool PathBuilder::BuildTeleportDoorPath() {
	NiPoint3 result;
	TeleportDoorSearch doorSearch;
	TeleportPath doorPath;
	doorPath.Spaces.SetReservedSize(10u);
	doorPath.Doors.SetReservedSize(10u);
	ActorLockData* plockData = 0;
	if (!pRequest.m_pObject->bCantOpenDoors) {
		plockData = &pRequest.m_pObject->LockData;
	}
	bool bteleportSearchSucceeded =
		doorSearch.BuildNodePath(*&pRequest.m_pObject->Origin, *&pRequest.m_pObject->Destination, &doorPath,
			Pathing::TELEPORT_DOOR_PATH_NONE, 0.0f, plockData, Pathing::DOOR_LOCK_DISPOSITION_NORMAL);
	if (doorPath.Spaces.iSize > 1) {
		NiPoint3 startLocation = (&pRequest.m_pObject->Origin)->GetLocation();
		TeleportLink* First = &doorPath.Doors.GetFirst();
		NiPoint3 goalLocation = First->pDoor->GetLinkedDoorTeleportPosition();
		TESObjectREFR* pDoor = (&doorPath.Doors.GetFirst())->pDoor;
		ParentSpaceNode* v4 = &doorPath.Spaces.GetFirst();
		if (!BuildNavMeshInfoPathInSpace(startLocation, goalLocation, v4, pDoor)) {
			return 0;
		}
		for (uint32_t i = 0;; ++i) {
			uint32_t FormFlags = doorPath.Doors.iSize;
			if (i >= FormFlags - 1) {
				break;
			}
			NiPoint3 startLocation_2 = (&doorPath.Doors.GetAt(static_cast<int32_t>(i)))->DoorTeleportLocation;
			NiPoint3 goalLocation_2 =
				(&doorPath.Doors.GetAt(static_cast<int32_t>(i + 1)))->pDoor->GetLinkedDoorTeleportPosition();
			TESObjectREFR* v10 = (&doorPath.Doors.GetAt(static_cast<int32_t>(i + 1)))->pDoor;
			if (!BuildNavMeshInfoPathInSpace(startLocation_2, goalLocation_2,
					&doorPath.Spaces.GetAt(static_cast<int32_t>(i + 1)), v10)) {
				pSolution.m_pObject->SetIncomplete(1);
				return 0;
			}
		}
		startLocation = (&doorPath.Doors.GetLast())->DoorTeleportLocation;
		goalLocation = (&pRequest.m_pObject->Destination)->GetLocation();
		ParentSpaceNode* Last = &doorPath.Spaces.GetLast();
		if (!BuildNavMeshInfoPathInSpace(startLocation, goalLocation, Last, 0)) {
			pSolution.m_pObject->SetIncomplete(1);
			return 0;
		}
	} else {
		if (bteleportSearchSucceeded) {
			ParentSpaceNode* v8 = &doorPath.Spaces.GetAt(0);
			NiPoint3* afMaxDist = ::new (&result) NiPoint3((&pRequest.m_pObject->Destination)->GetLocation());
			if (!BuildNavMeshInfoPathInSpace((&pRequest.m_pObject->Origin)->GetLocation(), *afMaxDist, v8, 0)) {
				return 0;
			}
		}
	}
	return bteleportSearchSucceeded;
}

bool PathBuilder::BuildDetailedPath(Actor* pActor, uint32_t aiFirstHighLevelNode) {
	NiPoint3 v17;
	NiPoint3 result;
	NiPoint3 tangent_2;
	NiPoint3 tangent;
	int32_t ilastLoadedNode;
	int32_t ifirstLoadedNode;
	pSolution.m_pObject->ClearCurrentDetailedPath();
	ifirstLoadedNode = -1;
	ilastLoadedNode = -1;
	if (!FindLoadedNodeIndices(aiFirstHighLevelNode, &ifirstLoadedNode, &ilastLoadedNode)) {
		return 1;
	}
	pSolution.m_pObject->iFirstLoadedVirtualNodeIndex = ifirstLoadedNode;
	pSolution.m_pObject->iLastLoadedVirtualNodeIndex = ilastLoadedNode;
	bool bbuildDetailedPathRes = BuildDetailedPathForLoadedVirtualNodes(pActor);
	if (!bbuildDetailedPathRes && !pSolution.m_pObject->GetNodeCount()) {
		return 0;
	}
	if (pRequest.m_pObject->GetFirstTangentUsesHeading() && !pSolution.m_pObject->iFirstLoadedVirtualNodeIndex &&
		pSolution.m_pObject->GetNodeCount() >= 2) {
		PathingNode* pnode0 = pSolution.m_pObject->GetNode(0);
		PathingNode* pnode1 = pSolution.m_pObject->GetNode(1u);
		if (pnode0) {
			if (pnode1) {
				PathingLocation startLoc(*&pnode0->Location);
				PathingLocation nextLoc(*&pnode1->Location);
				NiPoint3* afRadius = &(result = startLoc.GetLocation());
				if ((nextLoc.GetLocation() - *afRadius).SqrLength() > 4096.0) {
					float afRadiusa = pRequest.m_pObject->fActorRadius;
					float afHeading = pRequest.m_pObject->fInitialPathHeading;
					Pathing::ComputeInitialTangentForActor(&tangent, afHeading, startLoc, nextLoc, afRadiusa);
					pnode0->SetTangent(tangent);
				}
			}
		}
	}
	if (bbuildDetailedPathRes) {
		if (pRequest.m_pObject->bFaceTargetAtGoal) {
			int32_t ShaderPropertyType = pSolution.m_pObject->iLastLoadedVirtualNodeIndex;
			if (ShaderPropertyType == pSolution.m_pObject->GetVirtualNodeCount() - 1) {
				PathingNode* pnodeN = pSolution.m_pObject->GetNode(pSolution.m_pObject->GetNodeCount() - 1);
				PathingNode* pnodeN1 = pSolution.m_pObject->GetNode(pSolution.m_pObject->GetNodeCount() - 2);
				if (pnodeN) {
					if (pnodeN1) {
						PathingLocation endLoc(*&pnodeN->Location);
						PathingLocation prevLoc(*&pnodeN1->Location);
						NiPoint3* afRadiusb = &(v17 = prevLoc.GetLocation());
						if ((endLoc.GetLocation() - *afRadiusb).SqrLength() > 4096.0) {
							PathingLocation goalLoc(*&pnodeN->Location);
							float afRadiusc = pRequest.m_pObject->fActorRadius;
							float Lifetime = pRequest.m_pObject->fGoalAngle;
							Pathing::ComputeFinalTangentForActor(&tangent_2, Lifetime, goalLoc, afRadiusc);
							pnodeN->SetTangent(tangent_2);
						}
					}
				}
			}
		}
	}
	return bbuildDetailedPathRes;
}

// The water heights are doubles; a start or goal point below its water height plus 25 skips the side checks. The side
// lines run from the start and goal moved by the actor's radius along the horizontal normal of the path, first to one
// side, then the other.
bool PathBuilder::TryBuildStraightPath(PathingLocation& aStartLocation, PathingLocation& aGoalLocation) {
	alignas(CheckLineOfSightFilterWater) uint8_t aFilterStorage[sizeof(CheckLineOfSightFilterWater)];
	CheckLineOfSightFilterWater& kFilter = *::new (aFilterStorage) CheckLineOfSightFilterWater;
	// Reads the radius before the setting.
	float fRadius = pRequest->GetActorRadius();
	kFilter.bIsLargeCreature = fPathingLargeActorRadius.Float() < fRadius;
	if (!Pathing::CheckLineOfSight(aStartLocation, aGoalLocation, nullptr, &kFilter))
		return false;

	TESObjectCELL* pCell = aStartLocation.GetCell();
	NiPoint3 kLocation = aStartLocation.GetLocation();
	double dWaterHeight = TES::pInstance->GetWaterHeight(kLocation, pCell) + 25.0;
	if (aStartLocation.GetLocation().z < dWaterHeight) {
		pSolution->AddPathingNode(aStartLocation, NiPoint3_ZERO);
		return true;
	}
	pCell = aGoalLocation.GetCell();
	kLocation = aGoalLocation.GetLocation();
	dWaterHeight = TES::pInstance->GetWaterHeight(kLocation, pCell) + 25.0;
	if (aGoalLocation.GetLocation().z < dWaterHeight) {
		pSolution->AddPathingNode(aStartLocation, NiPoint3_ZERO);
		return true;
	}

	NiPoint3 kStart = aStartLocation.GetLocation();
	NiPoint3 kDirection = aGoalLocation.GetLocation() - kStart;
	float fOffsetRadius = pRequest->GetActorRadius();
	NiPoint3 kOffset = kDirection.UnitCross(NiPoint3_UNIT_Z) * fOffsetRadius;

	alignas(PathingLocation) uint8_t aStartLeftStorage[sizeof(PathingLocation)];
	PathingLocation& kStartLeft =
		*::new (aStartLeftStorage) PathingLocation(aStartLocation.GetLocation() - kOffset, aStartLocation);
	alignas(PathingLocation) uint8_t aGoalLeftStorage[sizeof(PathingLocation)];
	PathingLocation& kGoalLeft =
		*::new (aGoalLeftStorage) PathingLocation(aGoalLocation.GetLocation() - kOffset, aGoalLocation);
	if (!Pathing::CheckLineOfSight(kStartLeft, kGoalLeft, nullptr, nullptr)) {
		kGoalLeft.~PathingLocation();
		kStartLeft.~PathingLocation();
		return false;
	}

	alignas(PathingLocation) uint8_t aStartRightStorage[sizeof(PathingLocation)];
	PathingLocation& kStartRight =
		*::new (aStartRightStorage) PathingLocation(aStartLocation.GetLocation() + kOffset, aStartLocation);
	alignas(PathingLocation) uint8_t aGoalRightStorage[sizeof(PathingLocation)];
	PathingLocation& kGoalRight =
		*::new (aGoalRightStorage) PathingLocation(aGoalLocation.GetLocation() + kOffset, aGoalLocation);
	bool bResult = false;
	if (Pathing::CheckLineOfSight(kStartRight, kGoalRight, nullptr, nullptr)) {
		pSolution->AddPathingNode(aStartLocation, NiPoint3_ZERO);
		bResult = true;
	}
	kGoalRight.~PathingLocation();
	kStartRight.~PathingLocation();
	kGoalLeft.~PathingLocation();
	kStartLeft.~PathingLocation();
	return bResult;
}

// The location is passed by value as a bitwise copy: the destructor of the by-value location only stores the vtable, so
// the copy leaves nothing to release twice.
bool PathBuilder::FindClosestOpenEdge(PathingLocation aStartLocation, PathingLocation& aRetLocation, bool abStart) {
	PathingLocation v15;
	NiPoint3 v21;
	NiPoint3 pt;
	int32_t apRetEdgeIndex;
	NiPoint3 arLocation;
	using v32_t = BSSimpleArray<const NavMeshInfo*, 1024>;
	float v34 = 3.4028235e+38f;
	uint32_t v36 = -1;
	uint16_t v35 = -1;
	arLocation = NiPoint3_ZERO;
	v32_t v32;
	aStartLocation.GetNavMeshInfos(v32);
	uint32_t FormFlags = v32.iSize;
	for (uint32_t uiIndex = 0; uiIndex < FormFlags; ++uiIndex) {
		NavMeshPtr apMemory;
		if (v32.GetAt(static_cast<int32_t>(uiIndex))->GetNavMesh(apMemory)) {
			uint16_t apRetTriIndex[2];
			float afRadius = pRequest.m_pObject->fTargetRadius;
			if (apMemory.m_pObject->NavMesh::FindClosestOpenEdgeForLocation(aStartLocation.GetLocation(), afRadius,
					apRetTriIndex, &apRetEdgeIndex, abStart)) {
				NavMeshEdgeVertices v25 =
					apMemory.m_pObject->NavMesh::GetEdgeVertices(apRetTriIndex[0], apRetEdgeIndex);
				NiPoint3 v26 = apMemory.m_pObject->NavMesh::GetCenter(apRetTriIndex[0]);
				pt = v26;
				NavMeshEdgeVertices v38 = v25;
				NiPoint3* afRadiusa = ::new (&v21) NiPoint3(apMemory.m_pObject->NavMesh::GetCenter(apRetTriIndex[0]));
				if (NavMeshMath::Find2DIntersectionPoint(aStartLocation.GetLocation(), *afRadiusa,
						v38.Vertices[0]->Location, v38.Vertices[1]->Location, &pt)) {
					NiPoint3 v18;
					pt.operator+=(*::new (&v18) NiPoint3((v26 - pt) * 0.1f));
				}
				float v23 = (pt - aStartLocation.GetLocation()).SqrLength();
				if (v34 > static_cast<double>(v23)) {
					v34 = v23;
					v36 = uiIndex;
					v35 = apRetTriIndex[0];
					arLocation = pt;
				}
			}
		}
	}
	if (v36 == -1) {
		return 0;
	} else {
		PathingLocation* aLoc =
			FNVD_RECONSTRUCT(PathingLocation)(&v15, arLocation, v32.GetAt(static_cast<int32_t>(v36)), v35);
		aRetLocation.operator=(*aLoc);
		return 1;
	}
}

bool PathBuilder::RunNavMeshSearch(PathingLocation& aStartLocation, PathingLocation& aGoalLocation,
	const BSSimpleArray<NavMeshSearchLink, 1024>* aBadNodes, BSSimpleArray<NavMeshSearchNode, 1024>* aNodes,
	BSSimpleArray<NavMeshSearchLink, 1024>* aLinks, int32_t aiFirstVirtualNodeIndex, int32_t aiLastVirtualNodeIndex) {
	alignas(NavMeshPtr) uint8_t kStartMeshMemory[sizeof(NavMeshPtr)];
	NavMeshPtr* pStartMesh = static_cast<NavMeshPtr*>(static_cast<void*>(kStartMeshMemory));
	::new (pStartMesh) NavMeshPtr;
	uint16_t usStartTriangle;
	if (!aStartLocation.GetNavMeshAndTriangle(*pStartMesh, usStartTriangle)) {
		pStartMesh->~NavMeshPtr();
		return false;
	}
	FastNavMeshTriHandle kStart;
	kStart.pNavMesh = pStartMesh->operator->();
	kStart.usTriangle = usStartTriangle;

	alignas(NavMeshPtr) uint8_t kGoalMeshMemory[sizeof(NavMeshPtr)];
	NavMeshPtr* pGoalMesh = static_cast<NavMeshPtr*>(static_cast<void*>(kGoalMeshMemory));
	::new (pGoalMesh) NavMeshPtr;
	uint16_t usGoalTriangle;
	if (!aGoalLocation.GetNavMeshAndTriangle(*pGoalMesh, usGoalTriangle)) {
		pGoalMesh->~NavMeshPtr();
		pStartMesh->~NavMeshPtr();
		return false;
	}
	FastNavMeshTriHandle kGoal;
	kGoal.pNavMesh = pGoalMesh->operator->();
	kGoal.usTriangle = usGoalTriangle;

	alignas(NavMeshSearch) uint8_t kSearchMemory[sizeof(NavMeshSearch)];
	NavMeshSearch* pSearch = static_cast<NavMeshSearch*>(static_cast<void*>(kSearchMemory));
	::new (pSearch) NavMeshSearch;
	pSearch->SetBadNodeArray(aBadNodes);
	// BuildNodePath reads only pNavMesh and usTriangle of each node, so the handles are passed reinterpreted as
	// NavMeshSearchNode.
	bool bFound = pSearch->BuildNodePath(pRequest.m_pObject, reinterpret_cast<const NavMeshSearchNode&>(kStart),
		reinterpret_cast<const NavMeshSearchNode&>(kGoal), aNodes, aLinks, &DetailedPathInfos);
	pSearch->~NavMeshSearch();
	pGoalMesh->~NavMeshPtr();
	pStartMesh->~NavMeshPtr();
	return bFound;
}

RAYCASTPATH_RESULT PathBuilder::BuildPathToNavMeshSearchStart(uint32_t aiFirstVirtualNodeIndex,
	BSSimpleArray<PathingLocation, 1024>& aRetPath, BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges) {
	VirtualPathingNode* pNode = pSolution.m_pObject->GetVirtualNode(aiFirstVirtualNodeIndex);
	if (aiFirstVirtualNodeIndex != 0 && !bPathUpdate) {
		VirtualPathingNode* pPrevNode = pSolution.m_pObject->GetVirtualNode(aiFirstVirtualNodeIndex - 1);
		uint32_t uiVirtualNodeCount = pSolution.m_pObject->GetCount();
		// The index and the count are compared signed.
		if (static_cast<int32_t>(aiFirstVirtualNodeIndex) <= static_cast<int32_t>(uiVirtualNodeCount)) {
			// Tests the current node in this branch and the previous one in the other.
			if (!pNode->HasActualLocation()) {
				alignas(PathingLocation) uint8_t aEntryMemory[sizeof(PathingLocation)];
				PathingLocation* pEntry = ::new (aEntryMemory) PathingLocation;
				if (FindEntryPoint(pPrevNode, pNode, *pEntry)) {
					aRetPath.Add(*pEntry);
					pEntry->~PathingLocation();
					return RCP_SUCCESS;
				}
				pEntry->~PathingLocation();
			}
		} else if (pPrevNode->HasActualLocation()) {
			alignas(PathingLocation) uint8_t aEntryMemory[sizeof(PathingLocation)];
			PathingLocation* pEntry = ::new (aEntryMemory) PathingLocation;
			if (FindEntryPoint(pPrevNode, pNode, *pEntry)) {
				aRetPath.Add(*pEntry);
				pEntry->~PathingLocation();
				return RCP_SUCCESS;
			}
			pEntry->~PathingLocation();
		}
	}
	if (!pNode->HasActualLocation())
		return RCP_FAILED;
	PathingLocation& kLocation = pNode->GetLocation();
	if (!kLocation.ResolveNavMeshInfo(nullptr))
		return RCP_FAILED;
	if (kLocation.HasNavMeshData()) {
		aRetPath.Add(kLocation);
		return RCP_SUCCESS;
	}
	Pathing::FIND_CLOSEST_EDGES_RESULT eClosestEdges = Pathing::FindClosestEdges(kLocation, 0xAu, aRetEdges);
	if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_ONTRI) {
		aRetPath.Add(kLocation);
		return RCP_SUCCESS;
	}
	if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_OK)
		return BuildPathToValidNavMeshTriangle(kLocation, aRetPath, true, aRetEdges);
	return RCP_FAILED;
}

// The portal triangles are 32-bit values that are read as 16-bit where a triangle index is passed, as the game does.
bool PathBuilder::FindEntryPoint(VirtualPathingNode* apPrevNode, VirtualPathingNode* apCurrentNode,
	PathingLocation& aRetLocation) {
	NiPoint3 v27;
	// v28 and apLocation are built and destroyed with the Construct and Destruct calls only.
	NiPoint3 v31;
	NiPoint3 v32;
	using portalTriangles_t = BSScrapArray<int32_t, 1024>;
	NiPoint3 point1;
	NiPoint3 point2;
	int32_t iedge;
	(&apCurrentNode->Location)->ResolveNavMeshInfoOnly();
	NavMeshPtr pcurrentMesh;
	const NavMeshInfo* BestNavMeshInfo = (&apCurrentNode->Location)->GetBestNavMeshInfo();
	if (!BestNavMeshInfo->GetNavMesh(pcurrentMesh)) {
		return 0;
	}
	if (apCurrentNode->HasActualLocation()) {
		uint16_t usTriIndex[2];
		const NiPoint3* p_ApproxLocation = &(&apCurrentNode->Location)->GetBestNavMeshInfo()->ApproxLocation;
		float fdist = ((&apPrevNode->Location)->GetLocation() - *p_ApproxLocation).Length();
		usTriIndex[0] = -1;
		iedge = -1;
		float afRadius = fdist + fdist;
		if (pcurrentMesh.m_pObject->FindClosestOpenEdgeForLocation((&apPrevNode->Location)->GetLocation(), afRadius,
				usTriIndex, &iedge, 1)) {
			pcurrentMesh.m_pObject->GetEdgeVertices(usTriIndex[0], iedge, &point1, &point2);
			NiPoint3 center = pcurrentMesh.m_pObject->GetCenter(usTriIndex[0]);
			uint16_t v21 = usTriIndex[0];
			NavMeshInfo* NavMeshInfo_ = pcurrentMesh.m_pObject->GetNavMeshInfo();
			NiPoint3* fScalar = ::new (&v32) NiPoint3(center * 0.1f);
			NiPoint3* v11 = ::new (&v31) NiPoint3((point1 + point2) * 0.45f + *fScalar);
			PathingLocation apLocation(*v11, NavMeshInfo_, v21);
			PathingLocation* aLoc = &apLocation;
			aRetLocation.operator=(*aLoc);
			return 1;
		}
	} else {
		portalTriangles_t portalTriangles(0, 0, pScrapHeap);
		const NavMeshInfo* v12 = (&apPrevNode->Location)->GetBestNavMeshInfo();
		pcurrentMesh.m_pObject->FindPortalTriangles(v12, &portalTriangles);
		uint32_t ibestTri = 0xFFFF;
		float fbestDist = 3.4028235e+38f;
		for (uint32_t i = 0;; ++i) {
			uint32_t FormFlags = portalTriangles.iSize;
			if (i >= FormFlags) {
				break;
			}
			NavMeshTriangle* ptri =
				pcurrentMesh.m_pObject
					->GetTriangle(static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i))));
			if (!(pRequest.m_pObject->fActorRadius > 150.0) || !ptri->IsNoLargeCreature()) {
				NiPoint3 triCenter =
					pcurrentMesh.m_pObject
						->GetCenter(static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i))));
				float fdist_2 = (triCenter - (&pRequest.m_pObject->Origin)->GetLocation()).Length();
				if (fbestDist > static_cast<double>(fdist_2)) {
					fdist_2 = fbestDist;
					ibestTri = portalTriangles.GetAt(static_cast<int32_t>(i));
				}
			}
		}
		if (ibestTri != 0xFFFF) {
			uint16_t v22 = static_cast<uint16_t>(ibestTri);
			const NavMeshInfo* v19 = (&apCurrentNode->Location)->GetBestNavMeshInfo();
			NiPoint3* v16 = ::new (&v27) NiPoint3(pcurrentMesh.m_pObject->GetCenter(static_cast<uint16_t>(ibestTri)));
			PathingLocation v28(*v16, v19, v22);
			PathingLocation* v23 = &v28;
			aRetLocation.operator=(*v23);
			return 1;
		}
	}
	return 0;
}

// KEEP: the exe builds FindClosestOpenEdge's by-value PathingLocation argument in the outgoing argument slot with
// PathingLocation's out-of-line copy constructor (0x6DCC40), which writes the exe's vtable, and the callee destroys
// it there. src's PathingLocation has only the implicit copy constructor, which writes src's own vtable pointer into
// the argument; giving it a copy constructor that calls 0x6DCC40 (the form the NiPointer arguments use) would change
// every implicit copy of PathingLocation and of the classes holding one. So src builds the copy in a local and
// passes that.
// The exe's cleanup-only C++ EH frame is left out.
RAYCASTPATH_RESULT PathBuilder::BuildPathToValidNavMeshTriangle(PathingLocation& aStartLocation,
	BSSimpleArray<PathingLocation, 1024>& aRetPath, bool abStart, BSSimpleArray<NavMeshEdgeLocation, 1024>& aRetEdges) {
	if (bUseRayCasts.Bool()) {
		Pathing::FIND_CLOSEST_EDGES_RESULT eClosestEdges = Pathing::FindClosestEdges(aStartLocation, 10u, aRetEdges);
		if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_NONE) {
			return RCP_FAILED;
		}
		if (eClosestEdges == Pathing::FIND_CLOSEST_EDGES_ONTRI) {
			aRetPath.Add(aStartLocation);
			return RCP_SUCCESS;
		}
		if (eClosestEdges != Pathing::FIND_CLOSEST_EDGES_OK) {
			return RCP_FAILED;
		}
		BSTaskletManager* pTaskletManager = &BSTaskletManager::QInstance();
		if (pTaskletManager->IsTaskletThread(BSGetCurrentThreadID())) {
			return PathManager::QInstance()->GetRayCastPathToNavmesh(pRequest.m_pObject, aRetEdges, &aStartLocation,
				aRetPath, abStart);
		}
		RayCastPathRequest rayCastRequest;
		rayCastRequest.GoalLocation.operator=(aStartLocation);
		rayCastRequest.eType = static_cast<RAYCAST_REQUEST_TYPE>(!abStart);
		rayCastRequest.pInitialPathRequest = pRequest.m_pObject;
		aRetEdges.CopyTo(&rayCastRequest.Edges);
		RayCastPath retPath;
		PathingSearchRayCast rayCastSearch;
		RAYCASTPATH_RESULT eResult = rayCastSearch.BuildPath(rayCastRequest, retPath);
		while (eResult == RCP_NONE) {
			eResult = rayCastSearch.BuildPath(rayCastRequest, retPath);
		}
		RAYCASTPATH_RESULT eReturn = RCP_FAILED;
		if (eResult == RCP_SUCCESS) {
			retPath.Nodes.CopyTo(&aRetPath);
			eReturn = RCP_SUCCESS;
		}
		return eReturn;
	}
	PathingLocation goalLoc;
	FNVD_INLINED_LOCAL(PathingLocation, startArgument, std::nullopt, std::nullopt);
	::new (&startArgument) PathingLocation(aStartLocation);
	RAYCASTPATH_RESULT eReturn = RCP_FAILED;
	if (FindClosestOpenEdge(startArgument, goalLoc, abStart)) {
		aRetPath.Add(aStartLocation);
		aRetPath.Add(goalLoc);
		eReturn = RCP_SUCCESS;
	}
	return eReturn;
}

bool PathBuilder::CheckClosestEdgeToGoal(uint32_t aiLastVirtualNodeIndex,
	const BSSimpleArray<NavMeshEdgeLocation, 1024>& aEndEdges, BSSimpleArray<PathingLocation, 1024>& aEndNodes) {
	alignas(NavMeshEdgeLocation) uint8_t aEdgeStorage[sizeof(NavMeshEdgeLocation)];
	NavMeshEdgeLocation* pEdge = ::new (aEdgeStorage) NavMeshEdgeLocation(aEndEdges.GetFirst());
	alignas(NiPoint3) uint8_t aVert1Storage[sizeof(NiPoint3)];
	NiPoint3* pVert1 = ::new (aVert1Storage) NiPoint3;
	alignas(NiPoint3) uint8_t aVert2Storage[sizeof(NiPoint3)];
	NiPoint3* pVert2 = ::new (aVert2Storage) NiPoint3;
	alignas(NavMeshPtr) uint8_t aNavMeshStorage[sizeof(NavMeshPtr)];
	NavMeshPtr* pspNavMesh = ::new (aNavMeshStorage) NavMeshPtr;
	bool bResult = false;
	if (pEdge->pNavMeshInfo->GetNavMesh(*pspNavMesh)) {
		pspNavMesh->operator->()->GetEdgeVertices(pEdge->usTriangle, pEdge->iEdgeIndex, pVert1, pVert2);
		NiPoint3 kNodeLocation =
			pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex)->GetLocation().GetLocation();
		NiPoint3 kPoint = NavMeshMath::ProjectPointOntoSegment(kNodeLocation, *pVert1, *pVert2);
		// The squared distance and the first radius are kept as doubles; the product is a double.
		double dSqrDistance = (kPoint - kNodeLocation).SqrXYLength();
		double dRadius = pRequest.m_pObject->GetTargetRadius();
		if (pRequest.m_pObject->GetTargetRadius() * dRadius > dSqrDistance && (kPoint - kNodeLocation).z > -64.0 &&
			(kPoint - kNodeLocation).z < 180.0) {
			NiPoint3 kCenter = pspNavMesh->operator->()->GetCenter(pEdge->usTriangle);
			kPoint += (kCenter - kPoint) * 0.1f;
			aEndNodes.Clear(false);
			alignas(PathingLocation) uint8_t aLocationStorage[sizeof(PathingLocation)];
			PathingLocation* pLocation =
				::new (aLocationStorage) PathingLocation(kPoint, pEdge->pNavMeshInfo, pEdge->usTriangle);
			aEndNodes.Add(*pLocation);
			bResult = true;
			pLocation->~PathingLocation();
		}
	}
	pspNavMesh->~NavMeshPtr();
	return bResult;
}

bool PathBuilder::FindLoadedNodeIndices(uint32_t aiFirstHighLevelNode, int32_t* apFirstLoadedNodeIndex,
	int32_t* apLastLoadedNodeIndex) {
	VirtualPathingNode* VirtualNode = pSolution.m_pObject->GetVirtualNode(aiFirstHighLevelNode);
	TESObjectCELL* pcell = (&VirtualNode->Location)->GetCell();
	if (!pcell || !pcell->GetAttached()) {
		return 0;
	}
	*apFirstLoadedNodeIndex = aiFirstHighLevelNode;
	// The index is compared with the node count signed.
	for (*apLastLoadedNodeIndex = *apFirstLoadedNodeIndex + 1;
		*apLastLoadedNodeIndex < static_cast<int32_t>(pSolution.m_pObject->GetVirtualNodeCount());
		++*apLastLoadedNodeIndex) {
		VirtualPathingNode* v6 = pSolution.m_pObject->GetVirtualNode(*apLastLoadedNodeIndex);
		TESObjectCELL* pcell_2 = (&v6->Location)->GetCell();
		if (!pcell_2 || !pcell_2->GetAttached()) {
			break;
		}
		VirtualPathingNode* v7 = pSolution.m_pObject->GetVirtualNode(*apLastLoadedNodeIndex);
		if (v7->IsTransitionNode()) {
			++*apLastLoadedNodeIndex;
			break;
		}
	}
	--*apLastLoadedNodeIndex;
	return 1;
}

bool PathBuilder::FindVirtualNodeSegments(BSSimpleArray<PathBuilder::VirtualNodeSegment, 1024>& aRetSegmentArray) {
	NavMeshInfo* pnextInfo;
	using NodesHaveNavMeshInfo_t = BSScrapArray<bool, 1024>;
	aRetSegmentArray.Clear(0);
	int32_t ifirstLoadedVirtualNode = pSolution.m_pObject->iFirstLoadedVirtualNodeIndex;
	int32_t ilastLoadedVirtualNode = pSolution.m_pObject->iLastLoadedVirtualNodeIndex;
	NodesHaveNavMeshInfo_t NodesHaveNavMeshInfo(ilastLoadedVirtualNode - ifirstLoadedVirtualNode + 1, 0, pScrapHeap);
	for (int32_t i = ifirstLoadedVirtualNode; i <= ilastLoadedVirtualNode; ++i) {
		VirtualPathingNode* pvnode = pSolution.m_pObject->GetVirtualNode(static_cast<uint32_t>(i));
		bool aItem = pvnode->HasNavMesh();
		NodesHaveNavMeshInfo.BSSimpleArray<bool, 1024>::Add(aItem);
	}
	int32_t nodeIndex = ifirstLoadedVirtualNode;
	while (nodeIndex <= ilastLoadedVirtualNode) {
		bool aValue = 1;
		int32_t firstNodeWithInfoIndex = NodesHaveNavMeshInfo.BSSimpleArray<bool, 1024>::Find(aValue,
			nodeIndex - ifirstLoadedVirtualNode, &BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress);
		if (firstNodeWithInfoIndex == -1) {
			PathBuilder::VirtualNodeSegment v8;
			v8.iFirstVirtualNode = nodeIndex;
			v8.iLastVirtualNode = ilastLoadedVirtualNode;
			v8.bHasNavMeshes = 0;
			aRetSegmentArray.Add(v8);
			nodeIndex = ilastLoadedVirtualNode + 1;
		} else {
			PathBuilder::VirtualNodeSegment v17;
			bool v5 = 0;
			int32_t v2 = NodesHaveNavMeshInfo.BSSimpleArray<bool, 1024>::Find(v5, firstNodeWithInfoIndex,
				&BSSimpleArray<bool, 1024>::DefaultEqualFunctionByAddress);
			int32_t lastNodeWithInfoIndex = v2 - 1;
			if (v2 == -1) {
				lastNodeWithInfoIndex = NodesHaveNavMeshInfo.iSize - 1;
			}
			firstNodeWithInfoIndex += ifirstLoadedVirtualNode;
			lastNodeWithInfoIndex += ifirstLoadedVirtualNode;
			if (firstNodeWithInfoIndex > nodeIndex) {
				PathBuilder::VirtualNodeSegment a2;
				a2.iFirstVirtualNode = nodeIndex;
				a2.iLastVirtualNode = firstNodeWithInfoIndex - 1;
				a2.bHasNavMeshes = 0;
				aRetSegmentArray.Add(a2);
			}
			int32_t icurrentsubSegmentFirstNode = firstNodeWithInfoIndex;
			for (int32_t icurrentNode = firstNodeWithInfoIndex; icurrentNode + 1 <= lastNodeWithInfoIndex;
				++icurrentNode) {
				VirtualPathingNode* pthisNode =
					pSolution.m_pObject->GetVirtualNode(static_cast<uint32_t>(icurrentNode));
				VirtualPathingNode* pnextNode = pSolution.m_pObject->GetVirtualNode(icurrentNode + 1);
				(&pthisNode->Location)->ResolveNavMeshInfoOnly();
				(&pnextNode->Location)->ResolveNavMeshInfoOnly();
				const NavMeshInfo* pthisInfo = (&pthisNode->Location)->GetBestNavMeshInfo();
				// The adjacency arrays hold non-const infos, so this local's address is passed to their Find.
				pnextInfo = const_cast<NavMeshInfo*>((&pnextNode->Location)->GetBestNavMeshInfo());
				if (!pthisInfo || !pnextInfo) {
					return 0;
				}
				if (pthisInfo != pnextInfo &&
					pthisInfo->AdjacentNavMeshes.Find(pnextInfo, 0,
						&BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress) == -1 &&
					pthisInfo->PreferredAdjacentNavMeshes.Find(pnextInfo, 0,
						&BSSimpleArray<NavMeshInfo*>::DefaultEqualFunctionByAddress) == -1) {
					PathBuilder::VirtualNodeSegment v9;
					v9.iFirstVirtualNode = icurrentsubSegmentFirstNode;
					v9.iLastVirtualNode = icurrentNode;
					v9.bHasNavMeshes = 1;
					aRetSegmentArray.Add(v9);
					icurrentsubSegmentFirstNode = icurrentNode + 1;
				}
			}
			v17.iFirstVirtualNode = icurrentsubSegmentFirstNode;
			v17.iLastVirtualNode = lastNodeWithInfoIndex;
			v17.bHasNavMeshes = 1;
			aRetSegmentArray.Add(v17);
			nodeIndex = lastNodeWithInfoIndex + 1;
		}
	}
	return 1;
}

bool PathBuilder::BuildDetailedPathForLoadedVirtualNodes(Actor* pActor) {
	using segments_t = BSScrapArray<PathBuilder::VirtualNodeSegment, 1024>;
	segments_t segments(0, 0, pScrapHeap);
	if (!FindVirtualNodeSegments(segments)) {
		return false;
	}
	bool bGoDirectIfPathFails = pRequest.m_pObject->GetGoDirectIfPathingFails();
	for (uint32_t i = 0; i < segments.iSize; ++i) {
		bool bSegmentBuilt = false;
		PathBuilder::VirtualNodeSegment* pSegment = &segments.GetAt(static_cast<int32_t>(i));
		if (pSegment->bHasNavMeshes) {
			if (i && segments.GetAt(static_cast<int32_t>(i - 1)).bHasNavMeshes && !bGoDirectIfPathFails) {
				pSolution.m_pObject->SetIncomplete(1);
				return false;
			}
			bSegmentBuilt = BuildNavMeshPathForVirtualNodes(static_cast<uint32_t>(pSegment->iFirstVirtualNode),
				static_cast<uint32_t>(pSegment->iLastVirtualNode));
			if (!bSegmentBuilt && !bGoDirectIfPathFails) {
				return false;
			}
		}
		if (!bSegmentBuilt &&
			!BuildStraightPathForVirtualNodes(static_cast<uint32_t>(pSegment->iFirstVirtualNode),
				static_cast<uint32_t>(pSegment->iLastVirtualNode))) {
			return false;
		}
	}
	return true;
}

bool PathBuilder::RunNavMeshSearch_006CD670(PathingLocation& aStartLocation, PathingLocation& aGoalLocation,
	const BSSimpleArray<NavMeshSearchLink, 1024>* aBadNodes, BSSimpleArray<NavMeshSearchNode, 1024>& aNodes,
	BSSimpleArray<NavMeshSearchLink, 1024>& aLinks, uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex) {
	FNVD_INLINED_LOCAL(NiPointer<NavMesh>, apMemory, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(NiPointer<NavMesh>, v11, std::nullopt, std::nullopt);
	uint16_t apusTriangle;
	FNVD_INLINED_LOCAL(NavMeshSearch, apThis, std::nullopt, std::nullopt);
	uint16_t arTriangle;
	::new (&apMemory) NavMeshPtr;
	if (aStartLocation.GetNavMeshAndTriangle(static_cast<NavMeshPtr&>(apMemory), arTriangle)) {
		NavMeshSearchNode apStart;
		apStart.pNavMesh = apMemory.m_pObject;
		apStart.sTriangle = arTriangle;
		::new (&v11) NavMeshPtr;
		if (aGoalLocation.GetNavMeshAndTriangle(static_cast<NavMeshPtr&>(v11), apusTriangle)) {
			NavMeshSearchNode apGoal;
			apGoal.pNavMesh = v11.m_pObject;
			apGoal.sTriangle = apusTriangle;
			::new (&apThis) NavMeshSearch;
			apThis.SetBadNodeArray(aBadNodes);
			bool v16 = apThis.BuildNodePath(pRequest.m_pObject, apStart, apGoal, &aNodes, &aLinks, &DetailedPathInfos);
			if (v16) {
				apThis.~NavMeshSearch();
				v11.~NiPointer<NavMesh>();
				apMemory.~NiPointer<NavMesh>();
				return 1;
			} else {
				apThis.~NavMeshSearch();
				v11.~NiPointer<NavMesh>();
				apMemory.~NiPointer<NavMesh>();
				return 0;
			}
		} else {
			v11.~NiPointer<NavMesh>();
			apMemory.~NiPointer<NavMesh>();
			return 0;
		}
	} else {
		apMemory.~NiPointer<NavMesh>();
		return 0;
	}
}

bool PathBuilder::RunPathSmoother(PathingLocation& aStartLocation, PathingLocation& aGoalLocation,
	BSSimpleArray<NavMeshSearchLink, 1024>& aBadNodes, const BSSimpleArray<NavMeshSearchNode, 1024>& aNodes,
	const BSSimpleArray<NavMeshSearchLink, 1024>& aLinks) {
	NiPoint3 v20;
	NiPoint3 result;
	FastNavMeshTriHandle kTriHandle;
	using triangles_t = BSScrapArray<FastNavMeshTriHandle, 1024>;
	if (bUseAlternateSmoothingForPrime.Bool() && pRequest.m_pObject->fActorRadius > 150.0) {
		ScrapHeap* pScrapHeap = this->pScrapHeap;
		uint32_t FormFlags = aNodes.iSize;
		triangles_t kTriangles(FormFlags, 0, pScrapHeap);
		for (uint32_t i = 0;; ++i) {
			uint32_t v7 = aNodes.iSize;
			if (i >= v7) {
				break;
			}
			uint16_t sTriangle = (&aNodes.GetAt(static_cast<int32_t>(i)))->sTriangle;
			FastNavMeshTriHandle* v8 = FNVD_RECONSTRUCT(FastNavMeshTriHandle)(&kTriHandle,
				(&aNodes.GetAt(static_cast<int32_t>(i)))->pNavMesh, sTriangle);
			kTriangles.Add(*v8);
		}
		NavMeshPOVSearch povSearch;
		bool v25 =
			povSearch.BuildPath(aStartLocation, aGoalLocation, &kTriangles, pRequest.m_pObject, pSolution.m_pObject);
		return v25;
	} else {
		if (bUseOldPathSmoothing.Bool()) {
			PathSmoother pathSmoother;
			PathingSolution* m_pObject = pSolution.m_pObject;
			NiPoint3* Location_ = ::new (&result) NiPoint3(aGoalLocation.GetLocation());
			bool v24 = pathSmoother.SmoothPath(pRequest.m_pObject, aNodes, aLinks, aStartLocation.GetLocation(),
				*Location_, m_pObject, &aBadNodes, 1);
			return v24;
		} else {
			PathSmootherPOVSearch pathSmoother_2;
			PathingSolution* v15 = pSolution.m_pObject;
			NiPoint3* v13 = ::new (&v20) NiPoint3(aGoalLocation.GetLocation());
			bool v21 = pathSmoother_2.SmoothPath(pRequest.m_pObject, aNodes, aLinks, aStartLocation.GetLocation(), *v13,
				v15, &aBadNodes, &DetailedPathInfos);
			return v21;
		}
	}
}

// The portal triangles are 32-bit values that are read as 16-bit where a triangle index is passed, as the game does.
bool PathBuilder::FindExitPoint(VirtualPathingNode* apCurrentNode, VirtualPathingNode* apNextNode,
	PathingLocation& aRetLocation) {
	NiPoint3 v31;
	NiPoint3 loc_2;
	using portalTriangles_t = BSScrapArray<int32_t, 1024>;
	NiPoint3 point1;
	NiPoint3 loc;
	NiPoint3 point2;
	int32_t iedge;
	(&apCurrentNode->Location)->ResolveNavMeshInfoOnly();
	NavMeshPtr pcurrentMesh;
	const NavMeshInfo* BestNavMeshInfo = (&apCurrentNode->Location)->GetBestNavMeshInfo();
	if (!BestNavMeshInfo->GetNavMesh(pcurrentMesh)) {
		return 0;
	}
	if (apCurrentNode->HasActualLocation()) {
		uint16_t usTriIndex[2];
		const NiPoint3* fScalar = &(&apCurrentNode->Location)->GetBestNavMeshInfo()->ApproxLocation;
		float fdist = ((&apNextNode->Location)->GetLocation() - *fScalar).Length();
		usTriIndex[0] = -1;
		iedge = -1;
		float afRadius = fdist + fdist;
		if (pcurrentMesh.m_pObject->FindClosestOpenEdgeForLocation((&apNextNode->Location)->GetLocation(), afRadius,
				usTriIndex, &iedge, 0)) {
			pcurrentMesh.m_pObject->GetEdgeVertices(usTriIndex[0], iedge, &point1, &point2);
			NiPoint3 center = pcurrentMesh.m_pObject->GetCenter(usTriIndex[0]);
			NiPoint3* fScalara = ::new (&v31) NiPoint3(center * 0.1f);
			::new (&loc) NiPoint3((point1 + point2) * 0.45f + *fScalara);
			TESObjectCELL* Target = pcurrentMesh.m_pObject->GetParentCell();
			float fwaterHeight = TES::pInstance->GetWaterHeight(loc, Target);
			if (loc.z < fwaterHeight + 25.0) {
				loc.z = fwaterHeight;
			}
			uint16_t fScalarb = usTriIndex[0];
			NavMeshInfo* NavMeshInfo_ = pcurrentMesh.m_pObject->GetNavMeshInfo();
			PathingLocation apLocation(loc, NavMeshInfo_, fScalarb);
			PathingLocation* aLoc = &apLocation;
			aRetLocation.operator=(*aLoc);
			return 1;
		}
		return 0;
	}
	portalTriangles_t portalTriangles(0, 0, pScrapHeap);
	const NavMeshInfo* v13 = (&apNextNode->Location)->GetBestNavMeshInfo();
	pcurrentMesh.m_pObject->FindPortalTriangles(v13, &portalTriangles);
	int32_t ibestTri = 0xFFFF;
	float fbestDist = 3.4028235e+38f;
	for (uint32_t i = 0;; ++i) {
		uint32_t FormFlags = portalTriangles.iSize;
		if (i >= FormFlags) {
			break;
		}
		NavMeshTriangle* ptri =
			pcurrentMesh.m_pObject->GetTriangle(static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i))));
		if (!(pRequest.m_pObject->fActorRadius > 150.0) || !ptri->IsNoLargeCreature()) {
			NiPoint3 triCenter = pcurrentMesh.m_pObject
									 ->GetCenter(static_cast<uint16_t>(portalTriangles.GetAt(static_cast<int32_t>(i))));
			float fdist_2 = (triCenter - (&pRequest.m_pObject->Destination)->GetLocation()).Length();
			if (fbestDist > static_cast<double>(fdist_2)) {
				fdist_2 = fbestDist;
				ibestTri = portalTriangles.GetAt(static_cast<int32_t>(i));
			}
		}
	}
	if (ibestTri == 0xFFFF) {
		return 0;
	}
	::new (&loc_2) NiPoint3(pcurrentMesh.m_pObject->GetCenter(static_cast<uint16_t>(ibestTri)));
	TESObjectCELL* v17 = pcurrentMesh.m_pObject->GetParentCell();
	float fwaterHeight_2 = TES::pInstance->GetWaterHeight(loc_2, v17);
	if (loc_2.z < fwaterHeight_2 + 25.0) {
		loc_2.z = fwaterHeight_2;
	}
	uint16_t fScalarc = static_cast<uint16_t>(ibestTri);
	const NavMeshInfo* v18 = (&apCurrentNode->Location)->GetBestNavMeshInfo();
	PathingLocation v27(loc_2, v18, fScalarc);
	PathingLocation* v23 = &v27;
	aRetLocation.operator=(*v23);
	return 1;
}

bool PathBuilder::BuildNavMeshInfoPathInSpace(const NiPoint3& aStartLocation, const NiPoint3& aGoalLocation,
	ParentSpaceNode* pSpace, TESObjectREFR* apTeleportDoor) {
	PathingLocation startLoc(&aStartLocation, pSpace->pInteriorCell, pSpace->pWorldSpace);
	PathingLocation goalLoc(&aGoalLocation, pSpace->pInteriorCell, pSpace->pWorldSpace);
	if (!startLoc.ResolveNavMeshInfoOnly() || !goalLoc.ResolveNavMeshInfoOnly()) {
		return 0;
	}
	using NavMeshInfoArray = BSScrapArray<const NavMeshInfo*, 1024>;
	using PortalArray = BSScrapArray<TESObjectREFR*, 1024>;
	NavMeshInfoSearch infoSearch;
	NavMeshInfoArray returnNodes(20u, 0u, pScrapHeap);
	PortalArray portals(20u, 0u, pScrapHeap);
	ActorLockData* plockData = 0;
	// Pathfind's arguments are evaluated right to left, as the game does: the goal's before the start's.
	const NavMeshInfo* pgoalMesh = goalLoc.GetBestNavMeshInfo();
	NiPoint3 goalPoint = goalLoc.GetLocation();
	const NavMeshInfo* pstartMesh = startLoc.GetBestNavMeshInfo();
	NiPoint3 startPoint = startLoc.GetLocation();
	bool binfosearchRes = infoSearch.Pathfind(plockData, startPoint, pstartMesh, goalPoint, pgoalMesh, &returnNodes,
		&portals, Pathing::DOOR_LOCK_DISPOSITION_NORMAL);
	if (!binfosearchRes) {
		if (returnNodes.IsEmpty() || !pRequest.m_pObject->bAllowIncompletePath) {
			return 0;
		}
		pSolution.m_pObject->SetIncomplete(1);
	}
	pSolution.m_pObject->AddVirtualPathingNode(startLoc);
	for (uint32_t i = 1; i < returnNodes.iSize - 1; ++i) {
		pSolution.m_pObject->AddVirtualPathingNode_3(returnNodes.GetAt(static_cast<int32_t>(i)));
	}
	if (binfosearchRes) {
		if (apTeleportDoor) {
			pSolution.m_pObject->AddVirtualPathingNode(goalLoc, apTeleportDoor);
		} else {
			pSolution.m_pObject->AddVirtualPathingNode(goalLoc);
		}
	} else {
		// GetTail gives the last item's address; it is called once for the info and once for its location.
		const NavMeshInfo* plastInfo = *static_cast<const NavMeshInfo**>(returnNodes.GetTail());
		const NavMeshInfo* ptailInfo = *static_cast<const NavMeshInfo**>(returnNodes.GetTail());
		pSolution.m_pObject->AddVirtualPathingNode_0(ptailInfo->ApproxLocation, plastInfo);
	}
	return binfosearchRes;
}

bool PathBuilder::BuildStraightPathForVirtualNodes(uint32_t aiFirstVirtualNodeIndex, uint32_t aiLastVirtualNodeIndex) {
	for (uint32_t i = aiFirstVirtualNodeIndex; i <= aiLastVirtualNodeIndex; ++i) {
		VirtualPathingNode* VirtualNode = pSolution.m_pObject->GetVirtualNode(i);
		PathingLocation loc(*&VirtualNode->Location);
		VirtualPathingNode* v4 = pSolution.m_pObject->GetVirtualNode(i);
		if (!v4->HasActualLocation()) {
			const NavMeshInfo* BestNavMeshInfo = loc.GetBestNavMeshInfo();
			loc.SetLocation(BestNavMeshInfo->ApproxLocation);
		}
		pSolution.m_pObject->AddPathingNode(loc, NiPoint3_ZERO);
	}
	VirtualPathingNode* v6 = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex);
	if (v6->IsTransitionNode()) {
		TESObjectREFR* Location_ = pSolution.m_pObject->GetVirtualNode(aiLastVirtualNodeIndex)->pDoorRef;
		PathingNode* LastNode = pSolution.m_pObject->GetLastNode();
		LastNode->SetTeleportDoor(Location_);
	}
	return 1;
}

RayCastPath::RayCastPath() {}

RayCastPath::~RayCastPath() {}

void RayCastPath::operator delete(void* apMemory) {
	ourFree(apMemory);
}

NavMeshSearch* NavMeshSearch::SetBadNodeArray(const BSSimpleArray<NavMeshSearchLink, 1024>* apBadNodeArray) {
	pBadNodes = apBadNodeArray;
	return this;
}

bool PathingRequest::GetFirstTangentUsesHeading() const {
	return bFirstTangentUsesHeading;
}

float PathingRequest::GetInitialPathHeading() const {
	return fInitialPathHeading;
}

PathingLocation& VirtualPathingNode::GetLocation() {
	return Location;
}

// Empties the detailed path, releasing its buffer, and forgets the loaded virtual node range.
void PathingSolution::ClearCurrentDetailedPath() {
	CurrentPathingNodes.Clear(true);
	iFirstLoadedVirtualNodeIndex = -1;
	iLastLoadedVirtualNodeIndex = -1;
}

uint32_t PathingSolution::GetVirtualNodeCount() {
	return VirtualPathingNodes.iSize;
}

uint32_t PathingSolution::AddVirtualPathingNode(const PathingLocation& apLocation) {
	uint32_t auiIndex = VirtualPathingNodes.AddUninitialized();
	VirtualPathingNode* apWhere = &VirtualPathingNodes.GetAt(auiIndex);
	VirtualPathingNode* apMemory = static_cast<VirtualPathingNode*>(::operator new(48u, apWhere));
	if (apMemory) {
		::new (apMemory) VirtualPathingNode(apLocation);
	}
	return auiIndex;
}

uint32_t PathingSolution::AddVirtualPathingNode(const PathingLocation& aLocation, TESObjectREFR* apDoor) {
	VirtualPathingNode* v4;
	uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
	VirtualPathingNode* pnode = &VirtualPathingNodes.GetAt(static_cast<int32_t>(inewNodeIndex));
	VirtualPathingNode* apMemory = static_cast<VirtualPathingNode*>(::operator new(48u, pnode));
	if (apMemory) {
		v4 = ::new (apMemory) VirtualPathingNode(&aLocation, apDoor);
	} else {
		v4 = 0;
	}
	if (apDoor) {
		uint32_t a2 = apDoor->iFormID;
		DoorIDs.Add(a2);
	}
	return inewNodeIndex;
}

void PathingSolution::RemoveNode(uint32_t aiIndex) {
	CurrentPathingNodes.Remove(aiIndex, true);
}

void PathingSolution::RemoveLastNode() {
	CurrentPathingNodes.Remove(CurrentPathingNodes.GetSize() - 1, true);
}

uint32_t PathingSolution::AddVirtualPathingNode_3(const NavMeshInfo* apNavMeshInfo) {
	uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
	VirtualPathingNode* pnode = &VirtualPathingNodes.GetAt(inewNodeIndex);
	VirtualPathingNode* apMemory = static_cast<VirtualPathingNode*>(::operator new(48u, pnode));
	if (apMemory) {
		::new (apMemory) VirtualPathingNode(apNavMeshInfo);
	}
	return inewNodeIndex;
}

uint32_t PathingSolution::AddVirtualPathingNode_0(const NiPoint3& aLocation, const NavMeshInfo* apNavMeshInfo) {
	uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
	VirtualPathingNode* pnode = &VirtualPathingNodes.GetAt(inewNodeIndex);
	VirtualPathingNode* apMemory = static_cast<VirtualPathingNode*>(::operator new(48u, pnode));
	if (apMemory) {
		::new (apMemory) VirtualPathingNode(&aLocation, apNavMeshInfo);
	}
	return inewNodeIndex;
}

uint32_t PathingSolution::AddVirtualPathingNode_006D1520(const NiPoint3& aLocation, TESObjectCELL* apCell,
	TESWorldSpace* apWorldSpace) {
	PathingLocation loc(static_cast<const NiPoint3*>(&aLocation), apCell, apWorldSpace);
	loc.ResolveNavMeshInfo(0);
	uint32_t inewNodeIndex = VirtualPathingNodes.AddUninitialized();
	VirtualPathingNode* pnode = &VirtualPathingNodes.GetAt(inewNodeIndex);
	VirtualPathingNode* apMemory = static_cast<VirtualPathingNode*>(::operator new(48u, pnode));
	if (apMemory) {
		::new (apMemory) VirtualPathingNode(loc);
	}
	return inewNodeIndex;
}
