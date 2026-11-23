#include "Fallout/Interface/Menus/InventoryMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/BGSListForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EnchantmentItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESEnchantableForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectIMOD.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Fallout/Interface/HotKeysWheel.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/ContainerMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/ItemModMenu.h"
#include "Fallout/Interface/Menus/QuantityMenu.h"
#include "Fallout/Interface/Menus/RepairMenu.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

#include <limits>

static SettingT<GameSettingCollection> sDropEquippedItemWarning(
    "sDropEquippedItemWarning",
    "You cannot drop an equipped item until you complete your current action."
);
// The armor weight class names, light, heavy and medium, as pointers to their settings.
using ArmorWeightSettingArray = SettingT<GameSettingCollection>* [3];
static ArmorWeightSettingArray pGSArmorWeight{&sArmorWeightLight, &sArmorWeightHeavy, &sArmorWeightMedium};

bool IsDrBagUsable() {
    for (ActorValue::Index eAV = ActorValue::PERCEPTION_CONDITION; eAV < ActorValue::BRAIN_CONDITION;
         eAV = static_cast<ActorValue::Index>(eAV + 1)) {
        int32_t iCondition = pPlayer->GetActorValue(eAV);
        if (iCondition < pPlayer->GetPermanentActorValue(eAV)) {
            return true;
        }
    }
    return false;
}

int32_t InventoryMenu_SortFunc(ListBoxItem<ItemChange*>* apItemA, ListBoxItem<ItemChange*>* apItemB) {
    if (!apItemA->data) {
        return 1;
    }
    if (!apItemB->data) {
        return -1;
    }
    const char* pNameB = apItemB->data->GetFullName();
    const char* pNameA = apItemA->data->GetFullName();
    if (BSstrcmp(pNameA, pNameB)) {
        pNameB = apItemB->data->GetFullName();
        pNameA = apItemA->data->GetFullName();
        return BSstrcmp(pNameA, pNameB);
    }
    else {
        int32_t iCompare;
        double dHealthA = apItemA->data->GetItemHealth(false);
        if (!(apItemB->data->GetItemHealth(false) < dHealthA)) {
            dHealthA = apItemA->data->GetItemHealth(false);
            iCompare = apItemB->data->GetItemHealth(false) > dHealthA;
        }
        else {
            iCompare = -1;
        }
        if (!iCompare) {
            if (apItemA->data->GetWorn(false)) {
                return -1;
            }

            return apItemB->data->GetWorn(false);
        }
        return iCompare;
    }
}

void InventoryMenu::InitFunc(Tile* apTile, ItemChange* apItem) {
    if (apItem) {
        if (apItem->iNumber > 1) {
            char acText[132];
            if (apItem->GetModSlots()) {
                uint32_t uiCount = apItem->iNumber;
                BSsprintf(acText, 128, "%s+ (%d)", apItem->GetFullName(), uiCount);
            }
            else {
                uint32_t uiCount = apItem->iNumber;
                BSsprintf(acText, 128, "%s (%d)", apItem->GetFullName(), uiCount);
            }
            apTile->Set(4036, acText, true);
        }
        apTile->Set(4010, 29);
        if (apItem->pContainerObj->GetFormType() == AMMO_ID ||
            apItem->pContainerObj->GetFormType() == ARMO_ID ||
            apItem->pContainerObj->GetFormType() == WEAP_ID) {
            if (pPlayer->CanEquipOrUseObject(apItem)) {
                apTile->xChildren.GetTail()->Set(4009, 255);
            }
            else {
                apTile->xChildren.GetTail()->Set(4009, 127);
            }
        }
        if (apItem->pContainerObj->GetFormType() == AMMO_ID && pPlayer->GetCurrentWeapon()) {
            TESAmmo* pAmmo = static_cast<TESAmmo*>(apItem->pContainerObj);
            PlayerCharacter* pPlayerRef = pPlayer;
            TESObjectWEAP* pWeapon = pPlayer->GetCurrentWeapon();
            bool bCurrentAmmo = pAmmo == pWeapon->GetCurrentAmmo(pPlayerRef);
            apTile->xChildren.Get(apTile->xChildren.m_pkHead)->Set(4003, static_cast<int32_t>(bCurrentAmmo));
        }
        else {
            int32_t iWorn = apItem->GetWorn(false);
            apTile->xChildren.Get(apTile->xChildren.m_pkHead)->Set(4003, iWorn);
        }
    }
}

InventoryMenu::InventoryMenu() : Menu(), iCurrTab(0) {
    InventoryMenu::uiValueTrait = Tile::TextToTrait("_Value");
    InventoryMenu::iUT_ItemType = Tile::TextToTrait("_ItemType");
    InventoryMenu::iUT_CurrentTab = Tile::TextToTrait("_CurrentTab");
    InventoryMenu::iUT_KeyringOpen = Tile::TextToTrait("_KeyringOpen");
    InventoryMenu::iUT_EquippableItem = Tile::TextToTrait("_EquippableItem");
    for (InventoryMenu::sPageState& rPageState : pPageStatesA) {
        rPageState.iHighlightIndex = -1;
        rPageState.iScrollIndex = 0;
    }
}

InventoryMenu::~InventoryMenu() {
    InventoryMenu::pMe = nullptr;
}

void InventoryMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    // ids are compared unsigned: negative ids are out of range too
    if (static_cast<uint32_t>(aiID) >= 0x17) {
        return;
    }
    pTiles[aiID] = apTile;
}

// The menu's buttons: the filter tabs (24-28), drop (7), repair (8), the keyring's close (10) and open (30) tiles, mod
// (19) and an item (29: equip it, hotkey it while the wheel shows, or load ammo into the current weapon). The clicked
// tile is stored into apTarget when apTarget is null and aiID is in [0, NUM_TILES); nothing reads it afterwards.
void InventoryMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < NUM_TILES) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
            if (iCurrTab != static_cast<uint32_t>(aiID - 24)) {
                if (GetMenuTile()->IsTrue(iUT_KeyringOpen)) {
                    GetMenuTile()->Set(iUT_KeyringOpen, 0);
                }
                ClearCurrentItem();
                SavePageState();
                iCurrTab = aiID - 24;
                pTiles[TI_TABLINE]->Set(iUT_CurrentTab, static_cast<int32_t>(iCurrTab));
                float fIntensity = fMiniBurstIntensity.Float();
                float fDuration = fMiniBurstDuration.Float();
                InterfaceManager::Instance()->GetPipboy()->TriggerBurst(fDuration, fIntensity, 0.5f);
                InterfaceManager::Instance()->UpdateInventoryPageNumber(static_cast<int8_t>(aiID));
                lbInventoryList.Filter(&InventoryMenu::FilterFunc);
                lbInventoryList.Traverse(&InventoryMenu::SetVisibleFunc, -1, std::numeric_limits<int32_t>::max());
                LoadPageState();
                SetKnobPosition(true);
            }
            if (aiID == 24) {
                pTiles[TI_MOD_BTN]->Set(Tile::kTileValue_Visible, 1);
            }
            else if (aiID == 28) {
                // The menu id is fetched before the interface manager.
                int32_t iMenuID = GetClassStatic();
                InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_AMMO, iMenuID, 500);
                pMe->lbInventoryList.Traverse(&InventoryMenu::InitFunc, 0, std::numeric_limits<int32_t>::max());
            }
            else {
                if (aiID == 25) {
                    int32_t iMenuID = GetClassStatic();
                    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_APPAREL, iMenuID, 512);
                }
                pTiles[TI_MOD_BTN]->Set(Tile::kTileValue_Visible, 0);
                pTiles[TI_ITEM_BADGE]->Set(Tile::kTileValue_Visible, 0);
                pTiles[TI_SKILLREQ]->Set(Tile::kTileValue_Visible, 0);
            }
            break;
        case 8:
            if (pCurrItem) {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                SavePageState();
                RepairMenu::InitRepairMenu(pCurrItem);
            }
            break;
        case 19:
            if (pCurrItem && pCurrItem->GetItemChangeObject()->GetFormType() == WEAP_ID) {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                SavePageState();
                if (pCurrItem->GetExtraDataList() && !pCurrItem->GetExtraDataList()->GetItem()) {
                    InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
                    // The count is read before the object.
                    int32_t iCount = pCurrItem->GetCount();
                    TESBoundObject* pObject = pCurrItem->GetItemChangeObject();
                    pChanges->CreateExtraDataList(pObject, iCount, pPlayer, nullptr, pCurrItem);
                }
                ItemModMenu::InitItemModMenu(pCurrItem, true);
            }
            break;
        case 29:
            if (!pCurrItem ||
                GetMenuTile()->IsTrue(iUT_KeyringOpen) ||
                pCurrItem->GetItemChangeObject()->GetFormType() == AMMO_ID) {
                // Reads pItem's object here even when pItem is null.
                if (pCurrItem->GetItemChangeObject()->GetFormType() == AMMO_ID &&
                    pCurrItem->GetItemChangeObject() &&
                    GetMenuTile()->IsTrue(iUT_EquippableItem)) {
                    ItemChange* pAmmo = new ItemChange(pCurrItem->GetItemChangeObject(), pCurrItem->GetCount());
                    BaseProcess* pProcess = pPlayer->GetCurrentAIProcess();
                    TESObjectWEAP* pQueuedWeapon = pPlayer->GetQueuedWeaponAttach();
                    TESObjectWEAP* pWeapon = pPlayer->GetCurrentWeapon();
                    pProcess->SetCurrentAmmo(pAmmo);
                    pProcess->GetCurrentAmmo()->SetCount(0);
                    pPlayer->PlayPickUpSound(pCurrItem->GetItemChangeObject(), true, false);
                    int32_t iReloadAnim;
                    if (!pQueuedWeapon || pQueuedWeapon->GetIsLoopingReload()) {
                        iReloadAnim = 2;
                    }
                    else {
                        iReloadAnim = 0;
                    }
                    bool bUseModValue = pProcess->GetCurrentWeapon()->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE);
                    pPlayer->ReloadWeaponNV(pWeapon, iReloadAnim, bUseModValue, false);
                    TESBoundObject* pChanged = pCurrItem->GetItemChangeObject();
                    ItemUpdateList.AddHead(pChanged);
                    UpdateList();
                }
            }
            else {
                uint8_t bEquipped = 0;
                if (HKWheel.GetVisible()) {
                    if (!InterfaceManager::Has360Controller()) {
                        if (pCurrItem && pCurrItem->GetItemHealth(true) == 0.0) {
                            Interface::ShowMessage(
                                sCantHotkeyBrokenItem.String(),
                                2,
                                nullptr,
                                "UIVATSInsufficientAP",
                                2.0f,
                                false
                            );
                        }
                        else if (
                            !GetMenuTile()->IsTrue(iUT_EquippableItem) ||
                            pCurrItem->GetItemChangeObject()->GetFormType() == BOOK_ID
                        ) {
                            Interface::ShowMessage(
                                sCantHotkeyItem.String(),
                                2,
                                nullptr,
                                "UIVATSInsufficientAP",
                                2.0f,
                                false
                            );
                        }
                        else {
                            HKWheel.SetHotkeyItem(pCurrItem);
                            TESBoundObject* pChanged = pCurrItem->GetItemChangeObject();
                            ItemUpdateList.AddHead(pChanged);
                            UpdateList();
                        }
                    }
                }
                else {
                    SelectCurrentItem();
                    bEquipped = 1;
                }
                if (bEquipped) {
                    pPlayer->UpdatePlayer3d();
                    pPlayer->GetCurrentAIProcess()->SetShouldRefreshMagicShadersEffects(true, false, false);
                    pPlayer->ResetArmorRating();
                }
            }
            break;
        case 30:
            if (HKWheel.GetVisible()) {
                Interface::ShowMessage(
                    sCantHotkeyItem.String(),
                    0,
                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                    "UIVATSInsufficientAP",
                    2.0f,
                    false
                );
            }
            else {
                SavePageState();
                GetMenuTile()->Set(iUT_KeyringOpen, 1);
                lbInventoryList.Filter(&InventoryMenu::KeyFilterFunc);
                lbInventoryList.Traverse(&InventoryMenu::SetVisibleFunc, -1, std::numeric_limits<int32_t>::max());
                ClearCurrentItem();
                LoadPageState();
            }
            break;
        case 10:
            SavePageState();
            GetMenuTile()->Set(iUT_KeyringOpen, 0);
            lbInventoryList.Filter(&InventoryMenu::FilterFunc);
            lbInventoryList.Traverse(&InventoryMenu::SetVisibleFunc, -1, std::numeric_limits<int32_t>::max());
            LoadPageState();
            break;
        case 7:
            if (pCurrItem) {
                NiPoint3 kDropPoint = NiPoint3::ZERO;
                TESBoundObject* pObject = pCurrItem->GetItemChangeObject();
                ExtraDataList* pExtra;
                if (pCurrItem->GetExtraDataList()) {
                    pExtra = pCurrItem->GetExtraDataList()->GetItem();
                }
                else {
                    pExtra = nullptr;
                }
                if (GetMenuTile()->IsTrue(iUT_KeyringOpen)) {
                    BSAudio::QInstance()
                        .GetSoundHandleByName(
                            "UIVATSInsufficientAP",
                            BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                                BSGameSound::SOUND_FLAG_ONE_SHOT
                        )
                        .Play(false);
                }
                else if (pObject->GetQuestObject()) {
                    Interface::ShowMessage(sDropQuestItemWarning.String(), 2, nullptr, nullptr, 2.0f, false);
                }
                else if (pPlayer->GetAnimAction() != -1 && pCurrItem->GetWorn(false)) {
                    Interface::ShowMessage(sDropEquippedItemWarning.String(), 2, nullptr, nullptr, 2.0f, false);
                }
                else if (pPlayer->IsInAir()) {
                    Interface::ShowMessage(sNoJumpWarning.String(), 2, nullptr, nullptr, 2.0f, false);
                }
                else if (pExtra && pExtra->GetCanNotWear() && pExtra->GetWorn(false)) {
                    Interface::ShowMessage(sCantRemoveWornItem.String(), 2, nullptr, nullptr, 2.0f, false);
                }
                else if (!pPlayer->GetDropPoint(pObject, kDropPoint, true, true)) {
                    Interface::ShowMessage(sNotEnoughRoomWarning.String(), 2, nullptr, nullptr, 2.0f, false);
                }
                else {
                    // The compare is signed.
                    if (pCurrItem->GetCount() > static_cast<int32_t>(iInventoryAskQuantityAt.UInt())) {
                        QuantityMenu::Create(
                            pCurrItem->GetCount(),
                            &InventoryMenu::DropCurrentItem,
                            std::numeric_limits<int32_t>::max()
                        );
                    }
                    else {
                        DropCurrentItem(1);
                    }
                    pPlayer->UpdatePlayer3d();
                    pPlayer->GetCurrentAIProcess()->SetShouldRefreshMagicShadersEffects(true, false, false);
                    pPlayer->ResetArmorRating();
                }
            }
            break;
        default: break;
    }
}

void InventoryMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (aiID == 29) {
        InventoryMenu::pCurrItem = lbInventoryList.GetHighlightedData();
        if (InventoryMenu::pCurrItem) {
            const char* pcRepairLabel;
            char acIconPath[264];
            Tile* pHighlightTile = static_cast<Tile*>(lbInventoryList.GetHighlightedItem());
            int32_t iNewHighlightIndex = static_cast<int32_t>(pHighlightTile->GetFloat(4012));
            if (g_lastInventoryMenuSelectionListIndex != iNewHighlightIndex) {
                float fAmount;
                if (g_lastInventoryMenuSelectionListIndex >= iNewHighlightIndex) {
                    fAmount = -fScrollKnobIncrement.Float();
                }
                else {
                    fAmount = fScrollKnobIncrement.Float();
                }
                float fRate = fScrollKnobRate.Float();
                InterfaceManager::Instance()->GetPipboy()->MoveKnob(FOPipboyManager::KNOB_SCROLL, fAmount, fRate, true);
                g_lastInventoryMenuSelectionListIndex = iNewHighlightIndex;
            }
            float fMaintainValue = 75.0f;
            if (InventoryMenu::pCurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                fMaintainValue = 50.0f;
            }
            if (!(static_cast<double>(fMaintainValue) <= InventoryMenu::pCurrItem->GetItemHealth(true))) {
                pcRepairLabel = sRepairItem.String();
            }
            else {
                pcRepairLabel = sMaintainItem.String();
            }
            pTiles[8]->Set(4036, pcRepairLabel, true);
            InventoryMenu::SetupItemDataTile(InventoryMenu::pCurrItem, pTiles[12]);
            pTiles[12]->Set(4003, 1);
            bool bEquippable = pPlayer->CanEquipOrUseObject(InventoryMenu::pCurrItem);
            pMenu->Set(InventoryMenu::iUT_EquippableItem, static_cast<int32_t>(bEquippable));
            const char* pcIcon = InventoryMenu::pCurrItem->GetIconOne(nullptr);
            pTiles[11]->Set(4044, pcIcon, true);
            if (InventoryMenu::pCurrItem->pContainerObj->GetFormType() == WEAP_ID) {
                char acSkillRequirement[132];
                TESObjectWEAP* pWeap = static_cast<TESObjectWEAP*>(InventoryMenu::pCurrItem->pContainerObj);
                ActorValue::Index eWeaponSkill = pWeap->GetWeaponSkill();
                const char* pcBadge = ActorValue::GetItemWeaponTypeBadge(eWeaponSkill);
                pTiles[20]->Set(4044, pcBadge, true);
                BSsprintf(acSkillRequirement, 128, "%d", pWeap->data.iSkillRequirement);
                if (pTiles[22]) {
                    pTiles[22]->Set(4036, acSkillRequirement, true);
                }
            }
            const char* pcMissingImage = sMissingImage.String();
            const char* pcCurrentIcon = pTiles[11]->GetString(4044);
            if (BSstrcmp(pcCurrentIcon, pcMissingImage) &&
                InventoryMenu::pCurrItem->pContainerObj->GetFormType() == AMMO_ID) {
                float fX = -40.0f + 450.0;
                pTiles[11]->Set(4001, fX, true);
                float fSize = -30.0f + 380.0;
                pTiles[11]->Set(4017, fSize, true);
                pTiles[11]->Set(4016, fSize, true);
            }
            else {
                pTiles[11]->Set(4001, 450.0f, true);
                pTiles[11]->Set(4017, 380.0f, true);
                pTiles[11]->Set(4016, 380.0f, true);
            }
            pTiles[11]->AddNeedsUpdate(17);
            BSstrcpy(acIconPath, 260, "Interface\\Icons\\");
            uint32_t uiMinNameLength = BSstrlen(acIconPath);
            BGSEquipType::EQUIP_TYPE eEquipType = BGSEquipType::GetEquipType(InventoryMenu::pCurrItem->pContainerObj);
            BSstrcat(acIconPath, 260, BGSEquipType::GetEquipTypeIconFilename(eEquipType));
            uint32_t uiNameLength = BSstrlen(acIconPath);
            if (uiNameLength <= uiMinNameLength) {
                pTiles[5]->xChildren.GetHead()->Set(4003, 0);
            }
            else {
                pTiles[5]->xChildren.GetHead()->Set(4044, acIconPath, true);
                pTiles[5]->xChildren.GetHead()->Set(4003, 1);
            }
            if (!HKWheel.GetVisible()) {
                pTiles[11]->Set(4003, 1);
            }
            UpdateButtons();
        }
    }
    else if (aiID == 30) {
        ClearCurrentItem();
        pTiles[11]->Set(4044, "Interface\\Icons\\PipboyImages\\Items\\item_keyring.dds", true);
        if (!HKWheel.GetVisible()) {
            pTiles[11]->Set(4003, 1);
        }
        const char* pcLabel = psTargetTypes[2]->String();
        pTiles[6]->Set(4036, pcLabel, true);
        pTiles[6]->Set(4015, 1);
    }
}

// Leaving an item or the keyring tile clears the selection, unless a gamepad user has the quantity menu on top.
void InventoryMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    if (aiID == TI_INVENTORY_ITEM || aiID == TI_KEYRING) {
        if (!InterfaceManager::Has360Controller() || Interface::GetTopMenuID() != Interface::eQuantityMenu) {
            ClearCurrentItem();
        }
        GetMenuTile()->Set(iUT_EquippableItem, 0);
    }
}

void InventoryMenu::DoIdle() {
    static char bWheelUsed;
    if (Interface::IsTopMenuID(Interface::eInventoryMenu)) {
        if (Interface::GetPipboy()->GetInitialized()) {
            if (InterfaceManager::Has360Controller()) {
                if (HKWheel.GetVisible()) {
                    pTiles[20]->Set(4003, 0);
                    pTiles[22]->Set(4003, 0);
                    int32_t iCurrHighlight = HKWheel.MoveHighlight(-1);
                    if (iCurrHighlight < 0) {
                        if (bWheelUsed) {
                            if (InventoryMenu::pCurrItem && InventoryMenu::pCurrItem->GetItemHealth(true) == 0.0) {
                                Interface::ShowMessage(
                                    sCantHotkeyBrokenItem.String(),
                                    0,
                                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                    "UIVATSInsufficientAP",
                                    2.0f,
                                    false
                                );
                            }
                            else if (
                                !pMenu->IsTrue(InventoryMenu::iUT_EquippableItem) ||
                                (InventoryMenu::pCurrItem &&
                                 InventoryMenu::pCurrItem->pContainerObj->GetFormType() == BOOK_ID)
                            ) {
                                Interface::ShowMessage(
                                    sCantHotkeyItem.String(),
                                    0,
                                    "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds",
                                    "UIVATSInsufficientAP",
                                    2.0f,
                                    false
                                );
                            }
                            else {
                                HKWheel.SetHotkeyItem(InventoryMenu::pCurrItem);
                                TESBoundObject* pUpdatedObject = InventoryMenu::pCurrItem->pContainerObj;
                                ItemUpdateList.AddHead(pUpdatedObject);
                                InventoryMenu::UpdateList();
                                HKWheel.SetVisible(true);
                                pTiles[11]->Set(4003, 0);
                            }
                            bWheelUsed = 0;
                        }
                    }
                    else {
                        bWheelUsed = 1;
                    }
                }
                else if (
                    InventoryMenu::pCurrItem && InventoryMenu::pCurrItem->pContainerObj->GetFormType() == WEAP_ID
                ) {
                    pTiles[20]->Set(4003, 1);
                    pTiles[22]->Set(4003, 1);
                    InterfaceManager::UpdatePipBoyInventoryEffects(
                        nullptr,
                        nullptr,
                        pTiles[20],
                        InventoryMenu::pCurrItem,
                        true
                    );
                    InterfaceManager::UpdatePipBoyInventoryEffects(
                        nullptr,
                        nullptr,
                        pTiles[22],
                        InventoryMenu::pCurrItem,
                        true
                    );
                }
            }
            else {
                bool bButtonPressed = false;
                if (InventoryMenu::pCurrItem && InventoryMenu::pCurrItem->pContainerObj->GetFormType() == WEAP_ID) {
                    pTiles[20]->Set(4003, 1);
                    pTiles[22]->Set(4003, 1);
                }

                for (int32_t iHotkeyCt = 0; !bButtonPressed &&
                     !pMenu->IsTrue(InventoryMenu::iUT_KeyringOpen) &&
                     iHotkeyCt < HKWheel.QNumHotkeys();
                     ++iHotkeyCt) {
                    Controls* pControls = appMain->GetControls();
                    const Controls::USERACTIONS eAction = static_cast<Controls::USERACTIONS>(
                        iHotkeyCt + Controls::UACTN_HOTKEY1
                    );
                    if (!pControls->GetUserAction(eAction, 1)) {
                        pControls = appMain->GetControls();
                        if (!pControls->GetUserAction(eAction, 0) || iHotkeyCt == 1) {
                            continue;
                        }
                    }
                    if (iHotkeyCt != 1) {
                        if (HKWheel.GetVisible()) {
                            pTiles[20]->Set(4003, 0);
                            pTiles[22]->Set(4003, 0);
                        }
                        else {
                            HKWheel.SetVisible(true);
                            pTiles[11]->Set(4003, 0);
                        }
                        HKWheel.MoveHighlight(iHotkeyCt);
                        bButtonPressed = true;
                    }
                }
                if (!bButtonPressed && HKWheel.GetVisible()) {
                    HKWheel.SetVisible(false);
                    if (lbInventoryList.GetHighlightedItem()) {
                        pTiles[11]->Set(4003, 1);
                    }
                }
            }
            if (Interface::IsInMenuMode()) {
                Controls* pControls = appMain->GetControls();
                if (pControls->GetMouseButton(1, 1) && lbInventoryList.GetHighlightedItem()) {
                    pControls = appMain->GetControls();
                    if (!pControls->GetUserAction(Controls::UACTN_MENUMODE, 1)) {
                        DoClick(7, nullptr);
                    }
                }
            }
            int32_t iCurrTabIndex = InventoryMenu::pMe->iCurrTab + 24;
            if (iCurrTabIndex == 24) {
                InterfaceManager::UpdatePipBoyInventoryEffects(
                    pTiles[14],
                    pTiles[15],
                    pTiles[16],
                    InventoryMenu::pCurrItem,
                    false
                );
            }
            else if (iCurrTabIndex == 25) {
                Tile* pTileFade2;
                if (!InventoryMenu::pCurrItem ||
                    static_cast<TESObjectARMO*>(InventoryMenu::pCurrItem->pContainerObj)
                            ->TESObjectARMO::GetDamageThreshold() == 0.0) {
                    pTileFade2 = nullptr;
                }
                else {
                    pTileFade2 = pTiles[18];
                }
                if (!InventoryMenu::pCurrItem ||
                    static_cast<TESObjectARMO*>(InventoryMenu::pCurrItem->pContainerObj)
                            ->TESObjectARMO::GetArmorRating() == 0.0) {
                    InterfaceManager::UpdatePipBoyInventoryEffects(
                        nullptr,
                        pTileFade2,
                        nullptr,
                        InventoryMenu::pCurrItem,
                        false
                    );
                }
                else {
                    InterfaceManager::UpdatePipBoyInventoryEffects(
                        pTiles[17],
                        pTileFade2,
                        nullptr,
                        InventoryMenu::pCurrItem,
                        false
                    );
                }
            }
            pTiles[2]->Set(4009, 0.0, true);
            if (pPlayer->GetActorFloatValue(ActorValue::DAMAGE_RESISTANCE) == 0.0) {
                InterfaceManager::UpdatePipBoyInventoryEffects(
                    nullptr,
                    pTiles[21],
                    nullptr,
                    InventoryMenu::pCurrItem,
                    false
                );
            }
            else {
                InterfaceManager::UpdatePipBoyInventoryEffects(
                    pTiles[2],
                    pTiles[21],
                    nullptr,
                    InventoryMenu::pCurrItem,
                    false
                );
            }
        }
    }
}

int32_t InventoryMenu::GetClass() {
    return 1002;
}

bool InventoryMenu::DoGamepad(int32_t aiButton, float afValue) {
    if (!Interface::IsTopMenuID(Interface::eInventoryMenu)) {
        return false;
    }
    if (!Interface::GetPipboy()->GetInitialized()) {
        return false;
    }
    bool bHandled = false;
    if (Interface::Has360Controller() && HKWheel.GetVisible() && aiButton > 0 && aiButton <= 4) {
        bHandled = HotKeysWheel::IsDPadPressed();
    }
    if (!bHandled && Interface::IsTopMenuID(Interface::eInventoryMenu)) {
        switch (aiButton) {
            case 3:
            case 4: {
                uint32_t uiFilter = this->iCurrTab;
                bool bVisible = HKWheel.GetVisible();
                DoLeave(29, nullptr);
                int32_t iNewTab = uiFilter + (2 * (aiButton != 4) - 1);
                int32_t iWrappedTab = iNewTab > 4 ? 0 : iNewTab;
                int32_t iTab = iWrappedTab >= 0 ? iWrappedTab : 4;
                DoClick(iTab + 24, nullptr);
                if (bVisible) {
                    HKWheel.SetVisible(true);
                }
                bHandled = true;
                break;
            }
            case 13: {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                Interface::SetStatsMenuVisible(true, false);
                bHandled = true;
                break;
            }
            case 14: {
                InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
                Interface::SetMapMenuVisible(true, false);
                bHandled = true;
                break;
            }
            case 15: {
                DoClick(19, nullptr);
                bHandled = true;
                break;
            }
            case 16: {
                if (pTiles[TI_HOTKEY_BTN]->GetFloat(4015) != 0.0) {
                    HKWheel.SetVisible(true);
                    pTiles[TI_ITEM_ICON]->Set(4003, 0);
                }
                bHandled = true;
                break;
            }
            default: {
                break;
            }
        }
    }
    return bHandled;
}

bool InventoryMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (Interface::IsTopMenuID(Interface::eInventoryMenu)) {
        if (aiButton == 10) {
            DoClick(10, nullptr);
            return true;
        }

        if (aiButton == 16) {
            HKWheel.SetVisible(false);
            if (lbInventoryList.GetHighlightedItem()) {
                pTiles[TI_ITEM_ICON]->Set(4003, 1);
            }
            return true;
        }
    }
    return false;
}

void InventoryMenu::UpdateButtons() {
    if (InventoryMenu::pCurrItem) {
        if (InventoryMenu::pCurrItem->pContainerObj->GetFormType() == BOOK_ID) {
            const char* pcLabel = psTargetTypes[6]->String();
            pTiles[6]->Set(4036, pcLabel, true);
        }
        else if (InventoryMenu::pCurrItem->GetWorn(false)) {
            const char* pcLabel = sInventoryUnequip.String();
            pTiles[6]->Set(4036, pcLabel, true);
        }
        float fEquippable = pMenu->GetFloat(InventoryMenu::iUT_EquippableItem);
        pTiles[6]->Set(4015, fEquippable, true);
        pTiles[7]->Set(4015, 1);
        int32_t iFormType = InventoryMenu::pCurrItem->pContainerObj->GetFormType();
        pTiles[9]->Set(4015, static_cast<int32_t>(iFormType != 41));
        pTiles[19]->Set(4015, 1);
        int32_t iRepairable = this->IsItemRepairable(InventoryMenu::pCurrItem);
        pTiles[8]->Set(4015, iRepairable);
    }
    else {
        if (InventoryMenu::pMe->iCurrTab == 2) {
            const char* pcLabel = sInventoryUse.String();
            pTiles[6]->Set(4036, pcLabel, true);
        }
        else {
            const char* pcLabel = sInventoryEquip.String();
            pTiles[6]->Set(4036, pcLabel, true);
        }
        pTiles[6]->Set(4015, 0);
        pTiles[7]->Set(4015, 0);
        pTiles[9]->Set(4015, 0);
        pTiles[19]->Set(4015, 0);
        pTiles[8]->Set(4015, 0);
    }
}

void InventoryMenu::ClearCurrentItem() {
    InventoryMenu::pCurrItem = nullptr;
    pTiles[12]->Set(4003, 0);
    pTiles[11]->Set(4003, 0.0f, false);
    pTiles[20]->Set(4003, 0.0f, false);
    pTiles[22]->Set(4003, 0);
    pTiles[5]->xChildren.GetHead()->Set(4003, 0);
    UpdateButtons();
}

void InventoryMenu::SelectCurrentItem() {
    int32_t iHealth;
    ExtraDataList* pExtra;
    TESBoundObject* pObjectRef = InventoryMenu::pCurrItem->pContainerObj;
    if (InventoryMenu::pCurrItem->pExtraObjectList) {
        pExtra = InventoryMenu::pCurrItem->pExtraObjectList->m_item;
    }
    else {
        pExtra = nullptr;
    }
    if (pMenu->IsTrue(InventoryMenu::iUT_EquippableItem)) {
        lbInventoryList.Traverse(&InventoryMenu::AddToChangedItemsListIfWorn, 0, std::numeric_limits<int32_t>::max());
    }
    TESBoundObject* pUpdatedObject = InventoryMenu::pCurrItem->pContainerObj;
    ItemUpdateList.AddHead(pUpdatedObject);
    if (((InventoryMenu::pCurrItem->pContainerObj == BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK) ||
          InventoryMenu::pCurrItem->pContainerObj == TESForm::GetFormByNumericID(0xCCEF2u)) &&
         (iHealth = pPlayer->GetActorValue(ActorValue::HEALTH),
          iHealth == pPlayer->GetPermanentActorValue(ActorValue::HEALTH))) ||
        (InventoryMenu::pCurrItem->pContainerObj == BGSDefaultObjectManager::GetDefaultObject(DO_DOCTOR_BAG) &&
         !IsDrBagUsable())) {
        const char* pcMessage = static_cast<const char*>(sFullHealth);
        Interface::ShowMessage(pcMessage, 0, nullptr, nullptr, 2.0f, false);
    }
    else if (InventoryMenu::pCurrItem->GetWorn(false)) {
        uint32_t uiCount = InventoryMenu::pCurrItem->iNumber;
        pPlayer->QueueUnEquipObject(pObjectRef, uiCount, pExtra, false, false, true);
    }
    else {
        uint32_t uiCount = InventoryMenu::pCurrItem->iNumber;
        pPlayer->QueueEquipObject(pObjectRef, uiCount, pExtra, false, false, true);
    }
    InventoryMenu::UpdateList();
}

bool InventoryMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

bool InventoryMenu::IsItemRepairable(ItemChange* apItem) {
    TESBoundObject* pObj;
    bool bCanRepair = false;
    if (apItem) {
        pObj = apItem->pContainerObj;
    }
    else {
        pObj = nullptr;
    }
    if (apItem) {
        float fHealth = apItem->GetItemHealth(true);
        if (FloatCeil(fHealth) == 100.0) {
            return bCanRepair;
        }
    }
    if (apItem &&
        pObj &&
        (pObj->GetFormType() == WEAP_ID || pObj->GetFormType() == ARMO_ID) &&
        apItem->GetItemHealth(true) < 100.0) {
        if (apItem->iNumber <= 1) {
            ExtraDataList* pExtra;
            BSSimpleList<ItemChange*> kNewItemGroup;
            InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(pPlayer);
            if (apItem->pExtraObjectList) {
                pExtra = apItem->pExtraObjectList->m_item;
            }
            else {
                pExtra = nullptr;
            }
            BSSimpleList<ItemChange*>* pTemp = &kNewItemGroup;
            pInventory->GetRepairItemGroup(pObj, &kNewItemGroup);
            for (ItemChange*& rpItem : kNewItemGroup.ItemsUntilNull()) {
                ExtraDataList* pCurrExtra;
                TESBoundObject* pCurrObj = rpItem->pContainerObj;
                if (rpItem->pExtraObjectList) {
                    pCurrExtra = rpItem->pExtraObjectList->m_item;
                }
                else {
                    pCurrExtra = nullptr;
                }
                if (!rpItem->GetWorn(false)) {
                    float fHealth;
                    if (pCurrObj != pObj ||
                        ((fHealth = rpItem->GetItemHealth(true), FloatCeil(fHealth) < 100.0) &&
                         pCurrExtra != pExtra &&
                         pCurrExtra &&
                         !pCurrExtra->HasWeaponMods())) {
                        bCanRepair = true;
                    }
                }
            }
            while (pTemp && pTemp->m_item) {
                ItemChange* pItem = pTemp->m_item;
                if (pItem) {
                    delete pItem;
                }
                pTemp->RemoveHead();
            }
        }
        else {
            return true;
        }
    }

    return bCanRepair;
}

// Refreshes the player's DR, DT, caps, weight and health lines (and, outside English, the menu's labels), then the item
// list: in full when no worn-state change is pending, else only the entries of the changed objects. The DT sum is added
// in double.
void InventoryMenu::UpdateList() {
    if (!pMe || !Interface::GetPipboy()->GetInitialized()) {
        return;
    }
    InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
    ActorValueOwner* pAVOwner = static_cast<ActorValueOwner*>(pPlayer);
    char acBuffer[64];

    int32_t iDR = FloatToInt(pAVOwner->GetActorFloatValue(ActorValue::DAMAGE_RESISTANCE));
    int32_t iMaxDR = static_cast<int32_t>(fMaxArmorRating.Float());
    if (iDR > iMaxDR) {
        iDR = iMaxDR;
    }
    BSsprintf(acBuffer, 64, "%d", iDR);
    pMe->pTiles[TI_PLAYER_DR_INFO]->Set(uiValueTrait, acBuffer, true);

    float fDT = static_cast<float>(FloatToInt(pAVOwner->GetActorFloatValue(ActorValue::DAMAGE_THRESHOLD)));
    ItemChange* pWeapon = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
    TESForm* pWeaponForm = nullptr;
    if (pWeapon) {
        pWeaponForm = pWeapon->GetItemChangeObject();
    }
    else {
        pWeaponForm = TESForm::GetFormByNumericID(500);
    }
    float fDTBonus = 0.0f;
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_DEFENDER,
        pPlayer,
        pPlayer,
        pWeaponForm,
        &fDTBonus
    );
    BSsprintf(acBuffer, 64, "%1.1f", static_cast<double>(fDT) + fDTBonus);
    pMe->pTiles[TI_PLAYER_DT_INFO]->Set(uiValueTrait, acBuffer, true);

    if (pPlayer->GetGoldAmount() > static_cast<int32_t>(iCapsLimit.UInt())) {
        BSsprintf(acBuffer, 64, "%d+", iCapsLimit.UInt());
    }
    else {
        BSsprintf(acBuffer, 64, "%d", pPlayer->GetGoldAmount());
    }
    pMe->pTiles[TI_PLAYER_CAPS_INFO]->Set(uiValueTrait, acBuffer, true);

    int32_t iMaxWeight = static_cast<int32_t>(pPlayer->GetMaxCarryWeight());
    int32_t iWeight = pAVOwner->GetActorValue(ActorValue::INVENTORY_WEIGHT);
    BSsprintf(acBuffer, 64, "%d/%d", iWeight, iMaxWeight);
    pMe->pTiles[TI_PLAYER_WG_INFO]->Set(uiValueTrait, acBuffer, true);

    int32_t iMaxHealth = pAVOwner->GetPermanentActorValue(ActorValue::HEALTH);
    int32_t iHealth = pAVOwner->GetActorValue(ActorValue::HEALTH);
    BSsprintf(acBuffer, 64, "%d/%d", iHealth, iMaxHealth);
    pMe->pTiles[TI_PLAYER_HP_INFO]->Set(uiValueTrait, acBuffer, true);

    if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
        int32_t iTitleTrait = Tile::TextToTrait("_Title");
        pMe->pTiles[TI_DROP_BTN]->Set(Tile::kTileValue_String, sInventoryDrop.String(), true);
        pMe->pTiles[TI_REPAIR_BTN]->Set(Tile::kTileValue_String, sInventoryRepair.String(), true);
        pMe->pTiles[TI_HOTKEY_BTN]->Set(Tile::kTileValue_String, sInventoryHotKey.String(), true);
        pMe->pTiles[TI_MOD_BTN]->Set(Tile::kTileValue_String, sInventoryMod.String(), true);
        const char* pItems = sInventoryItems.String();
        pMe->GetMenuTile()->GetChildByName("IM_Headline_Title")->Set(Tile::kTileValue_String, pItems, true);
        pMe->pTiles[TI_PLAYER_DR_INFO]->Set(iTitleTrait, sInventoryDamageResistance.String(), true);
        pMe->pTiles[TI_PLAYER_DT_INFO]->Set(iTitleTrait, sInventoryDamageThreshold.String(), true);
        pMe->pTiles[TI_PLAYER_CAPS_INFO]->Set(iTitleTrait, sInventoryCaps.String(), true);
        pMe->pTiles[TI_PLAYER_WG_INFO]->Set(iTitleTrait, sInventoryWeight.String(), true);
        pMe->pTiles[TI_PLAYER_HP_INFO]->Set(iTitleTrait, sHitPointsShort.String(), true);
    }
    pMe->pTiles[TI_HOTKEY_WHEEL]->Set(Tile::kTileValue_Visible, 0);
    pMe->GetMenuTile()->Set(iUT_KeyringOpen, 0);
    pMe->DoLeave(TI_INVENTORY_ITEM, nullptr);

    ListBox<ItemChange*>* pList = &pMe->lbInventoryList;
    if (!pMe->ItemUpdateList.IsEmpty() && pChanges) {
        SavePageState();
        static_cast<ListBoxBase*>(pList)->Highlight(nullptr);
        for (BSSimpleList<TESBoundObject*>* pIter = &pMe->ItemUpdateList; pIter && pIter->GetItem();
             pIter = pIter->GetNext()) {
            BSSimpleList<ItemChange*> kGroup;
            pChanges->GetItemGroup(pIter->GetItem(), &kGroup);
            pMe->lbInventoryList.RemoveItemGroup(pIter->GetItem());
            for (BSSimpleList<ItemChange*>* pEntry = &kGroup; pEntry && pEntry->GetItem(); pEntry = pEntry->GetNext()) {
                char acName[128];
                TESFullName* pFullName = dynamic_cast<TESFullName*>(pEntry->GetItem()->GetItemChangeObject());
                // Measures the name and drops the length.
                BSstrlen(pFullName->GetFullName());
                BSstrcpy(acName, 128, pFullName->GetFullName());
                if (pEntry->GetItem()->GetModSlots()) {
                    BSstrcat(acName, 128, "+");
                }
                pMe->lbInventoryList.AddItem(pEntry->GetItem(), acName, &InventoryMenu_SortFunc, nullptr);
            }
        }
        pMe->lbInventoryList.Traverse(&InventoryMenu::InitFunc, 0, std::numeric_limits<int32_t>::max());
        pMe->lbInventoryList.Synchronize();
        pMe->lbInventoryList.Traverse(&InventoryMenu::SetVisibleFunc, -1, std::numeric_limits<int32_t>::max());
        pMe->ItemUpdateList.RemoveAll();
    }
    else {
        pMe->lbInventoryList.BuildFromContainer(pPlayer, nullptr, nullptr, &InventoryMenu::InitFunc, false, false);
        int32_t iKeyCount;
        if (pChanges && pChanges->GetObjectbyType(KEYM_ID, iKeyCount)) {
            Tile* pKeyring = pMe->lbInventoryList.AddItem(nullptr, sKeyring.String(), nullptr, nullptr);
            pKeyring->Set(Tile::kTileValue_ID, TI_KEYRING);
        }
        pMe->lbInventoryList.Filter(&InventoryMenu::FilterFunc);
        pMe->lbInventoryList.Traverse(&InventoryMenu::SetVisibleFunc, -1, std::numeric_limits<int32_t>::max());
        pList = &pMe->lbInventoryList;
        pList->Sort(&InventoryMenu_SortFunc);
    }
    pMe->ClearCurrentItem();
    LoadPageState();
}

// The item data tile's last 13 children, last first, are its stat rows: 0 damage resistance, 1 damage per second, 2
// weight, 3 value, 4 condition, 5 ammo or armor class, 6 effects, 7 to 9 the three mod rows, 10 strength requirement
// (may be missing), 11 damage, 12 damage threshold; missing children are null. Each row gets its value and title, and
// uiVisible picks the rows shown. It reads apItem's value, weight and object before the null checks.
void InventoryMenu::SetupItemDataTile(ItemChange* apItem, Tile* apTile) {
    uint32_t uiVisible = 0;
    int32_t iTitleTrait = Tile::TextToTrait("_Title");
    int32_t iValueTrait = Tile::TextToTrait("_Value");
    int32_t iConditionTrait = Tile::kTileValue_User5;
    float fMod2Offset = 0.0f;
    float fMod3Offset = 0.0f;
    float fValue = apItem->GetItemValue();
    bool bHardcore = pPlayer->IsHardcore();
    float fWeight = TESWeightForm::GetFormWeight(apItem->GetItemChangeObject(), bHardcore);
    if (!apItem || !apTile) {
        return;
    }
    Tile* pTiles[13];
    NiTListItem<Tile*>* pPos = apTile->xChildren.GetTailPos();
    int32_t i;
    for (i = 0; pPos && i < 13; ++i) {
        pTiles[i] = apTile->xChildren.GetPrev(pPos);
    }
    for (; i < 13; ++i) {
        pTiles[i] = nullptr;
    }
    Tile* pArrows = Tile::GetTileByName(apTile, "CNDArrows");
    Tile* pMeter = Tile::GetTileByName(apTile, "ConditionMeter");
    pArrows->Set(Tile::kTileValue_Height, pMeter->GetFloat(Tile::kTileValue_Height), true);
    pArrows->Set(Tile::kTileValue_Y, pMeter->GetFloat(Tile::kTileValue_Y), true);
    bool bHasMods = apItem->GetModSlots() != 0;
    switch (apItem->GetItemChangeObject()->GetFormType()) {
        case AMMO_ID: {
            uiVisible = 0xC;
            TESAmmo* pAmmo = static_cast<TESAmmo*>(apItem->GetItemChangeObject());
            char acAmmoEffects[256];
            pAmmo->BuildMenuString(acAmmoEffects, 0x100);
            if (BSstrlen(acAmmoEffects) > 0) {
                pTiles[6]->Set(iValueTrait, acAmmoEffects, true);
                uiVisible |= 0x40;
            }
            else {
                uiVisible &= ~0x40u;
            }
            break;
        }
        case WEAP_ID: {
            TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apItem->GetItemChangeObject());
            pTiles[4]->Set(Tile::kTileValue_User6, 1);
            // The player's loaded ammo and the player as the weapon's holder, when the item is the player's weapon.
            TESAmmo* pLoadedAmmo = nullptr;
            Actor* pHolder = nullptr;
            if (pPlayer->GetCurrentAIProcess() &&
                pPlayer->GetCurrentAIProcess()->GetCurrentWeapon() &&
                pWeapon == pPlayer->GetCurrentAIProcess()->GetCurrentWeapon()->GetItemChangeObject()) {
                TESAmmo* pAmmo;
                if (pPlayer->GetCurrentAIProcess()->GetCurrentAmmo()) {
                    pAmmo = static_cast<TESAmmo*>(
                        pPlayer->GetCurrentAIProcess()->GetCurrentAmmo()->GetItemChangeObject()
                    );
                }
                else {
                    pAmmo = nullptr;
                }
                pLoadedAmmo = pAmmo;
                pHolder = pPlayer;
            }
            fWeight = pWeapon->GetFormWeight(apItem->HasModEffectActive(WEAPON_MOD_DECREASE_WEIGHT));
            if (fWeight >= 10.0) {
                float fHeavyMult = 1.0f;
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &fHeavyMult);
                fWeight = fWeight * fHeavyMult;
            }
            float fCondition = static_cast<float>(apItem->GetItemHealth(true) / 100.0);
            uint32_t uiStrRequired = pWeapon->GetStrRequired();
            float fDamage = 0.0f;
            float fDPS = 0.0f;
            fDPS = CombatUtilities::CalculateItemDPS(
                pPlayer,
                pWeapon,
                fCondition,
                true,
                apItem,
                false,
                false,
                -1,
                0.0f,
                0.0f,
                false,
                false,
                pLoadedAmmo
            );
            // A NaN condition still asks for the damage.
            if (fCondition <= 0.0) {
                fDamage = 0.0f;
            }
            else {
                fDamage = CombatFormulas::CalcWeaponDamageForDisplay(pWeapon, fCondition, apItem, pLoadedAmmo);
            }
            char acBuffer[128];
            BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(BSRoundToNearest(fDPS, 1.0f)));
            pTiles[1]->Set(iValueTrait, acBuffer, true);
            Actor* pProjectileHolder = pHolder;
            uint32_t uiProjectiles = pWeapon->GetNumProjectiles(
                apItem->HasModEffectActive(WEAPON_MOD_SPLIT_BEAM),
                false,
                pProjectileHolder
            );
            if (uiProjectiles > 1) {
                float fPerProjectile = static_cast<float>(fDamage / static_cast<double>(uiProjectiles));
                BSsprintf(acBuffer, 0x80, "%.1fx%d", fPerProjectile, uiProjectiles);
            }
            else {
                BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(BSRoundToNearest(fDamage, 1.0f)));
            }
            pTiles[11]->Set(iValueTrait, acBuffer, true);
            pTiles[4]->Set(iConditionTrait, BSMIN(fCondition, 1.0f), true);
            float fStrBonus = 0.0f;
            BGSEntryPoint::HandleEntryPoint(
                BGSEntryPoint::WEAPON_HANDLING_STRENGTH_BONUS,
                pPlayer,
                pWeapon,
                &fStrBonus
            );
            // The bonus is converted by truncation (a float to unsigned conversion), and the difference is clamped at 0
            // on its sign bit.
            uint32_t uiStrBonus = static_cast<uint32_t>(static_cast<int64_t>(fStrBonus));
            uiStrRequired -= uiStrBonus;
            if (static_cast<int32_t>(uiStrRequired) < 0) {
                uiStrRequired = 0;
            }
            BSsprintf(acBuffer, 0x80, "%d", uiStrRequired);
            if (pTiles[10]) {
                pTiles[10]->Set(iValueTrait, acBuffer, true);
            }
            if (pWeapon->GetCurrentAmmo(pHolder)) {
                ItemChange* pWeaponInfo = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
                ItemChange* pAmmoInfo = pPlayer->GetCurrentAIProcess()->GetCurrentAmmo();
                const char* pAmmoName = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(pHolder))->GetShortName();
                TESForm* pAmmoForm = pWeapon->GetCurrentAmmo(pHolder);
                InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
                int32_t iAmmoCount = pChanges->GetObjectCount(static_cast<TESBoundObject*>(pAmmoForm));
                int32_t iClipRounds = 0;
                if (!pAmmoName || !*pAmmoName) {
                    pAmmoName = static_cast<TESAmmo*>(pWeapon->GetCurrentAmmo(pHolder))->TESFullName::GetFullName();
                }
                // The item is the equipped weapon when both share their first instance: the loaded rounds are the ammo
                // item's count, else a clip's worth of the carried ammo.
                if (pWeaponInfo &&
                    pWeaponInfo->GetExtraDataList() &&
                    apItem->GetExtraDataList() &&
                    pWeaponInfo->GetExtraDataList()->GetItem() == apItem->GetExtraDataList()->GetItem()) {
                    int32_t iLoaded;
                    if (pAmmoInfo) {
                        iLoaded = pAmmoInfo->GetCount();
                    }
                    else {
                        iLoaded = 0;
                    }
                    iClipRounds = iLoaded;
                }
                else {
                    iClipRounds = BSMIN(
                        pWeapon->GetFormClipRounds(apItem->HasModEffectActive(WEAPON_MOD_INCREASE_CLIP_SIZE)),
                        iAmmoCount
                    );
                }
                bool bRegenerates = pWeapon->GetAmmoRegenRate(false) != 0.0;
                if (bRegenerates) {
                    BSsprintf(acBuffer, 0x80, "%s", pAmmoName);
                }
                else {
                    BSsprintf(acBuffer, 0x80, "%s (%i/%i)", pAmmoName, iClipRounds, iAmmoCount - iClipRounds);
                }
            }
            else {
                BSstrcpy(acBuffer, 0x80, "--");
            }
            pTiles[5]->Set(iTitleTrait, acBuffer, true);
            uiVisible = 0xC3E;
            break;
        }
        case ARMO_ID:
        case CLOT_ID: {
            pTiles[4]->Set(Tile::kTileValue_User6, 0);
            float fCondition = static_cast<float>(apItem->GetItemHealth(true) / 100.0);
            uiVisible = 0x3C;
            char acBuffer[128];
            BSstrcpy(acBuffer, 0x80, "--");
            TESObjectARMO* pArmor = dynamic_cast<TESObjectARMO*>(apItem->GetItemChangeObject());
            if (pArmor) {
                if (pArmor->IsHeavy()) {
                    BSstrcpy(acBuffer, 0x80, pGSArmorWeight[1]->String());
                }
                else if (pArmor->IsMedium()) {
                    BSstrcpy(acBuffer, 0x80, pGSArmorWeight[2]->String());
                }
                else {
                    BSstrcpy(acBuffer, 0x80, pGSArmorWeight[0]->String());
                }
            }
            pTiles[5]->Set(iTitleTrait, acBuffer, true);
            float fArmorRating = CombatFormulas::CalcArmorRating(
                static_cast<uint16_t>(
                    static_cast<int32_t>(static_cast<TESObjectARMO*>(apItem->GetItemChangeObject())->GetArmorRating())
                ),
                fCondition
            );
            float fThreshold = CombatFormulas::CalcArmorRating(
                static_cast<uint16_t>(static_cast<int32_t>(
                    static_cast<TESObjectARMO*>(apItem->GetItemChangeObject())->GetDamageThreshold()
                )),
                fCondition
            );
            if (fArmorRating == 0.0 && fThreshold == 0.0) {
                uiVisible |= 1;
                BSstrcpy(acBuffer, 0x80, "--");
                pTiles[0]->Set(iValueTrait, acBuffer, true);
            }
            else {
                if (fArmorRating != 0.0) {
                    BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(FloatCeil(fArmorRating)));
                    pTiles[0]->Set(iValueTrait, acBuffer, true);
                    uiVisible |= 1;
                }
                if (fThreshold != 0.0) {
                    BSsprintf(acBuffer, 0x80, "%d", static_cast<int32_t>(FloatCeil(fThreshold)));
                    pTiles[12]->Set(iValueTrait, acBuffer, true);
                    uiVisible |= 0x1000;
                }
            }
            uint32_t uiMaxHealth;
            if (TESHealthForm::GetFormHealth(apItem->GetItemChangeObject())) {
                uiMaxHealth = TESHealthForm::GetFormHealth(apItem->GetItemChangeObject());
            }
            else {
                uiMaxHealth = 1;
            }
            float fHealthRatio = static_cast<float>(apItem->GetItemHealth(false) / static_cast<double>(uiMaxHealth));
            pTiles[4]->Set(iConditionTrait, BSMIN(1.0f, fHealthRatio), true);
            break;
        }
        default: uiVisible = 0xC; break;
    }
    EffectItemList* pEffects = nullptr;
    if (apItem->GetItemChangeObject()->GetFormType() == INGR_ID) {
        pEffects = static_cast<IngredientItem*>(apItem->GetItemOwnership());
    }
    else if (apItem->GetItemChangeObject()->GetFormType() == ALCH_ID) {
        pEffects = static_cast<AlchemyItem*>(apItem->GetItemChangeObject());
    }
    else {
        EnchantmentItem* pEnchantment = TESEnchantableForm::GetFormEnchanting(apItem->GetItemChangeObject());
        if (pEnchantment && !pEnchantment->GetFlag(EnchantmentItem::HIDE_EFFECT)) {
            pEffects = pEnchantment;
        }
    }
    if (pEffects && pEffects->GetItem()) {
        char acEffects[256];
        pEffects->GetEffectsString(acEffects, 0x100);
        if (BSstrlen(acEffects) > 0) {
            pTiles[6]->Set(iValueTrait, acEffects, true);
            uiVisible |= 0x40;
        }
        else {
            uiVisible &= ~0x40u;
        }
    }
    else if (apItem->GetItemChangeObject()->GetFormType() != AMMO_ID) {
        uiVisible &= ~0x40u;
    }
    if (apItem->GetItemChangeObject()->GetFormType() == IMOD_ID) {
        TESObjectIMOD* pItemMod = static_cast<TESObjectIMOD*>(apItem->GetItemChangeObject());
        pTiles[6]->Set(iValueTrait, pItemMod->GetDescription(nullptr, DESC_ID), true);
        uiVisible |= 0x40;
    }
    if (bHasMods) {
        TESObjectWEAP* pWeapon = static_cast<TESObjectWEAP*>(apItem->GetItemChangeObject());
        TESObjectIMOD* pMod = nullptr;
        uint8_t ucModSlots = apItem->GetModSlots();
        bool bTitleSet = false;
        char acModName[128];
        if (ucModSlots & 1) {
            uiVisible |= 0x80;
            pTiles[7]->Set(iTitleTrait, sModEffects.String(), true);
            bTitleSet = true;
            fMod2Offset = static_cast<float>(fMod2Offset + 41.0);
            fMod3Offset = static_cast<float>(fMod3Offset + 41.0);
            pMod = pWeapon->GetModItem(1);
            if (pMod) {
                BSstrcpy(acModName, 0x80, pMod->TESFullName::GetFullName());
            }
            else {
                BSstrcpy(acModName, 0x80, "Mod 1");
            }
            pTiles[7]->Set(iValueTrait, acModName, true);
        }
        if (ucModSlots & 2) {
            uiVisible |= 0x100;
            pMod = pWeapon->GetModItem(2);
            if (pMod) {
                BSstrcpy(acModName, 0x80, pMod->TESFullName::GetFullName());
            }
            else {
                BSstrcpy(acModName, 0x10, "Mod 2");
            }
            pTiles[8]->Set(iValueTrait, acModName, true);
            if (!bTitleSet) {
                pTiles[8]->Set(iTitleTrait, sModEffects.String(), true);
                bTitleSet = true;
            }
            fMod3Offset = static_cast<float>(fMod3Offset + 41.0);
        }
        if (ucModSlots & 4) {
            pMod = pWeapon->GetModItem(4);
            if (pMod) {
                BSstrcpy(acModName, 0x80, pMod->TESFullName::GetFullName());
            }
            else {
                BSstrcpy(acModName, 0x10, "Mod 3");
            }
            pTiles[9]->Set(iValueTrait, acModName, true);
            uiVisible |= 0x200;
            if (!bTitleSet) {
                pTiles[9]->Set(iTitleTrait, sModEffects.String(), true);
                bTitleSet = true;
            }
        }
        uiVisible &= ~0x40u;
    }
    else {
        uiVisible &= ~0x80u;
    }
    char acValue[16];
    if (fValue > 0.0) {
        // The count times the value at x87 precision, passed as a double.
        if (fValue < 1.0) {
            BSsprintf(acValue, 0x10, "%.1f", static_cast<double>(apItem->GetCount()) * fValue);
        }
        else {
            BSsprintf(acValue, 0x10, "%.0f", static_cast<double>(apItem->GetCount()) * fValue);
        }
    }
    else {
        BSstrcpy(acValue, 0x10, "--");
    }
    pTiles[3]->Set(iValueTrait, acValue, true);
    float fLightItems = 0.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_LIGHT_ITEMS_WEIGH_NOTHING, pPlayer, &fLightItems);
    if (fLightItems > 0.0) {
        if (fPackRatThreshold.Float() >= fWeight) {
            fWeight = fWeight * fPackRatModifier.Float();
        }
    }
    char acWeight[32];
    if (fWeight > 0.0) {
        BSsprintf(acWeight, 0x20, "%.2f", static_cast<double>(apItem->GetCount()) * fWeight);
    }
    else {
        BSstrcpy(acWeight, 0x20, "--");
    }
    pTiles[2]->Set(iValueTrait, acWeight, true);
    if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
        pTiles[0]->Set(iTitleTrait, sInventoryDamageResistance.String(), true);
        pTiles[1]->Set(iTitleTrait, sInventoryDamagePerSecond.String(), true);
        pTiles[11]->Set(iTitleTrait, sInventoryDamage.String(), true);
        pTiles[2]->Set(iTitleTrait, sInventoryWeightUpper.String(), true);
        pTiles[3]->Set(iTitleTrait, sInventoryValue.String(), true);
        pTiles[4]->Set(iTitleTrait, sInventoryCondition.String(), true);
        pTiles[6]->Set(iTitleTrait, sInventoryEffects.String(), true);
        pTiles[7]->Set(iTitleTrait, sModEffects.String(), true);
        pTiles[10]->Set(iTitleTrait, sInventoryStrReq.String(), true);
    }
    pTiles[0]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 1) != 0));
    pTiles[12]->Set(Tile::kTileValue_Visible, static_cast<int32_t>((uiVisible & 0x1000) != 0));
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
    float fY = static_cast<float>(apTile->GetFloat(Tile::kTileValue_Height) / 2.0 - 20.0);
    if ((uiVisible & 0x10) || (uiVisible & 0x20)) {
        fY = fY + fY;
    }
    pTiles[6]->Set(Tile::kTileValue_Y, fY, true);
    pTiles[7]->Set(Tile::kTileValue_Y, fY, true);
    float fMod2Y = fY + fMod2Offset;
    pTiles[8]->Set(Tile::kTileValue_Y, fMod2Y, true);
    float fMod3Y = fY + fMod3Offset;
    pTiles[9]->Set(Tile::kTileValue_Y, fMod3Y, true);
}

void InventoryMenu::SavePageState() {
    int32_t iHighlightIndex;
    uint32_t uiFilter;
    if (InventoryMenu::pMe->pMenu->IsTrue(InventoryMenu::iUT_KeyringOpen)) {
        uiFilter = 5;
    }
    else {
        uiFilter = InventoryMenu::pMe->iCurrTab;
    }
    if (InventoryMenu::pMe->lbInventoryList.GetHighlightedItem()) {
        Tile* pHighlightTile = static_cast<Tile*>(InventoryMenu::pMe->lbInventoryList.GetHighlightedItem());
        iHighlightIndex = static_cast<int32_t>(pHighlightTile->GetFloat(4012));
    }
    else {
        iHighlightIndex = -1;
    }
    InventoryMenu::pMe->pPageStatesA[uiFilter].iHighlightIndex = iHighlightIndex;
    int32_t iCurrentValueTrait = Tile::TextToTrait("_current_value");
    InventoryMenu::pMe->pPageStatesA[uiFilter].iScrollIndex = static_cast<int32_t>(
        InventoryMenu::pMe->lbInventoryList.pScrollBar->GetFloat(iCurrentValueTrait)
    );
}

bool InventoryMenu::IsKeyringOpen() {
    return pMe && pMe->GetMenuTile()->IsTrue(iUT_KeyringOpen);
}

void InventoryMenu::DropCurrentItem(int32_t aiAmountChosen) {
    ExtraDataList* pExtra;
    NiPoint3 kDropPoint = NiPoint3::ZERO;
    TESBoundObject* pObject = InventoryMenu::pCurrItem->pContainerObj;
    if (InventoryMenu::pCurrItem->pExtraObjectList) {
        pExtra = InventoryMenu::pCurrItem->pExtraObjectList->m_item;
    }
    else {
        pExtra = nullptr;
    }
    if (aiAmountChosen) {
        if (pExtra && pExtra->GetCount() < aiAmountChosen) {
            pExtra = nullptr;
        }

        pPlayer->GetDropPoint(pObject, kDropPoint, true, true);
        pPlayer->DropObject(pObject, pExtra, aiAmountChosen, &kDropPoint, nullptr);
        if (InventoryMenu::pMe) {
            if (InventoryMenu::pCurrItem) {
                TESBoundObject* pUpdatedObject = InventoryMenu::pCurrItem->pContainerObj;
                InventoryMenu::pMe->ItemUpdateList.AddHead(pUpdatedObject);
            }
            InventoryMenu::UpdateList();
        }
    }
}

int32_t InventoryMenu::GetClassStatic() {
    return 1002;
}

// Builds the inventory menu from inventory_menu.xml, replacing an open one; null when the menu is malformed.
Tile* InventoryMenu::Create() {
    InterfaceManager* pManager = InterfaceManager::Instance();
    // Reads the maximum depth and drops it.
    Tile::GetMaximumDepth();
    TileMenu* pOldMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(GetClassStatic()));
    if (pOldMenu) {
        delete pOldMenu;
    }
    Tile* pTile = pManager->GetMenusRoot()->ReadFile("Data\\Menus\\Main\\inventory_menu.xml");
    pMe = static_cast<InventoryMenu*>(pTile->GetMenu());
    Menu::RemoveFromXMLArchive("Data\\Menus\\Main\\inventory_menu.xml", pMe);
    if (!pMe || pMe->GetClass() != GetClassStatic()) {
        if (pMe && pMe->GetMenuTile()) {
            delete pMe;
        }
        return nullptr;
    }
    TileMenu* pMenuTile = static_cast<TileMenu*>(pTile);
    pMe->SetMenuTile(pMenuTile, false);
    pMe->SetRenderedMenu();
    if (!pMe->IsWellFormedMenu()) {
        Warning("MENUS: Inventory Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }
    if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
        pTile->Set(4013, Tile::GetMaximumDepth(), true);
    }
    int32_t iMenuID = GetClassStatic();
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_WEAPONS, iMenuID, 0x200);
    pMe->lbInventoryList.Initialize(pMe->pTiles[TI_INVENTORY_LIST], "IM_InventoryListTemplate");
    pMe->lbInventoryList.SetAutoUpdate(false);
    const char* pAmmo = sInventoryAmmoTab.String();
    const char* pMisc = sInventoryMisc.String();
    const char* pAid = sInventoryAid.String();
    const char* pApparel = sInventoryApparel.String();
    const char* pWeapons = sInventoryWeapons.String();
    Interface::InitializeTabline(
        pMe->pTiles[TI_TABLINE],
        24,
        pWeapons,
        pApparel,
        pAid,
        pMisc,
        pAmmo,
        static_cast<const char*>(nullptr)
    );
    pMe->HKWheel.Initialize(pMe->pTiles[TI_HOTKEY_WHEEL]);
    pMe->pTiles[TI_CANCEL_BTN]->Set(4036, sCancel.String(), true);
    int32_t iTitleTrait = Tile::TextToTrait("_Title");
    if (pMe->pTiles[TI_DAM]) {
        pMe->pTiles[TI_DAM]->Set(iTitleTrait, sInventoryDamage.String(), true);
    }
    if (pMe->pTiles[TI_DPS]) {
        pMe->pTiles[TI_DPS]->Set(iTitleTrait, sInventoryDamagePerSecond.String(), true);
    }
    if (pMe->pTiles[TI_STRREQ]) {
        pMe->pTiles[TI_STRREQ]->Set(iTitleTrait, sInventoryStrReq.String(), true);
    }
    if (pMe->pTiles[TI_DR]) {
        pMe->pTiles[TI_DR]->Set(iTitleTrait, sInventoryDamageResistance.String(), true);
    }
    if (pMe->pTiles[TI_DT]) {
        pMe->pTiles[TI_DT]->Set(iTitleTrait, sInventoryDamageThreshold.String(), true);
    }
    pMe->PrepForVisibility(true);
    return pMenuTile;
}

void InventoryMenu::SetKnobPosition(bool abPlaySounds) {
    uint32_t uiFilter;
    if (InventoryMenu::pMe) {
        uiFilter = InventoryMenu::pMe->iCurrTab;
        InterfaceManager::Instance()->GetPipboy()->SetTabKnobPosition(uiFilter, abPlaySounds);
    }
}

uint8_t InventoryMenu::GetCurrentInventoryPage() {
    if (InventoryMenu::pMe) {
        return static_cast<uint8_t>(InventoryMenu::pMe->iCurrTab);
    }

    return 0;
}

void InventoryMenu::AddToChangedItemsListIfWorn([[maybe_unused]] Tile* apTile, ItemChange* apItem) {
    if (apItem && apItem->GetWorn(false)) {
        TESBoundObject* pObject = apItem->GetContainerObject();
        pMe->ItemUpdateList.AddHead(pObject);
    }
}

void InventoryMenu::LoadPageState() {
    uint32_t uiTab;
    // The keyring has its own saved position after the five filter tabs.
    if (pMe->GetMenuTile()->IsTrue(iUT_KeyringOpen)) {
        uiTab = 5;
    }
    else {
        uiTab = pMe->iCurrTab;
    }
    pMe->lbInventoryList.SetListState(
        static_cast<float>(pMe->pPageStatesA[uiTab].iHighlightIndex),
        static_cast<float>(pMe->pPageStatesA[uiTab].iScrollIndex)
    );
}

bool InventoryMenu::KeyFilterFunc(ItemChange* apItem) {
    if (!apItem) {
        return true;
    }
    return apItem->pContainerObj->GetFormType() != 46;
}

void InventoryMenu::SetVisibleFunc(Tile* apTile, ItemChange* apItem) {
    int32_t iVisible = apTile->GetFloat(4012) >= 0.0;
    apTile->Set(4003, iVisible);
}

bool InventoryMenu::FilterFunc(ItemChange* apItem) {
    bool bHidden;
    if (InventoryMenu::pMe) {
        TESAmmo* pAmmo;
        if (apItem) {
            pAmmo = static_cast<TESAmmo*>(apItem->pContainerObj);
        }
        else {
            pAmmo = nullptr;
        }
        int32_t iCurrTabIndex = InventoryMenu::pMe->iCurrTab + 24;
        if (pAmmo) {
            ENUM_FORM_ID eFormType = pAmmo->GetFormType();
            TESBipedModelForm* pBipedForm = TESBipedModelForm::GetFormAsBipedModel(pAmmo);
            if ((pBipedForm && !pBipedForm->GetPlayable()) ||
                TESContainer::IsGold(pAmmo) ||
                (pAmmo->GetQuestObject() && !pPlayer->IsShowQuestItems()) ||
                eFormType == KEYM_ID) {
                bHidden = true;
            }
            else {
                switch (iCurrTabIndex) {
                    case 24: {
                        bHidden = eFormType != WEAP_ID;
                        break;
                    }
                    case 25: {
                        bHidden = eFormType != ARMO_ID && eFormType != CLOT_ID;
                        break;
                    }
                    case 26: {
                        bHidden = eFormType != INGR_ID && eFormType != ALCH_ID && eFormType != BOOK_ID;
                        break;
                    }
                    case 27: {
                        bHidden = eFormType == WEAP_ID ||
                            eFormType == ARMO_ID ||
                            eFormType == CLOT_ID ||
                            eFormType == INGR_ID ||
                            eFormType == ALCH_ID ||
                            eFormType == AMMO_ID ||
                            eFormType == BOOK_ID;
                        break;
                    }
                    case 28: {
                        if (eFormType == AMMO_ID) {
                            BGSListForm* pList = static_cast<BGSListForm*>(
                                TESForm::GetFormByEditorID("RegeneratingAmmo")
                            );
                            bHidden = pList && pList->GetItemIndex(pAmmo) >= 0;
                        }
                        else {
                            bHidden = true;
                        }
                        break;
                    }
                    default: {
                        bHidden = true;
                        break;
                    }
                }
            }
        }
        else {
            return iCurrTabIndex != 27;
        }
    }
    else {
        bHidden = true;
    }
    return bHidden;
}

void InterfaceManager::UpdateInventoryPageNumber(char acTab) {
    cInventoryPageNumber = acTab;
}
