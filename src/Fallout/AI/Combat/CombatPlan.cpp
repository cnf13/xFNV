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
CombatPlan::CombatPlan(CombatController* apCombatController, CombatGoal* apGoal,
	BSSimpleArray<CombatPlanNode*, 1024>* apPlanNodes, BSSimpleArray<CombatPlanLink*, 1024>* apPlanLinks,
	CombatWorldStateArray* apWorldStateArray)
	: iCurrentAction(0), pGoal(apGoal), iCombatLocation(-1), pCombatController(apCombatController) {
	AllocateSteps(apPlanLinks->iSize);
	for (uint32_t i = 0; i < iLength; ++i) {
		CombatPlanLink* plink = (*apPlanLinks)[iLength - i - 1];
		CombatPlanNode* pnode = (*apPlanNodes)[iLength - i];
		pPlanSteps[i]->pAction = plink->pAction;
		pPlanSteps[i]->iActionValue = plink->iDesiredValue;
		pPlanSteps[i]->uiNodeKey = pnode->GetKey();
		pPlanSteps[i]->pParentStep = nullptr;
		if (plink->LinkData.iLocation != -1) {
			iCombatLocation = plink->LinkData.iLocation;
		}
	}
	for (uint32_t i = 0; i < iLength; ++i) {
		uint32_t iparentKey = (*apPlanLinks)[iLength - i - 1]->pParentNode->GetKey();
		for (uint32_t j = 0; j < iLength; ++j) {
			if (pPlanSteps[j]->uiNodeKey == iparentKey) {
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
	pPlanSteps = static_cast<CombatPlan::Step**>(ourMalloc(iLength <= SIZE_MAX / sizeof(CombatPlan::Step*)
			? sizeof(CombatPlan::Step*) * iLength
			: SIZE_MAX));
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
	return (int32_t)((uint32_t)pPlanSteps[(int32_t)aiIndex]->iActionValue);
}

char CombatPlan::HasAction(int32_t aeAction) {
	for (uint32_t i = 0; i < iLength; ++i) {
		if (pPlanSteps[i]->pAction->eType == aeAction) {
			return 1;
		}
	}
	return 0;
}

bool CombatPlan::ValidatePlanStep(uint32_t aiStep, CombatAction* apAction, int32_t aiActionValue,
	CombatWorldStateArray* apWorldStateArray) {
	int32_t ipreconditionvalue;
	CombatAction* peffectedAction;
	CombatWorldStateList* peffects_2;
	int32_t ieffectedActionValue;
	uint32_t j;
	for (j = aiStep + 1;; ++j) {
		bool ballEffectsContained;
		if (j >= iLength) {
			return 1;
		}
		peffectedAction = pPlanSteps[j]->pAction;
		ieffectedActionValue = pPlanSteps[j]->iActionValue;
		ballEffectsContained = 1;
		if (peffectedAction != apAction) {
			for (CombatWorldStateList* peffects = &apAction->Effects; peffects && peffects->m_item;
				peffects = peffects->GetNext()) {
				bool beffectContained = 0;
				CombatWorldState* peffectState = peffects->m_item;
				for (CombatWorldStateList* peffects2 = &peffectedAction->Effects; peffects2 && peffects2->m_item;
					peffects2 = peffects2->GetNext()) {
					CombatWorldState* peffectState2 = peffects2->m_item;
					if (peffectState->eState == peffectState2->eState) {
						beffectContained = 1;
						break;
					}
				}
				if (!beffectContained) {
					ballEffectsContained = 0;
					break;
				}
			}
		}
		if (ballEffectsContained) {
			if (!pPlanSteps[aiStep]->pParentStep) {
				return 1;
			}
			uint32_t iparentIndex = GetIndexForStep(pPlanSteps[aiStep]->pParentStep);
			if (iparentIndex < j) {
				return 1;
			}
			CombatPlan::Step* ptemp = pPlanSteps[aiStep];
			ourMemcpy(&pPlanSteps[aiStep], &pPlanSteps[aiStep + 1],
				sizeof(CombatPlan::Step*) * (iparentIndex - 1 - aiStep));
			pPlanSteps[iparentIndex - 1] = ptemp;
			return 0;
		}
		peffects_2 = &apAction->Effects;
	LABEL_23:;
		if (peffects_2 && peffects_2->m_item) {
			break;
		}
	}
	CombatWorldState* peffectState_2 = peffects_2->m_item;
	uint32_t FormHealth_0 = peffectState_2->iValue;
	int32_t ieffectValue = apWorldStateArray->GetStateValue(FormHealth_0, aiActionValue);
	for (CombatWorldStateList* ppreconditions = &peffectedAction->Preconditions;;
		ppreconditions = ppreconditions->GetNext()) {
		if (!ppreconditions || !ppreconditions->m_item) {
			peffects_2 = peffects_2->GetNext();
			goto LABEL_23;
		}
		CombatWorldState* ppreconditionState = ppreconditions->m_item;
		COMBAT_WORLD_STATE v8 = peffectState_2->eState;
		if (v8 == ppreconditionState->eState) {
			uint32_t v9 = ppreconditionState->iValue;
			ipreconditionvalue = apWorldStateArray->GetStateValue(v9, ieffectedActionValue);
			if (ipreconditionvalue != -1 && ieffectValue != ipreconditionvalue) {
				break;
			}
		}
	}
	bool bsameBranch = 0;
	if (peffectedAction->GetFlag(CombatAction::INFINITE_ACTION)) {
		bsameBranch = 1;
	} else {
		for (CombatPlan::Step* pparent = pPlanSteps[aiStep]->pParentStep; pparent; pparent = pparent->pParentStep) {
			if (pparent == pPlanSteps[j]) {
				bsameBranch = 1;
				break;
			}
		}
	}
	if (!bsameBranch) {
		CombatPlan::Step* ptemp_2 = pPlanSteps[aiStep];
		uint32_t ishiftIndex = j;
		if (ptemp_2->pParentStep) {
			ishiftIndex = GetIndexForStep(pPlanSteps[aiStep]->pParentStep) - 1;
		}
		if (ishiftIndex <= aiStep) {
			bsameBranch = 1;
		} else {
			ourMemcpy(&pPlanSteps[aiStep], &pPlanSteps[aiStep + 1], sizeof(CombatPlan::Step*) * (ishiftIndex - aiStep));
			pPlanSteps[ishiftIndex] = ptemp_2;
		}
	}
	if (bsameBranch) {
		uint32_t icurrentLength = iLength;
		CombatPlan::Step** pcurrentSteps = pPlanSteps;
		iLength = icurrentLength + 1;
		pPlanSteps = static_cast<CombatPlan::Step**>(ourMalloc(iLength <= SIZE_MAX / sizeof(CombatPlan::Step*)
				? sizeof(CombatPlan::Step*) * iLength
				: SIZE_MAX));
		ourMemcpy(pPlanSteps, pcurrentSteps, sizeof(CombatPlan::Step*) * j);
		ourMemcpy(&pPlanSteps[j + 1], &pcurrentSteps[j], sizeof(CombatPlan::Step*) * (icurrentLength - j));
		pPlanSteps[j] = static_cast<CombatPlan::Step*>(ourMalloc(sizeof(CombatPlan::Step)));
		pPlanSteps[j]->pAction = apAction;
		pPlanSteps[j]->iActionValue = ipreconditionvalue;
		pPlanSteps[j]->uiNodeKey = 0;
		pPlanSteps[j]->pParentStep = pPlanSteps[j + 1];
		ourFree(pcurrentSteps);
	}
	return 0;
}

void CombatPlan::ValidatePlan(CombatWorldStateArray* apWorldStateArray) {
	uint32_t auiIndex;
	for (auiIndex = 0; auiIndex < iLength; ++auiIndex) {
		if (!ValidatePlanStep(auiIndex, pPlanSteps[auiIndex]->pAction, pPlanSteps[auiIndex]->iActionValue,
				apWorldStateArray)) {
			--auiIndex;
		}
	}
}

void CombatPlan::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	CombatPlan::Step* v3;
	int8_t apData;
	uint32_t i;
	apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iLength)), 4u);
	AllocateSteps(iLength);
	for (i = 0; i < iLength; ++i) {
		v3 = pPlanSteps[i];
		apData = 0;
		apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&apData)), 1u);
		apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&v3->iActionValue)), 4u);
		v3->pAction = CombatPlan::GetCombatAction((uint8_t)apData);
		v3->pParentStep = nullptr;
		v3->uiNodeKey = (uint32_t)0;
	}
	apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&iCurrentAction)), 4u);
	int8_t v6 = 0;
	apLoadGameBuffer->LoadData(static_cast<char*>(static_cast<void*>(&v6)), 1u);
	pGoal = CombatGoal::GetGoal((uint8_t)v6);
}

bool CombatPlan::ExecuteNextAction(CombatController* apCombatController) {
	while (!IsFinished() && pPlanSteps[iCurrentAction]->pAction->GetFlag(CombatAction::BACKGROUND_ACTION)) {
		pPlanSteps[iCurrentAction]->pAction->Execute(apCombatController, pPlanSteps[iCurrentAction]->iActionValue);
		++iCurrentAction;
	}
	if (!IsFinished()) {
		bool bIsEmpty = 1;
		CombatPlan::Step* v2 = pPlanSteps[iCurrentAction];
		if (v2->pAction == apCombatController->pCurrentAction) {
			CombatPlan::Step* v3 = pPlanSteps[iCurrentAction];
			if (v3->iActionValue == apCombatController->iCurrentActionValue &&
				pPlanSteps[iCurrentAction]->pAction->CheckIdentical(apCombatController,
					pPlanSteps[iCurrentAction]->iActionValue) &&
				!apCombatController->GetProcedureFailed()) {
				bIsEmpty = apCombatController->GetProcedureFinished();
			}
		}
		if (bIsEmpty) {
			apCombatController->ClearProcedures();
			pPlanSteps[iCurrentAction]->pAction->Execute(apCombatController, pPlanSteps[iCurrentAction]->iActionValue);
			// The exe stores both through its out-of-line one-field setters (0x9DB930, 0x5468D0).
			apCombatController->pCurrentAction = pPlanSteps[iCurrentAction]->pAction;
			apCombatController->iCurrentActionValue = pPlanSteps[iCurrentAction]->iActionValue;
		}
		++iCurrentAction;
	}
	return IsFinished();
}

void CombatPlan::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	uint8_t cactionType;
	uint8_t cgoalType;
	apSaveGameBuffer->SaveDataEndian(&iLength, 4u, 0);
	for (uint32_t i = 0; i < iLength; ++i) {
		CombatPlan::Step* pstep = pPlanSteps[i];
		cactionType = pstep->pAction->eType;
		apSaveGameBuffer->SaveDataEndian(&cactionType, 1u, 0);
		apSaveGameBuffer->SaveDataEndian(&pstep->iActionValue, 4u, 0);
	}
	apSaveGameBuffer->SaveDataEndian(&iCurrentAction, 4u, 0);
	cgoalType = pGoal->eType;
	apSaveGameBuffer->SaveDataEndian(&cgoalType, 1u, 0);
}

CombatAction* CombatPlan::GetCombatAction(int32_t aiAction) {
	return CombatAction::pCombatActions[aiAction];
}

// The PDB places CombatAction::GetFlag in this file.
bool CombatAction::GetFlag(COMBAT_ACTION_FLAG aeFlag) {
	return (iFlags & aeFlag) != 0;
}
