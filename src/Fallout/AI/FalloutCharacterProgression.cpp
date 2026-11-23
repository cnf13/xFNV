#include "Fallout/AI/FalloutCharacterProgression.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Xenon/XGamerProfile.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"

#include <cmath>
#include <cstdint>

void CharacterProgressionData::Initialize() {
    bReadyToLevelUp = false;
    iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(2);
    bIgnoreCallback = false;
}

bool CharacterProgressionData::CheckCanLevelUp(int32_t aiLevel) {
    uint16_t usLevel = pPlayer->GetLevel();
    if (usLevel >= iMaxCharacterLevel.Int()) {
        return false;
    }
    int32_t iRequiredXP = GamePlayFormulas::GetRequiredExperiencePoints(aiLevel);
    if (pPlayer->GetActorValue(ActorValue::EXPERIENCE_POINTS) >= iRequiredXP) {
        bReadyToLevelUp = true;
        return true;
    }
    return false;
}

void CharacterProgressionData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bReadyToLevelUp), sizeof(bReadyToLevelUp), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iNextThreshold), sizeof(iNextThreshold), 0);
}

void CharacterProgressionData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bReadyToLevelUp), sizeof(bReadyToLevelUp));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iNextThreshold), sizeof(iNextThreshold));
}

void CharacterProgressionData::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    bReadyToLevelUp = false;
    iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(2);
}

void CharacterProgression::DeleteData() {
    if (pData) {
        ourFree(pData);
        pData = nullptr;
    }
}

void CharacterProgression::RewardExperience(int32_t aiAmount) {
    uint16_t usLevel = pPlayer->GetLevel();
    if (usLevel < iMaxCharacterLevel.Int()) {
        float fAmount = static_cast<float>(aiAmount);
        BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_EXPERIENCE_POINTS, pPlayer, &fAmount);
        BSAudio::QInstance().Precache("UILevelUp", 289u);
        int32_t iAmount = static_cast<int32_t>(std::ceil(fAmount));
        int32_t iNew = iAmount + pPlayer->GetActorValue(ActorValue::EXPERIENCE_POINTS);
        int32_t iMax = GamePlayFormulas::GetRequiredExperiencePoints(iMaxCharacterLevel.Int());
        if (iNew > iMax) {
            iAmount -= iNew - iMax;
        }
        pPlayer->PermanentModActorValue(ActorValue::EXPERIENCE_POINTS, iAmount, nullptr);
    }
}

CharacterProgressionData* CharacterProgression::GetData() {
    return pData;
}

void CharacterProgression::BeginLevelUp() {
    if (GetData()->bReadyToLevelUp) {
        BSAudio::QInstance().Precache(
            "UILevelUp",
            static_cast<BSGameSound::SoundFlags>(
                BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND | BSGameSound::SOUND_FLAG_ONE_SHOT
            )
        );
        uint16_t usLevel = pPlayer->GetLevel() + 1;
        TESActorBaseData& rBaseData = *pPlayer->GetActorBase();
        rBaseData.SetLevel(usLevel);
        LevelUpMenu::Create();
    }
}

void CharacterProgression::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    pData->SaveGame(apSaveGameBuffer);
}

void CharacterProgression::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    pData->LoadGame(apLoadGameBuffer);
}

void CharacterProgression::AllocateData() {
    pData = new CharacterProgressionData;
}

void CharacterProgression::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    pData->Revert(apLoadGameBuffer);
}

void CharacterProgression::FinishLevelUp() {
    uint16_t usLevel = pPlayer->GetLevel();
    if (!pData->CheckCanLevelUp(usLevel + 1)) {
        pData->bReadyToLevelUp = false;
        pDataHandler->UpdateAutoCalcNPCs();
        ReferenceProcessLists.InitActorLeveledItems();
        if (XGamerProfile::Instance()) {
            XGamerProfile::Instance()->ForcePresenceUpdate();
        }
        if (bTrackProgress) {
            pPlayer->ExportProgressData(PlayerCharacter::LEVEL_UP);
        }
    }
}

void CharacterProgression::ForceLevel(int32_t aiLevel) {
    if (aiLevel != pPlayer->GetLevel()) {
        CharacterProgressionData* pdata = pPlayer->GetCharacterProgressionInfo()->pData;
        int32_t iCurrentXP = pPlayer->GetActorValue(ActorValue::EXPERIENCE_POINTS);
        int32_t iDelta = GamePlayFormulas::GetRequiredExperiencePoints(aiLevel - 1) - iCurrentXP;
        pdata->bIgnoreCallback = true;
        pPlayer->PermanentModActorValue(ActorValue::EXPERIENCE_POINTS, iDelta, static_cast<Actor*>(nullptr));
        pdata->bIgnoreCallback = false;
        pdata->bReadyToLevelUp = false;
        pdata->iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(aiLevel + 1);
        pPlayer->GetActorBase()->SetLevel(aiLevel);
        FinishLevelUp();
        StatsMenu::UpdatePlayerName();
        StatsMenu::UpdatePlayerTitle();
    }
}

int32_t CharacterProgression::GetSaveSize(uint32_t aiFlags) {
    return 5;
}
