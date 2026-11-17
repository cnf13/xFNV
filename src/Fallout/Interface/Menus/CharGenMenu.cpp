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
#include <stdint.h>

char ToggleSelection(ListBox<ActorValue::Index>* apThis, [[maybe_unused]] void* apEdx, Tile* a2) {
	return apThis->SetSelection(a2, ListBox<ActorValue::Index>::LBSM_TOGGLE);
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

void CharGenMenu::AttachTileByID(int32_t a1, Tile* a2) {
	if (static_cast<uint32_t>(a1) <= 6) {
		pTiles[a1] = a2;
	}
}

void CharGenMenu::DoClick(int32_t a1, Tile* a2) {
	switch (a1) {
	case 5: {
		int32_t iorigPointCount = iCurrPoints;
		iCurrPoints = 0;
		if (eItemType == CharGenMenu::IT_ATTRIBUTE) {
			lbItemList.Traverse(&CharGenMenu::ResetAttributesFunc, 0, 0x7FFFFFFF);
		}
		if (eItemType == CharGenMenu::IT_TAG_SKILL) {
			lbItemList.Traverse(&CharGenMenu::ResetSkillsFunc, 0, 0x7FFFFFFF);
		}
		pMenu->Set(g_CharGenMenuTrait_CurrPoints, iCurrPoints);
		SetDistributePointsString();
		if (iCurrPoints != iorigPointCount) {
			Interface::PlayMenuSound(1);
		}
		break;
	}
	case 6: {
		CharGenMenu::Close();
		break;
	}
	case 9: {
		TESClass* Class;
		ActorValue::Index currValue = lbItemList.GetDataFor(a2);
		bool bisSelected = lbItemList.IsSelected(a2);
		if ((!bisSelected && iCurrPoints < iMaxPoints) ||
			(bisSelected && (!bOnlyAddValues || (Class = pPlayer->GetClass(), !Class->IsTagSkill(currValue))))) {
			ToggleSelection(&lbItemList, nullptr, a2);
			if (lbItemList.IsSelected(a2)) {
				float afValue = fAVDTagSkillBonus.Float();
				a2->Set(CharGenMenu::iUT_AddedValue, afValue, 1);
			} else {
				a2->Set(CharGenMenu::iUT_AddedValue, 0.0f, 1);
			}
			Interface::PlayMenuSound(3);
			iCurrPoints += bisSelected ? -1 : 1;
			pMenu->Set(g_CharGenMenuTrait_CurrPoints, iCurrPoints);
			SetDisplayString(a2);
			SetDistributePointsString();
		}
		break;
	}
	case 10:
	case 11: {
		lbItemList.GetDataFor(a2->pParent);
		int32_t v8 = 2 * (a1 != 10) - 1;
		iCurrPoints += v8;
		pMenu->Set(g_CharGenMenuTrait_CurrPoints, iCurrPoints);
		float v4 = a2->pParent->GetFloat(CharGenMenu::iUT_AddedValue) + static_cast<double>(v8);
		a2->pParent->Set(CharGenMenu::iUT_AddedValue, v4, 1);
		SetDisplayString(a2->pParent);
		SetDistributePointsString();
		break;
	}
	default: {
		return;
	}
	}
}

void CharGenMenu::DoEnter(int32_t a1, Tile* a2) {
	switch (a1) {
	case 8:
	case 9: {
		if (fPrevHighlightIndex != a2->GetFloat(4012)) {
			const char* ActorValueIcon = ActorValue::GetActorValueIcon(lbItemList.GetDataFor(a2));
			pTiles[2]->Set(4044, ActorValueIcon, 1);
			const char* ActorValueDesc = ActorValue::GetActorValueDesc(lbItemList.GetDataFor(a2));
			pTiles[3]->Set(4036, ActorValueDesc, 1);
			pTiles[2]->Set(4003, 1);
			pTiles[3]->Set(4003, 1);
			Interface::PlayMenuSound(4);
			fPrevHighlightIndex = a2->GetFloat(4012);
		}
		break;
	}
	case 10:
	case 11: {
		// Highlight is called virtually, through the ListBoxBase pointer.
		static_cast<ListBoxBase*>(&lbItemList)->Highlight(a2->pParent);
		DoEnter(8, a2->pParent);
		break;
	}
	default: {
		return;
	}
	}
}

int32_t CharGenMenu::GetClass() {
	return 1048;
}

bool CharGenMenu::DoGamepad(int32_t a1, float a2) {
	bool bhandled = 0;
	switch (a1) {
	case 3:
	case 4: {
		if (eItemType == CharGenMenu::IT_ATTRIBUTE) {
			if (lbItemList.GetHighlightedItem()) {
				Tile* v3 = static_cast<Tile*>(lbItemList.GetHighlightedItem());
				Tile* parrowTile = v3->GetChildByID((a1 != 4) + 10);
				if (parrowTile) {
					if (parrowTile->IsTrue(4003)) {
						parrowTile->PlayTileSound(4043);
						DoClick((a1 != 4) + 10, parrowTile);
					}
				}
				bhandled = 1;
			}
		}
		break;
	}
	case 9: {
		if (eItemType == CharGenMenu::IT_TAG_SKILL && lbItemList.GetHighlightedItem()) {
			Tile* v4 = static_cast<Tile*>(lbItemList.GetHighlightedItem());
			DoClick(9, v4);
			bhandled = 1;
		} else {
			if (eItemType == CharGenMenu::IT_ATTRIBUTE) {
				bhandled = DoGamepad(3, 1.0f);
			}
		}
		break;
	}
	case 10: {
		if (eItemType == CharGenMenu::IT_ATTRIBUTE) {
			bhandled = DoGamepad(4, 1.0f);
		}
		break;
	}
	default: {
		return bhandled;
	}
	}
	return bhandled;
}

bool CharGenMenu::DoGamepadUpEvent(int32_t a1, float a2) {
	bool v5 = 0;
	if (a1 == 11 && pTiles[6]->IsTrue(4015)) {
		Interface::PlayMenuSound(1);
		DoClick(6, 0);
		return 1;
	}
	return v5;
}

char CharGenMenu::HasTiles() {
	for (int32_t i = 0; i < 7; ++i) {
		if (!pTiles[i]) {
			return 0;
		}
	}
	return 1;
}

uint32_t CharGenMenu::SetupSpecialPoints() {
	uint32_t result;
	iCurrPoints = 0;
	if (!bOnlyAddValues) {
		iMaxPoints -= 7;
	}
	lbItemList.Initialize(pTiles[1], "CGM_AttributeTemplate");
	pTiles[1]->Set(4002, 105.0f, 1);
	int32_t afValue = GetClass();
	InterfaceManager* v1 = InterfaceManager::Instance();
	v1->xTutorialManager.ShowMessage(Interface::TUT_CHARGEN_ATTRIBUTES, afValue, 500u);
	for (uint32_t i = 0;; ++i) {
		result = ActorValue::GetSectionCount(ActorValue::ATTRIBUTE);
		if (i >= result) {
			break;
		}
		ActorValue::Index aiIndex = ActorValue::ToActorValue(ActorValue::ATTRIBUTE, i);
		const char* ActorValueName = ActorValue::GetActorValueName(aiIndex);
		Tile* th_s = lbItemList.AddItem(aiIndex, ActorValueName, 0, 0);
		int32_t a3 = pPlayer->GetPermanentActorValue(aiIndex);
		th_s->Set(4010, 8);
		th_s->Set(g_CharGenMenuTrait_BaseValue, a3);
		th_s->Set(CharGenMenu::iUT_AddedValue, 0.0f, 1);
		SetDisplayString(th_s);
		if (!bOnlyAddValues) {
			iCurrPoints = a3 + iCurrPoints - 1;
		}
	}
	return result;
}

void CharGenMenu::SetupSkillPoints() {
	lbItemList.Initialize(pTiles[1], "CGM_SelectItemTemplate");
	pTiles[1]->Set(4002, 65.0f, 1);
	int32_t afValue = GetClass();
	InterfaceManager* v1 = InterfaceManager::Instance();
	v1->xTutorialManager.ShowMessage(Interface::TUT_CHARGEN_SKILLS, afValue, 500u);
	for (uint32_t i = 0; i < ActorValue::GetSectionCount(ActorValue::SKILL); ++i) {
		ActorValue::Index aiIndex = ActorValue::ToActorValue(ActorValue::SKILL, i);
		if (!ActorValue::IsFlagSet(aiIndex, ActorValue::DEPRECATED)) {
			const char* ActorValueName = ActorValue::GetActorValueName(aiIndex);
			Tile* th_s = lbItemList.AddItem(aiIndex, ActorValueName, 0, 0);
			float v10 = static_cast<float>(pPlayer->GetPermanentActorValue(aiIndex));
			const char* v3 = ActorValue::GetActorValueName(aiIndex);
			th_s->SetName(v3);
			th_s->Set(4010, 9);
			th_s->Set(g_CharGenMenuTrait_BaseValue, v10, 1);
			TESClass* Class = pPlayer->GetClass();
			if (Class->IsTagSkill(aiIndex)) {
				float v7 = v10 - fAVDTagSkillBonus.Float();
				th_s->Set(g_CharGenMenuTrait_BaseValue, v7, 1);
				if (bShowTaggedSkills) {
					th_s->Set(g_CharGenMenuTrait_selected, 1);
					float* v5 = &fAVDTagSkillBonus.Float();
					th_s->Set(CharGenMenu::iUT_AddedValue, *v5, 1);
					if (!bOnlyAddValues) {
						++iCurrPoints;
					}
				}
			}
			SetDisplayString(th_s);
		}
	}
	// Sort is called through lbItemList's vtable: a pointer keeps clang from devirtualising it.
	ListBox<ActorValue::Index>* pItemList = &lbItemList;
	pItemList->Sort(&ActorValue::CompareForSorting);
}

void CharGenMenu::SetDistributePointsString() {
	const char* v2[2];
	const char* apFormat[2];
	char Src[132];
	apFormat[0] = sAttributesTitle.String();
	apFormat[1] = sSkillsTitle.String();
	v2[0] = sAttributesCount.String();
	v2[1] = sSkillsCount.String();
	int32_t v3 = iMaxPoints - iCurrPoints;
	BSsprintf(Src, 128u, apFormat[eItemType], iCurrPoints, iMaxPoints);
	pTiles[0]->Set(4036, Src, 1);
	if (v3) {
		BSsprintf(Src, 128u, v2[eItemType], v3);
		if (v3 != 1) {
			BSstrcat(Src, 128, "s");
		}
		pTiles[4]->Set(4036, Src, 1);
		pTiles[4]->Set(4003, 1);
	} else {
		pTiles[4]->Set(4003, 0);
	}
}

void CharGenMenu::SetDisplayString(Tile* th_s) {
	char Src[16];
	double Float = th_s->GetFloat(g_CharGenMenuTrait_BaseValue);
	double v5 = th_s->GetFloat(g_CharGenMenuTrait_ExtraValue) + Float;
	int32_t aiA = static_cast<int32_t>(th_s->GetFloat(CharGenMenu::iUT_AddedValue) + v5);
	if (eItemType) {
		if (eItemType == 1) {
			int32_t v3 = BSMAX(aiA, 0);
			aiA = BSMIN(v3, 100);
		}
	} else {
		int32_t v2 = BSMAX(aiA, 1);
		aiA = BSMIN(v2, 10);
	}
	BSsprintf(Src, 16u, "%d", aiA);
	int32_t v4 = Tile::AddUserTrait("_ValueString", -1);
	th_s->Set(v4, Src, static_cast<bool>(1));
}

Tile* CharGenMenu::Create(int32_t aiItemType, int32_t aiMaxPoints, bool abOnlyAddValues, bool abShowTaggedSkills) {
	int32_t v5;
	Tile* pold_menu_tile = Menu::GetMenuByClass(1048);
	InterfaceManager* pin = InterfaceManager::Instance();
	// The item type is tested signed against both bounds.
	if (aiMaxPoints <= 0 || aiItemType < IT_ATTRIBUTE || aiItemType > IT_TAG_SKILL) {
		return 0;
	}
	if (pold_menu_tile) {
		delete pold_menu_tile;
	}
	Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\CharGen\\char_gen_menu.xml");
	CharGenMenu::pMe = static_cast<CharGenMenu*>(ptile->GetMenu());
	if (CharGenMenu::pMe && (v5 = CharGenMenu::pMe->GetClass(), v5 == CharGenMenu::GetClassStatic())) {
		CharGenMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
		if (!CharGenMenu::pMe->HasTiles()) {
			Warning("MENUS: Char Gen Menu Creation Failed... Are your menu and art resources up to date?");
			return 0;
		}
		if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
			float afValue = Tile::GetMaximumDepth();
			ptile->Set(4013, afValue, 1);
		}
		CharGenMenu::pMe->iMaxPoints = aiMaxPoints;
		CharGenMenu::pMe->eItemType = static_cast<CharGenMenu::ITEM_TYPE>(aiItemType);
		CharGenMenu::pMe->bShowTaggedSkills = abShowTaggedSkills;
		CharGenMenu::pMe->bOnlyAddValues = abOnlyAddValues;
		int32_t v6 = Tile::TextToTrait("_OnlyAdd");
		CharGenMenu::pMe->pMenu->Set(v6, static_cast<int32_t>(abOnlyAddValues));
		const char* v7 = sReset.String();
		CharGenMenu::pMe->pTiles[5]->Set(4036, v7, 1);
		const char* v8 = sDone.String();
		CharGenMenu::pMe->pTiles[6]->Set(4036, v8, 1);
		if (aiItemType) {
			CharGenMenu::pMe->SetupSkillPoints();
		} else {
			CharGenMenu::pMe->SetupSpecialPoints();
		}
		CharGenMenu::pMe->pMenu->Set(g_CharGenMenuTrait_CurrPoints, CharGenMenu::pMe->iCurrPoints);
		CharGenMenu::pMe->pMenu->Set(g_CharGenMenuTrait_MaxPoints, CharGenMenu::pMe->iMaxPoints);
		CharGenMenu::pMe->SetDistributePointsString();
		ActorValue::Index efirstIndex =
			CharGenMenu::pMe->lbItemList.GetDataFor(CharGenMenu::pMe->lbItemList.GetFirstItem());
		const char* ActorValueIcon = ActorValue::GetActorValueIcon(efirstIndex);
		CharGenMenu::pMe->pTiles[2]->Set(4044, ActorValueIcon, 1);
		const char* ActorValueDesc = ActorValue::GetActorValueDesc(efirstIndex);
		CharGenMenu::pMe->pTiles[3]->Set(4036, ActorValueDesc, 1);
		CharGenMenu::pMe->pTiles[2]->Set(4003, 1);
		CharGenMenu::pMe->pTiles[3]->Set(4003, 1);
		if (CharGenMenu::pMe) {
			CharGenMenu::pMe->PrepForVisibility(0);
			return ptile;
		}
	} else {
		if (CharGenMenu::pMe) {
			if (CharGenMenu::pMe->pMenu) {
				if (CharGenMenu::pMe) {
					delete CharGenMenu::pMe;
				}
			}
		}
	}
	return 0;
}

// Instance is called twice, as the game does.
Tile* CharGenMenu::Create_(int32_t aiItemType, int32_t aiMaxPoints, bool abOnlyAddValues, bool abShowTaggedSkills) {
	if (InterfaceManager::Instance() && InterfaceManager::Instance()->bFirstInit) {
		return CharGenMenu::Create(aiItemType, aiMaxPoints, abOnlyAddValues, abShowTaggedSkills);
	}
	return nullptr;
}

int32_t CharGenMenu::GetClassStatic() {
	return Interface::eCharGenMenu;
}

void CharGenMenu::ResetAttributesFunc(Tile* apTile, ActorValue::Index aeIndex) {
	if (CharGenMenu::pMe) {
		if (!CharGenMenu::pMe->bOnlyAddValues) {
			double Float = apTile->GetFloat(g_CharGenMenuTrait_BaseValue);
			CharGenMenu::pMe->iCurrPoints +=
				static_cast<int32_t>(apTile->GetFloat(g_CharGenMenuTrait_ExtraValue) + Float - 1.0);
		}
		apTile->Set(CharGenMenu::iUT_AddedValue, static_cast<float>(0.0), static_cast<bool>(1));
		CharGenMenu::pMe->SetDisplayString(apTile);
	}
}

void CharGenMenu::Close() {
	if (CharGenMenu::pMe) {
		if (CharGenMenu::pMe->eItemType) {
			for (int32_t aiIndex = 0;; ++aiIndex) {
				pPlayer->GetClass();
				if (aiIndex >= TESClass::GetTagSkillCount()) {
					break;
				}
				TESClass* Class = pPlayer->GetClass();
				Class->SetMajorSkill(aiIndex, ActorValue::VALUE_NONE);
			}
			CharGenMenu::pMe->lbItemList.Traverse(&CharGenMenu::TagSkillsFunc, 0, 0x7FFFFFFF);
		} else {
			CharGenMenu::pMe->lbItemList.Traverse(&CharGenMenu::SetAttributesFunc, 0, 0x7FFFFFFF);
		}
		CharGenMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
		CharGenMenu::pMe->StartFadeOut();
	}
}

void CharGenMenu::ResetSkillsFunc(Tile* a1, ActorValue::Index aiSkill) {
	if (a1->IsTrue(g_CharGenMenuTrait_selected)) {
		TESClass* Class;
		if (CharGenMenu::pMe->bOnlyAddValues && (Class = pPlayer->GetClass(), Class->IsTagSkill(aiSkill))) {
			if (!CharGenMenu::pMe->bOnlyAddValues) {
				++CharGenMenu::pMe->iCurrPoints;
			}
		} else {
			a1->Set(g_CharGenMenuTrait_selected, 0);
			a1->Set(CharGenMenu::iUT_AddedValue, static_cast<float>(0.0), static_cast<bool>(1));
		}
	}
	CharGenMenu::pMe->SetDisplayString(a1);
}

void CharGenMenu::SetAttributesFunc(Tile* th_s, ActorValue::Index a2) {
	if (CharGenMenu::pMe && (CharGenMenu::pMe->bOnlyAddValues && !CharGenMenu::pMe->eItemType)) {
		double v5 = static_cast<double>(pPlayer->GetBaseActorValue(a2));
		float v4 = th_s->GetFloat(CharGenMenu::iUT_AddedValue) + v5;
		pPlayer->SetActorValue(a2, v4);
	} else {
		double Float = th_s->GetFloat(g_CharGenMenuTrait_BaseValue);
		float a3 = th_s->GetFloat(CharGenMenu::iUT_AddedValue) + Float;
		pPlayer->SetActorBaseValueF(a2, a3);
	}
}

void CharGenMenu::TagSkillsFunc(Tile* th_s, ActorValue::Index a2) {
	if (th_s->GetFloat(4012) == 0.0) {
		aiIndex = 0;
	}
	if (th_s->IsTrue(g_CharGenMenuTrait_selected)) {
		int32_t v3 = aiIndex;
		TESClass* Class = pPlayer->GetClass();
		Class->SetMajorSkill(v3, a2);
		++aiIndex;
	}
}
