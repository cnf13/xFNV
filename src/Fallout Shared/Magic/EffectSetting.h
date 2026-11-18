#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/TESDescription.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESModel.h"

class TESEffectShader;
class TESObjectLIGH;
class TESSound;

float NiAbs(float fValue);

// A magic effect form (MGEF).
class EffectSetting : public TESForm, public TESModel, public TESDescription, public TESFullName, public TESIcon {
public:
    enum EffectFlag : uint32_t {
        HOSTILE = 0x1,
        RECOVER = 0x2,
        DETRIMENTAL = 0x4,
        MAGNITUDE_PERCENT = 0x8,
        RANGE_SELF = 0x10,
        RANGE_TOUCH = 0x20,
        RANGE_TARGET = 0x40,
        NO_DURATION = 0x80,
        NO_MAGNITUDE = 0x100,
        NO_AREA = 0x200,
        PERSIST = 0x400,
        CREATE_SPELLMAKING = 0x800,
        GORY_VISUALS = 0x1000,
        DISPLAY_EFFECT_NAME = 0x2000,
        LIGHT_NEGATE = 0x4000,
        NO_RECAST = 0x8000,
        USE_ASSOCIATED_WEAPON = 0x10000,
        USE_ASSOCIATED_ARMOR = 0x20000,
        USE_ASSOCIATED_CREATURE = 0x40000,
        USE_ASSOCIATED_SKILL = 0x80000,
        USE_ASSOCIATED_ATTR = 0x100000,
        PLAYER_KNOWS = 0x200000,
        DISABLED = 0x400000,
        MARKER_EFFECT = 0x800000,
        PAINLESS = 0x1000000,
        PROJECTILE_TYPE_BIT0 = 0x2000000,
        PROJECTILE_TYPE_BIT1 = 0x4000000,
        NO_HIT_EFFECT = 0x8000000,
        NO_DEATH_DISPEL = 0x10000000,
        HAVOK_EXPLOSION = 0x20000000,
        MAGNITUDE_LEVEL = 0x40000000,
        MAGNITUDE_FEET = 0x80000000,
    };

    // The effect's DATA chunk.
    struct EffectSettingData {
        // Swaps the record read from a big-endian file.
        void Endian();

        uint32_t iFlags;
        float fBaseCost;
        TESForm* pAssociatedItem;
        ActorValue::Index iAssociatedSkill;
        ActorValue::Index iResistVariable;
        int16_t iNumCounterEffects;
        TESObjectLIGH* pLight;
        float fSpeed;
        TESEffectShader* pEffectShader;
        TESEffectShader* pEnchantEffect;
        TESSound* pCastSound;
        TESSound* pBoltSound;
        TESSound* pHitSound;
        TESSound* pAreaSound;
        float fCEEnchantFactor;
        float fCEBarterFactor;
        EffectArchetypes::ArchetypeID eArchetype;
        ActorValue::Index eAssociatedActorValue;
    };

    EffectSetting();
    ~EffectSetting() override;

    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;
    const char* GetObjectTypeName() override;

    using TESForm::Save;

    // Whether the associated actor value is fixed.
    bool IsAssociatedActorValueFixed();
    // Whether all of the flags aeFlags are set.
    bool CheckEffectFlag(uint32_t aeFlags);
    bool IsFlagSet(EffectSetting::EffectFlag aiFlag);
    // The base cost (data.fBaseCost).
    float GetBaseCost();
    // True when the effect's archetype runs a script.
    bool IsScriptEffect();
    // Constructs a copy of the effect's name in apResult; returns apResult.
    BSStringT<char> GetEffectName();
    // The actor value that resists the effect, or -1 for none.
    ActorValue::Index GetResistanceVariable();
    bool IsAssociatedActorValueUsed();
    bool GetFlag(uint32_t aiFlag);
    bool IsDisabled();
    // The projectile type: 2 when PROJECTILE_TYPE_BIT0 (spray) is set, plus 1 when PROJECTILE_TYPE_BIT1 (bolt) is.
    MagicSystem::ProjectileType GetProjectileType();
    // Counts one more queued load of the associated item, or warns when it is already loaded; the new count.
    int32_t QueueAssociatedItem();
    // Counts one more queued load of the effect, or warns when it is already loaded; the new count.
    int32_t QueueEffect();
    // Whether the effect harms its target: the detrimental flag, flipped when aeActorValue (none for -1) is a value
    // whose damage is positive.
    bool IsDetrimental(ActorValue::Index aeActorValue);
    EffectArchetypes::ArchetypeID GetEffectArchetype();
    // Counts one more load of the effect (one fewer when the count is negative); the new count.
    int32_t IncEffectLoadedCount();
    void SetFlag(EffectSetting::EffectFlag aiFlag, bool abValue);
    int32_t IncAssociatedItemLoadedCount();
    int32_t QueueAssociatedItemFinished();
    bool IsAssociatedItemLoaded();
    bool IsAssociatedItemQueued();
    int32_t DecEffectLoadedCount();
    int32_t QueueEffectFinished();
    bool IsEffectLoaded();
    bool IsEffectQueued();
    int32_t GetNumCounterEffects();
    ActorValue::Index GetAssociatedActorValue();
    TESForm* GetAssociatedItem() {
        return data.pAssociatedItem;
    }
    // Returns data.fSpeed.
    float GetProjectileSpeed();
    // data.pBoltSound, through the shared one-field getter.
    TESSound* GetBoltSound();
    void SetBaseCost(float afBaseCost);
    void SetConstantEffectEnchantmentFactor(float afCEEnchantFactor);
    void SetConstantEffectBarterFactor(float afCEBarterFactor);
    void SetProjectileSpeed(float afSpeed);
    bool getArchType();
    void SetEffectArchetype(int32_t a2);
    void SetEffectName(const char* apEffectName);
    int32_t DecAssociatedItemLoadedCount();
    void SetCounterEffects(BSSimpleList<EffectSetting*>* apCounterEffects);

    bool (*pFilterValidationFunction)(EffectSetting*, void*);
    void* pFilterValidationItem;
    EffectSettingData data;
    BSSimpleList<EffectSetting*> counterEffects;
    int32_t iEffectLoadedCount;
    int32_t iAssociatedItemLoadedCount;
};
