#pragma once

#include "Fallout Shared/TESForm.h"

class Actor;
class BGSLoadFormBuffer;
class MobileObject;
class TESBoundObject;
class TESObjectCELL;
class TESObjectREFR;

// what the location entry of a package points at
enum PACK_LOC_TYPE : int32_t {
    PLT_NONE = -1,
    PLT_REFERENCE = 0,
    PLT_CELL = 1,
    PLT_CURRLOC = 2,
    PLT_EDITORLOC = 3,
    PLT_OBJECT = 4,
    PLT_TYPE = 5,
    PLT_LINKEDREF = 6,
    PLT_FIRSTLOC = 7
};

// category of object a package asks the actor to use
enum PACKAGE_OBJECT_TYPE : int32_t {
    POTYPE_NONE = 0,
    POTYPE_ACTI = 1,
    POTYPE_ARMO = 2,
    POTYPE_BOOK = 3,
    POTYPE_CLOT = 4,
    POTYPE_CONT = 5,
    POTYPE_DOOR = 6,
    POTYPE_INGR = 7,
    POTYPE_LIGH = 8,
    POTYPE_MISC = 9,
    POTYPE_FLOR = 10,
    POTYPE_FURN = 11,
    POTYPE_WEAP = 12,
    POTYPE_AMMO = 13,
    POTYPE_NPC = 14,
    POTYPE_CREA = 15,
    POTYPE_KEYM = 16,
    POTYPE_ALCH = 17,
    POTYPE_FOOD = 18,
    POTYPE_COMBATWEAR = 19,
    POTYPE_WEAR = 20,
    POTYPE_WEAP_NONE = 21,
    POTYPE_WEAP_MELEE = 22,
    POTYPE_WEAP_RANGED = 23,
    POTYPE_SPELL = 24,
    POTYPE_SPELL_RANGED = 25,
    POTYPE_SPELL_TOUCH = 26,
    POTYPE_SPELL_SELF = 27,
    POTYPE_ACTOR = 28,
    POTYPE_COUNT = 29
};

// a package location or target object; the meaning depends on the
// location/target type stored next to it
union PackageObject {
    TESObjectREFR* pRef;
    TESObjectCELL* pCell;
    TESForm* pObject;
    TESForm* pLocForm;
    PACKAGE_OBJECT_TYPE eObjType;
    // The form ID a saved game stores here until the object is resolved: PackageLocation::LoadGame and
    // PackageTarget::LoadGame pass it to BGSLoadGameBuffer::LoadFormID.
    uint32_t iFormID;
};

// A package location as a plugin file stores it: the object is a form ID until the package's InitItem resolves it.
struct PACK_LOC_FILE {
    // Byte-swaps the form ID and the radius.
    void Endian();

    char eLocType;
    uint32_t iFormID;
    uint32_t iRadius;
};

// location entry of a package (where the actor should travel to)
class PackageLocation {
public:
    PackageLocation();
    // The location read from a file.
    PackageLocation(const PACK_LOC_FILE* apPLFile);
    // Resets the type, the radius and the object.
    ~PackageLocation();

    PACK_LOC_TYPE GetLocType() const;
    TESObjectREFR* GetLocReference() const;
    TESObjectCELL* GetLocCell() const;
    TESForm* GetLocObject() const;
    PACKAGE_OBJECT_TYPE GetLocObjectType() const;
    uint32_t GetRadius() const;
    TESObjectREFR* GetRunningPackageLocReference(MobileObject* apActor) const;
    bool IsRefAtLocationTrigger(MobileObject* apPackOwner, TESObjectREFR* apRef, bool& abRefInsideTriggerOut) const;

    // Writes the location to a save game: the type byte, the radius and the object.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Reads the location from a save game.
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Reads the location from an old-format save through pSaveLoadGameOLD.
    void LoadGame();
    void SetLocType(PACK_LOC_TYPE aeLoctype);
    void SetLocReference(TESObjectREFR* apRef);
    void SetLocCell(TESObjectCELL* apCell);
    void SetLocObject(TESForm* apObj);
    void SetLocObjectType(PACKAGE_OBJECT_TYPE aeType);
    void SetRadius(uint32_t aiRad);
    void Copy(PackageLocation* apCopy, bool abNoneOK);
    // Resolves the location's form ID once the forms are loaded.
    void InitItem(TESForm* apOwner);
    bool IsLocationGeneric();
    bool IsRadiusNonZero();
    uint16_t GetSaveSize();
    void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // The TES-file variant: resolves the form ID read from a file.
    void InitLoadGame();
    // Writes the location as chunk aeChunk of the record being saved.
    void Save(CHUNK_ID aeChunk);
    bool Compare(PackageLocation* apCompare);
    void SaveGame();

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    // 0x0 location type, PACK_LOC_TYPE truncated to a byte
    uint8_t eLocType;
    // 0x1
    uint8_t padding01[3];
    // 0x4 search radius for the location
    uint32_t iRad;
    // 0x8 location object; meaning depends on eLocType
    PackageObject uObject;
};
