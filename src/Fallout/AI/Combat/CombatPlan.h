#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Reconstruction.hpp"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class CombatAction;
class CombatController;
class CombatGoal;
class CombatPlanLink;
class CombatPlanNode;
class CombatWorldStateArray;

class CombatPlan {
public:
	using GoalArray = CombatGoal* [16];

	// One action of the plan, the step it follows and the search node it came from (IDA's names; the PDB's
	// CombatPlanStep, at namespace scope there).
	struct Step {
		CombatAction* pAction;
		Step* pParentStep;
		uint32_t uiNodeKey;
		int32_t iActionValue;
	};

	CombatPlan(CombatController* apCombatController);
	// Inferred type: apWorldStateArray: CombatWorldStateArray* from 0x994fe6 (&CombatPlanner+0x205c (this of
	// CombatPlanner::CreatePlan))
	CombatPlan(CombatController* apCombatController, CombatGoal* apGoal,
		BSSimpleArray<CombatPlanNode*, 1024>* apPlanNodes, BSSimpleArray<CombatPlanLink*, 1024>* apPlanLinks,
		CombatWorldStateArray* apWorldStateArray);
	~CombatPlan();

	// Allocates the step stack for aiLength steps.
	void AllocateSteps(uint32_t aiLength);
	CombatAction* GetAction(uint32_t aiIndex);
	// Whether the plan has run all of its steps.
	bool IsFinished();
	// The index of apStep in the plan; a warning and 0 when the plan lacks it.
	uint32_t GetIndexForStep(Step* apStep);
	int32_t GetActionValue(uint32_t aiIndex);
	char HasAction(int32_t aeAction);
	// Inferred type: auiIndex: uint32_t: unsigned uses from 0x994663; apState: CombatWorldStateArray* from 0x99432a
	// (parameter 0 of CombatPlan::ValidatePlan)
	bool ValidatePlanStep(uint32_t aiStep, CombatAction* apAction, int32_t aiActionValue,
		CombatWorldStateArray* apWorldStateArray);
	void ValidatePlan(CombatWorldStateArray* apWorldStateArray);
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	bool ExecuteNextAction(CombatController* apCombatController);
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);

	// The CombatAction singleton of the action index aiAction.
	static CombatAction* GetCombatAction(int32_t aiAction);
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	Step** pPlanSteps;
	uint32_t iLength;
	uint32_t iCurrentAction;
	CombatGoal* pGoal;
	int32_t iCombatLocation;
	CombatController* pCombatController;

	static inline GoalArray pGoals{};
};
