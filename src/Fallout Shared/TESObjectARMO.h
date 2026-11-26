#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSBipedModelList.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSPickupPutdownSounds.h"
#include "Fallout Shared/BGSRepairItemList.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"

class TESSound;

// The armor record's main data.
struct OBJ_ARMO {
    enum ARMO_FLAGS : uint8_t {
        FLAG_MODULATES_VOICE = 0x1,
    };

    // Swaps the rating and the damage threshold read from a big-endian file.
    void Endian();

    uint16_t sRating;
    float fDamageThreshold;
    uint8_t cFlags;
    uint8_t cConditionLevel;
};

// The foley sounds an armor makes while its wearer moves.
enum ENUM_FOLEY_SOUND : uint32_t {
    ENUM_FOLEY_SNEAK = 17,
    ENUM_FOLEY_WALK = 18,
    ENUM_FOLEY_RUN = 19,
    ENUM_FOLEY_ARMOR_SNEAK = 20,
    ENUM_FOLEY_ARMOR_WALK = 21,
    ENUM_FOLEY_ARMOR_RUN = 22,
};

// One foley sound of an armor: the sound, its chance and the movement it plays for.
struct FOLEY_SOUND {
    FOLEY_SOUND();

    // The foley sound's byte swap; the body is empty.
    void Endian();

    TESSound* pSound;
    uint8_t iChance;
    ENUM_FOLEY_SOUND eType;
};

// The base form of armor and clothing.
class TESObjectARMO : public TESBoundObject,
                      public TESFullName,
                      public TESScriptableForm,
                      public TESEnchantableForm,
                      public TESValueForm,
                      public TESWeightForm,
                      public TESHealthForm,
                      public TESBipedModelForm,
                      public BGSDestructibleObjectForm,
                      public BGSEquipType,
                      public BGSRepairItemList,
                      public BGSBipedModelList,
                      public BGSPickupPutdownSounds {
public:
    TESObjectARMO();
    ~TESObjectARMO() override;

    void InitializeData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    // also override TESValueForm's slots of the same signatures
    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(uint32_t aiFlags) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    using TESForm::Save;

    bool ModulatesVoice();
    // The armor rating scaled from sRating.
    float GetArmorRating();
    float GetDamageThreshold();
    float GetCalculatedArmorRating();
    uint8_t GetArmorWeight();
    TESSound* GetFoleySound(ENUM_FOLEY_SOUND aeType);

    OBJ_ARMO data;
    uint32_t flags;

    // The foley sounds of an armor that is its own base; the base armor of one that borrows them otherwise.
    union {
        BSSimpleList<FOLEY_SOUND*>* pFoleySoundList;
        TESObjectARMO* pFoleyTemplate;
    };

    bool bIsFoleyBase;
};

// The armor weight class names, light, heavy and medium.
extern SettingT<GameSettingCollection> sArmorWeightLight;
extern SettingT<GameSettingCollection> sArmorWeightHeavy;
extern SettingT<GameSettingCollection> sArmorWeightMedium;
// The armor weight class names as pointers to their settings, light, heavy and medium.
extern SettingT<GameSettingCollection>* pGSArmorWeight[3];
