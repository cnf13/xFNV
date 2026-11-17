#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/Combat/GroupMemberData.h"

class CombatGroup;

// Scratch data of a combat group strategy: a name, a count, the group's average health and the ranged and melee member
// lists.
class CombatGroupData {
public:
	CombatGroupData(CombatGroup* apCombatGroup);

	void ComputeMinMaxTargetDetection();
	// Returns iMinimumTargetDetection, computed first by ComputeMinMaxTargetDetection while it still holds INT_MAX.
	int32_t GetMemberMinimumTargetDetection();
	// iMaximumTargetDetection, computed first by ComputeMinMaxTargetDetection while it still holds INT_MIN.
	int32_t GetMemberMaximumTargetDetection();
	GroupMemberData* GetMeleeMember(uint32_t aiIndex);
	GroupMemberData* GetRangedMember(uint32_t aiIndex);
	// Sorts the ranged members by their squared distance to the ranged members' centre, nearest first (a selection
	// sort).
	void SortRangedMembers();
	int32_t GetRangedMemberCount();
	uint32_t GetMeleeMemberCount();
	bool GetMembersTargetsIntersecting();
	double GetAverageGroupHealth();

	CombatGroup* pCombatGroup;
	uint32_t iMemberCount;
	// Whether the members' targets intersect, -1 until computed.
	int32_t iMembersTargetsIntersecting;
	// The lowest and highest detection level of the members' targets, INT_MAX and INT_MIN until computed.
	int32_t iMinimumTargetDetection;
	int32_t iMaximumTargetDetection;
	float fMemberHealthPercentage;
	BSSimpleArray<GroupMemberData> RangedMembers;
	BSSimpleArray<GroupMemberData> MeleeMembers;
};
