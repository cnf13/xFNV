#include "Fallout/Interface/Menus/TraitMenu.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSAbilityPerkEntry.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPerkEntry.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/MenuManager.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Interface/Menus/SleepWaitMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Misc/Main.h"

#include <cstdint>

TraitMenu::PerkEffectList_t TraitMenu::PerkEffectList;

void ClearSelectedTrait(Tile* a1, BGSPerk* a2) {
    if (a1->IsTrue(g_traitMenuSelected)) {
        a1->Set(g_traitMenuSelected, 0);
        TraitMenu::pMe->ApplyPerkEffects(a1);
    }
}

void UpdatePerkTextAlpha(Tile* a1, BGSPerk* a2) {
    int32_t level;
    float v3;
    if (a2->IsPerkAvailable(pPlayer) &&
        ((level = (int32_t)a2->GetLevel()), (level <= ((int32_t)pPlayer->GetLevel())))) {
        v3 = 255.0f;
    }
    else {
        v3 = 0x1p+7f;
    }
    a1->Set(TraitMenu::iUT_TextAlpha, v3, true);
}

TraitMenu::TraitMenu() : Menu(), lbPerksList(), AvailablePerks() {
    iUT_BaseValue = Tile::AddUserTrait("_BaseValue", -1);
    iUT_ExtraValue = Tile::AddUserTrait("_ExtraValue", -1);
    iUT_AddedValue = Tile::AddUserTrait("_AddedValue", -1);
    g_traitMenuCurrPoints = Tile::AddUserTrait("_CurrPoints", -1);
    g_traitMenuMaxPoints = Tile::AddUserTrait("_MaxPoints", -1);
    g_traitMenuRank = Tile::AddUserTrait("_Rank", -1);
    g_traitMenuSelected = Tile::AddUserTrait("_selected", -1);
    TraitMenu::iUT_TextAlpha = Tile::AddUserTrait("_TextAlpha", -1);
}

TraitMenu::~TraitMenu() {
    TraitMenu::pMe = nullptr;
}

void TraitMenu::AttachTileByID(int32_t a1, Tile* a2) {
    if (static_cast<uint32_t>(a1) <= 8) {
        pTiles[a1] = a2;
    }
}

void TraitMenu::DoClick(int32_t a1, Tile* a2) {
    switch (a1) {
        case 5: {
            Interface::PlayMenuSound(1);
            iCurrPoints = 0;
            pMenu->Set(g_traitMenuCurrPoints, iCurrPoints);
            lbPerksList.Traverse(&ClearSelectedTrait, 0, 0x7FFFFFFF);
            UpdatePointCounter();
            break;
        }
        case 6: {
            Close();
            break;
        }
        case 10: {
            if (IsTileSelected(&lbPerksList, nullptr, a2)) {
                lbPerksList.ToggleSelection(a2);
                ApplyPerkEffects(a2);
            }
            else {
                if (a2->GetFloat(TraitMenu::iUT_TextAlpha) == 255.0 && iCurrPoints < iMaxPoints) {
                    lbPerksList.Select(a2);
                    ApplyPerkEffects(a2);
                }
            }
            iCurrPoints = lbPerksList.NumSelected();
            pMenu->Set(g_traitMenuCurrPoints, iCurrPoints);
            UpdatePointCounter();
            break;
        }
    }
}

void TraitMenu::DoEnter(int32_t a1, Tile* a2) {
    if (a1 == 10) {
        char Src[516];
        pTiles[2]->Set(4044, lbPerksList.GetDataFor(a2)->GetTextureName(), true);
        lbPerksList.GetDataFor(a2)->BuildDescriptionString(Src, 512u);
        pTiles[3]->Set(4036, Src, true);
        const char* String = pTiles[2]->GetString(4044);
        if (BSstrlen(String) <= 1) {
            pTiles[2]->Set(4003, 0);
        }
        else {
            pTiles[2]->Set(4003, 1);
        }
        pTiles[3]->Set(4003, 1);
        pTiles[7]->Set(4003, 0);
        Interface::PlayMenuSound(4);
        Tile* pscrollbar = pTiles[8];
        int32_t iUT_CurrentValue = Tile::TextToTrait("_current_value");
        pscrollbar->Set(iUT_CurrentValue, 0.0f, false);
        int32_t v4 = Tile::TextToTrait("_SetInCode");
        pscrollbar->Poke(v4, 1.0f);
    }
}

void TraitMenu::DoIdle() {
    if ((xFadeState == 4) && (!(&pVATS)->IsInVats())) {
        Menu::PrepForVisibility(false);
    }
}

int32_t TraitMenu::GetClass() {
    return 1084;
}

bool TraitMenu::DoGamepad(int32_t a1, float a2) {
    static int32_t inext;
    bool bhandled = false;
    if ((a1 == 9) && (lbPerksList.GetHighlightedItem())) {
        Tile* v3 = static_cast<Tile*>(lbPerksList.GetHighlightedItem());
        v3->PlayTileSound(4043);
        Tile* v4 = static_cast<Tile*>(lbPerksList.GetHighlightedItem());
        DoClick(10, v4);
        bhandled = true;
    }

    if (Interface::Has360Controller() && pTiles[8]->IsTrue(4003)) {
        int32_t iscrollDelta = 0;
        Controls* Controls_ = appMain->GetControls();
        int32_t iyAxis = Controls_->GetControllerValue(0, 10);
        if (NiAbs(static_cast<float>(iyAxis)) <= 8689.0) {
            iyAxis = 0;
        }
        if (iyAxis) {
            iscrollDelta = 2 * (iyAxis < 0) - 1;
        }
        if (iscrollDelta && BSGetTickCount() > inext) {
            Tile* pscrollbar = pTiles[8];
            int32_t iUT_CurrentValue = Tile::TextToTrait("_current_value");
            int32_t icurrScrollbarValue = static_cast<int32_t>(pscrollbar->GetFloat(iUT_CurrentValue));
            pscrollbar->Set(iUT_CurrentValue, static_cast<float>(iscrollDelta + icurrScrollbarValue), false);
            if (static_cast<double>(icurrScrollbarValue) != pscrollbar->GetFloat(iUT_CurrentValue)) {
                int32_t v6 = Tile::TextToTrait("_SetInCode");
                pscrollbar->Poke(v6, 1.0f);
            }
            inext = (inext != 0 ? 50 : 500) + BSGetTickCount();
        }
        else {
            if (!iscrollDelta) {
                inext = 0;
            }
        }
    }
    return bhandled;
}

bool TraitMenu::DoGamepadUpEvent(int32_t a1, float a2) {
    bool v5 = false;
    if (a1 == 11 && pTiles[6]->IsTrue(4015)) {
        Interface::PlayMenuSound(1);
        DoClick(6, nullptr);
        return true;
    }
    return v5;
}

bool TraitMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 9; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

void TraitMenu::ApplyPerkEffects(Tile* apTile) {
    BGSPerk* pcurrPerk = lbPerksList.GetDataFor(apTile);
    uint8_t uccurrPerkRank = static_cast<int32_t>(apTile->GetFloat(g_traitMenuRank));
    BGSPerkEntry* pcurrPerkEntry = pcurrPerk->GetPerkEntryByRank(uccurrPerkRank);
    BGSPerkEntry* pprevRankEntry;
    if (uccurrPerkRank <= 1u) {
        pprevRankEntry = nullptr;
    }
    else {
        pprevRankEntry = pcurrPerk->GetPerkEntryByRank(uccurrPerkRank - 1);
    }
    if (pcurrPerkEntry && pcurrPerkEntry->GetType() == PERK_ENTRY_TYPE_ABILITY) {
        // Both entries are taken as ability entries: pAbility of the previous rank's entry is read without testing its
        // type, as the game does.
        SpellItem* peffectList = static_cast<BGSAbilityPerkEntry*>(pcurrPerkEntry)->pAbility;
        SpellItem* pprevRankEffectList;
        if (pprevRankEntry) {
            pprevRankEffectList = static_cast<BGSAbilityPerkEntry*>(pprevRankEntry)->pAbility;
        }
        else {
            pprevRankEffectList = nullptr;
        }
        TraitMenu::PerkEffectList.Clear(true);
        // The effect list is the spell's BSSimpleList<EffectItem*> base; the conversion keeps the null test, as the
        // game does.
        for (BSSimpleList<EffectItem*>* peffectObj = peffectList; peffectObj && peffectObj->m_item;
             peffectObj = peffectObj->GetNext()) {
            EffectItemData perkEffect;
            perkEffect.iActorValue = peffectObj->m_item->GetActorValueIndex();
            perkEffect.iMagnitude = peffectObj->m_item->GetMagnitude();
            if (perkEffect.iActorValue == ActorValue::VALUE_NONE) {
                continue;
            }
            for (BSSimpleList<EffectItem*>* pprevRankEffectObj = pprevRankEffectList;
                 pprevRankEffectObj && pprevRankEffectObj->m_item; pprevRankEffectObj = pprevRankEffectObj->GetNext()) {
                if (pprevRankEffectObj->m_item->GetActorValueIndex() == perkEffect.iActorValue) {
                    perkEffect.iMagnitude -= pprevRankEffectObj->m_item->GetMagnitude();
                }
            }
            if (!apTile->IsTrue(g_traitMenuSelected)) {
                perkEffect.iMagnitude = -perkEffect.iMagnitude;
            }
            if (peffectObj->m_item->pEffectSetting->IsDetrimental(perkEffect.iActorValue)) {
                perkEffect.iMagnitude = -perkEffect.iMagnitude;
            }
            TraitMenu::PerkEffectList.Add(perkEffect);
        }
    }
}

// Fills the perk list box with the available perks, each named with its next rank where it has several.
void TraitMenu::InitLists() {
    lbPerksList.Initialize(pTiles[1], "LUM_PerkTemplate");
    for (BSSimpleList<BGSPerk*>* pcurrentPerk = &AvailablePerks; pcurrentPerk && pcurrentPerk->m_item;
         pcurrentPerk = pcurrentPerk->m_pkNext) {
        char Dst[64];
        int32_t icurrRank = pPlayer->GetPerkRank(pcurrentPerk->m_item, false);
        BSstrcpy(Dst, 64, pcurrentPerk->m_item->TESFullName::GetFullName());
        if (!BSstrlen(Dst)) {
            BSstrcpy(Dst, 64, sMissingName.String());
        }
        if (pcurrentPerk->m_item->GetNumRanks() > 1u && icurrRank > 0) {
            BSsprintf(Dst, 64, "%s (%d)", Dst, icurrRank + 1);
        }
        Tile* pnew = lbPerksList.AddItem(pcurrentPerk->m_item, Dst, nullptr, nullptr);
        pnew->Set(Tile::kTileValue_ID, 10);
        pnew->Set(g_traitMenuRank, icurrRank + 1);
    }
}

void TraitMenu::UpdatePointCounter() {
    char Src[68];
    int32_t v2 = iMaxPoints - iCurrPoints;
    if (v2 == 1) {
        BSsprintf(Src, 64, sTraitMenuCounterSingular.String(), 1);
    }
    else {
        BSsprintf(Src, 64, sTraitMenuCounter.String(), v2);
    }
    pTiles[4]->Set(4036, Src, true);
    pTiles[4]->Set(4003, 1);
}

Tile* TraitMenu::Create() {
    int32_t v0;
    // pPerk's address goes to AddTail.
    BGSPerk* pPerk;
    Tile* pold_menu_tile = Menu::GetMenuByClass(1084);
    InterfaceManager* pin = InterfaceManager::Instance();
    if (pold_menu_tile) {
        delete pold_menu_tile;
    }
    Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\trait_menu.xml");
    TraitMenu::pMe = static_cast<TraitMenu*>(ptile->GetMenu());
    if (TraitMenu::pMe && (v0 = TraitMenu::pMe->GetClass(), v0 == TraitMenu::GetClassStatic())) {
        TraitMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
        if (TraitMenu::pMe->IsWellFormedMenu()) {
            char Src[64];
            if (ptile->GetFloat(Tile::kTileValue_StackingType) == 6006.0 ||
                ptile->GetFloat(Tile::kTileValue_StackingType) == 102.0) {
                float afValue = Tile::GetMaximumDepth();
                ptile->Set(Tile::kTileValue_Depth, afValue, true);
            }
            int32_t nTraitPoints = iTraitMenuMaxNumTraits.Int();
            TraitMenu::pMe->iCurrPoints = 0;
            TraitMenu::pMe->iMaxPoints = nTraitPoints;
            if (nTraitPoints == 1) {
                BSsprintf(Src, 64, sTraitMenuTitleTextSingular.String(), 1);
            }
            else {
                BSsprintf(Src, 64, sTraitMenuTitleText.String(), nTraitPoints);
            }
            TraitMenu::pMe->pTiles[0]->Set(Tile::kTileValue_String, Src, true);
            const char* v2 = sReset.String();
            TraitMenu::pMe->pTiles[5]->Set(Tile::kTileValue_String, v2, true);
            const char* afValuea = sDone.String();
            int32_t v3 = Tile::TextToTrait("_Title");
            TraitMenu::pMe->pTiles[6]->Set(v3, afValuea, true);
            for (BSSimpleList<BGSPerk*>* pIter = pDataHandler->GetPerkList(); pIter && pIter->m_item;
                 pIter = pIter->GetNext()) {
                pPerk = pIter->m_item;
                if (pPerk->GetLevel() > 0) {
                    int32_t v4 = pPlayer->GetPerkRank(pPerk, false);
                    if (v4 < pPerk->GetNumRanks() && pPerk->GetIsTrait() && pPerk->IsPerkAttainable(pPlayer) &&
                        pPerk->GetIsPlayable()) {
                        char v8 = 1;
                        if (bHideUnavailablePerks.Bool()) {
                            int32_t Level;
                            if (!pPerk->IsPerkAvailable(pPlayer) ||
                                (Level = pPerk->GetLevel(), Level > pPlayer->GetLevel())) {
                                v8 = 0;
                            }
                        }
                        if (v8) {
                            TraitMenu::pMe->AvailablePerks.AddTail(pPerk);
                        }
                    }
                }
            }
            TraitMenu::pMe->InitLists();
            TraitMenu::pMe->pMenu->Set(g_traitMenuCurrPoints, TraitMenu::pMe->iCurrPoints);
            TraitMenu::pMe->pMenu->Set(g_traitMenuMaxPoints, TraitMenu::pMe->iMaxPoints);
            TraitMenu::pMe->UpdatePointCounter();
            TraitMenu::pMe->lbPerksList.Traverse(&UpdatePerkTextAlpha, 0, 0x7FFFFFFF);
            // Sort is called through a pointer so that the call stays virtual; a call on the member itself would be
            // devirtualised.
            ListBox<BGSPerk*>* plbPerks = &TraitMenu::pMe->lbPerksList;
            plbPerks->Sort(&TraitMenu::SortPerksFunc);
            TraitMenu::pMe->lbPerksList.RestoreListState(true);
            return ptile;
        }

        Warning("MENUS: Trait Menu Creation Failed... Are your menu and art resources up to date?");
        return nullptr;
    }

    if ((TraitMenu::pMe) && (TraitMenu::pMe->pMenu) && (TraitMenu::pMe)) {
        delete TraitMenu::pMe;
    }

    return nullptr;
}

void TraitMenu::Close() {
    HUDMainMenu::ClearXPMeterFinishedMoving();
    if (TraitMenu::pMe) {
        TraitMenu::pMe->lbPerksList.Traverse(&TraitMenu::IncreaseSelectedTraitRank, 0, 0x7FFFFFFF);
        CharacterProgression* CharacterProgressionInfo = pPlayer->GetCharacterProgressionInfo();
        if (CharacterProgressionInfo->GetReadyToLevelUp()) {
            pPlayer->UpdateMagic(true, false);
        }
        StatsMenu::UpdatePlayerName();
        StatsMenu::UpdatePlayerTitle();
        TraitMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        TraitMenu::pMe->StartFadeOut();
        TraitMenu::pMe = nullptr;
    }
}

int32_t TraitMenu::GetClassStatic() {
    return 1084;
}

void TraitMenu::IncreaseSelectedTraitRank(Tile* a1, BGSPerk* a2) {
    if (a1->IsTrue(g_traitMenuSelected)) {
        uint8_t v2 = pPlayer->GetPerkRank(a2, false);
        pPlayer->AddPerk(a2, v2 + 1, false);
    }
}

int32_t TraitMenu::SortPerksFunc(ListBoxItem<BGSPerk*>* apItem1, ListBoxItem<BGSPerk*>* apItem2) {
    double Float = apItem1->pTile->GetFloat(TraitMenu::iUT_TextAlpha);
    if (apItem2->pTile->GetFloat(TraitMenu::iUT_TextAlpha) > Float) {
        return 1;
    }
    double v7 = apItem1->pTile->GetFloat(TraitMenu::iUT_TextAlpha);
    if (apItem2->pTile->GetFloat(TraitMenu::iUT_TextAlpha) < v7) {
        return -1;
    }
    int32_t Level = apItem1->data->GetLevel();
    if (Level < apItem2->data->GetLevel()) {
        return -1;
    }
    int32_t v3 = apItem1->data->GetLevel();
    if (v3 > apItem2->data->GetLevel()) {
        return 1;
    }
    const char* String = apItem2->pTile->GetString(4036);
    const char* v4 = apItem1->pTile->GetString(4036);
    return BSstrcmp(v4, String);
}
