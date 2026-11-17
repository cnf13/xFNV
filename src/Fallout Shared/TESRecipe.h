#pragma once

#include "BSCore/BSSimpleList.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"

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
    // Returns the item form (pInternalItem).
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
    // The required skill's actor value (data.ReqSkillId).
    ActorValue::Index GetRequiredSkill();
    // Returns the required skill level (data.ReqSkillLevel).
    int32_t GetRequiredSkillLevel();
    TESRecipeCategory* GetCategory();
    // Returns pRecipeSubCat.
    TESRecipeCategory* GetSubCategory();

    // RecipeMenu's ListBox<TESRecipe*>::Sort compare: reads each item's TESRecipe from ListBoxItem::object.
    static int32_t CompareForSorting(ListBoxItem<TESRecipe*>* a1, ListBoxItem<TESRecipe*>* a2);
    // RecipeMenu's ListBox<TESRecipe*>::Filter test, passed the recipe.
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
    // One byte, nothing to swap.
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

inline int32_t g_currentRecipeMenuCategory = -1;
