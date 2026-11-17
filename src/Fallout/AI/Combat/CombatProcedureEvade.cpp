#include "Fallout/AI/Combat/CombatProcedureEvade.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Pathfinding/PathingRequestFlee.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatDialogueManager.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

CombatProcedureEvade::CombatProcedureEvade(int32_t aiLocation)
	: eState(CombatProcedureEvade::EVADE_STATE_INITIALIZING), iLocation(aiLocation) {}

CombatProcedureEvade::~CombatProcedureEvade() {}

void CombatProcedureEvade::Update() {
	BGSWorldLocation apLocation;
	NiPoint3 destination;
	NiPoint3 attackerLocation;
	Actor* pattacker = GetAttacker();
	CombatState* pcombatState = pCombatController->pCombatState;
	CombatProcedureEvade::EVADE_STATE eState = this->eState;
	if (eState) {
		if (eState == CombatProcedureEvade::EVADE_STATE_EVADING) {
			if (pattacker->DidPathingFail()) {
				if (!pattacker->IsPathValid() && this->iLocation == 12) {
					pcombatState->BoostConfidence(0);
				}
				pcombatState->ClearCombatLocation();
				if (pattacker->IsPathValid()) {
					SetFailed("Pathing failed while evading");
				} else {
					SetFailed("Path Building failed to evade");
				}
			} else {
				if (pattacker->IsPathingComplete()) {
					int32_t a2a = this->iLocation;
					BGSWorldLocation* WorldLocation = pattacker->GetWorldLocation(&apLocation);
					pcombatState->ReachedDestination(*WorldLocation, a2a);
					SetFinished();
				}
			}
		}
	} else {
		attackerLocation = pattacker->GetLocationOnReference();
		PathingRequestFlee fleeRequest;
		fleeRequest.Initialize(pattacker);
		float a2 = pattacker->GetHeading(0);
		fleeRequest.SetInitialPathHeading(a2);
		if (pattacker->IsCreature()) {
			TESActorBaseData* v6 = static_cast<TESActorBaseData*>(pattacker->GetActorBase());
			fleeRequest.SetCantOpenDoors(v6->GetCantOpenDoors());
		}
		PathingAvoidNodeArray* pavoidNodeArray = 0;
		int32_t iLocation = this->iLocation;
		if (iLocation == 10 || iLocation == 12) {
			fleeRequest.SetMinFleeDistance(fCombatFleeNormalDistance.Float());
			fleeRequest.SetNoDestination();
			pavoidNodeArray = pCombatController->pCombatGroup->CreateFleeAvoidNodeArray(pattacker, 0);
		} else {
			if (iLocation == 14) {
				pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_AVOID_THREAT, 1);
				fleeRequest.SetMinFleeDistance(128.0f);
				if (pattacker->GetCurrentPathfindingGoal_008B3840(&destination)) {
					PathingLocation apMemory(destination, pattacker);
					PathingLocation* aDestination = &apMemory;
					fleeRequest.SetDestination(*aDestination);
				} else {
					fleeRequest.SetNoDestination();
				}
				pavoidNodeArray = pCombatManager->pThreatMap->CreateAvoidNodeArray(attackerLocation, pattacker, -1.0f);
				pavoidNodeArray = pCombatController->pCombatGroup->CreateMovementAvoidNodeArray(pattacker,
					attackerLocation, 384.0f, pavoidNodeArray, 0);
			}
		}
		fleeRequest.SetAvoidNodeArray(pavoidNodeArray);
		fleeRequest.SetAllowIncompletePath(1);
		if (pattacker->SetPathfindingHide(&fleeRequest)) {
			this->eState = CombatProcedureEvade::EVADE_STATE_EVADING;
			SetMoveMode(3);
		} else {
			SetFailed("Cannot setup evade path");
		}
	}
}

void CombatProcedureEvade::Shutdown() {
	Actor* attacker = CombatProcedure::GetAttacker();
	attacker->StopMoving();
	CombatProcedure::ClearMoveMode();
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureEvade::GetType() {
	return EVADE;
}

void CombatProcedureEvade::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	CombatProcedure::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iLocation), 4, 0);
}

void CombatProcedureEvade::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	CombatProcedure::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iLocation), 4);
}

void CombatProcedureEvade::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	CombatProcedure::InitLoadGame(apLoadGameBuffer);
}
