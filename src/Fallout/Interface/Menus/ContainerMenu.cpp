#include "Fallout/Interface/Menus/ContainerMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSAmmoForm.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/BarterMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/InventoryMenu.h"
#include "Fallout/Interface/Menus/QuantityMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/tchar.h"

#include <limits>

ContainerMenu::ContainerMenu()
  : Menu(), iValueStolen(0), bPickpocketFail(false), bPickPocketTallied(false), eItemsFilterMode(FM_ALL),
    eContainerFilterMode(FM_ALL), eFilterSound(MENU_SOUND_NEXT_PREVIOUS), plbCurrListbox(nullptr),
    m_hCurrentReactivityBark(0u) {
    ContainerMenu::pCurrItem = nullptr;
}

ContainerMenu::~ContainerMenu() {
    ContainerMenu::pMe = nullptr;
}

void ContainerMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 19) {
        pTiles[aiID] = apTile;
    }
}

void ContainerMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 19) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 0:
        case 1:
        case 2: {
            Interface::PlayMenuSound(eFilterSound);
            do {
                eItemsFilterMode = static_cast<ContainerMenu::FILTER_MODE>(eItemsFilterMode + 2 * (aiID != 0) - 1);
                if (eItemsFilterMode >= ContainerMenu::FM_ALL) {
                    if (eItemsFilterMode > ContainerMenu::FM_AMMO) {
                        eItemsFilterMode = ContainerMenu::FM_ALL;
                    }
                }
                else {
                    eItemsFilterMode = ContainerMenu::FM_AMMO;
                }
                SetFilterModeTitle(pTiles[1]);
                lbItems.Filter(&ContainerMenu::FilterModeFunc);
            }
            while (!lbItems.NumUnfiltered() && eItemsFilterMode);
            break;
        }
        case 5:
        case 6:
        case 7: {
            Interface::PlayMenuSound(eFilterSound);
            do {
                eContainerFilterMode = static_cast<ContainerMenu::FILTER_MODE>(
                    eContainerFilterMode + 2 * (aiID != 5) - 1
                );
                if (eContainerFilterMode >= ContainerMenu::FM_ALL) {
                    if (eContainerFilterMode > ContainerMenu::FM_AMMO) {
                        eContainerFilterMode = ContainerMenu::FM_ALL;
                    }
                }
                else {
                    eContainerFilterMode = ContainerMenu::FM_AMMO;
                }
                SetFilterModeTitle(pTiles[6]);
                lbContainer.Filter(&ContainerMenu::FilterModeFunc);
            }
            while (!lbContainer.NumUnfiltered() && eContainerFilterMode);
            break;
        }
        case 10: {
            lbContainer.Traverse(&ContainerMenu::TakeAllFn, 0, std::numeric_limits<int32_t>::max());
            BSAudio::QInstance().GetSoundHandleByName("UIItemTakeAll", 289u).Play(false);
            ContainerMenu::Close();
            break;
        }
        case 11: {
            ContainerMenu::Close();
            break;
        }
        case 20: {
            if (ContainerMenu::pCurrItem) {
                bool bhardcore;
                bMouseOverEnabled = 0;
                bool btakeAll = (TESContainer::IsGold(ContainerMenu::pCurrItem->pContainerObj) ||
                                 (bhardcore = pPlayer->IsHardcore(),
                                  TESWeightForm::GetFormWeight(ContainerMenu::pCurrItem->pContainerObj, bhardcore) <=
                                      0.0)) &&
                    (eContainerMode == Interface::CMM_STANDARD || eContainerMode == Interface::CMM_PICKPOCKET) &&
                    plbCurrListbox == &lbContainer;
                int32_t icount = ContainerMenu::pCurrItem->iNumber;
                if (icount <= iInventoryAskQuantityAt.Int() ||
                    IsPlacingMine(ContainerMenu::pCurrItem, pPlayer, pContainerRef) ||
                    btakeAll) {
                    if (btakeAll) {
                        ContainerMenu::ItemSelectCallback(ContainerMenu::pCurrItem->iNumber);
                    }
                    else {
                        ContainerMenu::ItemSelectCallback(1);
                    }
                }
                else {
                    plbCurrListbox->SaveListState();
                    plbCurrListbox->Highlight(nullptr);
                    QuantityMenu::Create(
                        ContainerMenu::pCurrItem->iNumber,
                        &ContainerMenu::ItemSelectCallback,
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

void ContainerMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 19) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 20: {
            if (apTarget) {
                bMouseOverEnabled = 1;
                if (plbCurrListbox == &lbItems || apTarget->pParent != pTiles[4]) {
                    if (plbCurrListbox != &lbContainer && apTarget->pParent == pTiles[8]) {
                        DoEnter(22, nullptr);
                    }
                }
                else {
                    DoEnter(21, nullptr);
                }
                ContainerMenu::pCurrItem = plbCurrListbox->GetHighlightedData();
                if (ContainerMenu::pCurrItem) {
                    InventoryMenu::SetupItemDataTile(ContainerMenu::pCurrItem, pTiles[13]);
                    pTiles[13]->Set(4003, 1);
                    const char* pcIconOne = ContainerMenu::pCurrItem->GetIconOne(nullptr);
                    pTiles[12]->Set(4044, pcIconOne, true);
                    pTiles[12]->Set(4003, 1);
                    pTiles[9]->Set(Tile::kTileValue_User0, (plbCurrListbox != &lbItems) + 1);
                    pTiles[9]->Set(4003, 1);
                }
            }
            break;
        }
        case 21: {
            DoLeave(20, nullptr);
            // Highlight is called virtually through ListBoxBase.
            static_cast<ListBoxBase*>(&lbContainer)->Highlight(nullptr);
            plbCurrListbox = &lbItems;
            break;
        }
        case 22: {
            DoLeave(20, nullptr);
            static_cast<ListBoxBase*>(&lbItems)->Highlight(nullptr);
            plbCurrListbox = &lbContainer;
            break;
        }
    }
}

void ContainerMenu::DoIdle() {
    if (m_nTimeToClearSubtitle && BSGetTickCount() > m_nTimeToClearSubtitle) {
        m_nTimeToClearSubtitle = 0;
        Tile* pTile = Tile::GetTileByName(ContainerMenu::pMe->pMenu, "CM_Subtitle");
        if (pTile) {
            pTile->Set(4036, "", true);
        }
    }

    if (lbContainer.NumItems() && bMouseOverEnabled) {
        if (ContainerMenu::pCurrItem && ContainerMenu::pCurrItem->pContainerObj->GetFormType() == WEAP_ID) {
            InterfaceManager::UpdatePipBoyInventoryEffects(
                pTiles[TI_DAM],
                pTiles[TI_DPS],
                pTiles[TI_STRREQ],
                ContainerMenu::pCurrItem,
                false
            );
        }
        else if (ContainerMenu::pCurrItem && ContainerMenu::pCurrItem->pContainerObj->GetFormType() == ARMO_ID) {
            Tile* pDTTile;
            if (!ContainerMenu::pCurrItem ||
                static_cast<TESObjectARMO*>(ContainerMenu::pCurrItem->pContainerObj)
                        ->TESObjectARMO::GetDamageThreshold() == 0.0) {
                pDTTile = nullptr;
            }
            else {
                pDTTile = pTiles[TI_DT];
            }
            if (!ContainerMenu::pCurrItem ||
                static_cast<TESObjectARMO*>(ContainerMenu::pCurrItem->pContainerObj)->TESObjectARMO::GetArmorRating() ==
                    0.0) {
                InterfaceManager::UpdatePipBoyInventoryEffects(
                    nullptr,
                    pDTTile,
                    nullptr,
                    ContainerMenu::pCurrItem,
                    false
                );
            }
            else {
                InterfaceManager::UpdatePipBoyInventoryEffects(
                    pTiles[TI_DR],
                    pDTTile,
                    nullptr,
                    ContainerMenu::pCurrItem,
                    false
                );
            }
        }
    }
}

bool ContainerMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bresult = false;
    if (Interface::IsTopMenuID(Interface::eContainerMenu)) {
        switch (aiButton) {
            case 3:
            case 4: {
                if (eContainerMode != 4) {
                    if (plbCurrListbox == &lbItems) {
                        lbItems.SaveListState();
                        DoClick(aiButton != 4 ? 2 : 0, nullptr);
                        lbItems.RestoreListState(false);
                    }
                    else {
                        lbContainer.SaveListState();
                        DoClick(2 * (aiButton != 4) + 5, nullptr);
                        lbContainer.RestoreListState(false);
                    }
                    bresult = true;
                }
                break;
            }
            case 13: {
                if (!lbItems.GetHighlightedItem() && lbItems.NumItems()) {
                    lbContainer.SaveListState();
                    DoEnter(21, nullptr);
                    lbItems.RestoreListState(true);
                    bresult = true;
                }

                break;
            }
            case 14: {
                if (!lbContainer.GetHighlightedItem() && lbContainer.NumItems()) {
                    lbItems.SaveListState();
                    DoEnter(22, nullptr);
                    lbContainer.RestoreListState(true);
                    bresult = true;
                }

                break;
            }
            default: {
                break;
            }
        }
    }
    return bresult;
}

bool ContainerMenu::DoKey(int32_t aiKey) {
    if (aiKey == 32) {
        ContainerMenu::Close();
        return true;
    }
    return false;
}

void ContainerMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    pTiles[TI_ITEM_INFO]->Tile::Set(4003, 0);
    pTiles[TI_ITEM_ICON]->Tile::Set(4003, 0);
    pTiles[TI_ARROW_ICON]->Tile::Set(4003, 0);
}

bool ContainerMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (aiButton != 11 || eContainerMode == 4 || eContainerMode == 3 || xFadeState != 1) {
        if (aiButton == 10) {
            ContainerMenu::Close();
            return true;
        }
    }
    else {
        DoClick(10, nullptr);
        return true;
    }
    return false;
}

void ContainerMenu::DoWheelMove(int32_t aiID, Tile* apTarget) {
    if (aiID == 1 || aiID == 6) {
        if (InterfaceManager::Instance()->GetMouseWheel() >= 0) {
            DoClick(aiID != 1 ? 7 : 2, nullptr);
        }
        else {
            DoClick(aiID != 1 ? 5 : 0, nullptr);
        }
    }
}

void ContainerMenu::UpdateLists(TESBoundObject* apUpdateObj) {
    float fMaxWeight;
    float fCurrWeight;
    char cWeightText[68];
    if (plbCurrListbox && plbCurrListbox->GetHighlightedItem()) {
        plbCurrListbox->SaveListState();
    }

    if (eContainerMode == Interface::CMM_ROCK_N_LOAD) {
        BSSimpleArray<ItemChange*, 1024>* prockItLauncherItems = pPlayer->GetRockItAmmoArray();
        // RemoveAll and Sort are called through pointers so that they dispatch virtually; calls on the members
        // themselves would be devirtualised.
        ListBox<ItemChange*>* pContainerList = &lbContainer;
        pContainerList->RemoveAll();
        for (ItemChange* pItem : *prockItLauncherItems) {
            const char* pcFullName = pItem->GetFullName();
            lbContainer.AddItem(pItem, pcFullName, nullptr, nullptr);
        }
        if (apUpdateObj) {
            InventoryChanges* pplayerInventory = InventoryChanges::GetInventoryChanges(pPlayer);
            UpdateItemInList(apUpdateObj, pplayerInventory, &lbItems);
        }
        else {
            lbItems.BuildFromContainer(pPlayer, &ContainerMenu::FirstPassFilterFunc, nullptr, nullptr, false, false);
        }
    }
    else if (apUpdateObj) {
        TESAmmo* pammoObj;
        InventoryChanges* pplayerInventory = InventoryChanges::GetInventoryChanges(pPlayer);
        InventoryChanges* pcontainerInventory = pContainerRef ? InventoryChanges::GetInventoryChanges(pContainerRef)
                                                              : nullptr;
        if (apUpdateObj->GetFormType() == WEAP_ID) {
            if (pContainerRef->IsActor()) {
                pammoObj = static_cast<TESObjectWEAP*>(apUpdateObj)
                               ->TESObjectWEAP::GetCurrentAmmo(static_cast<Actor*>(pContainerRef));
            }
            else {
                pammoObj = static_cast<TESObjectWEAP*>(apUpdateObj)->TESObjectWEAP::GetCurrentAmmo(nullptr);
            }
        }
        else {
            pammoObj = nullptr;
        }
        UpdateItemInList(apUpdateObj, pplayerInventory, &lbItems);
        if (pammoObj) {
            UpdateItemInList(pammoObj, pplayerInventory, &lbItems);
        }
        if (eContainerMode == Interface::CMM_TEAMMATE) {
            lbContainer
                .BuildFromContainer(pContainerRef, &ContainerMenu::FirstPassFilterFunc, nullptr, nullptr, false, false);
        }
        else {
            UpdateItemInList(apUpdateObj, pcontainerInventory, &lbContainer);
            if (pammoObj) {
                UpdateItemInList(pammoObj, pcontainerInventory, &lbContainer);
            }
        }
    }
    else {
        lbItems.BuildFromContainer(pPlayer, &ContainerMenu::FirstPassFilterFunc, nullptr, nullptr, false, false);
        lbContainer
            .BuildFromContainer(pContainerRef, &ContainerMenu::FirstPassFilterFunc, nullptr, nullptr, false, false);
    }
    for (TESBoundObject*& rpItem : ActiveMineList.ItemsUntilNull()) {
        if (!apUpdateObj || apUpdateObj == rpItem) {
            ItemChange* pnewItem = new ItemChange(rpItem, 1);
            BSStringT<char> strtemp;
            const char* pcFullName = pnewItem->GetFullName();
            strtemp.SPrintF("%s %s", sActiveMineDescription.String(), pcFullName);
            Tile* pmineTile = lbContainer.AddItem(pnewItem, strtemp.pString, nullptr, nullptr);
            pmineTile->xChildren.GetTail()->Set(4009, 128.0f, true);
        }
    }
    if (pContainerRef && pContainerRef->IsActor() && pContainerRef->IsDead(false)) {
        TESObjectREFR* pref = pContainerRef->m_Extra.GetDroppedWeapon();
        if (pref && !pref->GetDisabledOrDeleted() && (!apUpdateObj || apUpdateObj == pref->GetObjectReference())) {
            ItemChange* pnewItem;
            pnewItem = new ItemChange(pref->GetObjectReference(), 1);
            const char* pcFullName = pnewItem->GetFullName();
            lbContainer.AddItem(pnewItem, pcFullName, nullptr, nullptr);
            if (pnewItem->pExtraObjectList) {
                pnewItem->pExtraObjectList = new BSSimpleList<ExtraDataList*>;
            }
            if (!pnewItem->pExtraObjectList->m_item) {
                ExtraDataList* pextra = new ExtraDataList;
                pnewItem->pExtraObjectList->AddHead(pextra);
            }
            pnewItem->pExtraObjectList->m_item->SetItemDropper(pref);
            float fhealth = pref->GetHealth();
            pnewItem->pExtraObjectList->m_item->SetHealth(fhealth);
        }
    }

    lbItems.Traverse(&ContainerMenu::InitFunc, 0, std::numeric_limits<int32_t>::max());
    lbItems.Filter(&ContainerMenu::FilterModeFunc);
    ListBox<ItemChange*>* pItemList = &lbItems;
    pItemList->Sort(&ContainerMenu::SortFunc);
    lbContainer.Traverse(&ContainerMenu::InitFunc, 0, std::numeric_limits<int32_t>::max());
    lbContainer.Filter(&ContainerMenu::FilterModeFunc);
    ListBox<ItemChange*>* pContainerList = &lbContainer;
    pContainerList->Sort(&ContainerMenu::SortFunc);
    pPlayer->GetInventoryEncumbrance(fCurrWeight, fMaxWeight);
    float fmaxWeightRounded = BSRoundToNearest(fMaxWeight, 1.0f);
    int32_t imaxWeight = FloatToInt(fmaxWeightRounded);
    float fcurrWeightRounded = BSRoundToNearest(fCurrWeight, 1.0f);
    int32_t icurrWeight = FloatToInt(fcurrWeightRounded);
    BSsprintf(cWeightText, 64, "%s   %d/%d", sInventoryWeight.String(), icurrWeight, imaxWeight);
    ContainerMenu::pMe->pTiles[3]->Set(4036, cWeightText, true);
    if (!plbCurrListbox && InterfaceManager::Has360Controller()) {
        plbCurrListbox = &lbContainer;
    }
    if (plbCurrListbox) {
        bool bHasItems = plbCurrListbox->NumUnfiltered() != 0;
        if (!bHasItems) {
            eFilterSound = MENU_SOUND_NONE;
            DoClick(&lbItems != plbCurrListbox ? 7 : 2, nullptr);
            eFilterSound = MENU_SOUND_NEXT_PREVIOUS;
            bHasItems = plbCurrListbox->NumUnfiltered() != 0;
        }
        if (bHasItems) {
            plbCurrListbox->RestoreListState(true);
        }
        else {
            DoEnter((plbCurrListbox == &lbItems) + 21, nullptr);
            if (InterfaceManager::Has360Controller()) {
                Tile* pfirstItem = plbCurrListbox->GetFirstItem();
                InterfaceManager::Instance()->SetCurrentFocusTarget(pfirstItem, 4035, true);
            }
        }
    }
}

int32_t ContainerMenu::GetStolenValue(ItemChange* apItem, int32_t aiCount, bool abFromContainer) {
    TESForm* powner;
    float fitemValue = apItem->GetItemValue();
    int32_t iitemValue = FloatToInt(fitemValue);
    int32_t ireturnVal = 0;
    if (pContainerRef->IsActor()) {
        powner = pContainerRef;
    }
    else {
        powner = pContainerRef->GetOwner();
    }
    for (BSSimpleList<ExtraDataList*>* pextraObj = apItem->pExtraObjectList;
         aiCount > 0 && pextraObj && pextraObj->m_item;
         pextraObj = pextraObj->m_pkNext) {
        TESForm* pextraOwner = pextraObj->m_item->GetOwner();
        if ((!pextraOwner && abFromContainer) || (pextraOwner && pextraObj->m_item->GetOwner() == powner)) {
            ireturnVal += iitemValue * pextraObj->m_item->GetCount();
        }
        aiCount -= pextraObj->m_item->GetCount();
    }
    if (abFromContainer) {
        ireturnVal += iitemValue * aiCount;
    }
    return ireturnVal;
}

void ContainerMenu::PlayReactivityBark(TESTopic* aRelatedTopic) {
    Actor* pactor = nullptr;
    TESTopicInfo* pInfo = nullptr;
    Actor* pActionActor = pPlayer;
    if (ContainerMenu::pMe->pContainerRef && ContainerMenu::pMe->pContainerRef->IsActor()) {
        pactor = dynamic_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
    }
    if (aRelatedTopic) {
        pInfo = aRelatedTopic->GetMatchingInfo(pactor, pActionActor);
    }
    if (pInfo) {
        DialogueItem Exchange(pInfo->pOwnerQuest, aRelatedTopic, pInfo, pactor);
        Exchange.FirstResponse();
        DialogueResponse* pResponse = Exchange.GetCurrentResponse();
        if (pResponse) {
            if (pResponse->pVoiceSound) {
                uint32_t isoundFileID = pResponse->pVoiceSound->iFormID;
                this->m_hCurrentReactivityBark = pactor->PlaySound(isoundFileID, false, 258u, true);
            }
            else {
                PlayerCharacter* plistener = pPlayer;
                TESIdleForm* plistenIdle = pResponse->pListenIdle;
                TESIdleForm* pspeakerIdle = pResponse->pSpeakerIdle;
                uint32_t uiLength = pResponse->cText.GetLength();
                uint32_t uiEmotion = pResponse->uiEmotion;
                DIALOGUE_EMOTION eEmotion = pResponse->eEmotion;
                char* pcVoice = pResponse->GetVoice()->pString;
                pactor->SpeakSoundFunction(
                    pcVoice,
                    this->m_hCurrentReactivityBark,
                    eEmotion,
                    uiEmotion,
                    uiLength,
                    pspeakerIdle,
                    plistenIdle,
                    plistener,
                    true,
                    true,
                    false,
                    false,
                    true
                );
            }
            this->DisplayContainerMenuSubtitle(pResponse->cText, this->m_hCurrentReactivityBark);
        }
    }
}

// The duration is a volatile float, so that the sound's integer duration is rounded to a float before the sum.
void ContainerMenu::DisplayContainerMenuSubtitle(BSStringT<char> asText, BSSoundHandle ahSound) {
    Actor* pactor = nullptr;
    if (ContainerMenu::pMe->pContainerRef && ContainerMenu::pMe->pContainerRef->IsActor()) {
        pactor = dynamic_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
    }
    if (asText.GetLength() &&
        (gs_bGeneralSubtitles_Gameplay.Bool() || (pactor && static_cast<uint8_t>(pactor->GetShowSubtitles())))) {
        Tile* pTile = Tile::GetTileByName(ContainerMenu::pMe->pMenu, "CM_Subtitle");
        if (pTile) {
            pTile->Set(Tile::kTileValue_String, asText.pString, true);
        }
        float fSubtitleDuration = static_cast<float>(ahSound.GetDuration());
        if (fSubtitleDuration == 0.0) {
            double dlength = static_cast<double>(std::strlen(asText.pString));
            fSubtitleDuration = fNoticeTextTimePerCharacter.Float() * dlength * 1000.0;
        }
        m_nTimeToClearSubtitle = static_cast<int64_t>(static_cast<double>(BSGetTickCount()) + fSubtitleDuration);
    }
}

bool ContainerMenu::PickpocketCheck(Actor* apActor, int32_t aiStealValue) {
    bool bResult = true;
    if (!apActor || aiStealValue <= 0 || apActor->IsKnockedOut()) {
        return bResult;
    }
    int32_t iTargetSneak = static_cast<ActorValueOwner*>(apActor)->GetActorValue(ActorValue::SNEAK);
    int32_t iPlayerSneak = static_cast<ActorValueOwner*>(pPlayer)->GetClampedActorValue(ActorValue::SNEAK);
    uint32_t uiChance = AiFormulas::ComputePickpocketSuccess(
        iPlayerSneak,
        iTargetSneak,
        static_cast<float>(aiStealValue)
    );
    pPlayer->UseSkill(ActorValue::SNEAK, 1);
    if (BSRandom::GetSingleton()->UnsignedInt(100) < uiChance) {
        if (plbCurrListbox == &lbContainer && !bPickpocketFail) {
            MiscStatManager::Increment(MiscStatManager::MS_POCKETS_PICKED);
            bPickpocketFail = true;
        }
    }
    else {
        pPlayer->PickpocketAlarm(apActor, nullptr, 0);
        Interface::ShowMessage(sPickpocketFail.String(), 2, nullptr, nullptr, 2.0f, false);
        bResult = false;
        bPickPocketTallied = true;
    }
    // GetActorBase is called twice and the first result is dropped.
    apActor->GetActorBase();
    ActorValueOwner* pBaseValues = apActor->GetActorBase();
    TESActorBaseData::ALIGNMENT eAlignment = TESActorBaseData::GetAlignmentForKarma(
        pBaseValues->GetActorFloatValue(ActorValue::KARMA)
    );
    if (eAlignment == TESActorBaseData::ALIGNMENT_EVIL || eAlignment == TESActorBaseData::ALIGNMENT_VERY_EVIL) {
        return bResult;
    }
    pPlayer->RewardKarma(static_cast<int32_t>(fKarmaModStealing.Float()));
    return bResult;
}

bool ContainerMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

void ContainerMenu::PlayContainerSound(TESObjectREFR* apContainer, bool abOpenSound) {
    TESSound* psound = nullptr;
    if (apContainer) {
        int32_t iformType = apContainer->GetObjectReference()->GetFormType();
        if (iformType == 27) {
            if (abOpenSound) {
                psound = static_cast<TESObjectCONT*>(apContainer->GetObjectReference())->TESObjectCONT::GetOpenSound();
            }
            else {
                psound = static_cast<TESObjectCONT*>(apContainer->GetObjectReference())->TESObjectCONT::GetCloseSound();
            }
        }
        else if (iformType > 41 && iformType <= 43) {
            if (abOpenSound) {
                BSAudio::QInstance().GetSoundHandleByName("DRSBodyGenericOpen", 289u).Play(false);
            }
            else {
                BSAudio::QInstance().GetSoundHandleByName("DRSBodyGenericClose", 289u).Play(false);
            }
        }
        if (psound) {
            const char* pcFilename = psound->GetFilename();
            BSSoundHandle kSound = BSAudio::QInstance().GetSoundHandleByFilename(pcFilename, 289u, psound);
            kSound.Play(false);
        }
    }
}

void ContainerMenu::InitContainerMode() {
    switch (eContainerMode) {
        case Interface::CMM_STANDARD:
        case Interface::CMM_PICKPOCKET: {
            if (pContainerRef) {
                const char* pcFullName = pContainerRef->GetFullName();
                SetContainerTitle(pcFullName);
                PlayContainerSound(pContainerRef, true);
            }
            break;
        }
        case Interface::CMM_TEAMMATE: {
            if (pContainerRef) {
                const char* pcFullName = pContainerRef->GetFullName();
                SetContainerTitle(pcFullName);
                BSAudio::QInstance().GetSoundHandleByName("DRSTraderOpen", 289u).Play(false);
                DialogMenu::ServiceStart();
                TESForm* pForm = TESForm::GetFormByEditorID("FollowersTrade");
                TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
                ContainerMenu::pMe->PlayReactivityBark(pTopic);
            }
            pTiles[TI_TAKE_ALL_BTN]->Set(4003, 0);
            break;
        }
        case Interface::CMM_ROCK_N_LOAD: {
            const char* pcJunk = sJunk.String();
            pTiles[TI_ITEMS_TITLE]->Set(4036, pcJunk, true);
            if (pPlayer->GetCurrentWeapon()) {
                TESObjectWEAP* pcurrentWeapon = pPlayer->GetCurrentWeapon();
                SetContainerTitle(pcurrentWeapon->GetFullName());
            }
            pTiles[TI_TAKE_ALL_BTN]->Set(4003, 0);
            pTiles[TI_ITEMS_FILTER_LEFT]->Set(4003, 0);
            pTiles[TI_ITEMS_FILTER_RIGHT]->Set(4003, 0);
            pTiles[TI_CONTAINER_FILTER_LEFT]->Set(4003, 0);
            pTiles[TI_CONTAINER_FILTER_RIGHT]->Set(4003, 0);
            break;
        }
        default: {
            break;
        }
    }
}

void ContainerMenu::UpdateItemInList(
    TESBoundObject* apObj,
    InventoryChanges* apInventory,
    ListBox<ItemChange*>* apListbox
) {
    BSSimpleList<ItemChange*> pnewItemGroup;
    if (apInventory && apObj && apListbox) {
        apInventory->GetItemGroup(apObj, &pnewItemGroup);
        apListbox->RemoveItemGroup(apObj);
        for (ItemChange*& rpItem : pnewItemGroup.ItemsUntilNull()) {
            const char* pcFullName = rpItem->GetFullName();
            apListbox->AddItem(rpItem, pcFullName, nullptr, nullptr);
        }
        pnewItemGroup.RemoveAll();
    }
}

bool ContainerMenu::IsPlacingMine(ItemChange* apItem, TESObjectREFR* apRefFrom, TESObjectREFR* apRefTo) {
    if (eContainerMode != Interface::CMM_PICKPOCKET ||
        plbCurrListbox != &lbItems ||
        apRefFrom != pPlayer ||
        !pPlayer->IsSneaking() ||
        !apRefTo->IsActor() ||
        apRefTo->IsDead(false) ||
        apRefTo->IsChild(false) ||
        apItem->pContainerObj->GetFormType() != WEAP_ID ||
        !ActiveMineList.IsEmpty()) {
        return false;
    }
    Actor* ptoActor = nullptr;
    if (apRefTo->IsActor()) {
        ptoActor = static_cast<Actor*>(apRefTo);
        if (static_cast<Actor*>(apRefTo)->Actor::GetEssential()) {
            return false;
        }
        if (static_cast<Actor*>(apRefTo)->GetCurrentAIProcess()) {
            BaseProcess* pprocess = static_cast<Actor*>(apRefTo)->GetCurrentAIProcess();
            if (pprocess->GetPlantedExplosive()) {
                return false;
            }
        }
    }
    TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
    return pweapon->IsThrownWeapon() &&
        pweapon->GetProjectile(ptoActor) &&
        pweapon->GetProjectile(ptoActor)->GetMotionType() == BGSProjectile::MOTION_GRENADE;
}

void ContainerMenu::SwitchContainer(Tile* apTile, ItemChange* apItem, int32_t aiAmountSwitched) {
    if (apItem && apTile) {
        TESObjectREFR* prefFrom;
        TESObjectREFR* prefTo;
        if (plbCurrListbox == &lbItems) {
            prefTo = static_cast<PlayerCharacter*>(pContainerRef);
        }
        else {
            prefTo = pPlayer;
        }
        if (plbCurrListbox == &lbItems) {
            prefFrom = pPlayer;
        }
        else {
            prefFrom = pContainerRef;
        }
        bool bisPlacingMine = IsPlacingMine(apItem, prefFrom, prefTo);
        ExtraDataList* pextraFrom = apItem->pExtraObjectList ? apItem->pExtraObjectList->m_item : nullptr;
        if (bisPlacingMine) {
            TESObjectWEAP* pweapon = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
            BGSProjectile* pprojectile = static_cast<TESObjectWEAP*>(apItem->pContainerObj)
                                             ->TESObjectWEAP::GetProjectile(nullptr);
            BGSProjectile::InitLiveGrenade(pprojectile, static_cast<Actor*>(prefFrom), prefTo, pweapon);
            ActiveMineList.AddHead(apItem->pContainerObj);
            prefFrom->RemoveItem(
                apItem->pContainerObj,
                pextraFrom,
                aiAmountSwitched,
                false,
                false,
                nullptr,
                nullptr,
                nullptr,
                true,
                false
            );
        }
        else if (
            !pextraFrom ||
            !pextraFrom->GetItemDropper() ||
            pextraFrom->GetItemDropper()->GetDisabled() ||
            pextraFrom->GetItemDropper()->GetDelete()
        ) {
            if (eContainerMode == Interface::CMM_ROCK_N_LOAD) {
                int32_t iswitchCounter = aiAmountSwitched;
                BSSimpleList<ExtraDataList*>* pextraObj = apItem->pExtraObjectList;
                while (iswitchCounter > 0) {
                    int32_t iamountToSwitch;
                    ExtraDataList* pextra = pextraObj ? pextraObj->m_item : nullptr;
                    if (pextra) {
                        iamountToSwitch = pextra->GetCount();
                    }
                    else {
                        iamountToSwitch = iswitchCounter;
                    }
                    int32_t iamountSwitched = BSMIN(iamountToSwitch, iswitchCounter);
                    if (prefTo == pPlayer) {
                        ItemChange* premovedItem = pPlayer->RemoveRockItAmmo(
                            apItem->pContainerObj,
                            pextra,
                            iamountSwitched,
                            true
                        );
                        ExtraDataList* premovedExtra = premovedItem->pExtraObjectList
                            ? premovedItem->pExtraObjectList->m_item
                            : nullptr;
                        pPlayer
                            ->AddObjecttoContainer(premovedItem->pContainerObj, premovedExtra, premovedItem->iNumber);
                        if (premovedItem) {
                            delete premovedItem;
                        }
                        pextraObj = apItem->pExtraObjectList;
                    }
                    else if (prefFrom == pPlayer) {
                        pPlayer->AddRockItAmmo(apItem->pContainerObj, pextra, iamountSwitched);
                        prefFrom->RemoveItem(
                            apItem->pContainerObj,
                            pextra,
                            iamountSwitched,
                            false,
                            false,
                            nullptr,
                            nullptr,
                            nullptr,
                            true,
                            false
                        );
                        pextraObj = pextraObj ? pextraObj->m_pkNext : nullptr;
                    }
                    iswitchCounter -= iamountSwitched;
                }
            }
            else {
                prefFrom->RemoveItem(
                    apItem->pContainerObj,
                    pextraFrom,
                    aiAmountSwitched,
                    eContainerMode == Interface::CMM_PICKPOCKET,
                    false,
                    prefTo,
                    nullptr,
                    nullptr,
                    true,
                    false
                );
            }
        }
        else {
            TESObjectREFR* pitemDropper = pextraFrom->GetItemDropper();
            pPlayer->PickUpObject(pitemDropper, 1, false);
        }
    }
}

void ContainerMenu::SetFilterModeTitle(Tile* apTitleTile) {
    int32_t ifilterMode = apTitleTile == pTiles[TI_ITEMS_TITLE] ? eItemsFilterMode : eContainerFilterMode;
    const char* pcLanguage = sLanguage.String();
    bool bnotEnglish = BSstrcmp(pcLanguage, "ENGLISH") != 0;
    if (pContainerRef && apTitleTile) {
        char cTitle[268];
        switch (ifilterMode) {
            case 0: {
                if (apTitleTile == pTiles[TI_ITEMS_TITLE]) {
                    BSstrcpy(cTitle, 260, sInventoryItems.String());
                    if (!bnotEnglish) {
                        BSstrupr(cTitle, 260);
                    }
                    apTitleTile->Set(4036, cTitle, true);
                }
                else {
                    const char* pcFullName = pContainerRef->GetFullName();
                    SetContainerTitle(pcFullName);
                }
                break;
            }
            case 1: {
                BSstrcpy(cTitle, 260, sInventoryWeapons.String());
                if (!bnotEnglish) {
                    BSstrupr(cTitle, 260);
                }
                break;
            }
            case 2: {
                BSstrcpy(cTitle, 260, sInventoryApparel.String());
                if (!bnotEnglish) {
                    BSstrupr(cTitle, 260);
                }
                break;
            }
            case 3: {
                BSstrcpy(cTitle, 260, sInventoryAid.String());
                if (!bnotEnglish) {
                    BSstrupr(cTitle, 260);
                }
                break;
            }
            case 4: {
                BSstrcpy(cTitle, 260, sInventoryMisc.String());
                if (!bnotEnglish) {
                    BSstrupr(cTitle, 260);
                }
                break;
            }
            case 5: {
                BSstrcpy(cTitle, 260, sInventoryAmmo.String());
                if (!bnotEnglish) {
                    BSstrupr(cTitle, 260);
                }
                break;
            }
            default: {
                break;
            }
        }
        if (ifilterMode) {
            apTitleTile->Set(4036, cTitle, true);
        }
    }
}

void ContainerMenu::SetContainerTitle(const char* astrTitle) {
    if (astrTitle && *astrTitle) {
        int32_t ifontNum = static_cast<int32_t>(pTiles[TI_CONTAINER_TITLE]->GetFloat(4025));
        NiPoint3 stringDims = FontManager::Instance()->CalculateStringDimensions(
            astrTitle,
            ifontNum,
            std::numeric_limits<float>::max(),
            0
        );
        float fmaxWidth = pTiles[TI_CONTAINER_TITLE]->pParent->GetFloat(4017) - 80.0;
        if (pTiles[TI_CONTAINER_FILTER_LEFT]->IsTrue(4003)) {
            fmaxWidth = fmaxWidth - 35.0;
        }
        if (!(fmaxWidth < static_cast<double>(stringDims.x))) {
            pTiles[TI_CONTAINER_TITLE]->Set(4036, astrTitle, true);
        }
        else {
            char* newTitle = static_cast<char*>(ourMalloc(260u));
            FontLetter* plettersA = FontManager::Instance()->GetFont(ifontNum - 1)->pFontData->pFontLetters;
            int32_t icurrIndex = 0;
            ourMemset(newTitle, 0, 260u);
            float fnewWidth = (plettersA[46].fLeadingEdge + plettersA[46].fWidth + plettersA[46].fSpacing) * 3.0 + 0.0f;
            while (fmaxWidth >= static_cast<double>(fnewWidth) && icurrIndex < 260) {
                newTitle[icurrIndex] = astrTitle[icurrIndex];
                fnewWidth = plettersA[astrTitle[icurrIndex]].fLeadingEdge + plettersA[astrTitle[icurrIndex]].fWidth +
                    plettersA[astrTitle[icurrIndex]].fSpacing + fnewWidth;
                ++icurrIndex;
            }
            BSstrcat(newTitle, 260, "...");
            pTiles[TI_CONTAINER_TITLE]->Set(4036, newTitle, true);
            ourFree(newTitle);
        }
    }
}

TileMenu* ContainerMenu::Create(TESObjectREFR* apContainer, Interface::CONTAINER_MENU_MODE aeContainerMode) {
    ContainerMenu* pmenu;
    InterfaceManager* pim = InterfaceManager::Instance();
    InterfaceManager::SetContainerMenuQueued(false);
    if (!apContainer && aeContainerMode != Interface::CMM_ROCK_N_LOAD) {
        return nullptr;
    }
    uint32_t uiMenuID = ContainerMenu::GetClassStatic();
    Tile* pold_menu_tile = Menu::GetMenuByClass(uiMenuID);
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    Tile* ptile = pim->GetMenusRoot()->ReadFile("Data\\Menus\\container_menu.xml");
    if (ptile) {
        pmenu = static_cast<ContainerMenu*>(ptile->GetMenu());
    }
    else {
        pmenu = nullptr;
    }
    ContainerMenu::pMe = pmenu;
    if (pmenu) {
        int32_t iclass = ContainerMenu::pMe->GetClass();
        if (iclass == ContainerMenu::GetClassStatic()) {
            ContainerMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            if (ContainerMenu::pMe->IsWellFormedMenu()) {
                if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                    float fmaxDepth = Tile::GetMaximumDepth();
                    ptile->Set(4013, fmaxDepth, true);
                }
                pim->xTutorialManager.ShowMessage(Interface::TUT_CONTAINER, ContainerMenu::GetClassStatic(), 512u);
                ContainerMenu::pMe->lbItems.Initialize(ContainerMenu::pMe->pTiles[4], "CM_list_template");
                ContainerMenu::pMe->lbContainer.Initialize(ContainerMenu::pMe->pTiles[8], "CM_list_template");
                const char* pcItems = sInventoryItems.String();
                ContainerMenu::pMe->pTiles[1]->Set(4036, pcItems, true);
                const char* pcTakeAll = sTakeAll.String();
                ContainerMenu::pMe->pTiles[10]->Set(4036, pcTakeAll, true);
                const char* pcExit = sExit.String();
                ContainerMenu::pMe->pTiles[11]->Set(4036, pcExit, true);
                Tile* pTile = Tile::GetTileByName(ContainerMenu::pMe->pMenu, "CM_Subtitle");
                if (pTile) {
                    pTile->Set(4036, "", true);
                }
                ContainerMenu::pMe->pContainerRef = apContainer;
                ContainerMenu::pMe->eContainerMode = aeContainerMode;
                ContainerMenu::pMe->InitContainerMode();
                int32_t iTitleTrait = Tile::TextToTrait("_Title");
                if (ContainerMenu::pMe->pTiles[14]) {
                    const char* pcDamage = sInventoryDamage.String();
                    ContainerMenu::pMe->pTiles[14]->Set(iTitleTrait, pcDamage, true);
                }
                if (ContainerMenu::pMe->pTiles[15]) {
                    const char* pcDPS = sInventoryDamagePerSecond.String();
                    ContainerMenu::pMe->pTiles[15]->Set(iTitleTrait, pcDPS, true);
                }
                if (ContainerMenu::pMe->pTiles[16]) {
                    const char* pcStrReq = sInventoryStrReq.String();
                    ContainerMenu::pMe->pTiles[16]->Set(iTitleTrait, pcStrReq, true);
                }
                if (ContainerMenu::pMe->pTiles[17]) {
                    const char* pcDR = sInventoryDamageResistance.String();
                    ContainerMenu::pMe->pTiles[17]->Set(iTitleTrait, pcDR, true);
                }
                if (ContainerMenu::pMe->pTiles[18]) {
                    const char* pcDT = sInventoryDamageThreshold.String();
                    ContainerMenu::pMe->pTiles[18]->Set(iTitleTrait, pcDT, true);
                }
                ContainerMenu::pMe->PrepForVisibility(false);
                ContainerMenu::pMe->UpdateLists(nullptr);
                return static_cast<TileMenu*>(ptile);
            }

            Warning("MENUS: Container Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }
    }

    if (ContainerMenu::pMe && ContainerMenu::pMe->pMenu && ContainerMenu::pMe) {
        delete ContainerMenu::pMe;
    }

    return nullptr;
}

void ContainerMenu::Close() {
    if (ContainerMenu::pMe) {
        ContainerMenu::pMe->m_hCurrentReactivityBark.FadeOutAndRelease(100u);
        switch (ContainerMenu::pMe->eContainerMode) {
            case Interface::CMM_PICKPOCKET: {
                TESObjectREFR* pcontainerActor;
                if (ContainerMenu::pMe->pContainerRef->IsActor()) {
                    pcontainerActor = ContainerMenu::pMe->pContainerRef;
                }
                else {
                    pcontainerActor = nullptr;
                }
                if (!pcontainerActor && ContainerMenu::pMe->iValueStolen > 0) {
                    TESForm* powner = ContainerMenu::pMe->pContainerRef->GetOwner();
                    pPlayer->StealAlarm(
                        ContainerMenu::pMe->pContainerRef,
                        nullptr,
                        0,
                        ContainerMenu::pMe->iValueStolen,
                        powner
                    );
                }
                [[fallthrough]];
            }
            case Interface::CMM_STANDARD: {
                ContainerMenu::pMe->PlayContainerSound(ContainerMenu::pMe->pContainerRef, false);
                break;
            }
            case Interface::CMM_TEAMMATE: {
                bool bstartedWithWeapon = false;
                if (ContainerMenu::pMe->pContainerRef->IsActor() &&
                    ContainerMenu::pMe->pContainerRef->GetAnimation() &&
                    static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->GetCurrentAIProcess())

                // The container ref is an actor here (IsActor above).
                {
                    BaseProcess* pprocess = static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)
                                                ->GetCurrentAIProcess();
                    bstartedWithWeapon = pprocess->GetCurrentWeapon() != nullptr;
                }

                if (ContainerMenu::pMe->pContainerRef->IsCreature()) {
                    TESCreature* pcreature = static_cast<TESCreature*>(
                        ContainerMenu::pMe->pContainerRef->GetObjectReference()
                    );
                    pcreature->InitDefaultWorn(ContainerMenu::pMe->pContainerRef, true, true, true);
                }
                else {
                    TESNPC* pnpc = static_cast<TESNPC*>(ContainerMenu::pMe->pContainerRef->GetObjectReference());
                    BipedAnim* pbiped = ContainerMenu::pMe->pContainerRef->GetBiped();
                    pnpc->InitParts(ContainerMenu::pMe->pContainerRef, pbiped, true);
                }
                if (bstartedWithWeapon &&
                    static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->Actor::GetWantWeaponDrawn()) {
                    BaseProcess* pprocess = static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)
                                                ->GetCurrentAIProcess();
                    if (!pprocess->GetCurrentWeapon()) {
                        static_cast<Actor*>(ContainerMenu::pMe->pContainerRef)->Actor::SetWantWeaponDrawn(false);
                    }
                }
                BSAudio::QInstance().GetSoundHandleByName("DRSTraderClose", 289u).Play(false);
                DialogMenu::ServiceEnd(nullptr);
                break;
            }
            case Interface::CMM_ROCK_N_LOAD: {
                if (!pVATS.IsInVats()) {
                    pPlayer->UseActionPoints(ActionPoints::ACTION_RELOAD);
                }
                break;
            }
            default: {
                break;
            }
        }
        ContainerMenu::pMe->ActiveMineList.RemoveAll();
        ContainerMenu::pMe->pMenu->Set(6002, 1);
        ContainerMenu::pMe->StartFadeOut();
        if (pPlayer->pCurrentProcess->Get3DUpdateFlag(RESET_MODEL)) {
            pPlayer->UpdatePlayer3d();
        }
        HUDMainMenu::RefreshInfoForRef();
    }
}

uint32_t ContainerMenu::GetClassStatic() {
    return 0x3F0;
}

void ContainerMenu::ForceContainerUpdate() {
    if (ContainerMenu::pMe) {
        ContainerMenu::pMe->UpdateLists(nullptr);
    }
}

void ContainerMenu::TakeAllFn(Tile* apTile, ItemChange* apItem) {
    if (apItem && ContainerMenu::pMe && apTile && !ContainerMenu::pMe->bPickPocketTallied) {
        ExtraDataList* pextra = nullptr;
        if (apItem->pExtraObjectList) {
            pextra = apItem->pExtraObjectList->m_item;
        }
        if (!pextra ||
            !pextra->GetItemDropper() ||
            pextra->GetItemDropper()->GetDisabled() ||
            pextra->GetItemDropper()->GetDelete()) {
            if (apTile->xChildren.GetTail()->GetFloat(4009) != 128.0) {
                if (ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET) {
                    Actor* pcontainerActor = nullptr;
                    if (ContainerMenu::pMe->pContainerRef->IsActor()) {
                        pcontainerActor = static_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
                    }
                    int32_t iStealValue = ContainerMenu::pMe->GetStolenValue(apItem, apItem->iNumber, true);
                    ContainerMenu::pMe->iValueStolen += iStealValue;
                    ContainerMenu::pMe->bPickPocketTallied = !ContainerMenu::pMe->PickpocketCheck(
                        pcontainerActor,
                        iStealValue
                    );
                }
                ExtraDataList* premoveExtra = apItem->pExtraObjectList ? apItem->pExtraObjectList->m_item : nullptr;
                ContainerMenu::pMe->pContainerRef->RemoveItem(
                    apItem->pContainerObj,
                    premoveExtra,
                    apItem->iNumber,
                    ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET,
                    false,
                    pPlayer,
                    nullptr,
                    nullptr,
                    true,
                    false
                );
            }
        }
        else {
            TESObjectREFR* pitemDropper = pextra->GetItemDropper();
            pPlayer->PickUpObject(pitemDropper, 1, false);
        }
    }
}

bool ContainerMenu::FirstPassFilterFunc(ItemChange* apItem) {
    bool bisFiltered = false;
    if (ContainerMenu::pMe) {
        TESBoundObject* pobj = apItem ? apItem->pContainerObj : nullptr;
        if (ContainerMenu::pMe->eContainerMode == Interface::CMM_ROCK_N_LOAD) {
            TESObjectWEAP* pcurrentWeapon = pPlayer->GetCurrentWeapon();
            BGSListForm* pammoList = pcurrentWeapon->GetAmmoFormList();
            return pammoList->GetItemIndex(pobj) < 0;
        }

        TESBipedModelForm* pbipedForm = TESBipedModelForm::GetFormAsBipedModel(pobj);
        TESFullName* pfull = dynamic_cast<TESFullName*>(pobj);
        TESObjectWEAP* pweap = nullptr;
        if (apItem->pContainerObj->GetFormType() == WEAP_ID) {
            pweap = static_cast<TESObjectWEAP*>(apItem->pContainerObj);
        }
        TESAmmo* pammo = nullptr;
        if (apItem->pContainerObj->GetFormType() == AMMO_ID) {
            pammo = static_cast<TESAmmo*>(apItem->pContainerObj);
        }
        if (!pobj ||
            (pbipedForm && !pbipedForm->GetPlayable()) ||
            !pfull ||
            !pfull->GetFullNameLength() ||
            (pweap && !pweap->GetIsPlayable()) ||
            (pammo && !pammo->GetIsPlayable())) {
            return true;
        }
    }
    return bisFiltered;
}

int32_t ContainerMenu::SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2) {
    const char* pcText2 = apItem2->pTile->GetString(4036);
    const char* pcText1 = apItem1->pTile->GetString(4036);
    return _tcscmp(pcText1, pcText2);
}

void ContainerMenu::InitFunc(Tile* apTile, ItemChange* apItem) {
    if (apItem->GetWorn(false)) {
        apTile->xChildren.GetHead()->Set(4003, 1);
    }
    if (apItem->iNumber > 1) {
        char cText[132];
        uint32_t uiCount = apItem->iNumber;
        BSsprintf(cText, 128, "%s (%d)", apItem->GetFullName(), uiCount);
        apTile->Set(4036, cText, true);
    }
    const char* pcFullName = apItem->GetFullName();
    apTile->SetName(pcFullName);
    const char* pcText = apTile->GetString(4036);
    apTile->xChildren.GetTail()->Set(4036, pcText, true);
    apTile->Set(4016, 0.0f, true);
}

void ContainerMenu::ItemSelectCallback(int32_t aiAmountSelected) {
    TESObjectREFR* prefFrom;
    Tile* pcurrTile = ContainerMenu::pMe->plbCurrListbox->GetTileFor(ContainerMenu::pCurrItem);
    Actor* pactor = nullptr;
    if (ContainerMenu::pMe->pContainerRef && ContainerMenu::pMe->pContainerRef->IsActor()) {
        pactor = static_cast<Actor*>(ContainerMenu::pMe->pContainerRef);
    }
    bool bcancelTransfer = false;
    if (ContainerMenu::pMe->plbCurrListbox == &ContainerMenu::pMe->lbItems) {
        prefFrom = pPlayer;
    }
    else {
        prefFrom = ContainerMenu::pMe->pContainerRef;
    }
    PlayerCharacter* pfromPlayer = static_cast<PlayerCharacter*>(prefFrom);
    if (!aiAmountSelected || !ContainerMenu::pCurrItem || !pcurrTile) {
        bcancelTransfer = true;
    }
    if (pcurrTile->xChildren.GetTail()->GetFloat(4009) == 128.0) {
        bcancelTransfer = true;
    }
    if (ContainerMenu::pMe->eContainerMode == Interface::CMM_TEAMMATE &&
        ContainerMenu::pMe->plbCurrListbox == &ContainerMenu::pMe->lbItems &&
        pactor) {
        double dweightInContainer = pactor->GetWeightInContainer(false);
        bool bhardcore = pPlayer->IsHardcore();
        double dnewWeight = TESWeightForm::GetFormWeight(ContainerMenu::pCurrItem->pContainerObj, bhardcore) *
                static_cast<double>(aiAmountSelected) +
            dweightInContainer;
        if (pactor->GetMaxCarryWeight() <= dnewWeight) {
            char msg[268];
            const char* pcOverencumbered = sTeammateOverencumbered.String();
            BSsprintf(msg, 260, "%s %s", pactor->GetFullName(), pcOverencumbered);
            Interface::ShowMessage(msg, 2, nullptr, nullptr, 2.0f, false);
            bcancelTransfer = true;
            TESForm* pForm = TESForm::GetFormByEditorID("FollowersOverburdened");
            TESTopic* pTopic = dynamic_cast<TESTopic*>(pForm);
            ContainerMenu::pMe->PlayReactivityBark(pTopic);
        }
    }

    if (ContainerMenu::pMe && ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET) {
        TESBoundObject* pitemOwner;
        if ((pfromPlayer == pPlayer && pactor) ||
            (pfromPlayer != pPlayer &&
             (pitemOwner = static_cast<TESBoundObject*>(ContainerMenu::pCurrItem->GetItemOwnership()),
              pitemOwner != pPlayer->GetObjectReference()))) {
            bool bfromContainer = ContainerMenu::pMe->plbCurrListbox == &ContainerMenu::pMe->lbContainer;
            int32_t istealValue = (bfromContainer ? 1 : -1) *
                ContainerMenu::pMe->GetStolenValue(ContainerMenu::pCurrItem, aiAmountSelected, bfromContainer);
            ContainerMenu::pMe->iValueStolen += istealValue;
            if (!bfromContainer) {
                istealValue = 1;
            }
            if (pactor && !ContainerMenu::pMe->PickpocketCheck(pactor, istealValue)) {
                bcancelTransfer = true;
                ContainerMenu::Close();
            }
        }
    }

    if (!bcancelTransfer) {
        TESBoundObject* pcurrItemObj = ContainerMenu::pCurrItem->pContainerObj;
        BaseProcess* pprocess = pPlayer->pCurrentProcess;
        ItemChange* pammo = pprocess->GetCurrentAmmo();
        pPlayer->PlayPickUpSound(pcurrItemObj, true, false);
        ContainerMenu::pMe->SwitchContainer(pcurrTile, ContainerMenu::pCurrItem, aiAmountSelected);
        ContainerMenu::pMe->UpdateLists(pcurrItemObj);
        if (pammo) {
            TESBoundObject* pammoObj = pammo->pContainerObj;
            InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
            if (!pinventory->GetObjectCount(pammoObj)) {
                pPlayer->pCurrentProcess->SetCurrentAmmo(nullptr);
            }
        }
    }
}

bool ContainerMenu::FilterModeFunc(ItemChange* apItem) {
    ContainerMenu::FILTER_MODE emode;
    bool bfiltered = false;
    ENUM_FORM_ID eid = apItem->pContainerObj->GetFormType();
    char cfromItems = 1;
    if (ContainerMenu::pMe->lbItems.GetTileFor(apItem)) {
        emode = ContainerMenu::pMe->eItemsFilterMode;
    }
    else {
        ContainerMenu::pMe->lbContainer.GetTileFor(apItem);
        emode = ContainerMenu::pMe->eContainerFilterMode;
        cfromItems = 0;
    }
    switch (emode) {
        case ContainerMenu::FM_WEAPONS: {
            bfiltered = eid != WEAP_ID;
            break;
        }
        case ContainerMenu::FM_APPAREL: {
            bfiltered = eid != ARMO_ID && eid != CLOT_ID;
            break;
        }
        case ContainerMenu::FM_AID: {
            bfiltered = eid != INGR_ID && eid != ALCH_ID && eid != BOOK_ID;
            break;
        }
        case ContainerMenu::FM_MISC: {
            bfiltered = eid == WEAP_ID ||
                eid == ARMO_ID ||
                eid == CLOT_ID ||
                eid == INGR_ID ||
                eid == ALCH_ID ||
                eid == AMMO_ID ||
                eid == BOOK_ID ||
                eid == CHIP_ID ||
                eid == CCRD_ID ||
                eid == CMNY_ID;
            break;
        }
        case ContainerMenu::FM_AMMO: {
            bfiltered = eid != AMMO_ID;
            break;
        }
        default: {
            break;
        }
    }
    if (eid == AMMO_ID) {
        BGSListForm* pList = static_cast<BGSListForm*>(TESForm::GetFormByEditorID("RegeneratingAmmo"));
        TESAmmo* pammo = static_cast<TESAmmo*>(apItem->pContainerObj);
        if (pList && pList->GetItemIndex(pammo) >= 0) {
            bfiltered = true;
        }

        if (pammo && !pammo->GetIsPlayable()) {
            bfiltered = true;
        }
    }
    if (cfromItems) {
        if (!bfiltered && eid != KEYM_ID) {
            if (!apItem->pContainerObj->GetQuestObject()) {
                return false;
            }
        }
        return true;
    }

    if (ContainerMenu::pMe->eContainerMode == Interface::CMM_PICKPOCKET) {
        return bfiltered || apItem->GetWorn(false);
    }

    return bfiltered;
}
