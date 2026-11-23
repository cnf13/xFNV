#include "Fallout/Interface/Menus/ItemModMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectIMOD.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/ContainerMenu.h"
#include "Fallout/Interface/Menus/InventoryMenu.h"
#include "Fallout/Interface/Menus/RepairMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

#include <limits>

ItemModMenu::ItemModMenu() {
    g_ItemModMenuTrait_Title = Tile::TextToTrait("_Title");
    g_ItemModMenuTrait_Value = Tile::TextToTrait("_Value");
    g_ItemModMenuTrait_enabled = Tile::TextToTrait("_enabled");
    g_ItemModMenuTrait_IsItemModItem = Tile::TextToTrait("_IsItemModItem");
    ItemModMenu::bIsEquipped = false;
}

ItemModMenu::~ItemModMenu() {
    pTiles[8]->RemoveFadeControl(4009);
    ItemModMenu::pMe = nullptr;
}

void ItemModMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 14) {
        pTiles[aiID] = apTile;
    }
}

void ItemModMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 14) {
        apTarget = pTiles[aiID];
    }
    if (aiID == 12) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
        if (ItemModMenu::bIsEquipped) {
            ExtraDataList* pextra = ItemModMenu::pBrokenItem->pExtraObjectList->m_item;
            uint32_t uiCount = ItemModMenu::pBrokenItem->iNumber;
            TESBoundObject* pobject = ItemModMenu::pBrokenItem->pContainerObj;
            pPlayer->QueueEquipObject(pobject, uiCount, pextra, true, false, true);
            ItemModMenu::pBrokenItem->RemoveIsModding();
            ItemModMenu::bIsEquipped = false;
        }
        Interface::SetInventoryMenuVisible(true, false, false);
    }
    else if (aiID == 15 && !(apTarget->xChildren.GetTail()->GetFloat(4009) < 255.0)) {
        ItemChange* phighlighted = lbItemModList.GetHighlightedData();
        if (phighlighted != ItemModMenu::pBrokenItem) {
            ItemChange* psparePart = lbItemModList.GetHighlightedData();
            ExtraDataList* psparePartExtra = psparePart->pExtraObjectList ? psparePart->pExtraObjectList->m_item
                                                                          : nullptr;
            if (ItemModItem(psparePart)) {
                pPlayer->RemoveItem(
                    psparePart->pContainerObj,
                    psparePartExtra,
                    1,
                    false,
                    false,
                    nullptr,
                    nullptr,
                    nullptr,
                    true,
                    false
                );
                BSAudio::QInstance().GetSoundHandleByName("UIItemGunsSmallUp ", 289u).Play(false);
                Tile* phighlightedTile = static_cast<Tile*>(lbItemModList.GetHighlightedItem());
                lbItemModList.RemoveItem(phighlightedTile);
                ItemModMenu::InitItemModMenu(ItemModMenu::pBrokenItem, false);
                if (psparePart->GetWorn(false)) {
                    pPlayer->ResetArmorRating();
                    InventoryMenu::UpdateList();
                }
            }
        }
    }
}

void ItemModMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    static int32_t iprevHighlightIndex;
    if (aiID == 15) {
        int32_t inewHighlightIndex;
        ItemChange* pcurrItem = lbItemModList.GetHighlightedData();
        if (pcurrItem) {
            Tile* phighlightedTile = static_cast<Tile*>(lbItemModList.GetHighlightedItem());
            inewHighlightIndex = static_cast<int32_t>(phighlightedTile->GetFloat(4012));
        }
        else {
            inewHighlightIndex = 0;
        }
        if (iprevHighlightIndex != inewHighlightIndex) {
            float fAmount = iprevHighlightIndex >= inewHighlightIndex ? -fScrollKnobIncrement.Float()
                                                                      : fScrollKnobIncrement.Float();
            float fRate = fScrollKnobRate.Float();
            FOPipboyManager* ppipboy = InterfaceManager::Instance()->GetPipboy();
            ppipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, fAmount, fRate, true);
            iprevHighlightIndex = inewHighlightIndex;
        }
        pTiles[13]->Set(4036, "", true);
        if (pcurrItem && ItemModMenu::pBrokenItem) {
            TESObjectIMOD* pImod = static_cast<TESObjectIMOD*>(pcurrItem->pContainerObj);
            const char* pcDescription = pImod->GetDescription(nullptr, DESC_ID);
            pTiles[13]->Set(4036, pcDescription, true);
            const char* pcIconOne = pcurrItem->GetIconOne(nullptr);
            ItemModMenu::pMe->pTiles[3]->Set(4044, pcIconOne, true);
            int32_t iLineAlphaTrait = Tile::TextToTrait("_line_alpha");
            pTiles[11]->Set(iLineAlphaTrait, 255.0f, true);
        }
    }
}

void ItemModMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    if (aiID == 15) {
        pTiles[9]->Set(4003, 0);
        if (lbItemModList.NumUnfiltered() > 1) {
            pTiles[8]->Set(4003, 0);
        }
    }
}

int32_t ItemModMenu::GetClass() {
    return 1061;
}

bool ItemModMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (aiButton == 6 || aiButton == 10) {
        Interface::PlayMenuSound(3);
        DoClick(12, nullptr);
        return true;
    }
    return false;
}

bool ItemModMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

char ItemModMenu::ItemModItem(ItemChange* apSparePart) {
    TESBoundObject* pbrokenObj = ItemModMenu::pBrokenItem->pContainerObj;
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
    ItemChange* pitemGroup = pinventory->GetObjectInList(pbrokenObj, true, 0);
    ExtraDataList* pbrokenExtra = ItemModMenu::pBrokenItem->pExtraObjectList
        ? ItemModMenu::pBrokenItem->pExtraObjectList->m_item
        : nullptr;
    ExtraDataList* pmoddedExtra = new ExtraDataList;
    bool bfoundMatchingExtra = false;
    bool bReturn = false;
    TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj);
    TESObjectIMOD* pimod = static_cast<TESObjectIMOD*>(apSparePart->pContainerObj);
    int32_t iorigStackCount = pbrokenExtra->GetCount();
    pmoddedExtra->DuplicateExtraListForContainer(pbrokenExtra);
    pmoddedExtra->SetCount(1);
    ItemModMenu::pBrokenItem->pExtraObjectList->RemoveHead();
    if (iorigStackCount > 1) {
        pbrokenExtra->SetCount(iorigStackCount - 1);
    }
    uint8_t mSlot = 0;
    if ((pmoddedExtra->GetWeaponModFlags() & 1) != 0 || pweap->GetModItem(1u) != pimod) {
        if ((pmoddedExtra->GetWeaponModFlags() & 2) != 0 || pweap->GetModItem(2u) != pimod) {
            if ((pmoddedExtra->GetWeaponModFlags() & 4) == 0 && pweap->GetModItem(4u) == pimod) {
                pmoddedExtra->SetWeaponModSlot(4);
                mSlot = 4;
                bReturn = true;
            }
        }
        else {
            pmoddedExtra->SetWeaponModSlot(2);
            mSlot = 2;
            bReturn = true;
        }
    }
    else {
        pmoddedExtra->SetWeaponModSlot(1);
        mSlot = 1;
        bReturn = true;
    }
    for (BSSimpleList<ExtraDataList*>* pcurrExtraObj = pitemGroup->pExtraObjectList;
         pcurrExtraObj && pcurrExtraObj->m_item;
         pcurrExtraObj = pcurrExtraObj->m_pkNext) {
        if (!pcurrExtraObj->m_item->CompareListForContainer(pmoddedExtra, true, false) &&
            !pmoddedExtra->CompareListForContainer(pcurrExtraObj->m_item, true, false)) {
            int16_t icount = pcurrExtraObj->m_item->GetCount();
            pcurrExtraObj->m_item->SetCount(icount + 1);
            ItemModMenu::pBrokenItem->pExtraObjectList->AddHead(pcurrExtraObj->m_item);
            bfoundMatchingExtra = true;
        }
    }
    ExtraDataList* pmodExtra = nullptr;
    if (bfoundMatchingExtra) {
        if (iorigStackCount == 1) {
            pitemGroup->pExtraObjectList->Remove(pbrokenExtra);
        }
    }
    else if (iorigStackCount <= 1) {
        pbrokenExtra->SetWeaponModSlot(mSlot);
        ItemModMenu::pBrokenItem->pExtraObjectList->AddHead(pbrokenExtra);
        pmodExtra = pbrokenExtra;
    }
    else {
        pitemGroup->pExtraObjectList->AddHead(pmoddedExtra);
        ItemModMenu::pBrokenItem->pExtraObjectList->AddHead(pmoddedExtra);
        pmodExtra = pmoddedExtra;
    }
    if (bfoundMatchingExtra || iorigStackCount == 1) {
        pmoddedExtra->SetScriptLocals(nullptr);
        if (pmoddedExtra) {
            delete pmoddedExtra;
        }
        if (pmodExtra == pmoddedExtra) {
            pmodExtra = nullptr;
        }
    }
    if (pweap->GetModEffect(mSlot) == WEAPON_MOD_MAX_HEALTH && pmodExtra) {
        double dhealth = pmodExtra->GetHealth();
        float fhealth = pweap->GetModEffectValue(WEAPON_MOD_MAX_HEALTH, 0) + dhealth;
        pmodExtra->SetHealth(fhealth);
    }

    if (bReturn) {
        MiscStatManager::Increment(MiscStatManager::MS_WEAPONS_MODDED);
    }
    return bReturn;
}

Tile* ItemModMenu::Create() {
    InterfaceManager* pinterfaceManager = InterfaceManager::Instance();
    int32_t iclassID = ItemModMenu::GetClassStatic();
    Tile* poldMenu = Menu::GetMenuByClass(iclassID);
    if (poldMenu) {
        delete poldMenu;
    }
    TileMenu* ptileMenu = static_cast<TileMenu*>(
        pinterfaceManager->pMenusRoot->ReadFile("Data\\Menus\\item_mod_menu.xml")
    );
    ItemModMenu::pMe = static_cast<ItemModMenu*>(ptileMenu->GetMenu());
    if (ItemModMenu::pMe) {
        int32_t iclass = ItemModMenu::pMe->GetClass();
        if (iclass == ItemModMenu::GetClassStatic()) {
            ItemModMenu::pMe->SetMenuTile(ptileMenu, false);
            ItemModMenu::pMe->SetRenderedMenu();
            if (ItemModMenu::pMe->IsWellFormedMenu()) {
                if (ptileMenu->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
                    ptileMenu->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
                    float fmaxDepth = Tile::GetMaximumDepth();
                    ptileMenu->Set(Tile::kTileValue_Depth, fmaxDepth, true);
                }
                ItemModMenu::pMe->lbItemModList.Initialize(ItemModMenu::pMe->pTiles[1], "IMM_ItemModListTemplate");
                ItemModMenu::pMe->lbItemModList.SetAutoUpdate(false);
                ItemModMenu::pMe->pTiles[5]->Poke(g_ItemModMenuTrait_Value, 1.0f);
                ItemModMenu::pMe->pTiles[10]->Poke(g_ItemModMenuTrait_Value, 1.0f);
                ItemModMenu::pMe->PrepForVisibility(true);
                return ptileMenu;
            }

            Warning("MENUS: ItemMod Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }
    }

    if (ItemModMenu::pMe && ItemModMenu::pMe->pMenu && ItemModMenu::pMe) {
        delete ItemModMenu::pMe;
    }

    return nullptr;
}

int32_t ItemModMenu::GetClassStatic() {
    return 1061;
}

void ItemModMenu::InitItemModMenu(ItemChange* apBrokenItem, bool bCheckEquip) {
    if (bCheckEquip) {
        ItemModMenu::bIsEquipped = false;
    }
    if (ItemModMenu::pMe) {
        if (apBrokenItem) {
            ItemModMenu::pBrokenItem = apBrokenItem;
        }
        if (ItemModMenu::pBrokenItem && bCheckEquip && ItemModMenu::pBrokenItem->GetWorn(false)) {
            ItemModMenu::pBrokenItem->SetIsModding();
            ExtraDataList* pextra = ItemModMenu::pBrokenItem->pExtraObjectList->m_item;
            uint32_t uiCount = ItemModMenu::pBrokenItem->iNumber;
            TESBoundObject* pobject = ItemModMenu::pBrokenItem->pContainerObj;
            pPlayer->QueueUnEquipObject(pobject, uiCount, pextra, false, false, true);
            ItemModMenu::bIsEquipped = true;
        }

        float fcondition = ItemModMenu::pBrokenItem->GetItemHealth(true) / 100.0;
        ItemModMenu::pMe->pTiles[4]->Set(Tile::kTileValue_User0, fcondition, true);
        ItemModMenu::pMe->pTiles[4]->Set(4003, 1);
        if (ItemModMenu::pBrokenItem->pContainerObj->GetFormType() == WEAP_ID) {
            const char* pcDamage = sInventoryDamage.String();
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Title, pcDamage, true);
            float fdamage = CombatFormulas::CalcWeaponDamageForDisplay(
                static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj),
                fcondition,
                apBrokenItem,
                nullptr
            );
            double droundedDamage = BSRoundToNearest(fdamage, 1.0f);
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Value, static_cast<int32_t>(droundedDamage));
            const char* pcFullName = static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj)
                                         ->TESFullName::GetFullName();
            ItemModMenu::pMe->pTiles[0]->Set(4036, pcFullName, true);
        }
        else {
            const char* pcDR = sInventoryDamageResistance.String();
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Title, pcDR, true);
            double dbaseRating = static_cast<TESObjectARMO*>(ItemModMenu::pBrokenItem->pContainerObj)
                                     ->TESObjectARMO::GetArmorRating();
            float farmorRating = CombatFormulas::CalcArmorRating(static_cast<int32_t>(dbaseRating), fcondition);
            double dceilRating = FloatCeil(farmorRating);
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Value, static_cast<int32_t>(dceilRating));
        }
        if (apBrokenItem) {
            // RemoveAll, Sort, GetTileAt and GetHighlightedItem below are called through the list's vtable, so each
            // goes through a pointer, not the member.
            ListBox<ItemChange*>* pList = &ItemModMenu::pMe->lbItemModList;
            pList->RemoveAll();
            const char* pcIconOne = ItemModMenu::pBrokenItem->GetIconOne(nullptr);
            ItemModMenu::pMe->pTiles[3]->Set(4044, pcIconOne, true);
            ItemModMenu::Update();
        }
        ItemModMenu::pMe->lbItemModList.Traverse(&ItemModMenu::SetTileAlpha, 0, std::numeric_limits<int32_t>::max());
        ListBox<ItemChange*>* pSortedList = &ItemModMenu::pMe->lbItemModList;
        pSortedList->Sort(&ItemModMenu::CompareForSorting);
        if (apBrokenItem && InterfaceManager::Has360Controller()) {
            Tile* pfirstTile = static_cast<ListBoxBase*>(&ItemModMenu::pMe->lbItemModList)->GetTileAt(1u, false);
            InterfaceManager::Instance()->SetCurrentFocusTarget(pfirstTile, 4035, true);
        }

        ListBoxBase* pHighlightList = &ItemModMenu::pMe->lbItemModList;
        Tile* phighlightedTile = static_cast<Tile*>(pHighlightList->GetHighlightedItem());
        ItemModMenu::pMe->DoEnter(15, phighlightedTile);
        if (apBrokenItem) {
            Interface::SetItemModMenuVisible(true, false, false);
            int32_t iclassID = ItemModMenu::GetClassStatic();
            InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_REPAIR, iclassID, 500u);
        }
    }
}

void ItemModMenu::ResetMenu() {
    // RemoveAll is called through a pointer so that it goes through the list's vtable; a call on the member itself
    // would be devirtualised.
    ListBox<ItemChange*>* pList = &ItemModMenu::pMe->lbItemModList;
    pList->RemoveAll();
    ItemModMenu::pBrokenItem = nullptr;
    ItemModMenu::pMe->DoLeave(15, nullptr);
    ItemModMenu::pMe->pTiles[8]->RemoveFadeControl(4009);
    ItemModMenu::pMe->pTiles[8]->Set(4003, 0);
}

void ItemModMenu::Update() {
    if (ItemModMenu::pBrokenItem) {
        TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj);
        TESObjectIMOD* pmodItem1 = nullptr;
        TESBoundObject* pmodItem2 = nullptr;
        TESBoundObject* pmodItem4 = nullptr;
        uint8_t cmodSlots = ItemModMenu::pBrokenItem->GetModSlots();
        if (cmodSlots) {
            if ((cmodSlots & 1) != 0) {
                pmodItem1 = pweap->GetModItem(1u);
                ItemChange* pmodChange = new ItemChange(pmodItem1, 1);
                const char* pcFullName = pmodChange->GetFullName();
                Tile* pnewTile = ItemModMenu::pMe->lbItemModList.AddItem(pmodChange, pcFullName, nullptr, nullptr);
                pnewTile->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                pnewTile->xChildren.Get(pnewTile->xChildren.m_pkHead)->Set(4003, 1);
            }
            if ((cmodSlots & 2) != 0) {
                pmodItem2 = pweap->GetModItem(2u);
                ItemChange* pmodChange = new ItemChange(pmodItem2, 1);
                const char* pcFullName = pmodChange->GetFullName();
                Tile* pnewTile = ItemModMenu::pMe->lbItemModList.AddItem(pmodChange, pcFullName, nullptr, nullptr);
                pnewTile->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                pnewTile->xChildren.Get(pnewTile->xChildren.m_pkHead)->Set(4003, 1);
            }
            if ((cmodSlots & 4) != 0) {
                pmodItem4 = pweap->GetModItem(4u);
                ItemChange* pmodChange = new ItemChange(pmodItem4, 1);
                const char* pcFullName = pmodChange->GetFullName();
                Tile* pnewTile = ItemModMenu::pMe->lbItemModList.AddItem(pmodChange, pcFullName, nullptr, nullptr);
                pnewTile->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                pnewTile->xChildren.Get(pnewTile->xChildren.m_pkHead)->Set(4003, 1);
            }
        }
        BSSimpleList<ItemChange*> kitemModGroup;
        InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
        pinventory->GetItemModGroup(ItemModMenu::pBrokenItem->pContainerObj, &kitemModGroup);
        for (ItemChange*& rpItem : kitemModGroup.ItemsUntilNull()) {
            for (int32_t i = 0; i < rpItem->iNumber; ++i) {
                ItemChange* pitem = rpItem;
                if (i) {
                    if (pitem->pContainerObj != pmodItem1) {
                        if (pitem->pContainerObj != pmodItem2) {
                            if (pitem->pContainerObj != pmodItem4) {
                                ItemChange* pnewItem = new ItemChange(rpItem->pContainerObj, 1);
                                for (BSSimpleList<ExtraDataList*>* j = rpItem->pExtraObjectList; j && j->m_item;
                                     j = j->GetNext()) {
                                    pnewItem->pExtraObjectList->AddHead(j->m_item);
                                }
                                const char* pcFullName = pnewItem->GetFullName();
                                Tile* pnewTile = ItemModMenu::pMe->lbItemModList
                                                     .AddItem(pnewItem, pcFullName, nullptr, nullptr);
                                pnewTile->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                            }
                        }
                    }
                }
                else if (pitem->pContainerObj != pmodItem1) {
                    if (pitem->pContainerObj != pmodItem2) {
                        if (pitem->pContainerObj != pmodItem4) {
                            const char* pcFullName = pitem->GetFullName();
                            Tile* pnewTile = ItemModMenu::pMe->lbItemModList
                                                 .AddItem(pitem, pcFullName, nullptr, nullptr);
                            pnewTile->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                        }
                    }
                }
            }
        }
        const char* pcLanguage = sLanguage.String();
        if (BSstrcmp(pcLanguage, "ENGLISH")) {
            const char* pcItems = sInventoryItems.String();
            Tile* pheadline = ItemModMenu::pMe->pMenu->GetChildByName("IMM_Headline_Title");
            pheadline->Set(4036, pcItems, true);
            const char* pcCondition = sInventoryCondition.String();
            ItemModMenu::pMe->pTiles[4]->Set(g_ItemModMenuTrait_Title, pcCondition, true);
            pcCondition = sInventoryCondition.String();
            ItemModMenu::pMe->pTiles[9]->Set(g_ItemModMenuTrait_Title, pcCondition, true);
            const char* pcRepairItem = sRepairItem.String();
            ItemModMenu::pMe->pTiles[11]->Set(4036, pcRepairItem, true);
            const char* pcCancel = sCancel.String();
            ItemModMenu::pMe->pTiles[12]->Set(4036, pcCancel, true);
        }
    }
}

void ItemModMenu::SetTileAlpha(Tile* apTile, ItemChange* apItem) {
    if (apTile && apItem) {
        apItem->GetItemHealth(true);
        apTile->Set(4010, 15);
        int32_t ivisible = apTile->GetFloat(4012) >= 0.0;
        apTile->Set(4003, ivisible);
        apTile->Set(4003, 1.0f, true);
        TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj);
        if (ItemModMenu::pBrokenItem->GetModSlots()) {
            TESObjectIMOD* pimod = static_cast<TESObjectIMOD*>(apItem->pContainerObj);
            if (((ItemModMenu::pBrokenItem->GetModSlots() & 1) != 0 && pimod == pweap->GetModItem(1u)) ||
                ((ItemModMenu::pBrokenItem->GetModSlots() & 2) != 0 && pimod == pweap->GetModItem(2u)) ||
                ((ItemModMenu::pBrokenItem->GetModSlots() & 4) != 0 && pimod == pweap->GetModItem(4u))) {
                apTile->xChildren.GetTail()->Set(4009, 127.5f, true);
            }
        }
    }
}

int32_t ItemModMenu::CompareForSorting(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2) {
    bool bvisible1 = apItem1->pTile->xChildren.Get(apItem1->pTile->xChildren.m_pkHead)->IsVisible();
    bool bvisible2 = apItem2->pTile->xChildren.Get(apItem2->pTile->xChildren.m_pkHead)->IsVisible();
    if (bvisible1 && !bvisible2) {
        return -1;
    }
    if (!bvisible1 && bvisible2) {
        return 1;
    }
    return 0;
}
