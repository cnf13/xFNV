#include "Fallout/Interface/Menus/BarterMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
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
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/tchar.h"

#include <cmath>
#include <cstdint>
#include <limits>

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

void BarterMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 22) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 2:
        case 3:
        case 4: {
            Interface::PlayMenuSound(static_cast<int32_t>(eFilterSound));
            do {
                ePlayerFilterMode = static_cast<BarterMenu::FILTER_MODE>(ePlayerFilterMode + 2 * (aiID != 2) - 1);
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
                eVendorFilterMode = static_cast<BarterMenu::FILTER_MODE>(eVendorFilterMode + 2 * (aiID != 7) - 1);
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
                const char* pNo = sNo.String();
                const char* pYes = sYes.String();
                Interface::CreateMessageMenu(
                    static_cast<const char*>(nullptr),
                    static_cast<const char*>(nullptr),
                    sCancelBarter.String(),
                    &BarterMenu::CancelConfirmFunc,
                    0,
                    23,
                    0.0f,
                    0.0f,
                    pYes,
                    pNo,
                    nullptr
                );
            }
            else {
                BarterMenu::Close();
            }
            break;
        }
        case 23: {
            if (BarterMenu::pCurrItem && apTarget) {
                BarterMenu::bCrossFade = false;
                uint32_t uiNumber = BarterMenu::pCurrItem->iNumber;
                int32_t iNumBarteredTrait = Tile::TextToTrait("_NumBartered");
                float fNumBartered = apTarget->GetFloat(iNumBarteredTrait);
                int32_t iItemCount = uiNumber - FloatToInt(fNumBartered);
                if (Interface::IsConsoleVisible()) {
                    TESObjectREFR* pOriginalRef = nullptr;
                    if (BarterMenu::pCurrItem->pExtraObjectList && BarterMenu::pCurrItem->pExtraObjectList->m_item) {
                        pOriginalRef = BarterMenu::pCurrItem->pExtraObjectList->m_item->GetOriginalReference();
                    }
                    if (pOriginalRef) {
                        int32_t iFormID = pOriginalRef->iFormID;
                        Interface::PrintLine(
                            "Original Container Ref: '%s' (%08x)",
                            pOriginalRef->GetFormEditorID(),
                            iFormID
                        );
                    }
                    else {
                        Interface::PrintLine("Original Container Ref: NONE");
                    }
                }
                else if (iItemCount <= iInventoryAskQuantityAt.Int()) {
                    BarterMenu::ItemSelectCallback(1);
                }
                else {
                    plbCurrListbox->SaveListState();
                    plbCurrListbox->Highlight(nullptr);
                    QuantityMenu::Create(
                        iItemCount,
                        &BarterMenu::ItemSelectCallback,
                        std::numeric_limits<int32_t>::max()
                    );
                }
            }

            break;
        }
        default: {
            return;
        }
    }
}

void BarterMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 22) {
        apTarget = pTiles[aiID];
    }
    if (aiID) {
        if (aiID == 1) {
            DoLeave(23, nullptr);
            static_cast<ListBoxBase*>(&lbPlayerItems)->Highlight(nullptr);
            pMenu->Set(g_BarterMenu_Trait_IsContainerListSelected, 1);
            plbCurrListbox = &lbVendorItems;
        }
        else if (aiID == 23) {
            BarterMenu::bCrossFade = true;
            if (!plbCurrListbox) {
                DoEnter(lbVendorItems.IsListItem(apTarget), nullptr);
            }
            BarterMenu::pCurrItem = plbCurrListbox->GetHighlightedData();
            if (BarterMenu::pCurrItem) {
                Interface::PopulateItemStatsDisplay(BarterMenu::pCurrItem, pTiles[21]);
                pTiles[21]->Set(4003, 1);
                const char* pIconOne = BarterMenu::pCurrItem->GetIconOne(nullptr);
                pTiles[20]->Set(4044, pIconOne, true);
                pTiles[20]->Set(4003, 1);
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

void BarterMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    pTiles[21]->Set(4003, 0);
    pTiles[20]->Set(4003, 0);
}

void BarterMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
    if (aiID == 3 || aiID == 8) {
        if (InterfaceManager::Instance()->GetMouseWheel() >= 0) {
            DoClick(aiID != 3 ? 9 : 4, nullptr);
        }
        else {
            DoClick(aiID != 3 ? 7 : 2, nullptr);
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
                pTiles[14],
                pTiles[15],
                pTiles[16],
                BarterMenu::pCurrItem,
                false
            );
        }
        else if (BarterMenu::pCurrItem && BarterMenu::pCurrItem->pContainerObj->GetFormType() == ARMO_ID) {
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
                static_cast<TESObjectARMO*>(BarterMenu::pCurrItem->pContainerObj)->TESObjectARMO::GetArmorRating() ==
                    0.0) {
                InterfaceManager::UpdatePipBoyInventoryEffects(
                    nullptr,
                    pTileFade2,
                    nullptr,
                    BarterMenu::pCurrItem,
                    false
                );
            }
            else {
                InterfaceManager::UpdatePipBoyInventoryEffects(
                    pTiles[17],
                    pTileFade2,
                    nullptr,
                    BarterMenu::pCurrItem,
                    false
                );
            }
        }
    }
}

int32_t BarterMenu::GetClass() {
    return 1053;
}

bool BarterMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bResult = false;
    if (Interface::IsTopMenuID(Interface::eBarterMenu)) {
        switch (aiButton) {
            case 3:
            case 4: {
                if (plbCurrListbox == &lbPlayerItems) {
                    lbPlayerItems.SaveListState();
                    DoClick(2 * (aiButton != 4) + 2, nullptr);
                    lbPlayerItems.RestoreListState(false);
                }
                else {
                    lbVendorItems.SaveListState();
                    DoClick(2 * (aiButton != 4) + 7, nullptr);
                    lbVendorItems.RestoreListState(false);
                }
                bResult = true;
                break;
            }
            case 13: {
                if (!lbPlayerItems.IsEnabled() && lbPlayerItems.NumItems()) {
                    lbVendorItems.SaveListState();
                    DoEnter(0, nullptr);
                    lbPlayerItems.RestoreListState(true);
                    bResult = true;
                }

                break;
            }
            case 14: {
                if (!lbVendorItems.IsEnabled() && lbVendorItems.NumItems()) {
                    lbPlayerItems.SaveListState();
                    DoEnter(1, nullptr);
                    lbVendorItems.RestoreListState(true);
                    bResult = true;
                }

                break;
            }
            default: {
                break;
            }
        }
    }
    return bResult;
}

void BarterMenu::UpdateLists(TESBoundObject* apUpdateObj) {
    if (plbCurrListbox && plbCurrListbox->GetHighlightedItem()) {
        plbCurrListbox->SaveListState();
    }

    if (apUpdateObj) {
        BSSimpleList<ItemChange*> kNewItemGroup;
        InventoryChanges* pPlayerInventory = InventoryChanges::GetInventoryChanges(pPlayer);
        InventoryChanges* pVendorInventory = pDataHandler->GetBarterContainer();
        if (pPlayerInventory) {
            pPlayerInventory->GetItemGroup(apUpdateObj, &kNewItemGroup);
            lbPlayerItems.RemoveItemGroup(apUpdateObj);
            for (ItemChange*& rpItem : kNewItemGroup.ItemsUntilNull()) {
                if (BarterMenu::PlayerFirstPassFilterFunc(rpItem)) {
                    ItemChange* pItem = rpItem;
                    if (pItem) {
                        delete pItem;
                    }
                }
                else {
                    const char* pFullName = rpItem->GetFullName();
                    lbPlayerItems.AddItem(rpItem, pFullName, nullptr, nullptr);
                }
            }
            kNewItemGroup.RemoveAll();
        }
        if (pVendorInventory) {
            pVendorInventory->GetItemGroup(apUpdateObj, &kNewItemGroup);
            lbVendorItems.RemoveItemGroup(apUpdateObj);
            for (ItemChange*& rpItem : kNewItemGroup.ItemsUntilNull()) {
                if (BarterMenu::VendorFirstPassFilterFunc(rpItem)) {
                    ItemChange* pItem = rpItem;
                    if (pItem) {
                        delete pItem;
                    }
                }
                else {
                    const char* pFullName = rpItem->GetFullName();
                    lbVendorItems.AddItem(rpItem, pFullName, nullptr, nullptr);
                }
            }
            kNewItemGroup.RemoveAll();
        }
    }
    else {
        pDataHandler->CreateBarterContainer(pVendorRef);
        lbPlayerItems
            .BuildFromContainer(pPlayer, &BarterMenu::PlayerFirstPassFilterFunc, nullptr, nullptr, false, false);
        lbVendorItems
            .BuildFromContainer(pVendorRef, &BarterMenu::VendorFirstPassFilterFunc, nullptr, nullptr, false, true);
    }
    for (ItemChange*& rpItem : ItemsToBuy.ItemsUntilNull()) {
        if (!apUpdateObj || rpItem->pContainerObj == apUpdateObj) {
            ItemChange* pNewItem = new ItemChange(rpItem->pContainerObj, rpItem->iNumber);
            ExtraDataList* pNewExtra = rpItem->pExtraObjectList ? rpItem->pExtraObjectList->m_item : nullptr;
            Tile* pNewTile = nullptr;
            if (pNewExtra) {
                pNewItem->pExtraObjectList->AddHead(pNewExtra);
            }
            pNewTile = lbPlayerItems.AddItem(pNewItem, pNewItem->GetFullName(), nullptr, nullptr);
            pNewTile->Set(BarterMenu::iUT_IsBarterSelected, 1);
        }
    }
    for (ItemChange*& rpItem : ItemsToSell.ItemsUntilNull()) {
        if (!apUpdateObj || rpItem->pContainerObj == apUpdateObj) {
            ItemChange* pNewItem = new ItemChange(rpItem->pContainerObj, rpItem->iNumber);
            ExtraDataList* pNewExtra = rpItem->pExtraObjectList ? rpItem->pExtraObjectList->m_item : nullptr;
            Tile* pNewTile = nullptr;
            if (pNewExtra) {
                pNewItem->pExtraObjectList->AddHead(pNewExtra);
            }
            pNewTile = lbVendorItems.AddItem(pNewItem, pNewItem->GetFullName(), nullptr, nullptr);
            pNewTile->Set(BarterMenu::iUT_IsBarterSelected, 1);
        }
    }
    lbPlayerItems.Traverse(&BarterMenu::RemoveStolenItems, 0, std::numeric_limits<int32_t>::max());
    lbPlayerItems.Traverse(&BarterMenu::InitFunc, 0, std::numeric_limits<int32_t>::max());
    lbPlayerItems.Filter(&BarterMenu::FilterModeFunc);
    // Sort is called through each list's vtable; a call on the member itself would be devirtualised, so it goes through
    // a pointer.
    ListBox<ItemChange*>* plbSorted = &lbPlayerItems;
    plbSorted->Sort(&BarterMenu::SortFunc);
    lbVendorItems.Traverse(&BarterMenu::InitFunc, 0, std::numeric_limits<int32_t>::max());
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
                Tile* pFirstItem = plbCurrListbox->GetFirstItem();
                InterfaceManager::Instance()->SetCurrentFocusTarget(pFirstItem, 4035, true);
            }
        }
    }
    UpdateCapsLabels();
}

void BarterMenu::UpdateCapsLabels() {
    char acBuffer[68];
    iPlayerCaps = pPlayer->GetGoldAmount();
    iVendorCaps = pDataHandler->GetBarterContainer()->GetGoldAmount();
    BSsprintf(acBuffer, 64u, "%s   %d", sInventoryCaps.String(), iPlayerCaps);
    pTiles[5]->Set(4036, acBuffer, true);
    BSsprintf(acBuffer, 64u, "%s   %d", sInventoryCaps.String(), iVendorCaps);
    pTiles[10]->Set(4036, acBuffer, true);
}

bool BarterMenu::AreAllTilesNonNull() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

ItemChange* BarterMenu::GetTransactItem(BSSimpleList<ItemChange*>* apItemList, ItemChange* apCompareItem) {
    ItemChange* pFoundItem = nullptr;
    while (!pFoundItem && apItemList && apItemList->m_item) {
        ItemChange* pcurrItem = apItemList->m_item;
        if (apCompareItem->pContainerObj == pcurrItem->pContainerObj) {
            ExtraDataList* pCompareExtra = apCompareItem->pExtraObjectList ? apCompareItem->pExtraObjectList->m_item
                                                                           : nullptr;
            ExtraDataList* pCurrExtra = pcurrItem->pExtraObjectList ? pcurrItem->pExtraObjectList->m_item : nullptr;
            if (pCompareExtra == pCurrExtra) {
                pFoundItem = pcurrItem;
            }
        }
        apItemList = apItemList->m_pkNext;
    }
    return pFoundItem;
}

void BarterMenu::AddTransactItem(int32_t aiAmountSelected) {
    BSSimpleList<ItemChange*>* pTransactList = plbCurrListbox == &lbPlayerItems ? &ItemsToSell : &ItemsToBuy;
    ItemChange* pTransactItem = GetTransactItem(pTransactList, BarterMenu::pCurrItem);
    if (pTransactItem) {
        pTransactItem->iNumber = pTransactItem->iNumber + aiAmountSelected;
    }
    else {
        ExtraDataList* pExtra;
        if (BarterMenu::pCurrItem && BarterMenu::pCurrItem->pExtraObjectList) {
            pExtra = BarterMenu::pCurrItem->pExtraObjectList->m_item;
        }
        else {
            pExtra = nullptr;
        }
        pTransactItem = new ItemChange(BarterMenu::pCurrItem->pContainerObj, aiAmountSelected);
        if (pExtra) {
            pTransactItem->pExtraObjectList->AddHead(pExtra);
        }
        pTransactList->AddTail(pTransactItem);
    }
}

void BarterMenu::RemoveTransactItem(int32_t aiAmountSelected) {
    BSSimpleList<ItemChange*>* pTransactList = plbCurrListbox == &lbPlayerItems ? &ItemsToBuy : &ItemsToSell;
    ItemChange* pTransactItem = GetTransactItem(pTransactList, BarterMenu::pCurrItem);
    // Tests the sign of the wrapped difference, not iNumber > aiAmountSelected.
    if (static_cast<int32_t>(static_cast<uint32_t>(pTransactItem->iNumber) - static_cast<uint32_t>(aiAmountSelected)) >
        0) {
        pTransactItem->iNumber = pTransactItem->iNumber - aiAmountSelected;
    }
    else {
        pTransactList->Remove(pTransactItem);
        if (pTransactItem) {
            delete pTransactItem;
        }
    }
}

void BarterMenu::CompleteTransaction() {
    TESObjectREFR* pVendorContainer;
    if (pVendorRef->m_Extra.GetMerchantContainer()) {
        pVendorContainer = pVendorRef->m_Extra.GetMerchantContainer();
    }
    else {
        pVendorContainer = pVendorRef;
    }
    ItemChange* pAmmo = pPlayer->pCurrentProcess->GetCurrentAmmo();
    for (ItemChange* pBuyItem : ItemsToBuy.ItemsUntilNull()) {
        ExtraDataList* pBuyExtra = pBuyItem->pExtraObjectList ? pBuyItem->pExtraObjectList->m_item : nullptr;
        if (pBuyExtra) {
            Script::SetActionFlag(pVendorRef, pBuyExtra, SCRIPT_ACTION_ONSELL);
        }
        pDataHandler->RemoveItemBarterContainer(pBuyItem, pBuyItem->iNumber, pVendorRef);
        if (pBuyItem) {
            delete pBuyItem;
        }
    }
    for (ItemChange* pSellItem : ItemsToSell.ItemsUntilNull()) {
        ExtraDataList* pSellExtra = pSellItem->pExtraObjectList ? pSellItem->pExtraObjectList->m_item : nullptr;
        if (pSellExtra) {
            Script::SetActionFlag(pPlayer, pSellExtra, SCRIPT_ACTION_ONSELL);
        }
        pPlayer->RemoveItem(
            pSellItem->pContainerObj,
            pSellExtra,
            pSellItem->iNumber,
            false,
            false,
            pVendorContainer,
            nullptr,
            nullptr,
            true,
            false
        );
        if (pSellItem) {
            delete pSellItem;
        }
    }
    BarterMenu::pMe->pTiles[13]->Set(4015, 0);
    BarterMenu::pMe->pTiles[12]->Set(4003, 0);
    InventoryChanges* pInventory = nullptr;
    int32_t iWholeAmount = FloatToInt(std::floorf(BarterMenu::pMe->fTransactionAmount));
    MiscStatManager::ModVal(MiscStatManager::MS_BARTER_AMOUNT_TRADED, std::abs(iWholeAmount));
    if (fTransactionAmount > 0.0) {
        TESBoundObject* pCapsForm = static_cast<TESBoundObject*>(TESForm::GetFormByNumericID(15u));
        ItemChange kTempGold(pCapsForm, 0);
        pInventory = InventoryChanges::GetInventoryChanges(pVendorContainer);
        if (pInventory) {
            TESObjectREFR* pVendor = pVendorRef;
            int32_t iAmount = BSMIN(iWholeAmount, iVendorCaps);
            pDataHandler->RemoveItemBarterContainer(&kTempGold, iAmount, pVendor);
        }
        BSAudio::QInstance().GetSoundHandleByName("ITMBottlecapsUp", 257u).Play(false);
    }
    else if (fTransactionAmount < 0.0) {
        pInventory = InventoryChanges::GetInventoryChanges(pPlayer);
        if (pInventory) {
            pInventory->RemoveGold(pPlayer, -iWholeAmount, pVendorContainer);
        }
        BSAudio::QInstance().GetSoundHandleByName("ITMBottlecapsDown", 257u).Play(false);
    }
    fTransactionAmount = 0.0f;
    ItemsToBuy.RemoveAll();
    ItemsToSell.RemoveAll();
    bItemsTraded = true;
    if (pAmmo) {
        TESBoundObject* pAmmoObject = pAmmo->pContainerObj;
        pInventory = InventoryChanges::GetInventoryChanges(pPlayer);
        if (!pInventory->GetObjectCount(pAmmoObject)) {
            pPlayer->pCurrentProcess->SetCurrentAmmo(nullptr);
        }
    }
}

void BarterMenu::SetFilterModeTitle(Tile* apTitleTile) {
    char acBuffer[268];
    uint32_t uiFilterMode = apTitleTile == pTiles[3] ? ePlayerFilterMode : eVendorFilterMode;
    bool bNotEnglish = BSstrcmp(sLanguage.String(), "ENGLISH") != 0;
    switch (uiFilterMode) {
        case 0u: {
            if (apTitleTile == pTiles[3]) {
                BSstrcpy(acBuffer, 260, sInventoryItems.String());
                if (!bNotEnglish) {
                    BSstrupr(acBuffer, 260);
                }
            }
            else {
                BSstrcpy(acBuffer, 260, pVendorRef->GetFullName());
            }
            break;
        }
        case 1u: {
            BSstrcpy(acBuffer, 260, sInventoryWeapons.String());
            if (!bNotEnglish) {
                BSstrupr(acBuffer, 260);
            }
            break;
        }
        case 2u: {
            BSstrcpy(acBuffer, 260, sInventoryApparel.String());
            if (!bNotEnglish) {
                BSstrupr(acBuffer, 260);
            }
            break;
        }
        case 3u: {
            BSstrcpy(acBuffer, 260, sInventoryAid.String());
            if (!bNotEnglish) {
                BSstrupr(acBuffer, 260);
            }
            break;
        }
        case 4u: {
            BSstrcpy(acBuffer, 260, sInventoryMisc.String());
            if (!bNotEnglish) {
                BSstrupr(acBuffer, 260);
            }
            break;
        }
        case 5u: {
            BSstrcpy(acBuffer, 260, sInventoryAmmo.String());

            if (!bNotEnglish) {
                BSstrupr(acBuffer, 260);
            }
            break;
        }
        default: {
            break;
        }
    }
    apTitleTile->Set(4036, acBuffer, true);
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
    InterfaceManager* pIM = InterfaceManager::Instance();
    if (apVendor) {
        BarterMenu* pBarterMenu;
        Tile* pOldMenuTile = Menu::GetMenuByClass(BarterMenu::GetClassStatic());
        if (pOldMenuTile) {
            delete pOldMenuTile;
        }
        Tile* pTile = pIM->GetMenusRoot()->ReadFile("Data\\Menus\\barter_menu.xml");
        if (pTile) {
            pBarterMenu = static_cast<BarterMenu*>(pTile->GetMenu());
        }
        else {
            pBarterMenu = nullptr;
        }
        BarterMenu::pMe = pBarterMenu;
        if (pBarterMenu && BarterMenu::pMe->GetClass() == BarterMenu::GetClassStatic()) {
            BarterMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pTile), false);
            if (BarterMenu::pMe->AreAllTilesNonNull()) {
                if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
                    float fMaximumDepth = Tile::GetMaximumDepth();
                    pTile->Set(4013, fMaximumDepth, true);
                }
                DialogMenu::ServiceStart();
                pIM->xTutorialManager.ShowMessage(Interface::TUT_BARTER, BarterMenu::GetClassStatic(), 512u);
                BarterMenu::pMe->lbPlayerItems.Initialize(BarterMenu::pMe->pTiles[6], "CM_list_template");
                BarterMenu::pMe->lbVendorItems.Initialize(BarterMenu::pMe->pTiles[11], "CM_list_template");
                const char* pItems = sInventoryItems.String();
                BarterMenu::pMe->pTiles[3]->Set(4036, pItems, true);
                const char* pCaps = sInventoryCaps.String();
                Tile* pCapsLabel = pTile->GetChildByName("BM_CapsLabel");
                pCapsLabel->Set(4036, pCaps, true);
                const char* pAccept = sAccept.String();
                BarterMenu::pMe->pTiles[13]->Set(4036, pAccept, true);
                const char* pExit = sExit.String();
                BarterMenu::pMe->pTiles[19]->Set(4036, pExit, true);
                int32_t iTitleTrait = Tile::TextToTrait("_Title");
                Tile* pWeightInfo = pTile->GetChildByName("WeightInfo");
                if (pWeightInfo) {
                    pWeightInfo->Set(iTitleTrait, sInventoryWeightUpper.String(), true);
                }
                Tile* pValueInfo = pTile->GetChildByName("ValueInfo");
                if (pValueInfo) {
                    pValueInfo->Set(iTitleTrait, sInventoryValue.String(), true);
                }
                if (BarterMenu::pMe->pTiles[14]) {
                    const char* pDamage = sInventoryDamage.String();
                    BarterMenu::pMe->pTiles[14]->Set(iTitleTrait, pDamage, true);
                }
                if (BarterMenu::pMe->pTiles[15]) {
                    const char* pDamagePerSecond = sInventoryDamagePerSecond.String();
                    BarterMenu::pMe->pTiles[15]->Set(iTitleTrait, pDamagePerSecond, true);
                }
                if (BarterMenu::pMe->pTiles[16]) {
                    const char* pStrReq = sInventoryStrReq.String();
                    BarterMenu::pMe->pTiles[16]->Set(iTitleTrait, pStrReq, true);
                }
                if (BarterMenu::pMe->pTiles[17]) {
                    const char* pDamageResistance = sInventoryDamageResistance.String();
                    BarterMenu::pMe->pTiles[17]->Set(iTitleTrait, pDamageResistance, true);
                }
                if (BarterMenu::pMe->pTiles[18]) {
                    const char* pDamageThreshold = sInventoryDamageThreshold.String();
                    BarterMenu::pMe->pTiles[18]->Set(iTitleTrait, pDamageThreshold, true);
                }
                BarterMenu::pMe->pVendorRef = apVendor;
                BarterMenu::pMe->iDiscount = BSMIN(BSMAX(aiDiscount, -100), 100);
                BarterMenu::pMe->fTransactionAmount = 0.0f;
                float* pSellMult = &BarterMenu::pMe->fSellMult;
                float* pBuyMult = &BarterMenu::pMe->fBuyMult;
                int32_t iBarter = pPlayer->GetClampedActorValue(ActorValue::BARTER);
                GamePlayFormulas::GetBarterMultipliers(iBarter, pBuyMult, pSellMult);
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                BarterMenu::pMe->PrepForVisibility(false);
                BarterMenu::pMe->UpdateLists(nullptr);
                return pTile;
            }

            Warning("MENUS: Barter Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }

        if (BarterMenu::pMe && BarterMenu::pMe->pMenu && BarterMenu::pMe) {
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
    if (BarterMenu::pMe) {
        if (pPlayer->MobileObject::GetCurrentAIProcess()->Get3DUpdateFlag(RESET_MODEL)) {
            pPlayer->UpdatePlayer3d();
        }
        TESTopic* pTopic = nullptr;

        if (BarterMenu::pMe->bItemsTraded) {
            pTopic = TESTopic::GetTopic(DT_SERVICE, 4);
        }
        DialogMenu::ServiceEnd(pTopic);
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
    bool bIsFiltered = false;
    TESBoundObject* pObj = apItem ? apItem->pContainerObj : nullptr;
    TESBipedModelForm* pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pObj);
    TESFullName* pFull = dynamic_cast<TESFullName*>(pObj);
    TESObjectWEAP* pWeap = nullptr;
    if (apItem->pContainerObj->GetFormType() == WEAP_ID) {
        pWeap = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
    }
    TESAmmo* pAmmo = nullptr;
    if (apItem->pContainerObj->GetFormType() == AMMO_ID) {
        pAmmo = static_cast<TESAmmo*>(apItem->pContainerObj);
    }
    if (!pObj ||
        (pBipedForm && !pBipedForm->GetPlayable()) ||
        pObj->GetQuestObject() ||
        TESContainer::IsGold(pObj) ||
        !pFull ||
        !pFull->GetFullNameLength() ||
        (pWeap && !pWeap->GetIsPlayable()) ||
        (pAmmo && !pAmmo->GetIsPlayable()) ||
        pObj->GetFormType() == KEYM_ID) {
        return true;
    }
    return bIsFiltered;
}

bool BarterMenu::VendorFirstPassFilterFunc(ItemChange* apItem) {
    bool bIsFiltered = false;
    Actor* pActor = nullptr;
    if (BarterMenu::pMe->pVendorRef && BarterMenu::pMe->pVendorRef->IsActor()) {
        pActor = static_cast<Actor*>(BarterMenu::pMe->pVendorRef);
    }
    TESBoundObject* pObj = apItem ? apItem->pContainerObj : nullptr;
    if (pActor) {
        switch (apItem->pContainerObj->GetFormType()) {
            case ARMO_ID: {
                bIsFiltered = !pActor->GetActorBase()->GetOffersService(2u);
                break;
            }
            case BOOK_ID: {
                bIsFiltered = !pActor->GetActorBase()->GetOffersService(8u);
                break;
            }
            case MISC_ID:
            case COBJ_ID:
            case IMOD_ID: {
                bIsFiltered = !pActor->GetActorBase()->GetOffersService(1024u);
                break;
            }
            case WEAP_ID:
            case AMMO_ID: {
                bIsFiltered = !pActor->GetActorBase()->GetOffersService(1u);
                break;
            }
            case KEYM_ID: {
                bIsFiltered = true;
                break;
            }
            case ALCH_ID: {
                switch (BGSEquipType::GetEquipType(apItem->pContainerObj)) {
                    case BGSEquipType::ET_CHEMS: {
                        bIsFiltered = !pActor->GetActorBase()->GetOffersService(32u);
                        break;
                    }
                    case BGSEquipType::ET_STIMPACK: {
                        bIsFiltered = !pActor->GetActorBase()->GetOffersService(64u);
                        break;
                    }
                    case BGSEquipType::ET_FOOD: {
                        bIsFiltered = !pActor->GetActorBase()->GetOffersService(16u);
                        break;
                    }
                    case BGSEquipType::ET_ALCOHOL: {
                        bIsFiltered = !pActor->GetActorBase()->GetOffersService(4u);
                        break;
                    }
                    default: {
                        break;
                    }
                }
                break;
            }
            case NOTE_ID: {
                bIsFiltered = !pObj ||
                    TESValueForm::GetFormValue(pObj) <= 0 ||
                    !pActor->GetActorBase()->GetOffersService(1024u);
                break;
            }
            default: {
                break;
            }
        }
    }
    TESBipedModelForm* pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pObj);
    TESFullName* pFull = dynamic_cast<TESFullName*>(pObj);
    TESObjectWEAP* pWeap = nullptr;
    if (apItem->pContainerObj->GetFormType() == WEAP_ID) {
        pWeap = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
    }
    TESAmmo* pAmmo = nullptr;
    if (apItem->pContainerObj->GetFormType() == AMMO_ID) {
        pAmmo = static_cast<TESAmmo*>(apItem->pContainerObj);
    }
    if (!pObj ||
        (pBipedForm && !pBipedForm->GetPlayable()) ||
        TESContainer::IsGold(pObj) ||
        !pFull ||
        !pFull->GetFullNameLength() ||
        (pWeap && !pWeap->GetIsPlayable()) ||
        (pAmmo && !pAmmo->GetIsPlayable())) {
        return true;
    }
    return bIsFiltered;
}

void BarterMenu::RemoveStolenItems(Tile* apTile, ItemChange* apItem) {
    Actor* pActor = nullptr;
    if (BarterMenu::pMe->pVendorRef && BarterMenu::pMe->pVendorRef->IsActor()) {
        pActor = static_cast<Actor*>(BarterMenu::pMe->pVendorRef);
    }
    BSSimpleList<ExtraDataList*> removeList;
    for (ExtraDataList* pExtra : apItem->pExtraObjectList->ItemsUntilNull()) {
        if (pActor && pExtra->GetOwner()) {
            TESObjectREFR ownedItem;
            ownedItem.SetOwner(pExtra->GetOwner());
            ownedItem.SetGlobal(pExtra->GetGlobal());
            ownedItem.SetRank(pExtra->GetRank());

            ownedItem.SetObjectReference(apItem->pContainerObj);
            if (ownedItem.IsAnOwner(pActor, true)) {
                removeList.AddHead(pExtra);
            }
        }
    }
    for (ExtraDataList*& rpItem : removeList.ItemsUntilNull()) {
        uint32_t uiNumber = apItem->iNumber;
        apItem->iNumber = uiNumber - rpItem->GetCount();
        apItem->pExtraObjectList->Remove(rpItem);
    }
    removeList.RemoveAll();
}

float BarterMenu::GetSortValue(Tile* apTile, ItemChange* apItem) {
    float fSortValue = apItem->GetItemValue();
    float fOrigValue = fSortValue;
    bool bInVendorList = BarterMenu::pMe->lbVendorItems.IsListItem(apTile);
    bool bIsBarterSelected = apTile->IsTrue(BarterMenu::iUT_IsBarterSelected);
    float fReturnVal = 0.0;
    if (!(fSortValue < 0.0)) {
        if ((!bInVendorList || bIsBarterSelected) && (bInVendorList || !bIsBarterSelected)) {
            if ((bInVendorList && bIsBarterSelected) || (!bInVendorList && !bIsBarterSelected)) {
                float fSellMult;
                if (apItem->pContainerObj->GetFormType() == CHIP_ID ||
                    apItem->pContainerObj->GetFormType() == CMNY_ID) {
                    fSellMult = 1.0f;
                }
                else {
                    fSellMult = BarterMenu::pMe->fSellMult;
                }
                fSortValue = fSortValue * fSellMult;
                if (BarterMenu::pMe->iDiscount <= 0) {
                    if (BarterMenu::pMe->iDiscount < 0) {
                        fSortValue = (1.0 - static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0) *
                            fSortValue;
                    }
                }
                else {
                    fSortValue = (static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0 + 1.0) * fSortValue;
                }
                if (!(std::fabsf(fSortValue) > 1.0)) {
                    fSortValue = BSRoundToNearest(fSortValue, 0.1f);
                }
                else {
                    fSortValue = BSRoundToNearest(fSortValue, 1.0f);
                }
                return BSMIN(fSortValue, fOrigValue);
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
            fSortValue = fSortValue * fBuyMult;
            if (BarterMenu::pMe->iDiscount <= 0) {
                if (BarterMenu::pMe->iDiscount < 0) {
                    fSortValue = (static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0 + 1.0) * fSortValue;
                }
            }
            else {
                fSortValue = (1.0 - static_cast<double>(std::abs(BarterMenu::pMe->iDiscount)) / 100.0) * fSortValue;
            }
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::CALCULATE_BUY_PRICE,
                pPlayer,
                apItem->pContainerObj,
                &fSortValue
            );
            if (!(std::fabsf(fSortValue) > 1.0)) {
                fSortValue = BSRoundToNearest(fSortValue, 0.1f);
            }
            else {
                fSortValue = BSRoundToNearest(fSortValue, 1.0f);
            }
            return BSMAX(fSortValue, fOrigValue);
        }
    }
    else {
        return -1.0f;
    }
    return fReturnVal;
}

int32_t BarterMenu::SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2) {
    bool bSelected1 = apItem1->pTile->IsTrue(BarterMenu::iUT_IsBarterSelected);
    bool bSelected2 = apItem2->pTile->IsTrue(BarterMenu::iUT_IsBarterSelected);
    if (bSelected1 && !bSelected2) {
        return -1;
    }
    if (!bSelected1 && bSelected2) {
        return 1;
    }
    const char* pName2 = apItem2->pTile->GetString(4036);
    const char* pName1 = apItem1->pTile->GetString(4036);
    return _tcscmp(pName1, pName2);
}

void BarterMenu::InitFunc(Tile* apTile, ItemChange* apItem) {
    int32_t iItemCount;
    char acBuffer[132];
    float fSortValue = BarterMenu::GetSortValue(apTile, apItem);
    if (apTile->IsTrue(BarterMenu::iUT_IsBarterSelected)) {
        iItemCount = apItem->iNumber;
    }
    else {
        ItemChange* pTransactItem = BarterMenu::pMe->GetTransactItem(&BarterMenu::pMe->ItemsToBuy, apItem);
        if (!pTransactItem) {
            pTransactItem = BarterMenu::pMe->GetTransactItem(&BarterMenu::pMe->ItemsToSell, apItem);
        }
        uint32_t uiTransactCount = pTransactItem ? pTransactItem->iNumber : 0;
        iItemCount = apItem->iNumber - uiTransactCount;
        uint32_t uiNumBartered = pTransactItem ? pTransactItem->iNumber : 0;
        apTile->Set(Tile::TextToTrait("_NumBartered"), static_cast<int32_t>(uiNumBartered));
    }
    if (iItemCount > 1) {
        BSsprintf(acBuffer, 128, "%s (%d)", apItem->GetFullName(), iItemCount);
        apTile->Set(4036, acBuffer, true);
    }
    if (apItem->GetWorn(false) && !apTile->IsTrue(BarterMenu::iUT_IsBarterSelected)) {
        apTile->Set(Tile::AddUserTrait("_IsEquipped", -1), 1);
    }
    if (!(fSortValue <= 0.0)) {
        if (!(fSortValue < 1.0)) {
            BSsprintf(acBuffer, 128, "%.0f", fSortValue);
        }
        else {
            BSsprintf(acBuffer, 128, "%.1f", fSortValue);
        }
    }
    else {
        BSstrcpy(acBuffer, 128, "--");
    }
    apTile->Set(g_BarterMenu_Trait_Value, acBuffer, true);
    apTile->SetName(apItem->GetFullName());
    apTile->Set(4016, 0.0f, true);
}

void BarterMenu::ItemSelectCallback(int32_t aiAmountSelected) {
    Tile* pCurrTile = BarterMenu::pMe->plbCurrListbox->GetTileFor(BarterMenu::pCurrItem);
    if (aiAmountSelected && BarterMenu::pCurrItem && pCurrTile) {
        BarterMenu::pMe->fTransactionAmount = BarterMenu::GetSortValue(pCurrTile, BarterMenu::pCurrItem) *
                static_cast<double>(2 * (BarterMenu::pMe->plbCurrListbox == &BarterMenu::pMe->lbPlayerItems) - 1) *
                static_cast<double>(aiAmountSelected) +
            BarterMenu::pMe->fTransactionAmount;
        bool bToPlayer = BarterMenu::pMe->plbCurrListbox == &BarterMenu::pMe->lbVendorItems;
        pPlayer->PlayPickUpSound(BarterMenu::pCurrItem->pContainerObj, bToPlayer, false);
        if (pCurrTile->IsTrue(BarterMenu::iUT_IsBarterSelected)) {
            BarterMenu::pMe->RemoveTransactItem(aiAmountSelected);
        }
        else {
            BarterMenu::pMe->AddTransactItem(aiAmountSelected);
        }
        BarterMenu::pMe->UpdateLists(BarterMenu::pCurrItem->pContainerObj);
        if (BarterMenu::pMe->ItemsToBuy.IsEmpty() && BarterMenu::pMe->ItemsToSell.IsEmpty()) {
            BarterMenu::pMe->pTiles[13]->Set(4015, 0);
            BarterMenu::pMe->pTiles[12]->Set(4003, 0);
        }
        else {
            char acBuffer[16];
            int32_t iWholeAmount = FloatToInt(std::floorf(BarterMenu::pMe->fTransactionAmount));
            int32_t iArrowDirection = BarterMenu::pMe->fTransactionAmount > 0.0 ? 2 : 1;
            BarterMenu::pMe->pMenu->Set(g_BarterMenu_Trait_ArrowDirection, iArrowDirection);
            int32_t iShownAmount = BSMIN(iWholeAmount, BarterMenu::pMe->iVendorCaps);
            BSsprintf(acBuffer, 16, "%d", BSAbs(iShownAmount));
            BarterMenu::pMe->pTiles[12]->Set(g_BarterMenu_Trait_Value, acBuffer, true);
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
    BarterMenu::FILTER_MODE eMode;
    bool bIsFiltered = false;
    ENUM_FORM_ID eID = apItem->pContainerObj->GetFormType();
    Tile* pCurrTile = BarterMenu::pMe->lbPlayerItems.GetTileFor(apItem);
    if (pCurrTile) {
        eMode = BarterMenu::pMe->ePlayerFilterMode;
    }
    else {
        pCurrTile = BarterMenu::pMe->lbVendorItems.GetTileFor(apItem);
        eMode = BarterMenu::pMe->eVendorFilterMode;
    }
    switch (eMode) {
        case BarterMenu::FM_WEAPONS: {
            bIsFiltered = eID != WEAP_ID;
            break;
        }
        case BarterMenu::FM_APPAREL: {
            bIsFiltered = eID != ARMO_ID && eID != CLOT_ID;
            break;
        }
        case BarterMenu::FM_AID: {
            bIsFiltered = eID != INGR_ID && eID != ALCH_ID && eID != BOOK_ID;
            break;
        }
        case BarterMenu::FM_MISC: {
            bIsFiltered = eID == WEAP_ID ||
                eID == ARMO_ID ||
                eID == CLOT_ID ||
                eID == INGR_ID ||
                eID == ALCH_ID ||
                eID == AMMO_ID ||
                eID == BOOK_ID ||
                eID == CHIP_ID ||
                eID == CCRD_ID ||
                eID == CMNY_ID;
            break;
        }
        case BarterMenu::FM_AMMO: {
            bIsFiltered = eID != AMMO_ID;
            break;
        }
        default: {
            break;
        }
    }
    if (pCurrTile) {
        if (!bIsFiltered) {
            double dNumber = static_cast<double>(apItem->iNumber);
            if (!(dNumber - pCurrTile->GetFloat(Tile::TextToTrait("_NumBartered")) <= 0.0)) {
                return false;
            }
        }
        return true;
    }
    return bIsFiltered;
}
