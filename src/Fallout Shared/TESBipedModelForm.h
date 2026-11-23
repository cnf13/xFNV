#pragma once

#include "Fallout Shared/BGSMessageIcon.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModelRDT.h"
#include "Fallout Shared/TESModelTextureSwap.h"

class BGSBipedModelList;
class BipedAnim;

// The biped slots; InventoryChanges::GetWornItem checks BO_WEAPON = 5 and BO_HAT = 10.
enum BIPED_OBJECT : int32_t {
    BO_NONE = -1,
    BO_HEAD,
    BO_HAIR,
    BO_UPPER_BODY,
    BO_LEFT_HAND,
    BO_RIGHT_HAND,
    BO_WEAPON,
    BO_PIP_BOY,
    BO_BACKPACK,
    BO_NECKLACE,
    BO_HEADBAND,
    BO_HAT,
    BO_EYE_GLASSES,
    BO_NOSE_RING,
    BO_EARRINGS,
    BO_MASK,
    BO_CHOKER,
    BO_MOUTH_OBJECT,
    BO_BODY_ADDON_1,
    BO_BODY_ADDON_2,
    BO_BODY_ADDON_3,
    BIPED_OBJECT_COUNT,
};

// The biped slots a model form fills and its flags.
struct BIPED_MODEL {
    enum : uint8_t {
        HAS_BACKPACK = 0x4,
        MEDIUM = 0x8,
        ADDON_MODEL = 0x10,
        POWER_ARMOR = 0x20,
        NON_PLAYABLE = 0x40,
        HEAVY = 0x80,
    };

    // Byte-swaps the record read from a big-endian file.
    void Endian();

    uint32_t iBipedObjectSlots;
    uint8_t sFlags;
};

// The male and female biped and world models, icons and slots of wearable forms.
class TESBipedModelForm : public BaseFormComponent {
public:
    TESBipedModelForm();
    ~TESBipedModelForm();

    void InitializeDataComponent() override;
    void ClearDataComponent() override;
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    void AddToBiped(BipedAnim* apBiped, TESActorBase::SEX aeSex, BIPED_OBJECT aeBipedObject);

    bool FillsBipedSlot(BIPED_OBJECT aeBipedSlot, bool abHairTest, BGSBipedModelList* apBipedModelList);
    // Resolves the texture swaps for apOwner.
    void InitItem(TESForm* apOwner);
    // Writes the slots, the models, the icons and the constraint template as their chunks.
    void Save();
    bool IsPowerArmor();
    bool HasBackPack();
    const char* GetBipedConstraint();
    // Sets or clears BIPED_MODEL::ADDON_MODEL.
    void SetAddOnModel(bool abVal);
    bool IsHeavy();
    // Whether the armour is medium (bit 3 of the biped flags).
    bool IsMedium();
    // The world model of sex aeSex, the male one when a female one has no file.
    TESModelTextureSwap* GetWorldTESModel(TESActorBase::SEX aeSex);
    // Gets the icon path for aeSex.
    const char* GetIcon(TESActorBase::SEX aeSex);
    TESModelTextureSwap* GetBipedTESModel(TESActorBase::SEX aeSex);
    // The biped model path of sex aeSex, the male one when the female one has no file.
    const char* GetBipedModel(TESActorBase::SEX aeSex);
    const char* GetMessageIcon(TESActorBase::SEX aeSex);
    bool GetPlayable();
    // Always returns false.
    bool IsJewelry();
    bool FillsMatchingBipedSlots(TESBipedModelForm* apOtherBiped);
    const char* GetIconDefaultPath();
    const char* GetWorldModel_0(TESActorBase::SEX aeSex);

    // The form's biped model component, or null.
    static TESBipedModelForm* GetFormAsBipedModel(TESForm* apForm);
    // Selects the world model using apObj and apRef.
    static TESModelTextureSwap* GetWorldTESModel(TESForm* apObj, TESObjectREFR* apRef);
    // The biped model apObj shows on apRef: the owner's sex picks it when the owner is an NPC.
    static TESModelTextureSwap* GetBipedTESModel(TESForm* apObj, TESObjectREFR* apRef);
    // The message icon path apObj shows on apRef, or null.
    static const char* GetMessageIcon(TESForm* apObj, TESObjectREFR* apRef);
    static const char* GetWorldModel(TESForm* apObj, TESObjectREFR* apRef);
    static const char* GetIcon_0(TESForm* apObj, TESObjectREFR* apRef);

    BIPED_MODEL bipedModelData;
    TESModelTextureSwap bipedModel[2];
    TESModelTextureSwap worldModel[2];
    TESIcon inventoryIcon[2];
    BGSMessageIcon messageIcon[2];
    TESModelRDT ConstraintTemplate;
};

// The biped slot names, indexed by BIPED_OBJECT.
using BipedObjectNameArray = char* [BIPED_OBJECT_COUNT];
inline BipedObjectNameArray bipedObjectName{
    const_cast<char*>("Head"),       const_cast<char*>("Hair"),        const_cast<char*>("UpperBody"),
    const_cast<char*>("LeftHand"),   const_cast<char*>("RightHand"),   const_cast<char*>("Weapon"),
    const_cast<char*>("PipBoy"),     const_cast<char*>("Backpack"),    const_cast<char*>("Necklace"),
    const_cast<char*>("Headband"),   const_cast<char*>("Hat"),         const_cast<char*>("EyeGlasses"),
    const_cast<char*>("NoseRing"),   const_cast<char*>("Earrings"),    const_cast<char*>("Mask"),
    const_cast<char*>("Choker"),     const_cast<char*>("MouthObject"), const_cast<char*>("BodyAddOn1"),
    const_cast<char*>("BodyAddOn2"), const_cast<char*>("BodyAddOn3")
};
