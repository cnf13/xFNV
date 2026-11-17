#include "Fallout/AI/Combat/CombatGroupStrategy.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
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
#include "Reconstruction.hpp"
#include "SDK/MSVC/string.h"

#include <cctype>

// Not named in the PDB or IDA; name chosen here. Five name pointers no code of the exe reads, between kNames and the
// RTTI type descriptor at 0x11A4EF4.
using CombatGroupStrategyDisplayNameArray = const char* [5];
static CombatGroupStrategyDisplayNameArray pCombatGroupStrategyNames{"NONE", "FLANKING", "FALLING BACK", "AMBUSHING",
	"ADVANCING"};

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
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

// The exe compares the index unsigned.
void CombatGroupStrategy::InitGroupStrategies() {
	for (uint32_t i = 0; i < COMBAT_GROUP_STRATEGY_COUNT; ++i) {
		pCombatGroupStrategies[i] = InitNthCombatGroupStrategy(static_cast<COMBAT_GROUP_STRATEGY>(i));
	}
}

CombatGroupStrategy* CombatGroupStrategy::GetCombatGroupStrategy(uint32_t aiIndex) {
	return CombatGroupStrategy::pCombatGroupStrategies[(int32_t)aiIndex];
}

CombatGroupStrategy* CombatGroupStrategy::LookupByName(char* a1) {
	char SubStr[264];
	BSstrcpy(SubStr, 260, a1);
	for (char* i = SubStr; *i; ++i) {
		*i = std::toupper(*i);
	}
	for (uint32_t aiIndex = 0; aiIndex < 4; ++aiIndex) {
		if (std::strstr(CombatGroupStrategy::kNames[aiIndex], SubStr)) {
			return CombatGroupStrategy::GetCombatGroupStrategy(aiIndex);
		}
	}
	return 0;
}

// The exe's cleanup-only C++ EH frame is left out.
CombatGroupStrategy* InitNthCombatGroupStrategy(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType) {
	CombatGroupStrategy* result;
	switch (aeType) {
	case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_FALLBACK: {
		CombatGroupStrategyFallBack* v5;
		CombatGroupStrategyFallBack* apMemory =
			static_cast<CombatGroupStrategyFallBack*>(ourMalloc(sizeof(CombatGroupStrategyFallBack)));
		if (apMemory) {
			v5 = ::new (apMemory) CombatGroupStrategyFallBack(aeType);
		} else {
			v5 = 0;
		}
		result = v5;
		break;
	}
	case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_ADVANCE: {
		CombatGroupStrategyAdvance* v2;
		CombatGroupStrategyAdvance* v6 =
			static_cast<CombatGroupStrategyAdvance*>(ourMalloc(sizeof(CombatGroupStrategyAdvance)));
		if (v6) {
			v2 = ::new (v6) CombatGroupStrategyAdvance(aeType);
		} else {
			v2 = 0;
		}
		result = v2;
		break;
	}
	case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_FLANK: {
		CombatGroupStrategyFlank* v4;
		CombatGroupStrategyFlank* v8 =
			static_cast<CombatGroupStrategyFlank*>(ourMalloc(sizeof(CombatGroupStrategyFlank)));
		if (v8) {
			v4 = ::new (v8) CombatGroupStrategyFlank(aeType);
		} else {
			v4 = 0;
		}
		result = v4;
		break;
	}
	case CombatGroupStrategy::COMBAT_GROUP_STRATEGY_AMBUSH: {
		CombatGroupStrategyAmbush* v3;
		CombatGroupStrategyAmbush* v7 =
			static_cast<CombatGroupStrategyAmbush*>(ourMalloc(sizeof(CombatGroupStrategyAmbush)));
		if (v7) {
			v3 = ::new (v7) CombatGroupStrategyAmbush(aeType);
		} else {
			v3 = 0;
		}
		result = v3;
		break;
	}
	default: {
		Warning("COMBAT: Could not create combat group strategy of type %i", aeType);
		result = 0;
		break;
	}
	}
	return result;
}

void DestroyCombatGroupStrategies() {
	for (uint32_t i = 0; i < 4; ++i) {
		ourFree(CombatGroupStrategy::pCombatGroupStrategies[i]);
	}
}

CombatGroupStrategyFallBack::CombatGroupStrategyFallBack(CombatGroupStrategy::COMBAT_GROUP_STRATEGY aeType)
	: CombatGroupStrategy(aeType) {}

bool CombatGroupStrategyFallBack::CanExecute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
	if (static_cast<int32_t>(apGroupData->iMemberCount) < 2) {
		return 0;
	}
	if (apGroup->CheckGroupStrategyChosenTimeStamp(0, 30.0f)) {
		return !(apGroupData->GetAverageGroupHealth() > 0.33000001311302185);
	}
	return 0;
}

// Every member falls back. The exe reads the member count again on every pass.
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
		return 0;
	}
	if (apGroupData->GetRangedMemberCount() < 1) {
		return 0;
	}
	if (!apGroup->CheckGroupStrategyChosenTimeStamp(2u, 30.0f)) {
		return 0;
	}
	if (apGroupData->GetMemberMaximumTargetDetection() > 0) {
		return !apGroupData->GetMembersTargetsIntersecting();
	}
	return 0;
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
		for (uint32_t i_2 = irangedMemberCount - irangedFlankers; i_2 < irangedMemberCount; ++i_2) {
			GroupMemberData* pmember_2 = apGroupData->GetRangedMember(i_2);
			apGroup->SetGroupStrategyAssignment(pmember_2->iIndex, 1u);
		}
	}
}

// A flanking member with a target within 384 units of it stops flanking; the flank goes on while a flanking member
// has none. The exe reads the member count once.
bool CombatGroupStrategyFlank::Update(CombatGroup* apGroup) {
	bool bFlanking = false;
	uint32_t uiCount = apGroup->GetMemberCount();
	for (uint32_t i = 0; i < uiCount; ++i) {
		if (apGroup->GetMemberGroupStrategyAssignment(static_cast<int32_t>(i)) == STRATEGY_ASSIGNMENT_FLANKING) {
			Actor* pMember = apGroup->GetMember(i);
			BGSWorldLocation kLocation;
			if (apGroup->FindTargetNearLocation(*pMember->GetWorldLocation(&kLocation), 384.0f, false)) {
				apGroup->SetGroupStrategyAssignment(static_cast<int32_t>(i), STRATEGY_ASSIGNMENT_NONE);
			} else {
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
		return 0;
	}
	if (!apGroup->CheckGroupStrategyChosenTimeStamp(3u, 60.0f)) {
		return 0;
	}
	if (apGroupData->GetMemberMaximumTargetDetection() > 0) {
		return !apGroupData->GetMembersTargetsIntersecting();
	}
	return 0;
}

// The first half of the members fall back; the rest ambush. The exe reads the member count again on every pass of
// the second loop.
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
		return 0;
	}
	if (!apGroup->CheckGroupStrategyChosenTimeStamp(1u, 60.0f)) {
		return 0;
	}
	if (apGroupData->GetMemberMinimumTargetDetection() <= 0) {
		return 0;
	}
	uint32_t irangedMemberCount = apGroupData->GetRangedMemberCount();
	for (uint32_t i = 0; i < irangedMemberCount; ++i) {
		GroupMemberData* prangedMember = apGroupData->GetRangedMember(i);
		CombatController* pcombatController = prangedMember->pActor->GetCombatController();
		Actor* ptarget = pcombatController->GetTarget();
		if (ptarget) {
			const NiPoint3* kTargetPos = &ptarget->GetLocationOnReference();
			const NiPoint3* v4 = &prangedMember->pActor->GetLocationOnReference();
			NiPoint3 targetVector = *v4 - *kTargetPos;
			if (targetVector.SqrLength() < 1048576.0) {
				return 0;
			}
		}
	}
	return 1;
}

// Every ranged member advances. The exe reads the ranged member count again on every pass.
void CombatGroupStrategyAdvance::Execute(CombatGroup* apGroup, CombatGroupData* apGroupData) {
	for (int32_t i = 0; i < static_cast<int32_t>(apGroupData->GetRangedMemberCount()); ++i) {
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
