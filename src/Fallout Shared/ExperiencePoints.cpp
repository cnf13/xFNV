#include "Fallout Shared/ExperiencePoints.h"

#include "BSMain/Setting.h"

XPActivitySettingPairs s_xpRewardKillOpponent{{&iXPLevelKillCreatureVeryEasy, &iXPRewardKillOpponentVeryEasy},
	{&iXPLevelKillCreatureEasy, &iXPRewardKillOpponentEasy},
	{&iXPLevelKillCreatureAverage, &iXPRewardKillOpponentAverage},
	{&iXPLevelKillCreatureHard, &iXPRewardKillOpponentHard},
	{&iXPLevelKillCreatureVeryHard, &iXPRewardKillOpponentVeryHard}};
XPActivitySettingPairs s_xpRewardKillNPC{{&iXPLevelKillNPCVeryEasy, &iXPRewardKillNPCVeryEasy},
	{&iXPLevelKillNPCEasy, &iXPRewardKillNPCEasy}, {&iXPLevelKillNPCAverage, &iXPRewardKillNPCAverage},
	{&iXPLevelKillNPCHard, &iXPRewardKillNPCHard}, {&iXPLevelKillNPCVeryHard, &iXPRewardKillNPCVeryHard}};
XPActivitySettingPairs s_xpRewardHackComputer{{&iXPLevelHackComputerVeryEasy, &iXPRewardHackComputerVeryEasy},
	{&iXPLevelHackComputerEasy, &iXPRewardHackComputerEasy},
	{&iXPLevelHackComputerAverage, &iXPRewardHackComputerAverage},
	{&iXPLevelHackComputerHard, &iXPRewardHackComputerHard},
	{&iXPLevelHackComputerVeryHard, &iXPRewardHackComputerVeryHard}};
XPActivitySettingPairs s_xpRewardPickLock{{&iXPLevelPickLockVeryEasy, &iXPRewardPickLockVeryEasy},
	{&iXPLevelPickLockEasy, &iXPRewardPickLockEasy}, {&iXPLevelPickLockAverage, &iXPRewardPickLockAverage},
	{&iXPLevelPickLockHard, &iXPRewardPickLockHard}, {&iXPLevelPickLockVeryHard, &iXPRewardPickLockVeryHard}};
XPActivitySettingPairs s_xpRewardSpeechChallenge{{&iXPLevelSpeechChallengeVeryEasy, &iXPRewardSpeechChallengeVeryEasy},
	{&iXPLevelSpeechChallengeEasy, &iXPRewardSpeechChallengeEasy},
	{&iXPLevelSpeechChallengeAverage, &iXPRewardSpeechChallengeAverage},
	{&iXPLevelSpeechChallengeHard, &iXPRewardSpeechChallengeHard},
	{&iXPLevelSpeechChallengeVeryHard, &iXPRewardSpeechChallengeVeryHard}};

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<GameSettingCollection> iXPLevelKillCreatureVeryEasy("iXPLevelKillCreatureVeryEasy", 2);
SettingT<GameSettingCollection> iXPLevelKillCreatureEasy("iXPLevelKillCreatureEasy", 5);
SettingT<GameSettingCollection> iXPLevelKillCreatureAverage("iXPLevelKillCreatureAverage", 10);
SettingT<GameSettingCollection> iXPLevelKillCreatureHard("iXPLevelKillCreatureHard", 12);
SettingT<GameSettingCollection> iXPLevelKillCreatureVeryHard("iXPLevelKillCreatureVeryHard", 15);
SettingT<GameSettingCollection> iXPLevelKillNPCVeryEasy("iXPLevelKillNPCVeryEasy", 2);
SettingT<GameSettingCollection> iXPLevelKillNPCEasy("iXPLevelKillNPCEasy", 5);
SettingT<GameSettingCollection> iXPLevelKillNPCAverage("iXPLevelKillNPCAverage", 10);
SettingT<GameSettingCollection> iXPLevelKillNPCHard("iXPLevelKillNPCHard", 12);
SettingT<GameSettingCollection> iXPLevelKillNPCVeryHard("iXPLevelKillNPCVeryHard", 15);
SettingT<GameSettingCollection> iXPRewardKillOpponentVeryEasy("iXPRewardKillOpponentVeryEasy", 0);
SettingT<GameSettingCollection> iXPRewardKillOpponentEasy("iXPRewardKillOpponentEasy", 10);
SettingT<GameSettingCollection> iXPRewardKillOpponentAverage("iXPRewardKillOpponentAverage", 20);
SettingT<GameSettingCollection> iXPRewardKillOpponentHard("iXPRewardKillOpponentHard", 30);
SettingT<GameSettingCollection> iXPRewardKillOpponentVeryHard("iXPRewardKillOpponentVeryHard", 50);
SettingT<GameSettingCollection> iXPRewardKillNPCVeryEasy("iXPRewardKillNPCVeryEasy", 0);
SettingT<GameSettingCollection> iXPRewardKillNPCEasy("iXPRewardKillNPCEasy", 10);
SettingT<GameSettingCollection> iXPRewardKillNPCAverage("iXPRewardKillNPCAverage", 20);
SettingT<GameSettingCollection> iXPRewardKillNPCHard("iXPRewardKillNPCHard", 30);
SettingT<GameSettingCollection> iXPRewardKillNPCVeryHard("iXPRewardKillNPCVeryHard", 50);
SettingT<GameSettingCollection> iXPLevelHackComputerVeryEasy("iXPLevelHackComputerVeryEasy", 0);
SettingT<GameSettingCollection> iXPLevelHackComputerEasy("iXPLevelHackComputerEasy", 1);
SettingT<GameSettingCollection> iXPLevelHackComputerAverage("iXPLevelHackComputerAverage", 2);
SettingT<GameSettingCollection> iXPLevelHackComputerHard("iXPLevelHackComputerHard", 3);
SettingT<GameSettingCollection> iXPLevelHackComputerVeryHard("iXPLevelHackComputerVeryHard", 4);
SettingT<GameSettingCollection> iXPRewardHackComputerVeryEasy("iXPRewardHackComputerVeryEasy", 5);
SettingT<GameSettingCollection> iXPRewardHackComputerEasy("iXPRewardHackComputerEasy", 10);
SettingT<GameSettingCollection> iXPRewardHackComputerAverage("iXPRewardHackComputerAverage", 20);
SettingT<GameSettingCollection> iXPRewardHackComputerHard("iXPRewardHackComputerHard", 30);
SettingT<GameSettingCollection> iXPRewardHackComputerVeryHard("iXPRewardHackComputerVeryHard", 50);
SettingT<GameSettingCollection> iXPLevelPickLockVeryEasy("iXPLevelPickLockVeryEasy", 0);
SettingT<GameSettingCollection> iXPLevelPickLockEasy("iXPLevelPickLockEasy", 1);
SettingT<GameSettingCollection> iXPLevelPickLockAverage("iXPLevelPickLockAverage", 2);
SettingT<GameSettingCollection> iXPLevelPickLockHard("iXPLevelPickLockHard", 3);
SettingT<GameSettingCollection> iXPLevelPickLockVeryHard("iXPLevelPickLockVeryHard", 4);
SettingT<GameSettingCollection> iXPRewardPickLockVeryEasy("iXPRewardPickLockVeryEasy", 0);
SettingT<GameSettingCollection> iXPRewardPickLockEasy("iXPRewardPickLockEasy", 10);
SettingT<GameSettingCollection> iXPRewardPickLockAverage("iXPRewardPickLockAverage", 20);
SettingT<GameSettingCollection> iXPRewardPickLockHard("iXPRewardPickLockHard", 30);
SettingT<GameSettingCollection> iXPRewardPickLockVeryHard("iXPRewardPickLockVeryHard", 50);
SettingT<GameSettingCollection> iXPLevelSpeechChallengeVeryEasy("iXPLevelSpeechChallengeVeryEasy", 0);
SettingT<GameSettingCollection> iXPLevelSpeechChallengeEasy("iXPLevelSpeechChallengeEasy", 1);
SettingT<GameSettingCollection> iXPLevelSpeechChallengeAverage("iXPLevelSpeechChallengeAverage", 2);
SettingT<GameSettingCollection> iXPLevelSpeechChallengeHard("iXPLevelSpeechChallengeHard", 3);
SettingT<GameSettingCollection> iXPLevelSpeechChallengeVeryHard("iXPLevelSpeechChallengeVeryHard", 4);
SettingT<GameSettingCollection> iXPRewardSpeechChallengeVeryEasy("iXPRewardSpeechChallengeVeryEasy", 0);
SettingT<GameSettingCollection> iXPRewardSpeechChallengeEasy("iXPRewardSpeechChallengeEasy", 10);
SettingT<GameSettingCollection> iXPRewardSpeechChallengeAverage("iXPRewardSpeechChallengeAverage", 20);
SettingT<GameSettingCollection> iXPRewardSpeechChallengeHard("iXPRewardSpeechChallengeHard", 30);
SettingT<GameSettingCollection> iXPRewardSpeechChallengeVeryHard("iXPRewardSpeechChallengeVeryHard", 50);
SettingT<GameSettingCollection> iXPDeathRewardHealthThreshold("iXPDeathRewardHealthThreshold", 75);

bool ExperiencePoints::CheckDamageThreshold(float afPCDamage, float afOriginalHealth) {
	int32_t v3;
	if (!(afOriginalHealth > 0.0)) {
		v3 = 100;
	} else {
		v3 = (int32_t)((afPCDamage / afOriginalHealth) * 100.0);
	}
	return (bool)((uint8_t)(v3 > ((int32_t)((&iXPDeathRewardHealthThreshold)->UInt()))));
}

// The experience the activity awards at aiDifficulty: the reward of the first (difficulty, reward) setting pair whose
// difficulty is not below aiDifficulty, else the last pair's.
int32_t ExperiencePoints::GetExperiencePoints(Activity aeActivity, int32_t aiDifficulty) {
	int32_t iReward = 0;
	switch (aeActivity) {
	case ACTIVITY_KILL_CREATURE:
		iReward = GetExperiencePoints(aiDifficulty, s_xpRewardKillOpponent, 5);
		break;
	case ACTIVITY_KILL_NPC:
		iReward = GetExperiencePoints(aiDifficulty, s_xpRewardKillNPC, 5);
		break;
	case ACTIVITY_HACK_COMPUTER:
		iReward = GetExperiencePoints(aiDifficulty, s_xpRewardHackComputer, 5);
		break;
	case ACTIVITY_PICK_LOCK:
		iReward = GetExperiencePoints(aiDifficulty, s_xpRewardPickLock, 5);
		break;
	case ACTIVITY_SPEECH_CHALLENGE:
		iReward = GetExperiencePoints(aiDifficulty, s_xpRewardSpeechChallenge, 5);
		break;
	default:
		break;
	}
	return iReward;
}

// Walks aiPairCount (difficulty, reward) setting pairs and returns the reward of the first pair whose difficulty is not
// below aiDifficulty, else the last pair's.
int32_t ExperiencePoints::GetExperiencePoints(int32_t aiDifficulty, XPSettingPair* apDifficultyRewardList,
	int32_t aiPairCount) {
	int32_t i;
	int32_t v7 = (-1);
	uint32_t v6 = 0;
	for (i = 0; (i < aiPairCount) && (v7 < 0); ++i) {
		if (aiDifficulty > ((int32_t)(apDifficultyRewardList[i][0]->UInt()))) {
			v6 = apDifficultyRewardList[i][1]->UInt();
		} else {
			v7 = (int32_t)(apDifficultyRewardList[i][1]->UInt());
		}
	}
	if (v7 >= 0) {
		return (uint32_t)v7;
	} else {
		return v6;
	}
}
