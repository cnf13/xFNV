#include "Fallout/AI/Combat/CombatGroupStrategy.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/NewOverloads.h"
#include "BSMain/XINISetting.h"
#include "Fallout Shared/BGSWorldLocation.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatAction.h"
#include "Fallout/AI/Combat/CombatGoal.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatGroupStrategyData.h"
#include "Fallout/AI/Combat/CombatPlan.h"
#include "Fallout/AI/Combat/CombatPlanner.h"
#include "Fallout/AI/Combat/CombatWorldState.h"
#include "Fallout/AI/Combat/GroupMemberData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/string.h"

#include <cctype>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bDisableCombatGroupStrategies("bDisableCombatGroupStrategies:Combat", true);

CombatGroupStrategy::CombatGroupStrategy(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType) : eType(aeType) {}

bool CombatGroupStrategy::CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    return false;
}

bool CombatGroupStrategy::Update(CombatGroup* apGroup) {
    return false;
}

char* CombatGroupStrategy::GetName() {
    return CombatGroupStrategy::kNames[GetType()];
}

// The index compare is unsigned.
void CombatGroupStrategy::InitGroupStrategies() {
    for (uint32_t i = 0; i < COMBAT_GROUP_STRATEGY_COUNT; ++i) {
        pCombatGroupStrategies[i] = InitNthCombatGroupStrategy(static_cast<COMBAT_GROUP_STRATEGY>(i));
    }
}

CombatGroupStrategy* CombatGroupStrategy::GetCombatGroupStrategy(uint32_t aiIndex) {
    return CombatGroupStrategy::pCombatGroupStrategies[static_cast<int32_t>(aiIndex)];
}

CombatGroupStrategy* CombatGroupStrategy::LookupByName(char* apName) {
    char cName[264];
    BSstrcpy(cName, 260, apName);
    for (char* i = cName; *i; ++i) {
        *i = std::toupper(*i);
    }
    for (uint32_t uiIndex = 0; uiIndex < 4; ++uiIndex) {
        if (std::strstr(CombatGroupStrategy::kNames[uiIndex], cName)) {
            return CombatGroupStrategy::GetCombatGroupStrategy(uiIndex);
        }
    }
    return nullptr;
}

CombatGroupStrategy* InitNthCombatGroupStrategy(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType) {
    CombatGroupStrategy* pstrategy;
    switch (aeType) {
        case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_FALLBACK: {
            CombatGroupStrategyFallBack* pfallBack = new CombatGroupStrategyFallBack(aeType);
            pstrategy = pfallBack;
            break;
        }
        case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_ADVANCE: {
            CombatGroupStrategyAdvance* padvance = new CombatGroupStrategyAdvance(aeType);
            pstrategy = padvance;
            break;
        }
        case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_FLANK: {
            CombatGroupStrategyFlank* pflank = new CombatGroupStrategyFlank(aeType);
            pstrategy = pflank;
            break;
        }
        case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_AMBUSH: {
            CombatGroupStrategyAmbush* pambush = new CombatGroupStrategyAmbush(aeType);
            pstrategy = pambush;
            break;
        }
        default: {
            Warning("COMBAT: Could not create combat group strategy of type %i", aeType);
            pstrategy = nullptr;
            break;
        }
    }
    return pstrategy;
}

void DestroyCombatGroupStrategies() {
    for (CombatGroupStrategy* pStrategy : CombatGroupStrategy::pCombatGroupStrategies) {
        ourFree(pStrategy);
    }
}

CombatGroupStrategyFallBack::CombatGroupStrategyFallBack(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType)
  : CombatGroupStrategy(aeType) {}

bool CombatGroupStrategyFallBack::CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    if (static_cast<int32_t>(apGroupData->iMemberCount) < 2) {
        return false;
    }
    if (apGroup->CheckGroupStrategyChosenTimeStamp(0, 30.0f)) {
        return !(apGroupData->GetAverageGroupHealth() > 0.33000001311302185);
    }
    return false;
}

// Every member falls back. The member count is read again on every pass.
void CombatGroupStrategyFallBack::Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    for (uint32_t i = 0; i < apGroup->GetMemberCount(); ++i) {
        apGroup->SetGroupStrategyAssignment(i, STRATEGY_ASSIGNMENT_FALLING_BACK);
    }
}

// The fallback goes on while a member still has the falling-back assignment.
bool CombatGroupStrategyFallBack::Update(CombatGroup* apGroup) {
    return apGroup->FindMemberWithStrategyAssignment(STRATEGY_ASSIGNMENT_FALLING_BACK) != nullptr;
}

CombatGroupStrategyFlank::CombatGroupStrategyFlank(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType)
  : CombatGroupStrategy(aeType) {}

bool CombatGroupStrategyFlank::CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    if (static_cast<int32_t>(apGroupData->iMemberCount) < 3) {
        return false;
    }
    if (apGroupData->GetRangedMemberCount() < 1) {
        return false;
    }
    if (!apGroup->CheckGroupStrategyChosenTimeStamp(2u, 30.0f)) {
        return false;
    }
    if (apGroupData->GetMemberMaximumTargetDetection() > 0) {
        return !apGroupData->GetMembersTargetsIntersecting();
    }
    return false;
}

void CombatGroupStrategyFlank::Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    uint32_t imeleeMemberCount = apGroupData->GetMeleeMemberCount();
    for (uint32_t i = 0; i < imeleeMemberCount; ++i) {
        GroupMemberData* pmember = apGroupData->GetMeleeMember(i);
        apGroup->SetGroupStrategyAssignment(pmember->iIndex, 1u);
    }
    uint32_t irangedMemberCount = apGroupData->GetRangedMemberCount();
    float frangedPercentage = static_cast<double>(irangedMemberCount) / static_cast<double>(apGroupData->iMemberCount);
    uint32_t irangedFlankers = 0;
    if (frangedPercentage > 0.5) {
        irangedFlankers = static_cast<int64_t>((frangedPercentage - 0.5) * static_cast<double>(irangedMemberCount));
    }
    if (irangedFlankers) {
        apGroupData->SortRangedMembers();
        for (uint32_t i = irangedMemberCount - irangedFlankers; i < irangedMemberCount; ++i) {
            GroupMemberData* pmember = apGroupData->GetRangedMember(i);
            apGroup->SetGroupStrategyAssignment(pmember->iIndex, 1u);
        }
    }
}

// A flanking member with a target within 384 units of it stops flanking; the flank goes on while a flanking member has
// none. The member count is read once.
bool CombatGroupStrategyFlank::Update(CombatGroup* apGroup) {
    bool bFlanking = false;
    uint32_t uiCount = apGroup->GetMemberCount();
    for (uint32_t i = 0; i < uiCount; ++i) {
        if (apGroup->GetMemberGroupStrategyAssignment(static_cast<int32_t>(i)) == STRATEGY_ASSIGNMENT_FLANKING) {
            Actor* pMember = apGroup->GetMember(i);
            BGSWorldLocation kLocation;
            if (apGroup->FindTargetNearLocation(*pMember->GetWorldLocation(&kLocation), 384.0f, false)) {
                apGroup->SetGroupStrategyAssignment(static_cast<int32_t>(i), STRATEGY_ASSIGNMENT_NONE);
            }
            else {
                bFlanking = true;
            }
        }
    }
    return bFlanking;
}

CombatGroupStrategyAmbush::CombatGroupStrategyAmbush(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType)
  : CombatGroupStrategy(aeType) {}

bool CombatGroupStrategyAmbush::CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    if (static_cast<int32_t>(apGroupData->iMemberCount) < 2) {
        return false;
    }
    if (!apGroup->CheckGroupStrategyChosenTimeStamp(3u, 60.0f)) {
        return false;
    }
    if (apGroupData->GetMemberMaximumTargetDetection() > 0) {
        return !apGroupData->GetMembersTargetsIntersecting();
    }
    return false;
}

// The first half of the members fall back; the rest ambush. The member count is read again on every pass of the second
// loop.
void CombatGroupStrategyAmbush::Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    uint32_t uiHalf = apGroup->GetMemberCount() >> 1;
    for (uint32_t i = 0; i < uiHalf; ++i) {
        apGroup->SetGroupStrategyAssignment(i, STRATEGY_ASSIGNMENT_FALLING_BACK);
    }
    for (uint32_t i = uiHalf; i < apGroup->GetMemberCount(); ++i) {
        apGroup->SetGroupStrategyAssignment(i, STRATEGY_ASSIGNMENT_AMBUSHING);
    }
}

// The ambush goes on while a member still has the ambushing assignment.
bool CombatGroupStrategyAmbush::Update(CombatGroup* apGroup) {
    return apGroup->FindMemberWithStrategyAssignment(STRATEGY_ASSIGNMENT_AMBUSHING) != nullptr;
}

CombatGroupStrategyAdvance::CombatGroupStrategyAdvance(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType)
  : CombatGroupStrategy(aeType) {}

bool CombatGroupStrategyAdvance::CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    if (apGroupData->GetRangedMemberCount() < 2) {
        return false;
    }
    if (!apGroup->CheckGroupStrategyChosenTimeStamp(1u, 60.0f)) {
        return false;
    }
    if (apGroupData->GetMemberMinimumTargetDetection() <= 0) {
        return false;
    }
    uint32_t irangedMemberCount = apGroupData->GetRangedMemberCount();
    for (uint32_t i = 0; i < irangedMemberCount; ++i) {
        GroupMemberData* prangedMember = apGroupData->GetRangedMember(i);
        CombatController* pcombatController = prangedMember->pActor->GetCombatController();
        Actor* ptarget = pcombatController->GetTarget();
        if (ptarget) {
            const NiPoint3* kTargetPos = &ptarget->GetLocationOnReference();
            NiPoint3 targetVector = prangedMember->pActor->GetLocationOnReference() - *kTargetPos;
            if (targetVector.SqrLength() < 1048576.0) {
                return false;
            }
        }
    }
    return true;
}

// Every ranged member advances. The ranged member count is read again on every pass.
void CombatGroupStrategyAdvance::Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
    for (int32_t i = 0; i < apGroupData->GetRangedMemberCount(); ++i) {
        GroupMemberData* pMember = apGroupData->GetRangedMember(i);
        apGroup->SetGroupStrategyAssignment(pMember->iIndex, STRATEGY_ASSIGNMENT_ADVANCING);
    }
}

// The advance goes on while a member still has the advancing assignment.
bool CombatGroupStrategyAdvance::Update(CombatGroup* apGroup) {
    return apGroup->FindMemberWithStrategyAssignment(STRATEGY_ASSIGNMENT_ADVANCING) != nullptr;
}

int32_t CombatGroupData::GetRangedMemberCount() {
    return RangedMembers.iSize;
}

uint32_t CombatGroupData::GetMeleeMemberCount() {
    return MeleeMembers.iSize;
}

GroupMemberData::GroupMemberData(Actor* apActor, uint32_t aiIndex) : pActor(apActor), iIndex(aiIndex) {}
