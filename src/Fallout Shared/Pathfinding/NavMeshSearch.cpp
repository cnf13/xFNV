#include "Fallout Shared/Pathfinding/NavMeshSearch.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSTaskletManager.h"
#include "Fallout Shared/DoorLock.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/LockFreeMap.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/AngleSinCos.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarNodeHeap.h"
#include "Fallout Shared/Pathfinding/AStarQueue.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout Shared/Pathfinding/FastNavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshHandles.h"
#include "Fallout Shared/Pathfinding/NavMeshInfo.h"
#include "Fallout Shared/Pathfinding/NavMeshPOVSearch.h"
#include "Fallout Shared/Pathfinding/NavMeshStaticAvoidNode.h"
#include "Fallout Shared/Pathfinding/NavMeshTriangle.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

#include <cmath>
#include <stdint.h>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> fPathingLargeActorRadius("fPathingLargeActorRadius", 80.0f);

using AStarNodeArray_NavMeshSearch = BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>;
using AStarNodeScrapArray_NavMeshSearch = BSScrapArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>;

NiPoint3* NavMeshSearchNode::GetCenter(NiPoint3* apCenter) const {
	::new (apCenter) NiPoint3(CenterX, CenterY, CenterZ);
	return apCenter;
}

NavMeshTriangle* NavMeshSearchNode::GetTriangle() const {
	return pNavMesh->GetTriangle(sTriangle);
}

int32_t AStarSearch_GetOrCreateAStarNode_6A7E00(AStarSearch<NavMeshSearchNode, NavMeshSearchLink>* apThis,
	uintptr_t aiKey, float afCost, AStarNode<NavMeshSearchNode, NavMeshSearchLink>** apRetNode) {
	AStarNode<NavMeshSearchNode, NavMeshSearchLink>** pBuckHead =
		&apThis->kNodeMap.Buckets[(apThis->kNodeMap.GetBucketFromKey(aiKey))];
	if (*pBuckHead) {
		if ((*pBuckHead)->iKey == aiKey) {
			if (!(afCost < (*pBuckHead)->fCost)) {
				return 2;
			}
			*apRetNode = *pBuckHead;
			apThis->kSearchQueue.RemoveNode(*apRetNode);
			(*apRetNode)->fCost = afCost;
			return 1;
		}
		for (AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pcurrent = (*pBuckHead)->pNextNodeInBucket; pcurrent;
			pcurrent = pcurrent->pNextNodeInBucket) {
			if (pcurrent->iKey == aiKey) {
				if (!(afCost < pcurrent->fCost)) {
					return 2;
				}
				*apRetNode = pcurrent;
				apThis->kSearchQueue.RemoveNode(*apRetNode);
				(*apRetNode)->fCost = afCost;
				return 1;
			}
		}
	}
	*apRetNode = apThis->kNodeHeap.AllocateNode();
	(*apRetNode)->iKey = aiKey;
	(*apRetNode)->fCost = afCost;
	(*apRetNode)->pNextNodeInBucket = *pBuckHead;
	*pBuckHead = *apRetNode;
	return 0;
}

void ConstructNodes(NavMeshSearchNode* apNodes, uint32_t auiCount) {
	for (uint32_t i = 0; i < auiCount; ++i) {
	}
}

NavMeshSearch::NavMeshSearch() : pCurrentRequest(nullptr), pBadNodes(nullptr), pHighLevelSolution(nullptr) {}

NavMeshSearch::~NavMeshSearch() {}

bool NavMeshSearch::GetNodeConnections(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode,
	BSSimpleArray<AStarNode<NavMeshSearchNode, NavMeshSearchLink>*, 1024>* apRetNodes) {
	NiPoint3 kGoalCenter2;
	NiPoint3 kGoalCenter;
	NiPoint3 kStartCenter;
	NiPoint3 kTargetPoint;
	NiPoint3 parentnodeCenter;
	float fratio;
	NiPoint3 vert1;
	AStarNode<NavMeshSearchNode, NavMeshSearchLink>* pnode;
	NavMesh* potherMesh;
	int32_t iotherEdge;
	uint16_t usotherTri;
	const NavMeshSearchNode* parentNodeData = &apNode->GetNodeData();
	for (int32_t i = 0; i < 3; ++i) {
		if (pHighLevelSolution) {
			NavMeshTriangle* pparentTri = parentNodeData->pNavMesh->GetTriangle(parentNodeData->sTriangle);
			if (pparentTri->EdgeHasExtraInfo(i)) {
				uint16_t TriOrExtraInfo = pparentTri->GetTriOrExtraInfo(i);
				NavMeshInfo* potherNavmesh =
					parentNodeData->pNavMesh->GetExtraEdgeInfo(TriOrExtraInfo)->Portal.pOtherMesh;
				if (pHighLevelSolution->Find(potherNavmesh, 0,
						&BSSimpleArray<const NavMeshInfo*>::DefaultEqualFunctionByAddress) == -1) {
					continue;
				}
			}
		}
		if (!parentNodeData->pNavMesh->GetMatchingEdge(parentNodeData->sTriangle, i, &potherMesh, &usotherTri,
				&iotherEdge)) {
			continue;
		}
		EdgeExtraInfo* pextra = parentNodeData->pNavMesh->GetEdgeExtraInfo(parentNodeData->sTriangle, i);
		if (pextra && pextra->eType == EIT_LEDGE_UP) {
			continue;
		}
		if (parentNodeData->pNavMesh == potherMesh && parentNodeData->sTriangle == usotherTri) {
			continue;
		}
		if (pCurrentRequest->fCenterRadius > 0.0) {
			NiPoint3 nodeCenter = potherMesh->GetCenter(usotherTri);
			NiPoint3 vec = nodeCenter - *pCurrentRequest->GetTargetPoint(&kTargetPoint);
			double fsqrLength = vec.SqrLength();
			double fcenterRadius = pCurrentRequest->fCenterRadius;
			if (pCurrentRequest->fCenterRadius * fcenterRadius < fsqrLength) {
				continue;
			}
		}
		float fdoorMultiplier = 1.0f;
		if (potherMesh->GetTriangle(usotherTri)->GetFlag(NavMeshTriangle::NAV_TRI_CLOSED_DOOR)) {
			if (pCurrentRequest->bCantOpenDoors) {
				continue;
			}
			uint32_t doorID = potherMesh->GetClosedDoorInfoForTriangle(usotherTri)->DoorFormID;
			TESObjectREFR* pdoor = static_cast<TESObjectREFR*>(TESForm::GetFormByNumericID(doorID));
			if (pdoor) {
				if (!pCurrentRequest->GetIgnoreLocks()) {
					bool bmustPick;
					if (pdoor->GetInaccessible()) {
						continue;
					}
					TempDoorLock.SetFromDoorRef(pdoor);
					const ActorLockData* lockData = &pCurrentRequest->LockData;
					if (!TempDoorLock.CanActorUnlockDoorLock(lockData, pdoor, bmustPick)) {
						continue;
					}
				}
				if (static_cast<TESObjectDOOR*>(pdoor->GetObjectReference())->TESObjectDOOR::IsMinimalUse()) {
					fdoorMultiplier = 100.0f;
				}
			}
		}
		double factorRadius = pCurrentRequest->fActorRadius;
		if (fPathingLargeActorRadius.Float() < factorRadius &&
			potherMesh->GetTriangle(usotherTri)->GetFlag(NavMeshTriangle::NAV_TRI_LARGE_ACTOR)) {
			continue;
		}
		// vert2 is the plane's normal and the 512 edge length its constant.
		FNVD_INLINED_LOCAL(NiPlane, v46, std::nullopt, std::nullopt);
		float fwaterMultiplier = 1.0f;
		bool botherTriInWater = potherMesh->GetTriangle(usotherTri)->IsWater();
		bool bthisTriInWater = parentNodeData->GetTriangle()->IsWater();
		if (pCurrentRequest->GetCanSwim() || pCurrentRequest->GetCanFly()) {
			if (botherTriInWater == bthisTriInWater) {
				if (botherTriInWater) {
					fwaterMultiplier = 4.0f;
				}
			} else {
				fwaterMultiplier = 5.0f;
			}
		} else if (botherTriInWater) {
			continue;
		}
		float fbadNodeMultiplier = 1.0f;
		if (pBadNodes) {
			for (uint32_t j = 0; j < pBadNodes->iSize; ++j) {
				if ((*pBadNodes)[j].sTriangle == apNode->tNodeData.sTriangle &&
					(*pBadNodes)[j].pNavMesh == apNode->tNodeData.pNavMesh && (*pBadNodes)[j].iEdgeIndex == i) {
					fbadNodeMultiplier = 100.0f;
					break;
				}
			}
		}
		NiPoint3 nodeCenter_2 = potherMesh->GetCenter(usotherTri);
		apNode->GetNodeData().GetCenter(&parentnodeCenter);
		float fcost = (nodeCenter_2 - parentnodeCenter).XYLength();
		if (potherMesh->GetTriangle(usotherTri)->IsPreferred()) {
			fcost = fcost * 0.009999999776482582;
		}
		if (pCurrentRequest && pCurrentRequest->GetAvoidNodeArray()) {
			PathingAvoidNodeArray* AvoidNodeArray = pCurrentRequest->GetAvoidNodeArray();
			float favoidNodeCost = AvoidNodeArray->GetCostForPoint(nodeCenter_2);
			if (favoidNodeCost > 0.0) {
				fcost = (favoidNodeCost + 1.0) * fcost;
			}
		}
		if (potherMesh->GetTriangle(usotherTri)->GetFlag(NavMeshTriangle::NAV_TRI_AVOID_NODE)) {
			using staticNodes_t = BSSimpleArray<NavMeshStaticAvoidNode*, 1024>;
			staticNodes_t staticNodes;
			if (static_cast<uint8_t>(potherMesh->GetStaticAvoidNodesForTri(usotherTri, staticNodes))) {
				float ftotalStaticCost = 0.0f;
				for (uint32_t uiIndex = 0; uiIndex < staticNodes.iSize; ++uiIndex) {
					ftotalStaticCost = staticNodes.GetAt(static_cast<int32_t>(uiIndex))->fCost + ftotalStaticCost;
				}
				fcost = fcost * ftotalStaticCost;
			}
		}
		potherMesh->GetEdgeVertices(usotherTri, iotherEdge, &vert1, &v46.m_kNormal);
		float fedgeLength = (vert1 - v46.m_kNormal).XYLength();
		v46.m_fConstant = 512.0f;
		fratio = 1.0 - fedgeLength / v46.m_fConstant;
		BSClamp(fratio, 0.0f, 1.0f);
		fcost = (fratio * 0.5 + 0.5) * fcost;
		fcost = fcost * fwaterMultiplier;
		fcost = fcost * fbadNodeMultiplier;
		fcost = fcost * fdoorMultiplier;
		if (pCurrentRequest->GetTryToUseCurvedPath()) {
			// Builds the goal's center before the start's.
			const NiPoint3* pGoalCenter = GoalNode.GetCenter(&kGoalCenter);
			NiPoint3 deltaToGoal = *StartNode.GetCenter(&kStartCenter) - *pGoalCenter;
			NiPoint3 deltaToNode = nodeCenter_2 - *GoalNode.GetCenter(&kGoalCenter2);
			float fangleToGoal = GetZAngleFromVector(deltaToGoal);
			float fangleToNode = GetZAngleFromVector(deltaToNode);
			float fdeltaAngle = fangleToNode - fangleToGoal;
			BSWrap(fdeltaAngle, -3.1415927f, 3.1415927f);
			fdeltaAngle = std::fabs(fdeltaAngle);
			float fdiameter = deltaToGoal.Length();
			float fcurradius = (3.1415927410125732 - fdeltaAngle) * fdiameter / 3.1415927410125732;
			float flength = deltaToNode.Length();
			float fradiusError = flength - fcurradius;
			fcost = std::fabs(fradiusError) * fRadiusErrorCostMult * fcost;
		}
		fcost = apNode->fCost + fcost;
		int32_t iresult =
			AStarSearch_GetOrCreateAStarNode_6A7E00(this, GetKeyForNode(potherMesh, usotherTri), fcost, &pnode);
		if (iresult == 0) {
			SetNodeData(pnode, potherMesh, usotherTri);
		} else if (iresult != 1) {
			continue;
		}
		pnode->ParentLink.pNavMesh = apNode->tNodeData.pNavMesh;
		pnode->ParentLink.sTriangle = apNode->tNodeData.sTriangle;
		pnode->ParentLink.iEdgeIndex = i;
		apRetNodes->Add(pnode);
	}
	return 1;
}

bool NavMeshSearch::IsGoalNode(const AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode) {
	if (pCurrentRequest && pCurrentRequest->fActorRadius > 100.0) {
		NiPoint3 kNodeCenter;
		NiPoint3 kGoalCenter;
		const NiPoint3* pNodeCenter = apNode->GetNodeData().GetCenter(&kNodeCenter);
		NiPoint3 deltaToGoal = *GoalNode.GetCenter(&kGoalCenter) - *pNodeCenter;
		float fsqrxyLength = deltaToGoal.SqrXYLength();
		float fsqrzlength = deltaToGoal.z * deltaToGoal.z;
		double fquarterRadius = pCurrentRequest->fTargetRadius * 0.25;
		if (fsqrxyLength < pCurrentRequest->fTargetRadius * fquarterRadius && fsqrxyLength > fsqrzlength) {
			return true;
		}
	}
	return apNode->GetNodeData().pNavMesh == GoalNode.pNavMesh && apNode->GetNodeData().sTriangle == GoalNode.sTriangle;
}

bool NavMeshSearch::BuildNodePath(const PathingRequest* apRequest, const NavMeshSearchNode& aStartNode,
	const NavMeshSearchNode& aGoalNode, BSSimpleArray<NavMeshSearchNode, 1024>* apReturnNodeArray,
	BSSimpleArray<NavMeshSearchLink, 1024>* apReturnLinkArray,
	const BSSimpleArray<const NavMeshInfo*, 1024>* apHighLevelSolution) {
	NiPoint3 kGoalCenter2;
	NiPoint3 kStartCenter2;
	NiPoint3 kGoalCenter;
	NiPoint3 kStartCenter;
	using searchNavmeshes_t = BSSimpleArray<NavMeshPtr, 1024>;
	pCurrentRequest = apRequest;
	apReturnNodeArray->Clear(0);
	apReturnLinkArray->Clear(0);
	ClearSearchData();
	StartNode.pNavMesh = aStartNode.pNavMesh;
	StartNode.sTriangle = aStartNode.sTriangle;
	NiPoint3 center = StartNode.pNavMesh->GetCenter(StartNode.sTriangle);
	StartNode.CenterX = center.x;
	StartNode.CenterY = center.y;
	StartNode.CenterZ = center.z;
	GoalNode.pNavMesh = aGoalNode.pNavMesh;
	GoalNode.sTriangle = aGoalNode.sTriangle;
	center = GoalNode.pNavMesh->GetCenter(GoalNode.sTriangle);
	GoalNode.CenterX = center.x;
	GoalNode.CenterY = center.y;
	GoalNode.CenterZ = center.z;
	// Builds the goal's center before the start's, as the game does.
	const NiPoint3* pGoalCenter = GoalNode.GetCenter(&kGoalCenter);
	double fdistance = (*StartNode.GetCenter(&kStartCenter) - *pGoalCenter).Length();
	float fmaxFitness = fdistance + fdistance;
	SetMaxFitness(fmaxFitness);
	pGoalCenter = GoalNode.GetCenter(&kGoalCenter2);
	float afHeuristic = (*StartNode.GetCenter(&kStartCenter2) - *pGoalCenter).Length();
	AddStartNode(GetKeyForNode(StartNode.pNavMesh, StartNode.sTriangle), StartNode, afHeuristic);
	pHighLevelSolution = apHighLevelSolution;
	if (pHighLevelSolution) {
		searchNavmeshes_t searchNavmeshes(pHighLevelSolution->iSize);
		for (uint32_t i = 0; i < pHighLevelSolution->iSize; ++i) {
			NavMeshPtr spmesh;
			if ((*pHighLevelSolution)[i]->GetNavMesh(spmesh)) {
				searchNavmeshes.GetAt(static_cast<int32_t>(i)).operator=(spmesh);
			}
		}
	}
	return Search(apReturnNodeArray, apReturnLinkArray, 1);
}

uint32_t NavMeshSearch::GetKeyForNode(NavMesh* apNavMesh, uint16_t aiTri) const {
	return (uint32_t)(aiTri | (((uint16_t)apNavMesh->TESForm::GetFormID()) << 16));
}

void NavMeshSearch::SetNodeData(AStarNode<NavMeshSearchNode, NavMeshSearchLink>* apNode, NavMesh* apNavMesh,
	uint16_t aiTriangle) {
	NiPoint3 kGoalCenter;
	NiPoint3 kNodeCenter;
	NavMeshSearchNode* pdata = &apNode->tNodeData;
	pdata->pNavMesh = apNavMesh;
	pdata->sTriangle = aiTriangle;
	NiPoint3 center = apNavMesh->GetCenter(aiTriangle);
	pdata->CenterX = center.x;
	pdata->CenterY = center.y;
	pdata->CenterZ = center.z;
	const NiPoint3* pGoalCenter = GoalNode.GetCenter(&kGoalCenter);
	float fdistanceToGoal = (*pdata->GetCenter(&kNodeCenter) - *pGoalCenter).Length();
	float fheuristic = fdistanceToGoal * 0.009999999776482582;
	apNode->SetHeuristic(fheuristic);
}

void NavMeshSearch::operator delete(void* apMemory) {
	ourFree(apMemory);
}
