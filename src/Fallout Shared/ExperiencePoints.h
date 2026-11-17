#pragma once

#include "BSMain/Setting.h"

// The (difficulty, reward) setting pairs of each activity's experience: five pairs, walked by the reward lookup.
using XPSettingPair = SettingT<GameSettingCollection>* [2];

// The experience point rules; all members are static and it has no instance layout.
class ExperiencePoints {
public:
	// The activities that award experience.
	enum Activity : int32_t {
		ACTIVITY_KILL_CREATURE = 0x0,
		ACTIVITY_KILL_NPC = 0x1,
		ACTIVITY_HACK_COMPUTER = 0x2,
		ACTIVITY_PICK_LOCK = 0x3,
		ACTIVITY_SPEECH_CHALLENGE = 0x4,
		ACTIVITY_COUNT = 0x5,
	};

	// True if the player did more than iXPDeathRewardHealthThreshold percent of the target's original health.
	static bool CheckDamageThreshold(float afPCDamage, float afOriginalHealth);

	// The experience the activity gives at the difficulty.
	static int32_t GetExperiencePoints(Activity aeActivity, int32_t aiDifficulty);
	// Walks aiPairCount (difficulty, reward) setting pairs and returns the reward of the first pair whose difficulty
	// is not below aiDifficulty, else the last pair's reward.
	static int32_t GetExperiencePoints(int32_t aiDifficulty, XPSettingPair* apDifficultyRewardList,
		int32_t aiPairCount);
};

using XPActivitySettingPairs = XPSettingPair[5];
extern XPActivitySettingPairs s_xpRewardKillOpponent;
extern XPActivitySettingPairs s_xpRewardKillNPC;
extern XPActivitySettingPairs s_xpRewardHackComputer;
extern XPActivitySettingPairs s_xpRewardPickLock;
extern XPActivitySettingPairs s_xpRewardSpeechChallenge;

// The game settings ExperiencePoints.cpp defines.
extern SettingT<GameSettingCollection> iXPRewardSpeechChallengeAverage;
extern SettingT<GameSettingCollection> iXPRewardSpeechChallengeHard;
extern SettingT<GameSettingCollection> iXPRewardPickLockEasy;
extern SettingT<GameSettingCollection> iXPRewardKillOpponentAverage;
extern SettingT<GameSettingCollection> iXPRewardKillNPCVeryEasy;
extern SettingT<GameSettingCollection> iXPLevelPickLockHard;
extern SettingT<GameSettingCollection> iXPLevelSpeechChallengeHard;
extern SettingT<GameSettingCollection> iXPLevelPickLockEasy;
extern SettingT<GameSettingCollection> iXPRewardKillNPCEasy;
extern SettingT<GameSettingCollection> iXPLevelKillCreatureEasy;
extern SettingT<GameSettingCollection> iXPRewardPickLockVeryHard;
extern SettingT<GameSettingCollection> iXPLevelHackComputerAverage;
extern SettingT<GameSettingCollection> iXPRewardSpeechChallengeVeryHard;
extern SettingT<GameSettingCollection> iXPLevelPickLockAverage;
extern SettingT<GameSettingCollection> iXPRewardPickLockHard;
extern SettingT<GameSettingCollection> iXPRewardSpeechChallengeVeryEasy;
extern SettingT<GameSettingCollection> iXPLevelKillCreatureVeryHard;
extern SettingT<GameSettingCollection> iXPLevelKillCreatureAverage;
extern SettingT<GameSettingCollection> iXPRewardSpeechChallengeEasy;
extern SettingT<GameSettingCollection> iXPRewardKillNPCAverage;
extern SettingT<GameSettingCollection> iXPRewardHackComputerHard;
extern SettingT<GameSettingCollection> iXPRewardKillOpponentVeryEasy;
extern SettingT<GameSettingCollection> iXPRewardHackComputerEasy;
extern SettingT<GameSettingCollection> iXPRewardKillNPCVeryHard;
extern SettingT<GameSettingCollection> iXPRewardKillNPCHard;
extern SettingT<GameSettingCollection> iXPRewardKillOpponentHard;
extern SettingT<GameSettingCollection> iXPLevelPickLockVeryEasy;
extern SettingT<GameSettingCollection> iXPDeathRewardHealthThreshold;
extern SettingT<GameSettingCollection> iXPLevelKillCreatureVeryEasy;
extern SettingT<GameSettingCollection> iXPRewardHackComputerVeryHard;
extern SettingT<GameSettingCollection> iXPRewardPickLockVeryEasy;
extern SettingT<GameSettingCollection> iXPRewardHackComputerAverage;
extern SettingT<GameSettingCollection> iXPRewardHackComputerVeryEasy;
extern SettingT<GameSettingCollection> iXPLevelPickLockVeryHard;
extern SettingT<GameSettingCollection> iXPRewardKillOpponentEasy;
extern SettingT<GameSettingCollection> iXPLevelHackComputerHard;
extern SettingT<GameSettingCollection> iXPRewardPickLockAverage;
extern SettingT<GameSettingCollection> iXPLevelSpeechChallengeAverage;
extern SettingT<GameSettingCollection> iXPLevelHackComputerEasy;
extern SettingT<GameSettingCollection> iXPLevelKillCreatureHard;
extern SettingT<GameSettingCollection> iXPRewardKillOpponentVeryHard;
extern SettingT<GameSettingCollection> iXPLevelKillNPCHard;
extern SettingT<GameSettingCollection> iXPLevelSpeechChallengeVeryHard;
extern SettingT<GameSettingCollection> iXPLevelKillNPCVeryHard;
extern SettingT<GameSettingCollection> iXPLevelKillNPCVeryEasy;
extern SettingT<GameSettingCollection> iXPLevelKillNPCEasy;
extern SettingT<GameSettingCollection> iXPLevelHackComputerVeryEasy;
extern SettingT<GameSettingCollection> iXPLevelKillNPCAverage;
extern SettingT<GameSettingCollection> iXPLevelHackComputerVeryHard;
extern SettingT<GameSettingCollection> iXPLevelSpeechChallengeVeryEasy;
extern SettingT<GameSettingCollection> iXPLevelSpeechChallengeEasy;
