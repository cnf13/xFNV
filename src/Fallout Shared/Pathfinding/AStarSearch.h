#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarNodeHeap.h"
#include "Fallout Shared/Pathfinding/AStarNodeMap.h"
#include "Fallout Shared/Pathfinding/AStarQueue.h"

#include <stdint.h>

struct NavMeshSearchLink;

// The A* search the pathing code derives its searches from, over nodes of type NodeData joined by links of type Link.
// It owns the node heap, the node hash table, the open list and the best, last and start nodes; the derived search
// supplies the graph (GetNodeConnections) and the goal test (IsGoalNode).
template <class NodeData, class Link>
class AStarSearch {
public:
    // Clears the node pointers; the compiler builds the node heap, the node map and the open list.
    AStarSearch()
      : pBestNodeSoFar(nullptr), pLastSearchedNode(nullptr), pStartNode(nullptr), bIsRunningInTasklet(false) {}

    virtual ~AStarSearch();

protected:
    // Fills apRetNodes with the nodes reachable from apNode.
    virtual bool GetNodeConnections(
        const AStarNode<NodeData, Link>* apNode,
        BSSimpleArray<AStarNode<NodeData, Link>*, 1024>* apRetNodes
    ) = 0;
    // Whether apNode is the search's goal.
    virtual bool IsGoalNode(const AStarNode<NodeData, Link>* apNode) = 0;

public:
    // Adds the start node aNodeData under aiKey with the heuristic afHeuristic and returns it; null when the node
    // already existed and kept its cost. aiKey is pointer-sized, as AStarNode::iKey.
    AStarNode<NodeData, Link>* AddStartNode(uintptr_t aiKey, const NodeData& aNodeData, float afHeuristic);
    // The node for aiKey into apRetNode: 0 when it was created with cost afCost, 1 when it existed and afCost is lower
    // (taken off the open list, cost lowered), 2 when it existed and keeps its cost. aiKey is pointer-sized, as
    // AStarNode::iKey.
    int32_t GetOrCreateAStarNode(uintptr_t aiKey, float afCost, AStarNode<NodeData, Link>** aRetNode);
    // Runs the search, filling apReturnArray and apReturnLinkArray with the path found (or the
    // best one when abReturnBestPathIfFailed).
    bool Search(
        BSSimpleArray<NodeData, 1024>* apReturnArray,
        BSSimpleArray<Link, 1024>* apReturnLinkArray,
        bool abReturnBestPathIfFailed
    );
    // Clears the node map and the open list and nulls the best, last and start nodes.
    void ClearSearchData();
    // Sets the open list's largest fitness to afMaxFitness.
    void SetMaxFitness(float afMaxFitness);
    // Expands the open list until the goal is reached or it runs out; the goal node, or null.
    AStarNode<NodeData, Link>* BuildNodePath();
    // Walks the parents from apNode back to the start, filling apReturnArray with the nodes' data and
    // apReturnLinkArray with the links taken.
    bool GetPathToNode(
        const AStarNode<NodeData, Link>* apFinalNode,
        BSSimpleArray<NodeData, 1024>* apReturnArray,
        BSSimpleArray<Link, 1024>* apReturnLinkArray
    );
    // The best node found so far.
    AStarNode<NodeData, Link>* GetBestAStarNode();

    // The search is freed with ourFree when deleted.
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    AStarNodeHeap<NodeData, Link, 512> kNodeHeap;
    AStarMap<AStarNode<NodeData, Link>> kNodeMap;
    AStarQueue<NodeData, Link> kSearchQueue;
    AStarNode<NodeData, Link>* pBestNodeSoFar;
    AStarNode<NodeData, Link>* pLastSearchedNode;
    AStarNode<NodeData, Link>* pStartNode;
    bool bIsRunningInTasklet;
};

// The members' own destructors destroy the open list and the node heap.
template <class NodeData, class Link>
AStarSearch<NodeData, Link>::~AStarSearch() {}
