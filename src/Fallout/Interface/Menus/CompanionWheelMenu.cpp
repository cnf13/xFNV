#include "Fallout/Interface/Menus/CompanionWheelMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkPickData.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSMenu/RadialTile.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSDefaultObjectManager.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/FOScriptFunctions.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/BGSZoneTargetListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageSchedule.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestStage.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESScriptFunctions.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout Shared/TESTopicInfo.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/FollowerBarks.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/LowProcess.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Dialogue/DialogueItem.h"
#include "Fallout/Interface/FOPipboyManager.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LevelUpMenu.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <stdint.h>

int32_t return_1075() {
	return 1075;
}

CompanionWheelMenu::CompanionWheelMenu()
	: Menu(), m_CurrentlySelectedTile(TI_NO_TILE), bTalkToOnClose(false), m_hCurrentReactivityBark(0u) {}

CompanionWheelMenu::~CompanionWheelMenu() {
	if (bTalkToOnClose) {
		InterfaceManager::Instance()->InternalSetMenuMode(InterfaceManager::eMenuModeOff);
		TransitionFromCloseToTalkTo();
	}
	CompanionWheelMenu::pMe = nullptr;
}

void CompanionWheelMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
	if (aiID >= 0 && aiID < 16) {
		pTiles[aiID] = apTile;
	}
}

void CompanionWheelMenu::DoClick(int32_t aiID, Tile* apTarget) {
	if (!apTarget && aiID >= 0 && aiID < 16) {
		apTarget = pTiles[aiID];
	}
	if (Interface::IsTopMenuID(Interface::eCompanionWheelMenu)) {
		switch (aiID) {
		case 0: {
			Do_ToggleAggressivePassive();
			HandleMouseoverForTile(aiID, 1);
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 1: {
			Do_UseStim();
			HandleMouseoverForTile(aiID, 1);
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 2: {
			Do_ToggleStayFollow();
			HandleMouseoverForTile(aiID, 1);
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 3: {
			Do_Talk();
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 4: {
			Do_Backup();
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 5: {
			Do_ToggleNearFar();
			HandleMouseoverForTile(aiID, 1);
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 6: {
			OpenInventory();
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 7: {
			Do_ToggleRangedMelee();
			HandleMouseoverForTile(aiID, 1);
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			break;
		}
		case 11: {
			InterfaceManager::PlayMenuSound(MENU_SOUND_OK);
			CompanionWheelMenu::Close(1);
			break;
		}
		default: {
			return;
		}
		}
	}
}

void CompanionWheelMenu::DoEnter(int32_t aiID, Tile* apTarget) {
	// A null apTarget is filled from the tile table, though nothing reads it afterwards, as the game does.
	if (!apTarget && aiID >= 0 && aiID < 16) {
		apTarget = pTiles[aiID];
	}
	if (Interface::IsTopMenuID(Interface::eCompanionWheelMenu) && aiID >= 0 && (aiID <= 7 || aiID == 11)) {
		HandleMouseoverForTile(aiID, 0);
	}
}

void CompanionWheelMenu::DoIdle() {
	char v8 = 0;
	float sThumbLX = 0.0f;
	float sThumbLY = 0.0f;
	if (m_nTimeToClearSubtitle) {
		if (BSGetTickCount() > m_nTimeToClearSubtitle) {
			m_nTimeToClearSubtitle = 0;
			if (pTiles[12]) {
				pTiles[12]->Set(4036, "", 1);
			}
		}
	}
	if (Interface::IsTopMenuID(Interface::eCompanionWheelMenu)) {
		InterfaceManager::Instance();
		if (InterfaceManager::Has360Controller()) {
			InterfaceManager* v1 = InterfaceManager::Instance();
			if (!v1->GetMouseVisible()) {
				InterfaceManager::Instance();
				if (InterfaceManager::Has360Controller()) {
					InterfaceManager* v2 = InterfaceManager::Instance();
					if (!v2->GetMouseVisible() &&
						(std::abs(lastControllerState.Gamepad.sThumbLY) > 0x1EA9 ||
							std::abs(pState.Gamepad.sThumbLX) > 0x1EA9)) {
						v8 = 1;
						sThumbLX = static_cast<float>(lastControllerState.Gamepad.sThumbLX);
						sThumbLY = static_cast<float>(lastControllerState.Gamepad.sThumbLY);
					}
				}
			}
		}
		if (v8) {
			float fInputAngle;
			float v6 = -sThumbLY;
			if (v6 == 0.0) {
				if (!(sThumbLX > 0.0)) {
					fInputAngle = 4.71238f;
				} else {
					fInputAngle = 1.57075f;
				}
			} else {
				float afX = -sThumbLX / v6;
				fInputAngle = NiATan(afX);
			}
			if (v6 > 0.0) {
				fInputAngle = fInputAngle + 3.141590118408203;
			}
			if (sThumbLX < 0.0 && v6 < 0.0) {
				fInputAngle = fInputAngle + 6.283180236816406;
			}
			SetButtonAsHighlighted(fInputAngle);
		}
	}
}

bool CompanionWheelMenu::DoGamepad(int32_t aiButton, float afValue) {
	int8_t v5 = 0;
	if (!Interface::IsTopMenuID(static_cast<Interface::enumMenus>(1075))) {
		return (bool)0;
	}
	if (aiButton == 9) {
		if (m_CurrentlySelectedTile == -1) {
			DoClick(TI_EXIT_BUTTON, pTiles[TI_EXIT_BUTTON]);
		} else {
			DoClick(m_CurrentlySelectedTile, pTiles[m_CurrentlySelectedTile]);
		}
		return (bool)1;
	} else {
		if (aiButton == 10) {
			DoClick(11, pTiles[11]);
			return (bool)1;
		}
	}
	return (bool)v5;
}

bool CompanionWheelMenu::IsWellFormedMenu() {
	int32_t i;
	for (i = 0; i < 16; ++i) {
		if (!pTiles[i]) {
			return (bool)0;
		}
	}
	return (bool)1;
}

void CompanionWheelMenu::Do_Backup() {
	if (m_pRelatedCompanion) {
		m_pRelatedCompanion->InitiateBackUpPackage();
	}
	Close(true);
}

void CompanionWheelMenu::Do_Talk() {
	bTalkToOnClose = true;
	Close(true);
}

void CompanionWheelMenu::RunTopicScript(TESTopic* a2) {
	ExtraDataList* extraDataList;
	ExtraDataList* v3;
	ScriptLocals* scriptLocals;
	ScriptLocals* v5;
	Script* resultScript;
	Script* v8;
	TESTopicInfo* matchingInfo_1 = nullptr;
	if (a2) {
		matchingInfo_1 = a2->GetMatchingInfo(m_pRelatedCompanion, pPlayer);
	}
	if (matchingInfo_1) {
		resultScript = matchingInfo_1->GetResultScript(TIRS_BEGIN);
		if (resultScript) {
			if (resultScript->GetHeader()->dataSize) {
				extraDataList = m_pRelatedCompanion->TESObjectREFR::GetExtra();
				scriptLocals = extraDataList->GetScriptLocals();
				resultScript->Run(m_pRelatedCompanion, scriptLocals, nullptr, 1);
			}
		}
		v8 = matchingInfo_1->GetResultScript(TIRS_END);
		if (v8) {
			if (v8->GetHeader()->dataSize) {
				v3 = m_pRelatedCompanion->TESObjectREFR::GetExtra();
				v5 = v3->GetScriptLocals();
				v8->Run(m_pRelatedCompanion, v5, nullptr, 1);
			}
		}
	}
}

void CompanionWheelMenu::SetButtonPreviewText(const char* asText) {
	if (pTiles[9]) {
		pTiles[9]->Set(4036, asText, (bool)1);
	}
}

void CompanionWheelMenu::SetButtonContextText(int32_t aiID) {
	char acText[260];
	acText[0] = '\0';
	switch (aiID) {
	case 1:
		if (m_pRelatedCompanion) {
			int32_t iHealth = static_cast<int32_t>(static_cast<ActorValueOwner*>(m_pRelatedCompanion)
					->GetActorFloatValue(ActorValue::HEALTH));
			int32_t iBaseHealth =
				static_cast<ActorValueOwner*>(m_pRelatedCompanion)->GetBaseActorValue(ActorValue::HEALTH);
			int32_t iStimpaks = 0;
			TESBoundObject* pStimpak =
				dynamic_cast<TESBoundObject*>(BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK));
			if (pStimpak) {
				ItemChange* pItem = pPlayer->GetInventoryItem(pStimpak, 0);
				if (pItem) {
					iStimpaks = pItem->GetCount();
					delete pItem;
				}
			}
			const char* pStimpakText = sStimpak.String();
			const char* pHealthText = sHitPointsShort.String();
			BSsprintf(acText, 0x104, "%s  %d/%d\n%s  %d", pHealthText, iHealth, iBaseHealth, pStimpakText, iStimpaks);
		}
		break;
	case 6:
		if (m_pRelatedCompanion) {
			Actor* pCompanion = m_pRelatedCompanion;
			ActorValueOwner* pOwner = pCompanion ? static_cast<ActorValueOwner*>(pCompanion) : nullptr;
			int32_t iWeight = static_cast<int32_t>(Actor::GetInventoryWeight(pOwner, ActorValue::CARRY_WEIGHT));
			int32_t iMaxWeight = static_cast<int32_t>(m_pRelatedCompanion->GetMaxCarryWeight());
			BSsprintf(acText, 0x104, "%s  %d/%d", sInventoryWeight.String(), iWeight, iMaxWeight);
		}
		break;
	case 7:
		if (m_pRelatedCompanion) {
			ItemChange* pWeapon = m_pRelatedCompanion->GetBestWeapon(COMBAT_WEAPON_TYPE_COUNT);
			if (pWeapon) {
				BSsprintf(acText, 0x104, "%s", pWeapon->GetFullName());
			}
		}
		break;
	}
	if (pTiles[10]) {
		pTiles[10]->Set(Tile::kTileValue_String, acText, true);
	}
}

void CompanionWheelMenu::SpeakTopic(TESTopic* a1) {
	FNVD_INLINED_LOCAL(BSStringT<char>, v10, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, v11, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
	TESTopicInfo* apTopicInfo = 0;
	TESObjectREFR* apTarget = (TESObjectREFR*)pPlayer;
	Actor* apSpeaker = m_pRelatedCompanion;
	if (a1) {
		apTopicInfo = a1->GetMatchingInfo(apSpeaker, apTarget);
	}
	if (apTopicInfo) {
		DialogueItem apMemory(apTopicInfo->pOwnerQuest, a1, apTopicInfo, apSpeaker);
		apMemory.FirstResponse();
		DialogueResponse* pResponse = apMemory.GetCurrentResponse();
		if (pResponse) {
			if (pResponse->pVoiceSound) {
				int32_t refID = pResponse->pVoiceSound->iFormID;
				BSSoundHandle* v16 = apSpeaker->PlaySound(&a2, refID, 0, 258u, 1);
				this->m_hCurrentReactivityBark.operator=(*v16);
			} else {
				PlayerCharacter* v9 = pPlayer;
				TESIdleForm* ListenerIdle = pResponse->pListenIdle;
				TESIdleForm* SpeakerIdle = pResponse->pSpeakerIdle;
				uint32_t Length = pResponse->cText.GetLength();
				uint32_t CastingType = pResponse->uiEmotion;
				DIALOGUE_EMOTION FormFlags = pResponse->eEmotion;
				char* v3 = pResponse->GetVoice()->pString;
				float v21 = apSpeaker->SpeakSoundFunction(v3, this->m_hCurrentReactivityBark, FormFlags, CastingType,
					Length, SpeakerIdle, ListenerIdle, v9, 1, 1, 0, 0, 1);
			}
			BSSoundHandle* v19 = &v11;
			BSSoundHandle* v14 = &v11.operator=(this->m_hCurrentReactivityBark);
			BSStringT<char>* Text = &pResponse->cText;
			BSStringT<char>* v18 = &v10;
			BSStringT<char>* v12 = ::new (&v10) BSStringT<char>(*Text);
			this->DisplayCompanionWheelSubtitle(v10, v11);
			// The callee receives a copy of this raw-storage local, so the local is destroyed here.
			v10.~BSStringT<char>();
		}
	}
}

void CompanionWheelMenu::OpenInventory() {
	CompanionWheelMenu::Close(1);
	TESForm* formByEditorID = TESForm::GetFormByEditorID("FollowersTrade");
	TESTopic* a2 = dynamic_cast<TESTopic*>(formByEditorID);
	if (m_pRelatedCompanion) {
		SpeakTopic(a2);
		RunTopicScript(a2);
	}
}

void CompanionWheelMenu::HandleMouseoverForTile(int32_t aiID, uint8_t a2) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	if (a2 || m_CurrentlySelectedTile != aiID) {
		BSAudio* v3 = &BSAudio::QInstance();
		BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&a1, "UIPipBoyScroll", 289u);
		SoundHandleByName->Play(0);
		if (m_CurrentlySelectedTile != CompanionWheelMenu::TI_NO_TILE) {
			if (pTiles[m_CurrentlySelectedTile]) {
				pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User10, 0);
			}
		}
		switch (m_CurrentlySelectedTile) {
		case CompanionWheelMenu::TI_AGGRESSIVE_PASSIVE_BUTTON: {
			if (pTiles[m_CurrentlySelectedTile]) {
				pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User11,
					static_cast<int32_t>(m_bAggressiveStance));
			}
			break;
		}
		case CompanionWheelMenu::TI_STAY_FOLLOW_BUTTON: {
			if (pTiles[m_CurrentlySelectedTile]) {
				pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bFollowing));
			}
			break;
		}
		case CompanionWheelMenu::TI_NEAR_FAR_BUTTON: {
			if (pTiles[m_CurrentlySelectedTile]) {
				pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User11, static_cast<int32_t>(m_bFollowingLong));
			}
			break;
		}
		case CompanionWheelMenu::TI_RANGED_MELEE_BUTTON: {
			if (pTiles[m_CurrentlySelectedTile]) {
				pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User11, static_cast<int32_t>(m_bRangedCombat));
			}
			break;
		}
		default: {
			break;
		}
		}
		m_CurrentlySelectedTile = static_cast<CompanionWheelMenu::TILE_INDEX>(aiID);
		if (m_CurrentlySelectedTile != CompanionWheelMenu::TI_NO_TILE) {
			if (pTiles[m_CurrentlySelectedTile]) {
				pTiles[m_CurrentlySelectedTile]->Set(Tile::kTileValue_User10, 1);
			}
		}
		switch (aiID) {
		case 0: {
			const char* v8;
			if (m_bAggressiveStance) {
				v8 = sCWheelBePassive.String();
			} else {
				v8 = sCWheelBeAggressive.String();
			}
			SetButtonPreviewText(v8);
			if (pTiles[aiID]) {
				pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bAggressiveStance));
			}
			break;
		}
		case 1: {
			const char* v9 = gs_sCWheelUseStimpak.String();
			SetButtonPreviewText(v9);
			break;
		}
		case 2: {
			const char* v6;
			if (m_bFollowing) {
				v6 = sCWheelWaitHere.String();
			} else {
				v6 = sCWheelFollowMe.String();
			}
			SetButtonPreviewText(v6);
			if (pTiles[aiID]) {
				pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(m_bFollowing));
			}
			break;
		}
		case 3: {
			const char* v5 = sCWheelTalkTo.String();
			SetButtonPreviewText(v5);
			break;
		}
		case 4: {
			const char* v7 = sCWheelBackUp.String();
			SetButtonPreviewText(v7);
			break;
		}
		case 5: {
			const char* v12;
			if (m_bFollowingLong) {
				v12 = sCWheelStayClose.String();
			} else {
				v12 = sCWheelKeepDistance.String();
			}
			SetButtonPreviewText(v12);
			if (pTiles[aiID]) {
				pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bFollowingLong));
			}
			break;
		}
		case 6: {
			const char* v11 = sCWheelOpenInventory.String();
			SetButtonPreviewText(v11);
			break;
		}
		case 7: {
			const char* v10;
			if (m_bRangedCombat) {
				v10 = sCWheelUseMelee.String();
			} else {
				v10 = sCWheelUseRanged.String();
			}
			SetButtonPreviewText(v10);
			if (pTiles[aiID]) {
				pTiles[aiID]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bRangedCombat));
			}
			break;
		}
		case 11: {
			const char* v4 = sExit.String();
			SetButtonPreviewText(v4);
			break;
		}
		default: {
			SetButtonPreviewText("");
			break;
		}
		}
		SetButtonContextText(m_CurrentlySelectedTile);
	}
}

void CompanionWheelMenu::SetButtonAsHighlighted(float a1) {
	if (!(a1 >= 0.0) || !(a1 < 0.5235980153083801)) {
		if (!(a1 >= 2.617990016937256) || !(a1 < 3.141590118408203)) {
			if (!(a1 >= 3.14159) || !(a1 < 3.665186)) {
				if (a1 >= 5.759578) {
					a1 = 5.7f;
				}
			} else {
				a1 = 3.7f;
			}
		} else {
			a1 = 2.6f;
		}
	} else {
		a1 = 0.55f;
	}
	for (int32_t iter = 0; iter < 16; ++iter) {
		RadialTile* pRadialTileTest = dynamic_cast<RadialTile*>(pTiles[iter]);
		if (pRadialTileTest) {
			float fMinAngle = pRadialTileTest->GetFloat(Tile::kTileValue_User2);
			float fMaxAngle = pRadialTileTest->GetFloat(Tile::kTileValue_User3);
			if (fMinAngle <= static_cast<double>(a1) && fMaxAngle > static_cast<double>(a1)) {
				HandleMouseoverForTile(iter, 0);
				return;
			}
		}
	}
}

void CompanionWheelMenu::Do_ToggleNearFar() {
	m_bFollowingLong = !m_bFollowingLong;
	if (m_bFollowingLong) {
		TESForm* FormByEditorID = TESForm::GetFormByEditorID("FollowersTacticsDistanceLong");
		TESTopic* a2 = dynamic_cast<TESTopic*>(FormByEditorID);
		SpeakTopic(a2);
		RunTopicScript(a2);
	} else {
		TESForm* v2 = TESForm::GetFormByEditorID("FollowersTacticsDistanceDefault");
		TESTopic* a1 = dynamic_cast<TESTopic*>(v2);
		SpeakTopic(a1);
		RunTopicScript(a1);
	}
	if (pTiles[5]) {
		pTiles[5]->Set(Tile::kTileValue_User11, m_bFollowingLong);
	}
}

void CompanionWheelMenu::Do_ToggleAggressivePassive() {
	float v3;
	m_bAggressiveStance = !m_bAggressiveStance;
	if (m_bAggressiveStance) {
		TESForm* FormByEditorID = TESForm::GetFormByEditorID("FollowersTacticsCombatAggressive");
		TESTopic* a2 = dynamic_cast<TESTopic*>(FormByEditorID);
		SpeakTopic(a2);
		RunTopicScript(a2);
	} else {
		TESForm* v2 = TESForm::GetFormByEditorID("FollowersTacticsCombatPassive");
		TESTopic* a1 = dynamic_cast<TESTopic*>(v2);
		SpeakTopic(a1);
		RunTopicScript(a1);
	}
	if (m_bAggressiveStance) {
		v3 = 1.0f;
	} else {
		v3 = 0.0f;
	}
	CompanionWheelMenu::pMe->m_pRelatedCompanion->SetLocalVariableOnActor("FollowerSwitchAggressive", v3);
	if (pTiles[0]) {
		pTiles[0]->Set(Tile::kTileValue_User11, static_cast<int32_t>(m_bAggressiveStance));
	}
}

void CompanionWheelMenu::Do_ToggleRangedMelee() {
	if (m_pRelatedCompanion) {
		m_bRangedCombat = !m_bRangedCombat;
		if (m_bRangedCombat) {
			TESForm* FormByEditorID = TESForm::GetFormByEditorID("FollowersTacticsCombatRanged");
			TESTopic* a2 = dynamic_cast<TESTopic*>(FormByEditorID);
			SpeakTopic(a2);
			RunTopicScript(a2);
		} else {
			TESForm* v2 = TESForm::GetFormByEditorID("FollowersTacticsCombatMelee");
			TESTopic* a1 = dynamic_cast<TESTopic*>(v2);
			SpeakTopic(a1);
			RunTopicScript(a1);
		}
		if (pTiles[7]) {
			pTiles[7]->Set(Tile::kTileValue_User11, static_cast<int32_t>(CompanionWheelMenu::pMe->m_bRangedCombat));
		}
	}
}

void CompanionWheelMenu::Do_ToggleStayFollow() {
	m_bFollowing = !m_bFollowing;
	if (m_bFollowing) {
		TESForm* FormByEditorID = TESForm::GetFormByEditorID("FollowersLetsGo");
		TESTopic* a1 = dynamic_cast<TESTopic*>(FormByEditorID);
		SpeakTopic(a1);
		RunTopicScript(a1);
	} else {
		TESForm* v1 = TESForm::GetFormByEditorID("FollowersWait");
		TESTopic* a2 = dynamic_cast<TESTopic*>(v1);
		SpeakTopic(a2);
		RunTopicScript(a2);
	}
	if (pTiles[2]) {
		pTiles[2]->Set(Tile::kTileValue_User11, static_cast<int32_t>(!m_bFollowing));
	}
}

void CompanionWheelMenu::Do_UseStim() {
	if (this->m_pRelatedCompanion) {
		if (pPlayer) {
			TESForm* DefaultObject = BGSDefaultObjectManager::GetDefaultObject(DO_STIMPAK);
			TESBoundObject* apObject = dynamic_cast<TESBoundObject*>(DefaultObject);
			if (apObject) {
				ItemChange* InventoryItem_0 = pPlayer->GetInventoryItem(apObject, 0);
				if (InventoryItem_0) {
					double BodyPartCondition;
					double v3;
					double v4;
					double v5;
					double v6;
					double v7;
					float HealthPercentage = this->m_pRelatedCompanion->GetHealthPercentage();
					int32_t v20 = 0;
					Actor* m_pRelatedCompanion = this->m_pRelatedCompanion;
					if (m_pRelatedCompanion) {
						BodyPartCondition = CombatFormulas::GetBodyPartCondition(m_pRelatedCompanion,
							ActorValue::LEFT_ATTACK_CONDITION, 0);
					} else {
						BodyPartCondition =
							CombatFormulas::GetBodyPartCondition(0, ActorValue::LEFT_ATTACK_CONDITION, 0);
					}
					float v24 = BodyPartCondition;
					if (v24 == 0.0) {
						v20 = 1;
					}
					Actor* v16 = this->m_pRelatedCompanion;
					if (v16) {
						v3 = CombatFormulas::GetBodyPartCondition(v16, ActorValue::LEFT_MOBILITY_CONDITION, 0);
					} else {
						v3 = CombatFormulas::GetBodyPartCondition(0, ActorValue::LEFT_MOBILITY_CONDITION, 0);
					}
					float v27 = v3;
					if (v27 == 0.0) {
						++v20;
					}
					Actor* v15 = this->m_pRelatedCompanion;
					if (v15) {
						v4 = CombatFormulas::GetBodyPartCondition(v15, ActorValue::RIGHT_MOBILITY_CONDITION, 0);
					} else {
						v4 = CombatFormulas::GetBodyPartCondition(0, ActorValue::RIGHT_MOBILITY_CONDITION, 0);
					}
					float v25 = v4;
					if (v25 == 0.0) {
						++v20;
					}
					Actor* v14 = this->m_pRelatedCompanion;
					if (v14) {
						v5 = CombatFormulas::GetBodyPartCondition(v14, ActorValue::RIGHT_ATTACK_CONDITION, 0);
					} else {
						v5 = CombatFormulas::GetBodyPartCondition(0, ActorValue::RIGHT_ATTACK_CONDITION, 0);
					}
					float v21 = v5;
					if (v21 == 0.0) {
						++v20;
					}
					Actor* v13 = this->m_pRelatedCompanion;
					if (v13) {
						v6 = CombatFormulas::GetBodyPartCondition(v13, ActorValue::PERCEPTION_CONDITION, 0);
					} else {
						v6 = CombatFormulas::GetBodyPartCondition(0, ActorValue::PERCEPTION_CONDITION, 0);
					}
					float v23 = v6;
					if (v23 == 0.0) {
						++v20;
					}
					Actor* v12 = this->m_pRelatedCompanion;
					if (v12) {
						v7 = CombatFormulas::GetBodyPartCondition(v12, ActorValue::ENDURANCE_CONDITION, 0);
					} else {
						v7 = CombatFormulas::GetBodyPartCondition(0, ActorValue::ENDURANCE_CONDITION, 0);
					}
					float v26 = v7;
					if (v26 == 0.0) {
						++v20;
					}
					if (HealthPercentage < 1.0 || v20 > 0) {
						ExtraDataList* m_item;
						if (HealthPercentage < 1.0) {
							ExtraDataList* apList;
							if (InventoryItem_0->pExtraObjectList) {
								apList = InventoryItem_0->pExtraObjectList->m_item;
							} else {
								apList = 0;
							}
							AlchemyItem* v8 = dynamic_cast<AlchemyItem*>(InventoryItem_0->pContainerObj);
							this->m_pRelatedCompanion->DrinkPotion(v8, apList, 1);
						}
						if (InventoryItem_0->pExtraObjectList) {
							m_item = InventoryItem_0->pExtraObjectList->m_item;
						} else {
							m_item = 0;
						}
						TESBoundObject* FormFlags = InventoryItem_0->pContainerObj;
						pPlayer->RemoveItem(FormFlags, m_item, 1, 0, 0, 0, 0, 0, 1, 0);
						this->m_pRelatedCompanion->UpdateMagic(0, 1);
						this->m_pRelatedCompanion->RestoreActorValue(ActorValue::BRAIN_CONDITION, 1000.0f);
						this->m_pRelatedCompanion->RestoreActorValue(ActorValue::LEFT_MOBILITY_CONDITION, 1000.0f);
						this->m_pRelatedCompanion->RestoreActorValue(ActorValue::RIGHT_MOBILITY_CONDITION, 1000.0f);
						this->m_pRelatedCompanion->RestoreActorValue(ActorValue::LEFT_ATTACK_CONDITION, 1000.0f);
						this->m_pRelatedCompanion->RestoreActorValue(ActorValue::RIGHT_ATTACK_CONDITION, 1000.0f);
						this->m_pRelatedCompanion->RestoreActorValue(ActorValue::ENDURANCE_CONDITION, 1000.0f);
						this->m_pRelatedCompanion->RestoreActorValue(ActorValue::PERCEPTION_CONDITION, 1000.0f);
						TESTopic* BarkTopicByType = FollowerBarks::GetBarkTopicByType(FollowerBarks::BARK_REGENERATING);
						SpeakTopic(BarkTopicByType);
					}
					delete InventoryItem_0;
				}
			}
		}
	}
}

void CompanionWheelMenu::TransitionFromCloseToTalkTo() {
	Actor* pActionActor = pPlayer;
	Actor* pactor = m_pRelatedCompanion;
	if (pactor) {
		if (!pactor->IsDying()) {
			BaseProcess* pprocess = pactor->pCurrentProcess;
			if (pprocess) {
				if ((!pprocess->GetCommandingActor() || pactor->IsDead(0)) && !pprocess->GetFadeState()) {
					BaseProcess* pPlayerProc = pActionActor->pCurrentProcess;
					TESPackage* pactionpackage = pPlayerProc->GetPackageThatIsRunning();
					TESPackage* potherpackage = pprocess->GetPackageThatIsRunning();
					if (pActionActor) {
						BaseProcess* v22;
						BaseProcess* v23;
						if (pactor->IsInCombat() && (pActionActor == pPlayer && pactor->IsPlayerTeammate())) {
							pVATS.QuitVATSPlayback(0, 0);
							Interface::QueueMenuCreate(Interface::OPEN_DIALOG, pactor, 0, 0, Interface::CMM_STANDARD,
								0);
							return;
						}
						if ((!pactor->IsInCombat() || pactor->IsKnockedOut()) &&
							(pActionActor != pPlayer ||
								(pactor->GetSitSleepState() == SIT_SLEEP_NORMAL ||
									(pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING ||
										(pactor->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
											((v23 = pactor->pCurrentProcess, v23->GetFurnitureMarker()) &&
												(v22 = pactor->pCurrentProcess,
													v22->GetFurnitureMarker()->cNumber > 19u))))))) {
							if (!pactor->IsReanimated() && !pactor->IsDead(0) &&
								(pActionActor != pPlayer || (!pactor->IsKnockedOut() || !pactor->GetEssential()))) {
								TESPackage* v1;
								TESPackage* v2;
								TESPackage* v3;
								if (pActionActor == pPlayer ||
									(pactor == pPlayer &&
										((v1 = pActionActor->GetCurrentPackage(), v1->IsInterruptPackage()) ||
											((v2 = pActionActor->GetCurrentPackage(),
												 v2->GetPackType() == PACKAGE_TYPE_EXPLORE) ||
												(v3 = pActionActor->GetCurrentPackage(),
													v3->GetPackType() == PACKAGE_TYPE_AMBUSH))))) {
									if (pActionActor != pPlayer ||
										((!pactor->IsKnockedOut() &&
											 (!pPlayer->IsSneaking() || pactor->IsPlayerTeammate())) ||
											pactor->GetGhost())) {
										BaseProcess* CurrentAIProcess;
										TESTopic* pgreetTopic = 0;
										if (pActionActor == pPlayer ||
											(CurrentAIProcess = pActionActor->pCurrentProcess,
												CurrentAIProcess->GetNeedTalkPlayer())) {
											pgreetTopic = TESTopic::GetTopic(DT_TOPIC, 0);
										}
										DialogueItem* pgreetDialogue = 0;
										if (pgreetTopic) {
											if (pActionActor == pPlayer) {
												pgreetDialogue =
													pgreetTopic->CreateDialogueItem(pactor, pPlayer, 0, 0, 0);
											} else {
												BaseProcess* v20 = pActionActor->pCurrentProcess;
												if (v20->GetNeedTalkPlayer()) {
													pgreetDialogue =
														pgreetTopic->CreateDialogueItem(pActionActor, pPlayer, 0, 0, 0);
												}
											}
										}
										if (pgreetDialogue &&
											(pgreetDialogue->pInfo->IsGoodbye() &&
												(pgreetDialogue->FirstResponse() && !pgreetDialogue->NextResponse()))) {
											if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
												BaseProcess* v19 = pactor->pCurrentProcess;
												v19->SetNeedTalkPlayer(1);
												pactor->InitiateGetUpPackage();
												return;
											}
											if (pactor == pPlayer) {
												pActionActor->SayForceGoodbyeGreetingToPlayer(pgreetTopic, 0, 0);
											} else {
												pactor->SayForceGoodbyeGreetingToPlayer(pgreetTopic, 0, 0);
											}
										} else {
											BaseProcess* v18 = pactor->pCurrentProcess;
											if (v18->GetFurnitureMarker()) {
												BaseProcess* v17 = pactor->pCurrentProcess;
												if (v17->GetFurnitureMarker()->cNumber > 19u) {
													BaseProcess* v16 = pactor->pCurrentProcess;
													v16->AddPostAnimationAction(BaseProcess::
															POSTANIM_ACTION_GETOUT_OF_FURNITURE);
												}
											}
											if (pactor->GetSitSleepState() == SIT_SLEEP_NORMAL) {
												NiPoint3 v40 = pActionActor->GetLocationOnReference();
												pactor->RequestRotate(v40, static_cast<bool>(0));
											}
											TESTopic* ptopic = 0;
											Actor* pspeaker = 0;
											if (pActionActor == pPlayer) {
												if (!pactor->IsPathingComplete() && !pactor->IsRotating()) {
													BaseProcess* v15 = pactor->pCurrentProcess;
													v15->EndMoveMessage(pactor);
												}
												pActionActor->StopAttack();
												pactor->StopAttack();
												TESPackage* CurrentPackage = pActionActor->GetCurrentPackage();
												DialoguePackage* ppack = dynamic_cast<DialoguePackage*>(CurrentPackage);
												if (ppack) {
													ptopic = ppack->ptopic;
												}
												pspeaker = m_pRelatedCompanion;
												if (pactor->GetCurrentTarget() == pPlayer) {
													if (potherpackage) {
														if (potherpackage->GetPackType() == PACKAGE_TYPE_EXPLORE) {
															if (!potherpackage->GetPackageSchedule() ||
																potherpackage->GetPackageSchedule()->PSData.iDuration) {
																pprocess->AddToProcedureIndexRunning(pactor, 1);
															} else {
																pprocess->AddToProcedureIndexRunning(pActionActor, 2);
															}
														}
													}
												}
												BSAudio* v5 = &BSAudio::QInstance();
												v5->EndAllSoundsOfType(4u);
												pactor->InterruptCast();
												if (pactor->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
													BaseProcess* v14 = pactor->pCurrentProcess;
													v14->SetNeedTalkPlayer(1);
													pactor->InitiateGetUpPackage();
												} else {
													Actor* pact = pActionActor;
													if (pActionActor == pPlayer) {
														pact = pactor;
													}
													pVATS.QuitVATSPlayback(0, 0);
													Interface::QueueMenuCreate(Interface::OPEN_DIALOG, pact, ptopic, 0,
														Interface::CMM_STANDARD, 0);
												}
											} else {
												if (Interface::GetMenuModeType() != Interface::eDialogMenu) {
													BaseProcess* v13 = pPlayer->pCurrentProcess;
													if (!v13->CanForceGreet(pActionActor)) {
														return;
													}
													BSAudio* v6 = &BSAudio::QInstance();
													v6->EndAllSoundsOfType(4u);
													pActionActor->StopAttack();
													pactor->StopAttack();
													pspeaker = pActionActor;
													if (!pActionActor->IsInCombat()) {
														TESPackage* v7 = pActionActor->GetCurrentPackage();
														DialoguePackage* ppack_2 = dynamic_cast<DialoguePackage*>(v7);
														if (ppack_2) {
															ptopic = ppack_2->ptopic;
														}
													}
													if (!pactionpackage->GetPackageSchedule() ||
														pactionpackage->GetPackageSchedule()->PSData.iDuration) {
														BaseProcess* v11 = pActionActor->pCurrentProcess;
														v11->AddToProcedureIndexRunning(pActionActor, 1);
													} else {
														BaseProcess* pprocess_2 = pActionActor->pCurrentProcess;
														pprocess_2->AddToProcedureIndexRunning(pActionActor, 2);
													}
													pactor->InterruptCast();
													if (pactor == pPlayer) {
														pactor->ForceIdleAnimation();
													} else {
														BaseProcess* v10 = pactor->pCurrentProcess;
														v10->AddPostAnimationAction(BaseProcess::
																POSTANIM_FORCEIDLE_ANIMATION);
													}
													if (pActionActor == pPlayer) {
														pActionActor->ForceIdleAnimation();
													} else {
														BaseProcess* v9 = pActionActor->pCurrentProcess;
														v9->AddPostAnimationAction(BaseProcess::
																POSTANIM_FORCEIDLE_ANIMATION);
													}
													pVATS.QuitVATSPlayback(0, 0);
													Actor* pact_2 = pActionActor;
													if (pActionActor == pPlayer) {
														pact_2 = pactor;
													}
													Interface::QueueMenuCreate(Interface::OPEN_DIALOG, pact_2, ptopic,
														0, Interface::CMM_STANDARD, 0);
												}
											}
										}
										if (pgreetDialogue) {
											delete pgreetDialogue;
										}
									}
								}
							}
						} else {
							if (pactor->GetCombatController()) {
								if (pActionActor == pPlayer && pactor->GetCombatController()->QSwimAllowed()) {
									char msg[200];
									const char* v8 = sActivateNPCCalmed.String();
									std::sprintf(msg, "%s %s", pactor->GetFullName(), v8);
									Interface::ShowMessage(msg, 0, 0, 0, 2.0f, 0);
								}
							}
						}
					}
				}
			}
		}
	}
}

void CompanionWheelMenu::DisplayCompanionWheelSubtitle(BSStringT<char> asText, BSSoundHandle ahSound) {
	if (asText.GetLength()) {
		if (gs_bGeneralSubtitles_Gameplay.Bool() ||
			(m_pRelatedCompanion && static_cast<uint8_t>(m_pRelatedCompanion->GetShowSubtitles()))) {
			if (pTiles[12]) {
				pTiles[12]->Set(4036, asText.pString, 1);
			}
			float Duration = static_cast<float>(ahSound.GetDuration());
			if (Duration == 0.0) {
				double v3 = static_cast<double>(std::strlen(asText.pString));
				Duration = fNoticeTextTimePerCharacter.Float() * v3 * 1000.0;
			}
			m_nTimeToClearSubtitle = static_cast<int64_t>(static_cast<double>(BSGetTickCount()) + Duration);
		}
	}
}

bool CompanionWheelMenu::CreateCompanionWheelMenu(Actor* apRelatedCompanion) {
	CompanionWheelMenu* pmenu;
	Tile* ptile = Menu::GetMenuByClass(1075);
	if (ptile) {
		Menu* Menu_ = ptile->GetMenu();
		pmenu = dynamic_cast<CompanionWheelMenu*>(Menu_);
	} else {
		pmenu = 0;
	}
	if (pmenu) {
		delete pmenu;
	}
	return CompanionWheelMenu::Create(apRelatedCompanion) != 0;
}

void CompanionWheelMenu::Close(bool abClearOutSpeech) {
	if (CompanionWheelMenu::pMe) {
		if (abClearOutSpeech) {
			CompanionWheelMenu::pMe->m_hCurrentReactivityBark.FadeOutAndRelease(100u);
		}
		if (!pSaveLoadGame->GetSaveGameLoading()) {
			ReferenceProcessLists.Update3DAfterResting();
		}
		CompanionWheelMenu::pMe->pMenu->Set(Tile::kTileValue_DeleteOnFade, 1);
		CompanionWheelMenu::pMe->StartFadeOut();
	}
}

Tile* CompanionWheelMenu::Create(Actor* apRelatedCompanion) {
	int32_t v2;
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	InterfaceManager* pin = InterfaceManager::Instance();
	int32_t v1 = return_1075();
	Tile* pold_menu_tile = Menu::GetMenuByClass(v1);
	if (pold_menu_tile) {
		delete pold_menu_tile;
	}
	Tile* ptile = pin->GetMenusRoot()->ReadFile("Data\\Menus\\companion_wheel_menu.xml");
	CompanionWheelMenu::pMe = static_cast<CompanionWheelMenu*>(ptile->GetMenu());
	if (CompanionWheelMenu::pMe && (v2 = CompanionWheelMenu::pMe->GetClass(), v2 == return_1075())) {
		CompanionWheelMenu::pMe->m_pRelatedCompanion = apRelatedCompanion;
		if (CompanionWheelMenu::pMe->m_pRelatedCompanion) {
			CompanionWheelMenu::pMe->m_bAggressiveStance =
				CompanionWheelMenu::pMe->m_pRelatedCompanion->GetLocalVariableOnActor("FollowerSwitchAggressive") ==
				1.0;
			CompanionWheelMenu::pMe->m_bFollowingLong =
				CompanionWheelMenu::pMe->m_pRelatedCompanion->GetLocalVariableOnActor("IsFollowingLong") == 1.0;
			CompanionWheelMenu::pMe->m_bRangedCombat =
				CompanionWheelMenu::pMe->m_pRelatedCompanion->GetLocalVariableOnActor("CombatStyleRanged") == 1.0;
			CompanionWheelMenu::pMe->m_bFollowing =
				CompanionWheelMenu::pMe->m_pRelatedCompanion->GetLocalVariableOnActor("Waiting") == 0.0;
		}
		if (CompanionWheelMenu::pMe->pTiles[0]) {
			CompanionWheelMenu::pMe->pTiles[0]->Set(Tile::kTileValue_User11,
				static_cast<int32_t>(CompanionWheelMenu::pMe->m_bAggressiveStance));
		}
		if (CompanionWheelMenu::pMe->pTiles[2]) {
			CompanionWheelMenu::pMe->pTiles[2]->Set(Tile::kTileValue_User11,
				static_cast<int32_t>(!CompanionWheelMenu::pMe->m_bFollowing));
		}
		if (CompanionWheelMenu::pMe->pTiles[5]) {
			CompanionWheelMenu::pMe->pTiles[5]->Set(Tile::kTileValue_User11,
				static_cast<int32_t>(CompanionWheelMenu::pMe->m_bFollowingLong));
		}
		if (CompanionWheelMenu::pMe->pTiles[7]) {
			CompanionWheelMenu::pMe->pTiles[7]->Set(Tile::kTileValue_User11,
				static_cast<int32_t>(CompanionWheelMenu::pMe->m_bRangedCombat));
		}
		if (CompanionWheelMenu::pMe->pTiles[12]) {
			CompanionWheelMenu::pMe->pTiles[12]->Set(4036, "", 1);
		}
		if (CompanionWheelMenu::pMe->pTiles[8]) {
			const char* v3 = sCWheelTitle.String();
			CompanionWheelMenu::pMe->pTiles[8]->Set(4036, v3, 1);
		}
		if (CompanionWheelMenu::pMe->pTiles[13]) {
			const char* v4 = sExit.String();
			CompanionWheelMenu::pMe->pTiles[13]->Set(4036, v4, 1);
		}
		if (CompanionWheelMenu::pMe->pTiles[14]) {
			const char* v5 = sSelect.String();
			CompanionWheelMenu::pMe->pTiles[14]->Set(4036, v5, 1);
		}
		if (CompanionWheelMenu::pMe->pTiles[15]) {
			const char* v6 = sNavigate.String();
			CompanionWheelMenu::pMe->pTiles[15]->Set(4036, v6, 1);
		}
		CompanionWheelMenu::pMe->SetMenuTile(static_cast<TileMenu*>(ptile), 0);
		if (CompanionWheelMenu::pMe->IsWellFormedMenu()) {
			BSAudio* v8 = &BSAudio::QInstance();
			BSSoundHandle* SoundHandleByName = v8->GetSoundHandleByName(&a1, "UIPopUpMessageGeneral", 289u);
			SoundHandleByName->Play(0);
			if (ptile->GetFloat(4007) == 6006.0 || ptile->GetFloat(4007) == 102.0) {
				float afValue = Tile::GetMaximumDepth();
				ptile->Set(4013, afValue, 1);
			}
			CompanionWheelMenu::pMe->PrepForVisibility(0);
			return ptile;
		} else {
			Warning("MENUS: Companion Wheel Menu Creation Failed... Are your menu and art resources up to date?");
			return 0;
		}
	} else {
		if (CompanionWheelMenu::pMe) {
			if (CompanionWheelMenu::pMe->pMenu) {
				if (CompanionWheelMenu::pMe) {
					delete CompanionWheelMenu::pMe;
				}
			}
		}
		return 0;
	}
}

void InterfaceManager::InternalSetMenuMode(eMenuModeType aiMode) {
	this->cMenuMode = aiMode;
}
