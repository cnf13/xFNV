#include "Fallout Shared/TESActorBase.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSBodyPartData.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Reset3DMobIterator.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAnimation.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSpellList.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"

// The exe's destructor only runs the bases' destructors.
TESActorBase::~TESActorBase() {}

void TESActorBase::AddChange(int32_t aiChangeFlags) {
	uint32_t v2 = TESForm::GetFormID();
	if (pDataHandler->IsFormIDCreated(v2)) {
		if (pSaveLoadGame->ShouldAddChanges(this, 0)) {
			static_cast<TESActorBaseData*>(this)->AddChangeFlags(aiChangeFlags);
		}
	} else {
		TESForm::AddChange(aiChangeFlags);
	}
}

void TESActorBase::RemoveChange(int32_t aiChangeFlags) {
	uint32_t v2 = TESForm::GetFormID();
	if (pDataHandler->IsFormIDCreated(v2) && pSaveLoadGame->ShouldRemoveChanges(this)) {
		static_cast<TESActorBaseData*>(this)->RemoveChangeFlags(aiChangeFlags);
	} else {
		TESForm::RemoveChange(aiChangeFlags);
	}
}

// The bytes the actor base adds to an old-format save: the block header, the form's own data (version 109 on),
// the components' data and, with flag 0x20, the name and its terminator. The exe adds 4 under `aiFlags & 0`, a
// change flag compiled out to zero. The debug log goes through the exe's empty printf stub (InitializationFlow).
uint16_t TESActorBase::GetSaveSize(uint32_t aiFlags) {
	uint16_t sSize = 0;
	uint16_t sStartSize = sSize;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		sSize += 4;
		sSize += 2;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 109) {
		sSize += TESForm::GetSaveSize(aiFlags);
	}
	sSize += TESAttributes::GetSaveSize(aiFlags);
	sSize += TESActorBaseData::GetSaveSize(aiFlags);
	sSize += TESSpellList::GetSaveSize(aiFlags);
	sSize += TESAIForm::GetSaveSize(aiFlags);
	if (aiFlags & 0) {
		sSize += 4;
	}
	if (aiFlags & 0x20) {
		sSize += 1;
		sSize += BSstrlen(TESFullName::GetFullName());
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
		if (pHeader) {
			TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
			const char* pEditorID = pForm->GetFormEditorID();
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				sSize - sStartSize, pHeader->iFormID, pEditorID, pHeader->iFlags, 550,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", sSize - sStartSize, 550,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp");
		}
	}
	return sSize;
}

void TESActorBase::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	TESActorBaseData::SaveGame(apSaveGameBuffer);
	SaveGameBGS(apSaveGameBuffer);
	TESAttributes::SaveGame(apSaveGameBuffer);
	TESAIForm::SaveGame(apSaveGameBuffer);
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(32)) {
		apSaveGameBuffer->SaveString(GetFullName(), 0);
	}
}

// The actor base's old-format save data: the block header, the form's own data (version 109 on), the components'
// data and, with flag 0x20, the name. The exe reads the health under `aiFlags & 0`, a change flag compiled out to
// zero, so that block never runs. The debug log goes through the exe's empty printf stub (InitializationFlow).
void TESActorBase::SaveGame(uint32_t aiFlags) {
	uint16_t sBlockSize = 0;
	char* pBlockStart = nullptr;
	char* pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
	if (bDebugSaveBuffer.Bool()) {
		pStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint32_t iBlockID = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&iBlockID, 4);
		pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
		pSaveLoadGameOLD->SaveGameDataOLD(&sBlockSize, 2);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 109) {
		TESForm::SaveGame(aiFlags);
	}
	TESAttributes::SaveGameTES(aiFlags);
	TESActorBaseData::SaveGameTES(aiFlags);
	TESSpellList::SaveGame(aiFlags);
	TESAIForm::SaveGame(aiFlags);
	if (aiFlags & 0) {
		uint32_t uiHealth = TESHealthForm::GetFormHealth();
		TESForm::SaveGameDataOLD(&uiHealth, 4);
	}
	if (aiFlags & 0x20) {
		const char* pFullName = TESFullName::GetFullName();
		uint8_t cLength = static_cast<uint8_t>(BSstrlen(pFullName));
		TESForm::SaveGameDataOLD(&cLength, 1);
		if (cLength) {
			TESForm::SaveGameDataOLD(const_cast<char*>(pFullName), cLength);
		}
	}
	if (bDebugSaveBuffer.Bool()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
		SaveFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlySavingFormHeader();
		if (pHeader) {
			TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
			const char* pEditorID = pForm->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				static_cast<int32_t>(pEnd - pStart), pHeader->iFormID, pEditorID, pHeader->iFlags, 587,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", static_cast<int32_t>(pEnd - pStart),
				587, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		char* pEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
		char* pBlockDataStart = pBlockStart;
		if (pEnd > pBlockDataStart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp", 587);
		}
		// The save buffer is raw untyped memory: the exe writes the block's size as a bare word at its start.
		*reinterpret_cast<uint16_t*>(pBlockStart) = static_cast<uint16_t>(pEnd - pBlockDataStart);
	}
}

void TESActorBase::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	BGSChangeFlags result;
	int8_t apModel[268];
	TESForm::LoadGame(apLoadGameBuffer);
	static_cast<TESActorBaseData*>(this)->LoadGame(apLoadGameBuffer);
	static_cast<TESSpellList*>(this)->LoadGame(apLoadGameBuffer);
	static_cast<TESAttributes*>(this)->LoadGame(apLoadGameBuffer);
	static_cast<TESAIForm*>(this)->LoadGame(apLoadGameBuffer);
	BGSChangeFlags* v2 = &(result = apLoadGameBuffer->GetChangeFlags());
	if (v2->CheckFlags(0x20u)) {
		apLoadGameBuffer->LoadString(static_cast<char*>(static_cast<void*>(apModel)));
		static_cast<TESFullName*>(this)
			->TESFullName::SetFullName(static_cast<const char*>(static_cast<const void*>(apModel)));
	}
}

// The actor base's old-format load: reads the block header, the form's own data (version 109 on) and the components'
// data, and with flag 0x20 the name. The exe reads a 4-byte value and calls the shared one-field setter 0x6ECD40
// under `aiFlags & 0`, a change flag compiled out to zero, so that block never runs and is left out.
void TESActorBase::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
	uint16_t sBlockSize = 0;
	char* pBlockStart = nullptr;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint32_t iBlockID;
		pSaveLoadGameOLD->LoadGameDataOLD(&iBlockID, 4);
		if (iBlockID != 0x424C4F4B) {
			LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
			if (pHeader) {
				TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
				uint32_t iFlags = pHeader->iFlags;
				uint8_t cVersion = pHeader->cVersion;
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
						"Currently loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp", 596, pHeader->iFormID,
					pEditorID, cVersion, iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
						"Current version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp", 596,
					pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
		pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, 2);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 109) {
		TESForm::LoadGame(aiFlags, aiCurrentFlags);
	}
	TESAttributes::LoadGameTES(aiFlags, aiCurrentFlags);
	TESActorBaseData::LoadGameTES(aiFlags, aiCurrentFlags);
	TESSpellList::LoadGame(aiFlags, aiCurrentFlags);
	TESAIForm::LoadGame(aiFlags, aiCurrentFlags);
	if (aiFlags & 0x20) {
		char pFullName[0x104];
		ourMemset(pFullName, 0, 0x104);
		char cLength;
		TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&cLength), 1);
		if (cLength) {
			TESForm::LoadGameDataOLD(reinterpret_cast<double*>(pFullName), static_cast<uint8_t>(cLength));
		}
		TESFullName::SetFullName(pFullName);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
		LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
		if (pHeader) {
			TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
			if (pBlockEnd > pBlockStart + sBlockSize) {
				uint32_t iFlags = pHeader->iFlags;
				uint8_t cVersion = pHeader->cVersion;
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
						"form is %08X %s with version %i and flags %08X",
					static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp", 628, pHeader->iFormID,
					pEditorID, cVersion, iFlags);
			} else if (pBlockEnd < pBlockStart + sBlockSize) {
				uint32_t iFlags = pHeader->iFlags;
				uint8_t cVersion = pHeader->cVersion;
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
						"form is %08X %s with version %i and flags %08X",
					static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp", 628, pHeader->iFormID,
					pEditorID, cVersion, iFlags);
			}
		} else if (pBlockEnd > pBlockStart + sBlockSize) {
			uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
			Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
				static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp", 628, cVersion);
		} else if (pBlockEnd < pBlockStart + sBlockSize) {
			uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
			Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
				static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESActorBase.cpp", 628, cVersion);
		}
	}
}

// The exe's body (0x9DACE0, shared by many vtables) only calls TESForm's empty stub.
void TESActorBase::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {}

bool TESActorBase::IsAutoCalc() {
	if (GetFormType() == NPC__ID) {
		return GetFlag(BASE_AUTO_CALC_STATS);
	}
	return false;
}

void TESActorBase::SetAutoCalc(bool abAC) {
	if (GetFormType() == NPC__ID) {
		SetFlagBit(BASE_AUTO_CALC_STATS, abAC, true);
	}
}

// The exe's body takes the ActorValueOwner subobject as this.
int32_t TESActorBase::GetActorValue(ActorValue::Index aiWhich) {
	int32_t ExtraType = 0;
	switch (aiWhich) {
	case ActorValue::AGGRESSION: {
		ExtraType = AIData.cAggression;
		break;
	}
	case ActorValue::CONFIDENCE: {
		ExtraType = AIData.cConfidence;
		break;
	}
	case ActorValue::ENERGY: {
		ExtraType = AIData.cEnergy;
		break;
	}
	case ActorValue::RESPONSIBILITY: {
		ExtraType = GetResponsibility();
		break;
	}
	case ActorValue::MOOD: {
		ExtraType = AIData.cMood;
		break;
	}
	case ActorValue::STRENGTH: {
		ExtraType = GetStrength();
		break;
	}
	case ActorValue::PERCEPTION: {
		ExtraType = GetPerception();
		break;
	}
	case ActorValue::ENDURANCE: {
		ExtraType = GetEndurance();
		break;
	}
	case ActorValue::CHARISMA: {
		ExtraType = GetCharisma();
		break;
	}
	case ActorValue::INTELLIGENCE: {
		ExtraType = GetIntelligence();
		break;
	}
	case ActorValue::AGILITY: {
		ExtraType = GetAgility();
		break;
	}
	case ActorValue::LUCK: {
		ExtraType = GetLuck();
		break;
	}
	case ActorValue::HEALTH: {
		ExtraType = GetHealth();
		break;
	}
	case ActorValue::SPEED_MULT: {
		ExtraType = GetSpeedMultiplier();
		break;
	}
	case ActorValue::FATIGUE: {
		ExtraType = GetFatigue();
		break;
	}
	case ActorValue::KARMA: {
		ExtraType = static_cast<int32_t>(GetKarma());
		break;
	}
	case ActorValue::ASSISTANCE: {
		ExtraType = TESAIForm::GetAssistance();
		break;
	}
	default: {
		return ExtraType;
	}
	}
	return ExtraType;
}

// The exe's body runs on the ActorValueOwner subobject: GetActorValue through the vtable (slot 2), then fild of the
// int, returned in ST0 without a store.
float TESActorBase::GetActorFloatValue(ActorValue::Index aiWhich) {
	ActorValueOwner* pOwner = this;
	return static_cast<float>(pOwner->GetActorValue(aiWhich));
}

BGSBodyPartData* TESActorBase::GetBodyPartData() {
	if (BGSBodyPartData::pPlayerBodyPartData && (this == pPlayer->Actor::GetActorBase())) {
		return BGSBodyPartData::pPlayerBodyPartData;
	} else {
		return BGSBodyPartData::pDefaultBodyPartData;
	}
}

void TESActorBase::SetActorValue(ActorValue::Index aiWhich, int32_t aiValue) {
	float afKarma;
	switch (aiWhich) {
	case 0: {
		static_cast<TESAIForm*>(this)->SetAggression(aiValue, 1);
		break;
	}
	case 1: {
		static_cast<TESAIForm*>(this)->SetConfidence(aiValue, 1);
		break;
	}
	case 2: {
		static_cast<TESAIForm*>(this)->SetEnergy(aiValue, 1);
		break;
	}
	case 3: {
		static_cast<TESAIForm*>(this)->SetResponsibility(aiValue, 1);
		break;
	}
	case 4: {
		static_cast<TESAIForm*>(this)->SetMood(aiValue);
		break;
	}
	case 5: {
		static_cast<TESAttributes*>(this)->SetStrength(aiValue);
		break;
	}
	case 6: {
		static_cast<TESAttributes*>(this)->SetPerception(aiValue);
		break;
	}
	case 7: {
		static_cast<TESAttributes*>(this)->SetEndurance(aiValue);
		break;
	}
	case 8: {
		static_cast<TESAttributes*>(this)->SetCharisma(aiValue);
		break;
	}
	case 9: {
		static_cast<TESAttributes*>(this)->SetIntelligence(aiValue);
		break;
	}
	case 10: {
		static_cast<TESAttributes*>(this)->SetAgility(aiValue);
		break;
	}
	case 11: {
		static_cast<TESAttributes*>(this)->SetLuck(aiValue);
		break;
	}
	case 16: {
		SetHealth(aiValue);
		break;
	}
	case 21: {
		static_cast<TESActorBaseData*>(this)->SetSpeedMult(aiValue);
		break;
	}
	case 23: {
		afKarma = (float)(double)aiValue;
		static_cast<TESActorBaseData*>(this)->SetKarma(afKarma);
		break;
	}
	default: {
		return;
	}
	}
}

void TESActorBase::SetActorValue(ActorValue::Index aiWhich, float afValue) {
	SetActorValue(aiWhich, static_cast<int32_t>(afValue));
}

bool TESActorBase::FightsInWater() {
	if (!DoesSwim()) {
		return (bool)0;
	}
	return (bool)((uint8_t)((((int32_t)TESForm::GetFormType()) != 43) ||
		(!static_cast<TESActorBaseData*>(this)->GetFlag(262144))));
}

// Resets the 3D of the high and middle-high process actors made from this base, then the player's when it is one.
void TESActorBase::Reset3DForLoadedActors(uint8_t acResetFlags) {
	if (acResetFlags) {
		FNVD_CONSTRUCTED_LOCAL(Reset3DMobIterator, iterator, nullptr);
		iterator.pActorBase = this;
		iterator.cFlags = acResetFlags;
		iterator.eType = PROCESS_TYPE_HIGH;
		ReferenceProcessLists.IterateMobs(iterator);
		iterator.eType = PROCESS_TYPE_MIDDLE_HIGH;
		ReferenceProcessLists.IterateMobs(iterator);
		if (pPlayer->GetActorBase() == this) {
			if (pPlayer->pCurrentProcess) {
				BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
				CurrentAIProcess->Set3DUpdateFlag(static_cast<RESET_3D_FLAGS>(acResetFlags));
			}
		}
	}
}

TESActorBase::SEX TESActorBase::GetSex() const {
	if (GetFormType() == NPC__ID) {
		return GetFlag(BASE_FEMALE) ? SEX_FEMALE : SEX_MALE;
	}
	return SEX_NONE;
}

bool TESActorBase::IsImmobile() const {
	if (GetFormType() == CREA_ID) {
		return GetFlag(BASE_CREATURE_IMMOBILE);
	}
	return false;
}

bool TESActorBase::DoesSwim() {
	if (((int32_t)TESForm::GetFormType()) != 43) {
		return (bool)1;
	}
	return (bool)((uint8_t)(static_cast<TESActorBaseData*>(this)->GetFlag(16) ||
		static_cast<TESActorBaseData*>(this)->GetFlag(1)));
}

bool TESActorBase::DoesWalk() const {
	if (GetFormType() == CREA_ID) {
		return GetFlag(BASE_CREATURE_WALKS) || GetFlag(BASE_FEMALE);
	}
	return true;
}

int32_t TESActorBase::GetHealth() const {
	return static_cast<const TESHealthForm*>(this)->GetFormHealth();
}

// The desirability of an item to this actor: armour by rating and threshold, clothing by value, weapons by damage.
float TESActorBase::GetDesirability(TESForm* apForm) {
	float fRetVal = 0.0f;
	if (apForm) {
		switch (apForm->GetFormType()) {
		case ARMO_ID: {
			TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(apForm);
			if (pArmor) {
				if (TESHealthForm::GetFormHealth(pArmor)) {
					int32_t ArmorRating = static_cast<int32_t>(pArmor->GetArmorRating());
					double v6 = CombatFormulas::CalcArmorRating(ArmorRating, 1.0f);
					fRetVal = pArmor->GetDamageThreshold() + v6;
				}
			}
			break;
		}
		case CLOT_ID: {
			TESObjectCLOT* pClothing = dynamic_cast<TESObjectCLOT*>(apForm);
			if (pClothing) {
				uint32_t FormHealth_0 = pClothing->iValue;
				fRetVal = GamePlayFormulas::GetActorClothingDesirability(FormHealth_0);
			}
			break;
		}
		case LIGH_ID: {
			fRetVal = 100.0f;
			break;
		}
		case WEAP_ID: {
			TESObjectWEAP* pWeap = dynamic_cast<TESObjectWEAP*>(apForm);
			if (pWeap) {
				if (TESHealthForm::GetFormHealth(pWeap)) {
					double fRetVal_2;
					float condition = 1.0f;
					if (this) {
						fRetVal_2 = CombatFormulas::CalcWeaponDamage(this, pWeap, condition, 1.0f, 0, 0, 0, 0);
					} else {
						fRetVal_2 = CombatFormulas::CalcWeaponDamage(0, pWeap, condition, 1.0f, 0, 0, 0, 0);
					}
					fRetVal = fRetVal_2;
				}
			}
			break;
		}
		case AMMO_ID: {
			if (FNVD_DYNAMIC_CAST_VOID(TESAmmo)(apForm, RTTI_TESForm, RTTI_TESAmmo)) {
				fRetVal = 1.0f;
			}
			break;
		}
		default: {
			return fRetVal;
		}
		}
	}
	return fRetVal;
}

bool TESActorBase::GetStartsDead() {
	return (GetFormFlags() & kFormFlag_HasTemp3D) != 0;
}

// The change flags are the exe's own literals: 0x8000020 while a save loads, 0x400 for a dead reference. Each is
// built in its stack slot by the shared flags constructor before the query, as the exe does.
bool TESActorBase::ShouldRunInitDefaultWorn(TESObjectREFR* apRef) {
	bool bRun = true;
	if (pSaveLoadGame->GetSaveGameLoading()) {
		if (apRef->IsStillLoading()) {
			BGSChangeFlags kChanges;
			kChanges.~BGSChangeFlags();
			::new (&kChanges) BGSChangeFlags(0x8000020);
			if (pSaveLoadGame->GetChange(apRef, kChanges)) {
				bRun = false;
			}
		}
	} else if (apRef->IsDead(false)) {
		BGSChangeFlags kChanges;
		kChanges.~BGSChangeFlags();
		::new (&kChanges) BGSChangeFlags(0x400);
		if (pSaveLoadGame->GetChange(apRef, kChanges)) {
			bRun = false;
		}
	}
	return bRun;
}

void TESActorBase::SetHealth(int32_t aiValue) {
	TESHealthForm::SetFormHealth(aiValue);
}

int32_t TESActorBase::CalculateHealth(char a1) {
	int32_t v4;
	uint16_t Level = GetLevel();
	int32_t v11 = Level;
	if (!Level) {
		v11 = 1;
	}
	if (TESHealthForm::GetFormHealth()) {
		uint32_t FormHealth_0;
		if (a1) {
			FormHealth_0 = TESHealthForm::GetFormHealth();
		} else {
			FormHealth_0 = 0;
		}
		double Endurance = static_cast<double>(GetEndurance());
		double v6 = fAVDNPCHealthEnduranceOffset.Float() + Endurance;
		double v5 = fAVDNPCHealthEnduranceMult.Float() * v6 + static_cast<double>(FormHealth_0);
		v4 = static_cast<int32_t>(fAVDNPCHealthLevelMult.Float() * static_cast<double>(v11 - 1) + v5);
	} else {
		v4 = 0;
	}
	if (v4 < 0) {
		return 0;
	}
	return v4;
}

// pActorBase and cFlags are left unset, as the exe's constructor leaves them.
Reset3DMobIterator::Reset3DMobIterator() : MobIterOperator() {}

void Reset3DMobIterator::Operate(MobileObject* apMob) {
	BaseProcess* currentAIProcess;
	if (static_cast<const void*>(apMob->TESObjectREFR::GetObjectReference()) == static_cast<const void*>(pActorBase)) {
		if (apMob->MobileObject::GetCurrentAIProcess()) {
			currentAIProcess = apMob->MobileObject::GetCurrentAIProcess();
			currentAIProcess->Set3DUpdateFlag(static_cast<RESET_3D_FLAGS>(cFlags));
		}
	}
}
