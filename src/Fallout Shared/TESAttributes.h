#pragma once

#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BaseFormComponent.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;

// The seven SPECIAL attributes of an actor base.
class TESAttributes : public BaseFormComponent {
public:
    TESAttributes();
    ~TESAttributes();

    void InitializeDataComponent() override;
    void ClearDataComponent() override {}
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // Saves the attributes when the save changed them (CHANGE_ACTOR_BASE_ATTRIBUTES).
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    // Loads the attributes when the save changed them (CHANGE_ACTOR_BASE_ATTRIBUTES).
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
    // Saves the attributes to the old-format save.
    void SaveGameTES(uint32_t aiFlags);
    // Loads the attributes from the old-format save.
    void LoadGameTES(uint32_t aiFlags, uint32_t aiCurrentFlags);
    // The bytes the attributes add to a save.
    uint16_t GetSaveSize(uint32_t aiFlags);
    // The value of attribute aeAttribute.
    uint8_t GetAttributeValue(ActorValue::Index aeAttribute) const;
    // Sets attribute aeAttribute to acValue; with abMarkChange the owning form records the
    // attribute change.
    void SetAttributeValue(ActorValue::Index aeAttribute, uint8_t acValue, bool abMarkChange);
    // The bytes the attributes add to a form's data: 7.
    uint32_t GetAttributeDataSize();
    uint32_t Save(void* apData);
    uint32_t Load(void* apData);
    uint8_t GetStrength() const {
        return GetAttributeValue(ActorValue::STRENGTH);
    }
    uint8_t GetPerception() const {
        return GetAttributeValue(ActorValue::PERCEPTION);
    }
    uint8_t GetEndurance() const {
        return GetAttributeValue(ActorValue::ENDURANCE);
    }
    uint8_t GetCharisma() const {
        return GetAttributeValue(ActorValue::CHARISMA);
    }
    uint8_t GetIntelligence() const {
        return GetAttributeValue(ActorValue::INTELLIGENCE);
    }
    uint8_t GetAgility() const {
        return GetAttributeValue(ActorValue::AGILITY);
    }
    uint8_t GetLuck() const {
        return GetAttributeValue(ActorValue::LUCK);
    }
    void SetStrength(uint8_t acValue) {
        SetAttributeValue(ActorValue::STRENGTH, acValue, true);
    }
    void SetPerception(uint8_t acValue) {
        SetAttributeValue(ActorValue::PERCEPTION, acValue, true);
    }
    void SetEndurance(uint8_t acValue) {
        SetAttributeValue(ActorValue::ENDURANCE, acValue, true);
    }
    void SetCharisma(uint8_t acValue) {
        SetAttributeValue(ActorValue::CHARISMA, acValue, true);
    }
    void SetIntelligence(uint8_t acValue) {
        SetAttributeValue(ActorValue::INTELLIGENCE, acValue, true);
    }
    void SetAgility(uint8_t acValue) {
        SetAttributeValue(ActorValue::AGILITY, acValue, true);
    }
    void SetLuck(uint8_t acValue) {
        SetAttributeValue(ActorValue::LUCK, acValue, true);
    }

    uint8_t cAttribute[7];
};
