#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/TESForm.h"

class TESObjectCELL;

// An encounter zone's DATA: the owner, its faction rank, the level range and the flags.
struct ENCOUNTER_ZONE_DATA {
    enum ENCOUNTER_ZONE_FLAGS : char {
        FLAG_NEVER_RESET = 0x1,
        FLAG_MATCH_PC_BELOW_MIN = 0x2,
    };

    // Swaps the owner's form ID.
    void Endian();

    TESForm* pZoneOwner;
    int8_t iOwnerRank;
    uint8_t cMinLevel;
    // Signed char; it is sign-extended when read.
    char cFlags;
    uint8_t cMaxLevel;
};

// What the game keeps of a zone: when it was detached, attached and may reset, and its level.
struct ENCOUNTER_ZONE_GAME_DATA {
    uint32_t iDetachTime;
    uint32_t iAttachTime;
    uint32_t iResetTime;
    uint16_t sZoneLevel;
};

// An encounter zone form (ECZN).
class BGSEncounterZone : public TESForm {
public:
    BGSEncounterZone();
    ~BGSEncounterZone() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::LoadGame;
    using TESForm::Revert;
    using TESForm::Save;
    using TESForm::SaveGame;

    // Works out the zone's level.
    void DetermineLevel();
    // Stores the game hour aiVal the zone was detached at and marks the game data changed.
    void SetDetachTime(uint32_t aiVal);
    // Stores the game hour aiVal the zone was attached at; when it was detached longer ago than the corpse clearing
    // time, also as its reset time. Marks the game data changed.
    void SetAttachTime(uint32_t aiVal);
    // Stores the zone level aiVal and marks the game data changed.
    void SetZoneLevel(uint32_t aiVal);
    // The zone's owner form.
    TESForm* QOwnerForm();

    // The owner faction rank, sign-extended from its byte.
    int32_t QOwnerFactionRank();

    // The zone's level, else its calculated level.
    uint16_t QZoneLevel();

    // Whether the zone's never-reset flag (bit 0 of the byte at +0x1E) is set.
    bool QNeverReset();

    // Whether the zone's reset time has passed.
    bool ShouldReset();

    // Whether apCell should recalculate for this zone.
    bool ShouldCellRecalc(TESObjectCELL* apCell);
    void SetNeverReset(bool abVal);
    bool GetMatchPCBelowMin();
    uint8_t QMinLevel();
    uint16_t GetCalcedZoneLevel();

    // Creates the "no zone" encounter zone.
    static void CreateNoZone();

    // The "no zone" encounter zone.
    static BGSEncounterZone* GetNoZone();
    static void ClearNoZone();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    ENCOUNTER_ZONE_DATA Data;
    ENCOUNTER_ZONE_GAME_DATA GameData;

    // The shared "no zone" encounter zone.
    static inline BGSEncounterZone* pNoZone;
};

// The game settings BGSEncounterZone.cpp defines.
extern SettingT<GameSettingCollection> fLevelScalingMult;
