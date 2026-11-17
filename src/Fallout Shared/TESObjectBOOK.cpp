#include "Fallout Shared/TESObjectBOOK.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSMultiBoundRoom.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "SDK/MSVC/rtti.h"

// The game settings this file defines, in initialization order.
SettingT<GameSettingCollection> fBookPerkBonus("fBookPerkBonus", 1.0f);

TESObjectBOOK::TESObjectBOOK() {
	SetFormType(25u);
	TESObjectBOOK::InitializeData();
}

TESObjectBOOK::~TESObjectBOOK() {}

void TESObjectBOOK::InitializeData() {
	ourMemset(&Data, 0, sizeof(Data));
	Data.eTeaches = 0xFF;
	static_cast<TESEnchantableForm*>(this)->SetCastingType(MagicSystem::CAST_ONCE);
}

// Reads the book's chunks: its name, model, icon, script, enchantment and description components, the destructible
// data and the pickup and putdown sounds, then its own two data bytes.
bool TESObjectBOOK::Load(TESFile* apFile) {
	if ((uint8_t)apFile->GetTESForm() != 25) {
		return false;
	}
	LoadForm(apFile);
	SetInitialized(false);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		switch (uiChunk) {
		case EDID_ID: {
			// A 16-byte aligned stack buffer of the chunk's size.
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
		case MODL_ID:
		case MODD_ID:
		case MOSD_ID:
		case MODT_ID:
			TESModel::LoadModelChunk(static_cast<TESModelTextureSwap*>(this), apFile);
			break;
		case MODS_ID: {
			uint32_t uiSize = apFile->GetChunkSize();
			// A 16-byte aligned stack buffer of the chunk's size.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiSize, 128));
			apFile->GetChunkData(pBuffer, uiSize);
			uint16_t usFormVersion = apFile->GetFormVersion();
			static_cast<TESModelTextureSwap*>(this)->UnpackTextureSwapChunkData(pBuffer, uiSize, usFormVersion);
			break;
		}
		case ICON_ID:
			TESTexture::LoadTextureChunk(this, apFile);
			break;
		case MICO_ID:
			BGSMessageIcon::LoadMessageIcon(this, apFile);
			break;
		case SCRI_ID: {
			uint32_t uiScriptID = 0;
			apFile->GetChunkData(uiScriptID);
			TESScriptableForm* pScriptableForm = static_cast<TESScriptableForm*>(this);
			pScriptableForm->SetFormScript(reinterpret_cast<Script*>(static_cast<uintptr_t>(uiScriptID)));
			pScriptableForm->InitItem(this);
			break;
		}
		case ENAM_ID:
		case EITM_ID: {
			uint32_t uiEnchantmentID = 0;
			apFile->GetChunkData(uiEnchantmentID);
			static_cast<TESEnchantableForm*>(this)
				->SetFormEnchanting(reinterpret_cast<EnchantmentItem*>(static_cast<uintptr_t>(uiEnchantmentID)));
			break;
		}
		case ANAM_ID:
		case EAMT_ID: {
			int16_t sAmount = 0;
			apFile->LoadWord(&sAmount);
			static_cast<TESEnchantableForm*>(this)->SetFormEnchantmentAmount((uint16_t)sAmount);
			break;
		}
		case DESC_ID:
			TESDescription::LoadDescriptionData(this, apFile);
			break;
		case DSTD_ID:
		case DEST_ID:
			BGSDestructibleObjectForm::LoadChunk(this, apFile);
			break;
		case YNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData(uiSoundID);
			static_cast<BGSPickupPutdownSounds*>(this)
				->SetPickupSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case ZNAM_ID: {
			uint32_t uiSoundID = 0;
			apFile->GetChunkData(uiSoundID);
			static_cast<BGSPickupPutdownSounds*>(this)
				->SetPutdownSound(reinterpret_cast<TESSound*>(static_cast<uintptr_t>(uiSoundID)));
			break;
		}
		case DATA_ID:
			LoadData(apFile, reinterpret_cast<char*>(&Data), 2u);
			break;
		default:
			break;
		}
	} while (apFile->NextChunk());
	return true;
}

void TESObjectBOOK::Save() {
	StartForm();
	TESFullName::Save();
	TESModelTextureSwap::Save(MODL_ID, MODT_ID, MODS_ID);
	TESTexture::Save(ICON_ID);
	BGSMessageIcon::Save(MICO_ID);
	TESScriptableForm::Save();
	SaveSubRecord();
	TESDescription::Save();
	BGSDestructibleObjectForm::Save();
	BGSPickupPutdownSounds::Save();
	if (TESForm::QEndianSwapOnSave()) {
	}
	__SaveData(&Data, 2u);
	if (TESForm::QEndianSwapOnSave()) {
	}
	CloseForm();
}

uint16_t TESObjectBOOK::GetSaveSize(uint32_t aiFlags) {
	uint16_t saveSize = TESForm::GetSaveSize(aiFlags);
	uint16_t v5 = (uint16_t)(static_cast<TESValueForm*>(this)->TESValueForm::GetSaveSize(aiFlags) + saveSize);
	if ((aiFlags & 32) != 0) {
		++v5;
	}
	return v5;
}

void TESObjectBOOK::SaveGame(uint32_t aiFlags) {
	TESForm::SaveGame(aiFlags);
	TESValueForm::SaveGame(aiFlags);
	if ((aiFlags & 32) != 0) {
		pSaveLoadGameOLD->SaveGameDataOLD(&Data.eTeaches, 1u);
	}
}

// Saves the book's changes into the save buffer: the base form's and the value's, then the skill it teaches when
// that changed.
void TESObjectBOOK::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	TESValueForm::SaveGame(apSaveGameBuffer);
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BOOK_TEACHES_SKILL)) {
		apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&Data.eTeaches), 1, 0);
	}
}

void TESObjectBOOK::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	TESForm::LoadGame(aiFlags, aiCurrentFlags);
	TESValueForm::LoadGame(aiFlags, aiCurrentFlags);
	if ((aiFlags & 32) != 0) {
		pSaveLoadGameOLD->LoadGameDataOLD(&Data.eTeaches, 1u);
	}
}

// Loads the same changes back from the load buffer.
void TESObjectBOOK::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm::LoadGame(apLoadGameBuffer);
	TESValueForm::LoadGame(apLoadGameBuffer);
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_BOOK_TEACHES_SKILL)) {
		apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&Data.eTeaches), 1);
	}
}

void TESObjectBOOK::InitItem() {
	int32_t v2;
	if (!TESForm::GetInitialized()) {
		static_cast<TESScriptableForm*>(this)->InitItem(this);
		static_cast<TESEnchantableForm*>(this)->InitItem(this);
		static_cast<BGSDestructibleObjectForm*>(this)->InitItem(this);
		static_cast<TESModelTextureSwap*>(this)->InitItem(this);
		static_cast<BGSPickupPutdownSounds*>(this)->InitItem(this);
		if (((int8_t)Data.eTeaches) != ((-1))) {
			v2 = ActorValue::ToActorValue(ActorValue::SKILL, Data.eTeaches);
			if ((v2 < 32) || (v2 > 46)) {
				Data.eTeaches = (uint8_t)((-1));
			}
		}
		TESForm::SetInitialized(1);
	}
}

void TESObjectBOOK::Copy(TESForm* apCopy) {
	TESObjectBOOK* pbook = dynamic_cast<TESObjectBOOK*>(apCopy);
	if (pbook) {
		CopyAllComponents(apCopy);
		Data = *&pbook->Data;
	}
}

bool TESObjectBOOK::Compare(TESForm* apCompare) {
	TESObjectBOOK* v5 = dynamic_cast<TESObjectBOOK*>(apCompare);
	if (!v5) {
		return (bool)1;
	}
	if (TESForm::CompareAllComponents(v5)) {
		return (bool)1;
	}
	return (std::memcmp(&Data.cFlags, &v5->Data, 2u) != 0);
}

bool TESObjectBOOK::Activate(TESObjectREFR* apItemActivated, TESObjectREFR* apActionRef, bool abIdFlag,
	TESBoundObject* apObjectToGet, int32_t aiCount) {
	return (bool)((uint8_t)TESBoundObject::Activate(apItemActivated, apActionRef, abIdFlag, apObjectToGet, aiCount));
}

bool TESObjectBOOK::GetCantTake() {
	return ((Data.cFlags & 2) != 0);
}

bool TESObjectBOOK::IsScroll() {
	return ((Data.cFlags & 1) != 0);
}

// The skill byte is tested signed against -1; ToActorValue reads its argument as a signed char.
ActorValue::Index TESObjectBOOK::GetTeaches() {
	if (static_cast<int8_t>(Data.eTeaches) == -1) {
		return ActorValue::VALUE_NONE;
	} else {
		return ActorValue::ToActorValue(ActorValue::SKILL, static_cast<char>(Data.eTeaches));
	}
}

bool TESObjectBOOK::Read(TESObjectREFR* apActionRef) {
	float fskillPoints;
	bool bresult = 0;
	Actor* pactor = dynamic_cast<Actor*>(apActionRef);
	if (!pactor->IsPc()) {
		return bresult;
	}
	if (!pactor->IsInCombat() || Interface::IsInMenuMode()) {
		ActorValue::Index eteaches = GetTeaches();
		if (ActorValue::IsSkill(eteaches) && pPlayer->GetPermanentActorValue(eteaches) < 200) {
			fskillPoints = fBookPerkBonus.Float();
			BSStringT<char> strtemp;
			BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_BOOK_SKILL_POINTS, pPlayer, &fskillPoints);
			pPlayer->PermanentModActorValue(eteaches, fskillPoints, static_cast<Actor*>(0));
			MiscStatManager::Increment(MiscStatManager::MS_BOOKS_READ);
			int32_t v5 = static_cast<int32_t>(fskillPoints);
			const char* time = ActorValue::GetActorValueName(eteaches);
			strtemp.SPrintF(sSkillIncreasedNum.String(), time, v5);
			Interface::ShowMessage(strtemp.pString, 1, 0, 0, 2.0f, 0);
			bresult = 1;
		}
		return bresult;
	} else {
		const char* v3 = sCanNotReadBook.String();
		Interface::ShowMessage(v3, 0, 0, 0, 2.0f, 0);
		return bresult;
	}
}
