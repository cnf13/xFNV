#include "Fallout/Interface/Menus/BarterMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSMain/BSMultiBoundNode.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActionPoints.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSLoadedReferenceCollection.h"
#include "Fallout Shared/BGSNote.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ContainerItemExtra.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/FOCollisionListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSettingCollection.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageEventAction.h"
#include "Fallout Shared/Pathfinding/PathBuilder.h"
#include "Fallout Shared/Pathfinding/PathManager.h"
#include "Fallout Shared/Pathfinding/PathingRequest.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/ScriptCompileData.h"
#include "Fallout Shared/Sky/Sky.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESAnimGroup.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESGlobal.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESIdleManager.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESRaceForm.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESTexture.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESValueForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatCoverMap.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatTargetMap.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/HitData.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/FleePackage.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutRadio.h"
#include "Fallout/Audio/ImpactMixer.h"
#include "Fallout/Debug/DebugTextManager.h"
#include "Fallout/Dialogue/DialogueResponse.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/CharGenMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/InventoryMenu.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Interface/Menus/QuantityMenu.h"
#include "Fallout/Interface/Menus/VATSMenu.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/tchar.h"

#include <cmath>
#include <cstdint>

int32_t BSstrupr(char* apString1, int32_t aiSize) {
    return _strupr_s(apString1, aiSize);
}

BarterMenu::BarterMenu()
  : Menu(), iPlayerCaps(0), iVendorCaps(0), fBuyMult(1.0f), fSellMult(1.0f), ePlayerFilterMode(FM_ALL),
    eVendorFilterMode(FM_ALL), eFilterSound(MENU_SOUND_NEXT_PREVIOUS), plbCurrListbox(nullptr), bItemsTraded(false) {
    g_BarterMenu_Trait_Title = Tile::TextToTrait("_Title");
    g_BarterMenu_Trait_Value = Tile::TextToTrait("_Value");
    BarterMenu::iUT_IsBarterSelected = Tile::TextToTrait("_IsBarterSelected");
    g_BarterMenu_Trait_IsContainerListSelected = Tile::TextToTrait("_IsContainerListSelected");
    g_BarterMenu_Trait_ArrowDirection = Tile::TextToTrait("_ArrowDirection");
    BarterMenu::pCurrItem = nullptr;
}

BarterMenu::~BarterMenu() {
    if (BarterMenu::pMe) {
        for (ItemChange* pItem : BarterMenu::pMe->ItemsToBuy.ItemsUntilNull()) {
            if (pItem) {
                delete pItem;
            }
        }
        for (ItemChange* pItem : BarterMenu::pMe->ItemsToSell.ItemsUntilNull()) {
            if (pItem) {
                delete pItem;
            }
        }
        BarterMenu::pMe->ItemsToBuy.RemoveAll();
        BarterMenu::pMe->ItemsToSell.RemoveAll();
    }
    pDataHandler->ClearBarterContainer();
    BarterMenu::pMe = nullptr;
}

void BarterMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 22) {
        pTiles[aiID] = apTile;
    }
}

void BarterMenu::DoClick(int32_t a1, Tile* a2) {
    if (!a2 && a1 >= 0 && a1 < 22) {
        a2 = pTiles[a1];
    }
    switch (a1) {
        case 2:
        case 3:
        case 4: {
            Interface::PlayMenuSound(static_cast<int32_t>(eFilterSound));
            do {
                ePlayerFilterMode = static_cast<BarterMenu::FILTER_MODE>(ePlayerFilterMode + 2 * (a1 != 2) - 1);
                if (ePlayerFilterMode >= BarterMenu::FM_ALL) {
                    if (ePlayerFilterMode > BarterMenu::FM_AMMO) {
                        ePlayerFilterMode = BarterMenu::FM_ALL;
                    }
                }
                else {
                    ePlayerFilterMode = BarterMenu::FM_AMMO;
                }
                SetFilterModeTitle(pTiles[3]);
                lbPlayerItems.Filter(&BarterMenu::FilterModeFunc);
            }
            while (!lbPlayerItems.NumUnfiltered() && ePlayerFilterMode);
            break;
        }
        case 7:
        case 8:
        case 9: {
            Interface::PlayMenuSound(static_cast<int32_t>(eFilterSound));
            do {
                eVendorFilterMode = static_cast<BarterMenu::FILTER_MODE>(eVendorFilterMode + 2 * (a1 != 7) - 1);
                if (eVendorFilterMode >= BarterMenu::FM_ALL) {
                    if (eVendorFilterMode > BarterMenu::FM_AMMO) {
                        eVendorFilterMode = BarterMenu::FM_ALL;
                    }
                }
                else {
                    eVendorFilterMode = BarterMenu::FM_AMMO;
                }
                SetFilterModeTitle(pTiles[8]);
                lbVendorItems.Filter(&BarterMenu::FilterModeFunc);
            }
            while (!lbVendorItems.NumUnfiltered() && eVendorFilterMode);
            break;
        }
        case 13: {
            BarterMenu::bCrossFade = false;
            CompleteTransaction();
            UpdateLists(nullptr);
            break;
        }
        case 19: {
            if (ItemsToBuy.ItemsInList() || ItemsToSell.ItemsInList()) {
                const char* v6 = sNo.String();
                const char* v5 = sYes.String();
                Interface::CreateMessageMenu(
                    static_cast<const char*>(nullptr), static_cast<const char*>(nullptr), sCancelBarter.String(),
                    &BarterMenu::CancelConfirmFunc, 0, 23, 0.0f, 0.0f, v5, v6, nullptr
                );
            }
            else {
                BarterMenu::Close();
            }
            break;
        }
        case 23: {
            if ((BarterMenu::pCurrItem) && (a2)) {
                BarterMenu::bCrossFade = false;
                uint32_t FormHealth_0 = BarterMenu::pCurrItem->iNumber;
                int32_t v4 = Tile::TextToTrait("_NumBartered");
                float someFloat = a2->GetFloat(v4);
                int32_t iitemCount = FormHealth_0 - FloatToInt(someFloat);
                if (Interface::IsConsoleVisible()) {
                    TESObjectREFR* poriginalRef;
                    if (BarterMenu::pCurrItem->pExtraObjectList && BarterMenu::pCurrItem->pExtraObjectList->m_item) {
                        poriginalRef = BarterMenu::pCurrItem->pExtraObjectList->m_item->GetOriginalReference();
                    }
                    else {
                        poriginalRef = nullptr;
                    }
                    if (poriginalRef) {
                        int32_t someFloata = poriginalRef->iFormID;
                        Interface::PrintLine(
                            "Original Container Ref: '%s' (%08x)", poriginalRef->GetFormEditorID(), someFloata
                        );
                    }
                    else {
                        Interface::PrintLine("Original Container Ref: NONE");
                    }
                }
                else {
                    if (iitemCount <= iInventoryAskQuantityAt.Int()) {
                        BarterMenu::ItemSelectCallback(1);
                    }
                    else {
                        plbCurrListbox->SaveListState();
                        plbCurrListbox->Highlight(nullptr);
                        QuantityMenu::Create(iitemCount, &BarterMenu::ItemSelectCallback, 0x7FFFFFFF);
                    }
                }
            }

            break;
        }
        default: {
            return;
        }
    }
}

void BarterMenu::DoEnter(int32_t a1, Tile* a2) {
    if (!a2 && a1 >= 0 && a1 < 22) {
        a2 = pTiles[a1];
    }
    if (a1) {
        if (a1 == 1) {
            DoLeave(23, nullptr);
            static_cast<ListBoxBase*>(&lbPlayerItems)->Highlight(nullptr);
            pMenu->Set(g_BarterMenu_Trait_IsContainerListSelected, 1);
            plbCurrListbox = &lbVendorItems;
        }
        else {
            if (a1 == 23) {
                BarterMenu::bCrossFade = true;
                if (!plbCurrListbox) {
                    DoEnter(lbVendorItems.IsListItem(a2), nullptr);
                }
                BarterMenu::pCurrItem = plbCurrListbox->GetHighlightedData();
                if (BarterMenu::pCurrItem) {
                    InventoryMenu::SetupItemDataTile(BarterMenu::pCurrItem, pTiles[21]);
                    pTiles[21]->Set(4003, 1);
                    const char* IconOne = BarterMenu::pCurrItem->GetIconOne(nullptr);
                    pTiles[20]->Set(4044, IconOne, true);
                    pTiles[20]->Set(4003, 1);
                }
            }
        }
    }
    else {
        DoLeave(23, nullptr);
        static_cast<ListBoxBase*>(&lbVendorItems)->Highlight(nullptr);
        pMenu->Set(g_BarterMenu_Trait_IsContainerListSelected, 0);
        plbCurrListbox = &lbPlayerItems;
    }
}

void BarterMenu::DoLeave(int32_t a2, Tile* a3) {
    pTiles[21]->Set(4003, 0);
    pTiles[20]->Set(4003, 0);
}

void BarterMenu::DoWheelMove(int32_t a2, Tile* a3) {
    InterfaceManager* singleton;
    if ((a2 == 3) || (a2 == 8)) {
        singleton = InterfaceManager::Instance();
        if (singleton->GetMouseWheel() >= 0) {
            DoClick((a2 != 3) ? 9 : 4, nullptr);
        }
        else {
            DoClick((a2 != 3) ? 7 : 2, nullptr);
        }
    }
}

void BarterMenu::DoIdle() {
    if (BarterMenu::bForceUpdate) {
        BarterMenu::ForceBarterUpdate(true);
    }
    if (BarterMenu::bCrossFade) {
        if (BarterMenu::pCurrItem && BarterMenu::pCurrItem->pContainerObj->GetFormType() == WEAP_ID) {
            InterfaceManager::UpdatePipBoyInventoryEffects(
                pTiles[14], pTiles[15], pTiles[16], BarterMenu::pCurrItem, false
            );
        }
        else {
            if ((BarterMenu::pCurrItem) && (BarterMenu::pCurrItem->pContainerObj->GetFormType() == ARMO_ID)) {
                Tile* pTileFade2;
                if (!BarterMenu::pCurrItem ||
                    static_cast<TESObjectARMO*>(BarterMenu::pCurrItem->pContainerObj)
                            ->TESObjectARMO::GetDamageThreshold() == 0.0) {
                    pTileFade2 = nullptr;
                }
                else {
                    pTileFade2 = pTiles[18];
                }
                if (!BarterMenu::pCurrItem ||
                    static_cast<TESObjectARMO*>(BarterMenu::pCurrItem->pContainerObj)
                            ->TESObjectARMO::GetArmorRating() == 0.0) {
                    InterfaceManager::UpdatePipBoyInventoryEffects(
                        nullptr, pTileFade2, nullptr, BarterMenu::pCurrItem, false
                    );
                }
                else {
                    InterfaceManager::UpdatePipBoyInventoryEffects(
                        pTiles[17], pTileFade2, nullptr, BarterMenu::pCurrItem, false
                    );
                }
            }
        }
    }
}

int32_t BarterMenu::GetClass() {
    return 1053;
}

bool BarterMenu::DoGamepad(int32_t a1, float a2) {
    bool bresult = false;
    if (Interface::IsTopMenuID(Interface::eBarterMenu)) {
        switch (a1) {
            case 3:
            case 4: {
                if (plbCurrListbox == &lbPlayerItems) {
                    lbPlayerItems.SaveListState();
                    DoClick(2 * (a1 != 4) + 2, nullptr);
                    lbPlayerItems.RestoreListState(false);
                }
                else {
                    lbVendorItems.SaveListState();
                    DoClick(2 * (a1 != 4) + 7, nullptr);
                    lbVendorItems.RestoreListState(false);
                }
                bresult = true;
                break;
            }
            case 13: {
                if ((!lbPlayerItems.IsEnabled()) && (lbPlayerItems.NumItems())) {
                    lbVendorItems.SaveListState();
                    DoEnter(0, nullptr);
                    lbPlayerItems.RestoreListState(true);
                    bresult = true;
                }

                break;
            }
            case 14: {
                if ((!lbVendorItems.IsEnabled()) && (lbVendorItems.NumItems())) {
                    lbPlayerItems.SaveListState();
                    DoEnter(1, nullptr);
                    lbVendorItems.RestoreListState(true);
                    bresult = true;
                }

                break;
            }
            default: {
                return bresult;
            }
        }
    }
    return bresult;
}

void BarterMenu::UpdateLists(TESBoundObject* apUpdateObj) {
    int32_t pnewHighlight = 0;
    if ((plbCurrListbox) && (plbCurrListbox->GetHighlightedItem())) {
        plbCurrListbox->SaveListState();
    }

    if (apUpdateObj) {
        BSSimpleList<ItemChange*> pnewItemGroup;
        InventoryChanges* pplayerInventory = InventoryChanges::GetInventoryChanges(pPlayer);
        InventoryChanges* pvendorInventory = pDataHandler->GetBarterContainer();
        if (pplayerInventory) {
            pplayerInventory->GetItemGroup(apUpdateObj, &pnewItemGroup);
            lbPlayerItems.RemoveItemGroup(apUpdateObj);
            for (ItemChange*& rpItem : pnewItemGroup.ItemsUntilNull()) {
                if (BarterMenu::PlayerFirstPassFilterFunc(rpItem)) {
                    ItemChange* apThis = rpItem;
                    if (apThis) {
                        delete apThis;
                    }
                }
                else {
                    const char* FullName = rpItem->GetFullName();
                    lbPlayerItems.AddItem(rpItem, FullName, nullptr, nullptr);
                }
            }
            pnewItemGroup.RemoveAll();
        }
        if (pvendorInventory) {
            pvendorInventory->GetItemGroup(apUpdateObj, &pnewItemGroup);
            lbVendorItems.RemoveItemGroup(apUpdateObj);
            for (ItemChange*& rpItem : pnewItemGroup.ItemsUntilNull()) {
                if (BarterMenu::VendorFirstPassFilterFunc(rpItem)) {
                    ItemChange* m_item = rpItem;
                    if (m_item) {
                        delete m_item;
                    }
                }
                else {
                    const char* v3 = rpItem->GetFullName();
                    lbVendorItems.AddItem(rpItem, v3, nullptr, nullptr);
                }
            }
            pnewItemGroup.RemoveAll();
        }
    }
    else {
        pDataHandler->CreateBarterContainer(pVendorRef);
        lbPlayerItems.BuildFromContainer(
            pPlayer, &BarterMenu::PlayerFirstPassFilterFunc, nullptr, nullptr, false, false
        );
        lbVendorItems.BuildFromContainer(
            pVendorRef, &BarterMenu::VendorFirstPassFilterFunc, nullptr, nullptr, false, true
        );
    }
    for (ItemChange*& rpItem : ItemsToBuy.ItemsUntilNull()) {
        if (!apUpdateObj || rpItem->pContainerObj == apUpdateObj) {
            ExtraDataList* pnewExtra;
            ItemChange* pnewItem;
            ItemChange* v16 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (v16) {
                uint32_t FormHealth_0 = rpItem->iNumber;
                pnewItem = ::new (v16) ItemChange(rpItem->pContainerObj, FormHealth_0);
            }
            else {
                pnewItem = nullptr;
            }
            ItemChange* pnewItem_2 = pnewItem;
            if (rpItem->pExtraObjectList) {
                pnewExtra = rpItem->pExtraObjectList->m_item;
            }
            else {
                pnewExtra = nullptr;
            }
            ExtraDataList* pnewExtra_3 = pnewExtra;
            Tile* pnewTile = nullptr;
            if (pnewExtra) {
                pnewItem_2->pExtraObjectList->AddHead(pnewExtra_3);
            }
            const char* v5 = pnewItem_2->GetFullName();
            pnewTile = lbPlayerItems.AddItem(pnewItem_2, v5, nullptr, nullptr);
            pnewTile->Set(BarterMenu::iUT_IsBarterSelected, 1);
        }
    }
    for (ItemChange*& rpItem : ItemsToSell.ItemsUntilNull()) {
        if (!apUpdateObj || rpItem->pContainerObj == apUpdateObj) {
            ExtraDataList* pnewExtra_2;
            ItemChange* pnewItem_3;
            ItemChange* v15 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
            if (v15) {
                uint32_t v6 = rpItem->iNumber;
                pnewItem_3 = ::new (v15) ItemChange(rpItem->pContainerObj, v6);
            }
            else {
                pnewItem_3 = nullptr;
            }
            ItemChange* pnewItem_4 = pnewItem_3;
            if (rpItem->pExtraObjectList) {
                pnewExtra_2 = rpItem->pExtraObjectList->m_item;
            }
            else {
                pnewExtra_2 = nullptr;
            }
            ExtraDataList* pnewExtra_4 = pnewExtra_2;
            Tile* pnewTile_2 = nullptr;
            if (pnewExtra_2) {
                pnewItem_4->pExtraObjectList->AddHead(pnewExtra_4);
            }
            const char* v7 = pnewItem_4->GetFullName();
            pnewTile_2 = lbVendorItems.AddItem(pnewItem_4, v7, nullptr, nullptr);
            pnewTile_2->Set(BarterMenu::iUT_IsBarterSelected, 1);
        }
    }
    lbPlayerItems.Traverse(&BarterMenu::RemoveStolenItems, 0, 0x7FFFFFFF);
    lbPlayerItems.Traverse(&BarterMenu::InitFunc, 0, 0x7FFFFFFF);
    lbPlayerItems.Filter(&BarterMenu::FilterModeFunc);
    // Sort is called through each list's vtable; a call on the member itself would be devirtualised, so it goes through
    // a pointer.
    ListBox<ItemChange*>* plbSorted = &lbPlayerItems;
    plbSorted->Sort(&BarterMenu::SortFunc);
    lbVendorItems.Traverse(&BarterMenu::InitFunc, 0, 0x7FFFFFFF);
    lbVendorItems.Filter(&BarterMenu::FilterModeFunc);
    plbSorted = &lbVendorItems;
    plbSorted->Sort(&BarterMenu::SortFunc);
    if (!plbCurrListbox && InterfaceManager::Has360Controller()) {
        plbCurrListbox = &lbVendorItems;
    }
    if (plbCurrListbox) {
        if (plbCurrListbox->NumUnfiltered() ||
            (eFilterSound = MENU_SOUND_NONE,
             (DoClick(&lbPlayerItems != plbCurrListbox ? 9 : 4, nullptr),
              (eFilterSound = MENU_SOUND_NEXT_PREVIOUS, plbCurrListbox->NumUnfiltered())))) {
            plbCurrListbox->RestoreListState(true);
        }
        else {
            DoEnter(plbCurrListbox == &lbPlayerItems, nullptr);
            if (InterfaceManager::Has360Controller()) {
                Tile* FirstItem = plbCurrListbox->GetFirstItem();
                InterfaceManager* v8 = InterfaceManager::Instance();
                v8->SetCurrentFocusTarget(FirstItem, 4035, true);
            }
        }
    }
    UpdateCapsLabels();
}

void BarterMenu::UpdateCapsLabels() {
    char Src[68];
    iPlayerCaps = pPlayer->GetGoldAmount();
    iVendorCaps = pDataHandler->GetBarterContainer()->GetGoldAmount();
    BSsprintf(Src, 64u, "%s   %d", sInventoryCaps.String(), iPlayerCaps);
    pTiles[5]->Set(4036, Src, true);
    BSsprintf(Src, 64u, "%s   %d", sInventoryCaps.String(), iVendorCaps);
    pTiles[10]->Set(4036, Src, true);
}

bool BarterMenu::AreAllTilesNonNull() {
    for (int32_t i = 0; i < 22; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

ItemChange* BarterMenu::GetTransactItem(BSSimpleList<ItemChange*>* apItemList, ItemChange* apCompareItem) {
    ItemChange* pfoundItem = nullptr;
    while (!pfoundItem && (apItemList && apItemList->m_item)) {
        ItemChange* pcurrItem = apItemList->m_item;
        TESBoundObject* FormFlags = apCompareItem->pContainerObj;
        if (FormFlags == pcurrItem->pContainerObj) {
            ExtraDataList* v5;
            ExtraDataList* m_item;
            if (apCompareItem->pExtraObjectList) {
                m_item = apCompareItem->pExtraObjectList->m_item;
            }
            else {
                m_item = nullptr;
            }
            if (pcurrItem->pExtraObjectList) {
                v5 = pcurrItem->pExtraObjectList->m_item;
            }
            else {
                v5 = nullptr;
            }
            if (m_item == v5) {
                pfoundItem = pcurrItem;
            }
        }
        apItemList = apItemList->m_pkNext;
    }
    return pfoundItem;
}

void BarterMenu::AddTransactItem(int32_t aiAmountSelected) {
    BSSimpleList<ItemChange*>* ptransactList;
    if (plbCurrListbox == &lbPlayerItems) {
        ptransactList = &ItemsToSell;
    }
    else {
        ptransactList = &ItemsToBuy;
    }
    ItemChange* ptransactItem = GetTransactItem(ptransactList, BarterMenu::pCurrItem);
    if (ptransactItem) {
        ptransactItem->iNumber = ptransactItem->iNumber + aiAmountSelected;
    }
    else {
        ExtraDataList* pextra;
        if (BarterMenu::pCurrItem && BarterMenu::pCurrItem->pExtraObjectList) {
            pextra = BarterMenu::pCurrItem->pExtraObjectList->m_item;
        }
        else {
            pextra = nullptr;
        }
        ItemChange* apMemory = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
        if (apMemory) {
            ptransactItem = ::new (apMemory) ItemChange(BarterMenu::pCurrItem->pContainerObj, aiAmountSelected);
        }
        else {
            ptransactItem = nullptr;
        }
        if (pextra) {
            ptransactItem->pExtraObjectList->AddHead(pextra);
        }
        ptransactList->AddTail(ptransactItem);
    }
}

void BarterMenu::RemoveTransactItem(int32_t aiAmountSelected) {
    BSSimpleList<ItemChange*>* ptransactList;
    if (plbCurrListbox == &lbPlayerItems) {
        ptransactList = &ItemsToBuy;
    }
    else {
        ptransactList = &ItemsToSell;
    }
    ItemChange* ptransactItem = GetTransactItem(ptransactList, BarterMenu::pCurrItem);
    // Tests the sign of the wrapped difference, not iNumber > aiAmountSelected, as the game does.
    if (static_cast<int32_t>(static_cast<uint32_t>(ptransactItem->iNumber) - static_cast<uint32_t>(aiAmountSelected)) >
        0) {
        ptransactItem->iNumber = ptransactItem->iNumber - aiAmountSelected;
    }
    else {
        ptransactList->Remove(ptransactItem);
        if (ptransactItem) {
            delete ptransactItem;
        }
    }
}

void BarterMenu::CompleteTransaction() {
    TESObjectREFR* pvendorContainer;
    XFNV_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
    XFNV_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
    if ((&pVendorRef->m_Extra)->GetMerchantContainer()) {
        pvendorContainer = (&pVendorRef->m_Extra)->GetMerchantContainer();
    }
    else {
        pvendorContainer = pVendorRef;
    }
    BaseProcess* CurrentAIProcess = pPlayer->pCurrentProcess;
    ItemChange* pammo = CurrentAIProcess->GetCurrentAmmo();
    for (ItemChange* pbuyItem : ItemsToBuy.ItemsUntilNull()) {
        ExtraDataList* pbuyExtra;
        if (pbuyItem->pExtraObjectList) {
            pbuyExtra = pbuyItem->pExtraObjectList->m_item;
        }
        else {
            pbuyExtra = nullptr;
        }
        ExtraDataList* pbuyExtra_2 = pbuyExtra;
        if (pbuyExtra) {
            Script::SetActionFlag(pVendorRef, pbuyExtra_2, SCRIPT_ACTION_ONSELL);
        }
        TESObjectREFR* X = pVendorRef;
        uint32_t FormHealth_0 = pbuyItem->iNumber;
        pDataHandler->RemoveItemBarterContainer(pbuyItem, FormHealth_0, X);
        ItemChange* v26 = pbuyItem;
        ItemChange* pbuyItem_2 = pbuyItem;
        if (pbuyItem) {
            delete pbuyItem_2;
        }
    }
    for (ItemChange* psellItem : ItemsToSell.ItemsUntilNull()) {
        ExtraDataList* psellExtra;
        if (psellItem->pExtraObjectList) {
            psellExtra = psellItem->pExtraObjectList->m_item;
        }
        else {
            psellExtra = nullptr;
        }
        ExtraDataList* psellExtra_2 = psellExtra;
        if (psellExtra) {
            Script::SetActionFlag(pPlayer, psellExtra_2, SCRIPT_ACTION_ONSELL);
        }
        uint32_t v9 = psellItem->iNumber;
        TESBoundObject* FormFlags = psellItem->pContainerObj;
        pPlayer->RemoveItem(FormFlags, psellExtra_2, v9, false, false, pvendorContainer, nullptr, nullptr, true, false);
        ItemChange* v24 = psellItem;
        ItemChange* psellItem_2 = psellItem;
        if (psellItem) {
            delete psellItem_2;
        }
    }
    BarterMenu::pMe->pTiles[13]->Set(4015, 0);
    BarterMenu::pMe->pTiles[12]->Set(4003, 0);
    InventoryChanges* pinventory = nullptr;
    float Xa = std::floorf(BarterMenu::pMe->fTransactionAmount);
    int32_t iwholeAmount = FloatToInt(Xa);
    int32_t v2 = std::abs(iwholeAmount);
    MiscStatManager::ModVal(MiscStatManager::MS_BARTER_AMOUNT_TRADED, v2);
    if (!(fTransactionAmount > 0.0)) {
        if (fTransactionAmount < 0.0) {
            pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
            if (pinventory) {
                pinventory->RemoveGold(pPlayer, -iwholeAmount, pvendorContainer);
            }
            BSAudio* v6 = &BSAudio::QInstance();
            BSSoundHandle* SoundHandleByName = v6->GetSoundHandleByName(&arHandle, "ITMBottlecapsDown", 257u);
            SoundHandleByName->Play(false);
        }
    }
    else {
        TESBoundObject* FormByNumericID = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
        ItemChange tempGold(FormByNumericID, 0);
        pinventory = InventoryChanges::GetInventoryChanges(pvendorContainer);
        if (pinventory) {
            TESObjectREFR* Xb = pVendorRef;
            int32_t v4 = BSMIN(iwholeAmount, iVendorCaps);
            pDataHandler->RemoveItemBarterContainer(&tempGold, v4, Xb);
        }
        BSAudio* v5 = &BSAudio::QInstance();
        BSSoundHandle* v16 = v5->GetSoundHandleByName(&a1, "ITMBottlecapsUp", 257u);
        v16->Play(false);
    }
    fTransactionAmount = 0.0f;
    ItemsToBuy.RemoveAll();
    ItemsToSell.RemoveAll();
    bItemsTraded = true;
    if (pammo) {
        TESBoundObject* Xc = pammo->pContainerObj;
        InventoryChanges* pinventory_2 = InventoryChanges::GetInventoryChanges(pPlayer);
        if (!pinventory_2->GetObjectCount(Xc)) {
            BaseProcess* v14 = pPlayer->pCurrentProcess;
            v14->SetCurrentAmmo(nullptr);
        }
    }
}

void BarterMenu::SetFilterModeTitle(Tile* apTitleTile) {
    uint32_t ePlayerFilterMode;
    char Dst[268];
    if (apTitleTile == pTiles[3]) {
        ePlayerFilterMode = this->ePlayerFilterMode;
    }
    else {
        ePlayerFilterMode = eVendorFilterMode;
    }
    const char* v2 = sLanguage.String();
    bool v12 = BSstrcmp(v2, "ENGLISH") != 0;
    switch (ePlayerFilterMode) {
        case 0u: {
            if (apTitleTile == pTiles[3]) {
                const char* v3 = sInventoryItems.String();
                BSstrcpy(Dst, 260, v3);
                if (!v12) {
                    BSstrupr(Dst, 260);
                }
            }
            else {
                const char* FullName = pVendorRef->GetFullName();
                BSstrcpy(Dst, 260, FullName);
            }
            break;
        }
        case 1u: {
            const char* v5 = sInventoryWeapons.String();
            BSstrcpy(Dst, 260, v5);
            if (!v12) {
                BSstrupr(Dst, 260);
            }
            break;
        }
        case 2u: {
            const char* v6 = sInventoryApparel.String();
            BSstrcpy(Dst, 260, v6);
            if (!v12) {
                BSstrupr(Dst, 260);
            }
            break;
        }
        case 3u: {
            const char* v7 = sInventoryAid.String();
            BSstrcpy(Dst, 260, v7);
            if (!v12) {
                BSstrupr(Dst, 260);
            }
            break;
        }
        case 4u: {
            const char* v8 = sInventoryMisc.String();
            BSstrcpy(Dst, 260, v8);
            if (!v12) {
                BSstrupr(Dst, 260);
            }
            break;
        }
        case 5u: {
            const char* v9 = sInventoryAmmo.String();
            BSstrcpy(Dst, 260, v9);
            if (!v12) {
                BSstrupr(Dst, 260);
            }
            break;
        }
        default: {
            break;
        }
    }
    apTitleTile->Set(4036, Dst, true);
}

void BarterMenu::ForceBarterUpdate(bool abNow) {
    if (abNow) {
        if (pMe) {
            pMe->UpdateLists(nullptr);
        }
        bForceUpdate = false;
    }
    else {
        bForceUpdate = true;
    }
}

Tile* BarterMenu::Create(TESObjectREFR* apVendor, int32_t aiDiscount) {
    InterfaceManager* pim = InterfaceManager::Instance();
    if (apVendor) {
        int32_t v4;
        BarterMenu* Menu_;
        int32_t v3 = BarterMenu::GetClassStatic();
        Tile* pold_menu_tile = Menu::GetMenuByClass(v3);
        if (pold_menu_tile) {
            delete pold_menu_tile;
        }
        Tile* ptile = pim->GetMenusRoot()->ReadFile("Data\\Menus\\barter_menu.xml");
        if (ptile) {
            Menu_ = static_cast<BarterMenu*>(ptile->GetMenu());
        }
        else {
            Menu_ = nullptr;
        }
        BarterMenu::pMe = Menu_;
        if (Menu_ && (v4 = BarterMenu::pMe->GetClass(), v4 == BarterMenu::GetClassStatic())) {
            BarterMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            if (BarterMenu::pMe->AreAllTilesNonNull()) {
                if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                    float afValue = Tile::GetMaximumDepth();
                    ptile->Set(4013, afValue, true);
                }
                DialogMenu::ServiceStart();
                int32_t v5 = BarterMenu::GetClassStatic();
                pim->xTutorialManager.ShowMessage(Interface::TUT_BARTER, v5, 512u);
                BarterMenu::pMe->lbPlayerItems.Initialize(BarterMenu::pMe->pTiles[6], "CM_list_template");
                BarterMenu::pMe->lbVendorItems.Initialize(BarterMenu::pMe->pTiles[11], "CM_list_template");
                const char* v6 = sInventoryItems.String();
                BarterMenu::pMe->pTiles[3]->Set(4036, v6, true);
                const char* afValuea = sInventoryCaps.String();
                Tile* ChildByName = ptile->GetChildByName("BM_CapsLabel");
                ChildByName->Set(4036, afValuea, true);
                const char* v8 = sAccept.String();
                BarterMenu::pMe->pTiles[13]->Set(4036, v8, true);
                const char* v9 = sExit.String();
                BarterMenu::pMe->pTiles[19]->Set(4036, v9, true);
                int32_t a2 = Tile::TextToTrait("_Title");
                Tile* v25 = ptile->GetChildByName("WeightInfo");
                if (v25) {
                    const char* v10 = sInventoryWeightUpper.String();
                    v25->Set(a2, v10, true);
                }
                Tile* v26 = ptile->GetChildByName("ValueInfo");
                if (v26) {
                    const char* v11 = sInventoryValue.String();
                    v26->Set(a2, v11, true);
                }
                if (BarterMenu::pMe->pTiles[14]) {
                    const char* v12 = sInventoryDamage.String();
                    BarterMenu::pMe->pTiles[14]->Set(a2, v12, true);
                }
                if (BarterMenu::pMe->pTiles[15]) {
                    const char* v13 = sInventoryDamagePerSecond.String();
                    BarterMenu::pMe->pTiles[15]->Set(a2, v13, true);
                }
                if (BarterMenu::pMe->pTiles[16]) {
                    const char* v14 = sInventoryStrReq.String();
                    BarterMenu::pMe->pTiles[16]->Set(a2, v14, true);
                }
                if (BarterMenu::pMe->pTiles[17]) {
                    const char* v15 = sInventoryDamageResistance.String();
                    BarterMenu::pMe->pTiles[17]->Set(a2, v15, true);
                }
                if (BarterMenu::pMe->pTiles[18]) {
                    const char* v16 = sInventoryDamageThreshold.String();
                    BarterMenu::pMe->pTiles[18]->Set(a2, v16, true);
                }
                BarterMenu::pMe->pVendorRef = apVendor;
                int32_t v17 = BSMAX(aiDiscount, -100);
                BarterMenu::pMe->iDiscount = BSMIN(v17, 100);
                BarterMenu::pMe->fTransactionAmount = 0.0f;
                float* p_fSellMult = &BarterMenu::pMe->fSellMult;
                float* afValueb = &BarterMenu::pMe->fBuyMult;
                int32_t ClampedActorValue = pPlayer->GetClampedActorValue(ActorValue::BARTER);
                GamePlayFormulas::GetBarterMultipliers(ClampedActorValue, afValueb, p_fSellMult);
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                BarterMenu::pMe->PrepForVisibility(false);
                BarterMenu::pMe->UpdateLists(nullptr);
                return ptile;
            }

            Warning("MENUS: Barter Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }

        if ((BarterMenu::pMe) && (BarterMenu::pMe->pMenu) && (BarterMenu::pMe)) {
            delete BarterMenu::pMe;
        }

        return nullptr;
    }

    Warning(WARN_MENUS, "Barter menu called with NULL vendor!  Oh, noes!");
    return nullptr;
}

int32_t BarterMenu::GetClassStatic() {
    return 1053;
}

void BarterMenu::Close() {
    BaseProcess* currentAIProcess;
    TESTopic* apTopic;
    if (BarterMenu::pMe) {
        currentAIProcess = pPlayer->MobileObject::GetCurrentAIProcess();
        if (currentAIProcess->Get3DUpdateFlag(RESET_MODEL)) {
            pPlayer->UpdatePlayer3d();
        }
        apTopic = nullptr;
        if (BarterMenu::pMe->bItemsTraded) {
            apTopic = TESTopic::GetTopic(DT_SERVICE, 4);
        }
        DialogMenu::ServiceEnd(apTopic);
        BarterMenu::pMe->pMenu->Set(6002, 1);
        BarterMenu::pMe->StartFadeOut();
    }
}

void BarterMenu::CancelConfirmFunc() {
    if (!Interface::GetMessageMenuResult()) {
        BarterMenu::Close();
    }
}

bool BarterMenu::PlayerFirstPassFilterFunc(ItemChange* apItem) {
    TESAmmo* pammo;
    TESObjectWEAP* pweap;
    TESBoundObject* pobj;
    bool bisFiltered = false;
    if (apItem) {
        pobj = apItem->pContainerObj;
    }
    else {
        pobj = nullptr;
    }
    TESBipedModelForm* pbipedForm = TESBipedModelForm::GetFormAsBipedModel(pobj);
    TESFullName* pfull = dynamic_cast<TESFullName*>(pobj);
    if (apItem->pContainerObj->GetFormType() == WEAP_ID) {
        pweap = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
    }
    else {
        pweap = nullptr;
    }
    if (apItem->pContainerObj->GetFormType() == AMMO_ID) {
        pammo = static_cast<TESAmmo*>(apItem->pContainerObj);
    }
    else {
        pammo = nullptr;
    }
    if (!pobj ||
        ((pbipedForm && !pbipedForm->GetPlayable()) ||
         (pobj->GetQuestObject() ||
          (TESContainer::IsGold(pobj) ||
           (!pfull ||
            (!pfull->GetFullNameLength() ||
             ((pweap && !pweap->GetIsPlayable()) ||
              ((pammo && !pammo->GetIsPlayable()) || pobj->GetFormType() == KEYM_ID)))))))) {
        return true;
    }
    return bisFiltered;
}

bool BarterMenu::VendorFirstPassFilterFunc(ItemChange* apItem) {
    TESAmmo* pammo;
    TESObjectWEAP* pweap;
    TESBoundObject* pobj;
    Actor* pactor;
    bool bisFiltered_2 = false;
    if (BarterMenu::pMe->pVendorRef && BarterMenu::pMe->pVendorRef->IsActor()) {
        pactor = static_cast<Actor*>(BarterMenu::pMe->pVendorRef);
    }
    else {
        pactor = nullptr;
    }
    if (apItem) {
        pobj = apItem->pContainerObj;
    }
    else {
        pobj = nullptr;
    }
    if (pactor) {
        switch (apItem->pContainerObj->GetFormType()) {
            case ARMO_ID: {
                bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(2u);
                break;
            }
            case BOOK_ID: {
                bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(8u);
                break;
            }
            case MISC_ID:
            case COBJ_ID:
            case IMOD_ID: {
                bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(1024u);
                break;
            }
            case WEAP_ID:
            case AMMO_ID: {
                bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(1u);
                break;
            }
            case KEYM_ID: {
                bisFiltered_2 = true;
                break;
            }
            case ALCH_ID: {
                switch (BGSEquipType::GetEquipType(apItem->pContainerObj)) {
                    case BGSEquipType::ET_CHEMS: {
                        bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(32u);
                        break;
                    }
                    case BGSEquipType::ET_STIMPACK: {
                        bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(64u);
                        break;
                    }
                    case BGSEquipType::ET_FOOD: {
                        bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(16u);
                        break;
                    }
                    case BGSEquipType::ET_ALCOHOL: {
                        bisFiltered_2 = !pactor->GetActorBase()->GetOffersService(4u);
                        break;
                    }
                    default: {
                        goto LABEL_25;
                    }
                }
                break;
            }
            case NOTE_ID: {
                bool bisFiltered =
                    !pobj || TESValueForm::GetFormValue(pobj) <= 0 || !pactor->GetActorBase()->GetOffersService(1024u);
                bisFiltered_2 = bisFiltered;
                break;
            }
            default: {
                break;
            }
        }
    }
LABEL_25:;
    TESBipedModelForm* pbipedForm = TESBipedModelForm::GetFormAsBipedModel(pobj);
    TESFullName* pfull = dynamic_cast<TESFullName*>(pobj);
    if (apItem->pContainerObj->GetFormType() == WEAP_ID) {
        pweap = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
    }
    else {
        pweap = nullptr;
    }
    if (apItem->pContainerObj->GetFormType() == AMMO_ID) {
        pammo = static_cast<TESAmmo*>(apItem->pContainerObj);
    }
    else {
        pammo = nullptr;
    }
    if (!pobj ||
        ((pbipedForm && !pbipedForm->GetPlayable()) ||
         (TESContainer::IsGold(pobj) ||
          (!pfull ||
           (!pfull->GetFullNameLength() ||
            ((pweap && !pweap->GetIsPlayable()) || (pammo && !pammo->GetIsPlayable()))))))) {
        return true;
    }
    return bisFiltered_2;
}

void BarterMenu::RemoveStolenItems(Tile* apTile, ItemChange* apItem) {
    Actor* pactor;
    if (BarterMenu::pMe->pVendorRef && BarterMenu::pMe->pVendorRef->IsActor()) {
        pactor = static_cast<Actor*>(BarterMenu::pMe->pVendorRef);
    }
    else {
        pactor = nullptr;
    }
    BSSimpleList<ExtraDataList*> removeList;
    for (ExtraDataList* pextra : apItem->pExtraObjectList->ItemsUntilNull()) {
        if ((pactor) && (pextra->GetOwner())) {
            TESObjectREFR ownedItem;
            TESForm* Owner = pextra->GetOwner();
            ownedItem.SetOwner(Owner);
            TESGlobal* Global = pextra->GetGlobal();
            ownedItem.SetGlobal(Global);
            int32_t Rank = pextra->GetRank();
            ownedItem.SetRank(Rank);
            ownedItem.SetObjectReference(apItem->pContainerObj);
            if (ownedItem.IsAnOwner(pactor, true)) {
                removeList.AddHead(pextra);
            }
        }
    }
    for (ExtraDataList*& rpItem : removeList.ItemsUntilNull()) {
        uint32_t FormHealth_0 = apItem->iNumber;
        apItem->iNumber = FormHealth_0 - rpItem->GetCount();
        apItem->pExtraObjectList->Remove(rpItem);
    }
    removeList.RemoveAll();
}

float BarterMenu::GetSortValue(Tile* apTile, ItemChange* apItem) {
    float fsortValue_5;
    fsortValue_5 = apItem->GetItemValue();
    float forigValue = fsortValue_5;
    bool binVendorList = BarterMenu::pMe->lbVendorItems.IsListItem(apTile);
    bool bisBarterSelected = apTile->IsTrue(BarterMenu::iUT_IsBarterSelected);
    float freturnVal = 0.0;
    if (!(fsortValue_5 < 0.0)) {
        if ((!binVendorList || bisBarterSelected) && (binVendorList || !bisBarterSelected)) {
            if ((binVendorList && bisBarterSelected) || (!binVendorList && !bisBarterSelected)) {
                float fSellMult;
                if (apItem->pContainerObj->GetFormType() == CHIP_ID ||
                    apItem->pContainerObj->GetFormType() == CMNY_ID) {
                    fSellMult = 1.0f;
                }
                else {
                    fSellMult = BarterMenu::pMe->fSellMult;
                }
                fsortValue_5 = fsortValue_5 * fSellMult;
                if (BarterMenu::pMe->iDiscount <= 0) {
                    if (BarterMenu::pMe->iDiscount < 0) {
                        fsortValue_5 =
                            (1.0 - static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0) * fsortValue_5;
                    }
                }
                else {
                    fsortValue_5 =
                        (static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0 + 1.0) * fsortValue_5;
                }
                if (!(std::fabsf(fsortValue_5) > 1.0)) {
                    fsortValue_5 = BSRoundToNearest(fsortValue_5, 0.1f);
                }
                else {
                    fsortValue_5 = BSRoundToNearest(fsortValue_5, 1.0f);
                }
                return BSMIN(fsortValue_5, forigValue);
            }
        }
        else {
            float fBuyMult;
            if (apItem->pContainerObj->GetFormType() == CHIP_ID || apItem->pContainerObj->GetFormType() == CMNY_ID) {
                fBuyMult = 1.0f;
            }
            else {
                fBuyMult = BarterMenu::pMe->fBuyMult;
            }
            fsortValue_5 = fsortValue_5 * fBuyMult;
            if (BarterMenu::pMe->iDiscount <= 0) {
                if (BarterMenu::pMe->iDiscount < 0) {
                    fsortValue_5 =
                        (static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0 + 1.0) * fsortValue_5;
                }
            }
            else {
                fsortValue_5 = (1.0 - static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0) * fsortValue_5;
            }
            TESBoundObject* a1 = apItem->pContainerObj;
            BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::CALCULATE_BUY_PRICE, pPlayer, a1, &fsortValue_5);
            if (!(std::fabsf(fsortValue_5) > 1.0)) {
                fsortValue_5 = BSRoundToNearest(fsortValue_5, 0.1f);
            }
            else {
                fsortValue_5 = BSRoundToNearest(fsortValue_5, 1.0f);
            }
            return BSMAX(fsortValue_5, forigValue);
        }
    }
    else {
        return -1.0f;
    }
    return freturnVal;
}

int32_t BarterMenu::SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2) {
    ItemChange* data = apItem1->data;
    ItemChange* v5 = apItem2->data;
    bool IsTrue = apItem1->pTile->IsTrue(BarterMenu::iUT_IsBarterSelected);
    bool v6 = apItem2->pTile->IsTrue(BarterMenu::iUT_IsBarterSelected);
    if (IsTrue && !v6) {
        return -1;
    }
    if (!IsTrue && v6) {
        return 1;
    }
    const char* String = apItem2->pTile->GetString(4036);
    const char* v2 = apItem1->pTile->GetString(4036);
    return _tcscmp(v2, String);
}

void BarterMenu::InitFunc(Tile* apTile, ItemChange* apItem) {
    int32_t iitemCount;
    char Dst[132];
    float fsortValue = BarterMenu::GetSortValue(apTile, apItem);
    if (apTile->IsTrue(BarterMenu::iUT_IsBarterSelected)) {
        iitemCount = apItem->iNumber;
    }
    else {
        uint32_t a3;
        uint32_t FormHealth_0;
        ItemChange* ptransactItem = BarterMenu::pMe->GetTransactItem(&BarterMenu::pMe->ItemsToBuy, apItem);
        if (!ptransactItem) {
            ptransactItem = BarterMenu::pMe->GetTransactItem(&BarterMenu::pMe->ItemsToSell, apItem);
        }
        if (ptransactItem) {
            FormHealth_0 = ptransactItem->iNumber;
        }
        else {
            FormHealth_0 = 0;
        }
        iitemCount = apItem->iNumber - FormHealth_0;
        if (ptransactItem) {
            a3 = ptransactItem->iNumber;
        }
        else {
            a3 = 0;
        }
        int32_t v2 = Tile::TextToTrait("_NumBartered");
        apTile->Set(v2, static_cast<int32_t>(a3));
    }
    if (iitemCount > 1) {
        BSsprintf(Dst, 128, "%s (%d)", apItem->GetFullName(), iitemCount);
        apTile->Set(4036, Dst, true);
    }
    if (apItem->GetWorn(false) && !apTile->IsTrue(BarterMenu::iUT_IsBarterSelected)) {
        int32_t v3 = Tile::AddUserTrait("_IsEquipped", -1);
        apTile->Set(v3, 1);
    }
    if (!(fsortValue <= 0.0)) {
        if (!(fsortValue < 1.0)) {
            BSsprintf(Dst, 128, "%.0f", fsortValue);
        }
        else {
            BSsprintf(Dst, 128, "%.1f", fsortValue);
        }
    }
    else {
        BSstrcpy(Dst, 128, "--");
    }
    apTile->Set(g_BarterMenu_Trait_Value, Dst, true);
    const char* FullName = apItem->GetFullName();
    apTile->SetName(FullName);
    apTile->Set(4016, 0.0f, true);
}

void BarterMenu::ItemSelectCallback(int32_t aiAmountSelected) {
    Tile* pcurrTile = BarterMenu::pMe->plbCurrListbox->GetTileFor(BarterMenu::pCurrItem);
    if ((aiAmountSelected) && (BarterMenu::pCurrItem) && (pcurrTile)) {
        BarterMenu::pMe->fTransactionAmount = BarterMenu::GetSortValue(pcurrTile, BarterMenu::pCurrItem) *
                static_cast<double>(2 * (BarterMenu::pMe->plbCurrListbox == &BarterMenu::pMe->lbPlayerItems) - 1) *
                static_cast<double>(aiAmountSelected) +
            BarterMenu::pMe->fTransactionAmount;
        bool afValue = BarterMenu::pMe->plbCurrListbox == &BarterMenu::pMe->lbVendorItems;
        TESBoundObject* FormFlags = BarterMenu::pCurrItem->pContainerObj;
        pPlayer->PlayPickUpSound(FormFlags, afValue, false);
        if (pcurrTile->IsTrue(BarterMenu::iUT_IsBarterSelected)) {
            BarterMenu::pMe->RemoveTransactItem(aiAmountSelected);
        }
        else {
            BarterMenu::pMe->AddTransactItem(aiAmountSelected);
        }
        TESBoundObject* X = BarterMenu::pCurrItem->pContainerObj;
        BarterMenu::pMe->UpdateLists(X);
        if (BarterMenu::pMe->ItemsToBuy.IsEmpty() && BarterMenu::pMe->ItemsToSell.IsEmpty()) {
            BarterMenu::pMe->pTiles[13]->Set(4015, 0);
            BarterMenu::pMe->pTiles[12]->Set(4003, 0);
        }
        else {
            int32_t X_4;
            char Src[16];
            float Xa = std::floorf(BarterMenu::pMe->fTransactionAmount);
            int32_t iwholeAmount = FloatToInt(Xa);
            if (!(BarterMenu::pMe->fTransactionAmount > 0.0)) {
                X_4 = 1;
            }
            else {
                X_4 = 2;
            }
            BarterMenu::pMe->pMenu->Set(g_BarterMenu_Trait_ArrowDirection, X_4);
            int32_t v1 = BSMIN(iwholeAmount, BarterMenu::pMe->iVendorCaps);
            BSsprintf(Src, 16, "%d", BSAbs(v1));
            BarterMenu::pMe->pTiles[12]->Set(g_BarterMenu_Trait_Value, Src, true);
            BarterMenu::pMe->pTiles[12]->Set(4003, 1);
            if (!(BarterMenu::pMe->fTransactionAmount > 0.0) &&
                !(-BarterMenu::pMe->fTransactionAmount <= static_cast<double>(BarterMenu::pMe->iPlayerCaps))) {
                BarterMenu::pMe->pTiles[13]->Set(4015, 0);
                BarterMenu::pMe->pTiles[12]->Set(4009, 64.0f, true);
            }
            else {
                BarterMenu::pMe->pTiles[13]->Set(4015, 1);
                if (!(BarterMenu::pMe->fTransactionAmount > 0.0) ||
                    !(BarterMenu::pMe->fTransactionAmount > static_cast<double>(BarterMenu::pMe->iVendorCaps))) {
                    BarterMenu::pMe->pTiles[12]->Set(4009, 255.0f, true);
                }
                else {
                    BarterMenu::pMe->pTiles[12]->Set(4009, 64.0f, true);
                }
            }
        }
    }
}

bool BarterMenu::FilterModeFunc(ItemChange* apItem) {
    BarterMenu::FILTER_MODE emode;
    bool bisFiltered = false;
    ENUM_FORM_ID eid_2 = apItem->pContainerObj->GetFormType();
    Tile* pcurrTile = BarterMenu::pMe->lbPlayerItems.GetTileFor(apItem);
    if (pcurrTile) {
        emode = BarterMenu::pMe->ePlayerFilterMode;
    }
    else {
        pcurrTile = BarterMenu::pMe->lbVendorItems.GetTileFor(apItem);
        emode = BarterMenu::pMe->eVendorFilterMode;
    }
    switch (emode) {
        case BarterMenu::FM_WEAPONS: {
            bisFiltered = eid_2 != WEAP_ID;
            break;
        }
        case BarterMenu::FM_APPAREL: {
            bool v7 = eid_2 != ARMO_ID && eid_2 != CLOT_ID;
            bisFiltered = v7;
            break;
        }
        case BarterMenu::FM_AID: {
            bool v6 = eid_2 != INGR_ID && (eid_2 != ALCH_ID && eid_2 != BOOK_ID);
            bisFiltered = v6;
            break;
        }
        case BarterMenu::FM_MISC: {
            bool v5 = eid_2 == WEAP_ID ||
                (eid_2 == ARMO_ID ||
                 (eid_2 == CLOT_ID ||
                  (eid_2 == INGR_ID ||
                   (eid_2 == ALCH_ID ||
                    (eid_2 == AMMO_ID ||
                     (eid_2 == BOOK_ID || (eid_2 == CHIP_ID || (eid_2 == CCRD_ID || eid_2 == CMNY_ID))))))));
            bisFiltered = v5;
            break;
        }
        case BarterMenu::FM_AMMO: {
            bisFiltered = eid_2 != AMMO_ID;
            break;
        }
        default: {
            break;
        }
    }
    if (pcurrTile) {
        bool v3 = true;
        if (!bisFiltered) {
            double FormHealth_0 = static_cast<double>(apItem->iNumber);
            int32_t v1 = Tile::TextToTrait("_NumBartered");
            if (!(FormHealth_0 - pcurrTile->GetFloat(v1) <= 0.0)) {
                return false;
            }
        }
        return v3;
    }
    return bisFiltered;
}
