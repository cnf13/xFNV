#include "Fallout/Interface/Menus/LevelUpMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSAbilityPerkEntry.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSPerkEntry.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/CharacterProgression.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"

#include <cstdint>
#include <limits>

// IsTileSelected's guarded static and its guard word.
static int32_t iUT_Selected;
static uint32_t iUT_Selected_guard;

// The game settings this file defines, with their IDs and values, in initialization order.
LevelUpMenu::PerkEffectList_t LevelUpMenu::PerkEffectList;
SettingT<INISettingCollection> bHideUnavailablePerks("bHideUnavailablePerks:Interface", false);

bool IsTileSelected(void* apThis, [[maybe_unused]] void* apEdx, Tile* apTile) {
    if ((iUT_Selected_guard & 1) == 0) {
        iUT_Selected_guard |= 1u;
        iUT_Selected = Tile::AddUserTrait("_selected", -1);
    }
    return iUT_Selected && apTile && apTile->IsTrue(iUT_Selected);
}

LevelUpMenu::LevelUpMenu() {
    g_levelUpMenu_Trait_BaseValue = Tile::AddUserTrait("_BaseValue", -1);
    g_levelUpMenu_Trait_ExtraValue = Tile::AddUserTrait("_ExtraValue", -1);
    LevelUpMenu::iUT_AddedValue = Tile::AddUserTrait("_AddedValue", -1);
    g_levelUpMenu_Trait_CurrentPage = Tile::AddUserTrait("_CurrentPage", -1);
    g_levelUpMenu_Trait_CurrPoints = Tile::AddUserTrait("_CurrPoints", -1);
    g_levelUpMenu_Trait_MaxPoints = Tile::AddUserTrait("_MaxPoints", -1);
    g_levelUpMenu_Trait_Rank = Tile::AddUserTrait("_Rank", -1);
    LevelUpMenu::iUT_Selected = Tile::AddUserTrait("_selected", -1);
    g_levelUpMenu_Trait_TextAlpha = Tile::AddUserTrait("_TextAlpha", -1);
}

LevelUpMenu::~LevelUpMenu() {
    LevelUpMenu::pMe = nullptr;
}

void LevelUpMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 9) {
        pTiles[aiID] = apTile;
    }
}

void LevelUpMenu::DoClick(int32_t aiID, Tile* apTarget) {
    switch (aiID) {
        case 6: {
            if (iCurrPoints[this->eCurrentPage] > 0) {
                Interface::PlayMenuSound(1);
            }
            iCurrPoints[this->eCurrentPage] = 0;
            pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[this->eCurrentPage]);
            LevelUpMenu::PageIndex ePage = this->eCurrentPage;
            if (ePage) {
                if (ePage == LevelUpMenu::PI_PERKS_PAGE) {
                    lbPerksList.Traverse(&LevelUpMenu::ResetPerksFunc, 0, std::numeric_limits<int32_t>::max());
                }
            }
            else {
                lbSkillsList.Traverse(&LevelUpMenu::ResetSkillsFunc, 0, std::numeric_limits<int32_t>::max());
            }
            UpdatePointCounter();
            break;
        }
        case 7: {
            SetCurrentPage(static_cast<LevelUpMenu::PageIndex>(this->eCurrentPage + 1));
            pTiles[7]->Set(4035, 0.0f, true);
            break;
        }
        case 8: {
            SetCurrentPage(static_cast<LevelUpMenu::PageIndex>(this->eCurrentPage - 1));
            pTiles[8]->Set(4035, 0.0f, true);
            break;
        }
        case 12: {
            if (IsTileSelected(&lbPerksList, nullptr, apTarget)) {
                lbPerksList.ToggleSelection(apTarget);
                ApplyPerkEffects(apTarget);
            }
            else if (apTarget->GetFloat(g_levelUpMenu_Trait_TextAlpha) == 255.0) {
                lbPerksList.DeselectAll(true);
                lbPerksList.Select(apTarget);
                ApplyPerkEffects(apTarget);
            }
            iCurrPoints[this->eCurrentPage] = lbPerksList.NumSelected();
            pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[this->eCurrentPage]);
            UpdatePointCounter();
            break;
        }
        case 13:
        case 14: {
            if (apTarget->IsTrue(4003)) {
                ActorValue::Index eindex = lbSkillsList.GetDataFor(apTarget->pParent);
                int32_t iDelta = 2 * (aiID != 13) - 1;
                iCurrPoints[this->eCurrentPage] += iDelta;
                pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[this->eCurrentPage]);
                if (pPlayer->GetClass()->IsTagSkill(eindex)) {
                    iDelta *= iSkillPointsTagSkillMult.Int();
                }
                float fAddedValue = apTarget->pParent->GetFloat(LevelUpMenu::iUT_AddedValue) +
                    static_cast<double>(iDelta);
                apTarget->pParent->Set(LevelUpMenu::iUT_AddedValue, fAddedValue, true);
                bool bOverridden = false;
                float fNewValue = pPlayer->GetBaseValueOverride(eindex, bOverridden) + static_cast<double>(iDelta);
                pPlayer->SetActorValue(eindex, fNewValue);
                SetDisplayString(apTarget->pParent);
                UpdatePointCounter();
            }
            break;
        }
        default: {
            return;
        }
    }
}

void LevelUpMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    switch (aiID) {
        case 11: {
            const char* pIcon = ActorValue::GetActorValueIcon(lbSkillsList.GetDataFor(apTarget));
            pTiles[3]->Set(4044, pIcon, true);
            const char* pDescription = ActorValue::GetActorValueDesc(lbSkillsList.GetDataFor(apTarget));
            pTiles[4]->Set(4036, pDescription, true);
            pTiles[3]->Set(4003, 1);
            pTiles[4]->Set(4003, 1);
            if (ActorValue::GetItemWeaponTypeBadge(lbSkillsList.GetDataFor(apTarget))) {
                pTiles[9]->Set(4003, 1);
                const char* pBadge = ActorValue::GetItemWeaponTypeBadge(lbSkillsList.GetDataFor(apTarget));
                pTiles[9]->Set(4044, pBadge, true);
            }
            else {
                pTiles[9]->Set(4003, 0);
            }
            Interface::PlayMenuSound(4);
            break;
        }
        case 12: {
            char acDescription[516];
            pTiles[3]->Set(4044, lbPerksList.GetDataFor(apTarget)->GetTextureName(), true);
            lbPerksList.GetDataFor(apTarget)->BuildDescriptionString(acDescription, 512u);
            pTiles[4]->Set(4036, acDescription, true);
            const char* pTexture = pTiles[3]->GetString(4044);
            if (BSstrlen(pTexture) <= 1) {
                pTiles[3]->Set(4003, 0);
            }
            else {
                pTiles[3]->Set(4003, 1);
            }
            pTiles[4]->Set(4003, 1);
            pTiles[9]->Set(4003, 0);
            Interface::PlayMenuSound(4);
            break;
        }
        case 13:
        case 14: {
            static_cast<ListBoxBase*>(&lbSkillsList)->Highlight(apTarget->pParent);
            DoEnter(11, apTarget->pParent);
            break;
        }
        default: {
            return;
        }
    }
}

void LevelUpMenu::DoIdle() {
    if (xFadeState == 4 && !pVATS.IsInVats() && !StartMenu::PauseMenuIsActive()) {
        BSAudio::QInstance().GetSoundHandleByName("MUSSuccess", 2305u).Play(false);
        PrepForVisibility(false);
    }
}

int32_t LevelUpMenu::GetClass() {
    return 1027;
}

bool LevelUpMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bhandled = false;
    switch (aiButton) {
        case 3: {
            if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE && lbSkillsList.GetHighlightedItem()) {
                Tile* parrowTile = static_cast<Tile*>(lbSkillsList.GetHighlightedItem())->GetChildByID(14);
                if (parrowTile->IsTrue(4003)) {
                    parrowTile->PlayTileSound(4043);
                    DoClick(14, parrowTile);
                }
                bhandled = true;
            }

            break;
        }
        case 4: {
            if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE && lbSkillsList.GetHighlightedItem()) {
                Tile* parrowTile = static_cast<Tile*>(lbSkillsList.GetHighlightedItem())->GetChildByID(13);
                if (parrowTile->IsTrue(4003)) {
                    parrowTile->PlayTileSound(4043);
                    DoClick(13, parrowTile);
                }
                bhandled = true;
            }

            break;
        }
        case 9: {
            if (eCurrentPage == LevelUpMenu::PI_PERKS_PAGE && lbPerksList.GetHighlightedItem()) {
                static_cast<Tile*>(lbPerksList.GetHighlightedItem())->PlayTileSound(4043);
                DoClick(12, static_cast<Tile*>(lbPerksList.GetHighlightedItem()));
                bhandled = true;
            }
            else if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE) {
                bhandled = DoGamepad(3, 1.0f);
            }
            break;
        }
        case 10: {
            if (eCurrentPage == LevelUpMenu::PI_PERKS_PAGE) {
                pTiles[8]->PlayTileSound(4043);
                DoClick(8, nullptr);
                bhandled = true;
            }
            else if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE) {
                bhandled = DoGamepad(4, 1.0f);
            }
            break;
        }
        default: {
            break;
        }
    }
    return bhandled;
}

bool LevelUpMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    bool bhandled = false;
    if (aiButton == 11 && pTiles[7]->IsTrue(4015)) {
        Interface::PlayMenuSound(1);
        DoClick(7, nullptr);
        return true;
    }
    return bhandled;
}

bool LevelUpMenu::IsWellFormedMenu() {
    for (int32_t i = 0; i < 10; ++i) {
        if (!pTiles[i]) {
            return false;
        }
    }
    return true;
}

void LevelUpMenu::SetDisplayString(Tile* apTile) {
    char acBuffer[16];
    double dValue = apTile->GetFloat(g_levelUpMenu_Trait_BaseValue);
    dValue = apTile->GetFloat(g_levelUpMenu_Trait_ExtraValue) + dValue;
    dValue = apTile->GetFloat(LevelUpMenu::iUT_AddedValue) + dValue;
    int32_t iValue = BSMAX(static_cast<int32_t>(dValue), 0);
    BSsprintf(acBuffer, 16u, "%d", BSMIN(iValue, 100));
    int32_t iDisplayTrait = Tile::AddUserTrait("_DisplayString", -1);
    apTile->Set(iDisplayTrait, acBuffer, true);
}

void LevelUpMenu::ApplyPerkEffects(Tile* apTile) {
    BGSPerk* pcurrPerk = lbPerksList.GetDataFor(apTile);
    uint8_t uccurrPerkRank = static_cast<int32_t>(apTile->GetFloat(g_levelUpMenu_Trait_Rank));
    BGSPerkEntry* pcurrPerkEntry = pcurrPerk->GetPerkEntryByRank(uccurrPerkRank);
    BGSPerkEntry* pprevRankEntry = uccurrPerkRank <= 1u ? nullptr : pcurrPerk->GetPerkEntryByRank(uccurrPerkRank - 1);
    if (pcurrPerkEntry && pcurrPerkEntry->GetType() == PERK_ENTRY_TYPE_ABILITY) {
        SpellItem* pAbility = static_cast<BGSAbilityPerkEntry*>(pcurrPerkEntry)->pAbility;
        SpellItem* pprevAbility = pprevRankEntry ? static_cast<BGSAbilityPerkEntry*>(pprevRankEntry)->pAbility
                                                 : nullptr;
        LevelUpMenu::PerkEffectList.Clear(true);
        // The upcast to the spell's effect list tests the spell for null.
        BSSimpleList<EffectItem*>* peffectObj = pAbility;
        for (EffectItem*& rpItem : peffectObj->ItemsUntilNull()) {
            EffectItemData perkEffect;
            perkEffect.iActorValue = rpItem->GetActorValueIndex();
            perkEffect.iMagnitude = rpItem->GetMagnitude();
            if (perkEffect.iActorValue != ActorValue::VALUE_NONE) {
                BSSimpleList<EffectItem*>* pprevRankEffectObj = pprevAbility;
                for (EffectItem*& rpPrevItem : pprevRankEffectObj->ItemsUntilNull()) {
                    if (rpPrevItem->GetActorValueIndex() == perkEffect.iActorValue) {
                        perkEffect.iMagnitude -= rpPrevItem->GetMagnitude();
                    }
                }
                if (!apTile->IsTrue(LevelUpMenu::iUT_Selected)) {
                    perkEffect.iMagnitude = -perkEffect.iMagnitude;
                }
                if (rpItem->pEffectSetting->IsDetrimental(perkEffect.iActorValue)) {
                    perkEffect.iMagnitude = -perkEffect.iMagnitude;
                }
                LevelUpMenu::PerkEffectList.Add(perkEffect);
            }
        }
    }

    lbSkillsList.Traverse(&LevelUpMenu::PerkUpdateFunc, 0, std::numeric_limits<int32_t>::max());
}

void LevelUpMenu::SetCurrentPage(LevelUpMenu::PageIndex aePage) {
    int32_t iEndPageTrait = Tile::TextToTrait("_EndPage");
    if (pMenu->GetFloat(iEndPageTrait) < static_cast<double>(aePage)) {
        LevelUpMenu::Close();
    }
    else {
        this->eCurrentPage = aePage;
        pMenu->Set(g_levelUpMenu_Trait_CurrentPage, static_cast<int32_t>(aePage));
        pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[aePage]);
        pMenu->Set(g_levelUpMenu_Trait_MaxPoints, iMaxPoints[aePage]);
        UpdatePointCounter();
        LevelUpMenu::PageIndex ePage = this->eCurrentPage;
        if (ePage) {
            if (ePage == LevelUpMenu::PI_PERKS_PAGE) {
                lbPerksList
                    .Traverse(&LevelUpMenu::SetPerkAlphaIfRequirementsNotMet, 0, std::numeric_limits<int32_t>::max());
                // Sort is called through a pointer so that the call stays virtual; a call on the member itself would be
                // devirtualised.
                ListBox<BGSPerk*>* pPerksList = &lbPerksList;
                pPerksList->Sort(&BGSPerk::CompareForSorting);
                lbSkillsList.SaveListState();
                lbPerksList.RestoreListState(true);
            }
        }
        else {
            lbSkillsList.Traverse(&LevelUpMenu::UpdateSkillsFunc, 0, std::numeric_limits<int32_t>::max());
            lbPerksList.SaveListState();
            lbSkillsList.RestoreListState(true);
        }
    }
}

void LevelUpMenu::InitLists() {
    int32_t itotalSkillPoints = 0;
    int32_t inumSkills = 0;
    lbSkillsList.Initialize(pTiles[1], "LUM_SkillTemplate");
    for (int32_t ivalueCt = 0; ivalueCt < ActorValue::GetSectionCount(ActorValue::SKILL); ++ivalueCt) {
        ActorValue::Index currValue = ActorValue::ToActorValue(ActorValue::SKILL, ivalueCt);
        if (!ActorValue::IsFlagSet(currValue, ActorValue::DEPRECATED)) {
            Tile* pnew = lbSkillsList.AddItem(currValue, ActorValue::GetActorValueName(currValue), nullptr, nullptr);
            pnew->SetName(ActorValue::GetActorValueName(currValue));
            pnew->Set(4010, 11);
            pnew->Set(LevelUpMenu::iUT_AddedValue, 0.0f, true);
            int32_t iBaseValue = pPlayer->GetPermanentActorValue(currValue);
            pnew->Set(g_levelUpMenu_Trait_BaseValue, iBaseValue);
            itotalSkillPoints += pPlayer->GetPermanentActorValue(currValue);
            ++inumSkills;
        }
    }
    // Sort is called through a pointer so that the call goes through the list's vtable; a call on the member itself
    // would be devirtualised.
    ListBox<ActorValue::Index>* pSkillsList = &lbSkillsList;
    pSkillsList->Sort(&ActorValue::CompareForSorting);
    int32_t iPoints = BSMIN(iMaxPoints[0], 100 * inumSkills - itotalSkillPoints);
    iMaxPoints[0] = BSMAX(iPoints, 0);
    lbPerksList.Initialize(pTiles[2], "LUM_PerkTemplate");
    for (BSSimpleList<BGSPerk*>* pcurrentPerk = &AvailablePerks; pcurrentPerk && pcurrentPerk->m_item;
         pcurrentPerk = pcurrentPerk->m_pkNext) {
        char acName[68];
        int32_t icurrRank = pPlayer->GetPerkRank(pcurrentPerk->m_item, false);
        BSstrcpy(acName, 64, pcurrentPerk->m_item->GetFullName());
        if (!BSstrlen(acName)) {
            BSstrcpy(acName, 64, sMissingName.String());
        }
        if (pcurrentPerk->m_item->GetNumRanks() > 1u && icurrRank > 0) {
            BSsprintf(acName, 64, "%s (%d)", acName, icurrRank + 1);
        }
        Tile* pnew = lbPerksList.AddItem(pcurrentPerk->m_item, acName, nullptr, nullptr);
        pnew->Set(4010, 12);
        pnew->Set(g_levelUpMenu_Trait_Rank, icurrRank + 1);
    }
}

void LevelUpMenu::UpdatePointCounter() {
    const char* apFormat[2];
    apFormat[0] = sLevelUpSkillCounterPl.String();
    apFormat[1] = sLevelUpPerkCounterPl.String();
    int32_t iPointsLeft = iMaxPoints[eCurrentPage] - iCurrPoints[eCurrentPage];
    if (iPointsLeft) {
        if (iPointsLeft == 1) {
            const char* pText = eCurrentPage ? sLevelUpPerkCounter.String() : sLevelUpSkillCounter.String();
            pTiles[5]->Set(4036, pText, true);
            pTiles[5]->Set(4003, 1);
        }
        else {
            char acBuffer[68];
            BSsprintf(acBuffer, 64u, apFormat[eCurrentPage], iPointsLeft);
            pTiles[5]->Set(4036, acBuffer, true);
            pTiles[5]->Set(4003, 1);
        }
    }
    else {
        pTiles[5]->Set(4003, 0);
    }
}

int32_t LevelUpMenu::GetClassStatic() {
    return 1027;
}

// Both failing paths return null: a null Instance(), or bFirstInit not set.
Tile* LevelUpMenu::Create() {
    if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
        return LevelUpMenu::CreateMenu();
    }
    return nullptr;
}

Tile* LevelUpMenu::CreateMenu() {
    Tile* pOldMenu = Menu::GetMenuByClass(1027);
    InterfaceManager* pManager = InterfaceManager::Instance();
    if (pOldMenu) {
        delete pOldMenu;
    }
    Tile* ptile = pManager->pMenusRoot->ReadFile("Data\\Menus\\levelup_menu.xml");
    LevelUpMenu::pMe = static_cast<LevelUpMenu*>(ptile->GetMenu());
    if (LevelUpMenu::pMe) {
        int32_t iMenuClass = LevelUpMenu::pMe->GetClass();
        if (iMenuClass == LevelUpMenu::GetClassStatic()) {
            LevelUpMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), false);
            if (LevelUpMenu::pMe->IsWellFormedMenu()) {
                char acTitle[64];
                if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
                    float fDepth = Tile::GetMaximumDepth();
                    ptile->Set(4013, fDepth, true);
                }
                int32_t iClass = LevelUpMenu::GetClassStatic();
                InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_LEVELING, iClass, 512u);
                int32_t iLevel = pPlayer->GetLevel();
                BSsprintf(acTitle, 64, sLevelUpTitleText.String(), iLevel);
                LevelUpMenu::pMe->pTiles[0]->Set(4036, acTitle, true);
                const char* pResetText = sReset.String();
                LevelUpMenu::pMe->pTiles[6]->Set(4036, pResetText, true);
                const char* pContinueText = sContinue.String();
                int32_t iTitle0Trait = Tile::TextToTrait("_Title_0");
                LevelUpMenu::pMe->pTiles[7]->Set(iTitle0Trait, pContinueText, true);
                const char* pDoneText = sDone.String();
                int32_t iTitle1Trait = Tile::TextToTrait("_Title_1");
                LevelUpMenu::pMe->pTiles[7]->Set(iTitle1Trait, pDoneText, true);
                ptile->Poke(Tile::TextToTrait("_CurrentPage"), 1.0f);
                const char* pBackText = sBack.String();
                LevelUpMenu::pMe->pTiles[8]->Set(4036, pBackText, true);
                int32_t iSkillPoints = GamePlayFormulas::GetLevelUpSkillPoints_0(pPlayer);
                float fSkillPoints = static_cast<float>(iSkillPoints);
                BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_GAINED_SKILL_POINTS, pPlayer, &fSkillPoints);
                LevelUpMenu::pMe->iCurrPoints[0] = 0;
                LevelUpMenu::pMe->iMaxPoints[0] = static_cast<int32_t>(fSkillPoints);
                uint16_t usLevel = pPlayer->GetLevel();
                if (!(usLevel % iLevelsPerPerk.Int())) {
                    for (BSSimpleList<BGSPerk*>* pIter = pDataHandler->GetPerkList(); pIter && pIter->m_item;
                         pIter = pIter->GetNext()) {
                        BGSPerk* pPerk = pIter->m_item;
                        if (pPerk->GetLevel() > 0) {
                            int32_t iRank = pPlayer->GetPerkRank(pPerk, false);
                            if (iRank < pPerk->GetNumRanks() &&
                                !pPerk->Data.bTrait &&
                                pPerk->IsPerkAttainable(pPlayer) &&
                                pPerk->GetIsPlayable()) {
                                char bAdd = 1;
                                if (bHideUnavailablePerks.Bool()) {
                                    if (!pPerk->IsPerkAvailable(pPlayer)) {
                                        bAdd = 0;
                                    }
                                    else {
                                        int32_t iPerkLevel = pPerk->GetLevel();
                                        if (iPerkLevel > pPlayer->GetLevel()) {
                                            bAdd = 0;
                                        }
                                    }
                                }
                                if (bAdd) {
                                    LevelUpMenu::pMe->AvailablePerks.AddTail(pPerk);
                                }
                            }
                        }
                    }
                }
                LevelUpMenu::pMe->iCurrPoints[1] = 0;
                LevelUpMenu::pMe->iMaxPoints[1] = 1;
                if (!LevelUpMenu::pMe->AvailablePerks.ItemsInList()) {
                    int32_t iEndPageTrait = Tile::TextToTrait("_EndPage");
                    LevelUpMenu::pMe->pMenu->Set(iEndPageTrait, 0);
                }
                LevelUpMenu::pMe->InitLists();
                LevelUpMenu::pMe->SetCurrentPage(LevelUpMenu::PI_SKILLS_PAGE);
                return ptile;
            }

            Warning("MENUS: Level Up Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }
    }

    if (LevelUpMenu::pMe && LevelUpMenu::pMe->pMenu && LevelUpMenu::pMe) {
        delete LevelUpMenu::pMe;
    }

    return nullptr;
}

void LevelUpMenu::SetPerkAlphaIfRequirementsNotMet(Tile* apTile, BGSPerk* apPerk) {
    float fAlpha = 128.0f;
    if (apPerk->IsPerkAvailable(pPlayer)) {
        int32_t iPerkLevel = apPerk->GetLevel();
        if (iPerkLevel <= pPlayer->Actor::GetLevel()) {
            fAlpha = 255.0f;
        }
    }
    apTile->Set(g_levelUpMenu_Trait_TextAlpha, fAlpha, true);
}

// HandleClick's reset traverses the perks list with it (and the skills list with ResetSkillsFunc).
void LevelUpMenu::ResetPerksFunc(Tile* apTile, BGSPerk* apPerk) {
    if (apTile->IsTrue(LevelUpMenu::iUT_Selected)) {
        apTile->Set(LevelUpMenu::iUT_Selected, 0);
        pMe->ApplyPerkEffects(apTile);
    }
}

// SetCurrentPage traverses the skills list with this function (and the perks list with
// SetPerkAlphaIfRequirementsNotMet).
void LevelUpMenu::UpdateSkillsFunc(Tile* apTile, ActorValue::Index aeIndex) {
    pMe->SetDisplayString(apTile);
}

void LevelUpMenu::Close() {
    HUDMainMenu::ClearXPMeterFinishedMoving();
    if (LevelUpMenu::pMe) {
        pPlayer->GetCharacterProgressionInfo()->FinishLevelUp();
        LevelUpMenu::pMe->lbPerksList
            .Traverse(&LevelUpMenu::IncreaseSelectedPerkRank, 0, std::numeric_limits<int32_t>::max());
        if (pPlayer->GetCharacterProgressionInfo()->GetReadyToLevelUp()) {
            pPlayer->UpdateMagic(true, false);
        }
        StatsMenu::UpdatePlayerName();
        StatsMenu::UpdatePlayerTitle();
        LevelUpMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        LevelUpMenu::pMe->StartFadeOut();
        LevelUpMenu::pMe = nullptr;
    }
}

void LevelUpMenu::ResetSkillsFunc(Tile* apTile, ActorValue::Index aeIndex) {
    bool bOverridden = false;
    double dBaseValue = pPlayer->GetBaseValueOverride(aeIndex, bOverridden);
    float fNewValue = dBaseValue - apTile->GetFloat(LevelUpMenu::iUT_AddedValue);
    pPlayer->SetActorValue(aeIndex, fNewValue);
    apTile->Set(LevelUpMenu::iUT_AddedValue, static_cast<float>(0.0), true);
    LevelUpMenu::pMe->SetDisplayString(apTile);
}

void LevelUpMenu::PerkUpdateFunc(Tile* apTile, ActorValue::Index aeIndex) {
    for (EffectItemData& rEffect : LevelUpMenu::PerkEffectList) {
        if (rEffect.iActorValue == aeIndex) {
            double dValue = apTile->GetFloat(g_levelUpMenu_Trait_BaseValue);
            dValue = apTile->GetFloat(g_levelUpMenu_Trait_ExtraValue) + dValue;
            int32_t itotalValue = static_cast<int32_t>(apTile->GetFloat(LevelUpMenu::iUT_AddedValue) + dValue);
            int32_t iUT_Overflow = Tile::TextToTrait("_OverflowValue");
            if (apTile->GetFloat(iUT_Overflow) != 0.0) {
                rEffect.iMagnitude += static_cast<int32_t>(apTile->GetFloat(iUT_Overflow));
                apTile->Set(iUT_Overflow, 0.0f, true);
            }
            double dNewValue = static_cast<double>(rEffect.iMagnitude + itotalValue);
            int32_t iMaxTrait = Tile::TextToTrait("_MaxValue");
            if (apTile->GetFloat(iMaxTrait) < dNewValue) {
                dNewValue = static_cast<double>(rEffect.iMagnitude + itotalValue);
                iMaxTrait = Tile::TextToTrait("_MaxValue");
                int32_t idifference = static_cast<int32_t>(dNewValue - apTile->GetFloat(iMaxTrait));
                iUT_Overflow = Tile::TextToTrait("_OverflowValue");
                apTile->Set(iUT_Overflow, idifference);
                rEffect.iMagnitude -= idifference;
            }
            else {
                dNewValue = static_cast<double>(rEffect.iMagnitude + itotalValue);
                int32_t iMinTrait = Tile::TextToTrait("_MinValue");
                if (apTile->GetFloat(iMinTrait) > dNewValue) {
                    dNewValue = static_cast<double>(rEffect.iMagnitude + itotalValue);
                    iMinTrait = Tile::TextToTrait("_MinValue");
                    int32_t idifference = static_cast<int32_t>(dNewValue - apTile->GetFloat(iMinTrait));
                    iUT_Overflow = Tile::TextToTrait("_OverflowValue");
                    apTile->Set(iUT_Overflow, idifference);
                    rEffect.iMagnitude -= idifference;
                }
            }
            double dExtraValue = apTile->GetFloat(g_levelUpMenu_Trait_ExtraValue);
            float fNewExtraValue = static_cast<double>(rEffect.iMagnitude) + dExtraValue;
            apTile->Set(g_levelUpMenu_Trait_ExtraValue, fNewExtraValue, true);
        }
    }
}

void LevelUpMenu::IncreaseSelectedPerkRank(Tile* apTile, BGSPerk* apPerk) {
    if (apTile->IsTrue(LevelUpMenu::iUT_Selected)) {
        uint8_t ucRank = pPlayer->GetPerkRank(apPerk, false);
        pPlayer->AddPerk(apPerk, ucRank + 1, false);
    }
}
