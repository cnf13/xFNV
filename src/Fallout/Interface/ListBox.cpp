#include "Fallout/Interface/ListBox.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TESChallenge.h"
#include "Fallout Shared/TESConditionItem.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESRecipe.h"
#include "Fallout Shared/TESReputation.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.inl"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/tchar.h"

#include <cmath>
#include <cstdint>

using ItemChangeListBox = ListBox<ItemChange*>;

int32_t SortByName(ItemChange* apItemA, ItemChange* apItemB) {
    const char* fullName = apItemB->GetFullName();
    const char* v2 = apItemA->GetFullName();
    return _tcscmp(v2, fullName);
}

int32_t SortByCount(ItemChange* apItemA, ItemChange* apItemB) {
    if (apItemA->iNumber < apItemB->iNumber) {
        return -1;
    }
    return apItemA->iNumber > apItemB->iNumber;
}

int32_t SortByHealth(ItemChange* apItemA, ItemChange* apItemB) {
    TESHealthForm* pHealthA = dynamic_cast<TESHealthForm*>(apItemA->pContainerObj);
    TESHealthForm* pHealthB = dynamic_cast<TESHealthForm*>(apItemB->pContainerObj);
    uint32_t uiHealthA = pHealthA->GetFormHealth();
    if (uiHealthA < pHealthB->GetFormHealth()) {
        return -1;
    }
    uiHealthA = pHealthA->GetFormHealth();
    return uiHealthA > pHealthB->GetFormHealth();
}

bool IsWeapon(ItemChange* apItem) {
    return apItem && apItem->pContainerObj->GetFormType() == WEAP_ID;
}

bool IsApparel(ItemChange* apItem) {
    if (!apItem) {
        return false;
    }
    return apItem->pContainerObj->GetFormType() == CLOT_ID || apItem->pContainerObj->GetFormType() == ARMO_ID;
}

bool IsMiscItem(ItemChange* apItem) {
    if (!apItem) {
        return false;
    }
    return apItem->pContainerObj->GetFormType() == MISC_ID || apItem->pContainerObj->GetFormType() == AMMO_ID ||
        apItem->pContainerObj->GetFormType() == IMOD_ID || apItem->pContainerObj->GetFormType() == COBJ_ID ||
        apItem->pContainerObj->GetFormType() == CHIP_ID || apItem->pContainerObj->GetFormType() == CMNY_ID ||
        apItem->pContainerObj->GetFormType() == CCRD_ID;
}

// The standalone build's one instantiation of every list's generic members.
template class ListBox<ActorValue::Index>;
template class ListBox<MiscStatManager::MiscStatID>;
template class ListBox<StatsMenu::StatusEffect*>;
template class ListBox<PerkRankData*>;
template class ListBox<TESReputation*>;
template class ListBox<uint32_t>;
template class ListBox<int32_t>;
template class ListBox<TESRecipe*>;
template class ListBox<TESRecipeComponent*>;
template class ListBox<TESConditionItem*>;
template class ListBox<TERMINAL_MENU_ITEM*>;
template class ListBox<BGSPerk*>;
template class ListBox<TESQuest*>;
template class ListBox<BGSNote*>;
template class ListBox<TESObjectREFR*>;
template class ListBox<BGSQuestObjective*>;
template class ListBox<TESChallenge*>;
template class ListBox<StartMenu::Option*>;
template class ListBox<BGSSaveLoadFileEntry*>;

// RemoveItemGroup and BuildFromContainer work on ItemChange data: their constraint keeps every other list's
// instantiation from compiling them.
template <class T>
void ListBox<T>::RemoveItemGroup(TESBoundObject* apRemoveObj)
requires std::same_as<T, ItemChange*>
{
    BSSimpleList<ListBoxItem<T>*> kRemoveList;
    for (ListBoxItem<T>*& rpItem : static_cast<BSSimpleList<ListBoxItem<T>*>*>(this)->ItemsUntilNull()) {
        if (rpItem->data && rpItem->data->pContainerObj == apRemoveObj) {
            kRemoveList.AddHead(rpItem);
        }
    }
    for (ListBoxItem<T>*& rpItem : kRemoveList.ItemsUntilNull()) {
        RemoveItem(rpItem->pTile);
    }
    kRemoveList.RemoveAll();
}

// The data is the inventory's ItemChange objects.
template <class T>
void ListBox<T>::BuildFromContainer(
    TESObjectREFR* apContainer,
    bool (*apFilterFn)(T),
    int32_t (*apCompareFn)(ListBoxItem<T>*, ListBoxItem<T>*),
    void (*apInitFn)(Tile*, T),
    bool abNoStack,
    bool abBarter
)
requires std::same_as<T, ItemChange*>
{
    if (!apContainer || !pContainerRect || !pTemplateName) {
        return;
    }
    RemoveAll();
    SetFlag(LBF_FROM_CONTAINER, true);
    bool bSaveSetting = GetFlag(LBF_AUTO_UPDATE);
    SetFlag(LBF_AUTO_UPDATE, false);
    InventoryChanges::IterData* pIterator;
    InventoryChanges* pChanges;
    apContainer->GetInventoryIterator(abBarter, &pIterator, &pChanges);
    if (pIterator && pChanges) {
        for (ItemChange* pItem = pChanges->Iterate(pIterator); pItem; pItem = pChanges->Iterate(pIterator)) {
            if (apFilterFn && apFilterFn(pItem)) {
                continue;
            }
            // Without stacking, one item per unit: the first keeps the change (its count set to 1), the others
            // get a copy of it.
            int32_t iPerItemCount = 0;
            int32_t iCount = pItem->iNumber;
            do {
                char acName[128];
                TESFullName* pFullName = dynamic_cast<TESFullName*>(pItem->pContainerObj);
                uint32_t uiLength = BSstrlen(pFullName->GetFullName());
                BSstrcpy(acName, uiLength + 1, pFullName->GetFullName());
                if (pItem->GetModSlots()) {
                    BSstrcat(acName, uiLength + 2, "+");
                }
                if (abNoStack) {
                    if (iPerItemCount <= 0) {
                        if (iCount > 1) {
                            pItem->iNumber = 1;
                        }
                    }
                    else {
                        ItemChange* pCopy = new ItemChange();
                        pCopy->Copy(pItem);
                        pItem = pCopy;
                        pCopy->iNumber = 1;
                    }
                    ++iPerItemCount;
                }
                Tile* pTile = AddItem(pItem, acName, apCompareFn, nullptr);
                if (apInitFn) {
                    apInitFn(pTile, pItem);
                }
            }
            while (abNoStack && iPerItemCount < iCount);
        }
        delete pIterator;
    }
    if (apCompareFn) {
        Synchronize();
    }
    SetFlag(LBF_AUTO_UPDATE, bSaveSetting);
}

template class ListBox<ItemChange*>;
