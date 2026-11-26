#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

class BGSFormChanges;

// The changes a save game records, keyed by form ID: each form's change flags and its unloaded form buffer.
class BGSSaveLoadChangesMap : public NiTPointerMap<uint32_t, BGSFormChanges*> {
public:
    BGSSaveLoadChangesMap();
    ~BGSSaveLoadChangesMap() override;

    bool RemoveChanges(uint32_t aiFormID);
    // The changes recorded for aiFormID, or null.
    BGSFormChanges* GetChanges(uint32_t aiFormID);
    // aiFormID's change flags, or none when it has no changes. The result is written to apResult, which is returned.
    BGSChangeFlags* GetChangeFlags(BGSChangeFlags* apResult, uint32_t aiFormID);
    // The saved data buffer of aiFormID, or an empty buffer when it has no changes, written to apResult.
    BGSUnloadedFormBuffer* GetChangeBuffer(BGSUnloadedFormBuffer* apResult, uint32_t aiFormID);
    // Whether aiFormID has any of aiChangeFlags recorded.
    bool CheckChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
    // Records aiChangeFlags for aiFormID.
    void AddChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
    // Clears aiChangeFlags of aiFormID; whether its entry went away.
    bool RemoveChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
    // Sets aiFormID's change flags to aiChangeFlags.
    void SetChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags);
    // Records aiChangeFlags and the saved data akBuffer of the
    // unloaded form aiFormID.
    void SetChanges(uint32_t aiFormID, BGSChangeFlags aiChangeFlags, BGSUnloadedFormBuffer akBuffer);
    void RemoveAllChanges();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
};

// The changes map's base.
using NiTMapBaseUIntBGSFormChanges = NiTMapBase<NiTPointerAllocator<size_t>, uint32_t, BGSFormChanges*>;

// What the save game records for one form: which parts changed, and the buffer of its saved data.
class BGSFormChanges {
public:
    BGSFormChanges();
    // Frees the unloaded buffer.
    ~BGSFormChanges();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BGSChangeFlags iChangeFlags;
    BGSUnloadedFormBuffer UnloadedFormBuffer;
};
