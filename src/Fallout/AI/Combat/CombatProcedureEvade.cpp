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
    BGSWorldLocation kLocation;
    NiPoint3 destination;
    NiPoint3 attackerLocation;
    Actor* pattacker = GetAttacker();
    CombatState* pcombatState = pCombatController->pCombatState;
    CombatProcedureEvade::EVADE_STATE eState = this->eState;
    if (eState) {
        if (eState == CombatProcedureEvade::EVADE_STATE_EVADING) {
            if (pattacker->DidPathingFail()) {
                if (!pattacker->IsPathValid() && this->iLocation == 12) {
                    pcombatState->BoostConfidence(false);
                }
                pcombatState->ClearCombatLocation();
                if (pattacker->IsPathValid()) {
                    SetFailed("Pathing failed while evading");
                }
                else {
                    SetFailed("Path Building failed to evade");
                }
            }
            else if (pattacker->IsPathingComplete()) {
                int32_t iLocation = this->iLocation;
                pcombatState->ReachedDestination(*pattacker->GetWorldLocation(&kLocation), iLocation);
                SetFinished();
            }
        }
    }
    else {
        attackerLocation = pattacker->GetLocationOnReference();
        PathingRequestFlee fleeRequest;
        fleeRequest.Initialize(pattacker);
        float fHeading = pattacker->GetHeading(false);
        fleeRequest.SetInitialPathHeading(fHeading);
        if (pattacker->IsCreature()) {
            TESActorBaseData* pBaseData = static_cast<TESActorBaseData*>(pattacker->GetActorBase());
            fleeRequest.SetCantOpenDoors(pBaseData->GetCantOpenDoors());
        }
        PathingAvoidNodeArray* pavoidNodeArray = nullptr;
        int32_t iLocation = this->iLocation;
        if (iLocation == 10 || iLocation == 12) {
            fleeRequest.SetMinFleeDistance(fCombatFleeNormalDistance.Float());
            fleeRequest.SetNoDestination();
            pavoidNodeArray = pCombatController->pCombatGroup->CreateFleeAvoidNodeArray(pattacker, nullptr);
        }
        else if (iLocation == 14) {
            pCombatDialogueManager->StartDialogue(pCombatController, DT_COMBAT, DC_AVOID_THREAT, true);
            fleeRequest.SetMinFleeDistance(128.0f);
            if (pattacker->GetCurrentPathfindingGoal(&destination)) {
                PathingLocation kDestination(destination, pattacker);
                fleeRequest.SetDestination(kDestination);
            }
            else {
                fleeRequest.SetNoDestination();
            }
            pavoidNodeArray = pCombatManager->pThreatMap->CreateAvoidNodeArray(attackerLocation, pattacker, -1.0f);
            pavoidNodeArray = pCombatController->pCombatGroup->CreateMovementAvoidNodeArray(
                pattacker,
                attackerLocation,
                384.0f,
                pavoidNodeArray,
                nullptr
            );
        }
        fleeRequest.SetAvoidNodeArray(pavoidNodeArray);
        fleeRequest.SetAllowIncompletePath(true);
        if (pattacker->SetPathfindingHide(&fleeRequest)) {
            this->eState = CombatProcedureEvade::EVADE_STATE_EVADING;
            SetMoveMode(3);
        }
        else {
            SetFailed("Cannot setup evade path");
        }
    }
}

void CombatProcedureEvade::Shutdown() {
    Actor* pAttacker = CombatProcedure::GetAttacker();
    pAttacker->StopMoving();
    CombatProcedure::ClearMoveMode();
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureEvade::GetType() {
    return EVADE;
}

void CombatProcedureEvade::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    CombatProcedure::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eState), sizeof(eState), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iLocation), sizeof(iLocation), 0);
}

void CombatProcedureEvade::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eState), sizeof(eState));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iLocation), sizeof(iLocation));
}

void CombatProcedureEvade::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::InitLoadGame(apLoadGameBuffer);
}
