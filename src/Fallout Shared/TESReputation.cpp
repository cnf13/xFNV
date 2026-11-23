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
#include "xfnv/Compiler.h"

// The game settings this file defines, with their IDs and values, in initialization order.
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
SettingT<GameSettingCollection> sRepPositiveGainIcon(
    "sRepPositiveGainIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds"
);
SettingT<GameSettingCollection> sRepNegativeGain("sRepNegativeGain", "Reputation Loss");
SettingT<GameSettingCollection>
    sRepNegativeGainIcon("sRepNegativeGainIcon", "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepPositiveLoss("sRepPositiveLoss", "Reputation Loss");
SettingT<GameSettingCollection>
    sRepPositiveLossIcon("sRepPositiveLossIcon", "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
SettingT<GameSettingCollection> sRepNegativeLoss("sRepNegativeLoss", "Reputation Gain");
SettingT<GameSettingCollection> sRepNegativeLossIcon(
    "sRepNegativeLossIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds"
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegNoneIcon(
    "sRepTitlePosNoneNegNoneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds"
);
SettingT<GameSettingCollection> sRepTitlePosOneNegOneIcon(
    "sRepTitlePosOneNegOneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds"
);
SettingT<GameSettingCollection> sRepTitlePosTwoNegTwoIcon(
    "sRepTitlePosTwoNegTwoIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds"
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegThreeIcon(
    "sRepTitlePosThreeNegThreeIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds"
);
SettingT<GameSettingCollection> sRepTitlePosOneNegNoneIcon(
    "sRepTitlePosOneNegNoneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds"
);
SettingT<GameSettingCollection> sRepTitlePosTwoNegNoneIcon(
    "sRepTitlePosTwoNegNoneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds"
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegNoneIcon(
    "sRepTitlePosThreeNegNoneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds"
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegOneIcon(
    "sRepTitlePosNoneNegOneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds"
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegTwoIcon(
    "sRepTitlePosNoneNegTwoIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds"
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegThreeIcon(
    "sRepTitlePosNoneNegThreeIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds"
);
SettingT<GameSettingCollection> sRepTitlePosTwoNegOneIcon(
    "sRepTitlePosTwoNegOneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds"
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegOneIcon(
    "sRepTitlePosThreeNegOneIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds"
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegTwoIcon(
    "sRepTitlePosThreeNegTwoIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds"
);
SettingT<GameSettingCollection> sRepTitlePosOneNegTwoIcon(
    "sRepTitlePosOneNegTwoIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds"
);
SettingT<GameSettingCollection> sRepTitlePosOneNegThreeIcon(
    "sRepTitlePosOneNegThreeIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds"
);
SettingT<GameSettingCollection> sRepTitlePosTwoNegThreeIcon(
    "sRepTitlePosTwoNegThreeIcon",
    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds"
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegNoneDesc(
    "sRepTitlePosNoneNegNoneDesc",
    "People don't know enough about you to form an opinion."
);
SettingT<GameSettingCollection> sRepTitlePosOneNegOneDesc(
    "sRepTitlePosOneNegOneDesc",
    "A little bit good mixed with a little bit bad, people haven't figured you out yet."
);
SettingT<GameSettingCollection> sRepTitlePosTwoNegTwoDesc(
    "sRepTitlePosTwoNegTwoDesc",
    "No one's sure what to make of your unpredictable nature, but you've left a strong impression."
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegThreeDesc(
    "sRepTitlePosThreeNegThreeDesc",
    "Your wild, seemingly capricious behavior leaves people scratching their heads in confusion and avoiding close "
    "contact."
);
SettingT<GameSettingCollection>
    sRepTitlePosOneNegNoneDesc("sRepTitlePosOneNegNoneDesc", "Folks have come to accept you for your helpful nature.");
SettingT<GameSettingCollection> sRepTitlePosTwoNegNoneDesc(
    "sRepTitlePosTwoNegNoneDesc",
    "Enough news of your good works has been passed around that people like you."
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegNoneDesc(
    "sRepTitlePosThreeNegNoneDesc",
    "Renowned for your extensive support and goodwill, you are idolized by the community."
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegOneDesc(
    "sRepTitlePosNoneNegOneDesc",
    "You've left a poor impression on the community and may be shunned as a result."
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegTwoDesc(
    "sRepTitlePosNoneNegTwoDesc",
    "Now that folks know you're bad, most people outright hate you."
);
SettingT<GameSettingCollection> sRepTitlePosNoneNegThreeDesc(
    "sRepTitlePosNoneNegThreeDesc",
    "For your overwhelmingly monstrous behavior, you have become vilified by the community."
);
SettingT<GameSettingCollection> sRepTitlePosTwoNegOneDesc(
    "sRepTitlePosTwoNegOneDesc",
    "People know you're good at heart even though you're occasionally a troublemaker."
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegOneDesc(
    "sRepTitlePosThreeNegOneDesc",
    "Your reputation as a good-natured friend of the community manages to outshine your dark side."
);
SettingT<GameSettingCollection> sRepTitlePosThreeNegTwoDesc(
    "sRepTitlePosThreeNegTwoDesc",
    "Folks still think you're some kind of hero, but you sure can be nasty sometimes."
);
SettingT<GameSettingCollection> sRepTitlePosOneNegTwoDesc(
    "sRepTitlePosOneNegTwoDesc",
    "Even though you've done some good for the community, people still think you're a punk."
);
SettingT<GameSettingCollection> sRepTitlePosOneNegThreeDesc(
    "sRepTitlePosOneNegThreeDesc",
    "Despite your reputation as a thug, you are known to occasionally show a charitable side."
);
SettingT<GameSettingCollection> sRepTitlePosTwoNegThreeDesc(
    "sRepTitlePosTwoNegThreeDesc",
    "Most people say you're the devil himself, but most admit you've also done a world of good."
);
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
    fPositiveValue = 0.0f;
    fNegativeValue = 0.0f;
}

// REPU. Does not check the record type; it reads the name, icon and message icon components and the DATA float, which
// the engine's shared Endian swaps for a big-endian file.
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
                // A 16-byte aligned buffer of the chunk's size.
                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                apFile->GetChunkData(pBuffer, 0x200);
                SetFormEditorID(pBuffer);
                break;
            }
            case OBND_ID: LoadObjectBound(apFile); break;
            case FULL_ID: TESFullName::LoadFullNameChunk(this, apFile); break;
            case ICON_ID: TESTexture::LoadTextureChunk(this, apFile); break;
            case MICO_ID: BGSMessageIcon::LoadMessageIcon(this, apFile); break;
            case DATA_ID:
                // The DATA chunk is a raw 4-byte buffer.
                LoadData(apFile, static_cast<char*>(static_cast<void*>(&data)), sizeof(data));
                if (apFile->QEndian()) {
                    data.Endian();
                }
                break;
            default: break;
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
        data.Endian();
    }
    TESForm::__SaveData(&data, sizeof(data));
    if (TESForm::QEndianSwapOnSave()) {
        data.Endian();
    }
    TESForm::CloseForm();
}

uint16_t TESReputation::GetSaveSize(uint32_t aiFlags) {
    return static_cast<uint16_t>(TESForm::GetSaveSize(aiFlags) + 8);
}

void TESReputation::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESForm::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fPositiveValue), sizeof(fPositiveValue), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fNegativeValue), sizeof(fNegativeValue), 0);
}

void TESReputation::SaveGame(uint32_t aiFlags) {
    TESForm::SaveGame(aiFlags);
}

void TESReputation::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fPositiveValue), sizeof(fPositiveValue));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&fNegativeValue), sizeof(fNegativeValue));
}

void TESReputation::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    TESForm::LoadGame(aiFlags, aiCurrentFlags);
}

void TESReputation::Copy(TESForm* apCopy) {
    TESReputation* pReputation = dynamic_cast<TESReputation*>(apCopy);
    if (pReputation) {
        fNegativeValue = pReputation->fNegativeValue;
        fPositiveValue = pReputation->fPositiveValue;
        ourMemcpy(&data.fMaximumValue, &pReputation->data, sizeof(data.fMaximumValue));
        TESForm::CopyAllComponents(apCopy);
    }
}

bool TESReputation::Compare(TESForm* apCompare) {
    TESReputation* preputation = dynamic_cast<TESReputation*>(apCompare);
    if (!preputation) {
        return true;
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
    return fPositiveValue / data.fMaximumValue;
}

const float TESReputation::GetNegativeRatio() {
    return fNegativeValue / data.fMaximumValue;
}

const float TESReputation::GetReputationPct(ReputationType RepType) {
    if (RepType == REPUTATION_POSITIVE) {
        return GetPositiveRatio();
    }

    return GetNegativeRatio();
}

float TESReputation::GetReputationValue(ReputationType RepType) {
    if (RepType == REPUTATION_POSITIVE) {
        return fPositiveValue;
    }

    return fNegativeValue;
}

void TESReputation::AddReputationExact(ReputationType aeType, float afAmount) {
    char msg[256];
    char bLevelChanged = 0;
    char cOldLevel = GetReputationLevel(aeType);
    if (aeType == REPUTATION_POSITIVE) {
        fPositiveValue = fPositiveValue + afAmount;
        fPositiveValue = data.fMaximumValue < static_cast<double>(fPositiveValue) ? data.fMaximumValue : fPositiveValue;
        const char* pText = sRepPositiveGain.String();
        BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
        if (GetReputationLevel(REPUTATION_POSITIVE) != cOldLevel) {
            rSound = REPUTATION_POSITIVE;
            bLevelChanged = 1;
        }
        const char* pIcon = sRepPositiveGainIcon.String();
        Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
    }
    else {
        fNegativeValue = fNegativeValue + afAmount;
        fNegativeValue = data.fMaximumValue < static_cast<double>(fNegativeValue) ? data.fMaximumValue : fNegativeValue;
        const char* pText = sRepNegativeGain.String();
        BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
        if (GetReputationLevel(REPUTATION_NEGATIVE) != cOldLevel) {
            bLevelChanged = 1;
            rSound = REPUTATION_NEGATIVE;
        }
        const char* pIcon = sRepNegativeGainIcon.String();
        Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
    }
    if (bLevelChanged) {
        if (pPlayer->IsPlayerCharacterInCombat(nullptr) || Interface::InDialog()) {
            pPlayer->SetLastModifiedFactionReputation(this);
        }
        else {
            DisplayReputationTitleChange();
        }
    }
    AddChange(CHANGE_BASE_OBJECT_VALUE);
}

const char TESReputation::GetReputationLevel(ReputationType RepType) {
    float fPositiveRepRatio = RepType == REPUTATION_POSITIVE ? GetPositiveRatio() : GetNegativeRatio();
    if (fReputationThresholdOne.Float() <= static_cast<double>(fPositiveRepRatio) &&
        fReputationThresholdTwo.Float() > static_cast<double>(fPositiveRepRatio)) {
        return 1;
    }
    if (fReputationThresholdTwo.Float() <= static_cast<double>(fPositiveRepRatio) &&
        fReputationThresholdThree.Float() > static_cast<double>(fPositiveRepRatio)) {
        return 2;
    }
    if (fReputationThresholdThree.Float() <= static_cast<double>(fPositiveRepRatio)) {
        return 3;
    }
    return 0;
}

void TESReputation::RemoveReputationMinorOrMajorCrime(char acMajorCrime) {
    float fAmount = acMajorCrime ? fReputationMajorCrimeNeg.Float() : fReputationMinorCrimeNeg.Float();
    AddReputationExact(REPUTATION_NEGATIVE, fAmount);
}

const char TESReputation::GetReputationThreshold(const char cAxis) {
    char cPosLevel = GetReputationLevel(REPUTATION_POSITIVE);
    char cNegLevel = GetReputationLevel(REPUTATION_NEGATIVE);
    if (!cPosLevel && !cNegLevel) {
        return 1;
    }
    if (cAxis) {
        if (cAxis == 1) {
            if (cPosLevel == 1 && !cNegLevel) {
                return 4;
            }
            if (cPosLevel == 2) {
                if (!cNegLevel) {
                    return 5;
                }
                if (cNegLevel == 1) {
                    return 2;
                }
            }
            if (cPosLevel == 3) {
                if (!cNegLevel) {
                    return 6;
                }
                if (cNegLevel == 1) {
                    return 3;
                }
            }
        }
        else if (cAxis == 2) {
            if (cNegLevel == 1 && !cPosLevel) {
                return 4;
            }
            if (cNegLevel == 2) {
                if (!cPosLevel) {
                    return 5;
                }
                if (cPosLevel == 1) {
                    return 2;
                }
            }
            if (cNegLevel == 3) {
                if (!cPosLevel) {
                    return 6;
                }
                if (cPosLevel == 1) {
                    return 3;
                }
            }
        }
    }
    else {
        if (cPosLevel == 1 && cNegLevel == 1) {
            return 3;
        }
        if (cPosLevel == 3) {
            if (cNegLevel == 2) {
                return 2;
            }
            if (cNegLevel == 3) {
                return 5;
            }
        }
        if (cPosLevel == 2) {
            if (cNegLevel == 3) {
                return 2;
            }
            if (cNegLevel == 2) {
                return 4;
            }
        }
    }
    return 0;
}

void TESReputation::SetReputation(int32_t aiType, float afValue) {
    if (aiType == 1) {
        fPositiveValue = afValue;
    }
    else {
        fNegativeValue = afValue;
    }
    AddChange(CHANGE_BASE_OBJECT_VALUE);
}

void TESReputation::DisplayReputationTitleChange() {
    const char* pSound;
    const char* pButton;
    char apDest[516];
    const char* pDesc = GetReputationDesc();
    BSsprintf(apDest, 512, "%s\n%s", GetReputationTitle(), pDesc);
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_REPUTATION, 0, 500u);
    if (rSound == REPUTATION_POSITIVE) {
        pButton = sOk.String();
        pSound = sRepChangePosSound.String();
    }
    else {
        pButton = sOk.String();
        pSound = sRepChangeNegSound.String();
    }
    Interface::CreateMessageMenuSound(
        apDest,
        GetReputationIcon(),
        GetFullName(),
        pSound,
        &FakeModalMessageMenuCallback,
        1,
        23,
        0.0f,
        0.0f,
        pButton,
        nullptr
    );
}

// The bump table is indexed from one entry below its first; aiBump runs 1 to 5.
void TESReputation::RemoveReputation(ReputationType aeType, int32_t aiBump) {
    char bLevelChanged = 0;
    char cOldLevel = 0;
    char cNewLevel = 0;
    if (aiBump > 0 && aiBump < 6) {
        char msg[256];
        SettingT<GameSettingCollection>* pBumps[5];
        pBumps[0] = &fReputationBumpVeryMinor;
        pBumps[1] = &fReputationBumpMinor;
        pBumps[2] = &fReputationBumpAverage;
        pBumps[3] = &fReputationBumpMajor;
        pBumps[4] = &fReputationBumpVeryMajor;
        cOldLevel = GetReputationLevel(aeType);
        if (aeType == REPUTATION_POSITIVE) {
            fPositiveValue = fPositiveValue - pBumps[aiBump - 1]->Float();
            fPositiveValue = fPositiveValue < 0.0 ? 0.0f : fPositiveValue;
            const char* pText = sRepPositiveLoss.String();
            BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
            cNewLevel = GetReputationLevel(REPUTATION_POSITIVE);
            if (cNewLevel != cOldLevel) {
                rSound = REPUTATION_NEGATIVE;
                bLevelChanged = 1;
            }
            const char* pIcon = sRepPositiveLossIcon.String();
            Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
        }
        else {
            fNegativeValue = fNegativeValue - pBumps[aiBump - 1]->Float();
            fNegativeValue = fNegativeValue < 0.0 ? 0.0f : fNegativeValue;
            const char* pText = sRepNegativeLoss.String();
            BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
            cNewLevel = GetReputationLevel(REPUTATION_NEGATIVE);
            if (cNewLevel != cOldLevel) {
                rSound = REPUTATION_POSITIVE;
                bLevelChanged = 1;
            }
            // Shows the gain icon here (sRepNegativeGainIcon), not the loss icon, deliberately.
            const char* pIcon = sRepNegativeGainIcon.String();
            Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
        }
        if (bLevelChanged) {
            if (pPlayer->IsPlayerCharacterInCombat(nullptr) || Interface::InDialog()) {
                pPlayer->SetLastModifiedFactionReputation(this);
            }
            else {
                DisplayReputationTitleChange();
            }
        }
        AddChange(CHANGE_BASE_OBJECT_VALUE);
    }
}

void TESReputation::RemoveReputationExact(ReputationType aeType, float afAmount) {
    char msg[256];
    char bLevelChanged = 0;
    char cOldLevel = GetReputationLevel(aeType);
    if (aeType == REPUTATION_POSITIVE) {
        fPositiveValue = fPositiveValue - afAmount;
        fPositiveValue = fPositiveValue < 0.0 ? 0.0f : fPositiveValue;
        const char* pText = sRepPositiveLoss.String();
        BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
        if (GetReputationLevel(REPUTATION_POSITIVE) != cOldLevel) {
            rSound = REPUTATION_NEGATIVE;
            bLevelChanged = 1;
        }
        const char* pIcon = sRepPositiveLossIcon.String();
        Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
    }
    else {
        fNegativeValue = fNegativeValue - afAmount;
        fNegativeValue = fNegativeValue < 0.0 ? 0.0f : fNegativeValue;
        const char* pText = sRepNegativeLoss.String();
        BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
        if (GetReputationLevel(REPUTATION_NEGATIVE) != cOldLevel) {
            rSound = REPUTATION_POSITIVE;
            bLevelChanged = 1;
        }
        const char* pIcon = sRepNegativeLossIcon.String();
        Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
    }
    if (bLevelChanged) {
        if (pPlayer->IsPlayerCharacterInCombat(nullptr) || Interface::InDialog()) {
            pPlayer->SetLastModifiedFactionReputation(this);
        }
        else {
            DisplayReputationTitleChange();
        }
    }
    AddChange(CHANGE_BASE_OBJECT_VALUE);
}

// The bump table is indexed from one entry below its first; aiBump runs 1 to 5.
void TESReputation::AddReputation(ReputationType aeType, int32_t aiBump) {
    if (aiBump > 0 && aiBump < 6) {
        char msg[256];
        char bLevelChanged = 0;
        char cNewLevel = 0;
        SettingT<GameSettingCollection>* pBumps[5];
        pBumps[0] = &fReputationBumpVeryMinor;
        pBumps[1] = &fReputationBumpMinor;
        pBumps[2] = &fReputationBumpAverage;
        pBumps[3] = &fReputationBumpMajor;
        pBumps[4] = &fReputationBumpVeryMajor;
        char cOldLevel = GetReputationLevel(aeType);
        if (aeType == REPUTATION_POSITIVE) {
            fPositiveValue = fPositiveValue + pBumps[aiBump - 1]->Float();
            fPositiveValue = data.fMaximumValue < static_cast<double>(fPositiveValue) ? data.fMaximumValue
                                                                                      : fPositiveValue;
            const char* pText = sRepPositiveGain.String();
            BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
            cNewLevel = GetReputationLevel(REPUTATION_POSITIVE);
            if (cNewLevel != cOldLevel) {
                rSound = REPUTATION_POSITIVE;
                bLevelChanged = 1;
            }
            const char* pIcon = sRepPositiveGainIcon.String();
            Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
        }
        else {
            fNegativeValue = fNegativeValue + pBumps[aiBump - 1]->Float();
            fNegativeValue = data.fMaximumValue < static_cast<double>(fNegativeValue) ? data.fMaximumValue
                                                                                      : fNegativeValue;
            const char* pText = sRepNegativeGain.String();
            BSsprintf(msg, 250, "%s\n%s", GetFullName(), pText);
            cNewLevel = GetReputationLevel(REPUTATION_NEGATIVE);
            if (cNewLevel != cOldLevel) {
                rSound = REPUTATION_NEGATIVE;
                bLevelChanged = 1;
            }
            const char* pIcon = sRepNegativeGainIcon.String();
            Interface::ShowMessage(msg, 0, pIcon, nullptr, 2.0f, false);
        }
        if (bLevelChanged) {
            if (pPlayer->IsPlayerCharacterInCombat(nullptr) || Interface::InDialog()) {
                pPlayer->SetLastModifiedFactionReputation(this);
            }
            else {
                DisplayReputationTitleChange();
            }
        }
        AddChange(CHANGE_BASE_OBJECT_VALUE);
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
    Setting* pRepDescs[16];
    pRepDescs[0] = &sRepTitlePosNoneNegNoneDesc;
    pRepDescs[1] = &sRepTitlePosOneNegNoneDesc;
    pRepDescs[2] = &sRepTitlePosTwoNegNoneDesc;
    pRepDescs[3] = &sRepTitlePosThreeNegNoneDesc;
    pRepDescs[4] = &sRepTitlePosNoneNegOneDesc;
    pRepDescs[5] = &sRepTitlePosOneNegOneDesc;
    pRepDescs[6] = &sRepTitlePosTwoNegOneDesc;
    pRepDescs[7] = &sRepTitlePosThreeNegOneDesc;
    pRepDescs[8] = &sRepTitlePosNoneNegTwoDesc;
    pRepDescs[9] = &sRepTitlePosOneNegTwoDesc;
    pRepDescs[10] = &sRepTitlePosTwoNegTwoDesc;
    pRepDescs[11] = &sRepTitlePosThreeNegTwoDesc;
    pRepDescs[12] = &sRepTitlePosNoneNegThreeDesc;
    pRepDescs[13] = &sRepTitlePosOneNegThreeDesc;
    pRepDescs[14] = &sRepTitlePosTwoNegThreeDesc;
    pRepDescs[15] = &sRepTitlePosThreeNegThreeDesc;
    uint8_t ucPosIndex = GetReputationLevel(REPUTATION_POSITIVE);
    uint8_t ucNegIndex = GetReputationLevel(REPUTATION_NEGATIVE);
    return (*(&pRepDescs[4 * static_cast<char>(ucNegIndex)] + static_cast<char>(ucPosIndex)))->String();
}

const char* TESReputation::GetReputationMainIcon() {
    if (GetTextureNameLength()) {
        return GetTextureName();
    }

    return "";
}
