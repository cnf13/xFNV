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
#include <stdint.h>

void CharacterProgressionData::Initialize() {
	bReadyToLevelUp = (bool)0;
	iNextThreshold = (int32_t)((uint32_t)GamePlayFormulas::GetRequiredExperiencePoints(2));
	bIgnoreCallback = (bool)0;
}

bool CharacterProgressionData::CheckCanLevelUp(int32_t aiLevel) {
	bool v5 = 0;
	uint16_t Level = pPlayer->GetLevel();
	if (Level >= iMaxCharacterLevel.Int()) {
		return 0;
	}
	int32_t RequiredExperiencePoints = GamePlayFormulas::GetRequiredExperiencePoints(aiLevel);
	if (pPlayer->GetActorValue(ActorValue::EXPERIENCE_POINTS) >= RequiredExperiencePoints) {
		bReadyToLevelUp = 1;
		return 1;
	}
	return v5;
}

void CharacterProgressionData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&bReadyToLevelUp), 1, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iNextThreshold), 4, 0);
}

void CharacterProgressionData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&bReadyToLevelUp), 1);
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&iNextThreshold), 4);
}

void CharacterProgressionData::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	bReadyToLevelUp = 0;
	iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(2);
}

void CharacterProgression::DeleteData() {
	if (pData) {
		ourFree(pData);
		pData = nullptr;
	}
}

void CharacterProgression::RewardExperience(int32_t aiAmount) {
	float famount;
	uint16_t Level = pPlayer->GetLevel();
	if (Level < iMaxCharacterLevel.Int()) {
		famount = static_cast<float>(aiAmount);
		BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_EXPERIENCE_POINTS, pPlayer, &famount);
		BSAudio* v3 = &BSAudio::QInstance();
		v3->Precache("UILevelUp", 289u);
		int32_t iamount = static_cast<int32_t>(std::ceil(famount));
		int32_t inew = iamount + pPlayer->GetActorValue(ActorValue::EXPERIENCE_POINTS);
		int32_t imax = GamePlayFormulas::GetRequiredExperiencePoints(iMaxCharacterLevel.Int());
		if (inew > imax) {
			iamount -= inew - imax;
		}
		pPlayer->PermanentModActorValue(ActorValue::EXPERIENCE_POINTS, iamount, 0);
	}
}

CharacterProgressionData* CharacterProgression::GetData() {
	return pData;
}

void CharacterProgression::BeginLevelUp() {
	if (GetData()->bReadyToLevelUp) {
		BSAudio::QInstance().Precache("UILevelUp",
			static_cast<BSGameSound::SoundFlags>(BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
				BSGameSound::SOUND_FLAG_ONE_SHOT));
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
	CharacterProgressionData* v1;
	void* v3 = ourMalloc(sizeof(CharacterProgressionData));
	if (v3) {
		v1 = ::new (v3) CharacterProgressionData;
	} else {
		v1 = nullptr;
	}
	pData = v1;
}

void CharacterProgression::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	pData->Revert(apLoadGameBuffer);
}

void CharacterProgression::FinishLevelUp() {
	uint16_t Level = pPlayer->GetLevel();
	if (!pData->CheckCanLevelUp(Level + 1)) {
		pData->bReadyToLevelUp = 0;
		pDataHandler->UpdateAutoCalcNPCs();
		ReferenceProcessLists.InitActorLeveledItems();
		if (XGamerProfile::Instance()) {
			XGamerProfile* v2 = XGamerProfile::Instance();
			v2->ForcePresenceUpdate();
		}
		if (bTrackProgress.operator bool()) {
			pPlayer->ExportProgressData(PlayerCharacter::LEVEL_UP);
		}
	}
}

void CharacterProgression::ForceLevel(int32_t aiLevel) {
	if (aiLevel != pPlayer->GetLevel()) {
		CharacterProgressionData* pdata = pPlayer->GetCharacterProgressionInfo()->pData;
		int32_t icurrentXP = pPlayer->GetActorValue(ActorValue::EXPERIENCE_POINTS);
		int32_t idelta = GamePlayFormulas::GetRequiredExperiencePoints(aiLevel - 1) - icurrentXP;
		pdata->bIgnoreCallback = 1;
		pPlayer->PermanentModActorValue(ActorValue::EXPERIENCE_POINTS, idelta, static_cast<Actor*>(0));
		pdata->bIgnoreCallback = 0;
		pdata->bReadyToLevelUp = 0;
		pdata->iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(aiLevel + 1);
		pPlayer->GetActorBase()->SetLevel(aiLevel);
		FinishLevelUp();
		StatsMenu::UpdatePlayerName();
		StatsMenu::UpdatePlayerTitle();
	}
}

int32_t CharacterProgression::GetSaveSize(uint32_t a1) {
	return 5;
}
