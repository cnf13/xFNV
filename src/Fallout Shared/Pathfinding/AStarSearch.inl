// Kept out of AStarSearch.h: BuildNodePath's scrap array comes from BSScrapArray.h, which includes NavMeshSearch.h,
// whose search derives from AStarSearch.
#pragma once

// The generic members, instantiated by AStarSearch.cpp for every search.
#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSThread.h"
#include "BSSystem/BSTaskletManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"

template <class NodeData, class Link>
AStarNode<NodeData, Link>* AStarSearch<NodeData, Link>::AddStartNode(uintptr_t aiKey, const NodeData& aNodeData,
	float afHeuristic) {
	AStarNode<NodeData, Link>* pNode;
	switch (GetOrCreateAStarNode(aiKey, 0.0f, &pNode)) {
	case 0:
		pNode->fHeuristic = afHeuristic;
		pNode->tNodeData = aNodeData;
		kSearchQueue.AddNode(pNode);
		pStartNode = pNode;
		return pNode;
	case 1:
		return pNode;
	default:
		return nullptr;
	}
}

// The bucket's chain is searched for aiKey; a new node goes at the head of the chain. The map's count is deliberately
// not touched, as the game does.
template <class NodeData, class Link>
int32_t AStarSearch<NodeData, Link>::GetOrCreateAStarNode(uintptr_t aiKey, float afCost,
	AStarNode<NodeData, Link>** aRetNode) {
	AStarNode<NodeData, Link>** ppBucketHead = &kNodeMap.Buckets[kNodeMap.GetBucketFromKey(aiKey)];
	for (AStarNode<NodeData, Link>* pCurrent = *ppBucketHead; pCurrent; pCurrent = pCurrent->pNextNodeInBucket) {
		if (pCurrent->iKey == aiKey) {
			if (!(afCost < pCurrent->fCost)) {
				return 2;
			}
			*aRetNode = pCurrent;
			kSearchQueue.RemoveNode(pCurrent);
			pCurrent->fCost = afCost;
			return 1;
		}
	}
	AStarNode<NodeData, Link>* pNode = kNodeHeap.AllocateNode();
	*aRetNode = pNode;
	pNode->iKey = aiKey;
	pNode->fCost = afCost;
	pNode->pNextNodeInBucket = *ppBucketHead;
	*ppBucketHead = pNode;
	return 0;
}

// Whether the goal was reached; the path is that of the goal node, else of the best node so far when
// abReturnBestPathIfFailed.
template <class NodeData, class Link>
bool AStarSearch<NodeData, Link>::Search(BSSimpleArray<NodeData, 1024>* apReturnArray,
	BSSimpleArray<Link, 1024>* apReturnLinkArray, bool abReturnBestPathIfFailed) {
	BSTaskletManager* pTaskletManager = &BSTaskletManager::QInstance();
	uint32_t uiThreadID = BSGetCurrentThreadID();
	bIsRunningInTasklet = pTaskletManager->IsTaskletThread(uiThreadID);
	AStarNode<NodeData, Link>* pSolutionNode = BuildNodePath();
	AStarNode<NodeData, Link>* pBestNode;
	if (pSolutionNode) {
		pBestNode = pSolutionNode;
	} else if (abReturnBestPathIfFailed) {
		pBestNode = pBestNodeSoFar;
	} else {
		pBestNode = nullptr;
	}
	if (pBestNode) {
		GetPathToNode(pBestNode, apReturnArray, apReturnLinkArray);
	}
	return pSolutionNode != nullptr;
}

template <class NodeData, class Link> void AStarSearch<NodeData, Link>::ClearSearchData() {
	kNodeMap.Clear();
	kSearchQueue.Clear();
	pBestNodeSoFar = nullptr;
	pLastSearchedNode = nullptr;
	pStartNode = nullptr;
}

template <class NodeData, class Link> void AStarSearch<NodeData, Link>::SetMaxFitness(float afMaxFitness) {
	kSearchQueue.SetMaxFitness(afMaxFitness);
}

// Takes the fittest node off the open list until it is the goal: each connected node is moved under the searched node
// and back onto the open list, and the one with the lowest heuristic is kept as the best so far. A connected node that
// is the searched node's own parent ends the search without a node, as the game does. In a tasklet the thread yields
// every 50 nodes. When the open list runs out, the last searched node is the last one taken.
template <class NodeData, class Link> AStarNode<NodeData, Link>* AStarSearch<NodeData, Link>::BuildNodePath() {
	BSScrapArray<AStarNode<NodeData, Link>*, 1024> kConnectedNodes;
	AStarNode<NodeData, Link>* pTempLastSearchedNode = pLastSearchedNode;
	pLastSearchedNode = kSearchQueue.PopFront();
	if (!pLastSearchedNode) {
		return nullptr;
	}
	pBestNodeSoFar = pLastSearchedNode;
	int32_t iLoopCount = 0;
	while (pLastSearchedNode) {
		if (IsGoalNode(pLastSearchedNode)) {
			pBestNodeSoFar = pLastSearchedNode;
			return pLastSearchedNode;
		}
		kConnectedNodes.Clear(false);
		GetNodeConnections(pLastSearchedNode, &kConnectedNodes);
		for (uint32_t i = 0; i < kConnectedNodes.iSize; ++i) {
			AStarNode<NodeData, Link>* pConnectedNode = kConnectedNodes.GetAt(static_cast<int32_t>(i));
			kSearchQueue.RemoveNode(pConnectedNode);
			if (pLastSearchedNode->pParentAStarNode == pConnectedNode) {
				return nullptr;
			}
			pConnectedNode->pParentAStarNode = pLastSearchedNode;
			kSearchQueue.AddNode(pConnectedNode);
			if (pBestNodeSoFar->fHeuristic > pConnectedNode->fHeuristic) {
				pBestNodeSoFar = pConnectedNode;
			}
		}
		if (bIsRunningInTasklet && ++iLoopCount == 50) {
			iLoopCount = 0;
			BSSleep(0);
		}
		pTempLastSearchedNode = pLastSearchedNode;
		pLastSearchedNode = kSearchQueue.PopFront();
	}
	pLastSearchedNode = pTempLastSearchedNode;
	return nullptr;
}

// The arrays are sized to the path (one link fewer than nodes) and filled from the end, apNode's data last.
template <class NodeData, class Link>
bool AStarSearch<NodeData, Link>::GetPathToNode(const AStarNode<NodeData, Link>* apFinalNode,
	BSSimpleArray<NodeData, 1024>* apReturnArray, BSSimpleArray<Link, 1024>* apReturnLinkArray) {
	if (!apFinalNode) {
		return false;
	}
	uint32_t uiNodeCount = 0;
	for (const AStarNode<NodeData, Link>* pCurrent = apFinalNode; pCurrent; pCurrent = pCurrent->pParentAStarNode) {
		++uiNodeCount;
	}
	apReturnArray->SetSize(uiNodeCount, true);
	apReturnLinkArray->SetSize(uiNodeCount - 1, true);
	int32_t iIndex = static_cast<int32_t>(uiNodeCount) - 1;
	for (const AStarNode<NodeData, Link>* pCurrent = apFinalNode; pCurrent; pCurrent = pCurrent->pParentAStarNode) {
		apReturnArray->GetAt(iIndex) = pCurrent->tNodeData;
		if (iIndex > 0) {
			apReturnLinkArray->GetAt(iIndex - 1) = pCurrent->ParentLink;
		}
		--iIndex;
	}
	return true;
}

template <class NodeData, class Link> AStarNode<NodeData, Link>* AStarSearch<NodeData, Link>::GetBestAStarNode() {
	return pBestNodeSoFar;
}
