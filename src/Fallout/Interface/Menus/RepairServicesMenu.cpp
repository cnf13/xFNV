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

void RepairServicesMenu::DoClick(int32_t a1, Tile* a2) {
    if (!a2 && a1 >= 0 && a1 < 16) {
        a2 = pTiles[a1];
    }
    switch (a1) {
        case 14: {
            lbRepairList.Traverse(&RepairServicesMenu::RepairAllFunc, 0, 0x7FFFFFFF);
            pPlayer->PayGoldToActor(RepairServicesMenu::pRepairVendor, iRepairAllCost);
            BSAudio* v6 = &BSAudio::QInstance();
            v6->GetSoundHandleByName("UIRepairWeapon", 289u).Play(false);
            InitEntries();
            break;
        }
        case 15: {
            RepairServicesMenu::Close();
            break;
        }
        case 17: {
            if (a2->IsTrue(g_repairServicesMenu_trait_CanRepair)) {
                RepairItem(lbRepairList.GetHighlightedData());
                double Float = a2->GetFloat(g_repairServicesMenu_trait_RepairCost);
                pPlayer->PayGoldToActor(RepairServicesMenu::pRepairVendor, static_cast<int32_t>(Float));
                BSAudio* v4 = &BSAudio::QInstance();
                v4->GetSoundHandleByName("UIRepairWeapon", 289u).Play(false);
                InitEntries();
            }
            else {
                BSAudio* v5 = &BSAudio::QInstance();
                v5->GetSoundHandleByName("UIVATSInsufficientAP", 289u).Play(false);
            }
            break;
        }
    }
}

void RepairServicesMenu::DoEnter(int32_t a1, Tile* a2) {
    if (a1 == 17) {
        char Src[68];
        ItemChange* pcurrItem = lbRepairList.GetHighlightedData();
        float v42 = 0.0f;
        float v39 = 0.0f;
        bool IsTrue = a2->IsTrue(g_repairServicesMenu_trait_CanRepair);
        pTiles[13]->Set(4015, static_cast<int32_t>(IsTrue));
        pTiles[4]->Set(4003, 1);
        const char* IconOne = pcurrItem->GetIconOne(nullptr);
        pTiles[3]->Set(4044, IconOne, true);
        pTiles[3]->Set(4003, 1);
        float foldHealthPercent = pcurrItem->GetItemHealth(true) / 100.0;
        pTiles[5]->Set(Tile::kTileValue_User0, foldHealthPercent, true);
        if (pcurrItem->pContainerObj->GetFormType() == WEAP_ID) {
            const char* v5 = sInventoryDamage.String();
            pTiles[6]->Set(g_repairServicesMenu_trait_Title, v5, true);
            float afLeft = CombatFormulas::CalcWeaponDamageForDisplay(
                static_cast<TESObjectWEAP*>(pcurrItem->pContainerObj), foldHealthPercent, pcurrItem, nullptr
            );
            double v6 = BSRoundToNearest(afLeft, 1.0f);
            pTiles[6]->Set(RepairServicesMenu::iUT_Value, static_cast<int32_t>(v6));
        }
        else {
            if (pcurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                double ArmorRating =
                    static_cast<TESObjectARMO*>(pcurrItem->pContainerObj)->TESObjectARMO::GetArmorRating();
                float v37 = CombatFormulas::CalcArmorRating(static_cast<int32_t>(ArmorRating), foldHealthPercent);
                double DamageThreshold =
                    static_cast<TESObjectARMO*>(pcurrItem->pContainerObj)->TESObjectARMO::GetDamageThreshold();
                float v38 = CombatFormulas::CalcArmorRating(static_cast<int32_t>(DamageThreshold), foldHealthPercent);
                if (!(v38 > 0.0)) {
                    if (!(v37 > 0.0)) {
                        char Dst[132];
                        BSstrcpy(Dst, 128, "--");
                        const char* v11 = sInventoryDamageResistance.String();
                        RepairServicesMenu::pMe->pTiles[6]->Set(g_repairServicesMenu_trait_Title, v11, true);
                        RepairServicesMenu::pMe->pTiles[6]->Set(RepairServicesMenu::iUT_Value, Dst, true);
                    }
                    else {
                        const char* v10 = sInventoryDamageResistance.String();
                        RepairServicesMenu::pMe->pTiles[6]->Set(g_repairServicesMenu_trait_Title, v10, true);
                        RepairServicesMenu::pMe->pTiles[6]->Set(
                            RepairServicesMenu::iUT_Value, static_cast<int32_t>(v37)
                        );
                    }
                }
                else {
                    const char* v9 = sInventoryDamageThreshold.String();
                    RepairServicesMenu::pMe->pTiles[6]->Set(g_repairServicesMenu_trait_Title, v9, true);
                    RepairServicesMenu::pMe->pTiles[6]->Set(RepairServicesMenu::iUT_Value, static_cast<int32_t>(v38));
                }
            }
        }
        float* v12 = &fRepairSkillBase.Float();
        float* v13 = &fRepairSkillMax.Float();
        v39 = (*v13 - fRepairSkillBase.Float()) * (static_cast<double>(iVendorRepairSkill) / 100.0) + *v12;
        float v29 = v39 / 10.0;
        float fnewHealthPercent = BSMIN(v29, 1.0f);
        double v28 = BSRoundToNearest(fnewHealthPercent, 0.001f);
        if (!(BSRoundToNearest(foldHealthPercent, 0.001f) >= v28)) {
            pTiles[10]->Set(Tile::kTileValue_User0, fnewHealthPercent, true);
            pTiles[10]->Set(4003, 1);
            if (pcurrItem->pContainerObj->GetFormType() == WEAP_ID) {
                const char* v14 = sInventoryDamage.String();
                pTiles[11]->Set(g_repairServicesMenu_trait_Title, v14, true);
                float afLefta = CombatFormulas::CalcWeaponDamageForDisplay(
                    static_cast<TESObjectWEAP*>(pcurrItem->pContainerObj), fnewHealthPercent, pcurrItem, nullptr
                );
                double v15 = BSRoundToNearest(afLefta, 1.0f);
                pTiles[11]->Set(RepairServicesMenu::iUT_Value, static_cast<int32_t>(v15));
            }
            else {
                if (pcurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                    double v16 = static_cast<TESObjectARMO*>(pcurrItem->pContainerObj)->TESObjectARMO::GetArmorRating();
                    float v32 = CombatFormulas::CalcArmorRating(static_cast<int32_t>(v16), fnewHealthPercent);
                    double v17 =
                        static_cast<TESObjectARMO*>(pcurrItem->pContainerObj)->TESObjectARMO::GetDamageThreshold();
                    float v33 = CombatFormulas::CalcArmorRating(static_cast<int32_t>(v17), fnewHealthPercent);
                    if (!(v33 > 0.0)) {
                        if (!(v32 > 0.0)) {
                            char apDest[128];
                            BSstrcpy(apDest, 128, "--");
                            const char* v20 = sInventoryDamageResistance.String();
                            RepairServicesMenu::pMe->pTiles[11]->Set(g_repairServicesMenu_trait_Title, v20, true);
                            RepairServicesMenu::pMe->pTiles[11]->Set(RepairServicesMenu::iUT_Value, apDest, true);
                        }
                        else {
                            const char* v19 = sInventoryDamageResistance.String();
                            RepairServicesMenu::pMe->pTiles[11]->Set(g_repairServicesMenu_trait_Title, v19, true);
                            RepairServicesMenu::pMe->pTiles[11]->Set(
                                RepairServicesMenu::iUT_Value, static_cast<int32_t>(v32)
                            );
                        }
                    }
                    else {
                        const char* v18 = sInventoryDamageThreshold.String();
                        RepairServicesMenu::pMe->pTiles[11]->Set(g_repairServicesMenu_trait_Title, v18, true);
                        RepairServicesMenu::pMe->pTiles[11]->Set(
                            RepairServicesMenu::iUT_Value, static_cast<int32_t>(v33)
                        );
                    }
                }
            }
            double Float = pTiles[10]->GetFloat(Tile::kTileValue_User0);
            float fhealthImprovement = (Float - pTiles[5]->GetFloat(Tile::kTileValue_User0)) * 100.0;
            double v26 = pTiles[11]->GetFloat(RepairServicesMenu::iUT_Value);
            float fstatImprovement = v26 - pTiles[6]->GetFloat(RepairServicesMenu::iUT_Value);
            fhealthImprovement = BSMAX(0.0f, fhealthImprovement);
            fstatImprovement = BSMAX(0.0f, fstatImprovement);
            BSsprintf(Src, 64, "+%i", static_cast<int32_t>(fhealthImprovement + 0.5));
            BSstrcat(Src, 64, "%");
            pTiles[7]->Set(4036, Src, true);
            BSsprintf(Src, 64, "+%i", static_cast<int32_t>(fstatImprovement + 0.5));
            pTiles[8]->Set(4036, Src, true);
            int32_t a1b = static_cast<int32_t>(a2->GetFloat(g_repairServicesMenu_trait_RepairCost));
            BSsprintf(Src, 64, sRepairCost.String(), a1b);
            pTiles[12]->Set(4036, Src, true);
            if (a2->IsTrue(g_repairServicesMenu_trait_CanRepair)) {
                pTiles[12]->Set(4009, 255.0f, true);
            }
            else {
                pTiles[12]->Set(4009, 128.0f, true);
            }
        }
        else {
            BSsprintf(Src, 64, sCantRepairPastMax.String(), fnewHealthPercent * 100.0, "%");
            pTiles[9]->Set(4036, Src, true);
            pTiles[10]->Set(4003, 0);
        }
    }
}

void RepairServicesMenu::DoLeave(int32_t a2, Tile* a3) {
    if (a2 == 17) {
        pTiles[13]->Set(4015, 1);
        pTiles[3]->Set(4003, 0);
        pTiles[4]->Set(4003, 0);
    }
}

int32_t RepairServicesMenu::GetClass() {
    return 1058;
}

void RepairServicesMenu::RepairItem(ItemChange* apBrokenItem) {
    ExtraDataList* prepairedExtra;
    ExtraDataList* pbrokenExtra;
    float fnewHealth = 0.0f;
    float* v2 = &fRepairSkillBase.Float();
    float* v3 = &fRepairSkillMax.Float();
    fnewHealth = (*v3 - fRepairSkillBase.Float()) * (static_cast<double>(iVendorRepairSkill) / 100.0) + *v2;
    float afA = fnewHealth / 10.0;
    fnewHealth = BSMIN(afA, 1.0f);
    TESObjectWEAP* v17 = dynamic_cast<TESObjectWEAP*>(apBrokenItem->pContainerObj);
    if (v17) {
        bool HasModEffectActive_0 = apBrokenItem->HasModEffectActive(10);
        fnewHealth = static_cast<double>(v17->GetFormHealth(HasModEffectActive_0)) * fnewHealth;
    }
    else {
        fnewHealth = static_cast<double>(TESHealthForm::GetFormHealth(apBrokenItem->pContainerObj)) * fnewHealth;
    }
    TESBoundObject* FormFlags = apBrokenItem->pContainerObj;
    InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
    ItemChange* pitemGroup = InventoryChanges_->GetObjectInList(FormFlags, true, 0);
    if (apBrokenItem->pExtraObjectList) {
        pbrokenExtra = apBrokenItem->pExtraObjectList->m_item;
    }
    else {
        pbrokenExtra = nullptr;
    }
    ExtraDataList* pbrokenExtra_2 = pbrokenExtra;
    ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
    if (apMemory) {
        prepairedExtra = ::new (apMemory) ExtraDataList;
    }
    else {
        prepairedExtra = nullptr;
    }
    ExtraDataList* prepairedExtra_2 = prepairedExtra;
    bool bfoundMatchingExtra = false;
    int32_t iorigStackCount = pbrokenExtra_2->GetCount();
    prepairedExtra->DuplicateExtraListForContainer(pbrokenExtra_2);
    prepairedExtra->SetCount(1);
    prepairedExtra->SetHealth(fnewHealth);
    if (iorigStackCount > 1) {
        pbrokenExtra_2->SetCount(iorigStackCount - 1);
    }
    for (BSSimpleList<ExtraDataList*>* pcurrExtraObj = pitemGroup->pExtraObjectList; pcurrExtraObj &&
         (pcurrExtraObj->m_item &&
          (apBrokenItem->pContainerObj->GetFormType() != ARMO_ID || !apBrokenItem->GetScript()));
         pcurrExtraObj = pcurrExtraObj->m_pkNext) {
        if (!pcurrExtraObj->m_item->CompareListForContainer(prepairedExtra_2, true, false) &&
            !prepairedExtra_2->CompareListForContainer(pcurrExtraObj->m_item, true, false)) {
            int16_t Count = pcurrExtraObj->m_item->GetCount();
            pcurrExtraObj->m_item->SetCount(Count + 1);
            bfoundMatchingExtra = true;
        }
    }
    if (bfoundMatchingExtra) {
        if (iorigStackCount == 1) {
            pitemGroup->pExtraObjectList->Remove(pbrokenExtra_2);
        }
    }
    else {
        if (iorigStackCount <= 1) {
            pbrokenExtra_2->SetHealth(fnewHealth);
        }
        else {
            pitemGroup->pExtraObjectList->AddHead(prepairedExtra_2);
        }
    }
    if (bfoundMatchingExtra || iorigStackCount == 1) {
        prepairedExtra_2->SetScriptLocals(nullptr);
        if (prepairedExtra_2) {
            delete prepairedExtra_2;
        }
    }
}

bool RepairServicesMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 16; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

void RepairServicesMenu::InitEntries() {
    if (RepairServicesMenu::pMe) {
        char Src[68];
        RepairServicesMenu::pMe->iRepairAllCost = 0;
        RepairServicesMenu::pMe->DoLeave(17, nullptr);
        RepairServicesMenu::pMe->lbRepairList.BuildFromContainer(
            pPlayer, &RepairServicesMenu::ShouldHideItem, nullptr, nullptr, true, false
        );
        RepairServicesMenu::pMe->lbRepairList.Traverse(&RepairServicesMenu::SetCanRepair, -1, 0x7FFFFFFF);
        (&RepairServicesMenu::pMe->lbRepairList)->Sort(&RepairServicesMenu::Compare);
        int32_t GoldAmount = pPlayer->GetGoldAmount();
        if (GoldAmount <= iCapsLimit.Int()) {
            int32_t v2 = pPlayer->GetGoldAmount();
            BSsprintf(Src, 64, "%s   %d", sInventoryCaps.String(), v2);
        }
        else {
            BSsprintf(Src, 64, "%s   %d+", sInventoryCaps.String(), iCapsLimit.Int());
        }
        pTiles[1]->Set(4036, Src, true);
        BSsprintf(Src, 64, "%s   %d", sRepairSkill.String(), iVendorRepairSkill);
        pTiles[0]->Set(4036, Src, true);
        BSsprintf(Src, 64, sRepairAllItems.String(), RepairServicesMenu::pMe->iRepairAllCost);
        RepairServicesMenu::pMe->pTiles[14]->Set(4036, Src, true);
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
        int32_t v3;
        InterfaceManager* pin = InterfaceManager::Instance();
        int32_t ClassStatic = RepairServicesMenu::GetClassStatic();
        Tile* pold_menu_tile = Menu::GetMenuByClass(ClassStatic);
        if (pold_menu_tile) {
            delete pold_menu_tile;
        }
        Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\repair_services_menu.xml");
        RepairServicesMenu::pMe = static_cast<RepairServicesMenu*>(ptile->GetMenu());
        if (RepairServicesMenu::pMe &&
            (v3 = RepairServicesMenu::pMe->GetClass(), v3 == RepairServicesMenu::GetClassStatic())) {
            RepairServicesMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            if (RepairServicesMenu::pMe->IsWellFormedMenu()) {
                if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                    float MaximumDepth = Tile::GetMaximumDepth();
                    ptile->Set(4013, MaximumDepth, true);
                }
                RepairServicesMenu::pMe->pTiles[6]->Poke(RepairServicesMenu::iUT_Value, 1.0f);
                RepairServicesMenu::pMe->pTiles[11]->Poke(RepairServicesMenu::iUT_Value, 1.0f);
                RepairServicesMenu::pRepairVendor = apVendor;
                RepairServicesMenu::pMe->iVendorRepairSkill = apVendor->GetClampedActorValue(ActorValue::REPAIR);
                RepairServicesMenu::pMe->lbRepairList.Initialize(
                    RepairServicesMenu::pMe->pTiles[2], "RSM_RepairListTemplate"
                );
                RepairServicesMenu::pMe->InitEntries();
                DialogMenu::ServiceStart();
                const char* v4 = sLanguage.String();
                if (BSstrcmp(v4, "ENGLISH")) {
                    const char* v14 = sRepairServicesTitle.String();
                    int32_t v11 = g_repairServicesMenu_trait_Title;
                    Tile* ChildByName = ptile->GetChildByName("RSM_Headline_Title");
                    ChildByName->Set(v11, v14, true);
                    const char* v15 = sNoItemsToRepair.String();
                    int32_t v12 = g_repairServicesMenu_trait_Title;
                    Tile* v6 = ptile->GetChildByName("RSM_EmptyRepairListText");
                    v6->Set(v12, v15, true);
                    const char* v7 = sInventoryCondition.String();
                    RepairServicesMenu::pMe->pTiles[5]->Set(g_repairServicesMenu_trait_Title, v7, true);
                    const char* v8 = sInventoryCondition.String();
                    RepairServicesMenu::pMe->pTiles[10]->Set(g_repairServicesMenu_trait_Title, v8, true);
                    const char* v9 = sRepairItem.String();
                    RepairServicesMenu::pMe->pTiles[13]->Set(4036, v9, true);
                    const char* v10 = sDone.String();
                    RepairServicesMenu::pMe->pTiles[15]->Set(4036, v10, true);
                }
                RepairServicesMenu::pMe->PrepForVisibility(false);
                return static_cast<TileMenu*>(ptile);
            }

            Warning("MENUS: Repair Services Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }

        if ((RepairServicesMenu::pMe) && (RepairServicesMenu::pMe->pMenu) && (RepairServicesMenu::pMe)) {
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

void RepairServicesMenu::RepairAllFunc(Tile* a1, ItemChange* a2) {
    if (a1->IsTrue(g_repairServicesMenu_trait_CanRepair)) {
        RepairServicesMenu::pMe->RepairItem(a2);
    }
}

void RepairServicesMenu::SetCanRepair(Tile* a1, ItemChange* a2) {
    if ((a1) && (a2)) {
        float v8;
        float afValue = static_cast<float>(TESValueForm::GetFormValue(a2->pContainerObj));
        float ItemHealth = a2->GetItemHealth(true);
        int32_t aiValue =
            GamePlayFormulas::GetRepairCost(RepairServicesMenu::pMe->iVendorRepairSkill, ItemHealth, afValue);
        if (aiValue <= 0 || aiValue > pPlayer->GetGoldAmount()) {
            v8 = 128.0f;
        }
        else {
            v8 = 255.0f;
        }
        a1->Set(4010, 17);
        a1->Set(g_repairServicesMenu_trait_RepairCost, aiValue);
        RepairServicesMenu::pMe->iRepairAllCost += aiValue;
        a1->xChildren.Get(a1->xChildren.m_pkTail)->Set(4009, v8, true);
        Tile* ChildByID = a1->GetChildByID(18);
        ChildByID->Set(4009, v8, true);
        a1->xChildren.Get(a1->xChildren.m_pkHead)->Set(4009, v8, true);
        int32_t v7 = a1->GetFloat(4012) >= 0.0;
        a1->Set(4003, v7);
        float v6 = a2->GetItemHealth(true) * 0.6000000238418579;
        Tile* v3 = a1->GetChildByID(18);
        v3->Set(4017, v6, true);
        int32_t iCanRepair = v8 == 255.0;
        a1->Set(g_repairServicesMenu_trait_CanRepair, iCanRepair);
        if (a2->GetWorn(false)) {
            a1->xChildren.Get(a1->xChildren.m_pkHead)->Set(4003, 1);
        }
    }
}

bool RepairServicesMenu::ShouldHideItem(ItemChange* a1) {
    TESObjectARMO* a1a = static_cast<TESObjectARMO*>(a1->pContainerObj);
    TESBipedModelForm* FormAsBipedModel = TESBipedModelForm::GetFormAsBipedModel(a1a);
    if (!a1a ||
        ((FormAsBipedModel && !FormAsBipedModel->GetPlayable()) ||
         (TESContainer::IsGold(a1a) || a1a->GetQuestObject()))) {
        return true;
    }
    bool v2 = true;
    if (TESHealthForm::GetFormAsHealthForm(a1a)) {
        double ItemHealth = a1->GetItemHealth(true);
        if (!(fRepairSkillMax.Float() * 10.0 <= ItemHealth) &&
            (a1a->GetFormType() != ARMO_ID ||
             (!(a1a->GetArmorRating() <= 0.0) || !(a1a->GetDamageThreshold() <= 0.0)))) {
            return false;
        }
    }
    return v2;
}

int32_t RepairServicesMenu::Compare(ListBoxItem<ItemChange*>* a1, ListBoxItem<ItemChange*>* a2) {
    int32_t iResult = 0;
    bool bCannotRepair1 = !a1->pTile->IsTrue(g_repairServicesMenu_trait_CanRepair);
    bool bCannotRepair2 = !a2->pTile->IsTrue(g_repairServicesMenu_trait_CanRepair);
    if (!bCannotRepair1 || bCannotRepair2) {
        if (bCannotRepair1 || !bCannotRepair2) {
            double ItemHealth = a1->data->GetItemHealth(false);
            if (!(a2->data->GetItemHealth(false) > ItemHealth)) {
                double v3 = a1->data->GetItemHealth(false);
                if (!(a2->data->GetItemHealth(false) < v3)) {
                    if (a1->data->GetWorn(false)) {
                        iResult = -1;
                    }
                    else {
                        if (a2->data->GetWorn(false)) {
                            iResult = 1;
                        }
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
    if ((bCannotRepair1) && (bCannotRepair2)) {
        iResult = -iResult;
    }

    return iResult;
}
