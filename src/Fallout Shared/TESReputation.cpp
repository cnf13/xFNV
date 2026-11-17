#include "Fallout Shared/TESReputation.h"

#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<GameSettingCollection> fReputationMinorCrimeNeg("fReputationMinorCrimeNeg", 2.0f);
SettingT<GameSettingCollection> fReputationMajorCrimeNeg("fReputationMajorCrimeNeg", 30.0f);
SettingT<GameSettingCollection> fReputationBumpVeryMinor("fReputationBumpVeryMinor", 1.0f);
SettingT<GameSettingCollection> fReputationBumpMinor("fReputationBumpMinor", 2.0f);
SettingT<GameSettingCollection> fReputationBumpAverage("fReputationBumpAverage", 4.0f);
SettingT<GameSettingCollection> fReputationBumpMajor("fReputationBumpMajor", 7.0f);
SettingT<GameSettingCollection> fReputationBumpVeryMajor("fReputationBumpVeryMajor", 12.0f);
SettingT<GameSettingCollection> fReputationThresholdOne("fReputationThresholdOne", 0.15f);
SettingT<GameSettingCollection> fReputationThresholdTwo("fReputationThresholdTwo", 0.5f);
SettingT<GameSettingCollection> fReputationThresholdThree("fReputationThresholdThree", 1.0f);
SettingT<GameSettingCollection> sRepTitlePosNoneNegNone("sRepTitlePosNoneNegNone", "Neutral");
SettingT<GameSettingCollection> sRepTitlePosOneNegOne("sRepTitlePosOneNegOne", "Mixed");
SettingT<GameSettingCollection> sRepTitlePosTwoNegTwo("sRepTitlePosTwoNegTwo", "Unpredictable");
SettingT<GameSettingCollection> sRepTitlePosThreeNegThree("sRepTitlePosThreeNegThree", "Wild Child");
SettingT<GameSettingCollection> sRepTitlePosOneNegNone("sRepTitlePosOneNegNone", "Accepted");
SettingT<GameSettingCollection> sRepTitlePosTwoNegNone("sRepTitlePosTwoNegNone", "Liked");
SettingT<GameSettingCollection> sRepTitlePosThreeNegNone("sRepTitlePosThreeNegNone", "Idolized");
SettingT<GameSettingCollection> sRepTitlePosNoneNegOne("sRepTitlePosNoneNegOne", "Shunned");
SettingT<GameSettingCollection> sRepTitlePosNoneNegTwo("sRepTitlePosNoneNegTwo", "Hated");
SettingT<GameSettingCollection> sRepTitlePosNoneNegThree("sRepTitlePosNoneNegThree", "Vilified");
SettingT<GameSettingCollection> sRepTitlePosTwoNegOne("sRepTitlePosTwoNegOne", "Smiling Troublemaker");
SettingT<GameSettingCollection> sRepTitlePosThreeNegOne("sRepTitlePosThreeNegOne", "Good Natured Rascal");
SettingT<GameSettingCollection> sRepTitlePosThreeNegTwo("sRepTitlePosThreeNegTwo", "Dark Hero");
SettingT<GameSettingCollection> sRepTitlePosOneNegTwo("sRepTitlePosOneNegTwo", "Sneering Punk");
SettingT<GameSettingCollection> sRepTitlePosOneNegThree("sRepTitlePosOneNegThree", "Merciful Thug");
SettingT<GameSettingCollection> sRepTitlePosTwoNegThree("sRepTitlePosTwoNegThree", "Soft-Hearted Devil");
SettingT<GameSettingCollection> sRepPositiveGain("sRepPositiveGain", "Reputation Gain");
SettingT<GameSettingCollection> sRepPositiveGainIcon("sRepPositiveGainIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRepNegativeGain("sRepNegativeGain", "Reputation Loss");
SettingT<GameSettingCollection> sRepNegativeGainIcon("sRepNegativeGainIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepPositiveLoss("sRepPositiveLoss", "Reputation Loss");
SettingT<GameSettingCollection> sRepPositiveLossIcon("sRepPositiveLossIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepNegativeLoss("sRepNegativeLoss", "Reputation Gain");
SettingT<GameSettingCollection> sRepNegativeLossIcon("sRepNegativeLossIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRepTitlePosNoneNegNoneIcon("sRepTitlePosNoneNegNoneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds");
SettingT<GameSettingCollection> sRepTitlePosOneNegOneIcon("sRepTitlePosOneNegOneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds");
SettingT<GameSettingCollection> sRepTitlePosTwoNegTwoIcon("sRepTitlePosTwoNegTwoIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds");
SettingT<GameSettingCollection> sRepTitlePosThreeNegThreeIcon("sRepTitlePosThreeNegThreeIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds");
SettingT<GameSettingCollection> sRepTitlePosOneNegNoneIcon("sRepTitlePosOneNegNoneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds");
SettingT<GameSettingCollection> sRepTitlePosTwoNegNoneIcon("sRepTitlePosTwoNegNoneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRepTitlePosThreeNegNoneIcon("sRepTitlePosThreeNegNoneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRepTitlePosNoneNegOneIcon("sRepTitlePosNoneNegOneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds");
SettingT<GameSettingCollection> sRepTitlePosNoneNegTwoIcon("sRepTitlePosNoneNegTwoIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepTitlePosNoneNegThreeIcon("sRepTitlePosNoneNegThreeIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepTitlePosTwoNegOneIcon("sRepTitlePosTwoNegOneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRepTitlePosThreeNegOneIcon("sRepTitlePosThreeNegOneIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
SettingT<GameSettingCollection> sRepTitlePosThreeNegTwoIcon("sRepTitlePosThreeNegTwoIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepTitlePosOneNegTwoIcon("sRepTitlePosOneNegTwoIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepTitlePosOneNegThreeIcon("sRepTitlePosOneNegThreeIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepTitlePosTwoNegThreeIcon("sRepTitlePosTwoNegThreeIcon",
	"Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds");
SettingT<GameSettingCollection> sRepTitlePosNoneNegNoneDesc("sRepTitlePosNoneNegNoneDesc",
	"People don't know enough about you to form an opinion.");
SettingT<GameSettingCollection> sRepTitlePosOneNegOneDesc("sRepTitlePosOneNegOneDesc",
	"A little bit good mixed with a little bit bad, people haven't figured you out yet.");
SettingT<GameSettingCollection> sRepTitlePosTwoNegTwoDesc("sRepTitlePosTwoNegTwoDesc",
	"No one's sure what to make of your unpredictable nature, but you've left a strong impression.");
SettingT<GameSettingCollection> sRepTitlePosThreeNegThreeDesc("sRepTitlePosThreeNegThreeDesc",
	"Your wild, seemingly capricious behavior leaves people scratching their heads in confusion and avoiding close "
	"contact.");
SettingT<GameSettingCollection> sRepTitlePosOneNegNoneDesc("sRepTitlePosOneNegNoneDesc",
	"Folks have come to accept you for your helpful nature.");
SettingT<GameSettingCollection> sRepTitlePosTwoNegNoneDesc("sRepTitlePosTwoNegNoneDesc",
	"Enough news of your good works has been passed around that people like you.");
SettingT<GameSettingCollection> sRepTitlePosThreeNegNoneDesc("sRepTitlePosThreeNegNoneDesc",
	"Renowned for your extensive support and goodwill, you are idolized by the community.");
SettingT<GameSettingCollection> sRepTitlePosNoneNegOneDesc("sRepTitlePosNoneNegOneDesc",
	"You've left a poor impression on the community and may be shunned as a result.");
SettingT<GameSettingCollection> sRepTitlePosNoneNegTwoDesc("sRepTitlePosNoneNegTwoDesc",
	"Now that folks know you're bad, most people outright hate you.");
SettingT<GameSettingCollection> sRepTitlePosNoneNegThreeDesc("sRepTitlePosNoneNegThreeDesc",
	"For your overwhelmingly monstrous behavior, you have become vilified by the community.");
SettingT<GameSettingCollection> sRepTitlePosTwoNegOneDesc("sRepTitlePosTwoNegOneDesc",
	"People know you're good at heart even though you're occasionally a troublemaker.");
SettingT<GameSettingCollection> sRepTitlePosThreeNegOneDesc("sRepTitlePosThreeNegOneDesc",
	"Your reputation as a good-natured friend of the community manages to outshine your dark side.");
SettingT<GameSettingCollection> sRepTitlePosThreeNegTwoDesc("sRepTitlePosThreeNegTwoDesc",
	"Folks still think you're some kind of hero, but you sure can be nasty sometimes.");
SettingT<GameSettingCollection> sRepTitlePosOneNegTwoDesc("sRepTitlePosOneNegTwoDesc",
	"Even though you've done some good for the community, people still think you're a punk.");
SettingT<GameSettingCollection> sRepTitlePosOneNegThreeDesc("sRepTitlePosOneNegThreeDesc",
	"Despite your reputation as a thug, you are known to occasionally show a charitable side.");
SettingT<GameSettingCollection> sRepTitlePosTwoNegThreeDesc("sRepTitlePosTwoNegThreeDesc",
	"Most people say you're the devil himself, but most admit you've also done a world of good.");
SettingT<GameSettingCollection> sRepChangePosSound("sRepChangePosSound", "UIRepGood");
SettingT<GameSettingCollection> sRepChangeNegSound("sRepChangeNegSound", "UIRepBad");

void FakeModalMessageMenuCallback() {
	cFakeModalMessageMenuResult = Interface::GetMessageMenuResult();
}

void REPUTATION_DATA::Endian() {
	ENDIAN(fMaximumValue, false);
}

TESReputation::TESReputation() {
	TESReputation::InitializeData();
	SetFormType(REPU_ID);
}

TESReputation::~TESReputation() {}

void TESReputation::InitializeData() {
	TESReputation* result = this;
	fPositiveValue = (float)0.0;
	fNegativeValue = (float)0.0;
}

// REPU. The exe does not check the record type; it reads the name, icon and message icon components and the DATA
// float, which the engine's shared Endian swaps for a big-endian file.
bool TESReputation::Load(TESFile* apFile) {
	LoadForm(apFile);
	SetInitialized(false);
	for (;;) {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		switch (uiChunk) {
		case EDID_ID: {
			// The exe's _alloca (__alloca_probe_16, 0xEC5EC0): a 16-byte aligned buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case FULL_ID:
			TESFullName::LoadFullNameChunk(this, apFile);
			break;
		case ICON_ID:
			TESTexture::LoadTextureChunk(this, apFile);
			break;
		case MICO_ID:
			BGSMessageIcon::LoadMessageIcon(this, apFile);
			break;
		case DATA_ID:
			// The DATA chunk is a raw 4-byte buffer.
			LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), 4);
			if (apFile->QEndian())
				data.Endian();
			break;
		default:
			break;
		}
		if (!apFile->NextChunk()) {
			break;
		}
	}
	return true;
}

void TESReputation::Save() {
	TESForm::StartForm();
	static_cast<TESFullName*>(this)->Save();
	static_cast<TESIcon*>(this)->Save(ICON_ID);
	static_cast<BGSMessageIcon*>(this)->Save(MICO_ID);
	if (TESForm::QEndianSwapOnSave()) {
		(&data)->Endian();
	}
	TESForm::__SaveData(&data, 4u);
	if (TESForm::QEndianSwapOnSave()) {
		(&data)->Endian();
	}
	TESForm::CloseForm();
}

uint16_t TESReputation::GetSaveSize(uint32_t aiFlags) {
	return (uint16_t)(TESForm::GetSaveSize(aiFlags) + 8);
}

void TESReputation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPositiveValue), 4, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fNegativeValue), 4, 0);
}

void TESReputation::SaveGame(uint32_t aiFlags) {
	TESForm::SaveGame(aiFlags);
}

void TESReputation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPositiveValue), 4);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fNegativeValue), 4);
}

void TESReputation::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESReputation::Copy(TESForm* apCopy) {
	TESReputation* v3 = dynamic_cast<TESReputation*>(apCopy);
	if (v3) {
		fNegativeValue = v3->fNegativeValue;
		fPositiveValue = v3->fPositiveValue;
		ourMemcpy(&data.fMaximumValue, &v3->data, sizeof(data.fMaximumValue));
		TESForm::CopyAllComponents(apCopy);
	}
}

bool TESReputation::Compare(TESForm* apCompare) {
	TESReputation* preputation =
		static_cast<TESReputation*>(FNVD_DYNAMIC_CAST_VOID(TESReputation)(apCompare, RTTI_TESForm, RTTI_TESReputation));
	if (!preputation) {
		return 1;
	}
	return std::memcmp(&data, preputation->GetData(), sizeof(data)) || CompareAllComponents(preputation);
}

const char* TESReputation::GetReputationTitle() {
	SettingT<GameSettingCollection>* pRepTitles[4][4];
	pRepTitles[0][0] = &sRepTitlePosNoneNegNone;
	pRepTitles[0][1] = &sRepTitlePosOneNegNone;
	pRepTitles[0][2] = &sRepTitlePosTwoNegNone;
	pRepTitles[0][3] = &sRepTitlePosThreeNegNone;
	pRepTitles[1][0] = &sRepTitlePosNoneNegOne;
	pRepTitles[1][1] = &sRepTitlePosOneNegOne;
	pRepTitles[1][2] = &sRepTitlePosTwoNegOne;
	pRepTitles[1][3] = &sRepTitlePosThreeNegOne;
	pRepTitles[2][0] = &sRepTitlePosNoneNegTwo;
	pRepTitles[2][1] = &sRepTitlePosOneNegTwo;
	pRepTitles[2][2] = &sRepTitlePosTwoNegTwo;
	pRepTitles[2][3] = &sRepTitlePosThreeNegTwo;
	pRepTitles[3][0] = &sRepTitlePosNoneNegThree;
	pRepTitles[3][1] = &sRepTitlePosOneNegThree;
	pRepTitles[3][2] = &sRepTitlePosTwoNegThree;
	pRepTitles[3][3] = &sRepTitlePosThreeNegThree;
	char cPosIndex = GetReputationLevel(REPUTATION_POSITIVE);
	char cNegIndex = GetReputationLevel(REPUTATION_NEGATIVE);
	return pRepTitles[cNegIndex][cPosIndex]->String();
}

REPUTATION_DATA* TESReputation::GetData() {
	return &data;
}

const float TESReputation::GetPositiveRatio() {
	return (float)(fPositiveValue / data.fMaximumValue);
}

const float TESReputation::GetNegativeRatio() {
	return (float)(fNegativeValue / data.fMaximumValue);
}

const float TESReputation::GetReputationPct(ReputationType RepType) {
	if (((int32_t)RepType) == 1) {
		return GetPositiveRatio();
	} else {
		return GetNegativeRatio();
	}
}

float TESReputation::GetReputationValue(ReputationType RepType) {
	if (((int32_t)RepType) == 1) {
		return fPositiveValue;
	} else {
		return fNegativeValue;
	}
}

void TESReputation::AddReputationExact(ReputationType aeType, float afAmount) {
	char msg[256];
	char v12 = 0;
	char ReputationLevel = GetReputationLevel(aeType);
	if (aeType == REPUTATION_POSITIVE) {
		float fPositiveValue;
		this->fPositiveValue = this->fPositiveValue + afAmount;
		if (!(data.fMaximumValue < static_cast<double>(this->fPositiveValue))) {
			fPositiveValue = this->fPositiveValue;
		} else {
			fPositiveValue = data.fMaximumValue;
		}
		this->fPositiveValue = fPositiveValue;
		const char* v3 = sRepPositiveGain.String();
		BSsprintf(msg, 250, "%s\n%s", GetFullName(), v3);
		if (GetReputationLevel(REPUTATION_POSITIVE) != ReputationLevel) {
			rSound = REPUTATION_POSITIVE;
			v12 = 1;
		}
		const char* v4 = sRepPositiveGainIcon.String();
		Interface::ShowMessage(msg, 0, v4, 0, 2.0f, 0);
	} else {
		float fNegativeValue;
		this->fNegativeValue = this->fNegativeValue + afAmount;
		if (!(data.fMaximumValue < static_cast<double>(this->fNegativeValue))) {
			fNegativeValue = this->fNegativeValue;
		} else {
			fNegativeValue = data.fMaximumValue;
		}
		this->fNegativeValue = fNegativeValue;
		const char* v5 = sRepNegativeGain.String();
		BSsprintf(msg, 250, "%s\n%s", GetFullName(), v5);
		if (GetReputationLevel(REPUTATION_NEGATIVE) != ReputationLevel) {
			v12 = 1;
			rSound = REPUTATION_NEGATIVE;
		}
		const char* v6 = sRepNegativeGainIcon.String();
		Interface::ShowMessage(msg, 0, v6, 0, 2.0f, 0);
	}
	if (!v12 || (pPlayer->IsPlayerCharacterInCombat(0) || Interface::InDialog())) {
		if (v12) {
			pPlayer->SetLastModifiedFactionReputation(this);
		}
	} else {
		DisplayReputationTitleChange();
	}
	AddChange(2);
}

const char TESReputation::GetReputationLevel(ReputationType RepType) {
	float positiveRepRatio;
	int8_t v3 = 0;
	if (((int32_t)RepType) == 1) {
		positiveRepRatio = GetPositiveRatio();
	} else {
		positiveRepRatio = GetNegativeRatio();
	}
	if ((((&::fReputationThresholdOne)->Float()) <= ((double)positiveRepRatio)) &&
		(((&::fReputationThresholdTwo)->Float()) > ((double)positiveRepRatio))) {
		return 1;
	}
	if ((((&::fReputationThresholdTwo)->Float()) <= ((double)positiveRepRatio)) &&
		(((&::fReputationThresholdThree)->Float()) > ((double)positiveRepRatio))) {
		return 2;
	}
	if (((&::fReputationThresholdThree)->Float()) <= ((double)positiveRepRatio)) {
		return 3;
	}
	return v3;
}

void TESReputation::RemoveReputationMinorOrMajorCrime(char a2) {
	float a3;
	if (a2) {
		a3 = (&::fReputationMajorCrimeNeg)->Float();
	} else {
		a3 = (&::fReputationMinorCrimeNeg)->Float();
	}
	AddReputationExact(REPUTATION_NEGATIVE, a3);
}

const char TESReputation::GetReputationThreshold(const char cAxis) {
	char ReputationLevel = GetReputationLevel(REPUTATION_POSITIVE);
	char v5 = GetReputationLevel(REPUTATION_NEGATIVE);
	if (!ReputationLevel && !v5) {
		return 1;
	}
	if (cAxis) {
		if (cAxis == 1) {
			if (ReputationLevel == 1 && !v5) {
				return 4;
			}
			if (ReputationLevel == 2) {
				if (!v5) {
					return 5;
				}
				if (v5 == 1) {
					return 2;
				}
			}
			if (ReputationLevel == 3) {
				if (!v5) {
					return 6;
				}
				if (v5 == 1) {
					return 3;
				}
			}
		} else {
			if (cAxis == 2) {
				if (v5 == 1 && !ReputationLevel) {
					return 4;
				}
				if (v5 == 2) {
					if (!ReputationLevel) {
						return 5;
					}
					if (ReputationLevel == 1) {
						return 2;
					}
				}
				if (v5 == 3) {
					if (!ReputationLevel) {
						return 6;
					}
					if (ReputationLevel == 1) {
						return 3;
					}
				}
			}
		}
	} else {
		if (ReputationLevel == 1 && v5 == 1) {
			return 3;
		}
		if (ReputationLevel == 3) {
			if (v5 == 2) {
				return 2;
			}
			if (v5 == 3) {
				return 5;
			}
		}
		if (ReputationLevel == 2) {
			if (v5 == 3) {
				return 2;
			}
			if (v5 == 2) {
				return 4;
			}
		}
	}
	return 0;
}

void TESReputation::SetReputation(int32_t a2, float a3) {
	if (a2 == 1) {
		fPositiveValue = a3;
	} else {
		fNegativeValue = a3;
	}
	AddChange((int32_t)2u);
}

void TESReputation::DisplayReputationTitleChange() {
	const char* v2;
	const char* v3;
	char apDest[516];
	const char* ReputationDesc = GetReputationDesc();
	BSsprintf(apDest, 512, "%s\n%s", GetReputationTitle(), ReputationDesc);
	InterfaceManager* v1 = InterfaceManager::Instance();
	v1->xTutorialManager.ShowMessage(Interface::TUT_REPUTATION, 0, 500u);
	if (rSound == REPUTATION_POSITIVE) {
		v3 = sOk.String();
		v2 = sRepChangePosSound.String();
	} else {
		v3 = sOk.String();
		v2 = sRepChangeNegSound.String();
	}
	Interface::CreateMessageMenuSound(apDest, GetReputationIcon(), GetFullName(), v2, &FakeModalMessageMenuCallback, 1,
		23, 0.0f, 0.0f, v3, nullptr);
}

// The exe indexes the bump table from one entry below its first; a2 runs 1 to 5.
void TESReputation::RemoveReputation(ReputationType a1, int32_t a2) {
	char v19 = 0;
	char ReputationLevel = 0;
	char v17 = 0;
	if (a2 > 0 && a2 < 6) {
		char msg[256];
		SettingT<GameSettingCollection>* pBumps[5];
		pBumps[0] = &fReputationBumpVeryMinor;
		pBumps[1] = &fReputationBumpMinor;
		pBumps[2] = &fReputationBumpAverage;
		pBumps[3] = &fReputationBumpMajor;
		pBumps[4] = &fReputationBumpVeryMajor;
		ReputationLevel = GetReputationLevel(a1);
		if (a1 == REPUTATION_POSITIVE) {
			float fPositiveValue;
			this->fPositiveValue = this->fPositiveValue - pBumps[a2 - 1]->Float();
			if (!(this->fPositiveValue < 0.0)) {
				fPositiveValue = this->fPositiveValue;
			} else {
				fPositiveValue = 0.0f;
			}
			this->fPositiveValue = fPositiveValue;
			const char* v3 = sRepPositiveLoss.String();
			BSsprintf(msg, 250, "%s\n%s", GetFullName(), v3);
			v17 = GetReputationLevel(REPUTATION_POSITIVE);
			if (v17 != ReputationLevel) {
				rSound = REPUTATION_NEGATIVE;
				v19 = 1;
			}
			const char* v4 = sRepPositiveLossIcon.String();
			Interface::ShowMessage(msg, 0, v4, 0, 2.0f, 0);
		} else {
			float fNegativeValue;
			this->fNegativeValue = this->fNegativeValue - pBumps[a2 - 1]->Float();
			if (!(this->fNegativeValue < 0.0)) {
				fNegativeValue = this->fNegativeValue;
			} else {
				fNegativeValue = 0.0f;
			}
			this->fNegativeValue = fNegativeValue;
			const char* v5 = sRepNegativeLoss.String();
			BSsprintf(msg, 250, "%s\n%s", GetFullName(), v5);
			v17 = GetReputationLevel(REPUTATION_NEGATIVE);
			if (v17 != ReputationLevel) {
				rSound = REPUTATION_POSITIVE;
				v19 = 1;
			}
			// The exe shows the gain icon here (sRepNegativeGainIcon), not the loss icon.
			const char* v6 = sRepNegativeGainIcon.String();
			Interface::ShowMessage(msg, 0, v6, 0, 2.0f, 0);
		}
		if (!v19 || (pPlayer->IsPlayerCharacterInCombat(0) || Interface::InDialog())) {
			if (v19) {
				pPlayer->SetLastModifiedFactionReputation(this);
			}
		} else {
			DisplayReputationTitleChange();
		}
		AddChange(2);
	}
}

void TESReputation::RemoveReputationExact(ReputationType aeType, float afAmount) {
	char msg[256];
	char v12 = 0;
	char ReputationLevel = GetReputationLevel(aeType);
	if (aeType == REPUTATION_POSITIVE) {
		float fPositiveValue;
		this->fPositiveValue = this->fPositiveValue - afAmount;
		if (!(this->fPositiveValue < 0.0)) {
			fPositiveValue = this->fPositiveValue;
		} else {
			fPositiveValue = 0.0f;
		}
		this->fPositiveValue = fPositiveValue;
		const char* v3 = sRepPositiveLoss.String();
		BSsprintf(msg, 250, "%s\n%s", GetFullName(), v3);
		if (GetReputationLevel(REPUTATION_POSITIVE) != ReputationLevel) {
			rSound = REPUTATION_NEGATIVE;
			v12 = 1;
		}
		const char* v4 = sRepPositiveLossIcon.String();
		Interface::ShowMessage(msg, 0, v4, 0, 2.0f, 0);
	} else {
		float fNegativeValue;
		this->fNegativeValue = this->fNegativeValue - afAmount;
		if (!(this->fNegativeValue < 0.0)) {
			fNegativeValue = this->fNegativeValue;
		} else {
			fNegativeValue = 0.0f;
		}
		this->fNegativeValue = fNegativeValue;
		const char* v5 = sRepNegativeLoss.String();
		BSsprintf(msg, 250, "%s\n%s", GetFullName(), v5);
		if (GetReputationLevel(REPUTATION_NEGATIVE) != ReputationLevel) {
			rSound = REPUTATION_POSITIVE;
			v12 = 1;
		}
		const char* v6 = sRepNegativeLossIcon.String();
		Interface::ShowMessage(msg, 0, v6, 0, 2.0f, 0);
	}
	if (!v12 || (pPlayer->IsPlayerCharacterInCombat(0) || Interface::InDialog())) {
		if (v12) {
			pPlayer->SetLastModifiedFactionReputation(this);
		}
	} else {
		DisplayReputationTitleChange();
	}
	AddChange(2);
}

// The exe indexes the bump table from one entry below its first; a2 runs 1 to 5.
void TESReputation::AddReputation(ReputationType a1, int32_t a2) {
	if (a2 > 0 && a2 < 6) {
		char msg[256];
		char v19 = 0;
		char v17 = 0;
		SettingT<GameSettingCollection>* pBumps[5];
		pBumps[0] = &fReputationBumpVeryMinor;
		pBumps[1] = &fReputationBumpMinor;
		pBumps[2] = &fReputationBumpAverage;
		pBumps[3] = &fReputationBumpMajor;
		pBumps[4] = &fReputationBumpVeryMajor;
		char ReputationLevel = GetReputationLevel(a1);
		if (a1 == REPUTATION_POSITIVE) {
			float fPositiveValue;
			this->fPositiveValue = this->fPositiveValue + pBumps[a2 - 1]->Float();
			if (!(data.fMaximumValue < static_cast<double>(this->fPositiveValue))) {
				fPositiveValue = this->fPositiveValue;
			} else {
				fPositiveValue = data.fMaximumValue;
			}
			this->fPositiveValue = fPositiveValue;
			const char* v3 = sRepPositiveGain.String();
			BSsprintf(msg, 250, "%s\n%s", GetFullName(), v3);
			v17 = GetReputationLevel(REPUTATION_POSITIVE);
			if (v17 != ReputationLevel) {
				rSound = REPUTATION_POSITIVE;
				v19 = 1;
			}
			const char* v4 = sRepPositiveGainIcon.String();
			Interface::ShowMessage(msg, 0, v4, 0, 2.0f, 0);
		} else {
			float fNegativeValue;
			this->fNegativeValue = this->fNegativeValue + pBumps[a2 - 1]->Float();
			if (!(data.fMaximumValue < static_cast<double>(this->fNegativeValue))) {
				fNegativeValue = this->fNegativeValue;
			} else {
				fNegativeValue = data.fMaximumValue;
			}
			this->fNegativeValue = fNegativeValue;
			const char* v5 = sRepNegativeGain.String();
			BSsprintf(msg, 250, "%s\n%s", GetFullName(), v5);
			v17 = GetReputationLevel(REPUTATION_NEGATIVE);
			if (v17 != ReputationLevel) {
				rSound = REPUTATION_NEGATIVE;
				v19 = 1;
			}
			const char* v6 = sRepNegativeGainIcon.String();
			Interface::ShowMessage(msg, 0, v6, 0, 2.0f, 0);
		}
		if (!v19 || (pPlayer->IsPlayerCharacterInCombat(0) || Interface::InDialog())) {
			if (v19) {
				pPlayer->SetLastModifiedFactionReputation(this);
			}
		} else {
			DisplayReputationTitleChange();
		}
		AddChange(2);
	}
}

const char* TESReputation::GetReputationIcon() {
	SettingT<GameSettingCollection>* pRepIcons[4][4];
	pRepIcons[0][0] = &sRepTitlePosNoneNegNoneIcon;
	pRepIcons[0][1] = &sRepTitlePosOneNegNoneIcon;
	pRepIcons[0][2] = &sRepTitlePosTwoNegNoneIcon;
	pRepIcons[0][3] = &sRepTitlePosThreeNegNoneIcon;
	pRepIcons[1][0] = &sRepTitlePosNoneNegOneIcon;
	pRepIcons[1][1] = &sRepTitlePosOneNegOneIcon;
	pRepIcons[1][2] = &sRepTitlePosTwoNegOneIcon;
	pRepIcons[1][3] = &sRepTitlePosThreeNegOneIcon;
	pRepIcons[2][0] = &sRepTitlePosNoneNegTwoIcon;
	pRepIcons[2][1] = &sRepTitlePosOneNegTwoIcon;
	pRepIcons[2][2] = &sRepTitlePosTwoNegTwoIcon;
	pRepIcons[2][3] = &sRepTitlePosThreeNegTwoIcon;
	pRepIcons[3][0] = &sRepTitlePosNoneNegThreeIcon;
	pRepIcons[3][1] = &sRepTitlePosOneNegThreeIcon;
	pRepIcons[3][2] = &sRepTitlePosTwoNegThreeIcon;
	pRepIcons[3][3] = &sRepTitlePosThreeNegThreeIcon;
	char cPosIndex = GetReputationLevel(REPUTATION_POSITIVE);
	char cNegIndex = GetReputationLevel(REPUTATION_NEGATIVE);
	return pRepIcons[cNegIndex][cPosIndex]->String();
}

const char* TESReputation::GetReputationDesc() {
	Setting* v4[16];
	v4[0] = &sRepTitlePosNoneNegNoneDesc;
	v4[1] = &sRepTitlePosOneNegNoneDesc;
	v4[2] = &sRepTitlePosTwoNegNoneDesc;
	v4[3] = &sRepTitlePosThreeNegNoneDesc;
	v4[4] = &sRepTitlePosNoneNegOneDesc;
	v4[5] = &sRepTitlePosOneNegOneDesc;
	v4[6] = &sRepTitlePosTwoNegOneDesc;
	v4[7] = &sRepTitlePosThreeNegOneDesc;
	v4[8] = &sRepTitlePosNoneNegTwoDesc;
	v4[9] = &sRepTitlePosOneNegTwoDesc;
	v4[10] = &sRepTitlePosTwoNegTwoDesc;
	v4[11] = &sRepTitlePosThreeNegTwoDesc;
	v4[12] = &sRepTitlePosNoneNegThreeDesc;
	v4[13] = &sRepTitlePosOneNegThreeDesc;
	v4[14] = &sRepTitlePosTwoNegThreeDesc;
	v4[15] = &sRepTitlePosThreeNegThreeDesc;
	uint8_t ReputationLevel = GetReputationLevel(REPUTATION_POSITIVE);
	uint8_t v5 = GetReputationLevel(REPUTATION_NEGATIVE);
	return (*(&v4[4 * static_cast<char>(v5)] + static_cast<char>(ReputationLevel)))->String();
}

const char* TESReputation::GetReputationMainIcon() {
	if (GetTextureNameLength()) {
		return GetTextureName();
	} else {
		return "";
	}
}
