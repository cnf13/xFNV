#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/BaseFormComponent.h"

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
struct GROUP_REACTION;
class TESForm;

// One reaction of a faction or race: the other form and the reactions to it.
struct GROUP_REACTION {
	// Swaps the three dwords.
	void Endian();

	TESForm* pForm;
	int32_t iReaction;
	int32_t iFightReaction;
};

// The reactions (friend, ally, enemy) of a faction or race to other forms.
class TESReactionForm : public BaseFormComponent {
public:
	TESReactionForm();
	~TESReactionForm();

	void InitializeDataComponent() override;
	void ClearDataComponent() override;
	void CopyComponent(BaseFormComponent* apCopyFC) override;
	bool CompareComponent(BaseFormComponent* apCompareFC) override;

	uint16_t GetSaveSize(uint32_t aiFlags);
	void SetReaction(TESForm* apForm, int32_t aiReaction);

	// Resolves the reaction form IDs loaded for apOwner.
	void InitItem(TESForm* apOwner);

	// The reaction to apForm.
	int32_t GetReaction(TESForm* apForm);
	// Sets the fight reaction to apForm; adds an entry for a non-zero value when apForm has none.
	void SetFightReaction(TESForm* apForm, int32_t aiReaction);
	// Adds aiReaction to the reaction to apForm: frees the entry when the sum is 0, adds one when apForm has none.
	void ModReaction(TESForm* apForm, int32_t aiReaction);
	void FreeReactionList();
	// Writes the reaction chunks of the form being saved.
	void Save();

	BSSimpleList<GROUP_REACTION*>* QReactionList() { return &m_reactionList; }

	// Saves the reactions when they changed.
	void SaveGame(BGSSaveFormBuffer* apSaveGameBuffer);
	// Loads the reactions when they changed.
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	int32_t GetReaction_0048C1B0(TESForm* apForm);
	// Writes the reactions to the old-format save. Tests the caller's whole aiFlags value.
	void SaveGameTES(uint32_t aiFlags);
	// Reads the reactions from the old-format save.
	void LoadGameTES(uint32_t a2, uint32_t a3);
	// Resolves the loaded reactions' form IDs.
	void InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer);
	// Resolves the reaction form IDs after an old-format load when aiFlags has bit 2.
	void InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags);

	BSSimpleList<GROUP_REACTION*> m_reactionList;
	uint8_t m_cGroupFormType;
	uint8_t padding0D[3];
};
