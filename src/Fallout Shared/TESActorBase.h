#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BGSTouchSpellForm.h"
#include "Fallout Shared/TESAIForm.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAnimation.h"
#include "Fallout Shared/TESAttributes.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESScriptableForm.h"
#include "Fallout Shared/TESSpellList.h"

class BGSBodyPartData;
class TESCombatStyle;

// The base form of NPCs and creatures.
class TESActorBase : public TESBoundAnimObject,
                     public TESActorBaseData,
                     public TESContainer,
                     public BGSTouchSpellForm,
                     public TESSpellList,
                     public TESAIForm,
                     public TESHealthForm,
                     public TESAttributes,
                     public TESAnimation,
                     public TESFullName,
                     public TESModel,
                     public TESScriptableForm,
                     public ActorValueOwner,
                     public BGSDestructibleObjectForm {
public:
    // The sexes GetSex distinguishes; creatures have none.
    enum SEX : int32_t {
        SEX_NONE = -1,
        SEX_MALE = 0,
        SEX_FEMALE = 1,
    };

    TESActorBase();
    ~TESActorBase() override;

    void AddChange(int32_t aiChangeFlags) override;
    void RemoveChange(int32_t aiChangeFlags) override;
    // also overrides TESSpellList's and TESAIForm's GetSaveSize/SaveGame/LoadGame (adjustor thunks there)
    uint16_t GetSaveSize(uint32_t aiFlags) override;
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) override;
    void SaveGame(uint32_t aiFlags) override;
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) override;
    void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) override;
    void Revert(BGSLoadFormBuffer* apLoadGameBuffer) override;
    bool GetHasPLSpecTex() override {
        return false;
    }
    bool IsActorBase() override {
        return true;
    }
    bool IsAutoCalc() override;
    void SetAutoCalc(bool abAC) override;
    int32_t GetActorValue(ActorValue::Index aiWhich) override;
    float GetActorFloatValue(ActorValue::Index aiWhich) override;
    TESForm* GetAsForm() override {
        return this;
    }
    uint16_t GetActorLevel() override {
        return TESActorBaseData::GetLevel();
    }

    virtual BGSBodyPartData* GetBodyPartData();
    virtual void SetBodyPartData(BGSBodyPartData*) {}
    virtual TESCombatStyle* GetCombatStyle() const {
        return nullptr;
    }
    virtual void SetCombatStyle(TESCombatStyle*) {}
    virtual void SetActorValue(ActorValue::Index aiWhich, int32_t aiValue);
    virtual void SetActorValue(ActorValue::Index aiWhich, float afValue);
    virtual void ModActorValue(ActorValue::Index aiWhich, int32_t aiValue);
    virtual void ModActorValue(ActorValue::Index aiWhich, float afValue);

    using TESBoundAnimObject::Revert;

    bool FightsInWater();
    // Resets the 3D of the loaded actors with this base.
    void Reset3DForLoadedActors(uint8_t acResetFlags);
    SEX GetSex() const;
    bool IsImmobile() const;
    bool DoesSwim();
    bool DoesWalk() const;
    int32_t GetHealth() const;
    // How much the actor wants the object.
    float GetDesirability(TESForm* apForm);
    // Whether the actor starts dead.
    bool GetStartsDead();
    // Whether apRef's default outfit should be equipped now.
    bool ShouldRunInitDefaultWorn(TESObjectREFR* apRef);
    void SetHealth(int32_t aiValue);
    int32_t CalculateHealth(char acAddFormHealth);
};

// The sex names, indexed by GetSex.
using SexNameArray = char* [2];
inline SexNameArray sexName{const_cast<char*>("Male"), const_cast<char*>("Female")};
