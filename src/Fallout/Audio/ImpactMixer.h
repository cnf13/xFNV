#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/Audio/WeaponNearFarSound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"

#include <stdint.h>

class Actor;
class TESObjectWEAP;
class TESSound;

// ImpactMixer has only static members.
class ImpactMixer {
public:
    // One collision to make a sound for: where it happened, how fast and with what friction, the materials of the
    // two bodies and the bodies.
    struct COLLISION_SOUND {
        COLLISION_SOUND();

        NiPoint3 Location;
        float fVelocity;
        float fFriction;
        int8_t cMaterialType1;
        int8_t cMaterialType2;
        int32_t iID;
        bhkRigidBody* pBody1;
        bhkRigidBody* pBody2;
    };

    // The sound name of each material, indexed by material id; nothing writes through the pointers.
    using MaterialSoundNameTable = const char* [13];
    // The footstep sound name suffix of each step type (const because nothing writes through the pointers).
    using StepSoundNameTable = const char* [10];
    // The footstep sound of each material and step type.
    using FootstepSoundTable = TESSound* [13][10];
    // The time each collision id last made a sound.
    using ImpactTimeMap = NiTPointerMap<int32_t, uint32_t>;
    // Guessed bound: the database gives none; the next data item starts 0x34 bytes on.
    // The material each material falls back to for a footstep sound it lacks (13: none).
    using ReplacementMaterialTable = int32_t[13];
    using GunshotHandleArray = WeaponNearFarSound[20];

    // Plays apRef's jump sound; aiFootType is not read, and the only caller passes 0.
    static void PlayJump(TESObjectREFR* apRef, int32_t aiFootType);
    // Plays apWeapon's gunshot sound for apSource at aOriginNode.
    static void PlayGunshot(TESObjectWEAP* apWeapon, TESObjectREFR* apSource, NiAVObject* aOriginNode, bool abPlayer);
    static const char* GetMaterialSoundName(int32_t aiMat);
    static void InitFootsteps();
    static void InitGunshots();
    static void ReInitFootsteps();
    // Plays nothing: it only enters and leaves the audio memory context, and the body reads no argument.
    static void PlayHitSound(
        Actor* apAttacker,
        float afDamage,
        float afDefDamage,
        Actor* apTarget,
        int32_t aiWeaponType,
        int32_t aiArmorType,
        int32_t aiShieldType,
        bool abFakeHit,
        bool abEnchantedWeapon
    );
    static void PlayCollisionSound(ImpactMixer::COLLISION_SOUND& arDataIn);

protected:
    static bool GetGunshotsInitialized();
    static void SetGunshotsInitialized(bool abIn);

public:
    static void SetAltMaterialSoundName(int32_t aiMat, const char* apAltName);
    static void PlayFootstep(TESObjectREFR* apRef, int32_t aiFootType);
    static void PlayWeaponBlock(
        int32_t aiAttWeaponType,
        TESObjectWEAP* apDefWeapon,
        NiPoint3 aAttacker,
        NiPoint3 aDefender,
        bool bEnchantedWeapon
    );
    static TESSound* GetSoundForType(int32_t aiType, float afMass, float afVelocity);
    static int32_t GetFootstepIndexForType(HavokMaterialType aiType);
    static void PlayEquipSound(TESObjectREFR* apRef, int32_t aiType);
    static void DeInitialize();

    static inline bool bFootStepInitialized;
    static inline MaterialSoundNameTable pMaterialSoundNames{
        "ConcSolid",
        "ConcBroken",
        "MetalSolid",
        "MetalHollow",
        "MetalSheet",
        "Wood",
        "Sand",
        "Dirt",
        "Grass",
        "Water",
        "ChainLink",
        "Tile",
        "Carpet"
    };
    // The names SetAltMaterialSoundName stores in place of pMaterialSoundNames.
    static inline MaterialSoundNameTable pAltMaterialSoundNames{};
    enum : uint32_t {
        GUNSHOTS_INITIALIZED = 0x2,
    };
    static inline uint32_t iiInitializationFlags;
    static inline StepSoundNameTable
        pStepSoundNames{"WalkL", "WalkR", "RunR", "RunL", "SneakL", "SneakR", "StairsL", "StairsR", "Land", "Jump"};
    static inline FootstepSoundTable pFootSteps{};
    // The armour footstep and jump sounds PlayFootstep and PlayJump look up on first use
    // ("FSTArmorPowerL", "FSTArmorPowerR", "FSTArmorL", "FSTArmorR", "FSTJumpArmor", "FSTJumpPower").
    static inline TESSound* pPowerArmorStepL;
    static inline TESSound* pPowerArmorStepR;
    static inline TESSound* pHeavyArmorStepL;
    static inline TESSound* pHeavyArmorStepR;
    static inline TESSound* pJumpHeavy;
    static inline TESSound* pJumpPower;
    static ImpactTimeMap pImpactMap;
    static NiPoint3 lastloc;
    static SettingT<INISettingCollection> iCollisionSoundTimeDelta;
    static SettingT<INISettingCollection> fChainMediumMass;
    static SettingT<INISettingCollection> fChainLargeMass;
    static SettingT<INISettingCollection> fGlassMediumMass;
    static SettingT<INISettingCollection> fGlassLargeMass;
    static SettingT<INISettingCollection> fMedWpnWeight;
    static SettingT<INISettingCollection> fLargeWpnWeight;
    static SettingT<INISettingCollection> fMedWpnSpeed;
    static SettingT<INISettingCollection> fLargeWpnSpeed;
    static SettingT<INISettingCollection> iMaxImpactSoundCount;
    static inline ReplacementMaterialTable uiReplacementMaterials{13, 0, 0, 2, 3, 0, 7, 0, 7, 0, 7, 0, 0};
    static GunshotHandleArray GunshotHandles;
    static inline int32_t MaterialEquivalents[36]{0, 0, 7, 0, 8, 2, 0, 0, 9, 5, 0, 2, 5, 2, 0,  0,  3,  4,
                                                  6, 1, 2, 2, 3, 4, 6, 4, 2, 2, 5, 5, 2, 5, 10, 11, 12, 7};

private:
    static SettingT<INISettingCollection> fWoodMediumMass;
    static SettingT<INISettingCollection> fWoodLargeMass;
    static SettingT<INISettingCollection> fStoneMediumMass;
    static SettingT<INISettingCollection> fStoneLargeMass;
    static SettingT<INISettingCollection> fEarthMediumMass;
    static SettingT<INISettingCollection> fEarthLargeMass;
    static SettingT<INISettingCollection> fSkinMediumMass;
    static SettingT<INISettingCollection> fSkinLargeMass;
    static SettingT<INISettingCollection> fMetalMediumMass;
    static SettingT<INISettingCollection> fMetalLargeMass;
    static SettingT<INISettingCollection> fCollisionSoundHeavyThreshold;
    static SettingT<INISettingCollection> fPlayerFootVolume;
    static SettingT<INISettingCollection> fMaxFootstepDistance;
};

// The footstep sound index of each Havok material
// (GetFootstepIndexForType).
