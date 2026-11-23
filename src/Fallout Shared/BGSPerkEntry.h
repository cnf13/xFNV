#pragma once

#include "BSCore/MemoryManager.h"

#include <stdint.h>

class Actor;
class BGSEntryPointFunctionData;
class TESFile;
class TESForm;

// The three bytes of the PRKE chunk InitializeFromSaveData reads.
struct PerkEntrySaveData {
    uint8_t cPerkEntryType;
    uint8_t cRank;
    uint8_t cPriority;
};

// The kind of a perk entry, as GetType returns it.
enum PERK_ENTRY_TYPE : int32_t {
    PERK_ENTRY_TYPE_QUEST = 0,
    PERK_ENTRY_TYPE_ABILITY = 1,
    PERK_ENTRY_TYPE_ENTRY_POINT = 2,
    PERK_ENTRY_TYPE_COUNT = 3,
};

// One entry of a perk: what the perk does when it is applied to an actor. The kinds are BGSQuestPerkEntry (sets a
// quest stage), BGSEntryPointPerkEntry (changes a value at an entry point) and BGSAbilityPerkEntry.
class BGSPerkEntry {
public:
    BGSPerkEntry();

    // Whether the filter forms satisfy the entry's conditions; an entry without conditions never does.
    virtual bool CheckConditionFilters(uint32_t aiNumFilterForms, TESForm** apFilterForms) {
        return false;
    }
    virtual uint8_t GetFunction() {
        return 0;
    }
    // Returns void*, which the entry point kind's override returns as well.
    virtual void* GetFunctionData() {
        return nullptr;
    }
    virtual ~BGSPerkEntry();
    virtual PERK_ENTRY_TYPE GetType() = 0;
    virtual void ClearData() {}
    // Resolves the form IDs the entry holds, apOwner being the form that owns the entry.
    virtual void InitItem(TESForm* apOwner) {}
    // A new entry of the same kind holding the same data.
    virtual BGSPerkEntry* Duplicate() = 0;
    // Copies the rank and priority of apSource.
    virtual void Copy(BGSPerkEntry* apCopy);
    // true if apCompare differs in rank or priority.
    virtual bool Compare(BGSPerkEntry* apCompare);
    virtual void Save() {}
    virtual bool Load(TESFile* apFile) {
        return true;
    }
    virtual void ApplyPerkEntry(Actor* apActor, bool aucRank) = 0;
    virtual void RemovePerkEntry(Actor* apActor, bool aucRank) = 0;

    // Writes the PRKE chunk: the type, the entry's rank and its priority; only the type's low byte is stored.
    void StartPerkEntry(PERK_ENTRY_TYPE aePerkEntryType);
    // Writes the entry's closing PRKF chunk.
    void FinishPerkEntry();
    uint8_t GetRank();
    uint8_t GetPriority();
    // Reads the entry's saved data, the three bytes of the PRKE chunk.
    void InitializeFromSaveData(PerkEntrySaveData* apSaveData);

    // A new entry of the given kind, or null for an unknown kind.
    static BGSPerkEntry* CreatePerkEntryOfType(PERK_ENTRY_TYPE aePerkEntryType);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    uint8_t cRank;
    uint8_t cPriority;
};
