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
#include "Reconstruction.hpp"

#include <stdint.h>

// IsTileSelected's guarded static and its guard word.
static int32_t iUT_Selected;
static uint32_t iUT_Selected_guard;

// The game settings this file defines, with their IDs and values, in initialization order.
LevelUpMenu::PerkEffectList_t LevelUpMenu::PerkEffectList;
SettingT<INISettingCollection> bHideUnavailablePerks("bHideUnavailablePerks:Interface", false);

bool IsTileSelected(void* apThis, [[maybe_unused]] void* apEdx, Tile* a2) {
	if ((iUT_Selected_guard & 1) == 0) {
		iUT_Selected_guard |= 1u;
		iUT_Selected = Tile::AddUserTrait("_selected", (-1));
	}
	return (bool)((uint8_t)(iUT_Selected && (a2 && a2->IsTrue(iUT_Selected))));
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
	LevelUpMenu::pMe = 0;
}

void LevelUpMenu::AttachTileByID(int32_t a1, Tile* a2) {
	if (static_cast<uint32_t>(a1) <= 9) {
		pTiles[a1] = a2;
	}
}

void LevelUpMenu::DoClick(int32_t a1, Tile* a2) {
	switch (a1) {
	case 6: {
		if (iCurrPoints[this->eCurrentPage] > 0) {
			Interface::PlayMenuSound(1);
		}
		iCurrPoints[this->eCurrentPage] = 0;
		pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[this->eCurrentPage]);
		LevelUpMenu::PageIndex eCurrentPage = this->eCurrentPage;
		if (eCurrentPage) {
			if (eCurrentPage == LevelUpMenu::PI_PERKS_PAGE) {
				lbPerksList.Traverse(&LevelUpMenu::ResetPerksFunc, 0, 0x7FFFFFFF);
			}
		} else {
			lbSkillsList.Traverse(&LevelUpMenu::ResetSkillsFunc, 0, 0x7FFFFFFF);
		}
		UpdatePointCounter();
		break;
	}
	case 7: {
		SetCurrentPage(static_cast<LevelUpMenu::PageIndex>(this->eCurrentPage + 1));
		pTiles[7]->Set(4035, 0.0f, 1);
		break;
	}
	case 8: {
		SetCurrentPage(static_cast<LevelUpMenu::PageIndex>(this->eCurrentPage - 1));
		pTiles[8]->Set(4035, 0.0f, 1);
		break;
	}
	case 12: {
		if (IsTileSelected(&lbPerksList, nullptr, a2)) {
			lbPerksList.ToggleSelection(a2);
			ApplyPerkEffects(a2);
		} else {
			if (a2->GetFloat(g_levelUpMenu_Trait_TextAlpha) == 255.0) {
				lbPerksList.DeselectAll(1);
				lbPerksList.Select(a2);
				ApplyPerkEffects(a2);
			}
		}
		iCurrPoints[this->eCurrentPage] = lbPerksList.NumSelected();
		pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[this->eCurrentPage]);
		UpdatePointCounter();
		break;
	}
	case 13:
	case 14: {
		if (a2->IsTrue(4003)) {
			ActorValue::Index eindex = lbSkillsList.GetDataFor(a2->pParent);
			int32_t v8 = 2 * (a1 != 13) - 1;
			iCurrPoints[this->eCurrentPage] += v8;
			pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[this->eCurrentPage]);
			TESClass* Class = pPlayer->GetClass();
			if (Class->IsTagSkill(eindex)) {
				v8 *= iSkillPointsTagSkillMult.Int();
			}
			float afValue = a2->pParent->GetFloat(LevelUpMenu::iUT_AddedValue) + static_cast<double>(v8);
			a2->pParent->Set(LevelUpMenu::iUT_AddedValue, afValue, 1);
			bool v9 = 0;
			float v5 = pPlayer->GetBaseValueOverride(eindex, v9) + static_cast<double>(v8);
			pPlayer->SetActorValue(eindex, v5);
			SetDisplayString(a2->pParent);
			UpdatePointCounter();
		}
		break;
	}
	default: {
		return;
	}
	}
}

void LevelUpMenu::DoEnter(int32_t a1, Tile* a2) {
	switch (a1) {
	case 11: {
		const char* ActorValueIcon = ActorValue::GetActorValueIcon(lbSkillsList.GetDataFor(a2));
		pTiles[3]->Set(4044, ActorValueIcon, 1);
		const char* ActorValueDesc = ActorValue::GetActorValueDesc(lbSkillsList.GetDataFor(a2));
		pTiles[4]->Set(4036, ActorValueDesc, 1);
		pTiles[3]->Set(4003, 1);
		pTiles[4]->Set(4003, 1);
		if (ActorValue::GetItemWeaponTypeBadge(lbSkillsList.GetDataFor(a2))) {
			pTiles[9]->Set(4003, 1);
			const char* ItemWeaponTypeBadge = ActorValue::GetItemWeaponTypeBadge(lbSkillsList.GetDataFor(a2));
			pTiles[9]->Set(4044, ItemWeaponTypeBadge, 1);
		} else {
			pTiles[9]->Set(4003, 0);
		}
		Interface::PlayMenuSound(4);
		break;
	}
	case 12: {
		char Src[516];
		pTiles[3]->Set(4044, lbPerksList.GetDataFor(a2)->GetTextureName(), 1);
		lbPerksList.GetDataFor(a2)->BuildDescriptionString(Src, 512u);
		pTiles[4]->Set(4036, Src, 1);
		const char* String = pTiles[3]->GetString(4044);
		if (BSstrlen(String) <= 1) {
			pTiles[3]->Set(4003, 0);
		} else {
			pTiles[3]->Set(4003, 1);
		}
		pTiles[4]->Set(4003, 1);
		pTiles[9]->Set(4003, 0);
		Interface::PlayMenuSound(4);
		break;
	}
	case 13:
	case 14: {
		static_cast<ListBoxBase*>(&lbSkillsList)->Highlight(a2->pParent);
		DoEnter(11, a2->pParent);
		break;
	}
	default: {
		return;
	}
	}
}

void LevelUpMenu::DoIdle() {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (xFadeState == 4 && !pVATS.IsInVats() && !StartMenu::PauseMenuIsActive()) {
		BSAudio* v1 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v1->GetSoundHandleByName(&a1, "MUSSuccess", 2305u);
		SoundHandleByName->Play(0);
		PrepForVisibility(0);
	}
}

int32_t LevelUpMenu::GetClass() {
	return 1027;
}

bool LevelUpMenu::DoGamepad(int32_t a1, float a2) {
	bool bhandled = 0;
	switch (a1) {
	case 3: {
		if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE) {
			if (lbSkillsList.GetHighlightedItem()) {
				Tile* v4 = static_cast<Tile*>(lbSkillsList.GetHighlightedItem());
				Tile* parrowTile_2 = v4->GetChildByID(14);
				if (parrowTile_2->IsTrue(4003)) {
					parrowTile_2->PlayTileSound(4043);
					DoClick(14, parrowTile_2);
				}
				bhandled = 1;
			}
		}
		break;
	}
	case 4: {
		if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE) {
			if (lbSkillsList.GetHighlightedItem()) {
				Tile* v3 = static_cast<Tile*>(lbSkillsList.GetHighlightedItem());
				Tile* parrowTile = v3->GetChildByID(13);
				if (parrowTile->IsTrue(4003)) {
					parrowTile->PlayTileSound(4043);
					DoClick(13, parrowTile);
				}
				bhandled = 1;
			}
		}
		break;
	}
	case 9: {
		if (eCurrentPage == LevelUpMenu::PI_PERKS_PAGE && lbPerksList.GetHighlightedItem()) {
			Tile* v5 = static_cast<Tile*>(lbPerksList.GetHighlightedItem());
			v5->PlayTileSound(4043);
			Tile* v6 = static_cast<Tile*>(lbPerksList.GetHighlightedItem());
			DoClick(12, v6);
			bhandled = 1;
		} else {
			if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE) {
				bhandled = DoGamepad(3, 1.0f);
			}
		}
		break;
	}
	case 10: {
		if (eCurrentPage == LevelUpMenu::PI_PERKS_PAGE) {
			pTiles[8]->PlayTileSound(4043);
			DoClick(8, 0);
			bhandled = 1;
		} else {
			if (eCurrentPage == LevelUpMenu::PI_SKILLS_PAGE) {
				bhandled = DoGamepad(4, 1.0f);
			}
		}
		break;
	}
	default: {
		return bhandled;
	}
	}
	return bhandled;
}

bool LevelUpMenu::DoGamepadUpEvent(int32_t a1, float a2) {
	bool v5 = 0;
	if (a1 == 11 && pTiles[7]->IsTrue(4015)) {
		Interface::PlayMenuSound(1);
		DoClick(7, 0);
		return 1;
	}
	return v5;
}

bool LevelUpMenu::IsWellFormedMenu() {
	int32_t i;
	for (i = 0; i < 10; ++i) {
		if (!pTiles[i]) {
			return (bool)0;
		}
	}
	return (bool)1;
}

void LevelUpMenu::SetDisplayString(Tile* apTile) {
	char Src[16];
	double Float = apTile->GetFloat(g_levelUpMenu_Trait_BaseValue);
	double v5 = apTile->GetFloat(g_levelUpMenu_Trait_ExtraValue) + Float;
	double v2 = apTile->GetFloat(LevelUpMenu::iUT_AddedValue) + v5;
	int32_t v3 = BSMAX(static_cast<int32_t>(v2), 0);
	BSsprintf(Src, 16u, "%d", BSMIN(v3, 100));
	int32_t v4 = Tile::AddUserTrait("_DisplayString", -1);
	apTile->Set(v4, Src, static_cast<bool>(1));
}

void LevelUpMenu::ApplyPerkEffects(Tile* apTile) {
	BGSPerkEntry* pprevRankEntry;
	BGSPerk* pcurrPerk = lbPerksList.GetDataFor(apTile);
	uint8_t uccurrPerkRank = static_cast<int32_t>(apTile->GetFloat(g_levelUpMenu_Trait_Rank));
	BGSPerkEntry* pcurrPerkEntry = pcurrPerk->GetPerkEntryByRank(uccurrPerkRank);
	if (uccurrPerkRank <= 1u) {
		pprevRankEntry = 0;
	} else {
		pprevRankEntry = pcurrPerk->GetPerkEntryByRank(uccurrPerkRank - 1);
	}
	if (pcurrPerkEntry) {
		if (pcurrPerkEntry->GetType() == PERK_ENTRY_TYPE_ABILITY) {
			SpellItem* pAbility = static_cast<BGSAbilityPerkEntry*>(pcurrPerkEntry)->pAbility;
			SpellItem* pprevAbility =
				pprevRankEntry ? static_cast<BGSAbilityPerkEntry*>(pprevRankEntry)->pAbility : nullptr;
			LevelUpMenu::PerkEffectList.Clear(1);
			// The upcast to the spell's effect list tests the spell for null, as the game does.
			BSSimpleList<EffectItem*>* peffectObj = pAbility;
			for (EffectItem*& rpItem : peffectObj->ItemsUntilNull()) {
				EffectItemData perkEffect;
				perkEffect.iActorValue = rpItem->GetActorValueIndex();
				perkEffect.iMagnitude = rpItem->GetMagnitude();
				if (perkEffect.iActorValue != ActorValue::VALUE_NONE) {
					BSSimpleList<EffectItem*>* pprevRankEffectObj = pprevAbility;
					for (EffectItem*& rpItem_2 : pprevRankEffectObj->ItemsUntilNull()) {
						ActorValue::Index ActorValueIndex = rpItem_2->GetActorValueIndex();
						if (ActorValueIndex == perkEffect.iActorValue) {
							int32_t Magnitude = rpItem_2->GetMagnitude();
							perkEffect.iMagnitude -= Magnitude;
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
	}
	lbSkillsList.Traverse(&LevelUpMenu::PerkUpdateFunc, 0, 0x7FFFFFFF);
}

void LevelUpMenu::SetCurrentPage(LevelUpMenu::PageIndex aePage) {
	int32_t v2 = Tile::TextToTrait("_EndPage");
	if (!(pMenu->GetFloat(v2) < static_cast<double>(aePage))) {
		this->eCurrentPage = aePage;
		pMenu->Set(g_levelUpMenu_Trait_CurrentPage, static_cast<int32_t>(aePage));
		pMenu->Set(g_levelUpMenu_Trait_CurrPoints, iCurrPoints[aePage]);
		pMenu->Set(g_levelUpMenu_Trait_MaxPoints, iMaxPoints[aePage]);
		UpdatePointCounter();
		LevelUpMenu::PageIndex eCurrentPage = this->eCurrentPage;
		if (eCurrentPage) {
			if (eCurrentPage == LevelUpMenu::PI_PERKS_PAGE) {
				lbPerksList.Traverse(&LevelUpMenu::SetPerkAlphaIfRequirementsNotMet, 0, 0x7FFFFFFF);
				// Sort is called through a pointer so that the call stays virtual; a call on the member itself would be
				// devirtualised.
				ListBox<BGSPerk*>* pPerksList = &lbPerksList;
				pPerksList->Sort(&BGSPerk::CompareForSorting);
				lbSkillsList.SaveListState();
				lbPerksList.RestoreListState(1);
			}
		} else {
			lbSkillsList.Traverse(&LevelUpMenu::UpdateSkillsFunc, 0, 0x7FFFFFFF);
			lbPerksList.SaveListState();
			lbSkillsList.RestoreListState(1);
		}
	} else {
		LevelUpMenu::Close();
	}
}

void LevelUpMenu::InitLists() {
	int32_t itotalSkillPoints = 0;
	int32_t inumSkills = 0;
	lbSkillsList.Initialize(pTiles[1], "LUM_SkillTemplate");
	for (int32_t ivalueCt = 0; ivalueCt < ActorValue::GetSectionCount(ActorValue::SKILL); ++ivalueCt) {
		ActorValue::Index currValue = ActorValue::ToActorValue(ActorValue::SKILL, ivalueCt);
		if (!ActorValue::IsFlagSet(currValue, ActorValue::DEPRECATED)) {
			const char* ActorValueName = ActorValue::GetActorValueName(currValue);
			Tile* pnew = lbSkillsList.AddItem(currValue, ActorValueName, 0, 0);
			const char* v2 = ActorValue::GetActorValueName(currValue);
			pnew->SetName(v2);
			pnew->Set(4010, 11);
			pnew->Set(LevelUpMenu::iUT_AddedValue, 0.0f, 1);
			int32_t v3 = pPlayer->GetPermanentActorValue(currValue);
			pnew->Set(g_levelUpMenu_Trait_BaseValue, v3);
			itotalSkillPoints += pPlayer->GetPermanentActorValue(currValue);
			++inumSkills;
		}
	}
	// Sort is called through a pointer so that the call goes through the list's vtable; a call on the member itself
	// would be devirtualised.
	ListBox<ActorValue::Index>* pSkillsList = &lbSkillsList;
	pSkillsList->Sort(&ActorValue::CompareForSorting);
	int32_t v4 = BSMIN(iMaxPoints[0], 100 * inumSkills - itotalSkillPoints);
	iMaxPoints[0] = BSMAX(v4, 0);
	lbPerksList.Initialize(pTiles[2], "LUM_PerkTemplate");
	for (BSSimpleList<BGSPerk*>* pcurrentPerk = &AvailablePerks; pcurrentPerk && pcurrentPerk->m_item;
		pcurrentPerk = pcurrentPerk->m_pkNext) {
		char Dst[68];
		BGSPerk* afValue = pcurrentPerk->m_item;
		int32_t icurrRank = pPlayer->GetPerkRank(afValue, 0);
		BSstrcpy(Dst, 64, pcurrentPerk->m_item->GetFullName());
		if (!BSstrlen(Dst)) {
			const char* v5 = sMissingName.String();
			BSstrcpy(Dst, 64, v5);
		}
		if (pcurrentPerk->m_item->GetNumRanks() > 1u && icurrRank > 0) {
			BSsprintf(Dst, 64, "%s (%d)", Dst, icurrRank + 1);
		}
		Tile* pnew_2 = lbPerksList.AddItem(pcurrentPerk->m_item, Dst, 0, 0);
		pnew_2->Set(4010, 12);
		pnew_2->Set(g_levelUpMenu_Trait_Rank, icurrRank + 1);
	}
}

void LevelUpMenu::UpdatePointCounter() {
	const char* apFormat[2];
	apFormat[0] = sLevelUpSkillCounterPl.String();
	apFormat[1] = sLevelUpPerkCounterPl.String();
	int32_t v4 = iMaxPoints[eCurrentPage] - iCurrPoints[eCurrentPage];
	if (v4) {
		if (v4 == 1) {
			const char* Src;
			if (eCurrentPage) {
				Src = sLevelUpPerkCounter.String();
			} else {
				Src = sLevelUpSkillCounter.String();
			}
			pTiles[5]->Set(4036, Src, static_cast<bool>(1));
			pTiles[5]->Set(4003, 1);
		} else {
			char apDest[68];
			BSsprintf(apDest, 64u, apFormat[eCurrentPage], v4);
			pTiles[5]->Set(4036, apDest, static_cast<bool>(1));
			pTiles[5]->Set(4003, 1);
		}
	} else {
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
	int32_t v0;
	float fSkillPoints;
	Tile* MenuByClass = Menu::GetMenuByClass(1027);
	InterfaceManager* v22 = InterfaceManager::Instance();
	if (MenuByClass) {
		delete MenuByClass;
	}
	Tile* th_s = v22->pMenusRoot->ReadFile("Data\\Menus\\levelup_menu.xml");
	LevelUpMenu::pMe = static_cast<LevelUpMenu*>(th_s->GetMenu());
	if (LevelUpMenu::pMe && (v0 = LevelUpMenu::pMe->GetClass(), v0 == LevelUpMenu::GetClassStatic())) {
		LevelUpMenu::pMe->SetMenuTile(static_cast<TileMenu*>(th_s), 0);
		if (LevelUpMenu::pMe->IsWellFormedMenu()) {
			int32_t LevelUpSkillPoints_0;
			char Src[64];
			if (th_s->GetFloat(4007) == 6006.0 || th_s->GetFloat(4007) == 102.0) {
				float MaximumDepth = Tile::GetMaximumDepth();
				th_s->Set(4013, MaximumDepth, 1);
			}
			int32_t ClassStatic = LevelUpMenu::GetClassStatic();
			InterfaceManager* v2 = InterfaceManager::Instance();
			v2->xTutorialManager.ShowMessage(Interface::TUT_LEVELING, ClassStatic, 512u);
			int32_t afValue = pPlayer->GetLevel();
			BSsprintf(Src, 64, sLevelUpTitleText.String(), afValue);
			LevelUpMenu::pMe->pTiles[0]->Set(4036, Src, 1);
			const char* v3 = sReset.String();
			LevelUpMenu::pMe->pTiles[6]->Set(4036, v3, 1);
			const char* v15 = sContinue.String();
			int32_t v4 = Tile::TextToTrait("_Title_0");
			LevelUpMenu::pMe->pTiles[7]->Set(v4, v15, 1);
			const char* v16 = sDone.String();
			int32_t v5 = Tile::TextToTrait("_Title_1");
			LevelUpMenu::pMe->pTiles[7]->Set(v5, v16, 1);
			int32_t v6 = Tile::TextToTrait("_CurrentPage");
			th_s->Poke(v6, 1.0f);
			const char* v7 = sBack.String();
			LevelUpMenu::pMe->pTiles[8]->Set(4036, v7, 1);
			if (pPlayer) {
				LevelUpSkillPoints_0 = GamePlayFormulas::GetLevelUpSkillPoints_0(&static_cast<
					ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer))));
			} else {
				LevelUpSkillPoints_0 = GamePlayFormulas::GetLevelUpSkillPoints_0(0);
			}
			fSkillPoints = static_cast<float>(LevelUpSkillPoints_0);
			BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_GAINED_SKILL_POINTS, pPlayer, &fSkillPoints);
			LevelUpMenu::pMe->iCurrPoints[0] = 0;
			LevelUpMenu::pMe->iMaxPoints[0] = static_cast<int32_t>(fSkillPoints);
			uint16_t Level = pPlayer->GetLevel();
			if (!(Level % iLevelsPerPerk.Int())) {
				for (BSSimpleList<BGSPerk*>* pIter = pDataHandler->GetPerkList(); pIter && pIter->m_item;
					pIter = pIter->GetNext()) {
					BGSPerk* pPerk = pIter->m_item;
					if (pPerk->GetLevel() > 0) {
						int32_t v10 = pPlayer->GetPerkRank(pPerk, 0);
						if (v10 < pPerk->GetNumRanks() && !pPerk->Data.bTrait && pPerk->IsPerkAttainable(pPlayer) &&
							pPerk->GetIsPlayable()) {
							char v18 = 1;
							if (bHideUnavailablePerks.Bool()) {
								int32_t v11;
								if (!pPerk->IsPerkAvailable(pPlayer) ||
									(v11 = pPerk->GetLevel(), v11 > pPlayer->GetLevel())) {
									v18 = 0;
								}
							}
							if (v18) {
								LevelUpMenu::pMe->AvailablePerks.AddTail(pPerk);
							}
						}
					}
				}
			}
			LevelUpMenu::pMe->iCurrPoints[1] = 0;
			LevelUpMenu::pMe->iMaxPoints[1] = 1;
			if (!LevelUpMenu::pMe->AvailablePerks.ItemsInList()) {
				int32_t v12 = Tile::TextToTrait("_EndPage");
				LevelUpMenu::pMe->pMenu->Set(v12, 0);
			}
			LevelUpMenu::pMe->InitLists();
			LevelUpMenu::pMe->SetCurrentPage(LevelUpMenu::PI_SKILLS_PAGE);
			return th_s;
		} else {
			Warning("MENUS: Level Up Menu Creation Failed... Are your menu and art resources up to date?");
			return 0;
		}
	} else {
		if (LevelUpMenu::pMe) {
			if (LevelUpMenu::pMe->pMenu) {
				if (LevelUpMenu::pMe) {
					delete LevelUpMenu::pMe;
				}
			}
		}
		return 0;
	}
}

void LevelUpMenu::SetPerkAlphaIfRequirementsNotMet(Tile* a1, BGSPerk* a2) {
	int32_t level;
	float v3;
	if (a2->IsPerkAvailable(pPlayer) &&
		((level = (int32_t)a2->GetLevel()), (level <= ((int32_t)pPlayer->Actor::GetLevel())))) {
		v3 = 255.0f;
	} else {
		v3 = 0x1p+7f;
	}
	a1->Set(g_levelUpMenu_Trait_TextAlpha, v3, (bool)1);
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
		CharacterProgression* CharacterProgressionInfo = pPlayer->GetCharacterProgressionInfo();
		CharacterProgressionInfo->FinishLevelUp();
		LevelUpMenu::pMe->lbPerksList.Traverse(&LevelUpMenu::IncreaseSelectedPerkRank, 0, 0x7FFFFFFF);
		CharacterProgression* v1 = pPlayer->GetCharacterProgressionInfo();
		if (v1->GetReadyToLevelUp()) {
			pPlayer->UpdateMagic(1, 0);
		}
		StatsMenu::UpdatePlayerName();
		StatsMenu::UpdatePlayerTitle();
		LevelUpMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
		LevelUpMenu::pMe->StartFadeOut();
		LevelUpMenu::pMe = 0;
	}
}

void LevelUpMenu::ResetSkillsFunc(Tile* apTile, ActorValue::Index a2) {
	bool v4 = 0;
	double v3 = pPlayer->GetBaseValueOverride(a2, v4);
	float v2 = v3 - apTile->GetFloat(LevelUpMenu::iUT_AddedValue);
	pPlayer->SetActorValue(a2, v2);
	apTile->Set(LevelUpMenu::iUT_AddedValue, static_cast<float>(0.0), static_cast<bool>(1));
	LevelUpMenu::pMe->SetDisplayString(apTile);
}

void LevelUpMenu::PerkUpdateFunc(Tile* apTile, ActorValue::Index aeIndex) {
	for (int32_t ieffectCt = 0; ieffectCt < LevelUpMenu::PerkEffectList.iSize; ++ieffectCt) {
		if ((&LevelUpMenu::PerkEffectList.GetAt(ieffectCt))->iActorValue == aeIndex) {
			double Float = apTile->GetFloat(g_levelUpMenu_Trait_BaseValue);
			double v17 = apTile->GetFloat(g_levelUpMenu_Trait_ExtraValue) + Float;
			int32_t itotalValue = static_cast<int32_t>(apTile->GetFloat(LevelUpMenu::iUT_AddedValue) + v17);
			int32_t iUT_Overflow = Tile::TextToTrait("_OverflowValue");
			if (apTile->GetFloat(iUT_Overflow) != 0.0) {
				EffectItemData* v16 = &LevelUpMenu::PerkEffectList.GetAt(ieffectCt);
				v16->iMagnitude += static_cast<int32_t>(apTile->GetFloat(iUT_Overflow));
				apTile->Set(iUT_Overflow, 0.0f, 1);
			}
			double v15 = static_cast<double>((&LevelUpMenu::PerkEffectList.GetAt(ieffectCt))->iMagnitude + itotalValue);
			int32_t v2 = Tile::TextToTrait("_MaxValue");
			if (!(apTile->GetFloat(v2) < v15)) {
				double v12 =
					static_cast<double>((&LevelUpMenu::PerkEffectList.GetAt(ieffectCt))->iMagnitude + itotalValue);
				int32_t v5 = Tile::TextToTrait("_MinValue");
				if (apTile->GetFloat(v5) > v12) {
					double v11 =
						static_cast<double>((&LevelUpMenu::PerkEffectList.GetAt(ieffectCt))->iMagnitude + itotalValue);
					int32_t v6 = Tile::TextToTrait("_MinValue");
					int32_t idifference_2 = static_cast<int32_t>(v11 - apTile->GetFloat(v6));
					int32_t iUT_Overflow_3 = Tile::TextToTrait("_OverflowValue");
					apTile->Set(iUT_Overflow_3, idifference_2);
					EffectItemData* v10 = &LevelUpMenu::PerkEffectList.GetAt(ieffectCt);
					v10->iMagnitude -= idifference_2;
				}
			} else {
				double v14 =
					static_cast<double>((&LevelUpMenu::PerkEffectList.GetAt(ieffectCt))->iMagnitude + itotalValue);
				int32_t v3 = Tile::TextToTrait("_MaxValue");
				int32_t idifference = static_cast<int32_t>(v14 - apTile->GetFloat(v3));
				int32_t iUT_Overflow_2 = Tile::TextToTrait("_OverflowValue");
				apTile->Set(iUT_Overflow_2, idifference);
				EffectItemData* v13 = &LevelUpMenu::PerkEffectList.GetAt(ieffectCt);
				v13->iMagnitude -= idifference;
			}
			double v8 = apTile->GetFloat(g_levelUpMenu_Trait_ExtraValue);
			float afValue = static_cast<double>((&LevelUpMenu::PerkEffectList.GetAt(ieffectCt))->iMagnitude) + v8;
			apTile->Set(g_levelUpMenu_Trait_ExtraValue, afValue, 1);
		}
	}
}

void LevelUpMenu::IncreaseSelectedPerkRank(Tile* a1, BGSPerk* a2) {
	if (a1->IsTrue(LevelUpMenu::iUT_Selected)) {
		uint8_t v2 = pPlayer->GetPerkRank(a2, 0);
		pPlayer->AddPerk(a2, v2 + 1, false);
	}
}
