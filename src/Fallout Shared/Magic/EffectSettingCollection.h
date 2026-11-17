#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ActorValue.h"

class EffectSetting;

// The list of every magic effect (EffectSetting) loaded.
class EffectSettingCollection : public BSSimpleList<EffectSetting*> {
public:
    ~EffectSettingCollection();

    void ClearData();

    // Adds a non-null effect to the head of the collection.
    static void AddEffect(EffectSetting* apSetting);
    // Runs InitItem on every effect in the collection.
    static void InitItems();
    // Constructs an empty EffectSettingCollection.
    static EffectSettingCollection* Construct(void* apMemory);
    // aiResistVariable is the actor value index the effect is resisted by (-1 for none).
    static EffectSetting* CreateDefaultEffect(
        uint32_t aiFormID,
        const char* acEditorID,
        const char* apEffectName,
        ActorValue::Index aiAssociatedSkill,
        float afBaseCost,
        ActorValue::Index aeAssociatedActorVaue,
        uint32_t aiFlags,
        ActorValue::Index aiResistVariable,
        int32_t aiEffectArchetype
    );
    // Creates the six hard-coded effects (script effect, fire/frost/shock damage, restore health and the usage
    // monitor, which is kept in pUsageMonitorEffectSetting).
    static void CreateDefaultEffects();
    static EffectSetting* GetUsageMonitorEffectSetting();
    static void Reset();
    static int32_t GetKnownEffectsSaveSize();

    // The collection itself.
    static EffectSettingCollection Instance;
};

// The usage monitor effect loaded from the game data.
inline EffectSetting* pUsageMonitorEffectSetting;
