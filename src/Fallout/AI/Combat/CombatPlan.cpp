#include "Fallout/AI/Combat/CombatPlan.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

CombatPlan::CombatPlan(CombatController* apCombatController)
  : pPlanSteps(nullptr), iLength(0), iCurrentAction(0), pGoal(nullptr), iCombatLocation(-1),
    pCombatController(apCombatController) {}

// The plan from apGoal back along the planner's links: one step per link, each linked to the step of its parent node.
CombatPlan::CombatPlan(
    CombatController* apCombatController,
    CombatGoal* apGoal,
    BSSimpleArray<CombatPlanNode*, 1024>* apPlanNodes,
    BSSimpleArray<CombatPlanLink*, 1024>* apPlanLinks,
    CombatWorldStateArray* apWorldStateArray
)
  : iCurrentAction(0), pGoal(apGoal), iCombatLocation(-1), pCombatController(apCombatController) {
    AllocateSteps(apPlanLinks->iSize);
    for (uint32_t i = 0; i < iLength; ++i) {
        CombatPlanLink* pLink = (*apPlanLinks)[iLength - i - 1];
        CombatPlanNode* pNode = (*apPlanNodes)[iLength - i];
        pPlanSteps[i]->pAction = pLink->pAction;
        pPlanSteps[i]->iActionValue = pLink->iDesiredValue;
        pPlanSteps[i]->uiNodeKey = pNode->GetKey();
        pPlanSteps[i]->pParentStep = nullptr;
        if (pLink->LinkData.iLocation != -1) {
            iCombatLocation = pLink->LinkData.iLocation;
        }
    }
    for (uint32_t i = 0; i < iLength; ++i) {
        uint32_t uiParentKey = (*apPlanLinks)[iLength - i - 1]->pParentNode->GetKey();
        for (uint32_t j = 0; j < iLength; ++j) {
            if (pPlanSteps[j]->uiNodeKey == uiParentKey) {
                pPlanSteps[i]->pParentStep = pPlanSteps[j];
                break;
            }
        }
    }
    ValidatePlan(apWorldStateArray);
}

// Frees each step, then the step array.
CombatPlan::~CombatPlan() {
    for (uint32_t i = 0; i < iLength; ++i) {
        ourFree(pPlanSteps[i]);
    }
    ourFree(pPlanSteps);
}

void CombatPlan::AllocateSteps(uint32_t aiLength) {
    iLength = aiLength;
    pPlanSteps = static_cast<CombatPlan::Step**>(
        ourMalloc(iLength <= SIZE_MAX / sizeof(CombatPlan::Step*) ? sizeof(CombatPlan::Step*) * iLength : SIZE_MAX)
    );
    for (uint32_t i = 0; i < iLength; ++i) {
        pPlanSteps[i] = static_cast<CombatPlan::Step*>(ourMalloc(sizeof(CombatPlan::Step)));
    }
}

CombatAction* CombatPlan::GetAction(uint32_t aiIndex) {
    return pPlanSteps[aiIndex]->pAction;
}

bool CombatPlan::IsFinished() {
    return iCurrentAction >= iLength;
}

uint32_t CombatPlan::GetIndexForStep(Step* apStep) {
    for (uint32_t i = 0; i < iLength; ++i) {
        if (pPlanSteps[i] == apStep) {
            return i;
        }
    }
    // "COMBAT: Could not find plan step %08X in CombatPlan::GetIndexForStep()"
    Warning("COMBAT: Could not find plan step %08X in CombatPlan::GetIndexForStep()", apStep);
    return 0;
}

int32_t CombatPlan::GetActionValue(uint32_t aiIndex) {
    return pPlanSteps[static_cast<int32_t>(aiIndex)]->iActionValue;
}

char CombatPlan::HasAction(int32_t aeAction) {
    for (uint32_t i = 0; i < iLength; ++i) {
        if (pPlanSteps[i]->pAction->eType == aeAction) {
            return 1;
        }
    }
    return 0;
}

bool CombatPlan::ValidatePlanStep(
    uint32_t aiStep,
    CombatAction* apAction,
    int32_t aiActionValue,
    CombatWorldStateArray* apWorldStateArray
) {
    for (uint32_t j = aiStep + 1; j < iLength; ++j) {
        CombatAction* pEffectedAction = pPlanSteps[j]->pAction;
        int32_t iEffectedActionValue = pPlanSteps[j]->iActionValue;
        bool bAllEffectsContained = true;
        if (pEffectedAction != apAction) {
            for (CombatWorldStateList* pEffects = &apAction->Effects; pEffects && pEffects->m_item;
                 pEffects = pEffects->GetNext()) {
                bool bEffectContained = false;
                CombatWorldState* pEffectState = pEffects->m_item;
                for (CombatWorldStateList* pEffects2 = &pEffectedAction->Effects; pEffects2 && pEffects2->m_item;
                     pEffects2 = pEffects2->GetNext()) {
                    CombatWorldState* pEffectState2 = pEffects2->m_item;
                    if (pEffectState->eState == pEffectState2->eState) {
                        bEffectContained = true;
                        break;
                    }
                }
                if (!bEffectContained) {
                    bAllEffectsContained = false;
                    break;
                }
            }
        }
        if (bAllEffectsContained) {
            if (!pPlanSteps[aiStep]->pParentStep) {
                return true;
            }
            uint32_t uiParentIndex = GetIndexForStep(pPlanSteps[aiStep]->pParentStep);
            if (uiParentIndex < j) {
                return true;
            }
            CombatPlan::Step* pTemp = pPlanSteps[aiStep];
            ourMemcpy(
                &pPlanSteps[aiStep],
                &pPlanSteps[aiStep + 1],
                sizeof(CombatPlan::Step*) * (uiParentIndex - 1 - aiStep)
            );
            pPlanSteps[uiParentIndex - 1] = pTemp;
            return false;
        }
        for (CombatWorldStateList* pEffects = &apAction->Effects; pEffects && pEffects->m_item;
             pEffects = pEffects->GetNext()) {
            CombatWorldState* pEffectState = pEffects->m_item;
            int32_t iEffectValue = apWorldStateArray->GetStateValue(pEffectState->iValue, aiActionValue);
            for (CombatWorldStateList* pPreconditions = &pEffectedAction->Preconditions;
                 pPreconditions && pPreconditions->m_item;
                 pPreconditions = pPreconditions->GetNext()) {
                CombatWorldState* pPreconditionState = pPreconditions->m_item;
                if (pEffectState->eState == pPreconditionState->eState) {
                    int32_t iPreconditionValue = apWorldStateArray->GetStateValue(
                        pPreconditionState->iValue,
                        iEffectedActionValue
                    );
                    if (iPreconditionValue != -1 && iEffectValue != iPreconditionValue) {
                        bool bSameBranch = false;
                        if (pEffectedAction->GetFlag(CombatAction::INFINITE_ACTION)) {
                            bSameBranch = true;
                        }
                        else {
                            for (CombatPlan::Step* pParent = pPlanSteps[aiStep]->pParentStep; pParent;
                                 pParent = pParent->pParentStep) {
                                if (pParent == pPlanSteps[j]) {
                                    bSameBranch = true;
                                    break;
                                }
                            }
                        }
                        if (!bSameBranch) {
                            CombatPlan::Step* pTemp = pPlanSteps[aiStep];
                            uint32_t uiShiftIndex = j;
                            if (pTemp->pParentStep) {
                                uiShiftIndex = GetIndexForStep(pPlanSteps[aiStep]->pParentStep) - 1;
                            }
                            if (uiShiftIndex <= aiStep) {
                                bSameBranch = true;
                            }
                            else {
                                ourMemcpy(
                                    &pPlanSteps[aiStep],
                                    &pPlanSteps[aiStep + 1],
                                    sizeof(CombatPlan::Step*) * (uiShiftIndex - aiStep)
                                );
                                pPlanSteps[uiShiftIndex] = pTemp;
                            }
                        }
                        if (bSameBranch) {
                            uint32_t uiCurrentLength = iLength;
                            CombatPlan::Step** pCurrentSteps = pPlanSteps;
                            iLength = uiCurrentLength + 1;
                            pPlanSteps = static_cast<CombatPlan::Step**>(ourMalloc(
                                iLength <= SIZE_MAX / sizeof(CombatPlan::Step*) ? sizeof(CombatPlan::Step*) * iLength
                                                                                : SIZE_MAX
                            ));
                            ourMemcpy(pPlanSteps, pCurrentSteps, sizeof(CombatPlan::Step*) * j);
                            ourMemcpy(
                                &pPlanSteps[j + 1],
                                &pCurrentSteps[j],
                                sizeof(CombatPlan::Step*) * (uiCurrentLength - j)
                            );
                            pPlanSteps[j] = static_cast<CombatPlan::Step*>(ourMalloc(sizeof(CombatPlan::Step)));
                            pPlanSteps[j]->pAction = apAction;
                            pPlanSteps[j]->iActionValue = iPreconditionValue;
                            pPlanSteps[j]->uiNodeKey = 0;
                            pPlanSteps[j]->pParentStep = pPlanSteps[j + 1];
                            ourFree(pCurrentSteps);
                        }
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

void CombatPlan::ValidatePlan(CombatWorldStateArray* apWorldStateArray) {
    for (uint32_t uiIndex = 0; uiIndex < iLength; ++uiIndex) {
        if (!ValidatePlanStep(
                uiIndex,
                pPlanSteps[uiIndex]->pAction,
                pPlanSteps[uiIndex]->iActionValue,
                apWorldStateArray
            )) {
            --uiIndex;
        }
    }
}

void CombatPlan::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iLength)), sizeof(iLength));
    AllocateSteps(iLength);
    for (uint32_t i = 0; i < iLength; ++i) {
        CombatPlan::Step* pStep = pPlanSteps[i];
        int8_t cActionType = 0;
        apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&cActionType)), sizeof(cActionType));
        apLoadGameBuffer->LoadData(
            static_cast<char*>(static_cast<void*>(&pStep->iActionValue)),
            sizeof(pStep->iActionValue)
        );
        pStep->pAction = CombatPlan::GetCombatAction(static_cast<uint8_t>(cActionType));
        pStep->pParentStep = nullptr;
        pStep->uiNodeKey = 0;
    }
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iCurrentAction)), sizeof(iCurrentAction));
    int8_t cGoalType = 0;
    apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&cGoalType)), sizeof(cGoalType));
    pGoal = CombatGoal::GetGoal(static_cast<uint8_t>(cGoalType));
}

bool CombatPlan::ExecuteNextAction(CombatController* apCombatController) {
    while (!IsFinished() && pPlanSteps[iCurrentAction]->pAction->GetFlag(CombatAction::BACKGROUND_ACTION)) {
        pPlanSteps[iCurrentAction]->pAction->Execute(apCombatController, pPlanSteps[iCurrentAction]->iActionValue);
        ++iCurrentAction;
    }
    if (!IsFinished()) {
        bool bIsEmpty = true;
        if (pPlanSteps[iCurrentAction]->pAction == apCombatController->pCurrentAction) {
            if (pPlanSteps[iCurrentAction]->iActionValue == apCombatController->iCurrentActionValue &&
                pPlanSteps[iCurrentAction]
                    ->pAction->CheckIdentical(apCombatController, pPlanSteps[iCurrentAction]->iActionValue) &&
                !apCombatController->GetProcedureFailed()) {
                bIsEmpty = apCombatController->GetProcedureFinished();
            }
        }
        if (bIsEmpty) {
            apCombatController->ClearProcedures();
            pPlanSteps[iCurrentAction]->pAction->Execute(apCombatController, pPlanSteps[iCurrentAction]->iActionValue);
            apCombatController->pCurrentAction = pPlanSteps[iCurrentAction]->pAction;
            apCombatController->iCurrentActionValue = pPlanSteps[iCurrentAction]->iActionValue;
        }
        ++iCurrentAction;
    }
    return IsFinished();
}

void CombatPlan::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(&iLength, sizeof(iLength), 0);
    for (uint32_t i = 0; i < iLength; ++i) {
        CombatPlan::Step* pStep = pPlanSteps[i];
        uint8_t cActionType = pStep->pAction->eType;
        apSaveGameBuffer->SaveDataEndian(&cActionType, sizeof(cActionType), 0);
        apSaveGameBuffer->SaveDataEndian(&pStep->iActionValue, sizeof(pStep->iActionValue), 0);
    }
    apSaveGameBuffer->SaveDataEndian(&iCurrentAction, sizeof(iCurrentAction), 0);
    uint8_t cGoalType = pGoal->eType;
    apSaveGameBuffer->SaveDataEndian(&cGoalType, sizeof(cGoalType), 0);
}

CombatAction* CombatPlan::GetCombatAction(int32_t aiAction) {
    return CombatAction::pCombatActions[aiAction];
}

bool CombatAction::GetFlag(COMBAT_ACTION_FLAG aeFlag) {
    return (iFlags & aeFlag) != 0;
}
