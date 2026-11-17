#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Reconstruction.hpp"

class TESRecipeCategory;

// A recipe's DATA chunk: the required skill and level and the category form IDs.
struct RECIPE_DATA {
	// Swaps the four dwords read from a big-endian file.
	void Endian();

	uint32_t ReqSkillId;
	int32_t ReqSkillLevel;
	uint32_t CategoryID;
	uint32_t SubCategoryID;
};

// One ingredient or output of a recipe: a quantity and the item form (its form ID while loading).
class TESRecipeComponent {
public:
	TESRecipeComponent();

	uint32_t GetQuantity();
	void SetQuantity(uint32_t aquant);
	void SetComponentPointer(TESForm* apPointer);
	// The item form (pInternalItem), through the one-field getter the exe shares with
	// ItemChange::GetCount (thiscall, ret 0, 0x726070).
	TESForm* GetComponentPointer();
	// True when apCompare's quantity or item differs.
	bool Compare(TESRecipeComponent* apCompare);

	uint32_t Quantity;
	TESForm* pInternalItem;
};

// A recipe form (RCPE).
class TESRecipe : public TESForm, public TESFullName {
public:
	TESRecipe();
	~TESRecipe() override;

	void ClearData() override;
	bool Load(TESFile* apFile) override;
	void Save() override;
	void InitItem() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;

	using TESForm::Save;

	TESCondition* GetConditions();
	BSSimpleList<TESRecipeComponent*>* GetIngredientsList();
	BSSimpleList<TESRecipeComponent*>* GetOutputList();
	// The required skill's actor value (data.ReqSkillId), through the one-field getter the
	// exe shares (thiscall, ret 0, 0x59BB30).
	ActorValue::Index GetRequiredSkill();
	// The required skill level (data.ReqSkillLevel), through the one-field getter the exe
	// shares (thiscall, ret 0, 0x45CD60).
	int32_t GetRequiredSkillLevel();
	// pRecipeCat (+0x54), through the one-field getter the exe shares; the forwarder stays for src's callers.
	TESRecipeCategory* GetCategory();
	// pRecipeSubCat, through the one-field getter the exe shares (thiscall, ret 0,
	// 0x6286D0).
	TESRecipeCategory* GetSubCategory();

	// IDA name; not in the PDB. RecipeMenu's ListBox<TESRecipe*>::Sort compare: the exe reads
	// each item's TESRecipe at +4 (ListBoxItem::object).
	static int32_t CompareForSorting(ListBoxItem<TESRecipe*>* a1, ListBoxItem<TESRecipe*>* a2);
	// IDA name; not in the PDB. RecipeMenu's ListBox<TESRecipe*>::Filter test, passed the recipe (IDA: HighProcess*,
	// from a fold of its category getter).
	static bool ShouldHideItem(TESRecipe* a1);

	RECIPE_DATA data;
	TESCondition RecipeConditions;
	BSSimpleList<TESRecipeComponent*> ingredientlist;
	BSSimpleList<TESRecipeComponent*> outputlist;
	BSSimpleList<TESRecipeComponent*> garbageCollection;
	TESRecipeCategory* pRecipeCat;
	TESRecipeCategory* pRecipeSubCat;
};

// A recipe category's DATA chunk: its flags.
struct RECIPE_CATEGORY_DATA {
	// One byte, nothing to swap: the engine's shared empty thiscall function (0x483710).
	void Endian();

	uint8_t flags;
};

// A recipe category form (RCCT).
class TESRecipeCategory : public TESForm, public TESFullName {
public:
	TESRecipeCategory();
	~TESRecipeCategory() override;

	bool Load(TESFile* apFile) override;
	void Save() override;
	void Copy(TESForm* apCopy) override;
	bool Compare(TESForm* apCompare) override;
	using TESForm::Save;

	void ResetFlag(uint8_t flag);

	RECIPE_CATEGORY_DATA data;
	uint8_t padding25[3];
};

inline constexpr fnvd::Vtable TESRecipe_vftable_TESFullName{};
inline constexpr fnvd::Vtable TESRecipe_vftable{};
inline constexpr fnvd::Vtable TESRecipeCategory_vftable{};
inline constexpr fnvd::Vtable TESRecipeCategory_vftable_TESFullName{};

// IDA names; not in the PDB.
inline int32_t g_currentRecipeMenuCategory = -1;
