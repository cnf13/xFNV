#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatSearchDoor.h"

// The combat state of an actor group. This is the same object as CombatGroup, and its members are CombatGroup's.
class CombatActors {
public:
	uint32_t iGroupNum;
	uint32_t groupID;
	BSSimpleArray<CombatTarget> targets;
	BSSimpleArray<CombatMember> MemberArray;
	CombatGroupStrategy* pGroupStrategy;
	bool bStrategyForced;
	uint8_t padding2D[3];
	CombatTimer ChooseStrategyTimer;
	CombatTimer UpdateStrategyTimer;
	uint32_t iLastGroupStrategyChosenIndex;
	CombatTimeStamp fLastGroupStrategyChosenTimeStamp;
	CombatTimer AvoidThreatDialogueTimer;
	CombatTimer CombatStrengthTimer;
	CombatTimer DetectionDialogueTimers[10];
	CombatTimer TargetUpdateTimer;
	CombatTimer ClusterUpdateTimer;
	CombatTimer CombatMusicUpdateTimer;
	float sumThreatValue;
	float averageDPS;
	float fTargetCombatStrength;
	float fAverageTargetCombatStrength;
	uint32_t targetSearchState;
	NiPointer<PathingLOSGridMap> spPathingLOSGridMap;
	CombatTimer SearchUpdateTimer;
	CombatTimer SearchAreaUpdateTimer;
	CombatTimeStamp fSearchStartedTimeStamp;
	Actor* pSearchingMember;
	BGSWorldLocation SearchCenter;
	NiPoint3 SearchFocalPoint;
	float fSearchRadius;
	BSSimpleArray<CombatSearchLocation> searchLocations;
	BSSimpleArray<CombatSearchDoor> searchDoors;
	NiPointer<NiNode> spSearchDebugGeometry;
	bool bUpdateSearchDebugGeometry;
	uint8_t padding135[3];
	BSSimpleArray<CombatGroupCluster*> ClusterArray;
	uint32_t iInitializedMemberCount;
	uint32_t iFleeingMemberCount;
	uint32_t iNonFleeingMemberCount;
	uint8_t cCombatMusicState;
	uint8_t padding155[3];
	NiPointer<NiNode> spDebugGeometry;
};
