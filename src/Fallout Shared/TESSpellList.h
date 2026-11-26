#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BaseFormComponent.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class SpellItem;
class TESForm;
class TESLevSpell;

// The spells a form knows, and the leveled spell lists it draws from.
class TESSpellList : public BaseFormComponent {
public:
    TESSpellList();
    ~TESSpellList();

    void InitializeDataComponent() override {}
    void ClearDataComponent() override;
    void CopyComponent(BaseFormComponent* apCopyFC) override;
    bool CompareComponent(BaseFormComponent* apCompareFC) override;

    // The size of the component's save game data for the change flags aiFlags.
    virtual uint16_t GetSaveSize(uint32_t aiFlags);
    virtual void SaveGame(uint32_t aiFlags);
    virtual void LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags);

    // Adds apSpell to the list unless it is in it; false when it was.
    bool AddSpell(SpellItem* apSpell);
    // Adds apSpellIn, a spell or a leveled spell list (a form ID while a file loads), to the
    // matching list.
    bool AddSpell(TESForm* apSpellIn);
    // Adds the leveled spell list apSpellIn to leveledSpellList; false when it is null, when this is the spell list of
    // the base form of a global actor, or when the list holds it already.
    bool AddSpell(TESLevSpell* apSpellIn);
    // Frees the spells and leveled spell lists.
    void FreeSpellList();
    // Writes the spell and leveled-spell chunks of the form being saved.
    void Save();
    // Whether apSpell is in the spell list.
    bool HasSpell(SpellItem* apSpell);
    // Removes apSpell from the spell list.
    void RemoveSpell(SpellItem* apSpell);
    // Loads the spell and leveled spell lists when the save changed them (change flag 0x10); not virtual.
    void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

    // Resolves the spell form IDs loaded for apOwner.
    void InitItem(TESForm* apOwner);
    // Returns the address of the spell list (spellList).
    BSSimpleList<SpellItem*>* GetSpellList();
    // Saves both lists' form IDs when change flag 0x10 is set. TESActorBase::SaveGame passes its BGSSaveFormBuffer.
    void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
    bool HasLeveledSpell(TESLevSpell* apSpell);

    BSSimpleList<SpellItem*> spellList;
    BSSimpleList<TESLevSpell*> leveledSpellList;
};
