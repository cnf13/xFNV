#pragma once

#include "Fallout Shared/Pathfinding/TeleportPath.h"
#include "Fallout Shared/TESCondition.h"

class TESObjectREFR;

// A quest target's QSTA chunk as a file stores it: the target reference's 4-byte slot, then the flags byte.
struct QSTA_DATA {
    // Byte-swaps the reference read from a big-endian file.
    void Endian();

    uint32_t uiTargetRef;
    uint8_t cFlags;
};

// A reference a quest objective points the player at.
class TESQuestTarget {
public:
    enum QUEST_TARGET_FLAGS : uint8_t {
        FLAG_IGNORE_LOCKS = 0x1,
    };

    TESQuestTarget();
    ~TESQuestTarget();

    // The target reference; with abAllowPickUpActor, a deleted (picked-up) target resolves to
    // the reference its extra data points at.
    TESObjectREFR* GetReference(bool abAllowPickUpActor);
    bool GetIgnoreLocks();
    // Reads the target's QSTA chunk: the reference and the flags byte.
    void Load(TESFile* apFile);
    TESCondition* GetConditionList() {
        return &objConditions;
    }
    // Resolves the target's form IDs for apOwner.
    void InitItem(TESForm* apOwner);
    // The reference the target's path leads to: the first door of m_TargetPath when it has
    // one, else m_pTargetRef.
    TESObjectREFR* QTrackingRef();
    // Returns &m_TargetPath.
    TeleportPath* QTargetPath();

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    uint8_t cFlags;
    TESCondition objConditions;
    TESObjectREFR* m_pTargetRef;
    TeleportPath m_TargetPath;
};
