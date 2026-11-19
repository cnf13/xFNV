#include "Fallout/Interface/Menus/RecipeMenu.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileImage.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSMessage.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESIcon.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESRecipe.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/QuantityMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Interface/Menus/TutorialMenu.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/tchar.h"

#include <cmath>
#include <cstdint>

// ListBox<TESRecipe*>::NumUnfiltered's guarded static and its guard word.
static int32_t iUT_num_filtered;
static uint32_t iUT_num_filtered_guard;

RecipeMenu::filterlist_t RecipeMenu::filterlist;

BSSimpleArray<TESRecipeCategory*, 1024>* get_g_RecipeMenuCategories() {
    return &RecipeMenu::filterlist;
}

RecipeMenu::RecipeMenu() : eFilterSound(MENU_SOUND_NEXT_PREVIOUS), plbCurrListbox(&lbRecipeList), bTutorial(false) {
    g_recipeMenuTrait_Title = Tile::TextToTrait("_Title");
    g_recipeMenuTrait_Value = Tile::TextToTrait("_Value");
    g_recipeMenuTrait_IsRecipeSelected = Tile::TextToTrait("_IsRecipeSelected");
    RecipeMenu::pCurrItem = nullptr;
    RecipeMenu::iNumCanMake = 0;
    get_g_RecipeMenuCategories()->Clear(true);
}

RecipeMenu::~RecipeMenu() {
    pDataHandler->ClearRecipeContainer();
    RecipeMenu::pMe = nullptr;
}

void RecipeMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (((uint32_t)aiID) <= 13) {
        pTiles[aiID] = static_cast<TileImage*>(apTile);
    }
}

// The list box's virtuals are called through a ListBoxBase pointer so they stay indirect.
void RecipeMenu::DoClick(int32_t a1, Tile* a2) {
    // The tile is stored into a2 and never read again, as the game does.
    if (!a2 && a1 >= 0 && a1 < 14) {
        a2 = pTiles[a1];
    }
    ListBoxBase* pRecipeBase = &lbRecipeList;
    switch (a1) {
        case 0:
        case 1:
        case 2: {
            if (!RecipeMenu::bControlIngredientList) {
                Interface::PlayMenuSound(eFilterSound);
                g_currentRecipeMenuCategory += 2 * (a1 != 0) - 1;
                if (g_currentRecipeMenuCategory >= -1) {
                    BSSimpleArray<TESRecipeCategory*, 1024>* g_RecipeMenuCategories = get_g_RecipeMenuCategories();
                    if (g_currentRecipeMenuCategory >= g_RecipeMenuCategories->iSize) {
                        g_currentRecipeMenuCategory = -1;
                    }
                }
                else {
                    BSSimpleArray<TESRecipeCategory*, 1024>* v3 = get_g_RecipeMenuCategories();
                    g_currentRecipeMenuCategory = v3->iSize - 1;
                }
                if (g_currentRecipeMenuCategory == -1) {
                    const char* v6 = sRecipes.String();
                    pTiles[1]->Set(4036, v6, true);
                }
                else {
                    uint32_t v8 = g_currentRecipeMenuCategory;
                    BSSimpleArray<TESRecipeCategory*, 1024>* v5 = get_g_RecipeMenuCategories();
                    pTiles[1]->Set(4036, (*v5)[v8]->GetFullName(), true);
                }
                Tile* v7 = pRecipeBase->GetTileAt(0, false);
                pRecipeBase->Highlight(v7);
                pRecipeBase->ScrollToHighlight();
                lbRecipeList.SaveListState();
                lbRecipeList.Filter(&TESRecipe::ShouldHideItem);
                lbRecipeList.RestoreListState(true);
            }
            break;
        }
        case 7: {
            QuantityMenu::Create(RecipeMenu::iNumCanMake, &RecipeMenu::ItemSelectCallback, 0x7FFFFFFF);
            break;
        }
        case 8: {
            RecipeMenu::bControlIngredientList = false;
            RecipeMenu::Close();
            break;
        }
        default: {
            return;
        }
    }
}

// The list boxes' virtuals are called through ListBoxBase / ListBox pointers so they stay indirect, as the game does.
void RecipeMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    // The tile is stored into a2 and never read again, as the game does.
    if (!apTarget && aiID >= 0 && aiID < 14) {
        apTarget = pTiles[aiID];
    }
    ListBoxBase* pRecipeBase = &lbRecipeList;
    ListBoxBase* pIngredientBase = &lbIngredientList;
    ListBox<TESRecipeComponent*>* pIngredients = &lbIngredientList;
    switch (aiID) {
        case 3:
            DoLeave(6, nullptr);
            lbRecipeList.SaveListState();
            pIngredientBase->Highlight(nullptr);
            lbRecipeList.RestoreListState(false);
            return;
        case 6:
            DoLeave(3, nullptr);
            pRecipeBase->Highlight(nullptr);
            InterfaceManager::Instance();
            if (InterfaceManager::Has360Controller()) {
                pIngredientBase->Highlight(pIngredientBase->GetTileAt(0, false));
            }
            bControlIngredientList = true;
            DoEnter(15, static_cast<Tile*>(pIngredientBase->GetHighlightedItem()));
            return;
        case 15: break;
        default: return;
    }

    TESRecipeComponent* pItem = nullptr;
    bool bSingleOutput = false;
    if (bControlIngredientList) {
        if (lbIngredientList.GetHighlightedData()) {
            pItem = lbIngredientList.GetHighlightedData();
        }
    }
    else {
        pTiles[10]->Set(Tile::kTileValue_Visible, 0);
        pTiles[9]->Set(Tile::kTileValue_Visible, 0);
        if (plbCurrListbox->GetHighlightedData()) {
            pCurrItem = plbCurrListbox->GetHighlightedData();
        }
        Actor* pActor =
            (pMe->pCrafterRef && pMe->pCrafterRef->IsActor()) ? static_cast<Actor*>(pMe->pCrafterRef) : nullptr;
        if (pCurrItem) {
            const char* pCategoryName = "*** MISSING CATEGORY ***";
            if (pCurrItem->GetCategory()) {
                // A reference conversion: the TESFullName offset is added without a null check, as the game does.
                TESFullName& rCategoryName = *pCurrItem->GetCategory();
                pCategoryName = rCategoryName.GetFullName();
            }
            pMe->pTiles[4]->Set(Tile::kTileValue_String, pCategoryName, true);
            TESRecipeCategory* pCategory = pCurrItem->GetCategory();
            if (pMe->pCategory == pCategory) {
                pMe->pTiles[4]->Set(Tile::kTileValue_Alpha, 255);
            }
            else {
                pMe->pTiles[4]->Set(Tile::kTileValue_Alpha, 127);
            }
            BSSimpleList<TESRecipeComponent*>* pNode = pCurrItem->GetIngredientsList();
            pIngredients->RemoveAll();
            pItem = pCurrItem->GetOutputList()->GetItem();
            if (static_cast<uint32_t>(pCurrItem->GetOutputList()->ItemsInList()) > 1 &&
                pCurrItem->GetIngredientsList()->ItemsInList() == 1) {
                pNode = pCurrItem->GetOutputList();
                TESFullName& rSubCategoryName = *pCurrItem->GetSubCategory();
                pMe->pTiles[11]->Set(Tile::kTileValue_String, rSubCategoryName.GetFullName(), true);
                bSingleOutput = true;
            }
            else {
                pMe->pTiles[11]->Set(Tile::kTileValue_String, sIngredients.String(), true);
            }
            while (pNode && !pNode->IsEmpty()) {
                TESFullName* pName = dynamic_cast<TESFullName*>(pNode->GetItem()->GetComponentPointer());
                if (pName && pActor) {
                    TESBoundObject* pObject = dynamic_cast<TESBoundObject*>(pNode->GetItem()->GetComponentPointer());
                    if (pObject) {
                        char acText[256];
                        ItemChange* pInventoryItem = pActor->GetInventoryItem(pObject, 0);
                        uint32_t uiHave = pInventoryItem ? static_cast<uint32_t>(pInventoryItem->GetCount()) : 0;
                        if (pInventoryItem) {
                            delete pInventoryItem;
                        }
                        if (bSingleOutput) {
                            uint32_t uiQuantity = pNode->GetItem()->GetQuantity();
                            BSsprintf(acText, 0x100, "%s (%d)", pName->GetFullName(), uiQuantity);
                        }
                        else {
                            uint32_t uiQuantity = pNode->GetItem()->GetQuantity();
                            BSsprintf(acText, 0x100, "%s (%d/%d)", pName->GetFullName(), uiHave, uiQuantity);
                        }
                        Tile* pTile = lbIngredientList.AddItem(pNode->GetItem(), acText, nullptr, nullptr);
                        if (uiHave < pNode->GetItem()->GetQuantity() && !bSingleOutput) {
                            pTile->xChildren.GetTail()->Set(Tile::kTileValue_Alpha, 127);
                        }
                        else {
                            pTile->xChildren.GetTail()->Set(Tile::kTileValue_Alpha, 255);
                        }
                    }
                }
                else {
                    const char* pError = "ERROR WITH CAST";
                    lbIngredientList.AddItem(pNode->GetItem(), pError, nullptr, nullptr);
                }
                pNode = pNode->GetNext();
            }
            pIngredients->Sort(&RecipeMenu::SortFunc);
            if (pActor) {
                ActorValueOwner& rOwner = *pActor;
                char acSkill[256];
                acSkill[0] = '\0';
                if (pCurrItem->GetRequiredSkill() > -1) {
                    int32_t iLevel = pCurrItem->GetRequiredSkillLevel();
                    int32_t iValue = rOwner.GetClampedActorValue(pCurrItem->GetRequiredSkill());
                    const char* pSkillName = ActorValue::GetActorValueName(pCurrItem->GetRequiredSkill());
                    BSsprintf(acSkill, 0x100, "%s (%d/%d)", pSkillName, iValue, iLevel);
                }
                pMe->pTiles[5]->Set(Tile::kTileValue_String, acSkill, true);
                bool bLacking = false;
                if (pCurrItem->GetRequiredSkill() > -1) {
                    int32_t iValue = rOwner.GetClampedActorValue(pCurrItem->GetRequiredSkill());
                    bLacking = iValue < pCurrItem->GetRequiredSkillLevel();
                }
                if (bLacking) {
                    pMe->pTiles[5]->Set(Tile::kTileValue_Alpha, 127);
                }
                else {
                    pMe->pTiles[5]->Set(Tile::kTileValue_Alpha, 255);
                }
            }
            if (CanMakeRecipe(pCurrItem)) {
                pMe->pTiles[7]->Set(Tile::kTileValue_Target, 1);
            }
            else {
                pMe->pTiles[7]->Set(Tile::kTileValue_Target, 0);
            }
        }
    }

    if (!pItem) {
        pTiles[9]->Set(Tile::kTileValue_Visible, 0);
        return;
    }
    PopulateItemStatsDisplay(pItem->GetComponentPointer(), pTiles[10]);
    pTiles[10]->Set(Tile::kTileValue_Visible, 1);
    TESIcon* pIcon = dynamic_cast<TESIcon*>(pItem->GetComponentPointer());
    if (pIcon && pIcon->GetTextureNameLength()) {
        pTiles[9]->Set(Tile::kTileValue_Filename, pIcon->GetTextureName(), true);
        pTiles[9]->Set(Tile::kTileValue_Visible, 1);
        return;
    }
    TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(pItem->GetComponentPointer());
    if (pArmor && TESBipedModelForm::GetMessageIcon(pArmor, pPlayer)) {
        TESBipedModelForm& rBiped = *pArmor;
        pTiles[9]->Set(Tile::kTileValue_Filename, rBiped.GetMessageIcon(pPlayer->GetSex()), true);
        pTiles[9]->Set(Tile::kTileValue_Visible, 1);
    }
    else {
        pTiles[9]->Set(Tile::kTileValue_Visible, 0);
    }
}

void RecipeMenu::DoIdle() {
    double v1;
    float v2;
    float afValue;
    float v4;
    float aVal;
    uint32_t v6;
    float v7;
    uint32_t time;
    if (g_bShouldRefreshRecipeMenu) {
        RecipeMenu::ForceRecipeUpdate(true);
    }
    if (bPulseSelection) {
        v7 = 485.0f;
        v2 = (-230.0f);
        v4 = 0x1p-2f;
        afValue = RecipeMenu_afValue;
        BSClamp(afValue, (float)0.0, 255.0f);
        aVal = (float)(255.0 - afValue);
        BSClamp(aVal, (float)0.0, 255.0f);
        if ((pCrossFadeTile2) && (pCrossFadeTile1)) {
            pCrossFadeTile2->Set(4009, aVal, true);
            pCrossFadeTile1->Set(4009, afValue, true);
        }

        time = (&appTimer)->BSTimer::GetTime();
        v6 = time - uiLastIdleTime;
        uiLastIdleTime = time;
        if (bCrossFade) {
            v1 = RecipeMenu_afValue - (((double)v6) * v4);
        }
        else {
            v1 = (((double)v6) * v4) + RecipeMenu_afValue;
        }
        RecipeMenu_afValue = (float)v1;
        BSClamp(RecipeMenu_afValue, v2, v7);
        if (v7 <= ((double)RecipeMenu_afValue)) {
            bCrossFade = true;
        }
        if (v2 >= ((double)RecipeMenu_afValue)) {
            bCrossFade = false;
        }
    }
}

int32_t RecipeMenu::GetClass() {
    return 1077;
}

bool RecipeMenu::DoGamepad(int32_t a2, float a3) {
    int8_t v5 = 0;
    if (Interface::IsTopMenuID(static_cast<Interface::enumMenus>(1077))) {
        switch (a2) {
            case 3:
            case 4: {
                DoClick((a2 != 4) ? 2 : 0, nullptr);
                v5 = 1;
                break;
            }
            case 13: {
                DoEnter(3, nullptr);
                RecipeMenu::bControlIngredientList = false;
                v5 = 1;
                break;
            }
            case 14: {
                DoEnter(6, nullptr);
                RecipeMenu::bControlIngredientList = true;
                v5 = 1;
                break;
            }
            default: {
                return (bool)v5;
            }
        }
    }
    return (bool)v5;
}

void RecipeMenu::UpdateLists(TESBoundObject* apUpdateObj, TESForm* apCategory) {
    TESRecipeCategory* pThisSubCategory;
    TESRecipeCategory* pCompCategory = static_cast<TESRecipeCategory*>(apCategory);
    BSSimpleList<TESRecipe*>* pRecipeList = pDataHandler->GetRecipeList();
    int32_t v17 = 0;
    while (pRecipeList && !pRecipeList->IsEmpty()) {
        TESRecipe* pThisRecipe = pRecipeList->m_item;
        pRecipeList = pRecipeList->m_pkNext;
        TESRecipeCategory* pThisCategory = pThisRecipe->pRecipeCat;
        if ((!pCompCategory || pCompCategory == pThisCategory) &&
            (&pThisRecipe->RecipeConditions)->IsTrue(RecipeMenu::pMe->pCrafterRef, nullptr)) {
            char apDest[256];
            pThisSubCategory = pThisRecipe->GetSubCategory();
            if (pThisSubCategory) {
                BSSimpleArray<TESRecipeCategory*, 1024>* g_RecipeMenuCategories = get_g_RecipeMenuCategories();
                uint32_t v4 = g_RecipeMenuCategories->Find(
                    pThisSubCategory, 0, &BSSimpleArray<TESRecipeCategory*>::DefaultEqualFunctionByAddress
                );
                BSSimpleArray<TESRecipeCategory*, 1024>* v5 = get_g_RecipeMenuCategories();
                if (v4 >= v5->iSize) {
                    BSSimpleArray<TESRecipeCategory*, 1024>* v6 = get_g_RecipeMenuCategories();
                    v6->Add(pThisSubCategory);
                    ++v17;
                }
            }
            if ((&pThisRecipe->outputlist)->m_item && (&pThisRecipe->outputlist)->m_item->Quantity > 1) {
                int32_t Quantity = (&pThisRecipe->outputlist)->m_item->Quantity;
                BSsprintf(apDest, 256, "%s (%d)", pThisRecipe->GetFullName(), Quantity);
            }
            else {
                BSsprintf(apDest, 256, "%s", pThisRecipe->GetFullName());
            }
            Tile* pselectedTile = lbRecipeList.AddItem(pThisRecipe, apDest, nullptr, nullptr);
            if (RecipeMenu::CanMakeRecipe(pThisRecipe)) {
                pselectedTile->xChildren.GetTail()->Set(4009, 255);
            }
            else {
                pselectedTile->xChildren.GetTail()->Set(4009, 127);
            }
        }
    }
    // Sort is called through a pointer so that the call stays virtual; a call on the member itself would be
    // devirtualised.
    ListBox<TESRecipe*>* plbRecipes = &lbRecipeList;
    plbRecipes->Sort(&TESRecipe::CompareForSorting);
    RecipeMenu::filterlist.Sort(&RecipeMenu::FilterSortFunction);
}

// The display tile's last twelve children, last first, are the stat tiles: 0 damage resistance, 1 damage per second, 2
// weight, 3 value, 4 condition, 5 ammo, 6 effects, 7 mod effects, 8 and 9 two more rows, 10 strength requirement (may
// be missing), 11 damage; missing children are null. Each gets its value and title; uiVisible picks the rows shown. A
// NaN value or weight shows "--"; a NaN weapon condition still asks for the damage. this is unused.
void RecipeMenu::PopulateItemStatsDisplay(TESForm* apCurrItem, Tile* apDisplayTile) {
    uint32_t uiVisible = 0;
    int32_t iTitleTrait = Tile::TextToTrait("_Title");
    int32_t iValueTrait = Tile::TextToTrait("_Value");
    int32_t iConditionTrait = Tile::kTileValue_User5;
    float fRow8Offset = 0.0f;
    float fRow9Offset = 0.0f;
    float fValue =
        GamePlayFormulas::CalcItemHealthValue(static_cast<float>(TESValueForm::GetFormValue(apCurrItem)), 1.0f);
    float fWeight = TESWeightForm::GetFormWeight(apCurrItem, pPlayer->IsHardcore());
    if (!apCurrItem || !apDisplayTile) {
        return;
    }
    Tile* pTiles[12];
    NiTListItem<Tile*>* pPos = apDisplayTile->xChildren.GetTailPos();
    int32_t i;
    for (i = 0; pPos && i < 12; ++i) {
        pTiles[i] = apDisplayTile->xChildren.GetPrev(pPos);
    }
    for (; i < 12; ++i) {
        pTiles[i] = nullptr;
    }
    char acBuffer[128];
    switch (apCurrItem->GetFormType()) {
        case ARMO_ID:
        case CLOT_ID: {
            float fCondition = 1.0f;
            char acRating[128];
            int32_t iRating = static_cast<int32_t>(static_cast<TESObjectARMO*>(apCurrItem)->GetArmorRating());
            float fRating = CombatFormulas::CalcArmorRating(static_cast<uint16_t>(iRating), fCondition);
            BSsprintf(acRating, 0x80, "%d", static_cast<int32_t>(FloatCeil(fRating)));
            pTiles[0]->Set(iValueTrait, acRating, true);
            uiVisible = 0x1D;
            break;
        }
        case WEAP_ID: {
            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apCurrItem);
            uint32_t uiStrength = pWeapon->GetStrRequired();
            float fDamage = 0.0f;
            float fDPS = 0.0f;
            float fCondition = 1.0f;
            fDPS = CombatUtilities::CalculateItemDPS(
                static_cast<ActorValueOwner*>(pPlayer), pWeapon, fCondition, true, nullptr, false, false, -1, 0.0f,
                0.0f, false, false, nullptr
            );
            if (fCondition <= 0.0) {
                fDamage = 0.0f;
            }
            else {
                fDamage = CombatFormulas::CalcWeaponDamageForDisplay(pWeapon, fCondition, nullptr, nullptr);
            }
            BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(BSRoundToNearest(fDPS, 1.0f)));
            pTiles[1]->Set(iValueTrait, acBuffer, true);
            pCrossFadeTile1 = pTiles[1];
            uint32_t uiProjectiles = pWeapon->GetNumProjectiles(false, false, nullptr);
            if (uiProjectiles > 1) {
                float fPerProjectile = fDamage / static_cast<float>(uiProjectiles);
                BSsprintf(acBuffer, 0x80, "%.1fx%d", fPerProjectile, uiProjectiles);
            }
            else {
                BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(BSRoundToNearest(fDamage, 1.0f)));
            }
            pTiles[11]->Set(iValueTrait, acBuffer, true);
            pCrossFadeTile2 = pTiles[11];
            pTiles[4]->Set(iConditionTrait, BSMIN(fCondition, 1.0f), true);
            BSsprintf(acBuffer, 0x80, "%d", uiStrength);
            if (pTiles[10]) {
                pTiles[10]->Set(iValueTrait, acBuffer, true);
            }
            pAmmoTile = pTiles[10];
            if (pWeapon->GetCurrentAmmo(nullptr)) {
                // Asks for the player's current weapon and ammo and drops both, as the game does.
                pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
                pPlayer->GetCurrentAIProcess()->GetCurrentAmmo();
                const char* pName = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(nullptr))->GetShortName();
                TESForm* pAmmo = pWeapon->GetCurrentAmmo(nullptr);
                InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
                int32_t iCount = pChanges->GetObjectCount(static_cast<TESBoundObject*>(pAmmo));
                int32_t iLoaded = 0;
                if (!pName || !*pName) {
                    TESAmmo* pNamedAmmo = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(nullptr));
                    pName = static_cast<TESFullName*>(pNamedAmmo)->GetFullName();
                }
                bool bRegenerates = pWeapon->GetAmmoRegenRate(false) != 0.0;
                if (bRegenerates) {
                    BSsprintf(acBuffer, 0x80, "%s", pName);
                }
                else {
                    BSsprintf(acBuffer, 0x80, "%s (%i/%i)", pName, iLoaded, iCount - iLoaded);
                }
            }
            else {
                BSstrcpy(acBuffer, 0x80, "--");
            }
            pTiles[5]->Set(iTitleTrait, acBuffer, true);
            uiVisible = 0xC3E;
            break;
        }
        default: uiVisible = 0xC; break;
    }
    EffectItemList* pEffects = nullptr;
    TESBoundObject* pBound = dynamic_cast<TESBoundObject*>(apCurrItem);
    EnchantmentItem* pEnchantment = TESEnchantableForm::GetFormEnchanting(pBound);
    if (pEnchantment && !pEnchantment->GetFlag(4)) {
        pEffects = pEnchantment;
    }
    if (apCurrItem->GetFormType() == INGR_ID) {
        pEffects = static_cast<IngredientItem*>(apCurrItem);
    }
    else if (apCurrItem->GetFormType() == ALCH_ID) {
        pEffects = static_cast<AlchemyItem*>(pBound);
    }
    else {
        EnchantmentItem* pItemEnchantment = TESEnchantableForm::GetFormEnchanting(apCurrItem);
        if (pItemEnchantment && !pItemEnchantment->GetFlag(4)) {
            pEffects = pItemEnchantment;
        }
    }
    char acEffects[256];
    if (pEffects && pEffects->GetItem()) {
        pEffects->GetEffectsString(acEffects, 0x100);
        if (BSstrlen(acEffects) > 0) {
            pTiles[6]->Set(iValueTrait, acEffects, true);
            uiVisible |= 0x40;
        }
        else {
            uiVisible &= ~0x40u;
        }
    }
    else {
        uiVisible &= ~0x40u;
    }
    char acNumber[16];
    if (fValue > 0.0) {
        if (fValue >= 1.0) {
            BSsprintf(acNumber, 0x10, "%.0f", fValue);
        }
        else {
            BSsprintf(acNumber, 0x10, "%.1f", fValue);
        }
    }
    else {
        BSstrcpy(acNumber, 0x10, "--");
    }
    pTiles[3]->Set(iValueTrait, acNumber, true);
    if (fWeight > 0.0) {
        if (fWeight >= 1.0) {
            BSsprintf(acNumber, 0x10, "%.0f", fWeight);
        }
        else {
            BSsprintf(acNumber, 0x10, "%.1f", fWeight);
        }
    }
    else {
        BSstrcpy(acNumber, 0x10, "--");
    }
    pTiles[2]->Set(iValueTrait, acNumber, true);
    pTiles[0]->Set(iTitleTrait, sInventoryDamageResistance.String(), true);
    pTiles[1]->Set(iTitleTrait, sInventoryDamagePerSecond.String(), true);
    pTiles[11]->Set(iTitleTrait, sInventoryDamage.String(), true);
    pTiles[2]->Set(iTitleTrait, sInventoryWeightUpper.String(), true);
    pTiles[3]->Set(iTitleTrait, sInventoryValue.String(), true);
    pTiles[4]->Set(iTitleTrait, sInventoryCondition.String(), true);
    pTiles[6]->Set(iTitleTrait, sInventoryEffects.String(), true);
    pTiles[7]->Set(iTitleTrait, sModEffects.String(), true);
    pTiles[10]->Set(iTitleTrait, sInventoryStrReq.String(), true);
    pTiles[0]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 1) != 0));
    pTiles[1]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 2) != 0));
    pTiles[11]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x800) != 0));
    pTiles[2]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 4) != 0));
    pTiles[3]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 8) != 0));
    pTiles[4]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x10) != 0));
    pTiles[5]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x20) != 0));
    pTiles[6]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x40) != 0));
    pTiles[7]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x80) != 0));
    pTiles[8]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x100) != 0));
    pTiles[9]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x200) != 0));
    if (pTiles[10]) {
        pTiles[10]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x400) != 0));
        pTiles[10]->Set(Tile::kTileValue_Alpha, 255.0f, true);
    }
    float fY = apDisplayTile->GetFloat(Tile::kTileValue_Height) / 2.0 - 20.0;
    if ((uiVisible & 0x10) || (uiVisible & 0x20)) {
        fY = fY + fY;
    }
    pTiles[6]->Set(Tile::kTileValue_Y, fY, true);
    pTiles[7]->Set(Tile::kTileValue_Y, fY, true);
    float fRow8Y = fY + fRow8Offset;
    pTiles[8]->Set(Tile::kTileValue_Y, fRow8Y, true);
    float fRow9Y = fY + fRow9Offset;
    pTiles[9]->Set(Tile::kTileValue_Y, fRow9Y, true);
}

bool RecipeMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 14; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

Tile* RecipeMenu::Create(TESObjectREFR* apVendor, TESForm* apCategory) {
    InterfaceManager* pim = InterfaceManager::Instance();
    if (apVendor) {
        int32_t v4;
        RecipeMenu* Menu_;
        int32_t ClassStatic = RecipeMenu::GetClassStatic();
        Tile* pold_menu_tile = Menu::GetMenuByClass(ClassStatic);
        if (pold_menu_tile) {
            delete pold_menu_tile;
        }
        Tile* ptile = pim->GetMenusRoot()->ReadFile("Data\\Menus\\recipe_menu.xml");
        if (ptile) {
            Menu_ = static_cast<RecipeMenu*>(ptile->GetMenu());
        }
        else {
            Menu_ = nullptr;
        }
        RecipeMenu::pMe = Menu_;
        pCrossFadeTile1 = nullptr;
        pCrossFadeTile2 = nullptr;
        pAmmoTile = nullptr;
        if (Menu_ && (v4 = RecipeMenu::pMe->GetClass(), v4 == RecipeMenu::GetClassStatic())) {
            RecipeMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            if (!RecipeMenu::pMe->IsWellFormedMenu()) {
                Warning("MENUS: Recipe Menu Creation Failed... Are your menu and art resources up to date?");
            }
            if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                float afValue = Tile::GetMaximumDepth();
                ptile->Set(4013, afValue, true);
            }
            DialogMenu::ServiceStart();
            RecipeMenu::pMe->lbRecipeList.Initialize(RecipeMenu::pMe->pTiles[3], "CM_list_template");
            RecipeMenu::pMe->lbIngredientList.Initialize(RecipeMenu::pMe->pTiles[6], "CM_list_template");
            const char* v5 = sRecipes.String();
            RecipeMenu::pMe->pTiles[1]->Set(4036, v5, true);
            const char* v6 = sSkillRequirement.String();
            RecipeMenu::pMe->pTiles[13]->Set(4036, v6, true);
            const char* v7 = sIngredients.String();
            RecipeMenu::pMe->pTiles[11]->Set(4036, v7, true);
            const char* v8 = sMadeAt.String();
            RecipeMenu::pMe->pTiles[12]->Set(4036, v8, true);
            const char* v9 = sAccept.String();
            RecipeMenu::pMe->pTiles[7]->Set(4036, v9, true);
            const char* v10 = sExit.String();
            RecipeMenu::pMe->pTiles[8]->Set(4036, v10, true);
            float v16 = RecipeMenu::pMe->pTiles[7]->GetFloat(4001) - 20.0;
            RecipeMenu::pMe->pTiles[7]->Set(4001, v16, true);
            float v15 = RecipeMenu::pMe->pTiles[8]->GetFloat(4001) - 20.0;
            RecipeMenu::pMe->pTiles[8]->Set(4001, v15, true);
            RecipeMenu::pMe->pCrafterRef = apVendor;
            RecipeMenu::pMe->pCategory = apCategory;
            InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
            InterfaceManager* v11 = InterfaceManager::Instance();
            if (!v11->xTutorialManager.HasBeenShown(Interface::TUT_RECIPE)) {
                TESForm* FormByEditorID = TESForm::GetFormByEditorID("HelpCrafting");
                BGSMessage* helpForm = dynamic_cast<BGSMessage*>(FormByEditorID);
                if (helpForm) {
                    RecipeMenu::pMe->bTutorial = TutorialMenu::Create(helpForm, false);
                    InterfaceManager* v13 = InterfaceManager::Instance();
                    v13->xTutorialManager.FlagAsShown(Interface::TUT_RECIPE);
                }
            }
            RecipeMenu::pMe->PrepForVisibility(false);
            RecipeMenu::pMe->UpdateLists(nullptr, apCategory);
            return ptile;
        }

        if ((RecipeMenu::pMe) && (RecipeMenu::pMe->pMenu) && (RecipeMenu::pMe)) {
            delete RecipeMenu::pMe;
        }

        return nullptr;
    }

    Warning(WARN_MENUS, "Recipe menu called with NULL vendor!  Oh, noes!");
    return nullptr;
}

void RecipeMenu::ForceRecipeUpdate(bool abUpdateNow) {
    if (abUpdateNow) {
        if (RecipeMenu::pMe) {
            RecipeMenu::pMe->UpdateLists(nullptr, nullptr);
        }
        g_bShouldRefreshRecipeMenu = false;
    }
    else {
        g_bShouldRefreshRecipeMenu = true;
    }
}

int32_t RecipeMenu::GetClassStatic() {
    return 1077;
}

int32_t RecipeMenu::CanMakeRecipe(TESRecipe* apRecipe) {
    Actor* pactor;
    if (RecipeMenu::pMe->pCrafterRef && RecipeMenu::pMe->pCrafterRef->IsActor()) {
        pactor = static_cast<Actor*>(RecipeMenu::pMe->pCrafterRef);
    }
    else {
        pactor = nullptr;
    }
    RecipeMenu::iNumCanMake = 100;
    if (!apRecipe->pRecipeCat || !apRecipe->GetSubCategory()) {
        return 0;
    }
    if (pactor) {
        if (apRecipe->GetRequiredSkill() > ActorValue::VALUE_NONE) {
            int32_t ClampedActorValue = pactor->GetClampedActorValue(apRecipe->GetRequiredSkill());
            if (ClampedActorValue < apRecipe->data.ReqSkillLevel) {
                return 0;
            }
        }
        TESRecipeCategory* Category = apRecipe->pRecipeCat;
        if (RecipeMenu::pMe->pCategory != Category) {
            return 0;
        }
        for (BSSimpleList<TESRecipeComponent*>* pComponentList = &apRecipe->ingredientlist;
             pComponentList && !pComponentList->IsEmpty(); pComponentList = pComponentList->m_pkNext) {
            TESRecipeComponent* pComp = pComponentList->m_item;
            TESBoundObject* pingredientBO = dynamic_cast<TESBoundObject*>(pComponentList->m_item->pInternalItem);
            if (pingredientBO) {
                int32_t iCount;
                ItemChange* pingitem = pactor->GetInventoryItem(pingredientBO, 0);
                if (pingitem) {
                    iCount = pingitem->iNumber;
                }
                else {
                    iCount = 0;
                }
                if (pingitem) {
                    delete pingitem;
                }
                if (iCount < pComp->Quantity) {
                    return 0;
                }
                int32_t localNumCanMake = iCount / pComp->Quantity;
                if (localNumCanMake < RecipeMenu::iNumCanMake) {
                    RecipeMenu::iNumCanMake = localNumCanMake;
                }
            }
        }
    }
    return RecipeMenu::iNumCanMake;
}

void RecipeMenu::Close() {
    if (RecipeMenu::pMe) {
        BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
        if (CurrentAIProcess->Get3DUpdateFlag(RESET_MODEL)) {
            pPlayer->UpdatePlayer3d();
        }
        DialogMenu::ServiceEnd(nullptr);
        RecipeMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        RecipeMenu::pMe->StartFadeOut();
    }
}

int32_t RecipeMenu::FilterSortFunction(const void* apPtr1, const void* apPtr2) {
    TESRecipeCategory* pcat1 = *static_cast<TESRecipeCategory* const*>(apPtr1);
    TESRecipeCategory* pcat2 = *static_cast<TESRecipeCategory* const*>(apPtr2);
    // Takes the first name, then the second, before comparing: the locals keep that call order.
    const char* pName1 = pcat1->GetFullName();
    const char* pName2 = pcat2->GetFullName();
    return _tcscmp(pName1, pName2);
}

void RecipeMenu::ItemSelectCallback(int32_t aiAmountSelected) {
    Tile* pcurrTiple = RecipeMenu::pMe->plbCurrListbox->GetTileFor(RecipeMenu::pCurrItem);
    BSSimpleList<TESRecipeComponent*>* pCompList = &RecipeMenu::pCurrItem->ingredientlist;
    TESForm* apObj = nullptr;
    TESForm* pForm = nullptr;
    int32_t iQuantity = 0;
    if (aiAmountSelected) {
        char v23 = 0;
        char v24 = 0;
        while (pCompList && !pCompList->IsEmpty()) {
            pForm = pCompList->m_item->pInternalItem;
            if (!v23) {
                TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_CRAFT, 1, pForm, nullptr, 0, 0, 0);
                if (pForm->GetFormType() == CHIP_ID) {
                    v24 = 1;
                }
            }
            v23 = 1;
            iQuantity = pCompList->m_item->Quantity;
            if ((pForm) && (pForm->IsBoundObject())) {
                apObj = pForm;
            }

            if (apObj) {
                Actor* pactor;
                bool bAmmo = false;
                if (RecipeMenu::pMe->pCrafterRef && RecipeMenu::pMe->pCrafterRef->IsActor()) {
                    pactor = static_cast<Actor*>(RecipeMenu::pMe->pCrafterRef);
                }
                else {
                    pactor = nullptr;
                }
                Actor* pactor_2 = pactor;
                if ((pactor) && (pactor_2->GetCurrentWeapon())) {
                    PlayerCharacter* a2 = pPlayer;
                    TESObjectWEAP* CurrentWeapon = pactor_2->GetCurrentWeapon();
                    TESAmmo* CurrentAmmo = CurrentWeapon->GetCurrentAmmo(a2);
                    bAmmo = CurrentAmmo == pCompList->m_item->pInternalItem;
                }

                if (apObj->GetFormType() == WEAP_ID &&
                    pactor_2->GetInventoryObjectCount(static_cast<TESBoundObject*>(apObj)) == 1 &&
                    apObj == pactor_2->GetCurrentWeapon()) {
                    pactor_2->UnEquipObject(static_cast<TESBoundObject*>(apObj), 1, nullptr, false, false, true);
                }
                RecipeMenu::pMe->pCrafterRef->RemoveItem(
                    static_cast<TESBoundObject*>(apObj), nullptr, aiAmountSelected * iQuantity, false, false, nullptr,
                    nullptr, nullptr, true, false
                );
                if (bAmmo) {
                    BaseProcess* CurrentAIProcess = pactor_2->pCurrentProcess;
                    CurrentAIProcess->GetCurrentAmmo()->iNumber = 0;
                }
            }
            pCompList = pCompList->m_pkNext;
        }
        if (!v24) {
            MiscStatManager::ModVal(MiscStatManager::MS_ITEMS_CRAFTED, aiAmountSelected);
        }
        pCompList = &RecipeMenu::pCurrItem->outputlist;
        TESBoundObject* pobject = nullptr;
        pForm = nullptr;
        iQuantity = 0;
        while (true) {
            if (!pCompList || pCompList->IsEmpty()) {
                RecipeMenu::Close();
                return;
            }
            pForm = pCompList->m_item->pInternalItem;
            iQuantity = pCompList->m_item->Quantity;
            if ((pForm) && (pForm->IsBoundObject())) {
                pobject = static_cast<TESBoundObject*>(pForm);
            }

            if (pobject) {
                break;
            }
        LABEL_50:;
            pCompList = pCompList->m_pkNext;
        }
        if (pobject->GetFormType() == WEAP_ID) {
            TESObjectWEAP* v13 = static_cast<TESObjectWEAP*>(pobject);
            if (!static_cast<TESObjectWEAP*>(pobject)->TESObjectWEAP::IsThrownWeapon()) {
                TESContainer apMemory;
                if (pobject) {
                    apMemory.AddObject(pobject, aiAmountSelected * iQuantity, nullptr);
                }
                apMemory.ApplyHealthPercentToContainer(0.8f);
                apMemory.AddContainerToReference(RecipeMenu::pMe->pCrafterRef, false);
                goto LABEL_46;
            }
        }
        else {
            if (pobject->GetFormType() == ARMO_ID || pobject->GetFormType() == CLOT_ID) {
                TESContainer apContainer;
                if (pobject) {
                    apContainer.AddObject(pobject, aiAmountSelected * iQuantity, nullptr);
                }
                apContainer.ApplyHealthPercentToContainer(0.8f);
                apContainer.AddContainerToReference(RecipeMenu::pMe->pCrafterRef, false);
                goto LABEL_46;
            }
        }
        RecipeMenu::pMe->pCrafterRef->AddBoundObjectItem(pobject, nullptr, aiAmountSelected * iQuantity);
    LABEL_46:;
        BSStringT<char> pstring;
        if (aiAmountSelected * iQuantity <= 1) {
            const char* a2b = sAddItemtoInventory.String();
            pstring.SPrintF("%s %s", TESFullName::GetFullName(pobject), a2b);
        }
        else {
            const char* a2a = sAddItemtoInventory.String();
            const char* time = sPlural.String();
            const char* FullName = TESFullName::GetFullName(pobject);
            pstring.SPrintF("%i %s%s %s", aiAmountSelected * iQuantity, FullName, time, a2a);
        }
        const char* PickUpSoundName = pPlayer->GetPickUpSoundName(pobject, true, false);
        Interface::ShowMessage(
            pstring.pString, 0, "Interface\\Icons\\Message Icons\\glow_message_giftbox.dds", PickUpSoundName, 2.0f,
            false
        );
        goto LABEL_50;
    }
}

int32_t RecipeMenu::SortFunc(ListBoxItem<TESRecipeComponent*>* apItem1, ListBoxItem<TESRecipeComponent*>* apItem2) {
    TESFullName* pfullname1 = dynamic_cast<TESFullName*>(apItem1->data->pInternalItem);
    TESFullName* pfullname2 = dynamic_cast<TESFullName*>(apItem2->data->pInternalItem);
    // Takes the first name, then the second, before comparing: the locals keep that call order.
    const char* pName1 = pfullname1->GetFullName();
    const char* pName2 = pfullname2->GetFullName();
    return _tcscmp(pName1, pName2);
}
