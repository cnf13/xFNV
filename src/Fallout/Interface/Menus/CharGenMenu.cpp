#include "Fallout/Interface/Menus/CharGenMenu.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESClass.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/HUDMainMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"

#include <cmath>
#include <cstdint>
#include <limits>

char ToggleSelection(ListBox<ActorValue::Index>* apThis, [[maybe_unused]] void* apEdx, Tile* apTile) {
    return apThis->SetSelection(apTile, ListBox<ActorValue::Index>::LBSM_TOGGLE);
}

CharGenMenu::CharGenMenu()
  : Menu(), iCurrPoints(0), fPrevHighlightIndex(-1.0f), bOnlyAddValues(0), bShowTaggedSkills(0) {
    g_CharGenMenuTrait_BaseValue = Tile::AddUserTrait("_BaseValue", -1);
    g_CharGenMenuTrait_ExtraValue = Tile::AddUserTrait("_ExtraValue", -1);
    CharGenMenu::iUT_AddedValue = Tile::AddUserTrait("_AddedValue", -1);
    g_CharGenMenuTrait_CurrPoints = Tile::AddUserTrait("_CurrPoints", -1);
    g_CharGenMenuTrait_MaxPoints = Tile::AddUserTrait("_MaxPoints", -1);
    g_CharGenMenuTrait_selected = Tile::AddUserTrait("_selected", -1);
}

CharGenMenu::~CharGenMenu() {
    CharGenMenu::pMe = nullptr;
}

void CharGenMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
    if (static_cast<uint32_t>(aiID) <= 6) {
        pTiles[aiID] = apTile;
    }
}

void CharGenMenu::DoClick(int32_t aiID, Tile* apTarget) {
    switch (aiID) {
        case 5: {
            int32_t iOrigPointCount = iCurrPoints;
            iCurrPoints = 0;
            if (eItemType == CharGenMenu::IT_ATTRIBUTE) {
                lbItemList.Traverse(&CharGenMenu::ResetAttributesFunc, 0, std::numeric_limits<int32_t>::max());
            }
            if (eItemType == CharGenMenu::IT_TAG_SKILL) {
                lbItemList.Traverse(&CharGenMenu::ResetSkillsFunc, 0, std::numeric_limits<int32_t>::max());
            }
            pMenu->Set(g_CharGenMenuTrait_CurrPoints, iCurrPoints);
            SetDistributePointsString();
            if (iCurrPoints != iOrigPointCount) {
                Interface::PlayMenuSound(1);
            }
            break;
        }
        case 6: {
            CharGenMenu::Close();
            break;
        }
        case 9: {
            ActorValue::Index currValue = lbItemList.GetDataFor(apTarget);
            bool bIsSelected = lbItemList.IsSelected(apTarget);
            if ((!bIsSelected && iCurrPoints < iMaxPoints) ||
                (bIsSelected && (!bOnlyAddValues || !pPlayer->GetClass()->IsTagSkill(currValue)))) {
                ToggleSelection(&lbItemList, nullptr, apTarget);
                if (lbItemList.IsSelected(apTarget)) {
                    float fTagSkillBonus = fAVDTagSkillBonus.Float();
                    apTarget->Set(CharGenMenu::iUT_AddedValue, fTagSkillBonus, true);
                }
                else {
                    apTarget->Set(CharGenMenu::iUT_AddedValue, 0.0f, true);
                }
                Interface::PlayMenuSound(3);
                iCurrPoints += bIsSelected ? -1 : 1;
                pMenu->Set(g_CharGenMenuTrait_CurrPoints, iCurrPoints);
                SetDisplayString(apTarget);
                SetDistributePointsString();
            }
            break;
        }
        case 10:
        case 11: {
            lbItemList.GetDataFor(apTarget->pParent);
            int32_t iDelta = 2 * (aiID != 10) - 1;
            iCurrPoints += iDelta;
            pMenu->Set(g_CharGenMenuTrait_CurrPoints, iCurrPoints);
            float fAddedValue = apTarget->pParent->GetFloat(CharGenMenu::iUT_AddedValue) + static_cast<double>(iDelta);
            apTarget->pParent->Set(CharGenMenu::iUT_AddedValue, fAddedValue, true);
            SetDisplayString(apTarget->pParent);
            SetDistributePointsString();
            break;
        }
        default: {
            break;
        }
    }
}

void CharGenMenu::DoEnter(int32_t aiID, Tile* apTarget) {
    switch (aiID) {
        case 8:
        case 9: {
            if (fPrevHighlightIndex != apTarget->GetFloat(4012)) {
                const char* pcIcon = ActorValue::GetActorValueIcon(lbItemList.GetDataFor(apTarget));
                pTiles[2]->Set(4044, pcIcon, true);
                const char* pcDesc = ActorValue::GetActorValueDesc(lbItemList.GetDataFor(apTarget));
                pTiles[3]->Set(4036, pcDesc, true);
                pTiles[2]->Set(4003, 1);
                pTiles[3]->Set(4003, 1);
                Interface::PlayMenuSound(4);
                fPrevHighlightIndex = apTarget->GetFloat(4012);
            }
            break;
        }
        case 10:
        case 11: {
            // Highlight is called virtually, through the ListBoxBase pointer.
            static_cast<ListBoxBase*>(&lbItemList)->Highlight(apTarget->pParent);
            DoEnter(8, apTarget->pParent);
            break;
        }
        default: {
            break;
        }
    }
}

int32_t CharGenMenu::GetClass() {
    return 1048;
}

bool CharGenMenu::DoGamepad(int32_t aiButton, float afValue) {
    bool bHandled = false;
    switch (aiButton) {
        case 3:
        case 4: {
            if (eItemType == CharGenMenu::IT_ATTRIBUTE && lbItemList.GetHighlightedItem()) {
                Tile* pHighlighted = static_cast<Tile*>(lbItemList.GetHighlightedItem());
                Tile* pArrowTile = pHighlighted->GetChildByID((aiButton != 4) + 10);
                if (pArrowTile && pArrowTile->IsTrue(4003)) {
                    pArrowTile->PlayTileSound(4043);
                    DoClick((aiButton != 4) + 10, pArrowTile);
                }

                bHandled = true;
            }

            break;
        }
        case 9: {
            if (eItemType == CharGenMenu::IT_TAG_SKILL && lbItemList.GetHighlightedItem()) {
                Tile* pHighlighted = static_cast<Tile*>(lbItemList.GetHighlightedItem());
                DoClick(9, pHighlighted);
                bHandled = true;
            }
            else if (eItemType == CharGenMenu::IT_ATTRIBUTE) {
                bHandled = DoGamepad(3, 1.0f);
            }
            break;
        }
        case 10: {
            if (eItemType == CharGenMenu::IT_ATTRIBUTE) {
                bHandled = DoGamepad(4, 1.0f);
            }
            break;
        }
        default: {
            break;
        }
    }
    return bHandled;
}

bool CharGenMenu::DoGamepadUpEvent(int32_t aiButton, float afValue) {
    if (aiButton == 11 && pTiles[6]->IsTrue(4015)) {
        Interface::PlayMenuSound(1);
        DoClick(6, nullptr);
        return true;
    }
    return false;
}

char CharGenMenu::HasTiles() {
    for (Tile* pTile : pTiles) {
        if (!pTile) {
            return 0;
        }
    }
    return 1;
}

uint32_t CharGenMenu::SetupSpecialPoints() {
    uint32_t uiCount;
    iCurrPoints = 0;
    if (!bOnlyAddValues) {
        iMaxPoints -= 7;
    }
    lbItemList.Initialize(pTiles[1], "CGM_AttributeTemplate");
    pTiles[1]->Set(4002, 105.0f, true);
    int32_t iClass = GetClass();
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_CHARGEN_ATTRIBUTES, iClass, 500u);
    for (uint32_t i = 0;; ++i) {
        uiCount = ActorValue::GetSectionCount(ActorValue::ATTRIBUTE);
        if (i >= uiCount) {
            break;
        }
        ActorValue::Index eIndex = ActorValue::ToActorValue(ActorValue::ATTRIBUTE, i);
        const char* pcName = ActorValue::GetActorValueName(eIndex);
        Tile* pTile = lbItemList.AddItem(eIndex, pcName, nullptr, nullptr);
        int32_t iValue = pPlayer->GetPermanentActorValue(eIndex);
        pTile->Set(4010, 8);
        pTile->Set(g_CharGenMenuTrait_BaseValue, iValue);
        pTile->Set(CharGenMenu::iUT_AddedValue, 0.0f, true);
        SetDisplayString(pTile);
        if (!bOnlyAddValues) {
            iCurrPoints = iValue + iCurrPoints - 1;
        }
    }
    return uiCount;
}

void CharGenMenu::SetupSkillPoints() {
    lbItemList.Initialize(pTiles[1], "CGM_SelectItemTemplate");
    pTiles[1]->Set(4002, 65.0f, true);
    int32_t iClass = GetClass();
    InterfaceManager::Instance()->xTutorialManager.ShowMessage(Interface::TUT_CHARGEN_SKILLS, iClass, 500u);
    for (uint32_t i = 0; i < ActorValue::GetSectionCount(ActorValue::SKILL); ++i) {
        ActorValue::Index eIndex = ActorValue::ToActorValue(ActorValue::SKILL, i);
        if (!ActorValue::IsFlagSet(eIndex, ActorValue::DEPRECATED)) {
            const char* pcName = ActorValue::GetActorValueName(eIndex);
            Tile* pTile = lbItemList.AddItem(eIndex, pcName, nullptr, nullptr);
            float fValue = static_cast<float>(pPlayer->GetPermanentActorValue(eIndex));
            pTile->SetName(ActorValue::GetActorValueName(eIndex));
            pTile->Set(4010, 9);
            pTile->Set(g_CharGenMenuTrait_BaseValue, fValue, true);
            if (pPlayer->GetClass()->IsTagSkill(eIndex)) {
                float fBaseValue = fValue - fAVDTagSkillBonus.Float();
                pTile->Set(g_CharGenMenuTrait_BaseValue, fBaseValue, true);
                if (bShowTaggedSkills) {
                    pTile->Set(g_CharGenMenuTrait_selected, 1);
                    pTile->Set(CharGenMenu::iUT_AddedValue, fAVDTagSkillBonus.Float(), true);
                    if (!bOnlyAddValues) {
                        ++iCurrPoints;
                    }
                }
            }
            SetDisplayString(pTile);
        }
    }
    // Sort is called through lbItemList's vtable: a pointer keeps clang from devirtualising it.
    ListBox<ActorValue::Index>* pItemList = &lbItemList;
    pItemList->Sort(&ActorValue::CompareForSorting);
}

void CharGenMenu::SetDistributePointsString() {
    const char* pcCountFormat[2];
    const char* pcTitleFormat[2];
    char cText[132];
    pcTitleFormat[0] = sAttributesTitle.String();
    pcTitleFormat[1] = sSkillsTitle.String();
    pcCountFormat[0] = sAttributesCount.String();
    pcCountFormat[1] = sSkillsCount.String();
    int32_t iPointsLeft = iMaxPoints - iCurrPoints;
    BSsprintf(cText, 128u, pcTitleFormat[eItemType], iCurrPoints, iMaxPoints);
    pTiles[0]->Set(4036, cText, true);
    if (iPointsLeft) {
        BSsprintf(cText, 128u, pcCountFormat[eItemType], iPointsLeft);
        if (iPointsLeft != 1) {
            BSstrcat(cText, 128, "s");
        }
        pTiles[4]->Set(4036, cText, true);
        pTiles[4]->Set(4003, 1);
    }
    else {
        pTiles[4]->Set(4003, 0);
    }
}

void CharGenMenu::SetDisplayString(Tile* apTile) {
    char cText[16];
    double dBase = apTile->GetFloat(g_CharGenMenuTrait_BaseValue);
    double dTotal = apTile->GetFloat(g_CharGenMenuTrait_ExtraValue) + dBase;
    int32_t iValue = static_cast<int32_t>(apTile->GetFloat(CharGenMenu::iUT_AddedValue) + dTotal);
    if (eItemType) {
        if (eItemType == 1) {
            int32_t iClamped = BSMAX(iValue, 0);
            iValue = BSMIN(iClamped, 100);
        }
    }
    else {
        int32_t iClamped = BSMAX(iValue, 1);
        iValue = BSMIN(iClamped, 10);
    }
    BSsprintf(cText, 16u, "%d", iValue);
    apTile->Set(Tile::AddUserTrait("_ValueString", -1), cText, true);
}

Tile* CharGenMenu::Create(int32_t aiItemType, int32_t aiMaxPoints, bool abOnlyAddValues, bool abShowTaggedSkills) {
    Tile* pOldMenuTile = Menu::GetMenuByClass(1048);
    InterfaceManager* pin = InterfaceManager::Instance();
    // The item type is tested signed against both bounds.
    if (aiMaxPoints <= 0 || aiItemType < IT_ATTRIBUTE || aiItemType > IT_TAG_SKILL) {
        return nullptr;
    }
    if (pOldMenuTile) {
        delete pOldMenuTile;
    }
    Tile* pTile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\char_gen_menu.xml");
    CharGenMenu::pMe = static_cast<CharGenMenu*>(pTile->GetMenu());
    if (CharGenMenu::pMe && CharGenMenu::pMe->GetClass() == CharGenMenu::GetClassStatic()) {
        CharGenMenu::pMe->SetMenuTile(static_cast<TileMenu*>(pTile), false);
        if (!CharGenMenu::pMe->HasTiles()) {
            Warning("MENUS: Char Gen Menu Creation Failed... Are your menu and art resources up to date?");
            return nullptr;
        }
        if (pTile->GetFloat(4007) == 6006.0 || pTile->GetFloat(4007) == 102.0) {
            float fDepth = Tile::GetMaximumDepth();
            pTile->Set(4013, fDepth, true);
        }
        CharGenMenu::pMe->iMaxPoints = aiMaxPoints;
        CharGenMenu::pMe->eItemType = static_cast<CharGenMenu::ITEM_TYPE>(aiItemType);
        CharGenMenu::pMe->bShowTaggedSkills = abShowTaggedSkills;
        CharGenMenu::pMe->bOnlyAddValues = abOnlyAddValues;
        int32_t iOnlyAddTrait = Tile::TextToTrait("_OnlyAdd");
        CharGenMenu::pMe->pMenu->Set(iOnlyAddTrait, static_cast<int32_t>(abOnlyAddValues));
        const char* pcReset = sReset.String();
        CharGenMenu::pMe->pTiles[5]->Set(4036, pcReset, true);
        const char* pcDone = sDone.String();
        CharGenMenu::pMe->pTiles[6]->Set(4036, pcDone, true);
        if (aiItemType) {
            CharGenMenu::pMe->SetupSkillPoints();
        }
        else {
            CharGenMenu::pMe->SetupSpecialPoints();
        }
        CharGenMenu::pMe->pMenu->Set(g_CharGenMenuTrait_CurrPoints, CharGenMenu::pMe->iCurrPoints);
        CharGenMenu::pMe->pMenu->Set(g_CharGenMenuTrait_MaxPoints, CharGenMenu::pMe->iMaxPoints);
        CharGenMenu::pMe->SetDistributePointsString();
        ActorValue::Index eFirstIndex = CharGenMenu::pMe->lbItemList.GetDataFor(
            CharGenMenu::pMe->lbItemList.GetFirstItem()
        );
        const char* pcIcon = ActorValue::GetActorValueIcon(eFirstIndex);
        CharGenMenu::pMe->pTiles[2]->Set(4044, pcIcon, true);
        const char* pcDesc = ActorValue::GetActorValueDesc(eFirstIndex);
        CharGenMenu::pMe->pTiles[3]->Set(4036, pcDesc, true);
        CharGenMenu::pMe->pTiles[2]->Set(4003, 1);
        CharGenMenu::pMe->pTiles[3]->Set(4003, 1);
        if (CharGenMenu::pMe) {
            CharGenMenu::pMe->PrepForVisibility(false);
            return pTile;
        }
    }
    else if (CharGenMenu::pMe && CharGenMenu::pMe->pMenu && CharGenMenu::pMe) {
        delete CharGenMenu::pMe;
    }
    return nullptr;
}

int32_t CharGenMenu::GetClassStatic() {
    return Interface::eCharGenMenu;
}

void CharGenMenu::ResetAttributesFunc(Tile* apTile, ActorValue::Index aeIndex) {
    if (CharGenMenu::pMe) {
        if (!CharGenMenu::pMe->bOnlyAddValues) {
            double dBase = apTile->GetFloat(g_CharGenMenuTrait_BaseValue);
            CharGenMenu::pMe->iCurrPoints += static_cast<int32_t>(
                apTile->GetFloat(g_CharGenMenuTrait_ExtraValue) + dBase - 1.0
            );
        }
        apTile->Set(CharGenMenu::iUT_AddedValue, static_cast<float>(0.0), true);
        CharGenMenu::pMe->SetDisplayString(apTile);
    }
}

void CharGenMenu::Close() {
    if (CharGenMenu::pMe) {
        if (CharGenMenu::pMe->eItemType) {
            for (int32_t iIndex = 0;; ++iIndex) {
                pPlayer->GetClass();
                if (iIndex >= TESClass::GetTagSkillCount()) {
                    break;
                }
                pPlayer->GetClass()->SetMajorSkill(iIndex, ActorValue::VALUE_NONE);
            }
            CharGenMenu::pMe->lbItemList.Traverse(&CharGenMenu::TagSkillsFunc, 0, std::numeric_limits<int32_t>::max());
        }
        else {
            CharGenMenu::pMe->lbItemList
                .Traverse(&CharGenMenu::SetAttributesFunc, 0, std::numeric_limits<int32_t>::max());
        }
        CharGenMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
        CharGenMenu::pMe->StartFadeOut();
    }
}

void CharGenMenu::ResetSkillsFunc(Tile* apTile, ActorValue::Index aiSkill) {
    if (apTile->IsTrue(g_CharGenMenuTrait_selected)) {
        if (CharGenMenu::pMe->bOnlyAddValues && pPlayer->GetClass()->IsTagSkill(aiSkill)) {
            if (!CharGenMenu::pMe->bOnlyAddValues) {
                ++CharGenMenu::pMe->iCurrPoints;
            }
        }
        else {
            apTile->Set(g_CharGenMenuTrait_selected, 0);
            apTile->Set(CharGenMenu::iUT_AddedValue, static_cast<float>(0.0), true);
        }
    }
    CharGenMenu::pMe->SetDisplayString(apTile);
}

void CharGenMenu::SetAttributesFunc(Tile* apTile, ActorValue::Index aeIndex) {
    if (CharGenMenu::pMe && CharGenMenu::pMe->bOnlyAddValues && !CharGenMenu::pMe->eItemType) {
        double dBase = static_cast<double>(pPlayer->GetBaseActorValue(aeIndex));
        float fValue = apTile->GetFloat(CharGenMenu::iUT_AddedValue) + dBase;
        pPlayer->SetActorValue(aeIndex, fValue);
    }
    else {
        double dBase = apTile->GetFloat(g_CharGenMenuTrait_BaseValue);
        float fValue = apTile->GetFloat(CharGenMenu::iUT_AddedValue) + dBase;
        pPlayer->SetActorBaseValueF(aeIndex, fValue);
    }
}

void CharGenMenu::TagSkillsFunc(Tile* apTile, ActorValue::Index aeIndex) {
    if (apTile->GetFloat(4012) == 0.0) {
        aiIndex = 0;
    }
    if (apTile->IsTrue(g_CharGenMenuTrait_selected)) {
        int32_t iIndex = aiIndex;
        pPlayer->GetClass()->SetMajorSkill(iIndex, aeIndex);
        ++aiIndex;
    }
}
