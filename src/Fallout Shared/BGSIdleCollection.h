#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESFile.h"

class Actor;
class TESIdleForm;
class TESObjectREFR;

// The idle animations a form (an idle marker, a package) picks from, and how often it checks for one.
class BGSIdleCollection : public BaseFormComponent {
public:
    // How the next idle is picked.
    enum PICK_TYPE : int32_t {
        RANDOM = 0,
        SEQUENCE = 1,
    };

    enum IDLE_FLAGS : uint8_t {
        FLAG_RUN_IN_SEQUENCE = 0x1,
        FLAG_DO_ONCE = 0x4,
    };

    BGSIdleCollection();
    ~BGSIdleCollection();

    void InitializeDataComponent() override;
    void ClearDataComponent() override;
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // Resolves the idles' form IDs once the forms are loaded.
    void InitItem(TESForm* apOwner);
    // Reads one of the idle chunks of a record.
    void Load(TESFile* apFile);
    // Saves the idle chunks: the flags, the count, the check timer and the idles' form IDs.
    void Save();
    // The idle count is stored as a byte and returned zero-extended; callers test it as a signed int (count <= 0).
    int32_t GetIdleCount() const;
    bool GetDoOnce();
    BGSIdleCollection::PICK_TYPE GetPickType();
    void RemoveIdleAtIndex(uint32_t aiIndex);
    bool HasIdleForActor(Actor* apActor, TESObjectREFR* apTargetRef);
    uint32_t
    GetIdlesForActor(Actor* apActor, TESObjectREFR* apTargetRef, BSSimpleArray<TESIdleForm*, 1024>* apOutIdleArray);

    static bool IsValidChunk(CHUNK_ID aeChunk);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    uint8_t cIdleFlags;
    uint8_t cIdleCount;
    TESIdleForm** pIdleArray;
    float fTimerCheckForIdle;
};
