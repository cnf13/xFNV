#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "SDK/MSVC/ehvec.h"

class Actor;
class BGSPerkEntry;
class TESObjectREFR;

template <class T>
struct ListBoxItem;

// A perk's DATA: trait, level, ranks, playable and hidden.
struct PerkData {
    bool bTrait;
    uint8_t cLevel;
    uint8_t cNumRanks;
    bool bPlayable;
    bool bHidden;
};

// A perk form (PERK).
class BGSPerk : public TESForm, public TESFullName, public TESDescription, public TESIcon {
public:
    BGSPerk();
    ~BGSPerk() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    // Whether the conditions of the perk hold for apActionRef.
    bool IsPerkAvailable(TESObjectREFR* apActionRef);
    uint8_t GetNumRanks();
    char GetLevel();
    bool GetIsTrait();
    bool IsPerkHidden();
    // The first perk entry of rank acRank (entries count ranks from 0), or null; the scan stops at an empty item.
    BGSPerkEntry* GetPerkEntryByRank(uint8_t acRank);
    // Adds an entry to the perk's entry list.
    bool AddPerkEntry(BGSPerkEntry* apPerkEntry);
    void AddPerkEntryPoints(Actor* apActor, uint8_t acRank, bool bAlt);
    void RemovePerk(Actor* apActor, bool bAlt);
    bool GetIsPlayable();
    void ApplyPerk(Actor* apActor, uint8_t acOldRank, uint8_t acNewRank, bool bAlt);
    bool IsPerkAttainable(TESObjectREFR* apActionRef);
    void BuildDescriptionString(char* apOutText, uint32_t aiOutSize);

    // Sort order of perk entries: higher priority first, then lower rank first (list sort callbacks).
    static int32_t PerkEntryPrioritySortFunction(BGSPerkEntry* apPerkEntry1, BGSPerkEntry* apPerkEntry2);
    // Sort order of perk entries: lower rank first, then higher priority, then lower type (a list sort callback).
    static int32_t PerkEntryRankSortFunction(BGSPerkEntry* apPerkEntry1, BGSPerkEntry* apPerkEntry2);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // The comparison of the perk list's items: their alpha, then the perks' levels, then the names. Reads the level-up
    // menu's trait alpha.
    static int32_t CompareForSorting(ListBoxItem<BGSPerk*>* apItem1, ListBoxItem<BGSPerk*>* apItem2);

    PerkData Data;
    TESCondition PerkConditions;
    BSSimpleList<BGSPerkEntry*> PerkEntries;
};
