#include "Fallout/AI/Combat/CombatWorldState.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMenu/Font.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatProcedureActivateObject.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/MiddleHighProcess.h"

CombatWorldStateArray::CombatWorldStateArray() {
    ClearValues();
}

void CombatWorldStateArray::ClearValues() {
    ourMemset(iValues, 0, sizeof(iValues));
    ourMemset(iSpecifiedVariables, 0, sizeof(iSpecifiedVariables));
}

void CombatWorldStateArray::Update(CombatController* apCombatController) {
    TESActorBaseData* v16;
    COMBAT_WEAPON_TYPE etype;
    BGSWorldLocation apLocation;
    NiPoint3 coverLocation;
    BGSWorldLocation attackerWorldLoc;
    ClearValues();
    apCombatController->CheckCurrentWeapon();
    CombatState* pcombatState = apCombatController->pCombatState;
    Actor* pattacker = apCombatController->pAttacker;
    Actor* ptarget = apCombatController->GetTarget();
    pattacker->GetWorldLocation(&attackerWorldLoc);
    BGSWorldLocation targetWorldLoc;
    if (ptarget) {
        targetWorldLoc = *ptarget->GetWorldLocation(&apLocation);
    }
    CombatGroup* pgroup = apCombatController->pCombatGroup;
    int32_t iatlocationValue = 0;
    CombatProcedure* pactionProcedure = apCombatController->pActionProcedure;
    CombatProcedure* pmovementProcedure = apCombatController->pMovementProcedure;
    CombatCoverLocation* pcoverLocation = pcombatState->GetCoverLocation();
    if (pcoverLocation) {
        if (!pcoverLocation->bDirtyFlag) {
            if (pmovementProcedure && pmovementProcedure->GetType() == CombatProcedure::BE_IN_COVER) {
                if (!pmovementProcedure->GetFailed()) {
                    iatlocationValue |= 1 << pcoverLocation->iLocation;
                }
            }
            else {
                CombatUtilities::CalculateCoverPositions(apCombatController, pcoverLocation, &coverLocation, nullptr);
                NiPoint3 vec = coverLocation - attackerWorldLoc.Point;
                vec.z = vec.z * 0.10000000149011612;
                if (vec.SqrLength() < 1024.0) {
                    iatlocationValue |= 1 << pcoverLocation->iLocation;
                }
            }
        }
    }
    if (pactionProcedure) {
        if (!pactionProcedure->GetFailed() && pactionProcedure->GetType() == CombatProcedure::ACTIVATE_OBJECT) {
            CombatProcedureActivateObject* pactivateProcedure =
                static_cast<CombatProcedureActivateObject*>(pactionProcedure);
            if (pactivateProcedure->pTargetRef) {
                for (uint32_t i = 0; i < 5; ++i) {
                    CombatState::TargetedObject* pcombatItem = pcombatState->GetTargetedObject(i);
                    if (pcombatItem) {
                        TESObjectREFR* v2 = pactivateProcedure->pTargetRef;
                        if (pcombatItem->pRef == v2) {
                            iatlocationValue |= 1 << (i + 17);
                        }
                    }
                }
            }
        }
    }
    if (ptarget) {
        if (targetWorldLoc.GetDistanceSquared(attackerWorldLoc) < 589824.0 && !pcombatState->GetTargetUnreachable() &&
            pcombatState->CheckMovement(targetWorldLoc, true, false)) {
            iatlocationValue |= 2u;
        }
    }
    iatlocationValue |= 128u;
    iatlocationValue |= 256u;
    iatlocationValue |= 512u;
    iatlocationValue |= 0x10000u;
    CombatLocationTarget* plocationTarget = pcombatState->pLocationTarget;
    if (plocationTarget) {
        if (!plocationTarget->bNearTarget) {
            switch (plocationTarget->cType) {
                case 0: {
                    iatlocationValue &= ~128u;
                    break;
                }
                case 1: {
                    iatlocationValue &= ~256u;
                    break;
                }
                case 2: {
                    iatlocationValue &= ~512u;
                    break;
                }
                case 3: {
                    iatlocationValue &= ~0x10000u;
                    break;
                }
                default: {
                    Warning("COMBAT: Unsupported target ref type %i in world state update", plocationTarget->cType);
                    break;
                }
            }
        }
    }
    if (pcombatState->cFleeState == 2) {
        iatlocationValue |= 0x1000u;
    }
    if (pmovementProcedure) {
        if (pmovementProcedure->GetType() == CombatProcedure::HIDE_FROM_TARGET && !pmovementProcedure->GetFailed() &&
            !pmovementProcedure->GetFinished()) {
            iatlocationValue |= 1024u;
        }
    }
    SetValue(COMBAT_WORLD_STATE_AT_LOCATION, iatlocationValue);
    SetValue(COMBAT_WORLD_STATE_NOT_AT_LOCATION, ~iatlocationValue);
    double AltTimer = pcombatState->fThreatScore;
    SetValue(COMBAT_WORLD_STATE_THREATENED, fCombatThreatSignificantScore.Float() < AltTimer);
    TESForm* FormTwo = pcombatState->pExplosiveProjectileBlocked;
    SetValue(COMBAT_WORLD_STATE_EXPLOSIVE_PROJECTILE_BLOCKED, FormTwo != nullptr);
    if (ptarget) {
        bool HasBufferedDetection = pcombatState->GetHasBufferedDetection();
        SetValue(COMBAT_WORLD_STATE_TARGET_DETECTED, HasBufferedDetection);
        if (!pcombatState->GetHasBuffered360LineOfSight() || pcombatState->bTargetSpaceDifferent) {
            SetValue(COMBAT_WORLD_STATE_TARGET_360_LOS, 0);
        }
        else {
            SetValue(COMBAT_WORLD_STATE_TARGET_360_LOS, 1);
        }
        bool TargetBlocked = pcombatState->bTargetBlocked;
        SetValue(COMBAT_WORLD_STATE_TARGET_BLOCKED, TargetBlocked);
        bool TargetSpaceDifferent = pcombatState->bTargetSpaceDifferent;
        SetValue(COMBAT_WORLD_STATE_TARGET_SPACE_DIFFERENT, TargetSpaceDifferent);
        // ElapsedTime rounds its result to a float; it is kept as a double for the compare.
        double felapsed = pcombatState->GetLastTargetDetectedTimeStamp().ElapsedTime();
        int32_t v19 = fCombatDetectionLostTime.Float() < felapsed;
        SetValue(COMBAT_WORLD_STATE_TARGET_LOST, v19);
        bool TargetFlushed = pcombatState->GetTargetFlushed();
        SetValue(COMBAT_WORLD_STATE_TARGET_FLUSHED, TargetFlushed);
    }
    else {
        SetValue(COMBAT_WORLD_STATE_TARGET_LOST, 1);
    }
    TESObjectWEAP* pweapon = pattacker->GetCurrentWeapon();
    if (pweapon) {
        etype = pweapon->GetCombatWeaponType();
    }
    else {
        etype = COMBAT_WEAPON_TYPE_COUNT;
    }
    COMBAT_WEAPON_TYPE etype_2 = etype;
    if (pweapon) {
        TESObjectWEAP* WeaponByCombatType = pcombatState->GetWeaponByCombatType(etype_2);
        if (pweapon != WeaponByCombatType) {
            etype_2 = COMBAT_WEAPON_TYPE_FULL_COUNT;
        }
    }
    SetValue(COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED, etype_2);
    SetSpecifiedVariable(0xFFFF0018u, etype_2);
    BaseProcess* CurrentAIProcess = pattacker->pCurrentProcess;
    if (CurrentAIProcess->GetWeaponDrawn()) {
        SetValue(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, etype_2);
    }
    else {
        SetValue(COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN, 7);
    }
    SetValue(COMBAT_WORLD_STATE_HAS_WEAPON_TYPE, static_cast<int32_t>(pcombatState->iHasCombatWeapon));
    for (uint32_t i_2 = 0; i_2 < 5; ++i_2) {
        if (pcombatState->GetTargetedObject(i_2)) {
            SetSpecifiedVariable(0xFFFF0016u, i_2 + 17);
            break;
        }
    }
    SetValue(COMBAT_WORLD_STATE_HAS_COMBAT_ITEM, static_cast<int32_t>(pcombatState->iHasCombatItem));
    for (uint32_t i_3 = 0; i_3 < 2; ++i_3) {
        if (pcombatState->GetUsableCombatItem(static_cast<int32_t>(i_3))) {
            CombatTimer* UsableCombatItemTimer = pcombatState->GetUsableCombatItemTimer(static_cast<int32_t>(i_3));
            if (UsableCombatItemTimer->Check()) {
                bool buseItem = false;
                if (i_3) {
                    if (i_3 == 1) {
                        buseItem = true;
                    }
                }
                else {
                    double HealthPercentage = pattacker->GetHealthPercentage();
                    if (static_cast<double>(iAICombatRestoreHealthPercentage.Int()) > HealthPercentage * 100.0) {
                        buseItem = true;
                    }
                }
                if (buseItem) {
                    SetSpecifiedVariable(0xFFFF0017u, i_3 + 17);
                    break;
                }
            }
        }
    }
    bool v14 = pcombatState->CheckShouldFlee();
    SetValue(COMBAT_WORLD_STATE_WANTS_TO_FLEE, v14);
    if (pgroup->pGroupStrategy) {
        uint32_t MemberGroupStrategyAssignment = pgroup->GetMemberGroupStrategyAssignment(pattacker);
        SetValue(COMBAT_WORLD_STATE_GROUP_STRATEGY_ASSIGNMENT, MemberGroupStrategyAssignment);
    }
    int32_t iexecutionFlags = pcombatState->iCombatPermissions;
    if (pgroup->IsSearching()) {
        iexecutionFlags |= 0x10000u;
    }
    if (apCombatController->bAttackerCanMove) {
        iexecutionFlags |= 0x20000u;
    }
    if (!ptarget || (v16 = static_cast<TESActorBaseData*>(ptarget->GetActorBase()), !v16->GetNoVATSMelee())) {
        iexecutionFlags |= 0x40000u;
    }
    SetValue(COMBAT_WORLD_STATE_EXECUTION_FLAGS, iexecutionFlags);
}

void CombatWorldStateArray::Copy(CombatWorldStateArray* apWorldStateArray) {
    ourMemcpy(iValues, apWorldStateArray->iValues, sizeof(iValues));
    ourMemcpy(iSpecifiedVariables, apWorldStateArray->iSpecifiedVariables, sizeof(iSpecifiedVariables));
}

void CombatWorldStateArray::ApplyWorldState(COMBAT_WORLD_STATE aeWorldState, int32_t aiDesiredValue) {
    if (kCombatWorldStateData[aeWorldState].bBitfield) {
        uint32_t uiValue = GetValue(aeWorldState);
        SetValue(aeWorldState, uiValue | (1 << aiDesiredValue));
    }
    else {
        SetValue(aeWorldState, aiDesiredValue);
    }
}

void CombatWorldStateArray::SetValue(COMBAT_WORLD_STATE aeWorldState, int32_t aiValue) {
    iValues[(int32_t)aeWorldState] = (uint32_t)aiValue;
}

bool CombatWorldStateArray::IsTrue(COMBAT_WORLD_STATE aeWorldState, int32_t aiDesiredValue) {
    if (kCombatWorldStateData[aeWorldState].bBitfield) {
        return (GetValue(aeWorldState) & (1 << aiDesiredValue)) != 0;
    }
    return GetValue(aeWorldState) == static_cast<uint32_t>(aiDesiredValue);
}

int32_t CombatWorldStateArray::GetStateValue(int32_t aiValue, int32_t aiUnsatisfiedValue) {
    if (aiValue == -1) {
        aiValue = aiUnsatisfiedValue;
    }
    if (CombatWorldState::IsSpecifiedVariable(aiValue)) {
        return GetSpecifiedVariable(aiValue);
    }
    return aiValue;
}

int32_t CombatWorldStateArray::GetValue(COMBAT_WORLD_STATE aeWorldState) {
    return (int32_t)iValues[(int32_t)aeWorldState];
}

void CombatWorldStateArray::SetSpecifiedVariable(int32_t aiSpecifiedVariable, int32_t aiValue) {
    iSpecifiedVariables[aiSpecifiedVariable - static_cast<int32_t>(0xFFFF0016u)] = aiValue;
}

int32_t CombatWorldStateArray::GetSpecifiedVariable(int32_t aiSpecifiedVariable) {
    return iSpecifiedVariables[aiSpecifiedVariable - static_cast<int32_t>(0xFFFF0016u)];
}

void CombatWorldStateArray::GetDebugText(char* apDebugText, uint32_t aiMaxLength, COMBAT_WORLD_STATE aeState) {
    int32_t ivalue = GetValue(aeState);
    if (kCombatWorldStateData[aeState].bBitfield) {
        bool bstarted = false;
        for (uint32_t i = 0; i < 32; ++i) {
            if ((ivalue & 1 << i) != 0) {
                if (bstarted) {
                    char Src[264];
                    BSsprintf(Src, 260u, ", %s", CombatWorldState::GetStateValueText(aeState, i));
                    int32_t v4 = aiMaxLength - BSstrlen(apDebugText);
                    BSstrcat(apDebugText, v4, Src);
                }
                else {
                    const char* StateValueText = CombatWorldState::GetStateValueText(aeState, i);
                    BSsprintf(
                        apDebugText, aiMaxLength, "%s: %s", CombatWorldState::GetStateName(aeState), StateValueText
                    );
                }
                bstarted = true;
            }
        }
        if (!bstarted) {
            BSsprintf(apDebugText, aiMaxLength, "%s: None", CombatWorldState::GetStateName(aeState));
        }
    }
    else {
        const char* v6 = CombatWorldState::GetStateValueText(aeState, ivalue);
        BSsprintf(apDebugText, aiMaxLength, "%s: %s", CombatWorldState::GetStateName(aeState), v6);
    }
}

CombatWorldStateArray* CombatWorldStateArray::CombatWorldStateArray_0(CombatWorldStateArray* a1) {
    Copy(a1);
    return this;
}

CombatWorldState::CombatWorldState(COMBAT_WORLD_STATE aeState, int32_t aiValue) : eState(aeState), iValue(aiValue) {}

bool CombatWorldState::ValueMatches(int32_t aiValue) {
    return (bool)((uint8_t)((((uint32_t)aiValue) == iValue) || ((((uint32_t)aiValue) == (-1)) || (iValue == (-1)))));
}

bool CombatWorldState::Matches(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    return (bool)((uint8_t)((((uint32_t)aeState) == ((uint32_t)((int32_t)eState))) && ValueMatches(aiValue)));
}

const char* CombatWorldState::GetStateName() {
    return kCombatWorldStateData[eState].pName;
}

void CombatWorldState::GetDebugText(char* apDebugText, uint32_t aiMaxLength, int32_t aiValue) {
    if (aiValue == -1) {
        aiValue = iValue;
    }
    const char* pValueText = CombatWorldState::GetStateValueText(eState, aiValue);
    BSsprintf(apDebugText, aiMaxLength, "%s: %s", GetStateName(), pValueText);
}

bool CombatWorldState::IsSpecifiedVariable(int32_t aiValue) {
    return (bool)((uint8_t)((aiValue >= ((int32_t)0xFFFF0016u)) && (aiValue < ((int32_t)0xFFFF0019u))));
}

const char* CombatWorldState::GetStateName(COMBAT_WORLD_STATE aeState) {
    return kCombatWorldStateData[aeState].pName;
}

const char* CombatWorldState::GetStateValueText(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    const char* result;
    switch (aeState) {
        case COMBAT_WORLD_STATE_AT_LOCATION:
        case COMBAT_WORLD_STATE_NOT_AT_LOCATION:
        case COMBAT_WORLD_STATE_HAS_COMBAT_ITEM: {
            result = s_combatActionMoveTypes[aiValue];
            break;
        }
        case COMBAT_WORLD_STATE_WEAPON_TYPE_DRAWN:
        case COMBAT_WORLD_STATE_WEAPON_TYPE_EQUIPPED:
        case COMBAT_WORLD_STATE_HAS_WEAPON_TYPE: {
            result = s_combatActionDrawWeaponTypes[aiValue];
            break;
        }
        case COMBAT_WORLD_STATE_EXECUTION_FLAGS: {
            result = s_combatActionInvestigateTypes[aiValue];
            break;
        }
        default: {
            const char* v3;
            if (aiValue) {
                v3 = "True";
            }
            else {
                v3 = "False";
            }
            result = v3;
            break;
        }
    }
    return result;
}

CombatWorldStateList::~CombatWorldStateList() {}

void CombatWorldStateList::AddWorldState(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    void* pMemory = ourMalloc(sizeof(CombatWorldState));
    CombatWorldState* pState = pMemory ? ::new (pMemory) CombatWorldState(aeState, aiValue) : nullptr;
    AddTail(pState);
}

void CombatWorldStateList::RemoveWorldState(COMBAT_WORLD_STATE aeState) {
    for (CombatWorldStateList* plist = this; plist; plist = plist->GetNext()) {
        CombatWorldState* pstate = plist->m_item;
        if (pstate && pstate->eState == aeState) {
            plist->Remove(pstate);
            ourFree(pstate);
            return;
        }
    }
}

bool CombatWorldStateList::GetWorldStateActionValue(COMBAT_WORLD_STATE aeState, int32_t& ariActionValue) {
    CombatWorldStateList* pThis = this;
    while (pThis) {
        CombatWorldState* pstate = pThis->m_item;
        if (pstate && pstate->eState == aeState) {
            ariActionValue = pstate->iValue;
            return true;
        }
        pThis = pThis->GetNext();
    }
    return false;
}

bool CombatWorldStateList::HasMatchingWorldState(COMBAT_WORLD_STATE aeState, int32_t aiValue) {
    CombatWorldStateList* pThis = this;
    while (pThis) {
        CombatWorldState* pstate = pThis->m_item;
        if (pstate && pstate->Matches(aeState, aiValue)) {
            return true;
        }
        pThis = pThis->GetNext();
    }
    return false;
}

CombatWorldStateList* CombatWorldStateList::GetNext() {
    return static_cast<CombatWorldStateList*>(m_pkNext);
}

bool CombatState::GetTargetFlushed() {
    return cTargetFlushed != 0;
}
