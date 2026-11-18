#pragma once

#include "BSCore/BSStringT.h"
#include "BSMain/IOManager.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelPSA.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class BGSDebris;
class BGSExplosion;
class BGSImpactDataSet;
class NiAVObject;
class QueuedFile;
class TESForm;

// A body part's data (BPND): damage, explosion and dismemberment settings.
struct PART_DATA {
    PART_DATA();

    // Swaps the float fields read from a big-endian file; the form pointers still hold IDs and are not swapped.
    void Endian();
    // Sets the default values: health percent 5, damage mult 1, no actor value (-1), to-hit chance 100, the scales 1,
    // the rest 0.
    void Initialize();

    float fDamageMult;
    uint8_t cFlags;
    uint8_t cType;
    uint8_t cHealthPercent;
    char cActorValue;
    uint8_t cToHitChance;
    uint8_t cExplosionChance;
    uint8_t cExplosionGenericDebrisCount;
    uint8_t padding0B;
    BGSDebris* pExplosionGenericDebris;
    BGSExplosion* pExplosion;
    float fTrackingMaxAngle;
    float fExplosionGenericDebrisScale;
    uint8_t cDismemberGenericDebrisCount;
    uint8_t padding1D[3];
    BGSDebris* pDismemberGenericDebris;
    BGSExplosion* pDismemberExplosion;
    float fDismemberGenericDebrisScale;
    NiPoint3 kGoreTranslate;
    NiPoint3 kGoreRotate;
    BGSImpactDataSet* pDismemberImpactDataSet;
    BGSImpactDataSet* pExplosionImpactDataSet;
    uint8_t cDismemberDecalCount;
    uint8_t cExplosionDecalCount;
    uint8_t padding4E[2];
    float fExplosionSpecialDebrisScale;
};

// One body part of a body part data form. Only part of the class is declared. The class has no base and one virtual,
// QueueFiles.
class BGSBodyPart {
public:
    // The body part slots; the enumerators are not needed yet.
    enum LIMB_ENUM : int32_t;

    BGSBodyPart();
    ~BGSBodyPart();

    // Slot 0: queues the part's limb replacement model and its explosions', debris' and
    // impact data sets' files at aePriority under apParent.
    virtual void QueueFiles(IO_TASK_PRIORITY aePriority, QueuedFile* apParent);

    // Resolves the part's explosion, debris and impact data set references against the
    // owning form's file.
    void InitItem(TESForm* apParentForm);
    // The part's BODY_PART_TYPE, Data.ucType.
    LIMB_ENUM GetPartType();

    // Whether the part is an IK biped part (flag 4 of the part's flags byte).
    bool IsIKBiped();
    // Whether the part does head tracking (flag 0x20 of the part's flags byte).
    bool DoesHeadtracking();
    // The head tracking's maximum angle in degrees.
    float GetTrackingMaxAngle();
    // The name of the node the part's IK chain starts at.
    const char* GetIKStartNodeName();
    // The actor value whose damage dismembers the part.
    char GetActorValue();
    uint8_t GetHealthPercent();
    // Data.cToHitChance.
    uint8_t GetToHitChance() {
        return Data.cToHitChance;
    }
    bool IsExplodeChanceAbsolute();
    uint8_t GetExplosionChance();
    void InitializeData();
    void ClearData();
    bool Load(TESFile* apFile);
    bool IsIKData();
    uint8_t GetDebrisCount();
    uint8_t GetSeveredDecalCount();
    bool IsExplodablePart();
    bool IsSeverablePart();
    const char* GetPartName();
    // Returns Data.pDismemberImpactDataSet.
    BGSImpactDataSet* GetDismemberImpactDataSet();
    NiPoint3 GetGoreTranslate();
    const char* GetGoreObjectName();
    NiPoint3 GetGoreRotate();
    const char* GetTargetName();
    const char* GetNodeName();

    // Unhides the limb's geometry under apObj.
    static void UnhideLimb(LIMB_ENUM aeLimb, NiAVObject* apObj);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static void HideLimb(BGSBodyPart::LIMB_ENUM aeLimb, NiAVObject* apObj);
    static void ShowLimb(BGSBodyPart::LIMB_ENUM usBodyPart, NiObject* a2);

    BSStringT<char> NodeName;
    BSStringT<char> TargetName;
    BSStringT<char> IKStartNodeName;
    BSStringT<char> PartName;
    BSStringT<char> GoreObjectName;
    // The model of the special debris the part's explosion leaves.
    TESModel ExplosionSpecialDebris;
    TESModelPSA PSAModel;
    PART_DATA Data;
};

uint8_t GetGenericDebrisCount(BGSBodyPart* apThis, [[maybe_unused]] void* apEdx);

// The body part slots.
enum BGSBodyPart::LIMB_ENUM : int32_t {
    LIMB_NONE = -1,
    LIMB_TORSO = 0,
    LIMB_HEAD_1 = 1,
    LIMB_HEAD_2 = 2,
    LIMB_LEFT_ARM_1 = 3,
    LIMB_LEFT_ARM_2 = 4,
    LIMB_RIGHT_ARM_1 = 5,
    LIMB_RIGHT_ARM_2 = 6,
    LIMB_LEFT_LEG_1 = 7,
    LIMB_LEFT_LEG_2 = 8,
    LIMB_LEFT_LEG_3 = 9,
    LIMB_RIGHT_LEG_1 = 10,
    LIMB_RIGHT_LEG_2 = 11,
    LIMB_RIGHT_LEG_3 = 12,
    LIMB_BRAIN = 13,
    LIMB_WEAPON = 14,
    LIMB_COUNT = 15,
};
