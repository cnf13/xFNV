#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"

void FakeModalMessageMenuCallback();

// Which of a reputation's two values changed.
enum ReputationType : uint32_t {
	REPUTATION_NEGATIVE = 0,
	REPUTATION_POSITIVE = 1,
};

// A reputation's DATA chunk: its largest value.
struct REPUTATION_DATA {
	// Byte-swaps the value read from a big-endian file.
	void Endian();

	float fMaximumValue;
};

// A reputation form (REPU).
class TESReputation : public TESForm, public TESFullName, public TESIcon, public BGSMessageIcon {
public:
	TESReputation();
	~TESReputation() override;

	void InitializeData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	uint16_t GetSaveSize(uint32_t aiFlags) override;
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
	void SaveGame(uint32_t aiFlags) override;
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
	void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	// The title of the reputation's current level.
	const char* GetReputationTitle();
	REPUTATION_DATA* GetData();
	const float GetPositiveRatio();
	const float GetNegativeRatio();
	const float GetReputationPct(ReputationType RepType);
	float GetReputationValue(ReputationType RepType);
	// . aeType goes to GetReputationLevel(ReputationType) as it is.
	void AddReputationExact(ReputationType aeType, float afAmount);
	const char GetReputationLevel(ReputationType RepType);
	void RemoveReputationMinorOrMajorCrime(char a2);
	const char GetReputationThreshold(const char cAxis);
	void SetReputation(int32_t a2, float a3);
	void DisplayReputationTitleChange();
	void RemoveReputation(ReputationType a1, int32_t a2);
	// aeType goes to GetReputationLevel(ReputationType) as it is.
	void RemoveReputationExact(ReputationType aeType, float afAmount);
	void AddReputation(ReputationType a1, int32_t a2);
	const char* GetReputationIcon();
	const char* GetReputationDesc();
	// The TESIcon's path, or the shared empty string when it has none.
	const char* GetReputationMainIcon();

	REPUTATION_DATA data;
	float fPositiveValue;
	float fNegativeValue;
	ReputationType rSound;
};

// The game settings TESReputation.cpp defines.
extern SettingT<GameSettingCollection> sRepNegativeLossIcon;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegThree;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegTwoIcon;
extern SettingT<GameSettingCollection> sRepChangeNegSound;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegNoneDesc;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegOneIcon;
extern SettingT<GameSettingCollection> sRepNegativeGain;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegThreeIcon;
extern SettingT<GameSettingCollection> fReputationBumpAverage;
extern SettingT<GameSettingCollection> fReputationThresholdOne;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegTwo;
extern SettingT<GameSettingCollection> sRepNegativeLoss;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegThreeIcon;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegTwoIcon;
extern SettingT<GameSettingCollection> sRepPositiveGain;
extern SettingT<GameSettingCollection> sRepNegativeGainIcon;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegThreeDesc;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegNoneIcon;
extern SettingT<GameSettingCollection> fReputationMajorCrimeNeg;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegThree;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegTwoDesc;
extern SettingT<GameSettingCollection> fReputationMinorCrimeNeg;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegOne;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegThreeIcon;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegOneIcon;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegTwo;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegTwoDesc;
extern SettingT<GameSettingCollection> fReputationBumpVeryMajor;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegTwoIcon;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegOneIcon;
extern SettingT<GameSettingCollection> sRepPositiveLoss;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegNoneDesc;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegNone;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegTwoDesc;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegNoneIcon;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegTwo;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegOneDesc;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegOne;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegNoneIcon;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegNone;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegThreeDesc;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegTwoDesc;
extern SettingT<GameSettingCollection> sRepPositiveLossIcon;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegTwo;
extern SettingT<GameSettingCollection> fReputationBumpMajor;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegOne;
extern SettingT<GameSettingCollection> fReputationThresholdTwo;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegOneDesc;
extern SettingT<GameSettingCollection> fReputationBumpVeryMinor;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegNoneDesc;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegThreeDesc;
extern SettingT<GameSettingCollection> sRepChangePosSound;
extern SettingT<GameSettingCollection> fReputationBumpMinor;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegOneDesc;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegNone;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegOneIcon;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegThree;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegTwoIcon;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegOneDesc;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegNone;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegThreeIcon;
extern SettingT<GameSettingCollection> sRepTitlePosThreeNegNoneIcon;
extern SettingT<GameSettingCollection> sRepTitlePosTwoNegThreeDesc;
extern SettingT<GameSettingCollection> sRepPositiveGainIcon;
extern SettingT<GameSettingCollection> fReputationThresholdThree;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegOne;
extern SettingT<GameSettingCollection> sRepTitlePosOneNegThree;
extern SettingT<GameSettingCollection> sRepTitlePosNoneNegNoneDesc;
inline char cFakeModalMessageMenuResult = -1;
