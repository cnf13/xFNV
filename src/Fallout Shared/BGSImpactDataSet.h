#pragma once

#include "BSCore/BSCoreDefines.h"
#include "BSCore/BSMap.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/AI/Combat/Projectile.h"

class BGSImpactData;
class ImpactSwap;

// An impact data set form (IPDS). Its data members are not declared yet.
class BGSImpactDataSet : public TESForm, public BGSPreloadable {
public:
    enum IMPACT_MATERIAL_TYPE : int32_t {
        IMP_MAT_STONE = 0,
        IMP_MAT_DIRT = 1,
        IMP_MAT_GRASS = 2,
        IMP_MAT_GLASS = 3,
        IMP_MAT_METAL = 4,
        IMP_MAT_WOOD = 5,
        IMP_MAT_ORGANIC = 6,
        IMP_MAT_CLOTH = 7,
        IMP_MAT_WATER = 8,
        IMP_MAT_HOLLOW_METAL = 9,
        IMP_MAT_ORGANIC_BUG = 10,
        IMP_MAT_ORGANIC_GLOW = 11,
        IMP_MAT_COUNT = 12,
    };

    BGSImpactDataSet();
    ~BGSImpactDataSet() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) override;

    using TESForm::Save;

    // The impact data for aeMaterial.
    BGSImpactData* GetImpactDataForMaterial(int32_t aeMaterial);
    BGSImpactData* GetImpactData(BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeType);

    // Makes apSwap the impact swap in effect.
    static void ApplyImpactSwap(ImpactSwap* apSwap);
    static void CreateDefaultImpactDataSet();
    static void ClearDefaultImpactDataSet();
    static BGSImpactDataSet* GetDefaultImpactDataSet();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static BGSImpactDataSet::IMPACT_MATERIAL_TYPE GetImpactMaterialType(HavokMaterialType aehkMatType);

    // The impact data of every material.
    BGSImpactData* ppImpactData[IMP_MAT_COUNT];

    static inline BGSImpactDataSet* pDefaultImpactDataSet;
    // The impact swap currently being applied to the cell.
    static inline ImpactSwap* pActiveImpactSwap;
};

// A cell's or worldspace's remapping of impact data sets and footstep materials.
class ImpactSwap {
public:
    enum FST_MATERIALS : int32_t {
        FST_STONE = 0,
        FST_CONCRETE_BROKEN = 1,
        FST_METAL = 2,
        FST_METAL_HOLLOW = 3,
        FST_METAL_SHEET = 4,
        FST_WOOD = 5,
        FST_SAND = 6,
        FST_DIRT = 7,
        FST_GRASS = 8,
        FST_WATER = 9,
        FST_MAT_TOTAL = 10,
        FST_INVALID = 10,
    };

    // One swap chunk (IPDS's SPMI): the material it remaps, the impact data it replaces and its replacement.
    struct IMPACT_SWAP_DATA {
        // Swaps the three dwords read from a big-endian file.
        void Endian();

        BGSImpactDataSet::IMPACT_MATERIAL_TYPE eType;
        BGSImpactData* pOldData;
        BGSImpactData* pNewData;
    };

    ImpactSwap();
    ~ImpactSwap();

    BGSImpactData* GetSwappedImpactData(BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeType, BGSImpactData* apOrigImpactData);
    // The alternate footstep material name of aeType, or null when its first character is 0.
    const char* GetAltFootstepMaterialName(FST_MATERIALS aeType);
    // Resolves the swapped forms' IDs for apOwner.
    void InitItem(TESForm* apOwner);
    // Copies apSwap's swaps into this one.
    void Copy(ImpactSwap* apSwap);

    // Reads one swap chunk of apFile.
    void Load(TESFile* apFile);
    // Writes each swap as an IMPS chunk of form IDs, then the IMPF footstep names.
    void Save();
    // Whether apSwap differs from this swap.
    bool Compare(ImpactSwap* apSwap);
    // Maps apOrigImpactData to apNewImpactData for aeType, creating the material's map first. Both are passed to the
    // map's SetAt as key and value.
    void AddSwappedImpactData(
        BGSImpactDataSet::IMPACT_MATERIAL_TYPE aeType,
        BGSImpactData* apOrigImpactData,
        BGSImpactData* apNewImpactData
    );

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    // The remapped impact data of each material (AddSwappedImpactData constructs a BSMap), and the alternate footstep
    // material names.
    BSMap<BGSImpactData*, BGSImpactData*>* pRemappedData[BGSImpactDataSet::IMP_MAT_COUNT];
    char pAltFootstepMaterialNames[10][30];
};
