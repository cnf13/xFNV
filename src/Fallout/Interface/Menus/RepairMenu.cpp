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

// ListBox<ItemChange*>::NumUnfiltered's guarded static and its guard word.
static int32_t iUT_num_filtered;
static uint32_t iUT_num_filtered_guard;

static uint32_t data_11D8CE0;

RepairMenu::RepairMenu() {
    RepairMenu::iUT_Title = Tile::TextToTrait("_Title");
    RepairMenu::iUT_Value = Tile::TextToTrait("_Value");
    RepairMenu_Trait_enabled = Tile::TextToTrait("_enabled");
    RepairMenu::iUT_IsRepairItem = Tile::TextToTrait("_IsRepairItem");
}

RepairMenu::~RepairMenu() {
    pTiles[8]->RemoveFadeControl(4009);
    RepairMenu::pMe = 0;
}

void RepairMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (((uint32_t)aiID) <= 12) {
        pTiles[aiID] = apTile;
    }
}

void RepairMenu::DoClick(int32_t a1, Tile* a2) {
    FNVD_INLINED_LOCAL(BSSoundHandle, a12, std::nullopt, std::nullopt);
    char v13 = 0;
    if (!a2 && a1 >= 0 && a1 < 13) {
        a2 = pTiles[a1];
    }
    if (a1 == 12) {
        InterfaceManager::PlayMenuSound(MENU_SOUND_MODE);
        Interface::SetInventoryMenuVisible(1, 0, 0);
    }
    else {
        if (a1 == 14 && !(a2->xChildren.GetTail()->GetFloat(4009) < 255.0)) {
            ItemChange* HighlightedData = lbRepairList.GetHighlightedData();
            if (HighlightedData != RepairMenu::pBrokenItem && pTiles[9]->IsTrue(4003)) {
                ExtraDataList* pspareExtra;
                ItemChange* psparePart = lbRepairList.GetHighlightedData();
                if (psparePart->pExtraObjectList) {
                    pspareExtra = psparePart->pExtraObjectList->m_item;
                }
                else {
                    pspareExtra = 0;
                }
                RepairItem(psparePart);
                TESBoundObject* FormFlags = psparePart->pContainerObj;
                pPlayer->RemoveItem(FormFlags, pspareExtra, 1, 0, 0, 0, 0, 0, 1, 0);
                BSAudio* v4 = &BSAudio::QInstance();
                BSSoundHandle* SoundHandleByName = v4->GetSoundHandleByName(&a12, "UIRepairWeapon", 289u);
                SoundHandleByName->Play(0);
                Tile* v5 = static_cast<Tile*>(lbRepairList.GetHighlightedItem());
                lbRepairList.RemoveItem(v5);
                if (lbRepairList.NumUnfiltered() == 1 || RepairMenu::pBrokenItem->GetItemHealth(1) >= 99.0) {
                    DoClick(12, 0);
                }
                else {
                    RepairMenu::InitRepairMenu(0);
                }
                if (psparePart->GetWorn(0)) {
                    pPlayer->ResetArmorRating();
                    InventoryMenu::UpdateList();
                }
            }
        }
    }
}

void RepairMenu::DoEnter(int32_t a1, Tile* a2) {
    static int32_t iprevHighlightIndex;
    if (a1 == 14) {
        int32_t inewHighlightIndex;
        ItemChange* pcurrItem = lbRepairList.GetHighlightedData();
        if (pcurrItem) {
            Tile* v3 = static_cast<Tile*>(lbRepairList.GetHighlightedItem());
            inewHighlightIndex = static_cast<int32_t>(v3->GetFloat(4012));
        }
        else {
            inewHighlightIndex = 0;
        }
        if (iprevHighlightIndex != inewHighlightIndex) {
            float v29;
            if (iprevHighlightIndex >= inewHighlightIndex) {
                v29 = -fScrollKnobIncrement.Float();
            }
            else {
                v29 = fScrollKnobIncrement.Float();
            }
            float afLeft_4 = fScrollKnobRate.Float();
            InterfaceManager* v4 = InterfaceManager::Instance();
            FOPipboyManager* Pipboy = v4->GetPipboy();
            Pipboy->MoveKnob(FOPipboyManager::KNOB_SCROLL, v29, afLeft_4, 1);
            iprevHighlightIndex = inewHighlightIndex;
        }
        if (pcurrItem) {
            if (RepairMenu::pBrokenItem) {
                const char* v6;
                float fMaintainValue = 75.0f;
                if (pcurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                    fMaintainValue = 50.0f;
                }
                if (!(static_cast<double>(fMaintainValue) <= pcurrItem->GetItemHealth(1))) {
                    v6 = sRepairItem.String();
                }
                else {
                    v6 = sMaintainItem.String();
                }
                RepairMenu::pMe->pTiles[11]->Set(4036, v6, 1);
                if (pcurrItem == RepairMenu::pBrokenItem) {
                    if (lbRepairList.NumUnfiltered() > 1) {
                        const char* v7 = sSelectItemToRepair.String();
                        ShowErrorMessage(v7);
                    }
                    int32_t v8 = Tile::TextToTrait("_line_alpha");
                    pTiles[11]->Set(v8, 128.0f, 1);
                }
                else {
                    char Src[68];
                    float fnewHealthPercent = a2->GetFloat(Tile::kTileValue_User0);
                    int32_t iminSkillNeeded = static_cast<int32_t>(a2->GetFloat(Tile::kTileValue_User1));
                    if (iminSkillNeeded <= 100) {
                        if (iminSkillNeeded <= 0) {
                            int32_t v11 = Tile::TextToTrait("_line_alpha");
                            pTiles[11]->Set(v11, 255.0f, 1);
                            pTiles[9]->Set(Tile::kTileValue_User0, fnewHealthPercent, 1);
                            pTiles[9]->Set(4003, 1);
                            pTiles[8]->Set(4003, 0);
                            if (RepairMenu::pBrokenItem->pContainerObj->GetFormType() == WEAP_ID) {
                                const char* v12 = sInventoryDamage.String();
                                pTiles[10]->Set(RepairMenu::iUT_Title, v12, 1);
                                float afLeft_4a = CombatFormulas::CalcWeaponDamageForDisplay(
                                    static_cast<TESObjectWEAP*>(RepairMenu::pBrokenItem->pContainerObj),
                                    fnewHealthPercent, RepairMenu::pBrokenItem, 0
                                );
                                double v13 = BSRoundToNearest(afLeft_4a, 1.0f);
                                pTiles[10]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(v13));
                            }
                            else {
                                if (pcurrItem->pContainerObj->GetFormType() == ARMO_ID) {
                                    char Dst[132];
                                    TESObjectARMO* pBrokenArmor =
                                        static_cast<TESObjectARMO*>(RepairMenu::pBrokenItem->pContainerObj);
                                    float foldHealthPercent = RepairMenu::pBrokenItem->GetItemHealth(1) / 100.0;
                                    double ArmorRating = pBrokenArmor->GetArmorRating();
                                    float fBrokenDR = CombatFormulas::CalcArmorRating(
                                        static_cast<int32_t>(ArmorRating), foldHealthPercent
                                    );
                                    double DamageThreshold = pBrokenArmor->GetDamageThreshold();
                                    float fBrokenDT = CombatFormulas::CalcArmorRating(
                                        static_cast<int32_t>(DamageThreshold), foldHealthPercent
                                    );
                                    double v16 = pBrokenArmor->GetArmorRating();
                                    float fRepairedDR =
                                        CombatFormulas::CalcArmorRating(static_cast<int32_t>(v16), fnewHealthPercent);
                                    double v17 = pBrokenArmor->GetDamageThreshold();
                                    float fRepairedDT =
                                        CombatFormulas::CalcArmorRating(static_cast<int32_t>(v17), fnewHealthPercent);
                                    BSstrcpy(Dst, 128, "--");
                                    if (!(fRepairedDT > 0.0)) {
                                        if (!(fRepairedDR > 0.0)) {
                                            const char* v22 = sInventoryDamageResistance.String();
                                            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v22, 1);
                                            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, Dst, 1);
                                            const char* v23 = sInventoryDamageResistance.String();
                                            RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Title, v23, 1);
                                            RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Value, Dst, 1);
                                        }
                                        else {
                                            const char* v20 = sInventoryDamageResistance.String();
                                            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v20, 1);
                                            if (!(fBrokenDR > 0.0)) {
                                                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, Dst, 1);
                                            }
                                            else {
                                                RepairMenu::pMe->pTiles[5]->Set(
                                                    RepairMenu::iUT_Value, static_cast<int32_t>(fBrokenDR)
                                                );
                                            }
                                            const char* v21 = sInventoryDamageResistance.String();
                                            RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Title, v21, 1);
                                            RepairMenu::pMe->pTiles[10]->Set(
                                                RepairMenu::iUT_Value, static_cast<int32_t>(fRepairedDR)
                                            );
                                        }
                                    }
                                    else {
                                        const char* v18 = sInventoryDamageThreshold.String();
                                        RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v18, 1);
                                        if (!(fBrokenDT > 0.0)) {
                                            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, Dst, 1);
                                        }
                                        else {
                                            RepairMenu::pMe->pTiles[5]->Set(
                                                RepairMenu::iUT_Value, static_cast<int32_t>(fBrokenDT)
                                            );
                                        }
                                        const char* v19 = sInventoryDamageThreshold.String();
                                        RepairMenu::pMe->pTiles[10]->Set(RepairMenu::iUT_Title, v19, 1);
                                        RepairMenu::pMe->pTiles[10]->Set(
                                            RepairMenu::iUT_Value, static_cast<int32_t>(fRepairedDT)
                                        );
                                    }
                                }
                            }
                            double Float = pTiles[9]->GetFloat(Tile::kTileValue_User0);
                            float fhealthImprovement = (Float - pTiles[4]->GetFloat(Tile::kTileValue_User0)) * 100.0;
                            double v27 = pTiles[10]->GetFloat(RepairMenu::iUT_Value);
                            float fstatImprovement = v27 - pTiles[5]->GetFloat(RepairMenu::iUT_Value);
                            fhealthImprovement = BSMAX(0.0f, fhealthImprovement);
                            fstatImprovement = BSMAX(0.0f, fstatImprovement);
                            BSsprintf(Src, 64, "+%i", static_cast<int32_t>(fhealthImprovement + 0.5));
                            BSstrcat(Src, 64, "%");
                            pTiles[6]->Set(4036, Src, 1);
                            BSsprintf(Src, 64, "+%i", static_cast<int32_t>(fstatImprovement + 0.5));
                            pTiles[7]->Set(4036, Src, 1);
                        }
                        else {
                            BSsprintf(Src, 64, sRepairSkillTooLow.String(), iminSkillNeeded, "%");
                            ShowErrorMessage(Src);
                            int32_t v10 = Tile::TextToTrait("_line_alpha");
                            pTiles[11]->Set(v10, 128.0f, 1);
                        }
                    }
                    else {
                        double afLeft = fRepairSkillMax.Float() * 10.0;
                        BSsprintf(Src, 64, sCantRepairPastMax.String(), afLeft, "%");
                        ShowErrorMessage(Src);
                        int32_t v9 = Tile::TextToTrait("_line_alpha");
                        pTiles[11]->Set(v9, 128.0f, 1);
                    }
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

bool RepairMenu::DoGamepadUpEvent(int32_t a2, float a3) {
    if ((a2 == 6) || (a2 == 10)) {
        Interface::PlayMenuSound(3);
        DoClick(12, nullptr);
    }
    return (bool)0;
}

void RepairMenu::ShowErrorMessage(const char* astrMessage) {
    pTiles[8]->Set(4036, astrMessage, (bool)1);
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
            return (bool)0;
        }
    }
    return (bool)1;
}

void RepairMenu::RepairItem(ItemChange* apSparePart) {
    ExtraDataList* prepairedExtra;
    ExtraDataList* pbrokenExtra;
    float afCondition2 = apSparePart->GetItemHealth(1);
    float afCondition1 = RepairMenu::pBrokenItem->GetItemHealth(1);
    int32_t ClampedActorValue = pPlayer->GetClampedActorValue(ActorValue::REPAIR);
    float fnewHealth = GamePlayFormulas::GetRepairedCondition(ClampedActorValue, afCondition1, afCondition2, 0) / 100.0;
    fnewHealth = BSMIN(fnewHealth, 1.0f);
    TESObjectWEAP* pWeap = dynamic_cast<TESObjectWEAP*>(RepairMenu::pBrokenItem->pContainerObj);
    if (pWeap) {
        bool HasModEffectActive_0 = RepairMenu::pBrokenItem->HasModEffectActive(10);
        fnewHealth = static_cast<double>(pWeap->GetFormHealth(HasModEffectActive_0)) * fnewHealth;
    }
    else {
        fnewHealth =
            static_cast<double>(TESHealthForm::GetFormHealth(RepairMenu::pBrokenItem->pContainerObj)) * fnewHealth;
    }
    TESBoundObject* afCondition1a = RepairMenu::pBrokenItem->pContainerObj;
    InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
    ItemChange* pitemGroup = InventoryChanges_->GetObjectInList(afCondition1a, 1, 0);
    if (RepairMenu::pBrokenItem->pExtraObjectList) {
        pbrokenExtra = RepairMenu::pBrokenItem->pExtraObjectList->m_item;
    }
    else {
        pbrokenExtra = 0;
    }
    ExtraDataList* pbrokenExtra_2 = pbrokenExtra;
    ExtraDataList* apMemory = static_cast<ExtraDataList*>(ourMalloc(sizeof(ExtraDataList)));
    if (apMemory) {
        prepairedExtra = ::new (apMemory) ExtraDataList;
    }
    else {
        prepairedExtra = 0;
    }
    ExtraDataList* prepairedExtra_2 = prepairedExtra;
    bool bfoundMatchingExtra = 0;
    int32_t iorigStackCount = pbrokenExtra_2->GetCount();
    prepairedExtra->DuplicateExtraListForContainer(pbrokenExtra_2);
    prepairedExtra->SetCount(1);
    prepairedExtra->SetHealth(fnewHealth);
    MiscStatManager::Increment(MiscStatManager::MS_ITEMS_REPAIRED);
    RepairMenu::pBrokenItem->pExtraObjectList->RemoveHead();
    if (iorigStackCount > 1) {
        pbrokenExtra_2->SetCount(iorigStackCount - 1);
    }
    for (BSSimpleList<ExtraDataList*>* pcurrExtraObj = pitemGroup->pExtraObjectList;
         pcurrExtraObj && pcurrExtraObj->m_item; pcurrExtraObj = pcurrExtraObj->m_pkNext) {
        if (!pcurrExtraObj->m_item->CompareListForContainer(prepairedExtra_2, 1, 0) &&
            !prepairedExtra_2->CompareListForContainer(pcurrExtraObj->m_item, 1, 0)) {
            int16_t Count = pcurrExtraObj->m_item->GetCount();
            pcurrExtraObj->m_item->SetCount(Count + 1);
            BSSimpleList<ExtraDataList*>* afHealth = pcurrExtraObj;
            RepairMenu::pBrokenItem->pExtraObjectList->AddHead(afHealth->m_item);
            bfoundMatchingExtra = 1;
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
            RepairMenu::pBrokenItem->pExtraObjectList->AddHead(pbrokenExtra_2);
        }
        else {
            pitemGroup->pExtraObjectList->AddHead(prepairedExtra_2);
            RepairMenu::pBrokenItem->pExtraObjectList->AddHead(prepairedExtra_2);
        }
    }
    if (bfoundMatchingExtra || iorigStackCount == 1) {
        prepairedExtra_2->SetScriptLocals(0);
        if (prepairedExtra_2) {
            delete prepairedExtra_2;
        }
    }
}

Tile* RepairMenu::Create() {
    int32_t v1;
    InterfaceManager* v4 = InterfaceManager::Instance();
    int32_t ClassStatic = RepairMenu::GetClassStatic();
    Tile* MenuByClass = Menu::GetMenuByClass(ClassStatic);
    if (MenuByClass) {
        delete MenuByClass;
    }
    TileMenu* th_s = static_cast<TileMenu*>(v4->pMenusRoot->ReadFile("Data\\Menus\\repair_menu.xml"));
    RepairMenu::pMe = static_cast<RepairMenu*>(th_s->GetMenu());
    if (RepairMenu::pMe && (v1 = RepairMenu::pMe->GetClass(), v1 == RepairMenu::GetClassStatic())) {
        RepairMenu::pMe->SetMenuTile(th_s, 0);
        RepairMenu::pMe->SetRenderedMenu();
        if (RepairMenu::pMe->IsWellFormedMenu()) {
            if (th_s->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
                th_s->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
                float MaximumDepth = Tile::GetMaximumDepth();
                th_s->Set(Tile::kTileValue_Depth, MaximumDepth, 1);
            }
            RepairMenu::pMe->lbRepairList.Initialize(RepairMenu::pMe->pTiles[1], "RM_RepairListTemplate");
            RepairMenu::pMe->lbRepairList.SetAutoUpdate(0);
            RepairMenu::pMe->pTiles[5]->Poke(RepairMenu::iUT_Value, 1.0f);
            RepairMenu::pMe->pTiles[10]->Poke(RepairMenu::iUT_Value, 1.0f);
            RepairMenu::pMe->PrepForVisibility(1);
            return th_s;
        }
        else {
            Warning("MENUS: Repair Menu Creation Failed... Are your menu and art resources up to date?");
            return 0;
        }
    }
    else {
        if (RepairMenu::pMe) {
            if (RepairMenu::pMe->pMenu) {
                if (RepairMenu::pMe) {
                    delete RepairMenu::pMe;
                }
            }
        }
        return 0;
    }
}

int32_t RepairMenu::GetClassStatic() {
    return 1035;
}

void RepairMenu::InitRepairMenu(ItemChange* apBrokenItem) {
    if (RepairMenu::pMe) {
        if (apBrokenItem) {
            RepairMenu::pBrokenItem = apBrokenItem;
        }
        float fcondition = RepairMenu::pBrokenItem->GetItemHealth(1) / 100.0;
        RepairMenu::pMe->pTiles[4]->Set(Tile::kTileValue_User0, fcondition, 1);
        RepairMenu::pMe->pTiles[4]->Set(4003, 1);
        TESForm* pBrokenArmor = RepairMenu::pBrokenItem->pContainerObj;
        int32_t FormType = pBrokenArmor->GetFormType();
        if (FormType == 40) {
            const char* v1 = sInventoryDamage.String();
            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v1, 1);
            float afVal = CombatFormulas::CalcWeaponDamageForDisplay(
                static_cast<TESObjectWEAP*>(pBrokenArmor), fcondition, apBrokenItem, 0
            );
            double v2 = BSRoundToNearest(afVal, 1.0f);
            RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(v2));
        }
        else {
            if (FormType == 24) {
                TESObjectARMO* v19 = static_cast<TESObjectARMO*>(pBrokenArmor);
                double ArmorRating = static_cast<TESObjectARMO*>(pBrokenArmor)->TESObjectARMO::GetArmorRating();
                float fBrokenDR = CombatFormulas::CalcArmorRating(static_cast<int32_t>(ArmorRating), fcondition);
                double DamageThreshold = static_cast<TESObjectARMO*>(pBrokenArmor)->TESObjectARMO::GetDamageThreshold();
                float fBrokenDT = CombatFormulas::CalcArmorRating(static_cast<int32_t>(DamageThreshold), fcondition);
                if (!(fBrokenDT > 0.0)) {
                    if (!(fBrokenDR > 0.0)) {
                        char Dst[132];
                        BSstrcpy(Dst, 128, "--");
                        const char* v7 = sInventoryDamageResistance.String();
                        RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v7, 1);
                        RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, Dst, 1);
                    }
                    else {
                        const char* v6 = sInventoryDamageResistance.String();
                        RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v6, 1);
                        RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(fBrokenDR));
                    }
                }
                else {
                    const char* v5 = sInventoryDamageThreshold.String();
                    RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v5, 1);
                    RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, static_cast<int32_t>(fBrokenDT));
                }
            }
            else {
                char Src[128];
                BSstrcpy(Src, 128, "--");
                const char* v8 = sInventoryDamageResistance.String();
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Title, v8, 1);
                RepairMenu::pMe->pTiles[5]->Set(RepairMenu::iUT_Value, Src, 1);
            }
        }
        if (apBrokenItem) {
            const char* IconOne = RepairMenu::pBrokenItem->GetIconOne(0);
            RepairMenu::pMe->pTiles[3]->Set(4044, IconOne, 1);
            RepairMenu::Update();
        }
        RepairMenu::pMe->lbRepairList.Traverse(&RepairMenu::InitFunc, 0, 0x7FFFFFFF);
        (&RepairMenu::pMe->lbRepairList)->Sort(&RepairMenu::SortFunc);
        int32_t ClampedActorValue = pPlayer->GetClampedActorValue(ActorValue::REPAIR);
        RepairMenu::pMe->pTiles[0]->Set(RepairMenu::iUT_Value, ClampedActorValue);
        if (apBrokenItem) {
            if (InterfaceManager::Has360Controller()) {
                Tile* v14 = static_cast<ListBoxBase*>(&RepairMenu::pMe->lbRepairList)->GetTileAt(1u, 0);
                InterfaceManager* v11 = InterfaceManager::Instance();
                v11->SetCurrentFocusTarget(v14, 4035, 1);
            }
        }
        Tile* v12 = static_cast<Tile*>(static_cast<ListBoxBase*>(&RepairMenu::pMe->lbRepairList)->GetHighlightedItem());
        RepairMenu::pMe->DoEnter(14, v12);
        if (apBrokenItem) {
            Interface::SetRepairMenuVisible(1, 0, 0);
            int32_t afVala = RepairMenu::GetClassStatic();
            InterfaceManager* v13 = InterfaceManager::Instance();
            v13->xTutorialManager.ShowMessage(Interface::TUT_PIPBOY_REPAIR, afVala, 500u);
        }
    }
}

// RemoveAll is virtual and is called through a pointer so that the call stays indirect.
void RepairMenu::ResetMenu() {
    (&RepairMenu::pMe->lbRepairList)->RemoveAll();
    RepairMenu::pBrokenItem = 0;
    RepairMenu::pMe->DoLeave(14, 0);
    RepairMenu::pMe->pTiles[8]->RemoveFadeControl(4009);
    RepairMenu::pMe->pTiles[8]->Set(4003, 0);
}

void RepairMenu::InitFunc(Tile* a1, ItemChange* a2) {
    int32_t apiMinSkillNeeded;
    if (a1) {
        if (a2) {
            float ItemHealth = a2->GetItemHealth(1);
            a1->Set(4010, 14);
            int32_t a3 = a1->GetFloat(4012) >= 0.0;
            a1->Set(4003, a3);
            float v6 = ItemHealth * 0.6000000238418579;
            Tile* ChildByID = a1->GetChildByID(15);
            ChildByID->Set(4017, v6, static_cast<bool>(1));
            if (a2->GetWorn(0)) {
                a1->xChildren.Get(a1->xChildren.m_pkHead)->Set(4003, 1);
            }
            float v11 = RepairMenu::pBrokenItem->GetItemHealth(1);
            float v10 = 0.0f;
            apiMinSkillNeeded = -1;
            int32_t ClampedActorValue = pPlayer->GetClampedActorValue(ActorValue::REPAIR);
            v10 =
                GamePlayFormulas::GetRepairedCondition(ClampedActorValue, v11, ItemHealth, &apiMinSkillNeeded) / 100.0;
            v10 = BSMIN(v10, 1.0f);
            a1->Set(Tile::kTileValue_User0, v10, static_cast<bool>(1));
        }
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
    double ItemHealth = apItem1->data->GetItemHealth(1);
    if (!(apItem2->data->GetItemHealth(1) > ItemHealth)) {
        int32_t v3;
        double v4 = apItem1->data->GetItemHealth(1);
        if (!(apItem2->data->GetItemHealth(1) < v4)) {
            v3 = 0;
        }
        else {
            v3 = -1;
        }
        iresult = v3;
    }
    else {
        iresult = 1;
    }
    int32_t iresult_2 = iresult;
    if (bisGreyedOut1 && bisGreyedOut2) {
        iresult_2 = -iresult;
    }
    if (!iresult_2) {
        if (apItem1->data->GetWorn(0)) {
            return -1;
        }
        else {
            return apItem2->data->GetWorn(0);
        }
    }
    return iresult_2;
}

void RepairMenu::Update() {
    FNVD_INLINED_LOCAL(BSSimpleList<ItemChange*>, pnewItemGroup, std::nullopt, std::nullopt);
    if (RepairMenu::pBrokenItem) {
        ::new (&pnewItemGroup) BSSimpleList<void*>;
        InventoryChanges* InventoryChanges_ = InventoryChanges::GetInventoryChanges(pPlayer);
        InventoryChanges_->GetRepairItemGroup(RepairMenu::pBrokenItem->pContainerObj, &pnewItemGroup);
        for (ItemChange*& rpItem : pnewItemGroup.ItemsUntilNull()) {
            for (int32_t iitemCt = 0; iitemCt < rpItem->iNumber; ++iitemCt) {
                ExtraDataList* m_item;
                float fcondition = rpItem->GetItemHealth(1);
                float fCeilcondition = FloatCeil(fcondition);
                char v21 = 0;
                if (rpItem->pExtraObjectList) {
                    m_item = rpItem->pExtraObjectList->m_item;
                }
                else {
                    m_item = 0;
                }
                if (m_item) {
                    if (m_item->HasWeaponMods()) {
                        v21 = 1;
                    }
                }
                if (!(fCeilcondition < 100.0)) {
                    TESBoundObject* FormFlags = rpItem->pContainerObj;
                    if (FormFlags == RepairMenu::pBrokenItem->pContainerObj) {
                        continue;
                    }
                }
                if (iitemCt) {
                    ItemChange* pnewItem_2;
                    ItemChange* v17 = static_cast<ItemChange*>(ourMalloc(sizeof(ItemChange)));
                    if (v17) {
                        pnewItem_2 = ::new (v17) ItemChange(rpItem->pContainerObj, 1);
                    }
                    else {
                        pnewItem_2 = 0;
                    }
                    for (ExtraDataList*& rpItem_2 : rpItem->pExtraObjectList->ItemsUntilNull()) {
                        pnewItem_2->pExtraObjectList->AddHead(rpItem_2);
                    }
                    const char* FullName = pnewItem_2->GetFullName();
                    RepairMenu::pMe->lbRepairList.AddItem(pnewItem_2, FullName, 0, 0);
                }
                else {
                    ItemChange* pnewItem = rpItem;
                    const char* v2 = pnewItem->GetFullName();
                    Tile* pnewTile = RepairMenu::pMe->lbRepairList.AddItem(pnewItem, v2, 0, 0);
                    if (pnewTile) {
                        TESBoundObject* v3 = pnewItem->pContainerObj;
                        if (v3 == RepairMenu::pBrokenItem->pContainerObj) {
                            ExtraDataList* v14;
                            ExtraDataList* v15;
                            if (pnewItem->pExtraObjectList) {
                                v15 = pnewItem->pExtraObjectList->m_item;
                            }
                            else {
                                v15 = 0;
                            }
                            if (RepairMenu::pBrokenItem->pExtraObjectList) {
                                v14 = RepairMenu::pBrokenItem->pExtraObjectList->m_item;
                            }
                            else {
                                v14 = 0;
                            }
                            if (v15 == v14) {
                                RepairMenu::pBrokenItem = pnewItem;
                                pnewTile->Set(RepairMenu::iUT_IsRepairItem, 1);
                                Tile::Value* pheight = RepairMenu::pMe->pTiles[2]->GetValue(4016);
                                Tile::Value* py = RepairMenu::pMe->pTiles[2]->GetValue(4002);
                                pheight->ClearActions();
                                pheight->AddAction(2000, pnewTile, 4016);
                                pheight->AddAction(2002, 1.0f);
                                pheight->CalculateValue(0);
                                py->ClearActions();
                                py->AddAction(2000, pnewTile, 4002);
                                py->CalculateValue(0);
                            }
                            else {
                                if (v21 || pnewItem->GetWorn(0)) {
                                    RepairMenu::pMe->lbRepairList.RemoveItem(pnewTile);
                                }
                            }
                        }
                    }
                }
            }
        }
        const char* v5 = sLanguage.String();
        if (BSstrcmp(v5, "ENGLISH")) {
            const char* v12 = sInventoryItems.String();
            Tile* ChildByName = RepairMenu::pMe->pMenu->GetChildByName("RM_Headline_Title");
            ChildByName->Set(4036, v12, 1);
            const char* v7 = sRepairSkill.String();
            RepairMenu::pMe->pTiles[0]->Set(RepairMenu::iUT_Title, v7, 1);
            const char* v8 = sInventoryCondition.String();
            RepairMenu::pMe->pTiles[4]->Set(RepairMenu::iUT_Title, v8, 1);
            const char* v9 = sInventoryCondition.String();
            RepairMenu::pMe->pTiles[9]->Set(RepairMenu::iUT_Title, v9, 1);
            const char* v10 = sRepairItem.String();
            RepairMenu::pMe->pTiles[11]->Set(4036, v10, 1);
            const char* v11 = sCancel.String();
            RepairMenu::pMe->pTiles[12]->Set(4036, v11, 1);
        }
        pnewItemGroup.~BSSimpleList<ItemChange*>();
    }
}
