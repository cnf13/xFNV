#include "Fallout/Interface/HotKeysWheel.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Tile.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSEquipType.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "SDK/MSVC/rtti.h"

HotKeysWheel::~HotKeysWheel() {
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
    for (int32_t iKey = 0; iKey < 8; ++iKey) {
        ItemChange* pHotKeyItem = pinventory->GetHotKeyItem(iKey);
        if (pHotKeyItem && pHotKeyItem->pContainerObj) {
            TESModelTextureSwap* pModel = TESBipedModelForm::GetBipedTESModel(pHotKeyItem->pContainerObj, pPlayer);
            if (!pModel) {
                pModel = dynamic_cast<TESModelTextureSwap*>(pHotKeyItem->pContainerObj);
            }
            if (pModel) {
                const char* pcModelName = pModel->GetModel();
                pModelLoader->ReleaseFile(pcModelName);
            }
        }

        if (pHotKeyItem) {
            delete pHotKeyItem;
        }
    }
}

void HotKeysWheel::Initialize(Tile* apWheel) {
    BSStringT<char> strName;
    if (apWheel) {
        pWheel = apWheel;
        for (int32_t i = 0; i < 8; ++i) {
            strName.SPrintF("HK_Item_%d", i);
            pHotkeyTilesA[i] = pWheel->GetChildByName(strName.pString);
        }
        iUT_HighlightedHotkey = Tile::AddUserTrait("_SelectedHotkey", -1);
        iUT_HighlightedText = Tile::AddUserTrait("_SelectedText", -1);
        bSelectingHotkey = false;
        pWheel->Set(iUT_HighlightedHotkey, -1);
        pWheel->Set(iUT_HighlightedText, "", true);
    }
    else {
        Warning("HotKeyWheel: Hot key wheel being initialized with a NULL parent tile.  This...should not...be!");
    }
}

bool HotKeysWheel::GetVisible() {
    return pWheel->Tile::IsTrue(4003);
}

void HotKeysWheel::SetVisible(bool abVisible) {
    if (abVisible) {
        UpdateHotkeyList();
        pWheel->Tile::Set(4003, 1);
    }
    else {
        pWheel->Tile::Set(4003, 0);
    }
}

void HotKeysWheel::UpdateHotkeyList() {
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
    BSStringT<char> strtemp;
    if (pinventory) {
        ItemChange** pcurrHotkeyedItem = pinventory->GetHotKeyItem(0, 7);
        for (int32_t ihotkeyCt = 0; ihotkeyCt < 8; ++ihotkeyCt) {
            if (ihotkeyCt != 1) {
                ItemChange* pcurrItem = pcurrHotkeyedItem[ihotkeyCt];
                if (pcurrItem) {
                    strtemp = "Interface\\Icons\\";
                    BGSEquipType::EQUIP_TYPE eEquipType = BGSEquipType::GetEquipType(pcurrItem->pContainerObj);
                    strtemp += BGSEquipType::GetEquipTypeIconFilename(eEquipType);
                    char* pcIconPath = strtemp.pString;
                    int32_t iTrait = Tile::TextToTrait("_HotKeyIcon");
                    pHotkeyTilesA[ihotkeyCt]->Set(iTrait, pcIconPath, true);
                    iTrait = Tile::TextToTrait("_HotKeyAssigned");
                    pHotkeyTilesA[ihotkeyCt]->Set(iTrait, 1);
                    if (pcurrItem->iNumber <= 1) {
                        strtemp = pcurrItem->GetFullName();
                    }
                    else {
                        uint32_t uiCount = pcurrItem->iNumber;
                        strtemp.SPrintF("%s (%d)", pcurrItem->GetFullName(), uiCount);
                    }
                    pHotkeyTilesA[ihotkeyCt]->Set(4036, strtemp.pString, true);
                }
                else {
                    int32_t iTrait = Tile::TextToTrait("_HotKeyAssigned");
                    pHotkeyTilesA[ihotkeyCt]->Set(iTrait, 0);
                    pHotkeyTilesA[ihotkeyCt]->Set(4036, "", true);
                }
            }
        }
        for (int32_t i = 0; i < 8; ++i) {
            ItemChange* pcurrItem = pcurrHotkeyedItem[i];
            if (pcurrItem) {
                delete pcurrItem;
            }
        }
        ourFree(pcurrHotkeyedItem);
    }
}

int32_t HotKeysWheel::MoveHighlight(int32_t aiNewIndex) {
    static int32_t inewKeyPressed = -1;
    static int32_t ikeyPressStart;
    int32_t iIndex = -1;
    if (aiNewIndex == 1) {
        return -1;
    }
    if (InterfaceManager::Has360Controller()) {
        if (aiNewIndex < 0) {
            int32_t iDown = InterfaceManager::CheckMenuButton(Interface::eButtonDPadDown, false);
            int32_t iUp = InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, false);
            int32_t iLeft = InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, false);
            int32_t iRight = InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, false);
            bool bDown = iDown == 2 || !iDown;
            bool bUp = iUp == 2 || !iUp;
            bool bLeft = iLeft == 2 || !iLeft;
            bool bRight = iRight == 2 || !iRight;
            if (bDown) {
                if (bLeft) {
                    iIndex = 5;
                }
                else if (bRight) {
                    iIndex = 7;
                }
                else {
                    iIndex = 6;
                }
            }
            else if (bUp) {
                if (bLeft) {
                    iIndex = 0;
                }
                else if (bRight) {
                    iIndex = 2;
                }
            }
            else if (bLeft) {
                iIndex = 3;
            }
            else if (bRight) {
                iIndex = 4;
            }
        }
        int32_t iCurrent = static_cast<int32_t>(pWheel->GetFloat(iUT_HighlightedHotkey));
        if (iIndex == iCurrent) {
            return iIndex;
        }
        if (iIndex != inewKeyPressed) {
            inewKeyPressed = iIndex;
            ikeyPressStart = appTimer.GetTime();
        }
        if (!(static_cast<double>(appTimer.GetTime() - ikeyPressStart) > 50.0f) && iIndex != -1 && iCurrent != -1) {
            return iIndex;
        }
    }
    else if (aiNewIndex >= 0) {
        iIndex = aiNewIndex;
    }
    HighlightHotkey(iIndex);
    return iIndex;
}

void HotKeysWheel::UseHotkeyItem() {
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
    ItemChange* pitem = pinventory ? pinventory->GetHotKeyItem(iLastHighlightedHotkey) : nullptr;
    ExtraDataList* pextra = pitem && pitem->pExtraObjectList ? pitem->pExtraObjectList->m_item : nullptr;
    bool bThrown = pitem &&
        pitem->pContainerObj->GetFormType() == WEAP_ID &&
        static_cast<TESObjectWEAP*>(pitem->pContainerObj)->TESObjectWEAP::IsThrownWeapon();
    if (bSelectingHotkey &&
        pinventory &&
        pitem &&
        (pPlayer->GetAnimAction() == ANIM_ACTION_NONE || pitem->pContainerObj->GetFormType() != ARMO_ID) &&
        (pPlayer->GetAnimAction() == ANIM_ACTION_NONE || pitem->pContainerObj->GetFormType() != WEAP_ID)) {
        int32_t iHealth;
        TESBoundObject* pObject = pitem->pContainerObj;
        if (pObject == BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK) &&
            (iHealth = pPlayer->GetActorValue(ActorValue::HEALTH),
             iHealth == pPlayer->GetPermanentActorValue(ActorValue::HEALTH))) {
            Interface::ShowMessage(static_cast<const char*>(sFullHealth), 0, nullptr, nullptr, 2.0f, false);
        }
        else {
            if (pitem->GetWorn(false)) {
                uint32_t uiCount = bThrown ? pitem->iNumber : 1;
                pPlayer->QueueUnEquipObject(pitem->pContainerObj, uiCount, pextra, false, false, true);
            }
            else {
                uint32_t uiCount = bThrown ? pitem->iNumber : 1;
                pPlayer->QueueEquipObject(pitem->pContainerObj, uiCount, pextra, false, false, true);
            }
            pPlayer->UpdatePlayer3d();
            pPlayer->pCurrentProcess->SetShouldRefreshMagicShadersEffects(true, false, false);
            pPlayer->ResetArmorRating();
            delete pitem;
        }
    }
}

void HotKeysWheel::SetHotkeyItem(ItemChange* apHotkeyItem) {
    InventoryChanges* pinventory = InventoryChanges::GetInventoryChanges(pPlayer);
    if (apHotkeyItem && bSelectingHotkey && pinventory) {
        ItemChange* pHotKeyItem = pinventory->GetHotKeyItem(iLastHighlightedHotkey);
        ExtraDataList* pextra = apHotkeyItem->pExtraObjectList ? apHotkeyItem->pExtraObjectList->m_item : nullptr;
        if (pHotKeyItem) {
            pinventory->RemoveHotKeyItem(pHotKeyItem, iLastHighlightedHotkey);
            if (pHotKeyItem->pContainerObj) {
                TESModelTextureSwap* pModel = TESBipedModelForm::GetBipedTESModel(pHotKeyItem->pContainerObj, pPlayer);
                if (!pModel) {
                    pModel = dynamic_cast<TESModelTextureSwap*>(pHotKeyItem->pContainerObj);
                }
                if (pModel) {
                    const char* pcModelName = pModel->GetModel();
                    pModelLoader->ReleaseFile(pcModelName);
                }
            }
        }
        pinventory->SetHotKeyItem(apHotkeyItem, pextra, iLastHighlightedHotkey);
        if (apHotkeyItem->pContainerObj) {
            TESModelTextureSwap* pModel = TESBipedModelForm::GetBipedTESModel(apHotkeyItem->pContainerObj, pPlayer);
            if (!pModel) {
                pModel = dynamic_cast<TESModelTextureSwap*>(apHotkeyItem->pContainerObj);
            }
            if (pModel) {
                const char* pcModelName = pModel->GetModel();
                pModelLoader->LoadFile(pcModelName, LOD_MULT_NONE, true, false, false, false);
            }
        }
        UpdateHotkeyList();
        pPlayer->PlayPickUpSound(apHotkeyItem->pContainerObj, true, false);
        bSelectingHotkey = false;
        if (pHotKeyItem) {
            delete pHotKeyItem;
        }
    }
}

int32_t HotKeysWheel::QNumHotkeys() {
    return ciNumHotkeys;
}

void HotKeysWheel::HighlightHotkey(int32_t aiIndex) {
    pWheel->Set(iUT_HighlightedHotkey, aiIndex);
    if (aiIndex < 0 || aiIndex == 1) {
        pWheel->Set(iUT_HighlightedText, "", true);
    }
    else {
        const char* pcText = pHotkeyTilesA[aiIndex]->GetString(4036);
        pWheel->Set(iUT_HighlightedText, pcText, true);
    }
    if (aiIndex >= 0) {
        iLastHighlightedHotkey = aiIndex;
        bSelectingHotkey = true;
    }
}

bool HotKeysWheel::IsDPadPressed() {
    return Interface::Has360Controller() &&
        (InterfaceManager::CheckMenuButton(Interface::eButtonDPadDown, false) != -1 ||
         InterfaceManager::CheckMenuButton(Interface::eButtonDPadUp, false) != -1 ||
         InterfaceManager::CheckMenuButton(Interface::eButtonDPadLeft, false) != -1 ||
         InterfaceManager::CheckMenuButton(Interface::eButtonDPadRight, false) != -1);
}
