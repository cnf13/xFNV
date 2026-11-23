#include "Fallout/Interface/Menus/RepairServicesMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESContainer.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESHealthForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

RepairServicesMenu::RepairServicesMenu() : iRepairAllCost(0) {
    g_repairServicesMenu_trait_Title = Tile::TextToTrait("_Title");
    RepairServicesMenu::iUT_Value = Tile::TextToTrait("_Value");
    g_repairServicesMenu_trait_RepairCost = Tile::TextToTrait("_RepairCost");
    g_repairServicesMenu_trait_CanRepair = Tile::TextToTrait("_CanRepair");
}

RepairServicesMenu::~RepairServicesMenu() {
    RepairServicesMenu::pMe = nullptr;
}

void RepairServicesMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (aiID >= 0 && aiID < 16) {
        pTiles[aiID] = apTile;
    }
}

void RepairServicesMenu::DoClick(int32_t aiID, Tile* apTarget) {
    if (!apTarget && aiID >= 0 && aiID < 16) {
        apTarget = pTiles[aiID];
    }
    switch (aiID) {
        case 14: {
            lbRepairList.Traverse(&RepairServicesMenu::RepairAllFunc, 0, std::numeric_limits<int32_t>::max());
            pPlayer->PayGoldToActor(RepairServicesMenu::pRepairVendor, iRepairAllCost);
            BSAudio::QInstance().GetSoundHandleByName("UIRepairWeapon", 289u).Play(false);
            InitEntries();
            break;
        }
        case 15: {
            RepairServicesMenu::Close();
            break;
        }
        case 17: {
            if (apTarget->IsTrue(g_repairServicesMenu_trait_CanRepair)) {
                RepairItem(lbRepairList.GetHighlightedData());
                double dRepairCost = apTarget->GetFloat(g_repairServicesMenu_trait_RepairCost);
                pPlayer->PayGoldToActor(RepairServicesMenu::pRepairVendor, static_cast<int32_t>(dRepairCost));
                BSAudio::QInstance().GetSoundHandleByName("UIRepairWeapon", 289u).Play(false);
                InitEntries();
            }
            else {
                BSAudio::QInstance().GetSoundHandleByName("UIVATSInsufficientAP", 289u).Play(false);
            }
            break;
        }
    }
}

void RepairServicesMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    if (aiID == 17) {
        char acBuffer[68];
        ItemChange* pCurrItem = lbRepairList.GetHighlightedData();
        bool bCanRepair = apTarget->IsTrue(g_repairServicesMenu_trait_CanRepair);
        pTiles[13]->Set(4015, static_cast<int32_t>(bCanRepair));
        pTiles[4]->Set(4003, 1);
        const char* pIconOne = pCurrItem->GetIconOne(nullptr);
        pTiles[3]->Set(4044, pIconOne, true);
        pTiles[3]->Set(4003, 1);
        float fOldHealthPercent = pCurrItem->GetItemHealth(true) / 100.0;
        pTiles[5]->Set(Tile::kTileValue_User0, fOldHealthPercent, true);
        if (pCurrItem->pContainerObj->GetFormType() == WEAP_ID) {
            const char* pTitle = sInventoryDamage.String();
            pTiles[6]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
            float fDisplayDamage = CombatFormulas::CalcWeaponDamageForDisplay(
                static_cast<TESObjectWEAP*>(pCurrItem->pContainerObj),
                fOldHealthPercent,
                pCurrItem,
                nullptr
            );
            double dRoundedDamage = BSRoundToNearest(fDisplayDamage, 1.0f);
            pTiles[6]->Set(RepairServicesMenu::iUT_Value, static_cast<int32_t>(dRoundedDamage));
        }
        else if (pCurrItem->pContainerObj->GetFormType() == ARMO_ID) {
            double dBaseArmorRating = static_cast<TESObjectARMO*>(pCurrItem->pContainerObj)
                                          ->TESObjectARMO::GetArmorRating();
            float fArmorRating = CombatFormulas::CalcArmorRating(
                static_cast<int32_t>(dBaseArmorRating),
                fOldHealthPercent
            );
            double dBaseDamageThreshold = static_cast<TESObjectARMO*>(pCurrItem->pContainerObj)
                                              ->TESObjectARMO::GetDamageThreshold();
            float fDamageThreshold = CombatFormulas::CalcArmorRating(
                static_cast<int32_t>(dBaseDamageThreshold),
                fOldHealthPercent
            );
            if (fDamageThreshold > 0.0) {
                const char* pTitle = sInventoryDamageThreshold.String();
                RepairServicesMenu::pMe->pTiles[6]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
                RepairServicesMenu::pMe->pTiles[6]->Set(
                    RepairServicesMenu::iUT_Value,
                    static_cast<int32_t>(fDamageThreshold)
                );
            }
            else if (fArmorRating > 0.0) {
                const char* pTitle = sInventoryDamageResistance.String();
                RepairServicesMenu::pMe->pTiles[6]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
                RepairServicesMenu::pMe->pTiles[6]->Set(
                    RepairServicesMenu::iUT_Value,
                    static_cast<int32_t>(fArmorRating)
                );
            }
            else {
                char acText[132];
                BSstrcpy(acText, 128, "--");
                const char* pTitle = sInventoryDamageResistance.String();
                RepairServicesMenu::pMe->pTiles[6]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
                RepairServicesMenu::pMe->pTiles[6]->Set(RepairServicesMenu::iUT_Value, acText, true);
            }
        }
        float fNewHealth = (fRepairSkillMax.Float() - fRepairSkillBase.Float()) *
                (static_cast<double>(iVendorRepairSkill) / 100.0) +
            fRepairSkillBase.Float();
        float fSkillPercent = fNewHealth / 10.0;
        float fNewHealthPercent = BSMIN(fSkillPercent, 1.0f);
        double dRoundedNewPercent = BSRoundToNearest(fNewHealthPercent, 0.001f);
        if (!(BSRoundToNearest(fOldHealthPercent, 0.001f) >= dRoundedNewPercent)) {
            pTiles[10]->Set(Tile::kTileValue_User0, fNewHealthPercent, true);
            pTiles[10]->Set(4003, 1);
            if (pCurrItem->pContainerObj->GetFormType() == WEAP_ID) {
                const char* pTitle = sInventoryDamage.String();
                pTiles[11]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
                float fDisplayDamage = CombatFormulas::CalcWeaponDamageForDisplay(
                    static_cast<TESObjectWEAP*>(pCurrItem->pContainerObj),
                    fNewHealthPercent,
                    pCurrItem,
                    nullptr
                );
                double dRoundedDamage = BSRoundToNearest(fDisplayDamage, 1.0f);
                pTiles[11]->Set(RepairServicesMenu::iUT_Value, static_cast<int32_t>(dRoundedDamage));
            }
            else if (pCurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                double dBaseArmorRating = static_cast<TESObjectARMO*>(pCurrItem->pContainerObj)
                                              ->TESObjectARMO::GetArmorRating();
                float fArmorRating = CombatFormulas::CalcArmorRating(
                    static_cast<int32_t>(dBaseArmorRating),
                    fNewHealthPercent
                );
                double dBaseDamageThreshold = static_cast<TESObjectARMO*>(pCurrItem->pContainerObj)
                                                  ->TESObjectARMO::GetDamageThreshold();
                float fDamageThreshold = CombatFormulas::CalcArmorRating(
                    static_cast<int32_t>(dBaseDamageThreshold),
                    fNewHealthPercent
                );
                if (fDamageThreshold > 0.0) {
                    const char* pTitle = sInventoryDamageThreshold.String();
                    RepairServicesMenu::pMe->pTiles[11]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
                    RepairServicesMenu::pMe->pTiles[11]->Set(
                        RepairServicesMenu::iUT_Value,
                        static_cast<int32_t>(fDamageThreshold)
                    );
                }
                else if (fArmorRating > 0.0) {
                    const char* pTitle = sInventoryDamageResistance.String();
                    RepairServicesMenu::pMe->pTiles[11]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
                    RepairServicesMenu::pMe->pTiles[11]->Set(
                        RepairServicesMenu::iUT_Value,
                        static_cast<int32_t>(fArmorRating)
                    );
                }
                else {
                    char acText[128];
                    BSstrcpy(acText, 128, "--");
                    const char* pTitle = sInventoryDamageResistance.String();
                    RepairServicesMenu::pMe->pTiles[11]->Set(g_repairServicesMenu_trait_Title, pTitle, true);
                    RepairServicesMenu::pMe->pTiles[11]->Set(RepairServicesMenu::iUT_Value, acText, true);
                }
            }
            double dNewCondition = pTiles[10]->GetFloat(Tile::kTileValue_User0);
            float fHealthImprovement = (dNewCondition - pTiles[5]->GetFloat(Tile::kTileValue_User0)) * 100.0;
            double dNewStat = pTiles[11]->GetFloat(RepairServicesMenu::iUT_Value);
            float fStatImprovement = dNewStat - pTiles[6]->GetFloat(RepairServicesMenu::iUT_Value);
            fHealthImprovement = BSMAX(0.0f, fHealthImprovement);
            fStatImprovement = BSMAX(0.0f, fStatImprovement);
            BSsprintf(acBuffer, 64, "+%i", static_cast<int32_t>(fHealthImprovement + 0.5));
            BSstrcat(acBuffer, 64, "%");
            pTiles[7]->Set(4036, acBuffer, true);
            BSsprintf(acBuffer, 64, "+%i", static_cast<int32_t>(fStatImprovement + 0.5));
            pTiles[8]->Set(4036, acBuffer, true);
            int32_t iRepairCost = static_cast<int32_t>(apTarget->GetFloat(g_repairServicesMenu_trait_RepairCost));
            BSsprintf(acBuffer, 64, sRepairCost.String(), iRepairCost);
            pTiles[12]->Set(4036, acBuffer, true);
            if (apTarget->IsTrue(g_repairServicesMenu_trait_CanRepair)) {
                pTiles[12]->Set(4009, 255.0f, true);
            }
            else {
                pTiles[12]->Set(4009, 128.0f, true);
            }
        }
        else {
            BSsprintf(acBuffer, 64, sCantRepairPastMax.String(), fNewHealthPercent * 100.0, "%");
            pTiles[9]->Set(4036, acBuffer, true);
            pTiles[10]->Set(4003, 0);
        }
    }
}

void RepairServicesMenu::DoLeave(int32_t aiID, Tile* apTarget) {
    if (aiID == 17) {
        pTiles[13]->Set(4015, 1);
        pTiles[3]->Set(4003, 0);
        pTiles[4]->Set(4003, 0);
    }
}

int32_t RepairServicesMenu::GetClass() {
    return 1058;
}

void RepairServicesMenu::RepairItem(ItemChange* apBrokenItem) {
    float fNewHealth = (fRepairSkillMax.Float() - fRepairSkillBase.Float()) *
            (static_cast<double>(iVendorRepairSkill) / 100.0) +
        fRepairSkillBase.Float();
    float fSkillPercent = fNewHealth / 10.0;
    fNewHealth = BSMIN(fSkillPercent, 1.0f);
    TESObjectWEAP* pWeapon = dynamic_cast<TESObjectWEAP*>(apBrokenItem->pContainerObj);
    if (pWeapon) {
        bool bHasModEffect = apBrokenItem->HasModEffectActive(10);
        fNewHealth = static_cast<double>(pWeapon->GetFormHealth(bHasModEffect)) * fNewHealth;
    }
    else {
        fNewHealth = static_cast<double>(TESHealthForm::GetFormHealth(apBrokenItem->pContainerObj)) * fNewHealth;
    }
    TESBoundObject* pContainerObj = apBrokenItem->pContainerObj;
    InventoryChanges* pInventoryChanges = InventoryChanges::GetInventoryChanges(pPlayer);
    ItemChange* pItemGroup = pInventoryChanges->GetObjectInList(pContainerObj, true, 0);
    ExtraDataList* pBrokenExtra = apBrokenItem->pExtraObjectList ? apBrokenItem->pExtraObjectList->m_item : nullptr;
    ExtraDataList* pRepairedExtra = new ExtraDataList;
    bool bFoundMatchingExtra = false;
    int32_t iOrigStackCount = pBrokenExtra->GetCount();
    pRepairedExtra->DuplicateExtraListForContainer(pBrokenExtra);
    pRepairedExtra->SetCount(1);
    pRepairedExtra->SetHealth(fNewHealth);
    if (iOrigStackCount > 1) {
        pBrokenExtra->SetCount(iOrigStackCount - 1);
    }
    for (BSSimpleList<ExtraDataList*>* pCurrExtraObj = pItemGroup->pExtraObjectList; pCurrExtraObj &&
         pCurrExtraObj->m_item &&
         (apBrokenItem->pContainerObj->GetFormType() != ARMO_ID || !apBrokenItem->GetScript());
         pCurrExtraObj = pCurrExtraObj->m_pkNext) {
        if (!pCurrExtraObj->m_item->CompareListForContainer(pRepairedExtra, true, false) &&
            !pRepairedExtra->CompareListForContainer(pCurrExtraObj->m_item, true, false)) {
            int16_t sCount = pCurrExtraObj->m_item->GetCount();
            pCurrExtraObj->m_item->SetCount(sCount + 1);
            bFoundMatchingExtra = true;
        }
    }
    if (bFoundMatchingExtra) {
        if (iOrigStackCount == 1) {
            pItemGroup->pExtraObjectList->Remove(pBrokenExtra);
        }
    }
    else if (iOrigStackCount <= 1) {
        pBrokenExtra->SetHealth(fNewHealth);
    }
    else {
        pItemGroup->pExtraObjectList->AddHead(pRepairedExtra);
    }
    if (bFoundMatchingExtra || iOrigStackCount == 1) {
        pRepairedExtra->SetScriptLocals(nullptr);
        if (pRepairedExtra) {
            delete pRepairedExtra;
        }
    }
}

bool RepairServicesMenu::IsWellFormedMenu() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return false;
        }
    }
    return true;
}

void RepairServicesMenu::InitEntries() {
    if (RepairServicesMenu::pMe) {
        char acBuffer[68];
        RepairServicesMenu::pMe->iRepairAllCost = 0;
        RepairServicesMenu::pMe->DoLeave(17, nullptr);
        RepairServicesMenu::pMe->lbRepairList
            .BuildFromContainer(pPlayer, &RepairServicesMenu::ShouldHideItem, nullptr, nullptr, true, false);
        RepairServicesMenu::pMe->lbRepairList
            .Traverse(&RepairServicesMenu::SetCanRepair, -1, std::numeric_limits<int32_t>::max());
        RepairServicesMenu::pMe->lbRepairList.Sort(&RepairServicesMenu::Compare);
        int32_t iGoldAmount = pPlayer->GetGoldAmount();
        if (iGoldAmount <= iCapsLimit.Int()) {
            iGoldAmount = pPlayer->GetGoldAmount();
            BSsprintf(acBuffer, 64, "%s   %d", sInventoryCaps.String(), iGoldAmount);
        }
        else {
            BSsprintf(acBuffer, 64, "%s   %d+", sInventoryCaps.String(), iCapsLimit.Int());
        }
        pTiles[1]->Set(4036, acBuffer, true);
        BSsprintf(acBuffer, 64, "%s   %d", sRepairSkill.String(), iVendorRepairSkill);
        pTiles[0]->Set(4036, acBuffer, true);
        BSsprintf(acBuffer, 64, sRepairAllItems.String(), RepairServicesMenu::pMe->iRepairAllCost);
        RepairServicesMenu::pMe->pTiles[14]->Set(4036, acBuffer, true);
        if (RepairServicesMenu::pMe->iRepairAllCost <= 0 ||
            RepairServicesMenu::pMe->iRepairAllCost > pPlayer->GetGoldAmount()) {
            RepairServicesMenu::pMe->pTiles[14]->Set(4015, 0);
        }
        else {
            RepairServicesMenu::pMe->pTiles[14]->Set(4015, 1);
        }
    }
}

TileMenu* RepairServicesMenu::Create(Actor* apVendor) {
    if (apVendor) {
        InterfaceManager* pin = InterfaceManager::Instance();
        Tile* pOldMenuTile = Menu::GetMenuByClass(RepairServicesMenu::GetClassStatic());
        if (pOldMenuTile) {
            delete pOldMenuTile;
        }
        Tile* pTile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\repair_services_menu.xml");
        RepairServicesMenu::pMe = static_cast<RepairServicesMenu*>(pTile->GetMenu());
        if (RepairServicesMenu::pMe && RepairServicesMenu::pMe->GetClass() == RepairServicesMenu::GetClassStatic()) {
            RepairServicesMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pTile), false);
            if (RepairServicesMenu::pMe->IsWellFormedMenu()) {
                if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
                    pTile->Set(4013, Tile::GetMaximumDepth(), true);
                }
                RepairServicesMenu::pMe->pTiles[6]->Poke(RepairServicesMenu::iUT_Value, 1.0f);
                RepairServicesMenu::pMe->pTiles[11]->Poke(RepairServicesMenu::iUT_Value, 1.0f);
                RepairServicesMenu::pRepairVendor = apVendor;
                RepairServicesMenu::pMe->iVendorRepairSkill = apVendor->GetClampedActorValue(ActorValue::REPAIR);
                RepairServicesMenu::pMe->lbRepairList.Initialize(
                    RepairServicesMenu::pMe->pTiles[2],
                    "RSM_RepairListTemplate"
                );
                RepairServicesMenu::pMe->InitEntries();
                DialogMenu::ServiceStart();
                if (BSstrcmp(sLanguage.String(), "ENGLISH")) {
                    const char* pTitle = sRepairServicesTitle.String();
                    int32_t iTitleTrait = g_repairServicesMenu_trait_Title;
                    Tile* pHeadline = pTile->GetChildByName("RSM_Headline_Title");
                    pHeadline->Set(iTitleTrait, pTitle, true);
                    const char* pNoItems = sNoItemsToRepair.String();
                    iTitleTrait = g_repairServicesMenu_trait_Title;
                    Tile* pEmptyText = pTile->GetChildByName("RSM_EmptyRepairListText");
                    pEmptyText->Set(iTitleTrait, pNoItems, true);
                    const char* pCondition = sInventoryCondition.String();
                    RepairServicesMenu::pMe->pTiles[5]->Set(g_repairServicesMenu_trait_Title, pCondition, true);
                    pCondition = sInventoryCondition.String();
                    RepairServicesMenu::pMe->pTiles[10]->Set(g_repairServicesMenu_trait_Title, pCondition, true);
                    const char* pRepairItem = sRepairItem.String();
                    RepairServicesMenu::pMe->pTiles[13]->Set(4036, pRepairItem, true);
                    const char* pDone = sDone.String();
                    RepairServicesMenu::pMe->pTiles[15]->Set(4036, pDone, true);
                }
                RepairServicesMenu::pMe->PrepForVisibility(false);
                return static_cast<TileMenu*>(pTile);
            }

            Warning("MENUS: Repair Services Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }

        if (RepairServicesMenu::pMe && RepairServicesMenu::pMe->pMenu && RepairServicesMenu::pMe) {
            delete RepairServicesMenu::pMe;
        }

        return nullptr;
    }

    InitializationFlow(
        "Create() -- Missing vendor parameter.  4 bytes large, responds to the name 'Vendy'.  "
        "Please call if you see him!"
    );
    return nullptr;
}

int32_t RepairServicesMenu::GetClassStatic() {
    return 1058;
}

void RepairServicesMenu::Close() {
    DialogMenu::ServiceEnd(TESTopic::GetTopic(DT_SERVICE, 5));
    pMe->pMenu->Set(6002, 1);
    pMe->StartFadeOut();
}

void RepairServicesMenu::RepairAllFunc(Tile* apTile, ItemChange* apItem) {
    if (apTile->IsTrue(g_repairServicesMenu_trait_CanRepair)) {
        RepairServicesMenu::pMe->RepairItem(apItem);
    }
}

void RepairServicesMenu::SetCanRepair(Tile* apTile, ItemChange* apItem) {
    if (apTile && apItem) {
        float fValue = static_cast<float>(TESValueForm::GetFormValue(apItem->pContainerObj));
        float fItemHealth = apItem->GetItemHealth(true);
        int32_t iRepairCost = GamePlayFormulas::GetRepairCost(
            RepairServicesMenu::pMe->iVendorRepairSkill,
            fItemHealth,
            fValue
        );
        float fAlpha = iRepairCost <= 0 || iRepairCost > pPlayer->GetGoldAmount() ? 128.0f : 255.0f;
        apTile->Set(4010, 17);
        apTile->Set(g_repairServicesMenu_trait_RepairCost, iRepairCost);
        RepairServicesMenu::pMe->iRepairAllCost += iRepairCost;
        apTile->xChildren.Get(apTile->xChildren.m_pkTail)->Set(4009, fAlpha, true);
        apTile->GetChildByID(18)->Set(4009, fAlpha, true);
        apTile->xChildren.Get(apTile->xChildren.m_pkHead)->Set(4009, fAlpha, true);
        int32_t iVisible = apTile->GetFloat(4012) >= 0.0;
        apTile->Set(4003, iVisible);
        float fHealthWidth = apItem->GetItemHealth(true) * 0.6000000238418579;
        apTile->GetChildByID(18)->Set(4017, fHealthWidth, true);
        int32_t iCanRepair = fAlpha == 255.0;
        apTile->Set(g_repairServicesMenu_trait_CanRepair, iCanRepair);
        if (apItem->GetWorn(false)) {
            apTile->xChildren.Get(apTile->xChildren.m_pkHead)->Set(4003, 1);
        }
    }
}

bool RepairServicesMenu::ShouldHideItem(ItemChange* apItem) {
    TESObjectARMO* pArmor = static_cast<TESObjectARMO*>(apItem->pContainerObj);
    TESBipedModelForm* pBipedModel = TESBipedModelForm::GetFormAsBipedModel(pArmor);
    if (!pArmor ||
        (pBipedModel && !pBipedModel->GetPlayable()) ||
        TESContainer::IsGold(pArmor) ||
        pArmor->GetQuestObject()) {
        return true;
    }
    if (TESHealthForm::GetFormAsHealthForm(pArmor)) {
        double dItemHealth = apItem->GetItemHealth(true);
        if (!(fRepairSkillMax.Float() * 10.0 <= dItemHealth) &&
            (pArmor->GetFormType() != ARMO_ID ||
             !(pArmor->GetArmorRating() <= 0.0) ||
             !(pArmor->GetDamageThreshold() <= 0.0))) {
            return false;
        }
    }
    return true;
}

int32_t RepairServicesMenu::Compare(ListBoxItem<ItemChange*>* apItem1, ListBoxItem<ItemChange*>* apItem2) {
    int32_t iResult = 0;
    bool bCannotRepair1 = !apItem1->pTile->IsTrue(g_repairServicesMenu_trait_CanRepair);
    bool bCannotRepair2 = !apItem2->pTile->IsTrue(g_repairServicesMenu_trait_CanRepair);
    if (!bCannotRepair1 || bCannotRepair2) {
        if (bCannotRepair1 || !bCannotRepair2) {
            double dItemHealth = apItem1->data->GetItemHealth(false);
            if (!(apItem2->data->GetItemHealth(false) > dItemHealth)) {
                dItemHealth = apItem1->data->GetItemHealth(false);
                if (!(apItem2->data->GetItemHealth(false) < dItemHealth)) {
                    if (apItem1->data->GetWorn(false)) {
                        iResult = -1;
                    }
                    else if (apItem2->data->GetWorn(false)) {
                        iResult = 1;
                    }
                }
                else {
                    iResult = -1;
                }
            }
            else {
                iResult = 1;
            }
        }
        else {
            iResult = -1;
        }
    }
    else {
        iResult = 1;
    }
    if (bCannotRepair1 && bCannotRepair2) {
        iResult = -iResult;
    }

    return iResult;
}
