#pragma once

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"

class TESObjectREFR;
class TESWaterForm;
class TESWaterListener;

// A placeable water's DATA: its flags and the water type it uses.
struct BGSPlaceableWaterData {
    BGSPlaceableWaterData();

    void Clear();
    // Swaps both dwords, the water type then holding a form ID.
    void Endian();
    // Initialiser of a DATA record: Load calls it on its stack copy before LoadData fills it. The other initialiser is
    // Construct above.
    void Initialize();
    // Swaps the 8 bytes of the DATA record read from a big-endian file, which Load calls on its local copy. Save uses
    // EXTERIOR_DATA::Endian above instead.
    void EndianFromFile();

    // Initialize and the big-endian load (EndianFromFile) touch only the low 16 bits of the flags; Clear and Endian
    // touch all 32 bits.
    union {
        uint32_t ulFlags;
        uint16_t usFlags;
    };

    TESWaterForm* pWaterType;
};

// A placeable water form (PWAT).
class BGSPlaceableWater : public TESBoundObject, public TESModel {
public:
    enum BGSPlaceableWaterFlags : uint32_t {
        REFLECTION_ENABLED = 0x1,
        REFLECTS_ACTORS = 0x2,
        REFLECTS_LAND = 0x4,
        REFLECTS_LOD_LAND = 0x8,
        REFLECTS_LOD_BUILDINGS = 0x10,
        REFLECTS_LOD_TREES = 0x20,
        REFLECTS_SKY = 0x40,
        REFLECTS_NO_REFERENCE = 0x80,
        REFLECTS_DEAD_ACTORS = 0x100,
        REFRACTION_ENABLED = 0x200,
        REFRACTS_ACTORS = 0x400,
        REFRACTS_LAND = 0x800,
        REFRACTS_NO_REFERENCE = 0x10000,
        REFRACTS_DEAD_ACTORS = 0x20000,
        SILHOUETTE_REFLECTIONS = 0x40000,
        LOD_WATER = 0x8000000,
        ENABLE_DEPTH = 0x10000000,
        ENABLE_OBJECT_TEXCOORD = 0x20000000,
        AUTOMATIC_WATER = 0x40000000,
        DISABLE_FOGGING_PASS = 0x80000000,
    };

    BGSPlaceableWater();
    ~BGSPlaceableWater() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    bool Activate(
        TESObjectREFR* apItemActivated,
        TESObjectREFR* apActionRef,
        bool abIdFlag,
        TESBoundObject* apObjecttoGet,
        int32_t aiCount
    ) override;
    TESWaterForm* GetWaterType() override;
    using TESForm::Save;

    bool IsAutomaticWater();
    bool IsLODWater();
    // Sets flag aeFlag to abValue.
    void SetFlag(BGSPlaceableWaterFlags aeFlag, bool abValue);
    // Turns the underwater fog pass on or off; flag 0x80000000 is set while it is off.
    void EnableUnderwaterFoggingPass(bool abValue);
    bool GetFlag(uint32_t aiValue);
    bool GetRefractionEnabled() {
        return GetFlag(REFRACTION_ENABLED);
    }
    bool GetRefractsLand() {
        return GetFlag(REFRACTS_LAND);
    }
    bool GetSilhouetteReflections() {
        return GetFlag(SILHOUETTE_REFLECTIONS);
    }
    bool GetReflectionEnabled() {
        return GetFlag(REFLECTION_ENABLED);
    }
    bool GetReflectsLand() {
        return GetFlag(REFLECTS_LAND);
    }
    bool GetReflectsLODLand() {
        return GetFlag(REFLECTS_LOD_LAND);
    }
    bool GetReflectsLODBuildings() {
        return GetFlag(REFLECTS_LOD_BUILDINGS);
    }
    bool GetReflectsLODTrees() {
        return GetFlag(REFLECTS_LOD_TREES);
    }
    bool GetReflectsSky() {
        return GetFlag(REFLECTS_SKY);
    }
    bool GetRefractsActors() {
        return GetFlag(REFRACTS_ACTORS);
    }
    bool IsDepthEnabled();
    void SetReflectionEnabled(bool abValue);
    void SetReflectsLand(bool abValue);
    void SetReflectsLODLand(bool abValue);
    void SetReflectsSky(bool abValue);
    void SetRefractionEnabled(bool abValue);
    void SetRefractsLand(bool abValue);
    void SetAutomaticWater(bool abValue);
    void SetObjectTexCoordsEnabled(bool abValue);
    void SetDepthEnabled(bool abValue);
    void SetReflectsLODBuildings(bool abValue);
    void SetReflectsLODTrees(bool abValue);
    void SetRefractsActors(bool abValue);
    void SetReflectsActors(bool abValue);
    void SetReflectsNoReference(bool abValue);
    void SetReflectsDeadActors(bool abValue);
    void SetLODWater(bool abValue);
    int32_t GetUnderwaterFog();
    bool GetObjectTexCoordsEnabled();

    // The water listener of the world of apRef's parent cell, or null.
    static TESWaterListener* GetWaterListener(TESObjectREFR* apRef);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    BGSPlaceableWaterData Data;
};
