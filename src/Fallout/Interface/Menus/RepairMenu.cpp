#include "Fallout/Interface/Menus/RepairMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/ContainerMenu.h"
#include "Fallout/Interface/Menus/InventoryMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

RepairMenu::RepairMenu() {
    RepairMenu::iUT_Title = Tile::TextToTrait("_Title");
    RepairMenu::iUT_Value = Tile::TextToTrait("_Value");
    RepairMenu::iUT_enabled = Tile::TextToTrait("_enabled");
    RepairMenu::iUT_IsRepairItem = Tile::TextToTrait("_IsRepairItem");
}

RepairMenu::~RepairMenu() {
    pTiles[8]->RemoveFadeControl(4009);
    RepairMenu::pMe = nullptr;
}

void RepairMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 12) {
        pTiles[aiID] = apTile;
    }
}

void RepairMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 13) {
        apTarget = pTiles[aiID];
    }
    if (aiID == 12) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
        Interface::SetInventoryMenuVisible(true, false, false);
    }
    else if (aiID == 14 && !(apTarget->xChildren.GetTail()->GetFloat(4009) < 255.0)) {
        ItemChange* pHighlighted = lbRepairList.GetHighlightedData();
        if (pHighlighted != RepairMenu::pBrokenItem && pTiles[9]->IsTrue(4003)) {
            ItemChange* psparePart = lbRepairList.GetHighlightedData();
            ExtraDataList* pspareExtra = psparePart->pExtraObjectList ? psparePart->pExtraObjectList->m_item : nullptr;
            RepairItem(psparePart);
            pPlayer->RemoveItem(
                psparePart->pContainerObj,
                pspareExtra,
                1,
                false,
                false,
                nullptr,
                nullptr,
                nullptr,
                true,
                false
            );
            BSAudio::QInstance().GetSoundHandleByName("UIRepairWeapon", 289u).Play(false);
            Tile* pItemTile = static_cast<Tile*>(lbRepairList.GetHighlightedItem());
            lbRepairList.RemoveItem(pItemTile);
            if (lbRepairList.NumUnfiltered() == 1 || RepairMenu::pBrokenItem->GetItemHealth(true) >= 99.0) {
                DoClick(12, nullptr);
            }
            else {
                RepairMenu::InitRepairMenu(nullptr);
            }
            if (psparePart->GetWorn(false)) {
                pPlayer->ResetArmorRating();
                InventoryMenu::UpdateList();
            }
        }
    }
}

void RepairMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    static int32_t iprevHighlightIndex;
    if (aiID == 14) {
        int32_t inewHighlightIndex;
        ItemChange* pcurrItem = lbRepairList.GetHighlightedData();
        if (pcurrItem) {
            Tile* pItemTile = static_cast<Tile*>(lbRepairList.GetHighlightedItem());
            inewHighlightIndex = static_cast<int32_t>(pItemTile->GetFloat(4012));
        }
        else {
            inewHighlightIndex = 0;
        }
        if (iprevHighlightIndex != inewHighlightIndex) {
            float fKnobStep = iprevHighlightIndex >= inewHighlightIndex ? -fScrollKnobIncrement.Float()
                                                                        : fScrollKnobIncrement.Float();
            float fKnobRate = fScrollKnobRate.Float();
            InterfaceManager::Instance()
                ->GetPipboy()
                ->MoveKnob(FOPipboyManager::KNOB_SCROLL, fKnobStep, fKnobRate, true);
            iprevHighlightIndex = inewHighlightIndex;
        }
        if (pcurrItem && RepairMenu::pBrokenItem) {
            const char* pButtonText;
            float fMaintainValue = 75.0f;
            if (pcurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                fMaintainValue = 50.0f;
            }
            if (static_cast<double>(fMaintainValue) <= pcurrItem->GetItemHealth(true)) {
                pButtonText = sMaintainItem.String();
            }
            else {
                pButtonText = sRepairItem.String();
            }
            RepairMenu::pMe->pTiles[11]->Set(4036, pButtonText, true);
            if (pcurrItem == RepairMenu::pBrokenItem) {
                if (lbRepairList.NumUnfiltered() > 1) {
                    ShowErrorMessage(sSelectItemToRepair.String());
                }
                int32_t iLineAlpha = Tile::TextToTrait("_line_alpha");
                pTiles[11]->Set(iLineAlpha, 128.0f, true);
            }
            else {
                char acText[68];
                float fnewHealthPercent = apTarget->GetFloat(Tile::kTileValue_User0);
                int32_t iminSkillNeeded = static_cast<int32_t>(apTarget->GetFloat(Tile::kTileValue_User1));
                if (iminSkillNeeded <= 100) {
                    if (iminSkillNeeded <= 0) {
                        int32_t iLineAlpha = Tile::TextToTrait("_line_alpha");
                        pTiles[11]->Set(iLineAlpha, 255.0f, true);
                        pTiles[9]->Set(Tile::kTileValue_User0, fnewHealthPercent, true);
                        pTiles[9]->Set(4003, 1);
                        pTiles[8]->Set(4003, 0);
                        if (RepairMenu::pBrokenItem->pContainerObj->GetFormType() == WEAP_ID) {
                            const char* pTitle = sInventoryDamage.String();
                            pTiles[10]->Set(RepairMenu::iUT_Title, pTitle, true);
                            float fDamage = CombatFormulas::CalcWeaponDamageForDisplay(
                                static_cast<TESObjectWEAP*>(RepairMenu::pBrokenItem->pContainerObj),
                                fnewHealthPercent,
                                RepairMenu::pBrokenItem,
                                nullptr
                            );
                            double dRoundedDamage = BSRoundToNearest(fDamage, 1.0f);
                            pTiles[10]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(dRoundedDamage));
                        }
                        else if (pcurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                            char acValue[132];
                            TESObjectARMO* pBrokenArmor = static_cast<TESObjectARMO*>(
                                RepairMenu::pBrokenItem->pContainerObj
                            );
                            float foldHealthPercent = RepairMenu::pBrokenItem->GetItemHealth(true) / 100.0;
                            double dArmorRating = pBrokenArmor->GetArmorRating();
                            float fBrokenDR = CombatFormulas::CalcArmorRating(
                                static_cast<int32_t>(dArmorRating),
                                foldHealthPercent
                            );
                            double dDamageThreshold = pBrokenArmor->GetDamageThreshold();
                            float fBrokenDT = CombatFormulas::CalcArmorRating(
                                static_cast<int32_t>(dDamageThreshold),
                                foldHealthPercent
                            );
                            dArmorRating = pBrokenArmor->GetArmorRating();
                            float fRepairedDR = CombatFormulas::CalcArmorRating(
                                static_cast<int32_t>(dArmorRating),
                                fnewHealthPercent
                            );
                            dDamageThreshold = pBrokenArmor->GetDamageThreshold();
                            float fRepairedDT = CombatFormulas::CalcArmorRating(
                                static_cast<int32_t>(dDamageThreshold),
                                fnewHealthPercent
                            );
                            BSstrcpy(acValue, 128, "--");
                            if (fRepairedDT > 0.0) {
                                const char* pTitle = sInventoryDamageThreshold.String();
                                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
                                if (fBrokenDT > 0.0) {
                                    RepairMenu::pMe->pTiles[5]->Set(
                                        RepairMenu::iUT_Value,
                                        static_cast<int32_t>(fBrokenDT)
                                    );
                                }
                                else {
                                    RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, acValue, true);
                                }
                                pTitle = sInventoryDamageThreshold.String();
                                RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Title, pTitle, true);
                                RepairMenu::pMe->pTiles[10]->Set(
                                    RepairMenu::iUT_Value,
                                    static_cast<int32_t>(fRepairedDT)
                                );
                            }
                            else if (fRepairedDR > 0.0) {
                                const char* pTitle = sInventoryDamageResistance.String();
                                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
                                if (fBrokenDR > 0.0) {
                                    RepairMenu::pMe->pTiles[5]->Set(
                                        RepairMenu::iUT_Value,
                                        static_cast<int32_t>(fBrokenDR)
                                    );
                                }
                                else {
                                    RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, acValue, true);
                                }
                                pTitle = sInventoryDamageResistance.String();
                                RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Title, pTitle, true);
                                RepairMenu::pMe->pTiles[10]->Set(
                                    RepairMenu::iUT_Value,
                                    static_cast<int32_t>(fRepairedDR)
                                );
                            }
                            else {
                                const char* pTitle = sInventoryDamageResistance.String();
                                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
                                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, acValue, true);
                                pTitle = sInventoryDamageResistance.String();
                                RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Title, pTitle, true);
                                RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Value, acValue, true);
                            }
                        }
                        double dRepairedHealth = pTiles[9]->GetFloat(Tile::kTileValue_User0);
                        float fhealthImprovement = (dRepairedHealth - pTiles[4]->GetFloat(Tile::kTileValue_User0)) *
                            100.0;
                        double dRepairedStat = pTiles[10]->GetFloat(RepairMenu::iUT_Value);
                        float fstatImprovement = dRepairedStat - pTiles[5]->GetFloat(RepairMenu::iUT_Value);
                        fhealthImprovement = BSMAX(0.0f, fhealthImprovement);
                        fstatImprovement = BSMAX(0.0f, fstatImprovement);
                        BSsprintf(acText, 64, "+%i", static_cast<int32_t>(fhealthImprovement + 0.5));
                        BSstrcat(acText, 64, "%");
                        pTiles[6]->Set(4036, acText, true);
                        BSsprintf(acText, 64, "+%i", static_cast<int32_t>(fstatImprovement + 0.5));
                        pTiles[7]->Set(4036, acText, true);
                    }
                    else {
                        BSsprintf(acText, 64, sRepairSkillTooLow.String(), iminSkillNeeded, "%");
                        ShowErrorMessage(acText);
                        int32_t iLineAlpha = Tile::TextToTrait("_line_alpha");
                        pTiles[11]->Set(iLineAlpha, 128.0f, true);
                    }
                }
                else {
                    double dMaxSkill = fRepairSkillMax.Float() * 10.0;
                    BSsprintf(acText, 64, sCantRepairPastMax.String(), dMaxSkill, "%");
                    ShowErrorMessage(acText);
                    int32_t iLineAlpha = Tile::TextToTrait("_line_alpha");
                    pTiles[11]->Set(iLineAlpha, 128.0f, true);
                }
            }
        }
    }
}

// Leaving the repair list hides the item tile, and the second tile too when more than one item is listed.
void RepairMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    if (aiID == 14) {
        pTiles[9]->Set(Tile::kTileValue_Visible, 0);
        if (lbRepairList.NumUnfiltered() > 1) {
            pTiles[8]->Set(Tile::kTileValue_Visible, 0);
        }
    }
}

int32_t RepairMenu::GetClass() {
    return 1035;
}

bool RepairMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (aiButton == 6 || aiButton == 10) {
        Interface::PlayMenuSound(3);
        DoClick(12, nullptr);
    }
    return false;
}

void RepairMenu::ShowErrorMessage(const char* astrMessage) {
    pTiles[8]->Set(4036, astrMessage, true);
    pTiles[9]->Set(4003, 0);
    pTiles[8]->Set(4003, 1);
    if (!pTiles[8]->HasFadeControl(4009)) {
        pTiles[8]->AddFadeControl(4009, 255.0f, 0x1p+6f, 0x1p+1f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
    }
}

bool RepairMenu::IsWellFormedMenu() {
    int32_t i;
    for (i = 0; i < 13; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

void RepairMenu::RepairItem(ItemChange* apSparePart) {
    float fSpareCondition = apSparePart->GetItemHealth(true);
    float fBrokenCondition = RepairMenu::pBrokenItem->GetItemHealth(true);
    int32_t iRepairSkill = pPlayer->GetClampedActorValue(ActorValue::REPAIR);
    float
        fnewHealth = GamePlayFormulas::GetRepairedCondition(iRepairSkill, fBrokenCondition, fSpareCondition, nullptr) /
        100.0;
    fnewHealth = BSMIN(fnewHealth, 1.0f);
    TESObjectWEAP* pWeap = dynamic_cast<TESObjectWEAP*>(RepairMenu::pBrokenItem->pContainerObj);
    if (pWeap) {
        bool bHealthMod = RepairMenu::pBrokenItem->HasModEffectActive(10);
        fnewHealth = static_cast<double>(pWeap->GetFormHealth(bHealthMod)) * fnewHealth;
    }
    else {
        fnewHealth = static_cast<double>(TESHealthForm::GetFormHealth(RepairMenu::pBrokenItem->pContainerObj)) *
            fnewHealth;
    }
    TESBoundObject* pBrokenObject = RepairMenu::pBrokenItem->pContainerObj;
    InventoryChanges* pInventory = InventoryChanges::GetInventoryChanges(pPlayer);
    ItemChange* pitemGroup = pInventory->GetObjectInList(pBrokenObject, true, 0);
    ExtraDataList* pbrokenExtra = RepairMenu::pBrokenItem->pExtraObjectList
        ? RepairMenu::pBrokenItem->pExtraObjectList->m_item
        : nullptr;
    ExtraDataList* prepairedExtra = new ExtraDataList;
    bool bfoundMatchingExtra = false;
    int32_t iorigStackCount = pbrokenExtra->GetCount();
    prepairedExtra->DuplicateExtraListForContainer(pbrokenExtra);
    prepairedExtra->SetCount(1);
    prepairedExtra->SetHealth(fnewHealth);
    MiscStatManager::Increment(MiscStatManager::MS_ITEMS_REPAIRED);
    RepairMenu::pBrokenItem->pExtraObjectList->RemoveHead();
    if (iorigStackCount > 1) {
        pbrokenExtra->SetCount(iorigStackCount - 1);
    }
    for (BSSimpleList<ExtraDataList*>* pcurrExtraObj = pitemGroup->pExtraObjectList;
         pcurrExtraObj && pcurrExtraObj->m_item;
         pcurrExtraObj = pcurrExtraObj->m_pkNext) {
        if (!pcurrExtraObj->m_item->CompareListForContainer(prepairedExtra, true, false) &&
            !prepairedExtra->CompareListForContainer(pcurrExtraObj->m_item, true, false)) {
            int16_t sCount = pcurrExtraObj->m_item->GetCount();
            pcurrExtraObj->m_item->SetCount(sCount + 1);
            RepairMenu::pBrokenItem->pExtraObjectList->AddHead(pcurrExtraObj->m_item);
            bfoundMatchingExtra = true;
        }
    }
    if (bfoundMatchingExtra) {
        if (iorigStackCount == 1) {
            pitemGroup->pExtraObjectList->Remove(pbrokenExtra);
        }
    }
    else if (iorigStackCount <= 1) {
        pbrokenExtra->SetHealth(fnewHealth);
        RepairMenu::pBrokenItem->pExtraObjectList->AddHead(pbrokenExtra);
    }
    else {
        pitemGroup->pExtraObjectList->AddHead(prepairedExtra);
        RepairMenu::pBrokenItem->pExtraObjectList->AddHead(prepairedExtra);
    }
    if (bfoundMatchingExtra || iorigStackCount == 1) {
        prepairedExtra->SetScriptLocals(nullptr);
        if (prepairedExtra) {
            delete prepairedExtra;
        }
    }
}

Tile* RepairMenu::Create() {
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    Tile* pOldMenu = Menu::GetMenuByClass(RepairMenu::GetClassStatic());
    if (pOldMenu) {
        delete pOldMenu;
    }
    TileMenu* pMenuTile = static_cast<TileMenu*>(
        pInterfaceManager->pMenusRoot->ReadFile("Data\\Menus\\repair_menu.xml")
    );
    RepairMenu::pMe = static_cast<RepairMenu*>(pMenuTile->GetMenu());
    if (RepairMenu::pMe && RepairMenu::pMe->GetClass() == RepairMenu::GetClassStatic()) {
        RepairMenu::pMe->SetMenuTile(pMenuTile, false);
        RepairMenu::pMe->SetRenderedMenu();
        if (RepairMenu::pMe->IsWellFormedMenu()) {
            if (pMenuTile->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
                pMenuTile->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
                float fMaxDepth = Tile::GetMaximumDepth();
                pMenuTile->Set(Tile::kTileValue_Depth, fMaxDepth, true);
            }
            RepairMenu::pMe->lbRepairList.Initialize(RepairMenu::pMe->pTiles[1], "RM_RepairListTemplate");
            RepairMenu::pMe->lbRepairList.SetAutoUpdate(false);
            RepairMenu::pMe->pTiles[5]->Poke(RepairMenu::iUT_Value, 1.0f);
            RepairMenu::pMe->pTiles[10]->Poke(RepairMenu::iUT_Value, 1.0f);
            RepairMenu::pMe->PrepForVisibility(true);
            return pMenuTile;
        }

        Warning("MENUS: Repair Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }

    if (RepairMenu::pMe && RepairMenu::pMe->pMenu && RepairMenu::pMe) {
        delete RepairMenu::pMe;
    }

    return nullptr;
}

int32_t RepairMenu::GetClassStatic() {
    return 1035;
}

void RepairMenu::InitRepairMenu(ItemChange* apBrokenItem) {
    if (RepairMenu::pMe) {
        if (apBrokenItem) {
            RepairMenu::pBrokenItem = apBrokenItem;
        }
        float fcondition = RepairMenu::pBrokenItem->GetItemHealth(true) / 100.0;
        RepairMenu::pMe->pTiles[4]->Set(Tile::kTileValue_User0, fcondition, true);
        RepairMenu::pMe->pTiles[4]->Set(4003, 1);
        TESForm* pBrokenArmor = RepairMenu::pBrokenItem->pContainerObj;
        int32_t iFormType = pBrokenArmor->GetFormType();
        if (iFormType == 40) {
            const char* pTitle = sInventoryDamage.String();
            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
            float fDamage = CombatFormulas::CalcWeaponDamageForDisplay(
                static_cast<TESObjectWEAP*>(pBrokenArmor),
                fcondition,
                apBrokenItem,
                nullptr
            );
            double dRoundedDamage = BSRoundToNearest(fDamage, 1.0f);
            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(dRoundedDamage));
        }
        else if (iFormType == 24) {
            double dArmorRating = static_cast<TESObjectARMO*>(pBrokenArmor)->TESObjectARMO::GetArmorRating();
            float fBrokenDR = CombatFormulas::CalcArmorRating(static_cast<int32_t>(dArmorRating), fcondition);
            double dDamageThreshold = static_cast<TESObjectARMO*>(pBrokenArmor)->TESObjectARMO::GetDamageThreshold();
            float fBrokenDT = CombatFormulas::CalcArmorRating(static_cast<int32_t>(dDamageThreshold), fcondition);
            if (fBrokenDT > 0.0) {
                const char* pTitle = sInventoryDamageThreshold.String();
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(fBrokenDT));
            }
            else if (fBrokenDR > 0.0) {
                const char* pTitle = sInventoryDamageResistance.String();
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(fBrokenDR));
            }
            else {
                char acValue[132];
                BSstrcpy(acValue, 128, "--");
                const char* pTitle = sInventoryDamageResistance.String();
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, acValue, true);
            }
        }
        else {
            char acValue[128];
            BSstrcpy(acValue, 128, "--");
            const char* pTitle = sInventoryDamageResistance.String();
            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, pTitle, true);
            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, acValue, true);
        }
        if (apBrokenItem) {
            const char* pIcon = RepairMenu::pBrokenItem->GetIconOne(nullptr);
            RepairMenu::pMe->pTiles[3]->Set(4044, pIcon, true);
            RepairMenu::Update();
        }
        RepairMenu::pMe->lbRepairList.Traverse(&RepairMenu::InitFunc, 0, std::numeric_limits<int32_t>::max());
        RepairMenu::pMe->lbRepairList.Sort(&RepairMenu::SortFunc);
        int32_t iRepairSkill = pPlayer->GetClampedActorValue(ActorValue::REPAIR);
        RepairMenu::pMe->pTiles[0]->Set(RepairMenu::iUT_Value, iRepairSkill);
        if (apBrokenItem && InterfaceManager::Has360Controller()) {
            Tile* pFirstTile = static_cast<ListBoxBase*>(&RepairMenu::pMe->lbRepairList)->GetTileAt(1u, false);
            InterfaceManager::Instance()->SetCurrentFocusTarget(pFirstTile, 4035, true);
        }

        Tile* pHighlighted = static_cast<Tile*>(
            static_cast<ListBoxBase*>(&RepairMenu::pMe->lbRepairList)->GetHighlightedItem()
        );
        RepairMenu::pMe->DoEnter(14, pHighlighted);
        if (apBrokenItem) {
            Interface::SetRepairMenuVisible(true, false, false);
            int32_t iClass = RepairMenu::GetClassStatic();
            InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_REPAIR, iClass, 500u);
        }
    }
}

// RemoveAll is virtual and is called through a pointer so that the call stays indirect.
void RepairMenu::ResetMenu() {
    RepairMenu::pMe->lbRepairList.RemoveAll();
    RepairMenu::pBrokenItem = nullptr;
    RepairMenu::pMe->DoLeave(14, nullptr);
    RepairMenu::pMe->pTiles[8]->RemoveFadeControl(4009);
    RepairMenu::pMe->pTiles[8]->Set(4003, 0);
}

void RepairMenu::InitFunc(Tile* apTile, ItemChange* apItem) {
    int32_t iMinSkillNeeded;
    if (apTile && apItem) {
        float fItemHealth = apItem->GetItemHealth(true);
        apTile->Set(4010, 14);
        int32_t iVisible = apTile->GetFloat(4012) >= 0.0;
        apTile->Set(4003, iVisible);
        float fBarWidth = fItemHealth * 0.6000000238418579;
        apTile->GetChildByID(15)->Set(4017, fBarWidth, true);
        if (apItem->GetWorn(false)) {
            apTile->xChildren.Get(apTile->xChildren.m_pkHead)->Set(4003, 1);
        }
        float fBrokenHealth = RepairMenu::pBrokenItem->GetItemHealth(true);
        float fRepairedCondition = 0.0f;
        iMinSkillNeeded = -1;
        int32_t iRepairSkill = pPlayer->GetClampedActorValue(ActorValue::REPAIR);
        fRepairedCondition = GamePlayFormulas::GetRepairedCondition(
                                 iRepairSkill,
                                 fBrokenHealth,
                                 fItemHealth,
                                 &iMinSkillNeeded
                             ) /
            100.0;
        fRepairedCondition = BSMIN(fRepairedCondition, 1.0f);
        apTile->Set(Tile::kTileValue_User0, fRepairedCondition, true);
    }
}

int32_t RepairMenu::SortFunc(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2) {
    int32_t iresult;
    bool bisRepairItem1 = apItem1->pTile->IsTrue(RepairMenu::iUT_IsRepairItem);
    bool bisRepairItem2 = apItem2->pTile->IsTrue(RepairMenu::iUT_IsRepairItem);
    bool bisGreyedOut1 = apItem1->pTile->xChildren.GetTail()->GetFloat(4009) < 255.0;
    bool bisGreyedOut2 = apItem2->pTile->xChildren.GetTail()->GetFloat(4009) < 255.0;
    if (bisRepairItem1 && !bisRepairItem2) {
        return -1;
    }
    if (!bisRepairItem1 && bisRepairItem2) {
        return 1;
    }
    if (bisGreyedOut1 && !bisGreyedOut2) {
        return 1;
    }
    if (!bisGreyedOut1 && bisGreyedOut2) {
        return -1;
    }
    double dItemHealth = apItem1->data->GetItemHealth(true);
    if (apItem2->data->GetItemHealth(true) > dItemHealth) {
        iresult = 1;
    }
    else {
        dItemHealth = apItem1->data->GetItemHealth(true);
        iresult = apItem2->data->GetItemHealth(true) < dItemHealth ? -1 : 0;
    }
    if (bisGreyedOut1 && bisGreyedOut2) {
        iresult = -iresult;
    }
    if (!iresult) {
        if (apItem1->data->GetWorn(false)) {
            return -1;
        }

        return apItem2->data->GetWorn(false);
    }
    return iresult;
}

void RepairMenu::Update() {
    if (RepairMenu::pBrokenItem) {
        BSSimpleList<ItemChange*> pnewItemGroup;
        InventoryChanges::GetInventoryChanges(pPlayer)->GetRepairItemGroup(
            RepairMenu::pBrokenItem->pContainerObj,
            &pnewItemGroup
        );
        for (ItemChange*& rpItem : pnewItemGroup.ItemsUntilNull()) {
            for (int32_t iitemCt = 0; iitemCt < rpItem->iNumber; ++iitemCt) {
                float fcondition = rpItem->GetItemHealth(true);
                float fCeilcondition = FloatCeil(fcondition);
                char cHasMods = 0;
                ExtraDataList* pExtra = rpItem->pExtraObjectList ? rpItem->pExtraObjectList->m_item : nullptr;
                if (pExtra && pExtra->HasWeaponMods()) {
                    cHasMods = 1;
                }

                if (!(fCeilcondition < 100.0)) {
                    if (rpItem->pContainerObj == RepairMenu::pBrokenItem->pContainerObj) {
                        continue;
                    }
                }
                if (iitemCt) {
                    ItemChange* pnewItem = new ItemChange(rpItem->pContainerObj, 1);
                    for (ExtraDataList*& rpExtra : rpItem->pExtraObjectList->ItemsUntilNull()) {
                        pnewItem->pExtraObjectList->AddHead(rpExtra);
                    }
                    const char* pName = pnewItem->GetFullName();
                    RepairMenu::pMe->lbRepairList.AddItem(pnewItem, pName, nullptr, nullptr);
                }
                else {
                    ItemChange* pnewItem = rpItem;
                    const char* pName = pnewItem->GetFullName();
                    Tile* pnewTile = RepairMenu::pMe->lbRepairList.AddItem(pnewItem, pName, nullptr, nullptr);
                    if (pnewTile) {
                        if (pnewItem->pContainerObj == RepairMenu::pBrokenItem->pContainerObj) {
                            ExtraDataList* pNewExtra = pnewItem->pExtraObjectList ? pnewItem->pExtraObjectList->m_item
                                                                                  : nullptr;
                            ExtraDataList* pBrokenExtra = nullptr;
                            if (RepairMenu::pBrokenItem->pExtraObjectList) {
                                pBrokenExtra = RepairMenu::pBrokenItem->pExtraObjectList->m_item;
                            }
                            if (pNewExtra == pBrokenExtra) {
                                RepairMenu::pBrokenItem = pnewItem;
                                pnewTile->Set(RepairMenu::iUT_IsRepairItem, 1);
                                Tile::Value* pheight = RepairMenu::pMe->pTiles[2]->GetValue(4016);
                                Tile::Value* py = RepairMenu::pMe->pTiles[2]->GetValue(4002);
                                pheight->ClearActions();
                                pheight->AddAction(2000, pnewTile, 4016);
                                pheight->AddAction(2002, 1.0f);
                                pheight->CalculateValue(false);
                                py->ClearActions();
                                py->AddAction(2000, pnewTile, 4002);
                                py->CalculateValue(false);
                            }
                            else if (cHasMods || pnewItem->GetWorn(false)) {
                                RepairMenu::pMe->lbRepairList.RemoveItem(pnewTile);
                            }
                        }
                    }
                }
            }
        }
        if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
            const char* pTitle = sInventoryItems.String();
            RepairMenu::pMe->pMenu->GetChildByName("RM_Headline_Title")->Set(4036, pTitle, true);
            pTitle = sRepairSkill.String();
            RepairMenu::pMe->pTiles[0]->Set(RepairMenu::iUT_Title, pTitle, true);
            pTitle = sInventoryCondition.String();
            RepairMenu::pMe->pTiles[4]->Set(RepairMenu::iUT_Title, pTitle, true);
            pTitle = sInventoryCondition.String();
            RepairMenu::pMe->pTiles[9]->Set(RepairMenu::iUT_Title, pTitle, true);
            pTitle = sRepairItem.String();
            RepairMenu::pMe->pTiles[11]->Set(4036, pTitle, true);
            pTitle = sCancel.String();
            RepairMenu::pMe->pTiles[12]->Set(4036, pTitle, true);
        }
    }
}
