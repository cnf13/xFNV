#include "Fallout Shared/BGSEncounterZone.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<GameSettingCollection> fLevelScalingMult("fLevelScalingMult", 0.6f);

// Reads the zone's DATA (byte swapped when the file is big endian), its editor ID and its object bound. The record
// type is narrowed to a byte before the zero-extended compare with ECZN.
bool BGSEncounterZone::Load(TESFile* apFile) {
	uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (ucFormType != ECZN_ID) {
		return false;
	}
	TESForm::LoadForm(apFile);
	this->SetInitialized(false);
	while (true) {
		CHUNK_ID eChunk = static_cast<CHUNK_ID>(apFile->GetTESChunk());
		if (eChunk == NO_CHUNK) {
			break;
		}
		if (eChunk == DATA_ID) {
			apFile->GetChunkData(&Data, 8u);
#if FNVD_64BIT
			// The file's record is 8 bytes, the owner's 4-byte form ID first.
			TESFile::WidenFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(ENCOUNTER_ZONE_DATA, pZoneOwner)}});
#endif
			if (apFile->QEndian()) {
				Data.Endian();
			}
		} else if (eChunk == EDID_ID) {
			// The exe allocates the chunk's size on the stack (__alloca_probe_16) and reads at most 0x200 bytes.
			char* pEditorID = static_cast<char*>(__builtin_alloca(apFile->GetChunkSize()));
			apFile->GetChunkData(pEditorID, 0x200u);
			// The exe calls the form's own override through the vtable (slot 77).
			TESForm* pForm = this;
			pForm->SetFormEditorID(pEditorID);
		} else if (eChunk == OBND_ID) {
			// The exe calls the form's own override through the vtable (slot 56).
			TESForm* pForm = this;
			pForm->LoadObjectBound(apFile);
		}
		if (!apFile->NextChunk()) {
			break;
		}
	}
	return true;
}

void ENCOUNTER_ZONE_DATA::Endian() {
	ENDIAN(pZoneOwner, false);
}

BGSEncounterZone::BGSEncounterZone() : TESForm() {
	SetFormType(97);
	BGSEncounterZone::InitializeData();
}

BGSEncounterZone::~BGSEncounterZone() {}

void BGSEncounterZone::InitializeData() {
	ourMemset(&Data, 0, sizeof(Data));
	ourMemset(&GameData, 0, sizeof(GameData));
}

// The owner is written as its form ID and restored afterwards.
void BGSEncounterZone::Save() {
	StartForm();
	TESForm* pOwner = Data.pZoneOwner;
	if (pOwner) {
		Data.pZoneOwner = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(pOwner->GetFormID()));
	}
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	TESForm::__AddChunkData(DATA_ID, &Data, 8);
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	Data.pZoneOwner = pOwner;
	CloseForm();
}

// Saves the base form's changes, then the never-reset bit of the flags (one byte) and the game data when they
// changed.
void BGSEncounterZone::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	TESForm::SaveGame(apSaveGameBuffer);
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ENCOUNTER_ZONE_FLAGS)) {
		char cFlags = Data.cFlags & 1;
		apSaveGameBuffer->SaveDataEndian(&cFlags, 1, 0);
	}
	if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ENCOUNTER_ZONE_GAME_DATA)) {
		apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&GameData), 0x10, 0);
	}
}

void BGSEncounterZone::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	uint8_t ctempFlags;
	TESForm::LoadGame(apLoadGameBuffer);
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(2)) {
		ctempFlags = 0;
		apLoadGameBuffer->LoadData(&ctempFlags, 1u);
		bool v2 = ((Data.cFlags & 1) | (Data.cFlags & 0xFFFFFFFEu)) == 0;
		Data.cFlags = Data.cFlags;
	}
	if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ENCOUNTER_ZONE_GAME_DATA)) {
		if (apLoadGameBuffer->GetVersion() < 8u) {
			char v6[8];
			apLoadGameBuffer->LoadData(v6, 8u);
		} else {
			apLoadGameBuffer->LoadData(&GameData, 16u);
		}
	}
}

void BGSEncounterZone::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
	if (apLoadGameBuffer->GetOldChangeFlags().CheckFlags(CHANGE_ENCOUNTER_ZONE_GAME_DATA)) {
		ourMemset(&GameData, 0, sizeof(GameData));
	}
}

// The owner is stored as a form ID while loading: resolve it to a form, warning when it is missing.
void BGSEncounterZone::InitItem() {
	if (!this->GetInitialized()) {
		if (Data.pZoneOwner) {
			// The exe keeps the owner's form ID in the owner field while loading, so the field is punned to the
			// integer the same way Save pokes the ID into it.
			uint32_t uiOwnerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pZoneOwner));
			TESForm::AddCompileIndex(uiOwnerID, this->GetFile(-1));
			TESForm* pOwner = TESForm::GetFormByNumericID(uiOwnerID);
			if (!pOwner) {
				// The exe reads the form ID before the editor ID, which it calls through the vtable (slot 76).
				uint32_t uiFormID = this->GetFormID();
				TESForm* pForm = this;
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("Unable to find owner form (%08X) on encounter zone '%s' (%08X).", uiOwnerID, pEditorID,
					uiFormID);
			}
			Data.pZoneOwner = pOwner;
		}
		this->SetInitialized(true);
	}
}

void BGSEncounterZone::Copy(TESForm* apCopy) {
	BGSEncounterZone* v3 = dynamic_cast<BGSEncounterZone*>(apCopy);
	if (v3) {
		TESForm::CopyAllComponents(v3);
		ourMemcpy(&Data, &v3->Data, sizeof(Data));
	}
}

bool BGSEncounterZone::Compare(TESForm* apCompare) {
	TESForm* v4 =
		static_cast<TESForm*>(FNVD_DYNAMIC_CAST_VOID(BGSEncounterZone)(apCompare, RTTI_TESForm, RTTI_BGSEncounterZone));
	if (!v4) {
		return 1;
	}
	if (CompareAllComponents(v4)) {
		return 1;
	}
	return std::memcmp(&Data, &v4[1], sizeof(Data)) != 0;
}

void BGSEncounterZone::DetermineLevel() {
	uint16_t usCalculatedLevel;
	if (!GameData.sZoneLevel) {
		usCalculatedLevel = GetCalcedZoneLevel();
		SetZoneLevel(usCalculatedLevel);
	}
}

// Change flag 0x80000000 is the zone's game data; it has no name in the PDB or IDA.
void BGSEncounterZone::SetDetachTime(uint32_t aiVal) {
	GameData.iDetachTime = aiVal;
	AddChange(0x80000000);
}

// The exe takes the elapsed hours before calling GetHoursToRespawnCell, and compares them unsigned.
void BGSEncounterZone::SetAttachTime(uint32_t aiVal) {
	GameData.iAttachTime = aiVal;
	if (GameData.iDetachTime) {
		uint32_t uiElapsed = GameData.iAttachTime - GameData.iDetachTime;
		if (uiElapsed > TESObjectCELL::GetHoursToRespawnCell())
			GameData.iResetTime = aiVal;
	}
	AddChange(CHANGE_ENCOUNTER_ZONE_GAME_DATA);
}

void BGSEncounterZone::SetZoneLevel(uint32_t aiVal) {
	GameData.sZoneLevel = static_cast<uint16_t>(aiVal);
	AddChange(0x80000000);
}

int32_t BGSEncounterZone::QOwnerFactionRank() {
	return (int32_t)Data.iOwnerRank;
}

uint16_t BGSEncounterZone::QZoneLevel() {
	if (GameData.sZoneLevel) {
		return GameData.sZoneLevel;
	} else {
		return GetCalcedZoneLevel();
	}
}

bool BGSEncounterZone::QNeverReset() {
	return ((Data.cFlags & 1) != 0);
}

bool BGSEncounterZone::ShouldReset() {
	bool bReset = true;
	if (this != BGSEncounterZone::pNoZone && GameData.sZoneLevel) {
		if (QNeverReset()) {
			bReset = false;
		} else {
			// The exe reads the detach time through a one-field getter (0x7AF430, [this+0x20]).
			uint32_t iDetachTime = GameData.iDetachTime;
			if (iDetachTime) {
				uint32_t iHoursPassed = appCalendar.GetHoursPassed();
				// Unsigned compare (ja).
				if (iHoursPassed - iDetachTime <= static_cast<uint32_t>(TESObjectCELL::GetHoursToRespawnCell())) {
					bReset = false;
				}
			} else {
				bReset = false;
			}
		}
	}
	return bReset;
}

bool BGSEncounterZone::ShouldCellRecalc(TESObjectCELL* apCell) {
	return (bool)((uint8_t)(GameData.iResetTime &&
		(((uint32_t)apCell->GetDetachTime()) < ((uint32_t)((int32_t)GameData.iResetTime)))));
}

void BGSEncounterZone::SetNeverReset(bool abVal) {
	uint8_t ucFlags;
	uint8_t v3;
	if (abVal) {
		Data.cFlags |= 1u;
		ucFlags = Data.cFlags;
	} else {
		Data.cFlags &= (~1u);
		v3 = Data.cFlags;
	}
	uint8_t v4 = Data.cFlags;
}

bool BGSEncounterZone::GetMatchPCBelowMin() {
	return ((Data.cFlags & 2) != 0);
}

uint8_t BGSEncounterZone::QMinLevel() {
	return Data.cMinLevel;
}

// QMinLevel is called out of line, once per use. The game test of e03ae5d6 caught the earlier body, which read the
// level through a bool getter and lost every level above 1.
uint16_t BGSEncounterZone::GetCalcedZoneLevel() {
	uint16_t usLevel = pPlayer->Actor::GetLevel();
	if (usLevel < QMinLevel()) {
		if (!GetMatchPCBelowMin()) {
			usLevel = QMinLevel();
		}
	} else {
		double dMinLevel = QMinLevel();
		double dLevelDifference = usLevel - QMinLevel();
		usLevel = (uint16_t)(int32_t)(fLevelScalingMult.Float() * dLevelDifference + dMinLevel);
	}
	return usLevel;
}

void BGSEncounterZone::CreateNoZone() {
	BGSEncounterZone* v0;
	void* apMemory;
	if (!BGSEncounterZone::pNoZone) {
		apMemory = ourMalloc(sizeof(BGSEncounterZone));
		if (apMemory) {
			v0 = ::new (apMemory) BGSEncounterZone;
		} else {
			v0 = nullptr;
		}
		BGSEncounterZone::pNoZone = v0;
		v0->SetFormID(0x1Eu, (bool)1);
		BGSEncounterZone::pNoZone->SetFormEditorID("NoZoneZone");
		pDataHandler->AddFormToDataHandler(BGSEncounterZone::pNoZone);
	}
}

BGSEncounterZone* BGSEncounterZone::GetNoZone() {
	return BGSEncounterZone::pNoZone;
}

void BGSEncounterZone::ClearNoZone() {
	BGSEncounterZone::pNoZone = nullptr;
}
