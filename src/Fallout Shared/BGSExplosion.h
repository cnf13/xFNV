#pragma once

#include "Fallout Shared/BGSPreloadable.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESImageSpaceModifiableForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"

class BGSImpactData;
class BGSImpactDataSet;
class TESObjectLIGH;
class TESSound;

enum HavokMaterialType : int32_t;
enum SOUND_LEVEL : int32_t;

// An explosion's DATA: force, damage, radius, light, sounds, flags, image space radius, impact data set, radiation
// and sound level.
struct BGSExplosionData {
    BGSExplosionData();

    // Byte-swaps all thirteen dwords read from a big-endian file.
    void Endian();
    // Zeroes the data and sets the normal sound level.
    void Initialize();

    float fForce;
    float fDamage;
    float fRadius;
    TESObjectLIGH* pLight;
    TESSound* pSound1;
    uint32_t iFlags;
    float fImageSpaceRadius;
    BGSImpactDataSet* pImpactDataSet;
    TESSound* pSound2;
    float fRadiationRads;
    float fRadiationDissipationTime;
    float fRadiationRadius;
    SOUND_LEVEL eSoundLevel;
};

// An explosion form (EXPL).
class BGSExplosion : public TESBoundObject,
                     public TESFullName,
                     public TESModel,
                     public TESEnchantableForm,
                     public BGSPreloadable,
                     public TESImageSpaceModifiableForm {
public:
    BGSExplosion();
    ~BGSExplosion() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    NiAVObject* Clone3D(TESObjectREFR* pRequester, bool deepCopy) override;
    void UnClone3D(TESObjectREFR* apRequester) override;
    void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent) override;

    using TESBoundObject::Clone3D;
    using TESForm::Save;

    float GetDamage();
    // The explosion's radius in game units, scaled unless flag 1 is set.
    float GetRadiusBSUnits();
    // Whether any of the explosion's flag bits aiFlag is set.
    bool GetFlag(int32_t aiFlag);
    double GetDissipationTime();
    double GetISRadius();
    double GetRADRadius();
    // Returns the address of Data.
    BGSExplosionData* GetData();
    TESBoundObject* QImpactPlacedObject();
    void SetImpactPlacedObject(TESBoundObject* apObj);
    // Data.pImpactDataSet.
    BGSImpactDataSet* GetImpactDataSet() {
        return Data.pImpactDataSet;
    }
    // Sets Data.fForce.
    void SetForce(float afValue);
    // The impact data set's impact data for aehkMatType; null without a set.
    BGSImpactData* GetImpactData(HavokMaterialType aehkMatType);
    // The value of the gs_iSoundLevel setting for Data.eSoundLevel; 0 for an unknown level.
    int32_t GetSoundLevelValue();

    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

    TESBoundObject* pImpactPlacedObject;
    BGSExplosionData Data;
};
