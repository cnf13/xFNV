#pragma once

#include "Fallout Shared/BGSPerkEntry.h"

class SpellItem;

// A perk entry that gives the actor a spell (an ability) while the perk applies.
class BGSAbilityPerkEntry : public BGSPerkEntry {
public:
    BGSAbilityPerkEntry();
    ~BGSAbilityPerkEntry() override;

    PERK_ENTRY_TYPE GetType() override;
    void InitItem(TESForm* apOwner) override;
    BGSPerkEntry* Duplicate() override;
    bool Compare(BGSPerkEntry* apCompare) override;
    void Save() override;
    bool Load(TESFile* apFile) override;
    void ApplyPerkEntry(Actor* apActor, bool bAlt) override;
    void RemovePerkEntry(Actor* apActor, bool bAlt) override;

    SpellItem* pAbility;
};
