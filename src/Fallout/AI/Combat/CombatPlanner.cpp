#include "Fallout/AI/Combat/CombatPlanner.h"

#include "BSCore/BSScrapArray.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
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

using CombatPlanSearch = AStarSearch<CombatPlanNode*, CombatPlanLink*>;
using CombatPlanNodeArray = BSSimpleArray<AStarNode<CombatPlanNode*, CombatPlanLink*>*, 1024>;
using CombatPlanNodeScrapArray = BSScrapArray<AStarNode<CombatPlanNode*, CombatPlanLink*>*, 1024>;

CombatPlanLinkData::CombatPlanLinkData() : iLocation(-1) {}

// The world state is a copy of apWorldStateArray's; the unsatisfied states are deliberately left unset, as the game
// does.
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
    for (uint32_t i_2 = 0; i_2 < iUnsatisfiedStateCount; ++i_2) {
        CombatWorldState* punsatisfiedState = UnsatisfiedStates[i_2].pUnsatisfiedState;
        iKey = CombatPlanNode::HashKey(static_cast<uint32_t>(punsatisfiedState->eState), iKey);
        iKey = CombatPlanNode::HashKey(punsatisfiedState->iValue, iKey);
    }
    return iKey;
}

void CombatPlanNode::AddUnsatisfiedState(CombatWorldState* apWorldState, CombatPlanNode* apParent) {
    UnsatisfiedStates[(int32_t)iUnsatisfiedStateCount].pUnsatisfiedState = apWorldState;
    UnsatisfiedStates[(int32_t)(iUnsatisfiedStateCount++)].pUnsatisfiedStateParent = apParent;
}

uint32_t CombatPlanNode::HashKey(uint32_t auiValue, uint32_t auiKey) {
    return auiValue ^ (16777619u * auiKey);
}

CombatPlanLink::CombatPlanLink(CombatAction* apAction, int32_t aiDesiredValue, CombatPlanNode* apParentNode)
  : pAction(apAction), pParentNode(apParentNode), iDesiredValue(aiDesiredValue) {}

// pLinkData is deliberately left unset, as the game does.
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

CombatCoverLocation* CombatPlannerData::GetCombatCoverLocation(int32_t a2) {
    return pCombatCoverLocation[a2];
}

void CombatPlannerData::ClearLocation(int32_t aiType) {
    pCombatLocation[aiType] = nullptr;
    pCombatCoverLocation[aiType] = nullptr;
}

void CombatPlannerData::SetUnk4Ptr(int32_t a2) {
    pLinkData->iLocation = a2;
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
    CombatPlanLinkData linkData;
    float fparentCost = apNode->fCost;
    AStarNode<CombatPlanNode*, CombatPlanLink*>* v3 = apNode->pParentAStarNode;
    bool bfirstLink = v3 == 0;
    if (!v3) {
        apRetNodes->SetReservedSize(16u);
    }
    for (uint32_t i = 0; i < apNode->tNodeData->iUnsatisfiedStateCount; ++i) {
        CombatWorldState* punsatisfiedState = apNode->tNodeData->UnsatisfiedStates[i].pUnsatisfiedState;
        BSSimpleList<CombatAction*>* pactionList = CombatAction::GetActionsForTargetState(punsatisfiedState->eState);
        bool bactionFound = 0;
        while (pactionList && pactionList->m_item) {
            CombatAction* paction = pactionList->m_item;
            if (!paction->GetFlag(CombatAction::INFINITE_ACTION) || bfirstLink) {
                bool bactionMatches = paction->HasTargetState(punsatisfiedState);
                int32_t iunsatisfiedStateValue = punsatisfiedState->iValue;
                if (CombatWorldState::IsSpecifiedVariable(iunsatisfiedStateValue)) {
                    iunsatisfiedStateValue = apNode->tNodeData->GetSpecifiedVariable(iunsatisfiedStateValue);
                }
                linkData.~CombatPlanLinkData();
                ::new (&linkData) CombatPlanLinkData;
                pCombatController->pCombatPlannerData->pLinkData = &linkData;
                if (bactionMatches &&
                    paction->CanExecute(pCombatController, &InitialWorldState, iunsatisfiedStateValue)) {
                    float fcost = paction->GetCost(pCombatController, iunsatisfiedStateValue);
                    if (fcost < 3.4028235e+38f) {
                        CombatPlanLink* plink;
                        CombatPlanNode* pnewNode;
                        void* apWhere = PlannerHeap.Allocate(sizeof(CombatPlanNode), ScrapHeap::ALIGNMENT);
                        CombatPlanNode* apMemory =
                            static_cast<CombatPlanNode*>(::operator new(sizeof(CombatPlanNode), apWhere));
                        if (apMemory) {
                            pnewNode = ::new (apMemory) CombatPlanNode(&InitialWorldState);
                        }
                        else {
                            pnewNode = 0;
                        }
                        CombatPlanNode* pnewNode_2 = pnewNode;
                        for (CombatWorldStateList* peffectsList = &paction->Effects;
                             peffectsList && peffectsList->m_item; peffectsList = peffectsList->GetNext()) {
                            CombatWorldState* pstate = peffectsList->m_item;
                            int32_t iunsatisfiedStateValue_2 = iunsatisfiedStateValue;
                            uint32_t FormHealth_0 = pstate->iValue;
                            int32_t ivalue = pnewNode_2->GetStateValue(FormHealth_0, iunsatisfiedStateValue_2);
                            COMBAT_WORLD_STATE afCost = pstate->eState;
                            pnewNode_2->ApplyWorldState(afCost, ivalue);
                        }
                        for (CombatWorldStateList* ppossibleEffectsList = &paction->PossibleEffects;
                             ppossibleEffectsList && ppossibleEffectsList->m_item;
                             ppossibleEffectsList = ppossibleEffectsList->GetNext()) {
                            CombatWorldState* pstate_2 = ppossibleEffectsList->m_item;
                            int32_t iunsatisfiedStateValue_3 = iunsatisfiedStateValue;
                            uint32_t v5 = pstate_2->iValue;
                            int32_t ivalue_2 = pnewNode_2->GetStateValue(v5, iunsatisfiedStateValue_3);
                            COMBAT_WORLD_STATE afCosta = pstate_2->eState;
                            pnewNode_2->ApplyWorldState(afCosta, ivalue_2);
                        }
                        for (uint32_t j = 0; j < apNode->tNodeData->iUnsatisfiedStateCount; ++j) {
                            CombatWorldState* pstate_3 = apNode->tNodeData->UnsatisfiedStates[j].pUnsatisfiedState;
                            int32_t iunsatisfiedStateValue_4 = iunsatisfiedStateValue;
                            uint32_t v6 = pstate_3->iValue;
                            int32_t ivalue_3 = pnewNode_2->GetStateValue(v6, iunsatisfiedStateValue_4);
                            COMBAT_WORLD_STATE afCostb = pstate_3->eState;
                            if (!pnewNode_2->IsTrue(afCostb, ivalue_3)) {
                                CombatWorldState* v22;
                                void* v29 = PlannerHeap.Allocate(sizeof(CombatWorldState), ScrapHeap::ALIGNMENT);
                                CombatWorldState* v30 =
                                    static_cast<CombatWorldState*>(::operator new(sizeof(CombatWorldState), v29));
                                if (v30) {
                                    v22 = ::new (v30) CombatWorldState(pstate_3->eState, ivalue_3);
                                }
                                else {
                                    v22 = 0;
                                }
                                pnewNode_2->AddUnsatisfiedState(
                                    v22, apNode->tNodeData->UnsatisfiedStates[j].pUnsatisfiedStateParent
                                );
                            }
                        }
                        for (CombatWorldStateList* ppreconditionsList = &paction->Preconditions;
                             ppreconditionsList && ppreconditionsList->m_item;
                             ppreconditionsList = ppreconditionsList->GetNext()) {
                            CombatWorldState* pstate_4 = ppreconditionsList->m_item;
                            int32_t iunsatisfiedStateValue_5 = iunsatisfiedStateValue;
                            uint32_t v7 = pstate_4->iValue;
                            int32_t ivalue_4 = pnewNode_2->GetStateValue(v7, iunsatisfiedStateValue_5);
                            COMBAT_WORLD_STATE afCostc = pstate_4->eState;
                            if (!pnewNode_2->IsTrue(afCostc, ivalue_4)) {
                                CombatWorldState* v21;
                                void* v27 = PlannerHeap.Allocate(sizeof(CombatWorldState), ScrapHeap::ALIGNMENT);
                                CombatWorldState* v28 =
                                    static_cast<CombatWorldState*>(::operator new(sizeof(CombatWorldState), v27));
                                if (v28) {
                                    v21 = ::new (v28) CombatWorldState(pstate_4->eState, ivalue_4);
                                }
                                else {
                                    v21 = 0;
                                }
                                pnewNode_2->AddUnsatisfiedState(v21, pnewNode_2);
                            }
                        }
                        fcost = fcost + fparentCost;
                        bactionFound = 1;
                        uint32_t ikey = GetKeyForNode(pnewNode_2);
                        int32_t uiResult = GetOrCreateAStarNode(ikey, fcost, &pnode);
                        if (uiResult) {
                            if (uiResult != 1) {
                                goto LABEL_51;
                            }
                        }
                        else {
                            pnode->tNodeData = pnewNode_2;
                            pnode->fHeuristic = 0.0f;
                        }
                        void* v25 = PlannerHeap.Allocate(sizeof(CombatPlanLink), ScrapHeap::ALIGNMENT);
                        CombatPlanLink* v26 = static_cast<CombatPlanLink*>(::operator new(sizeof(CombatPlanLink), v25));
                        if (v26) {
                            plink = ::new (v26) CombatPlanLink(
                                paction, iunsatisfiedStateValue,
                                apNode->tNodeData->UnsatisfiedStates[i].pUnsatisfiedStateParent
                            );
                        }
                        else {
                            plink = 0;
                        }
                        plink->LinkData = linkData;
                        pnode->ParentLink = plink;
                        apRetNodes->Add(pnode);
                    }
                }
            }
        LABEL_51:;
            pactionList = pactionList->m_pkNext;
        }
        if (!bactionFound) {
            apRetNodes->Clear(0);
            return 0;
        }
    }
    return 1;
}

bool CombatPlanner::IsGoalNode(const AStarNode<CombatPlanNode*, CombatPlanLink*>* apNode) {
    return (((int32_t)apNode->tNodeData->iUnsatisfiedStateCount) == 0);
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
    return (uint32_t)((int32_t)aNode->GetKey());
}

CombatPlan* CombatPlanner::CreatePlan_0(CombatController* apCombatController, bool abDebugPlan) {
    CombatPlanNode startNode;
    pCombatController = apCombatController;
    pCombatController->pCombatPlannerData = &PlannerData;
    InitialWorldState.Update(apCombatController);
    for (uint32_t i = 0; i < 16; ++i) {
        CombatGoal* pgoal = CombatGoal::GetGoal(i);
        if (pgoal->IsActive(&InitialWorldState)) {
            startNode.~CombatPlanNode();
            ::new (&startNode) CombatPlanNode(&InitialWorldState);
            for (CombatWorldStateList* pgoalStateList = &pgoal->GoalStates; pgoalStateList && pgoalStateList->m_item;
                 pgoalStateList = pgoalStateList->GetNext()) {
                CombatWorldState* pgoalState = pgoalStateList->m_item;
                int32_t ivalue = pgoalState->iValue;
                if (CombatWorldState::IsSpecifiedVariable(ivalue)) {
                    ivalue = startNode.GetSpecifiedVariable(ivalue);
                }
                if (!startNode.IsTrue(pgoalState->eState, ivalue)) {
                    CombatWorldState* pGroupMember;
                    void* apWhere = PlannerHeap.Allocate(sizeof(CombatWorldState), ScrapHeap::ALIGNMENT);
                    CombatWorldState* v7 =
                        static_cast<CombatWorldState*>(::operator new(sizeof(CombatWorldState), apWhere));
                    if (v7) {
                        pGroupMember = ::new (v7) CombatWorldState(pgoalState->eState, ivalue);
                    }
                    else {
                        pGroupMember = 0;
                    }
                    startNode.AddUnsatisfiedState(pGroupMember, &startNode);
                }
            }
            CombatPlan* pplan = CreatePlan(apCombatController, pgoal, &startNode, abDebugPlan);
            if (pplan) {
                return pplan;
            }
            pgoal->CombatPlannerFailed(apCombatController, &InitialWorldState);
        }
    }
    return 0;
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
        return 0;
    }
    ClearSearchData();
    uint32_t KeyForNode = GetKeyForNode(apStartNode);
    AddStartNode(KeyForNode, apStartNode, 0.0f);
    NodeArray_t NodeArray;
    LinkArray_t LinkArray;
    Search(&NodeArray, &LinkArray, 0);
    CombatPlan* pplan = 0;
    if (LinkArray.iSize) {
        CombatPlan* pplan_2;
        CombatPlan* pPlan = static_cast<CombatPlan*>(ourMalloc(sizeof(CombatPlan)));
        if (pPlan) {
            pplan_2 = ::new (pPlan) CombatPlan(apCombatController, apGoal, &NodeArray, &LinkArray, &InitialWorldState);
        }
        else {
            pplan_2 = 0;
        }
        pplan = pplan_2;
    }
    CombatPlannerWindow* pWindow = 0;
    if (abDebugPlan) {
        CombatPlannerWindow* v7;
        CombatPlannerWindow* apMemory = static_cast<CombatPlannerWindow*>(ourMalloc(sizeof(CombatPlannerWindow)));
        if (apMemory) {
            v7 = ::new (apMemory) CombatPlannerWindow(this, pplan, &NodeArray);
        }
        else {
            v7 = 0;
        }
        pWindow = v7;
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
