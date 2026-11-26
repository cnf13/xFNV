#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout/AI/Combat/CombatWorldState.h"

class CombatAction;
class CombatController;
class CombatCoverLocation;
class CombatGoal;
class CombatLocation;
class CombatPlan;
class CombatPlanner;
class CombatPlanNode;

// The location a plan link acts at.
struct CombatPlanLinkData {
    CombatPlanLinkData();

    int32_t iLocation;
};

// A world state entry the planner has not satisfied yet, with the node that left it unsatisfied.
struct CombatPlanUnsatisfiedState {
    CombatWorldState* pUnsatisfiedState;
    CombatPlanNode* pUnsatisfiedStateParent;
};

// A node of the combat planner's A* search: a world state and the states that still have to be satisfied.
class CombatPlanNode : public CombatWorldStateArray {
public:
    CombatPlanNode() = default;
    CombatPlanNode(CombatWorldStateArray* apWorldStateArray);

    // The node's key.
    uint32_t GetKey();
    void AddUnsatisfiedState(CombatWorldState* apWorldState, CombatPlanNode* apParent);

    // One FNV-1 step: auiKey times the FNV prime, xor auiValue. Its only caller, GetKey, treats both values and the
    // result as unsigned.
    static uint32_t HashKey(uint32_t auiValue, uint32_t auiKey);

    CombatPlanUnsatisfiedState UnsatisfiedStates[16];
    uint32_t iUnsatisfiedStateCount;
    uint32_t iKey;
};

// A link of the combat planner's A* search: the action taken from a parent node and the value it was chosen for.
class CombatPlanLink {
public:
    struct Key {
        int32_t uiVal;
    };

    CombatPlanLink(CombatAction* apAction, int32_t aiDesiredValue, CombatPlanNode* apParentNode);

    CombatAction* pAction;
    CombatPlanNode* pParentNode;
    int32_t iDesiredValue;
    CombatPlanLinkData LinkData;
};

// What the planner hands a plan search: the planner, the link data and per-location candidate arrays with their costs.
class CombatPlannerData {
public:
    CombatPlannerData();
    ~CombatPlannerData();

    // The location candidate of type aiType (pCombatLocation[aiType]).
    CombatLocation* GetLocation(int32_t aiType);
    double GetCost(int32_t aeCombatLocationType);
    void SetCombatLocation(int32_t aiLocation, CombatLocation* apCombatLocation, float afCost);
    void SetCombatCoverLocation(int32_t aiLocation, CombatCoverLocation* apCombatCoverLocation);
    // The cover candidate of type aiLocation (pCombatCoverLocation[aiLocation]).
    CombatCoverLocation* GetCombatCoverLocation(int32_t aiLocation);
    // Clears both candidates of type aiType.
    void ClearLocation(int32_t aiType);
    void SetUnk4Ptr(int32_t aiLocation);

    CombatPlanner* pPlanner;
    CombatPlanLinkData* pLinkData;
    CombatLocation* pCombatLocation[22];
    CombatCoverLocation* pCombatCoverLocation[22];
    float fCombatLocationCost[22];
};

// The planner that searches (A*) for the sequence of combat actions that takes a combat controller's world state to
// a goal state. It owns the heap the search's nodes live in.
class CombatPlanner : public AStarSearch<CombatPlanNode*, CombatPlanLink*> {
public:
    CombatPlanner();
    ~CombatPlanner() override;

    bool GetNodeConnections(
        const AStarNode<CombatPlanNode*, CombatPlanLink*>* apNode,
        BSSimpleArray<AStarNode<CombatPlanNode*, CombatPlanLink*>*, 1024>* apRetNodes
    ) override;
    bool IsGoalNode(const AStarNode<CombatPlanNode*, CombatPlanLink*>* apNode) override;

    // Releases the cover map's write locks the current thread holds.
    void CleanUp();
    CombatWorldStateArray* GetWorldState();
    uint32_t GetKeyForNode(CombatPlanNode* const& aNode) const;
    // The CreatePlan(apCombatController, abDebugPlan) overload.
    CombatPlan* CreatePlan_0(CombatController* apCombatController, bool abDebugPlan);
    CombatPlan*
    CreatePlan(CombatController* apCombatController, CombatGoal* apGoal, CombatPlanNode* apStartNode, bool abDebugPlan);
    AStarNode<CombatPlanNode*, CombatPlanLink*>* GetStartNode();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    CombatController* pCombatController;
    CombatWorldStateArray InitialWorldState;
    ScrapHeap PlannerHeap;
    CombatPlannerData PlannerData;
};

// The planner's search node, the instantiation AStarQueue<CombatPlanNode*, CombatPlanLink*> links through.
using AStarNode_CombatPlanNode_CombatPlanLink = AStarNode<CombatPlanNode*, CombatPlanLink*>;
