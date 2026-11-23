#pragma once

#include "Fallout Shared/PackageLocation.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObjectREFR;

// what the target entry of a package points at
enum PACK_TARG_TYPE : int32_t { PTT_NONE = -1, PTT_REFERENCE = 0, PTT_OBJECT = 1, PTT_TYPE = 2, PTT_LINKEDREF = 3 };

// A package target as a plugin file stores it: the object is a form ID until the package's InitItem resolves it.
struct PACK_TARG_FILE {
    // Byte-swaps the form ID, the value and the radius.
    void Endian();

    uint8_t eTargType;
    uint32_t iFormID;
    int32_t iValue;
    float fAcquireRadius;
};

// target entry of a package (what the actor should act upon)
class PackageTarget {
public:
    PackageTarget();
    // The target read from a file.
    PackageTarget(const PACK_TARG_FILE* apPTFile);

    PACK_TARG_TYPE GetTargType() const;
    TESObjectREFR* GetTargReference() const;
    TESForm* GetTargObject() const;
    PACKAGE_OBJECT_TYPE GetTargObjectType() const;
    float GetTargAcquireRadius() const {
        return fAcquireRadius;
    }

    void SetTargType(PACK_TARG_TYPE aeType);
    void SetTargReference(TESObjectREFR* apRef);
    void SetTargObject(TESForm* apObj);
    void SetTargObjectType(PACKAGE_OBJECT_TYPE aeType);
    void SetTargAcquireRadius(float afAcquireRadius);
    void Copy(PackageTarget* apCopy);
    // Resolves the target's form ID once the forms are loaded.
    void InitItem(TESForm* apOwner);
    // The bytes the target writes to a save.
    uint16_t GetSaveSize();
    // Resolves the saved form ID once the save is loaded.
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Resolves the target's form ID after a TES load.
    void InitLoadGame();
    // Reads the target from a save game.
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Reads the target from an old-format save through pSaveLoadGameOLD.
    void LoadGame();
    // Writes the type, iValue and fAcquireRadius, then the form ID (types 0 and 1) or the value (type 2) to the save.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    bool Compare(PackageTarget* apCompare);
    void Save(CHUNK_ID aeChunk);
    void SaveGame();

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    // 0x0 target type, PACK_TARG_TYPE truncated to a byte
    uint8_t eTargType;
    // 0x4 target object; meaning depends on eTargType
    PackageObject uObject;
    // 0x8 target value used by some target types
    int32_t iValue;
    // 0xC radius around the target to acquire it within
    float fAcquireRadius;
};
