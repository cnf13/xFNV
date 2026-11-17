#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/Combat/GroupMemberData.h"
#include "Reconstruction.hpp"

class CombatGroup;

// Scratch data of a combat group strategy: a name, a count, the group's average health and the ranged and melee member
// lists (fields as IDA has them). The PDB's CombatGroupData (IDA: CombatGroupStrategyData).
class CombatGroupData {
public:
	CombatGroupData(CombatGroup* apCombatGroup);

	void ComputeMinMaxTargetDetection();
	// iMinimumTargetDetection, computed first by ComputeMinMaxTargetDetection while it still holds INT_MAX. IDA
	// name; not in the PDB.
	// Inferred type: return: int32_t: signed uses from 0x990da2
	int32_t GetMemberMinimumTargetDetection();
	// iMaximumTargetDetection, computed first by ComputeMinMaxTargetDetection while it still holds INT_MIN.
	int32_t GetMemberMaximumTargetDetection();
	// The PDB's CombatGroupData::GetMeleeMember.
	GroupMemberData* GetMeleeMember(uint32_t aiIndex);
	// The PDB's CombatGroupData::GetRangedMember.
	GroupMemberData* GetRangedMember(uint32_t aiIndex);
	// Sorts the ranged members by their squared distance to the ranged members' centre, nearest first (a selection
	// sort; thiscall, ret 0, 0x990640). The PDB's CombatGroupData::SortRangedMembers (IDA:
	// CombatGroup::SortFlankActors).
	void SortRangedMembers();
	int32_t GetRangedMemberCount();
	uint32_t GetMeleeMemberCount();
	bool GetMembersTargetsIntersecting();
	// Inferred type: return: double: used as one at 0x990baa
	double GetAverageGroupHealth();

	CombatGroup* pCombatGroup;
	uint32_t iMemberCount;
	// Whether the members' targets intersect, -1 until computed (IDA: _DWORD dword8).
	int32_t iMembersTargetsIntersecting;
	// The lowest and highest detection level of the members' targets, INT_MAX and INT_MIN until computed.
	int32_t iMinimumTargetDetection;
	int32_t iMaximumTargetDetection;
	float fMemberHealthPercentage;
	// The ranged members (PDB CombatGroupData::RangedMembers).
	BSSimpleArray<GroupMemberData> RangedMembers;
	// The melee members (PDB CombatGroupData::MeleeMembers).
	BSSimpleArray<GroupMemberData> MeleeMembers;
};
