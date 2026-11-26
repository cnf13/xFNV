#include "Fallout Shared/BGSEncounterZone.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Settings/XGameSetting.h"
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
#include "xfnv/Compiler.h"

// The game settings this file defines, with their IDs and values, in initialization order.
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
            // The file's record is 8 bytes, the owner's 4-byte form ID first.
            TESFile::WidenFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(ENCOUNTER_ZONE_DATA, pZoneOwner)}});
            if (apFile->QEndian()) {
                Data.Endian();
            }
        }
        else if (eChunk == EDID_ID) {
            // Allocates the chunk's size on the stack and reads at most 0x200 bytes.
            char* pEditorID = static_cast<char*>(XFNV_ALLOCA(apFile->GetChunkSize()));
            apFile->GetChunkData(pEditorID, 0x200u);
            // Calls the form's own override through the vtable.
            TESForm* pForm = this;
            pForm->SetFormEditorID(pEditorID);
        }
        else if (eChunk == OBND_ID) {
            // Calls the form's own override through the vtable.
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
    BSEndian(pZoneOwner, false);
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
    // The file's record is Load's 8 bytes, the owner's 4-byte form ID first.
    TESFile::NarrowFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(ENCOUNTER_ZONE_DATA, pZoneOwner)}});
    TESForm::__AddChunkData(DATA_ID, &Data, 8);
    TESFile::WidenFormIDRecords(&Data, 1, 8, sizeof(Data), {{0, offsetof(ENCOUNTER_ZONE_DATA, pZoneOwner)}});
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
        char cFlags = Data.cFlags & ENCOUNTER_ZONE_DATA::FLAG_NEVER_RESET;
        apSaveGameBuffer->SaveDataEndian(&cFlags, sizeof(cFlags), 0);
    }
    if (apSaveGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ENCOUNTER_ZONE_GAME_DATA)) {
        apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&GameData), sizeof(GameData), 0);
    }
}

void BGSEncounterZone::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    uint8_t ctempFlags;
    TESForm::LoadGame(apLoadGameBuffer);
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ENCOUNTER_ZONE_FLAGS)) {
        ctempFlags = 0;
        apLoadGameBuffer->LoadData(&ctempFlags, sizeof(ctempFlags));
        Data.cFlags = Data.cFlags;
    }
    if (apLoadGameBuffer->GetChangeFlags().CheckFlags(CHANGE_ENCOUNTER_ZONE_GAME_DATA)) {
        if (apLoadGameBuffer->GetVersion() < 8u) {
            char acOldGameData[8];
            apLoadGameBuffer->LoadData(acOldGameData, sizeof(acOldGameData));
        }
        else {
            apLoadGameBuffer->LoadData(&GameData, sizeof(GameData));
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
            // While loading, the owner field holds the owner's form ID, so the field is punned to the integer the same
            // way Save pokes the ID into it.
            uint32_t uiOwnerID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(Data.pZoneOwner));
            TESForm::AddCompileIndex(uiOwnerID, this->GetFile(-1));
            TESForm* pOwner = TESForm::GetFormByNumericID(uiOwnerID);
            if (!pOwner) {
                // Reads the form ID before the editor ID, which is called through the vtable.
                uint32_t uiFormID = this->GetFormID();
                TESForm* pForm = this;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "Unable to find owner form (%08X) on encounter zone '%s' (%08X).",
                    uiOwnerID,
                    pEditorID,
                    uiFormID
                );
            }
            Data.pZoneOwner = pOwner;
        }
        this->SetInitialized(true);
    }
}

void BGSEncounterZone::Copy(TESForm* apCopy) {
    BGSEncounterZone* pCopy = dynamic_cast<BGSEncounterZone*>(apCopy);
    if (pCopy) {
        TESForm::CopyAllComponents(pCopy);
        ourMemcpy(&Data, &pCopy->Data, sizeof(Data));
    }
}

bool BGSEncounterZone::Compare(TESForm* apCompare) {
    BGSEncounterZone* pZone = dynamic_cast<BGSEncounterZone*>(apCompare);
    if (!pZone) {
        return true;
    }
    if (CompareAllComponents(pZone)) {
        return true;
    }
    return std::memcmp(&Data, &pZone->Data, sizeof(Data)) != 0;
}

void BGSEncounterZone::DetermineLevel() {
    if (!GameData.sZoneLevel) {
        uint16_t usCalculatedLevel = GetCalcedZoneLevel();
        SetZoneLevel(usCalculatedLevel);
    }
}

void BGSEncounterZone::SetDetachTime(uint32_t aiVal) {
    GameData.iDetachTime = aiVal;
    AddChange(CHANGE_ENCOUNTER_ZONE_GAME_DATA);
}

// Takes the elapsed hours before calling GetHoursToRespawnCell, and compares them unsigned.
void BGSEncounterZone::SetAttachTime(uint32_t aiVal) {
    GameData.iAttachTime = aiVal;
    if (GameData.iDetachTime) {
        uint32_t uiElapsed = GameData.iAttachTime - GameData.iDetachTime;
        if (uiElapsed > TESObjectCELL::GetHoursToRespawnCell()) {
            GameData.iResetTime = aiVal;
        }
    }
    AddChange(CHANGE_ENCOUNTER_ZONE_GAME_DATA);
}

void BGSEncounterZone::SetZoneLevel(uint32_t aiVal) {
    GameData.sZoneLevel = static_cast<uint16_t>(aiVal);
    AddChange(CHANGE_ENCOUNTER_ZONE_GAME_DATA);
}

int32_t BGSEncounterZone::QOwnerFactionRank() {
    return Data.iOwnerRank;
}

uint16_t BGSEncounterZone::QZoneLevel() {
    if (GameData.sZoneLevel) {
        return GameData.sZoneLevel;
    }

    return GetCalcedZoneLevel();
}

bool BGSEncounterZone::QNeverReset() {
    return (Data.cFlags & ENCOUNTER_ZONE_DATA::FLAG_NEVER_RESET) != 0;
}

bool BGSEncounterZone::ShouldReset() {
    bool bReset = true;
    if (this != BGSEncounterZone::pNoZone && GameData.sZoneLevel) {
        if (QNeverReset()) {
            bReset = false;
        }
        else {
            uint32_t uiDetachTime = GameData.iDetachTime;
            if (uiDetachTime) {
                uint32_t uiHoursPassed = appCalendar.GetHoursPassed();
                // The compare is unsigned.
                if (uiHoursPassed - uiDetachTime <= TESObjectCELL::GetHoursToRespawnCell()) {
                    bReset = false;
                }
            }
            else {
                bReset = false;
            }
        }
    }
    return bReset;
}

bool BGSEncounterZone::ShouldCellRecalc(TESObjectCELL* apCell) {
    return GameData.iResetTime && apCell->GetDetachTime() < GameData.iResetTime;
}

void BGSEncounterZone::SetNeverReset(bool abVal) {
    if (abVal) {
        Data.cFlags |= ENCOUNTER_ZONE_DATA::FLAG_NEVER_RESET;
    }
    else {
        Data.cFlags &= ~ENCOUNTER_ZONE_DATA::FLAG_NEVER_RESET;
    }
}

bool BGSEncounterZone::GetMatchPCBelowMin() {
    return (Data.cFlags & ENCOUNTER_ZONE_DATA::FLAG_MATCH_PC_BELOW_MIN) != 0;
}

uint8_t BGSEncounterZone::QMinLevel() {
    return Data.cMinLevel;
}

uint16_t BGSEncounterZone::GetCalcedZoneLevel() {
    uint16_t usLevel = pPlayer->Actor::GetLevel();
    if (usLevel < QMinLevel()) {
        if (!GetMatchPCBelowMin()) {
            usLevel = QMinLevel();
        }
    }
    else {
        double dMinLevel = QMinLevel();
        double dLevelDifference = usLevel - QMinLevel();
        usLevel = static_cast<uint16_t>(static_cast<int32_t>(fLevelScalingMult.Float() * dLevelDifference + dMinLevel));
    }
    return usLevel;
}

void BGSEncounterZone::CreateNoZone() {
    if (!BGSEncounterZone::pNoZone) {
        BGSEncounterZone* pZone = new BGSEncounterZone;
        BGSEncounterZone::pNoZone = pZone;
        pZone->SetFormID(0x1Eu, true);
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
