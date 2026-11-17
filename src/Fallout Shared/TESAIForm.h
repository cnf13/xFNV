#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/PackageList.h"

class Actor;
class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class TESObject;
class TESPackage;

// How readily an actor attacks (AIDATA's cAggression).
enum ACTOR_AGGRESSION : int32_t {
    ACTOR_AGGRESSION_UNAGGRESSIVE = 0,
    ACTOR_AGGRESSION_AGGRESSIVE = 1,
    ACTOR_AGGRESSION_VERY_AGGRESSIVE = 2,
    ACTOR_AGGRESSION_FRENZIED = 3,
    ACTOR_AGGRESSION_NUMLEVELS = 4,
};

// The AI data record of an actor base.
struct AIDATA {
    // Byte-swaps the service flags and the aggro radius read from a big-endian file.
    void Endian();

    uint8_t cAggression;
    uint8_t cConfidence;
    uint8_t cEnergy;
    uint8_t cResponsibility;
    uint8_t cMood;
    uint8_t padding05[3];
    uint32_t iServiceFlags;
    uint8_t cTrainingSkill;
    uint8_t cTrainingLevel;
    uint8_t cAssistance;
    uint8_t bAggroRadius;
    uint32_t uiAggroRadius;
};

// The AI settings and package list of an actor base.
class TESAIForm : public BaseFormComponent {
public:
    // The form change flag for the AI data record.
    enum FORM_CHANGE : uint32_t {
        CHANGE_AI_DATA = 0x8,
    };

    TESAIForm();
    ~TESAIForm();

    void InitializeDataComponent() override;
    void ClearDataComponent() override;
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;
    // The size of the component's save game data for the change flags aiFlags.
    virtual uint16_t GetSaveSize(uint32_t aiFlags);
    virtual void SaveGame(uint32_t aiFlags);
    virtual void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags);

    // Adds apPack to the package list.
    void AddPackage(TESPackage* apPack);
    // Resolves the packages loaded for apOwner.
    void InitItem(TESForm* apOwner);
    // Saves the AI data when the save changed it (CHANGE_ACTOR_BASE_AIDATA).
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Loads the AI data when the save changed it (CHANGE_ACTOR_BASE_AIDATA).
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    void SetAggression(uint8_t acAggression, bool abAddChange);
    void SetConfidence(uint8_t acConfidence, bool abAddChange);
    void SetEnergy(uint8_t acEnergy, bool abAddChange);
    void SetResponsibility(uint8_t acResponsibility, bool abAddChange);
    void SetMood(uint8_t acMood);
    void SetAssistance(uint8_t acAssistance);
    void SetHasAggroRadius(uint8_t abAggro);
    void SetAggroRadius(uint32_t auiRadius);
    void SetServiceFlags(uint32_t aiServeFlags, bool abAddChange);
    bool GetOffersService(uint32_t aiServeFlag);
    ActorValue::Index GetTrainingSkill() const;
    // Copies apAIForm's AI settings.
    void CopyAIData(TESAIForm* apAIForm);
    // Replaces the package list with apAIForm's.
    void CopyAIPackageList(TESAIForm* apAIForm);
    // Marks the TESActorBase this component belongs to as changed.
    void AddFormChange(int32_t aiChangeFlags);
    uint8_t GetResponsibility();
    // The energy level, AIData.cEnergy.
    uint8_t GetEnergy() {
        return AIData.cEnergy;
    }
    void SetAIData(AIDATA* pAIData);
    // The aggression, AIData.cAggression.
    uint8_t GetAggression() {
        return AIData.cAggression;
    }
    // AIData.cAssistance.
    uint8_t GetAssistance() {
        return AIData.cAssistance;
    }
    // AIData.cConfidence.
    uint8_t GetConfidence() {
        return AIData.cConfidence;
    }
    // Returns AIData.cMood.
    uint8_t GetMood();
    // AIData.bAggroRadius.
    uint8_t GetHasAggroRadius() {
        return AIData.bAggroRadius;
    }
    // AIData.uiAggroRadius.
    uint32_t GetAggroRadius() {
        return AIData.uiAggroRadius;
    }
    // AIData.iServiceFlags.
    uint32_t GetServiceFlags() {
        return AIData.iServiceFlags;
    }
    // AIData.cTrainingLevel.
    uint8_t GetTrainingSkillLevel() {
        return AIData.cTrainingLevel;
    }
    // Stores aeSkill - 32 in AIData.cTrainingSkill when it is a skill, then marks the AI data
    // changed.
    void SetTrainingSkill(ActorValue::Index aeSkill);
    // Stores aiVal in AIData.cTrainingLevel and marks the AI data changed.
    void SetTrainingSkillLevel(uint8_t aiVal);
    bool SellBuysItem(TESObject* apObject);

    static TESPackage* GetCurrentPackage(Actor* apActor);
    // The value of confidence level acConfidence (fConfidences); 0 past the last level.
    static float GetConfidenceLevel(uint8_t acConfidence);
    // Adds to apPacks the packages of apActor's base that should have run between afStartdate
    // and afhour.
    static void GetMissedPackages(Actor* apActor, BSSimpleList<TESPackage*>* apPacks, float afStartdate, float afhour);

    AIDATA AIData;
    PackageList AIPackList;
};

// The confidence levels' values, in level order (cowardly to foolhardy); the table is filled from the confidence
// settings once, when the first TESAIForm initialises its data.
using ConfidenceTable = float[5];
inline ConfidenceTable fConfidences{};
// Whether fConfidences is filled.
inline uint8_t confidenceSet;

extern SettingT<INISettingCollection> bFixAIPackagesOnLoad;
