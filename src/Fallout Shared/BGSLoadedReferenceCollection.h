#pragma once

#include "Fallout Shared/LoadedReferenceMap.h"
#include "Fallout Shared/TESForm.h"

class FindObjectIterator;
class ItemChange;
class TESObjectREFR;

// The loaded references sorted into per-kind maps (items, weapons, actors, teleport doors, ...).
class BGSLoadedReferenceCollection {
public:
    enum FIND_FLAGS : uint32_t {
        FIND_IN_CONTAINERS = 0x1,
        FIND_ON_DEAD_ACTORS = 0x2,
        FIND_ON_LIVING_ACTORS = 0x4,
        FIND_OBSTACLES = 0x8,
    };

    BGSLoadedReferenceCollection();
    ~BGSLoadedReferenceCollection();

    // The map of the references of form type aeFormType.
    LoadedReferenceMap* GetFormTypeMap(ENUM_FORM_ID aeFormType);
    // Files apRef in the maps of its kind.
    void AddReference(TESObjectREFR* apRef);
    // Drops apRef from the maps.
    void RemoveReference(TESObjectREFR* apRef);
    bool GetNextObject(
        FindObjectIterator& arIterator,
        TESObjectREFR*& arpRef,
        ItemChange*& arpItemChange,
        uint32_t aiFindFlags
    );

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Deletes the game's collection and clears the pointer.
    static void Destroy();
    static void Create();

    LoadedReferenceMap DynamicReferenceMap;
    LoadedReferenceMap ItemsMap;
    LoadedReferenceMap WeaponsMap;
    LoadedReferenceMap AmmoMap;
    LoadedReferenceMap ContainersMap;
    LoadedReferenceMap ActorsMap;
    LoadedReferenceMap RadiationMap;
    LoadedReferenceMap TeleportDoorMap;
};

// The game's collection, set by Create.
inline BGSLoadedReferenceCollection* pLoadedReferenceCollection;
