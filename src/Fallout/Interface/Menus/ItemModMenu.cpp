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

void ItemModMenu::DoClick(int32_t a1, Tile* a2) {
    char v16 = 0;
    if (!a2 && a1 >= 0 && a1 < 14) {
        a2 = pTiles[a1];
    }
    if (a1 == 12) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
        if (ItemModMenu::bIsEquipped) {
            ExtraDataList* m_item = ItemModMenu::pBrokenItem->pExtraObjectList->m_item;
            uint32_t FormHealth_0 = ItemModMenu::pBrokenItem->iNumber;
            TESBoundObject* FormFlags = ItemModMenu::pBrokenItem->pContainerObj;
            pPlayer->QueueEquipObject(FormFlags, FormHealth_0, m_item, true, false, true);
            ItemModMenu::pBrokenItem->RemoveIsModding();
            ItemModMenu::bIsEquipped = false;
        }
        Interface::SetInventoryMenuVisible(true, false, false);
    }
    else {
        if (a1 == 15 && !(a2->xChildren.GetTail()->GetFloat(4009) < 255.0)) {
            ItemChange* HighlightedData = lbItemModList.GetHighlightedData();
            if (HighlightedData != ItemModMenu::pBrokenItem) {
                ExtraDataList* v11;
                ItemChange* psparePart = lbItemModList.GetHighlightedData();
                if (psparePart->pExtraObjectList) {
                    v11 = psparePart->pExtraObjectList->m_item;
                }
                else {
                    v11 = nullptr;
                }
                if (ItemModItem(psparePart)) {
                    TESBoundObject* v6 = psparePart->pContainerObj;
                    pPlayer->RemoveItem(v6, v11, 1, false, false, nullptr, nullptr, nullptr, true, false);
                    BSAudio* v4 = &BSAudio::QInstance();
                    v4->GetSoundHandleByName("UIItemGunsSmallUp ", 289u).Play(false);
                    Tile* v5 = static_cast<Tile*>(lbItemModList.GetHighlightedItem());
                    lbItemModList.RemoveItem(v5);
                    ItemModMenu::InitItemModMenu(ItemModMenu::pBrokenItem, false);
                    if (psparePart->GetWorn(false)) {
                        pPlayer->ResetArmorRating();
                        InventoryMenu::UpdateList();
                    }
                }
            }
        }
    }
}

void ItemModMenu::DoEnter(int32_t a1, Tile* a2) {
    static int32_t iprevHighlightIndex;
    if (a1 == 15) {
        int32_t inewHighlightIndex;
        ItemChange* pcurrItem = lbItemModList.GetHighlightedData();
        if (pcurrItem) {
            Tile* v3 = static_cast<Tile*>(lbItemModList.GetHighlightedItem());
            inewHighlightIndex = static_cast<int32_t>(v3->GetFloat(4012));
        }
        else {
            inewHighlightIndex = 0;
        }
        if (iprevHighlightIndex != inewHighlightIndex) {
            float afAmount;
            if (iprevHighlightIndex >= inewHighlightIndex) {
                afAmount = -fScrollKnobIncrement.Float();
            }
            else {
                afAmount = fScrollKnobIncrement.Float();
            }
            float afRate = fScrollKnobRate.Float();
            InterfaceManager* v4 = InterfaceManager::Instance();
            FOPipboyManager* Pipboy = v4->GetPipboy();
            Pipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, afAmount, afRate, true);
            iprevHighlightIndex = inewHighlightIndex;
        }
        pTiles[13]->Set(4036, "", true);
        if ((pcurrItem) && (ItemModMenu::pBrokenItem)) {
            TESObjectIMOD* pImod = static_cast<TESObjectIMOD*>(pcurrItem->pContainerObj);
            const char* v6 = pImod->GetDescription(nullptr, DESC_ID);
            pTiles[13]->Set(4036, v6, true);
            const char* IconOne = pcurrItem->GetIconOne(nullptr);
            ItemModMenu::pMe->pTiles[3]->Set(4044, IconOne, true);
            int32_t v8 = Tile::TextToTrait("_line_alpha");
            pTiles[11]->Set(v8, 255.0f, true);
        }
    }
}

void ItemModMenu::DoLeave(int32_t a2, Tile* a3) {
    if (a2 == 15) {
        pTiles[9]->Set(4003, 0);
        if (lbItemModList.NumUnfiltered() > 1) {
            pTiles[8]->Set(4003, 0);
        }
    }
}

int32_t ItemModMenu::GetClass() {
    return 1061;
}

bool ItemModMenu::DoGamepadUpEvent(int32_t a2, float a3) {
    int8_t v5 = 0;
    if ((a2 == 6) || (a2 == 10)) {
        Interface::PlayMenuSound(3);
        DoClick(12, nullptr);
        return true;
    }
    return (bool)v5;
}

bool ItemModMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

char ItemModMenu::ItemModItem(ItemChange* a1) {
    ExtraDataList* pmoddedExtra;
    ExtraDataList* pbrokenExtra;
    TESBoundObject* FormFlags = ItemModMenu::pBrokenItem->pContainerObj;
    InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
    ItemChange* pitemGroup = InventoryChanges_->GetObjectInList(FormFlags, true, 0);
    if (ItemModMenu::pBrokenItem->pExtraObjectList) {
        pbrokenExtra = ItemModMenu::pBrokenItem->pExtraObjectList->m_item;
    }
    else {
        pbrokenExtra = nullptr;
    }
    ExtraDataList* pbrokenExtra_2 = pbrokenExtra;
    ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
    if (apMemory) {
        pmoddedExtra = ::new (apMemory) ExtraDataList;
    }
    else {
        pmoddedExtra = nullptr;
    }
    ExtraDataList* pmoddedExtra_2 = pmoddedExtra;
    bool bfoundMatchingExtra = false;
    bool bReturn = false;
    TESObjectWEAP* pweap = static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj);
    uint8_t cSlot = 1;
    TESObjectIMOD* imod = static_cast<TESObjectIMOD*>(a1->pContainerObj);
    ItemChange* pInv = InventoryMenu::pCurrItem;
    int32_t iorigStackCount = pbrokenExtra_2->GetCount();
    pmoddedExtra->DuplicateExtraListForContainer(pbrokenExtra_2);
    pmoddedExtra->SetCount(1);
    ItemModMenu::pBrokenItem->pExtraObjectList->RemoveHead();
    if (iorigStackCount > 1) {
        pbrokenExtra_2->SetCount(iorigStackCount - 1);
    }
    uint8_t mSlot = 0;
    if ((pmoddedExtra_2->GetWeaponModFlags() & 1) != 0 || pweap->GetModItem(1u) != imod) {
        if ((pmoddedExtra_2->GetWeaponModFlags() & 2) != 0 || pweap->GetModItem(2u) != imod) {
            if ((pmoddedExtra_2->GetWeaponModFlags() & 4) == 0 && pweap->GetModItem(4u) == imod) {
                pmoddedExtra_2->SetWeaponModSlot(4);
                mSlot = 4;
                bReturn = true;
            }
        }
        else {
            pmoddedExtra_2->SetWeaponModSlot(2);
            mSlot = 2;
            bReturn = true;
        }
    }
    else {
        pmoddedExtra_2->SetWeaponModSlot(1);
        mSlot = 1;
        bReturn = true;
    }
    for (BSSimpleList<ExtraDataList*>* pcurrExtraObj = pitemGroup->pExtraObjectList;
         pcurrExtraObj && pcurrExtraObj->m_item; pcurrExtraObj = pcurrExtraObj->m_pkNext) {
        if (!pcurrExtraObj->m_item->CompareListForContainer(pmoddedExtra_2, true, false) &&
            !pmoddedExtra_2->CompareListForContainer(pcurrExtraObj->m_item, true, false)) {
            int16_t Count = pcurrExtraObj->m_item->GetCount();
            pcurrExtraObj->m_item->SetCount(Count + 1);
            BSSimpleList<ExtraDataList*>* afHealth = pcurrExtraObj;
            ItemModMenu::pBrokenItem->pExtraObjectList->AddHead(afHealth->m_item);
            bfoundMatchingExtra = true;
        }
    }
    ExtraDataList* pbrokenExtra_3 = nullptr;
    if (bfoundMatchingExtra) {
        if (iorigStackCount == 1) {
            pitemGroup->pExtraObjectList->Remove(pbrokenExtra_2);
        }
    }
    else {
        if (iorigStackCount <= 1) {
            pbrokenExtra_2->SetWeaponModSlot(mSlot);
            ItemModMenu::pBrokenItem->pExtraObjectList->AddHead(pbrokenExtra_2);
            pbrokenExtra_3 = pbrokenExtra_2;
        }
        else {
            pitemGroup->pExtraObjectList->AddHead(pmoddedExtra_2);
            ItemModMenu::pBrokenItem->pExtraObjectList->AddHead(pmoddedExtra_2);
            pbrokenExtra_3 = pmoddedExtra_2;
        }
    }
    if (bfoundMatchingExtra || iorigStackCount == 1) {
        pmoddedExtra_2->SetScriptLocals(nullptr);
        if (pmoddedExtra_2) {
            delete pmoddedExtra_2;
        }
        if (pbrokenExtra_3 == pmoddedExtra_2) {
            pbrokenExtra_3 = nullptr;
        }
    }
    if ((pweap->GetModEffect(mSlot) == WEAPON_MOD_MAX_HEALTH) && (pbrokenExtra_3)) {
        double Health = pbrokenExtra_3->GetHealth();
        float v7 = pweap->GetModEffectValue(WEAPON_MOD_MAX_HEALTH, 0) + Health;
        pbrokenExtra_3->SetHealth(v7);
    }

    if (bReturn) {
        MiscStatManager::Increment(MiscStatManager::MS_WEAPONS_MODDED);
    }
    return bReturn;
}

Tile* ItemModMenu::Create() {
    int32_t v1;
    InterfaceManager* v4 = InterfaceManager::Instance();
    int32_t ClassStatic = ItemModMenu::GetClassStatic();
    Tile* MenuByClass = Menu::GetMenuByClass(ClassStatic);
    if (MenuByClass) {
        delete MenuByClass;
    }
    TileMenu* th_s = static_cast<TileMenu*>(v4->pMenusRoot->ReadFile("Data\\Menus\\item_mod_menu.xml"));
    ItemModMenu::pMe = static_cast<ItemModMenu*>(th_s->GetMenu());
    if (ItemModMenu::pMe && (v1 = ItemModMenu::pMe->GetClass(), v1 == ItemModMenu::GetClassStatic())) {
        ItemModMenu::pMe->SetMenuTile(th_s, false);
        ItemModMenu::pMe->SetRenderedMenu();
        if (ItemModMenu::pMe->IsWellFormedMenu()) {
            if (th_s->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
                th_s->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
                float MaximumDepth = Tile::GetMaximumDepth();
                th_s->Set(Tile::kTileValue_Depth, MaximumDepth, true);
            }
            ItemModMenu::pMe->lbItemModList.Initialize(ItemModMenu::pMe->pTiles[1], "IMM_ItemModListTemplate");
            ItemModMenu::pMe->lbItemModList.SetAutoUpdate(false);
            ItemModMenu::pMe->pTiles[5]->Poke(g_ItemModMenuTrait_Value, 1.0f);
            ItemModMenu::pMe->pTiles[10]->Poke(g_ItemModMenuTrait_Value, 1.0f);
            ItemModMenu::pMe->PrepForVisibility(true);
            return th_s;
        }

        Warning("MENUS: ItemMod Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }

    if ((ItemModMenu::pMe) && (ItemModMenu::pMe->pMenu) && (ItemModMenu::pMe)) {
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
        if ((ItemModMenu::pBrokenItem) && (bCheckEquip && ItemModMenu::pBrokenItem->GetWorn(false))) {
            ItemModMenu::pBrokenItem->SetIsModding();
            ExtraDataList* afCondition = ItemModMenu::pBrokenItem->pExtraObjectList->m_item;
            uint32_t FormHealth_0 = ItemModMenu::pBrokenItem->iNumber;
            TESBoundObject* FormFlags = ItemModMenu::pBrokenItem->pContainerObj;
            pPlayer->QueueUnEquipObject(FormFlags, FormHealth_0, afCondition, false, false, true);
            ItemModMenu::bIsEquipped = true;
        }

        float fcondition = ItemModMenu::pBrokenItem->GetItemHealth(true) / 100.0;
        ItemModMenu::pMe->pTiles[4]->Set(Tile::kTileValue_User0, fcondition, true);
        ItemModMenu::pMe->pTiles[4]->Set(4003, 1);
        if (ItemModMenu::pBrokenItem->pContainerObj->GetFormType() == WEAP_ID) {
            const char* v2 = sInventoryDamage.String();
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Title, v2, true);
            float afVal = CombatFormulas::CalcWeaponDamageForDisplay(
                static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj), fcondition, apBrokenItem, nullptr
            );
            double v3 = BSRoundToNearest(afVal, 1.0f);
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Value, static_cast<int32_t>(v3));
            const char* Model_0 =
                static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj)->TESFullName::GetFullName();
            ItemModMenu::pMe->pTiles[0]->Set(4036, Model_0, true);
        }
        else {
            const char* v5 = sInventoryDamageResistance.String();
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Title, v5, true);
            double ArmorRating =
                static_cast<TESObjectARMO*>(ItemModMenu::pBrokenItem->pContainerObj)->TESObjectARMO::GetArmorRating();
            float a1 = CombatFormulas::CalcArmorRating(static_cast<int32_t>(ArmorRating), fcondition);
            double v7 = FloatCeil(a1);
            ItemModMenu::pMe->pTiles[5]->Set(g_ItemModMenuTrait_Value, static_cast<int32_t>(v7));
        }
        if (apBrokenItem) {
            // RemoveAll, Sort, GetTileAt and GetHighlightedItem below are called through the list's vtable, so each
            // goes through a pointer, not the member.
            ListBox<ItemChange*>* pList = &ItemModMenu::pMe->lbItemModList;
            pList->RemoveAll();
            const char* IconOne = ItemModMenu::pBrokenItem->GetIconOne(nullptr);
            ItemModMenu::pMe->pTiles[3]->Set(4044, IconOne, true);
            ItemModMenu::Update();
        }
        ItemModMenu::pMe->lbItemModList.Traverse(&ItemModMenu::SetTileAlpha, 0, 0x7FFFFFFF);
        ListBox<ItemChange*>* pSortedList = &ItemModMenu::pMe->lbItemModList;
        pSortedList->Sort(&ItemModMenu::CompareForSorting);
        if ((apBrokenItem) && (InterfaceManager::Has360Controller())) {
            Tile* afVala = static_cast<ListBoxBase*>(&ItemModMenu::pMe->lbItemModList)->GetTileAt(1u, false);
            InterfaceManager* v9 = InterfaceManager::Instance();
            v9->SetCurrentFocusTarget(afVala, 4035, true);
        }

        ListBoxBase* pHighlightList = &ItemModMenu::pMe->lbItemModList;
        Tile* v10 = static_cast<Tile*>(pHighlightList->GetHighlightedItem());
        ItemModMenu::pMe->DoEnter(15, v10);
        if (apBrokenItem) {
            Interface::SetItemModMenuVisible(true, false, false);
            int32_t afValb = ItemModMenu::GetClassStatic();
            InterfaceManager* v11 = InterfaceManager::Instance();
            v11->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_REPAIR, afValb, 500u);
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
        TESObjectWEAP* FormFlags = static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj);
        TESObjectIMOD* apObject = nullptr;
        TESBoundObject* ModItem = nullptr;
        TESBoundObject* v40 = nullptr;
        uint8_t ModSlots = ItemModMenu::pBrokenItem->GetModSlots();
        if (ModSlots) {
            if ((ModSlots & 1) != 0) {
                ItemChange* v21;
                apObject = FormFlags->GetModItem(1u);
                ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                if (apMemory) {
                    v21 = ::new (apMemory) ItemChange(apObject, 1);
                }
                else {
                    v21 = nullptr;
                }
                const char* FullName = v21->GetFullName();
                Tile* v32 = ItemModMenu::pMe->lbItemModList.AddItem(v21, FullName, nullptr, nullptr);
                v32->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                v32->xChildren.Get(v32->xChildren.m_pkHead)->Set(4003, 1);
            }
            if ((ModSlots & 2) != 0) {
                ItemChange* v20;
                ModItem = (TESBoundObject*)FormFlags->GetModItem(2u);
                ItemChange* v24 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                if (v24) {
                    v20 = ::new (v24) ItemChange(ModItem, 1);
                }
                else {
                    v20 = nullptr;
                }
                const char* v1 = v20->GetFullName();
                Tile* v33 = ItemModMenu::pMe->lbItemModList.AddItem(v20, v1, nullptr, nullptr);
                v33->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                v33->xChildren.Get(v33->xChildren.m_pkHead)->Set(4003, 1);
            }
            if ((ModSlots & 4) != 0) {
                ItemChange* v19;
                v40 = (TESBoundObject*)FormFlags->GetModItem(4u);
                ItemChange* v23 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                if (v23) {
                    v19 = ::new (v23) ItemChange(v40, 1);
                }
                else {
                    v19 = nullptr;
                }
                const char* v2 = v19->GetFullName();
                Tile* v34 = ItemModMenu::pMe->lbItemModList.AddItem(v19, v2, nullptr, nullptr);
                v34->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                v34->xChildren.Get(v34->xChildren.m_pkHead)->Set(4003, 1);
            }
        }
        BSSimpleList<ItemChange*> apList;
        InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
        InventoryChanges_->GetItemModGroup(ItemModMenu::pBrokenItem->pContainerObj, &apList);
        for (ItemChange*& rpItem : apList.ItemsUntilNull()) {
            for (int32_t i = 0; i < rpItem->iNumber; ++i) {
                ItemChange* m_item = rpItem;
                if (i) {
                    if (m_item->pContainerObj != apObject) {
                        TESBoundObject* v7 = m_item->pContainerObj;
                        if (v7 != ModItem) {
                            TESBoundObject* v8 = m_item->pContainerObj;
                            if (v8 != v40) {
                                ItemChange* v18;
                                ItemChange* v22 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                                if (v22) {
                                    TESBoundObject* v9 = rpItem->pContainerObj;
                                    v18 = ::new (v22) ItemChange(v9, 1);
                                }
                                else {
                                    v18 = nullptr;
                                }
                                for (BSSimpleList<ExtraDataList*>* j = rpItem->pExtraObjectList; j && j->m_item;
                                     j = j->GetNext()) {
                                    v18->pExtraObjectList->AddHead(j->m_item);
                                }
                                const char* v10 = v18->GetFullName();
                                Tile* v30 = ItemModMenu::pMe->lbItemModList.AddItem(v18, v10, nullptr, nullptr);
                                v30->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                            }
                        }
                    }
                }
                else {
                    if (m_item->pContainerObj != apObject) {
                        TESBoundObject* v4 = m_item->pContainerObj;
                        if (v4 != ModItem) {
                            TESBoundObject* v5 = m_item->pContainerObj;
                            if (v5 != v40) {
                                const char* v6 = m_item->GetFullName();
                                Tile* v29 = ItemModMenu::pMe->lbItemModList.AddItem(m_item, v6, nullptr, nullptr);
                                v29->Set(g_ItemModMenuTrait_IsItemModItem, 1);
                            }
                        }
                    }
                }
            }
        }
        const char* v11 = sLanguage.String();
        if (BSstrcmp(v11, "ENGLISH")) {
            const char* v17 = sInventoryItems.String();
            Tile* ChildByName = ItemModMenu::pMe->pMenu->GetChildByName("IMM_Headline_Title");
            ChildByName->Set(4036, v17, true);
            const char* v13 = sInventoryCondition.String();
            ItemModMenu::pMe->pTiles[4]->Set(g_ItemModMenuTrait_Title, v13, true);
            const char* v14 = sInventoryCondition.String();
            ItemModMenu::pMe->pTiles[9]->Set(g_ItemModMenuTrait_Title, v14, true);
            const char* v15 = sRepairItem.String();
            ItemModMenu::pMe->pTiles[11]->Set(4036, v15, true);
            const char* v16 = sCancel.String();
            ItemModMenu::pMe->pTiles[12]->Set(4036, v16, true);
        }
    }
}

void ItemModMenu::SetTileAlpha(Tile* a1, ItemChange* a2) {
    if ((a1) && (a2)) {
        a2->GetItemHealth(true);
        a1->Set(4010, 15);
        int32_t a3 = a1->GetFloat(4012) >= 0.0;
        a1->Set(4003, a3);
        a1->Set(4003, 1.0f, true);
        TESObjectWEAP* FormFlags = static_cast<TESObjectWEAP*>(ItemModMenu::pBrokenItem->pContainerObj);
        if (ItemModMenu::pBrokenItem->GetModSlots()) {
            TESObjectIMOD* v3 = static_cast<TESObjectIMOD*>(a2->pContainerObj);
            if (((ItemModMenu::pBrokenItem->GetModSlots() & 1) != 0 && v3 == FormFlags->GetModItem(1u)) ||
                (((ItemModMenu::pBrokenItem->GetModSlots() & 2) != 0 && v3 == FormFlags->GetModItem(2u)) ||
                 ((ItemModMenu::pBrokenItem->GetModSlots() & 4) != 0 && v3 == FormFlags->GetModItem(4u)))) {
                a1->xChildren.GetTail()->Set(4009, 127.5f, true);
            }
        }
    }
}

int32_t ItemModMenu::CompareForSorting(ListBoxItem<ItemChange*>* a1, ListBoxItem<ItemChange*>* a2) {
    int32_t v4 = 0;
    bool IsVisible = a1->pTile->xChildren.Get(a1->pTile->xChildren.m_pkHead)->IsVisible();
    bool v5 = a2->pTile->xChildren.Get(a2->pTile->xChildren.m_pkHead)->IsVisible();
    if (IsVisible && !v5) {
        return -1;
    }
    if (!IsVisible && v5) {
        return 1;
    }
    return v4;
}
