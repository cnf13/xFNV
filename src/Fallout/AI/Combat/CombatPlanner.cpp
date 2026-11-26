#include "Fallout/AI/Combat/CombatPlanner.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/ScrapHeap.h"
#include "Fallout Shared/Pathfinding/AStarNode.h"
#include "Fallout Shared/Pathfinding/AStarNodeHeap.h"
#include "Fallout Shared/Pathfinding/AStarSearch.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatLocation.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlannerWindow.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/CombatController.h"
#include "Gamebryo2.2/CoreLibs/NiCollision/NiPick.h"

#include <limits>

using CombatPlanSearch = AStarSearch<CombatPlanNode*, CombatPlanLink*>;
using CombatPlanNodeArray = BSSimpleArray<AStarNode<CombatPlanNode*, CombatPlanLink*>*, 1024>;
using CombatPlanNodeScrapArray = BSScrapArray<AStarNode<CombatPlanNode*, CombatPlanLink*>*, 1024>;

CombatPlanLinkData::CombatPlanLinkData() : iLocation(-1) {}

// The world state is a copy of apWorldStateArray's; the unsatisfied states are deliberately left unset.
CombatPlanNode::CombatPlanNode(CombatWorldStateArray* apWorldStateArray)
  : iUnsatisfiedStateCount(0), iKey(static_cast<uint32_t>(-1)) {
    CombatWorldStateArray_0(apWorldStateArray);
}

uint32_t CombatPlanNode::GetKey() {
    if (iKey != 0xFFFFFFFFu) {
        return iKey;
    }
    // The FNV-1 offset basis, then each world state's value, the unsatisfied count and each unsatisfied state.
    iKey = 0x811C9DC5u;
    for (uint32_t i = 0; i < COMBAT_WORLD_STATE_COUNT; ++i) {
        iKey = CombatPlanNode::HashKey(static_cast<uint32_t>(GetValue(static_cast<COMBAT_WORLD_STATE>(i))), iKey);
    }
    iKey = CombatPlanNode::HashKey(iUnsatisfiedStateCount, iKey);
    for (uint32_t i = 0; i < iUnsatisfiedStateCount; ++i) {
        CombatWorldState* punsatisfiedState = UnsatisfiedStates[i].pUnsatisfiedState;
        iKey = CombatPlanNode::HashKey(static_cast<uint32_t>(punsatisfiedState->eState), iKey);
        iKey = CombatPlanNode::HashKey(punsatisfiedState->iValue, iKey);
    }
    return iKey;
}

void CombatPlanNode::AddUnsatisfiedState(CombatWorldState* apWorldState, CombatPlanNode* apParent) {
    UnsatisfiedStates[static_cast<int32_t>(iUnsatisfiedStateCount)].pUnsatisfiedState = apWorldState;
    UnsatisfiedStates[static_cast<int32_t>(iUnsatisfiedStateCount++)].pUnsatisfiedStateParent = apParent;
}

uint32_t CombatPlanNode::HashKey(uint32_t auiValue, uint32_t auiKey) {
    return auiValue ^ (16777619u * auiKey);
}

CombatPlanLink::CombatPlanLink(CombatAction* apAction, int32_t aiDesiredValue, CombatPlanNode* apParentNode)
  : pAction(apAction), pParentNode(apParentNode), iDesiredValue(aiDesiredValue) {}

// pLinkData is deliberately left unset.
CombatPlannerData::CombatPlannerData()
  : pPlanner(nullptr), pCombatLocation{}, pCombatCoverLocation{}, fCombatLocationCost{} {}

// Frees the location candidates and deletes the cover candidates.
CombatPlannerData::~CombatPlannerData() {
    for (uint32_t i = 0; i < 22; ++i) {
        ourFree(pCombatLocation[i]);
        if (pCombatCoverLocation[i]) {
            delete pCombatCoverLocation[i];
        }
    }
}

CombatLocation* CombatPlannerData::GetLocation(int32_t aiType) {
    return pCombatLocation[aiType];
}

double CombatPlannerData::GetCost(int32_t aeCombatLocationType) {
    return fCombatLocationCost[aeCombatLocationType];
}

void CombatPlannerData::SetCombatLocation(int32_t aiLocation, CombatLocation* apCombatLocation, float afCost) {
    pCombatLocation[aiLocation] = apCombatLocation;
    fCombatLocationCost[aiLocation] = afCost;
    pLinkData->iLocation = aiLocation;
}

void CombatPlannerData::SetCombatCoverLocation(int32_t aiLocation, CombatCoverLocation* apCombatCoverLocation) {
    pCombatCoverLocation[aiLocation] = apCombatCoverLocation;
    pLinkData->iLocation = aiLocation;
}

CombatCoverLocation* CombatPlannerData::GetCombatCoverLocation(int32_t aiLocation) {
    return pCombatCoverLocation[aiLocation];
}

void CombatPlannerData::ClearLocation(int32_t aiType) {
    pCombatLocation[aiType] = nullptr;
    pCombatCoverLocation[aiType] = nullptr;
}

void CombatPlannerData::SetUnk4Ptr(int32_t aiLocation) {
    pLinkData->iLocation = aiLocation;
}

CombatPlanner::CombatPlanner() : pCombatController(nullptr) {
    PlannerData.pPlanner = this;
}

// Releases the cover map's write locks; the members and the search base are destroyed after.
CombatPlanner::~CombatPlanner() {
    CleanUp();
}

bool CombatPlanner::GetNodeConnections(
    const AStarNode<CombatPlanNode*, CombatPlanLink*>* apNode,
    BSSimpleArray<AStarNode<CombatPlanNode*, CombatPlanLink*>*, 1024>* apRetNodes
) {
    AStarNode<CombatPlanNode*, CombatPlanLink*>* pnode;
    float fparentCost = apNode->fCost;
    AStarNode<CombatPlanNode*, CombatPlanLink*>* pparentNode = apNode->pParentAStarNode;
    bool bfirstLink = pparentNode == nullptr;
    if (!pparentNode) {
        apRetNodes->SetReservedSize(16u);
    }
    for (uint32_t i = 0; i < apNode->tNodeData->iUnsatisfiedStateCount; ++i) {
        CombatWorldState* punsatisfiedState = apNode->tNodeData->UnsatisfiedStates[i].pUnsatisfiedState;
        BSSimpleList<CombatAction*>* pactionList = CombatAction::GetActionsForTargetState(punsatisfiedState->eState);
        bool bactionFound = false;
        for (; pactionList && pactionList->m_item; pactionList = pactionList->m_pkNext) {
            CombatAction* paction = pactionList->m_item;
            if (!paction->GetFlag(CombatAction::INFINITE_ACTION) || bfirstLink) {
                bool bactionMatches = paction->HasTargetState(punsatisfiedState);
                int32_t iunsatisfiedStateValue = punsatisfiedState->iValue;
                if (CombatWorldState::IsSpecifiedVariable(iunsatisfiedStateValue)) {
                    iunsatisfiedStateValue = apNode->tNodeData->GetSpecifiedVariable(iunsatisfiedStateValue);
                }
                CombatPlanLinkData linkData;
                pCombatController->pCombatPlannerData->pLinkData = &linkData;
                if (bactionMatches &&
                    paction->CanExecute(pCombatController, &InitialWorldState, iunsatisfiedStateValue)) {
                    float fcost = paction->GetCost(pCombatController, iunsatisfiedStateValue);
                    if (fcost < std::numeric_limits<float>::max()) {
                        void* pMemory = PlannerHeap.Allocate(sizeof(CombatPlanNode), ScrapHeap::ALIGNMENT);
                        CombatPlanNode* pnewNode = new (pMemory) CombatPlanNode(&InitialWorldState);
                        for (CombatWorldStateList* peffectsList = &paction->Effects;
                             peffectsList && peffectsList->m_item;
                             peffectsList = peffectsList->GetNext()) {
                            CombatWorldState* pstate = peffectsList->m_item;
                            int32_t ivalue = pnewNode->GetStateValue(pstate->iValue, iunsatisfiedStateValue);
                            pnewNode->ApplyWorldState(pstate->eState, ivalue);
                        }
                        for (CombatWorldStateList* ppossibleEffectsList = &paction->PossibleEffects;
                             ppossibleEffectsList && ppossibleEffectsList->m_item;
                             ppossibleEffectsList = ppossibleEffectsList->GetNext()) {
                            CombatWorldState* pstate = ppossibleEffectsList->m_item;
                            int32_t ivalue = pnewNode->GetStateValue(pstate->iValue, iunsatisfiedStateValue);
                            pnewNode->ApplyWorldState(pstate->eState, ivalue);
                        }
                        for (uint32_t j = 0; j < apNode->tNodeData->iUnsatisfiedStateCount; ++j) {
                            CombatWorldState* pstate = apNode->tNodeData->UnsatisfiedStates[j].pUnsatisfiedState;
                            int32_t ivalue = pnewNode->GetStateValue(pstate->iValue, iunsatisfiedStateValue);
                            if (!pnewNode->IsTrue(pstate->eState, ivalue)) {
                                pMemory = PlannerHeap.Allocate(sizeof(CombatWorldState), ScrapHeap::ALIGNMENT);
                                CombatWorldState* pnewState = new (pMemory) CombatWorldState(pstate->eState, ivalue);
                                pnewNode->AddUnsatisfiedState(
                                    pnewState,
                                    apNode->tNodeData->UnsatisfiedStates[j].pUnsatisfiedStateParent
                                );
                            }
                        }
                        for (CombatWorldStateList* ppreconditionsList = &paction->Preconditions;
                             ppreconditionsList && ppreconditionsList->m_item;
                             ppreconditionsList = ppreconditionsList->GetNext()) {
                            CombatWorldState* pstate = ppreconditionsList->m_item;
                            int32_t ivalue = pnewNode->GetStateValue(pstate->iValue, iunsatisfiedStateValue);
                            if (!pnewNode->IsTrue(pstate->eState, ivalue)) {
                                pMemory = PlannerHeap.Allocate(sizeof(CombatWorldState), ScrapHeap::ALIGNMENT);
                                CombatWorldState* pnewState = new (pMemory) CombatWorldState(pstate->eState, ivalue);
                                pnewNode->AddUnsatisfiedState(pnewState, pnewNode);
                            }
                        }
                        fcost = fcost + fparentCost;
                        bactionFound = true;
                        uint32_t ikey = GetKeyForNode(pnewNode);
                        int32_t iResult = GetOrCreateAStarNode(ikey, fcost, &pnode);
                        if (iResult) {
                            if (iResult != 1) {
                                continue;
                            }
                        }
                        else {
                            pnode->tNodeData = pnewNode;
                            pnode->fHeuristic = 0.0f;
                        }
                        pMemory = PlannerHeap.Allocate(sizeof(CombatPlanLink), ScrapHeap::ALIGNMENT);
                        CombatPlanLink* plink = new (pMemory) CombatPlanLink(
                            paction,
                            iunsatisfiedStateValue,
                            apNode->tNodeData->UnsatisfiedStates[i].pUnsatisfiedStateParent
                        );
                        plink->LinkData = linkData;
                        pnode->ParentLink = plink;
                        apRetNodes->Add(pnode);
                    }
                }
            }
        }
        if (!bactionFound) {
            apRetNodes->Clear(false);
            return false;
        }
    }
    return true;
}

bool CombatPlanner::IsGoalNode(const AStarNode<CombatPlanNode*, CombatPlanLink*>* apNode) {
    return apNode->tNodeData->iUnsatisfiedStateCount == 0;
}

// Releases every write lock this thread holds on the cover map. Re-reads pCombatManager and GetCoverMap before each
// call, and re-tests IsCurrentThreadWriting after every UnlockForWrite.
void CombatPlanner::CleanUp() {
    while (pCombatManager->GetCoverMap()->IsCurrentThreadWriting()) {
        pCombatManager->GetCoverMap()->UnlockForWrite();
    }
}

CombatWorldStateArray* CombatPlanner::GetWorldState() {
    return &InitialWorldState;
}

uint32_t CombatPlanner::GetKeyForNode(CombatPlanNode* const& aNode) const {
    return aNode->GetKey();
}

CombatPlan* CombatPlanner::CreatePlan_0(CombatController* apCombatController, bool abDebugPlan) {
    pCombatController = apCombatController;
    pCombatController->pCombatPlannerData = &PlannerData;
    InitialWorldState.Update(apCombatController);
    for (uint32_t i = 0; i < 16; ++i) {
        CombatGoal* pgoal = CombatGoal::GetGoal(i);
        if (pgoal->IsActive(&InitialWorldState)) {
            CombatPlanNode startNode(&InitialWorldState);
            for (CombatWorldStateList* pgoalStateList = &pgoal->GoalStates; pgoalStateList && pgoalStateList->m_item;
                 pgoalStateList = pgoalStateList->GetNext()) {
                CombatWorldState* pgoalState = pgoalStateList->m_item;
                int32_t ivalue = pgoalState->iValue;
                if (CombatWorldState::IsSpecifiedVariable(ivalue)) {
                    ivalue = startNode.GetSpecifiedVariable(ivalue);
                }
                if (!startNode.IsTrue(pgoalState->eState, ivalue)) {
                    void* pMemory = PlannerHeap.Allocate(sizeof(CombatWorldState), ScrapHeap::ALIGNMENT);
                    CombatWorldState* pnewState = new (pMemory) CombatWorldState(pgoalState->eState, ivalue);
                    startNode.AddUnsatisfiedState(pnewState, &startNode);
                }
            }
            CombatPlan* pplan = CreatePlan(apCombatController, pgoal, &startNode, abDebugPlan);
            if (pplan) {
                return pplan;
            }
            pgoal->CombatPlannerFailed(apCombatController, &InitialWorldState);
        }
    }
    return nullptr;
}

CombatPlan* CombatPlanner::CreatePlan(
    CombatController* apCombatController,
    CombatGoal* apGoal,
    CombatPlanNode* apStartNode,
    bool abDebugPlan
) {
    using NodeArray_t = BSScrapArray<CombatPlanNode*, 1024>;
    using LinkArray_t = BSScrapArray<CombatPlanLink*, 1024>;
    if (!apStartNode->iUnsatisfiedStateCount) {
        return nullptr;
    }
    ClearSearchData();
    uint32_t ikey = GetKeyForNode(apStartNode);
    AddStartNode(ikey, apStartNode, 0.0f);
    NodeArray_t NodeArray;
    LinkArray_t LinkArray;
    Search(&NodeArray, &LinkArray, false);
    CombatPlan* pplan = nullptr;
    if (LinkArray.iSize) {
        pplan = new CombatPlan(apCombatController, apGoal, &NodeArray, &LinkArray, &InitialWorldState);
    }
    if (abDebugPlan) {
        new CombatPlannerWindow(this, pplan, &NodeArray);
    }
    ClearSearchData();
    return pplan;
}

AStarNode<CombatPlanNode*, CombatPlanLink*>* CombatPlanner::GetStartNode() {
    return pStartNode;
}

void CombatController::SetCombatPlannerData(CombatPlannerData* apPlannerData) {
    pCombatPlannerData = apPlannerData;
}
