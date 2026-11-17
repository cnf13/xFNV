#include "Fallout/Interface/Menus/HUDMainMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSCore/TLSData.h"
#include "BSDevices/Controls.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "BSMenu/Tile.h"
#include "BSMenu/TileMenu.h"
#include "BSMenu/TileRect.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/ShadowSceneNode.h"
#include "BSShader/TileShaderProperty.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSEntryPoint.h"
#include "Fallout Shared/BGSOpenCloseForm.h"
#include "Fallout Shared/BGSPerk.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSQuestObjective.h"
#include "Fallout Shared/BGSRagdoll.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GamePlayFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESActorBaseData.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESChallengeManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESQuest.h"
#include "Fallout Shared/TESQuestTarget.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout Shared/TESWeightForm.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Character.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Audio/FalloutAudio.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Interface/HotKeysWheel.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Interface/Menus/CreditsMenu.h"
#include "Fallout/Interface/Menus/DialogMenu.h"
#include "Fallout/Interface/Menus/LoveTesterMenu.h"
#include "Fallout/Interface/Menus/MapMenu.h"
#include "Fallout/Interface/Menus/SPECIALBookMenu.h"
#include "Fallout/Interface/Menus/StartMenu.h"
#include "Fallout/Interface/Menus/StatsMenu.h"
#include "Fallout/Interface/Menus/TraitMenu.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerSequence.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cmath>
#include <stdint.h>

// UpdateWeaponStatus's function-local statics (the PDB's names): the last shown loaded and remaining ammo and condition
// percentage (FLT_MIN at load: nothing shown yet), whether the bar's pulse is fading down, and the weapon last shown.
// The three text buffers are the ammo type text (cleared every call, then the abbreviation; the PDB names both
// ammosubtype) and the ammo count text.
static float flastWeapHealthPercent = 1.1754944e-38f;
static bool bWeapFadeDecrease = true;
static float flastTotalWeaponAmmo = 1.1754944e-38f;
static float flastWeaponAmmo = 1.1754944e-38f;

// UpdateHotkeysWheel's function-local statics (the PDB's names): the hotkey whose name and icon the HUD shows (-1:
// none), the timer's time when a hotkey was last pressed or the wheel last used (0: not timing) and whether a gamepad
// hotkey is being held.
static int32_t ilastHighlight = -1;

// The scale Draw3DElements passes for the scope camera's Eyefinity adjustment.
static float fScopeEyeFinityScale = 0.1f;

// Set while the enemy health meter is hidden; SetMenuMode then drops it from every mode.
static bool bHideEnemyHealthMeter;

// Whether the compass shows the current enemy as a hostile, detected target.
static bool bCurrentEnemyHostile;

// DoIdle's last GetActivationIsCrime of the crosshair reference (the map's name).
static bool bCheckSneakChanged;

// The ammo text buffers' type (the PDB's char[260]).
using AmmoTextBuffer = char[260];
static AmmoTextBuffer ammo{};
static TESObjectWEAP* plastWeaponObj;
static AmmoTextBuffer ammosubtype2{};
static AmmoTextBuffer ammosubtype{};

// SetInfoForRef's function-local text buffers: the item weight (wg), the item value (caps), the crosshair reference's
// name (IDA name; not in the PDB) as it last built it, and the lock state.
using PickupPromptString = char[8];
static PickupPromptString wg{};
static PickupPromptString caps{};
using InfoString = char[0x104];
static InfoString g_sCrosshairRefName{};
static InfoString lockLevel{};
static int32_t iHotkeyStartTime;
static bool bwheelBeingUsed;

// UpdateShake's function-local static (the PDB's name): whether the last frame shook the tile.
static bool bupdate;
SettingT<INISettingCollection> fXenonMenuStickSpeedPlayerRotMod("fXenonMenuStickSpeedPlayerRotMod:Controls", 3000.0f);
SettingT<INISettingCollection> HUDMainMenu::fHUDShakeMult("fHUDShakeMult:Interface", 10.0f);
SettingT<INIPrefSettingCollection> fHudOpacity("fHudOpacity:Interface", 1.0f);
NiPointer<NiNode> HUDMainMenu::spScopeModel;
NiPointer<NiCamera> HUDMainMenu::spScopeCamera;
using QuestUpdateList = BSSimpleList<QuestUpdateManager::QuestUpdate*>;
QuestUpdateList QuestUpdateManager::xQuestNames;
BSCriticalSection HUDMainMenu::HUDShowQuestNameCritSect;
BSCriticalSection HUDMainMenu::HUDShowQuestObjectivsCritSect;
BSCriticalSection HUDMainMenu::HUDCritSect;
float fCurrentHUDOpacity = fHudOpacity.Float() * 255.0;

// The vtable of BSSimpleArray<HUDMainMenu::SubtitleData*,1024>.
inline constexpr fnvd::Vtable BSSimpleArray_HUDMainMenu_SubtitleData_vftable{};

using HUDMainMenuSubtitleArray = BSSimpleArray<HUDMainMenu::SubtitleData*, 1024>;

// The victim's damage threshold after both actors' perk entry points, at least 0.
float GetAdjustedDamageThreshold(Actor* pAttacker, Actor* pVictim) {
	if (!pAttacker || !pVictim) {
		return 0.0f;
	}
	float fThreshold =
		BSMAX(static_cast<ActorValueOwner*>(pVictim)->GetActorFloatValue(ActorValue::DAMAGE_THRESHOLD), 0.0f);
	BaseProcess* pProcess = pAttacker->GetCurrentAIProcess();
	ItemChange* pWeaponChange = pProcess->GetCurrentWeapon();
	TESForm* pWeapon;
	if (pWeaponChange) {
		pWeapon = pWeaponChange->GetContainerObject();
	} else {
		pWeapon = pFists;
	}
	float fAttackerMod = 0.0f;
	BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_ATTACKER, pAttacker, pWeapon, pVictim,
		&fAttackerMod);
	float fDefenderMod = 0.0f;
	BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_DEFENDER, pVictim, pAttacker, pWeapon,
		&fDefenderMod);
	fThreshold = fThreshold - fAttackerMod + fDefenderMod;
	fThreshold = BSMAX(0.0f, fThreshold);
	return fThreshold;
}

// The exe's cleanup-only C++ EH frame is left out.
void HandleQuestObjectiveBlinking(Tile* th_s, float value, float a3) {
	float x1;
	float x2;
	float y1;
	float y2;
	static int32_t iUT_AlphaDown = Tile::AddUserTrait("_AlphaDown", -1);
	int32_t v26 = SquareInt(iHUDCompassIconBlinkMaxDist.Int());
	int32_t v21 = SquareInt(iHUDCompassIconBlinkMinDist.Int());
	int32_t v22 = SquareInt(iHUDCompassIconBlinkThresholdDist.Int());
	if (!(value > static_cast<double>(v26))) {
		if (value < static_cast<double>(v21)) {
			value = static_cast<float>(v21);
		}
	} else {
		value = static_cast<float>(v26);
	}
	// The exe tests below-threshold once (false on NaN) and keeps the byte for both lerps.
	bool bBelowThreshold = value < static_cast<double>(v22);
	float Float = th_s->GetFloat(iUT_AlphaDown);
	if (bBelowThreshold) {
		y2 = static_cast<float>(v21);
		y1 = static_cast<float>(v22);
		x2 = static_cast<float>(iHUDCompassIconBlinkFastestBlinkTime.Int());
		x1 = static_cast<float>(iHUDCompassIconBlinkThresholdBlinkTime.Int());
	} else {
		y2 = static_cast<float>(v22);
		y1 = static_cast<float>(v26);
		x2 = static_cast<float>(iHUDCompassIconBlinkThresholdBlinkTime.Int());
		x1 = static_cast<float>(iHUDCompassIconBlinkSlowestBlinkTime.Int());
	}
	float v20 = FLerp(x1, x2, y1, y2, value);
	float v23 = 0.0;
	if (Float == 0.0) {
		double v16;
		if (fCurrentHUDOpacity == 0.0) {
			v16 = 0.0;
		} else {
			double v18 = th_s->GetFloat(4009);
			double v17 = v20 * a3 / fCurrentHUDOpacity;
			v16 = appTimer.fDelta * v17 + v18;
		}
		float v24 = v16;
		th_s->Set(4009, v24, 1);
	} else {
		if (Float == 1.0) {
			double v13;
			if (fCurrentHUDOpacity == 0.0) {
				v13 = 0.0;
			} else {
				double v15 = th_s->GetFloat(4009);
				double v14 = v20 * a3 / fCurrentHUDOpacity;
				v13 = v15 - appTimer.fDelta * v14;
			}
			float v25 = v13;
			th_s->Set(4009, v25, 1);
		} else {
			if (Float == 3.0) {
				v23 = fCurrentHUDOpacity;
			}
			th_s->Set(4009, v23, 1);
		}
	}
	float v27 = th_s->GetFloat(4009);
	if (!(a3 < static_cast<double>(v27))) {
		if (!(v27 < 0.0)) {
			double v11 = th_s->GetFloat(Tile::kTileValue_User10);
			if (static_cast<double>(appTimer.iLastTime) > v11 && Float == 3.0) {
				th_s->Set(iUT_AlphaDown, 1);
			}
		} else {
			float x1a;
			float x2a;
			float y1a;
			float y2a;
			if (bBelowThreshold) {
				y2a = static_cast<float>(v21);
				y1a = static_cast<float>(v22);
				x2a = static_cast<float>(iHUDCompassIconBlinkShortestPause.Int());
				x1a = static_cast<float>(iHUDCompassIconBlinkThresholdPause.Int());
			} else {
				y2a = static_cast<float>(v22);
				y1a = static_cast<float>(v26);
				x2a = static_cast<float>(iHUDCompassIconBlinkThresholdPause.Int());
				x1a = static_cast<float>(iHUDCompassIconBlinkLongestPause.Int());
			}
			float v19 = FLerp(x1a, x2a, y1a, y2a, value);
			th_s->Set(4009, 0.0f, 1);
			th_s->Set(iUT_AlphaDown, 0);
			float v12 = static_cast<double>(appTimer.iLastTime) + v19;
			th_s->Set(Tile::kTileValue_User10, v12, 1);
		}
	} else {
		th_s->Set(4009, a3, 1);
		th_s->Set(iUT_AlphaDown, 3);
	}
}

int32_t SquareInt(int32_t a1) {
	return a1 * a1;
}

bool QuestUpdateManager::IsQuestNamesEmpty() {
	return xQuestNames.IsEmpty();
}

void QuestUpdateManager::RemoveQuestNamesHead() {
	xQuestNames.RemoveHead();
}

void QuestUpdateManager::RemoveQuestNamesAll() {
	xQuestNames.RemoveAll();
}

QuestUpdateManager::QuestUpdate* QuestUpdateManager::GetQuestNamesItem() {
	return QuestUpdateManager::xQuestNames.m_item;
}

void QuestUpdateManager::AddQuestNamesTail(QuestUpdateManager::QuestUpdate* pQuestUpdate) {
	QuestUpdateManager::xQuestNames.AddTail(pQuestUpdate);
	QuestUpdateManager::xQuestNames.Sort(&QuestUpdateManager::SortQuestNamesByPriorityFunction);
}

int32_t QuestUpdateManager::SortQuestNamesByPriorityFunction(QuestUpdateManager::QuestUpdate* pQU1,
	QuestUpdateManager::QuestUpdate* pQU2) {
	if (((int32_t)((uint32_t)pQU1->CustomQueuePriority)) > ((int32_t)((uint32_t)pQU2->CustomQueuePriority))) {
		return 1;
	}
	if (((int32_t)((uint32_t)pQU1->CustomQueuePriority)) >= ((int32_t)((uint32_t)pQU2->CustomQueuePriority))) {
		return 0;
	}
	return (-1);
}

void QuestUpdateManager::SetCustomQuestText(const char* apCustomTextTitle, const char* apCustomTextSubtitle,
	QuestUpdateManager::HUD_CUSTOM_QUEST_QUEUE_PRIORITY aiCustomQueuePriority,
	QuestUpdateManager::HUD_CUSTOM_QUEST_TEXT_JUSTIFICATION aiCustomTextJustification, FONTS aiCustomTextTitleFont,
	FONTS aiCustomTextSubtitleFont, const char* apCustomSoundHandleName) {
	QuestUpdateManager::QuestUpdate* pCustomQuestUpdate =
		static_cast<QuestUpdateManager::QuestUpdate*>(ourMalloc(sizeof(QuestUpdateManager::QuestUpdate)));
	pCustomQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_CUSTOM;
	BSstrcpy(pCustomQuestUpdate->CustomTextTitle, 260, apCustomTextTitle);
	BSstrcpy(pCustomQuestUpdate->CustomTextSubtitle, 260, apCustomTextSubtitle);
	pCustomQuestUpdate->CustomTextJustification = aiCustomTextJustification;
	pCustomQuestUpdate->CustomQueuePriority = aiCustomQueuePriority;
	if (aiCustomTextTitleFont == FONT_1) {
		Warning(WARN_QUESTS,
			"QuestUpdateManager::SetCustomQuestText() - FONT_1 is an invalid value for aiCustomTextTitleFont.  Setting "
			"to FONT_NONE.");
		aiCustomTextTitleFont = FONT_NONE;
	}
	pCustomQuestUpdate->CustomTextTitleFont = aiCustomTextTitleFont;
	if (aiCustomTextSubtitleFont == FONT_1) {
		Warning(WARN_QUESTS,
			"QuestUpdateManager::SetCustomQuestText() - FONT_1 is an invalid value for aiCustomTextSubtitleFont.  "
			"Setting to FONT_NONE.");
		aiCustomTextSubtitleFont = FONT_NONE;
	}
	pCustomQuestUpdate->CustomTextSubtitleFont = aiCustomTextSubtitleFont;
	BSstrcpy(pCustomQuestUpdate->CustomSoundHandleName, 260, apCustomSoundHandleName);
	if (aiCustomQueuePriority) {
		if (aiCustomQueuePriority <= QuestUpdateManager::HCQQP_NOW ||
			aiCustomQueuePriority > QuestUpdateManager::HCQQP_LAST) {
			Warning(WARN_QUESTS,
				"QuestUpdateManager::SetCustomQuestText() - Invalid aiCustomQueuePriority (%i)!  Setting to "
				"HCQQP_NORMAL.",
				aiCustomQueuePriority);
			QuestUpdateManager::AddQuestNamesTail(pCustomQuestUpdate);
		} else {
			QuestUpdateManager::AddQuestNamesTail(pCustomQuestUpdate);
		}
	} else {
		QuestUpdateManager::AddQuestNamesHead(pCustomQuestUpdate);
	}
}

void QuestUpdateManager::AddQuestNamesHead(QuestUpdateManager::QuestUpdate* pQuestUpdate) {
	QuestUpdateManager::xQuestNames.AddHead(pQuestUpdate);
	QuestUpdateManager::xQuestNames.Sort(&QuestUpdateManager::SortQuestNamesByPriorityFunction);
}

HUDMainMenu::HUDMainMenu()
	: Menu(), pTiles{}, pActive(nullptr), fSeenLevel(0.0f), uiMessageStartTime(0), pCurrSubtitle(nullptr),
	  pLastObject(nullptr), iHUDVisibilityUserSettings((gs_bCrossHair_GamePlay.operator bool() ? 64 : 0) | 0x1FFBF),
	  eMenuMode(HMM_START), bScopeVisible(false), pBreathMeterAnim(nullptr), bXPMeterFinishedMoving(false),
	  bForceObjectiveReminders(false), pQuestList(nullptr), pCurrentEnemy(nullptr), iEnemyHealthTimer(0),
	  uiQuestReminderTimer(appTimer.iLastTime), uiDDTdisplayTimer(0xFFFFFFFFu), uiEnemyDDTdisplayTimer(0xFFFFFFFFu),
	  bKillQuestUpdates(false), bQuestNameShowing(false), bObjectiveNameShowing(false), fShake(0.0f) {
	uiMessageDelayTime = appTimer.iLastTime;
}

// The queued messages are raw allocations the list points to; the members' own destructors free the lists.
HUDMainMenu::~HUDMainMenu() {
	TileMenu* pMenuTile = HUDMainMenu::pme->pMenu;
	if (pMenuTile) {
		delete pMenuTile;
	}
	BSShaderManager::GetShadowSceneNode(1)->DetachChild(HUDMainMenu::spScopeModel.m_pObject);
	HUDMainMenu::spScopeModel = nullptr;
	HUDMainMenu::spScopeCamera = nullptr;
	HUDMainMenu::pQuestUpdateManager = nullptr;
	while (xMessages.m_item) {
		ourFree(xMessages.m_item);
		xMessages.RemoveHead();
	}
	HUDMainMenu::pme = nullptr;
}

void HUDMainMenu::AttachTileByID(int32_t aiID, Tile* apTile) {
	if (aiID > 0 && aiID <= 86)
		this->pTiles[aiID] = apTile;
}

// Elapsed times are unsigned timer differences; the subtitle and message ones are widened to double and compared
// against the duration in milliseconds (the qword 1000.0 at 0x1017B70). The exe reads the subtitle array's size
// through the shared one-field getter (0x44DDC0) and its items through 0x877A30; src reads iSize and GetAt.
void HUDMainMenu::DoIdle() {
	if (!pPlayer || !pPlayer->GetParentCell())
		return;

	this->HudTimer.Update();
	HUDMainMenu::UpdateCompass(this->pTiles[HMMT_COMPASS], this->pTiles[HMMT_COMPASS_GENERAL_ICONS],
		this->pTiles[HMMT_COMPASS_MARKER_ICONS], this->pTiles[HMMT_COMPASS_NPC_ICONS]);
	this->UpdateHotkeysWheel();
	this->UpdateWeaponStatus();
	this->UpdateExplosiveIndicator();
	this->UpdateShake();

	// The HUD timer time of the last refresh.
	static uint32_t uiLastUpdate = this->HudTimer.GetTime();
	// The rest runs at most every 200 ms of HUD time.
	if (this->HudTimer.GetTime() - uiLastUpdate < 200)
		return;

	// The damage threshold icons go away a second after they went up.
	if (this->uiDDTdisplayTimer == 0xFFFFFFFF || appTimer.GetTime() - this->uiDDTdisplayTimer > 1000) {
		this->uiDDTdisplayTimer = 0xFFFFFFFF;
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_AP]->Set(Tile::kTileValue_Visible, 0);
	}
	if (this->uiEnemyDDTdisplayTimer == 0xFFFFFFFF || appTimer.GetTime() - this->uiEnemyDDTdisplayTimer > 1000) {
		this->uiEnemyDDTdisplayTimer = 0xFFFFFFFF;
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(Tile::kTileValue_Visible, 0);
	}
	uiLastUpdate = this->HudTimer.GetTime();

	// The screen sizes go through _ftol2 (truncation to 64 bits; the exe keeps the low 32).
	const int32_t iScreenHeight = static_cast<int32_t>(static_cast<int64_t>(InterfaceManager::GetScreenHeight()));
	const int32_t iScreenWidth = static_cast<int32_t>(static_cast<int64_t>(InterfaceManager::GetScreenWidth()));
	const int32_t iSafeX = static_cast<int32_t>(static_cast<int64_t>(InterfaceManager::GetScreenSafeX()));
	const int32_t iSafeY = static_cast<int32_t>(static_cast<int64_t>(InterfaceManager::GetScreenSafeY()));

	// The HP meter sits in the bottom left corner of the safe area, the AP meter in the bottom right one.
	HUDMainMenu::pme->pTiles[HMMT_HP]->Set(Tile::kTileValue_X, iSafeX * 2 + 10);
	const float fHPY = static_cast<double>(iScreenHeight - iSafeY * 2) -
		HUDMainMenu::pme->pTiles[HMMT_HP]->GetFloat(Tile::kTileValue_Height);
	HUDMainMenu::pme->pTiles[HMMT_HP]->Set(Tile::kTileValue_Y, fHPY, true);
	const float fAPX = static_cast<double>(iScreenWidth - iSafeX * 2) -
		HUDMainMenu::pme->pTiles[HMMT_AP]->GetFloat(Tile::kTileValue_Width) + 30.0;
	HUDMainMenu::pme->pTiles[HMMT_AP]->Set(Tile::kTileValue_X, fAPX, true);
	const float fAPY = static_cast<double>(iScreenHeight - iSafeY * 2) -
		HUDMainMenu::pme->pTiles[HMMT_AP]->GetFloat(Tile::kTileValue_Height);
	HUDMainMenu::pme->pTiles[HMMT_AP]->Set(Tile::kTileValue_Y, fAPY, true);
	HUDMainMenu::pme->pTiles[HMMT_HP]->AddNeedsUpdate(1);
	HUDMainMenu::pme->pTiles[HMMT_AP]->AddNeedsUpdate(1);

	this->UpdateActorValue(ActorValue::HEALTH);
	this->UpdateActorValue(ActorValue::ACTION_POINTS);
	this->UpdateBreathMeter();

	HUDMainMenu::pme->pTiles[HMMT_CND_ARROWS]->Set(Tile::kTileValue_Alpha, fCurrentHUDOpacity, true);
	HUDMainMenu::pme->pTiles[HMMT_HARDCORE_MODE]->Set(Tile::kTileValue_Alpha, fCurrentHUDOpacity, true);
	HUDMainMenu::pme->pTiles[HMMT_DDT_ICON]->Set(Tile::kTileValue_Alpha, fCurrentHUDOpacity, true);
	HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_AP]->Set(Tile::kTileValue_Alpha, fCurrentHUDOpacity, true);
	HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(Tile::kTileValue_Alpha, fCurrentHUDOpacity, true);
	HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(Tile::kTileValue_Alpha, fCurrentHUDOpacity, true);
	HUDMainMenu::pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(Tile::kTileValue_Alpha, fCurrentHUDOpacity, true);

	// The region and location texts fade out over 2 seconds once they have been up 4 seconds (kTileValue_User0
	// holds the global timer's time they went up).
	if (HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->GetFloat(Tile::kTileValue_Alpha) > 0.0 &&
		HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
		const double fNow = appTimer.GetTime();
		if (fNow - HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->GetFloat(Tile::kTileValue_User0) > 4000.0) {
			HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->AddFadeControl(Tile::kTileValue_Alpha,
				HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->GetFloat(Tile::kTileValue_Alpha), 0.0f, 2.0f,
				Tile::TFCT_STANDARD);
		}
	}
	if (HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_Alpha) > 0.0 &&
		HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
		const double fNow = appTimer.GetTime();
		if (fNow - HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_User0) > 4000.0) {
			const float fOpacity = fHudOpacity.Float() * 255.0;
			HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->AddFadeControl(Tile::kTileValue_Alpha, fOpacity, 0.0f, 2.0f,
				Tile::TFCT_STANDARD);
		}
	}
	// A held region text fades back in once the location text is gone, and is stamped and released.
	if (HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->IsTrue(Tile::TextToTrait("_Hold")) &&
		HUDMainMenu::pme->pTiles[HMMT_REGION_LOCATION]->GetFloat(Tile::kTileValue_Visible) != 0.0 &&
		HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_Alpha) == 0.0 &&
		HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
		const float fOpacity = fHudOpacity.Float() * 255.0;
		HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fOpacity, 2.0f,
			Tile::TFCT_STANDARD);
		HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::kTileValue_User0, appTimer.GetTime());
		HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::TextToTrait("_Hold"), 0);
	}

	if (pPlayer->IsSneaking()) {
		this->UpdateSneak();
	} else if (HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->GetFloat(Tile::kTileValue_Alpha) > 0.0 &&
		HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
		HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->AddFadeControl(Tile::kTileValue_Alpha,
			HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->GetFloat(Tile::kTileValue_Alpha), 0.0f, 0.5f,
			Tile::TFCT_STANDARD);
	}

	if (HUDMainMenu::pme->eMenuMode != HMM_SPECIAL_BOOK && HUDMainMenu::pme->eMenuMode != HMM_LOVE_TESTER)
		this->UpdateQuestText();
	this->UpdateXPMeter();
	this->UpdateRadiationMeter();
	this->UpdateStatusModeUI(pPlayer->IsHardcore());

	// The enemy counts as hostile while the compass shows it as a hostile, detected target. The exe walks the list
	// with the out-of-line IsEmpty, GetItem and GetNext (0x8256D0, 0x6815C0, 0x726070).
	bCurrentEnemyHostile = false;
	if (this->pCurrentEnemy) {
		for (BSSimpleList<CompassTarget*>* pNode = pPlayer->GetListofPerceivedActors(); pNode && !pNode->IsEmpty();
			pNode = pNode->GetNext()) {
			CompassTarget* pTarget = pNode->GetItem();
			if (pTarget->pTarget == this->pCurrentEnemy && pTarget->bIsHostile && !pTarget->bIsUndetected) {
				bCurrentEnemyHostile = true;
				break;
			}
		}
	}

	bool bShowEnemyHealth = false;
	// The exe calls GetHeading through the player's vtable (slot 175, PlayerCharacter::GetHeading 0x953F20). The
	// qword 57.2957763671875 at 0x102F248 is the float radians-to-degrees factor widened.
	const float fPlayerHeading = pPlayer->GetHeading(false) * 57.2957763671875;
	float fEnemyHeading = 0.0f;
	if (this->pCurrentEnemy) {
		const NiPoint3* pEnemyLocation = &this->pCurrentEnemy->GetLocationOnReference();
		fEnemyHeading = HUDMainMenu::HeadingBetweenLocations(pPlayer->GetLocationOnReference(), *pEnemyLocation);
	}
	// Within fEnemyHealthBarTimer seconds of the last showing, or while the player faces the enemy (within 45
	// degrees). The limit goes through fistp with truncation; the compare uses its low 32 bits, unsigned.
	const uint32_t uiSinceEnemyHealth = this->HudTimer.GetTime() - this->iEnemyHealthTimer;
	const uint32_t uiEnemyHealthLimit =
		static_cast<uint32_t>(static_cast<int64_t>(fEnemyHealthBarTimer.Float() * 1000.0));
	const bool bFacingEnemy = uiSinceEnemyHealth <= uiEnemyHealthLimit ||
		std::fabs(static_cast<float>(fPlayerHeading - fEnemyHeading)) < 45.0;

	if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
		if (pVATSTarget && pVATSTarget->IsActor() && !pVATSTarget->IsDead(false)) {
			bShowEnemyHealth = true;
			HUDMainMenu::SetEnemyActor(static_cast<Actor*>(pVATSTarget));
		}
	} else if (this->pCurrentEnemy && !this->pCurrentEnemy->IsDead(false) && !this->pCurrentEnemy->GetDestroyed() &&
		bCurrentEnemyHostile && !HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->IsTrue(Tile::kTileValue_Visible) &&
		!HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->IsTrue(Tile::kTileValue_Visible) &&
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(Tile::kTileValue_SystemColor) == 2.0 && bFacingEnemy) {
		bShowEnemyHealth = true;
		HUDMainMenu::SetInfoForRef(this->pLastObject, false, true);
	} else if (HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(Tile::kTileValue_SystemColor) == 2.0) {
		this->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 0);
		this->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 0);
		this->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 0);
	}

	if (bShowEnemyHealth) {
		bool bHostile = false;
		if (this->pCurrentEnemy) {
			if ((pPlayer->IsActorPerceived(this->pCurrentEnemy) &&
					pPlayer->IsActorPerceived(this->pCurrentEnemy)->bIsHostile) ||
				this->pCurrentEnemy->IsInCombatWithActor(pPlayer)) {
				bHostile = true;
			}
		}
		this->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 1);
		this->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 1);
		this->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 1);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(Tile::kTileValue_SystemColor, bHostile ? 2 : 1);
		// The exe flags the name tile twice.
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->AddNeedsUpdate(8);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_METER]->AddNeedsUpdate(8);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->AddNeedsUpdate(8);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->AddNeedsUpdate(8);
	}

	if (this->pCurrentEnemy && (this->pCurrentEnemy->IsInCombat() || pVATS.GetMode() == VATS::VATS_PLAYBACK))
		this->UpdateEnemyHealth(this->pCurrentEnemy);

	// The crosshair info is rebuilt when whether activating the reference is a crime changes.
	if (this->pLastObject) {
		const bool bWasCrime = bCheckSneakChanged;
		if (bWasCrime != this->pLastObject->IsCrimeToActivate()) {
			bCheckSneakChanged = this->pLastObject->IsCrimeToActivate();
			HUDMainMenu::SetInfoForRef(this->pLastObject, false, true);
		}
	}

	// The quest reminder repeats every iQuestReminderPipboyDisabledTime seconds while forced, and shows once in
	// run-and-gun mode.
	bool bReminderDue = false;
	if (this->bForceObjectiveReminders) {
		const uint32_t uiSinceReminder = appTimer.GetTime() - this->uiQuestReminderTimer;
		bReminderDue = uiSinceReminder > iQuestReminderPipboyDisabledTime.UInt() * 1000 &&
			HUDMainMenu::pme->eMenuMode != HMM_SPECIAL_BOOK && HUDMainMenu::pme->eMenuMode != HMM_LOVE_TESTER;
	}
	if (bReminderDue) {
		this->uiQuestReminderTimer = appTimer.GetTime();
		HUDMainMenu::DisplayQuestReminder(pPlayer->GetActiveQuest());
	} else if (this->uiQuestReminderTimer == 0 && HUDMainMenu::GetMenuMode() == HMM_RUN_AND_GUN) {
		this->uiQuestReminderTimer = appTimer.GetTime();
		HUDMainMenu::DisplayQuestReminder(pPlayer->GetActiveQuest());
		this->bKillQuestUpdates = false;
	}

	if (this->bKillQuestUpdates) {
		this->KillQuestName();
		this->KillQuestObjectives();
		this->bKillQuestUpdates = false;
	} else if (!this->xSubtitles.IsEmpty()) {
		// Expired subtitles are freed and dropped.
		for (uint32_t i = 0; i < this->xSubtitles.iSize; ++i) {
			const double fElapsed = this->HudTimer.GetTime() - this->xSubtitles.GetAt(i)->uiSubtitleStartTime;
			if (this->xSubtitles.GetAt(i)->fDurationSecs * 1000.0 <= fElapsed) {
				if (this->xSubtitles.GetAt(i) == this->pCurrSubtitle)
					this->HideSubtitle();
				SubtitleData* pSubtitle = this->xSubtitles.GetAt(i);
				ourFree(pSubtitle);
				this->xSubtitles.Remove(i, true);
				--i;
			}
		}
		if (!this->pCurrSubtitle) {
			this->pCurrSubtitle = this->GetNextSubtitle();
			if (this->pCurrSubtitle) {
				HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(Tile::kTileValue_String, this->pCurrSubtitle->strText,
					true);
				HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(Tile::kTileValue_Visible, 1);
			}
		}
	}

	// A queued message fades in over 0.35 seconds and is hidden once its duration has run out.
	if (this->xMessages.ItemsInList() && HUDMainMenu::pme->pTiles[HMMT_MESSAGE]->IsTrue(Tile::kTileValue_Visible)) {
		if (HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
			HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f,
				fCurrentHUDOpacity, 0.35f, Tile::TFCT_STANDARD);
			HUDMainMenu::pme->pTiles[HMMT_MESSAGE_ICON]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f,
				fCurrentHUDOpacity, 0.35f, Tile::TFCT_STANDARD);
			HUDMainMenu::pme->pTiles[HMMT_MESSAGE_TEXT]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f,
				fCurrentHUDOpacity, 0.35f, Tile::TFCT_STANDARD);
		}
		if (HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]->GetFadeFinished(Tile::kTileValue_Alpha)) {
			MessageData* pMessage = this->xMessages.GetItem();
			if (this->uiMessageStartTime == 0)
				this->uiMessageStartTime = this->HudTimer.GetTime();
			const double fElapsed = this->HudTimer.GetTime() - this->uiMessageStartTime;
			if (pMessage->fDurationSecs * 1000.0 <= fElapsed)
				this->HideMessage();
		}
	}

	fCurrentHUDOpacity = fHudOpacity.Float() * 255.0;
}

int32_t HUDMainMenu::GetClass() {
	return 1004;
}

// The exe's cleanup-only C++ EH frame is left out.
bool HUDMainMenu::ShowMessage(const char* apString, HUDMainMenu::HUD_MESSAGE_EMOTION aeEmotion,
	const char* apIconFilename, const char* apSoundName, float afShowSeconds, bool abMoveToHead) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	bool bresult = 0;
	if (apString) {
		HUDMainMenu::MessageData* newMessage_2 =
			static_cast<HUDMainMenu::MessageData*>(ourMalloc(sizeof(HUDMainMenu::MessageData)));
		HUDMainMenu::MessageData* newMessage = newMessage_2;
		BSstrcpy(newMessage_2->strText, 260, apString);
		newMessage_2->fDurationSecs = afShowSeconds;
		if (apIconFilename && *apIconFilename) {
			BSstrcpy(newMessage->IconFilename, 260, apIconFilename);
		} else {
			switch (aeEmotion) {
			case HUDMainMenu::HME_NEUTRAL: {
				BSstrcpy(newMessage->IconFilename, 260,
					"Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds");
				break;
			}
			case HUDMainMenu::HME_HAPPY: {
				BSstrcpy(newMessage->IconFilename, 260,
					"Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds");
				break;
			}
			case HUDMainMenu::HME_SAD: {
				BSstrcpy(newMessage->IconFilename, 260,
					"Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds");
				break;
			}
			case HUDMainMenu::HME_PAIN: {
				BSstrcpy(newMessage->IconFilename, 260,
					"Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds");
				break;
			}
			default: {
				break;
			}
			}
		}
		if (apSoundName) {
			BSstrcpy(newMessage->SoundName, 260, apSoundName);
		} else {
			newMessage->SoundName[0u] = 0;
		}
		if (xMessages.IsEmpty()) {
			xMessages.AddTail(newMessage);
			HUDMainMenu::MessageData* m_item = (&xMessages)->m_item;
			HUDMainMenu::pme->pTiles[26]->Set(4036, m_item->strText, 1);
			char* IconFilename = (&xMessages)->m_item->IconFilename;
			HUDMainMenu::pme->pTiles[25]->Set(4044, IconFilename, 1);
			if (apSoundName) {
				BSAudio* v7 = &BSAudio::QInstance();
				BSSoundHandle* SoundHandleByName = v7->GetSoundHandleByName(&a1, apSoundName, 289u);
				SoundHandleByName->Play(0);
			}
		} else {
			BSSimpleList<HUDMainMenu::MessageData*>* ptailMessage;
			for (ptailMessage = &xMessages; ptailMessage->m_pkNext; ptailMessage = ptailMessage->m_pkNext) {
				;
			}
			if (BSstrcmp(ptailMessage->m_item->strText, newMessage->strText)) {
				if (abMoveToHead) {
					(&xMessages)->m_item->fDurationSecs = 0.0f;
					xMessages.AddHead(newMessage);
				} else {
					xMessages.AddTail(newMessage);
				}
			} else {
				ourFree(newMessage);
			}
		}
		return 1;
	}
	return bresult;
}

bool HUDMainMenu::IsWellFormedMenu() {
	for (int32_t i = 1; i < 86; ++i) {
		if (!this->pTiles[i])
			return false;
	}
	return true;
}

NiCamera* HUDMainMenu::Create3DCamera() {
	void* pMemory = NiMemObject::operator new(sizeof(NiCamera));
	NiCamera* pCamera = pMemory ? ::new (pMemory) NiCamera : nullptr;
	// The exe builds the viewport with NiColorA's constructor (0x414430).
	NiRect<float> kPort;
	::new (&kPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
	if (BSShaderManager::bLetterbox)
		kPort = BSShaderManager::kFramebufferViewport;
	pCamera->SetViewPort(kPort);
	NiFrustum kFrustum;
	kFrustum.~NiFrustum();
	::new (&kFrustum) NiFrustum(false);
	kFrustum.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
	kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
	pCamera->SetMaxFarNearRatio(kFrustum.m_fFar / kFrustum.m_fNear);
	int32_t iHeight;
	if (BSShaderManager::bLetterbox)
		iHeight = BSShaderManager::iLetterboxHeight;
	else
		iHeight = Renderer::GetScreenHeight();
	const double dHeight = iHeight;
	const float fAspect = static_cast<float>(dHeight / static_cast<double>(Renderer::GetScreenWidth()));
	// The exe copies the float 0.75 (0x1016264) into a local; the FOV factors are the doubles pi/180 (0x1023128)
	// and the float 0.15 widened (0x1070C80).
	const float fScale = 0.75f;
	const float fHalfFOV =
		static_cast<float>(BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448);
	kFrustum.m_fLeft = -std::tan(fHalfFOV) * fScale;
	kFrustum.m_fRight = std::tan(fHalfFOV) * fScale;
	kFrustum.m_fBottom = -std::tan(fHalfFOV) * fAspect * fScale;
	kFrustum.m_fTop = std::tan(fHalfFOV) * fAspect * fScale;
	kFrustum.m_bOrtho = false;
	pCamera->SetViewFrustum(kFrustum);
	pCamera->LookAtWorldPoint(NiPoint3(0.0f, 0.0f, 1.0f), NiPoint3(0.0f, 0.0f, 1.0f));
	pCamera->SetMinNearPlaneDist(1.0f);
	NiUpdateData kUpdateData;
	kUpdateData.~NiUpdateData();
	::new (&kUpdateData) NiUpdateData(0.0f, false, false);
	pCamera->Update(kUpdateData);
	return pCamera;
}

// The subtitle tile is the HUD singleton's; the cleared subtitle is this menu's.
void HUDMainMenu::HideSubtitle() {
	if (!Interface::IsMenuIDVisible(Interface::eStartMenu, 0))
		HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(Tile::kTileValue_String, " ", true);
	HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(Tile::kTileValue_Visible, 0);
	this->pCurrSubtitle = nullptr;
}

// The handle's 12 bytes go on the stack as they are: the exe's destructor of the by-value handle (0x483710) is the
// engine's empty function.
// The exe's cleanup-only C++ EH frame is left out.
bool HUDMainMenu::ShowSubtitle(const char* apString, BSSoundHandle ahandle, NiPoint3 aSpeakerLoc,
	TESObjectREFR* apTarget, bool abForceShow) {
	Actor* pActor;
	bool bresult = 0;
	if (apTarget && apTarget->IsActor()) {
		pActor = dynamic_cast<Actor*>(apTarget);
	} else {
		pActor = 0;
	}
	if (apString) {
		if (gs_bGeneralSubtitles_Gameplay.Bool() || (pActor && static_cast<uint8_t>(pActor->GetShowSubtitles()))) {
			HUDMainMenu::SubtitleData* newSubtitle;
			HUDMainMenu::SubtitleData* apMemory =
				static_cast<HUDMainMenu::SubtitleData*>(ourMalloc(sizeof(HUDMainMenu::SubtitleData)));
			if (apMemory) {
				newSubtitle = ::new (apMemory) HUDMainMenu::SubtitleData;
			} else {
				newSubtitle = 0;
			}
			HUDMainMenu::SubtitleData* newSubtitle_2 = newSubtitle;
			BSstrcpy(newSubtitle->strText, 260, apString);
			newSubtitle->SpeakerLoc = aSpeakerLoc;
			newSubtitle_2->fDurationSecs = static_cast<double>(ahandle.GetDuration()) / 1000.0;
			if (newSubtitle_2->fDurationSecs == 0.0) {
				double v7 = static_cast<double>(std::strlen(apString));
				newSubtitle_2->fDurationSecs = fNoticeTextTimePerCharacter.Float() * v7;
			}
			if (abForceShow) {
				newSubtitle_2->iPriority = 0x7FFFFFFF;
			} else {
				if (apTarget == pPlayer) {
					newSubtitle_2->iPriority = 0;
				} else {
					if (apTarget) {
						newSubtitle_2->iPriority = 2;
					} else {
						newSubtitle_2->iPriority = 1;
					}
				}
			}
			newSubtitle_2->uiSubtitleStartTime = HudTimer.iLastTime;
			xSubtitles.Add(newSubtitle_2);
			bresult = 1;
		}
	}
	return bresult;
}

void HUDMainMenu::ClearAllSubtitlesAndMessages() {
	for (BSSimpleList<MessageData*>* pNode = &this->xMessages; pNode && !pNode->IsEmpty(); pNode = pNode->GetNext())
		ourFree(pNode->GetItem());
	this->xMessages.RemoveAll();
	HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]->Set(Tile::kTileValue_Alpha, 0.0f, true);
	HUDMainMenu::pme->pTiles[HMMT_MESSAGE_TEXT]->Set(Tile::kTileValue_String, " ", true);
	HUDMainMenu::pme->pTiles[HMMT_MESSAGE_TEXT]->Set(Tile::kTileValue_Alpha, 0.0f, true);
	HUDMainMenu::pme->pTiles[HMMT_MESSAGE_ICON]->Set(Tile::kTileValue_Alpha, 0.0f, true);
	this->HideSubtitle();
}

// The elapsed times are the unsigned timer difference widened to double, compared against the setting in
// milliseconds (the qword 1000.0 at 0x1017B70). The exe calls HotKeysWheel's out-of-line one-field setter
// (0x61CC40) for iLastHighlightedHotkey and ItemChange's count getter (0x726070); src stores and reads iNumber.
void HUDMainMenu::UpdateHotkeysWheel() {
	if (InterfaceManager::Instance()->IsInMenuMode() || pPlayer->IsDead(false) ||
		pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY)) {
		iHotkeyStartTime = 0;
		this->HKWheel.SetVisible(false);
		this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 0);
	} else if (InterfaceManager::Has360Controller()) {
		const int32_t iHotkey = this->HKWheel.MoveHighlight(-1);
		if (!bwheelBeingUsed && iHotkey >= 0) {
			this->HKWheel.UpdateHotkeyList();
			bwheelBeingUsed = true;
			iHotkeyStartTime = static_cast<int32_t>(appTimer.GetTime());
		}
		if (bwheelBeingUsed) {
			if (iHotkey >= 0) {
				if (iHotkeyStartTime > 0) {
					const double dElapsed =
						static_cast<double>(appTimer.GetTime() - static_cast<uint32_t>(iHotkeyStartTime));
					if (fHotKeyDownTimer.Float() * 1000.0 < dElapsed && !this->HKWheel.GetVisible()) {
						this->HKWheel.SetVisible(true);
						this->pTiles[HMMT_XP_BRACKET]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_TEXT]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_LABEL]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_POINTER]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_LASTLEVEL]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_NEXTLEVEL]->Set(Tile::kTileValue_Visible, 0);
					}
				}
				if (this->HKWheel.GetVisible() && iHotkey != ilastHighlight) {
					InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
					ItemChange* pItem;
					if (pChanges)
						pItem = pChanges->GetHotKeyItem(iHotkey);
					else
						pItem = nullptr;
					bool bHasIcon = false;
					const char* pIcon = nullptr;
					if (pItem) {
						char acName[260];
						pIcon = pItem->GetIconOne(nullptr);
						if (pIcon)
							bHasIcon = true;
						if (pItem->GetCount() > 1)
							BSsprintf(acName, 260, "%s (%d)", pItem->GetFullName(), pItem->GetCount());
						else
							BSstrcpy(acName, 260, pItem->GetFullName());
						this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_String, acName, true);
						this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 1);
					} else {
						this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 0);
					}
					if (bHasIcon) {
						this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconFilename"), pIcon, true);
						this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 1);
					} else {
						this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 0);
					}
					ilastHighlight = iHotkey;
					if (pItem)
						delete pItem;
				}
			} else {
				this->HKWheel.UseHotkeyItem();
				this->HKWheel.SetVisible(false);
				this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 0);
				iHotkeyStartTime = 0;
				bwheelBeingUsed = false;
				ilastHighlight = -1;
			}
		}
	} else {
		for (int32_t i = 0; i < this->HKWheel.QNumHotkeys(); ++i) {
			const Controls::USERACTIONS eAction = static_cast<Controls::USERACTIONS>(i + Controls::UACTN_HOTKEY1);
			if (appMain->GetControls()->GetUserAction(eAction, Controls::isPressed)) {
				if (i == 1) {
					this->HKWheel.iLastHighlightedHotkey = i;
					continue;
				}
				InventoryChanges* pChanges = InventoryChanges::GetInventoryChanges(pPlayer);
				bool bHasIcon = false;
				iHotkeyStartTime = static_cast<int32_t>(appTimer.GetTime());
				this->HKWheel.MoveHighlight(i);
				ItemChange* pItem;
				if (pChanges)
					pItem = pChanges->GetHotKeyItem(i);
				else
					pItem = nullptr;
				const char* pIcon = nullptr;
				if (pItem) {
					char acName[260];
					pIcon = pItem->GetIconOne(nullptr);
					if (pIcon)
						bHasIcon = true;
					if (pItem->GetCount() > 1)
						BSsprintf(acName, 260, "%s (%d)", pItem->GetFullName(), pItem->GetCount());
					else
						BSstrcpy(acName, 260, pItem->GetFullName());
					this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_String, acName, true);
				} else {
					this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_String, " ", true);
				}
				if (bHasIcon) {
					this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconFilename"), pIcon, true);
					this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 1);
				} else {
					this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 0);
				}
				if (pItem)
					delete pItem;
			} else if (appMain->GetControls()->GetUserAction(eAction, Controls::isHeld)) {
				if (!this->HKWheel.GetVisible() && iHotkeyStartTime > 0) {
					const double dElapsed =
						static_cast<double>(appTimer.GetTime() - static_cast<uint32_t>(iHotkeyStartTime));
					if (fHotKeyDownTimer.Float() * 1000.0 < dElapsed && i != 1) {
						this->HKWheel.SetVisible(true);
						this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 1);
						this->pTiles[HMMT_XP_BRACKET]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_TEXT]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_LABEL]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_POINTER]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_LASTLEVEL]->Set(Tile::kTileValue_Visible, 0);
						this->pTiles[HMMT_XP_NEXTLEVEL]->Set(Tile::kTileValue_Visible, 0);
					}
				}
				if (this->HKWheel.GetVisible())
					iHotkeyStartTime = static_cast<int32_t>(appTimer.GetTime());
			} else if (appMain->GetControls()->GetUserAction(eAction, Controls::isDepressed)) {
				this->HKWheel.UseHotkeyItem();
				if (!this->HKWheel.GetVisible())
					iHotkeyStartTime = 0;
			}
		}
		if (this->HKWheel.GetVisible() && iHotkeyStartTime > 0) {
			const double dElapsed = static_cast<double>(appTimer.GetTime() - static_cast<uint32_t>(iHotkeyStartTime));
			if (fHotKeyShowTimer.Float() * 1000.0 < dElapsed) {
				this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 0);
				this->HKWheel.SetVisible(false);
				iHotkeyStartTime = 0;
			}
		}
	}
}

// The exe reads the weapon and ammo objects through the shared one-field getter (0x44DDC0, registered as
// TESForm::GetFormFlags), which ItemChange::GetContainerObject folds with, and the ammo count through 0x726070;
// src reads iNumber. The no-weapon branch hides the ammo text and the condition text of the HUD singleton (pme),
// the meter tiles of this menu. Counts subtract from the loaded rounds in double, as the exe's fild/fsub does.
void HUDMainMenu::UpdateWeaponStatus() {
	if (!pPlayer || !pPlayer->GetCurrentAIProcess())
		return;
	const float fLocalHudOpacity = fHudOpacity.Float() * 255.0;
	ItemChange* pweap = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
	static float fWeapCondBarAlpha = fLocalHudOpacity;
	BSsprintf(ammosubtype, sizeof(ammosubtype), "%s", "");
	this->pTiles[HMMT_AMMO_SUB_TYPE]->Set(Tile::kTileValue_String, ammosubtype, true);

	if (!pweap) {
		flastWeaponAmmo = 1.17549435082228750797e-38f;
		flastTotalWeaponAmmo = 1.17549435082228750797e-38f;
		plastWeaponObj = nullptr;
		flastWeapHealthPercent = 1.17549435082228750797e-38f;
		HUDMainMenu::pme->pTiles[HMMT_AMMO_TEXT]->Set(Tile::kTileValue_Visible, 0);
		this->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(Tile::kTileValue_Visible, 0);
		this->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_WEAPON_CONDITION_TEXT]->Set(Tile::kTileValue_Visible, 0);
		this->pTiles[HMMT_CND_ARROWS]->Set(Tile::kTileValue_Visible, 0);
		return;
	}

	TESObjectWEAP* pweaponObj = static_cast<TESObjectWEAP*>(pweap->GetContainerObject());
	float fweaponAmmo = 0.0f;
	float ftotalAmmo = 0.0f;
	const int32_t iWeaponType = pweaponObj->GetWeaponType();
	if (iWeaponType >= WEAPON_ONE_HAND_GRENADE && iWeaponType <= WEAPON_ONE_HAND_THROWN) {
		// Thrown weapons and mines: the rounds of one throw, and the rest of the stack.
		fweaponAmmo = pweaponObj->GetAmmoPerShot();
		ftotalAmmo = static_cast<double>(InventoryChanges::GetInventoryChanges(pPlayer)->GetObjectCount(pweaponObj)) -
			fweaponAmmo;
	} else {
		ItemChange* pammo = pPlayer->GetCurrentAIProcess()->GetCurrentAmmo();
		TESAmmo* pAmmoForm = nullptr;
		if (pammo && pammo->GetContainerObject()) {
			pAmmoForm = dynamic_cast<TESAmmo*>(pammo->GetContainerObject());
		}
		if (pAmmoForm) {
			// The ammo type's abbreviation; the condition tiles move over while there is one.
			const char* abbrev = pAmmoForm->GetAbbrev();
			BSsprintf(ammosubtype2, sizeof(ammosubtype2), "%s", abbrev ? abbrev : "");
			this->pTiles[HMMT_AMMO_SUB_TYPE]->Set(Tile::kTileValue_String, ammosubtype2, true);
			if (abbrev) {
				if (!bAmmoShifted) {
					this->ShiftConditionTileOver(-50);
					bAmmoShifted = true;
				}
			} else if (bAmmoShifted) {
				this->ShiftConditionTileOver(50);
				bAmmoShifted = false;
			}
		}
		// A regenerating weapon shows its clip size as the total.
		bool bGetAmmoTotal = true;
		if (pweaponObj->GetAmmoRegenRate(pweap->HasModEffectActive(6)) > 0.0) {
			ftotalAmmo = pweaponObj->GetFormClipRounds(pweap->HasModEffectActive(2));
			bGetAmmoTotal = false;
		}
		if (pweaponObj->GetCurrentAmmo(pPlayer) && pammo) {
			fweaponAmmo = pammo->GetCount();
			if (bGetAmmoTotal) {
				TESBoundObject* pCurrentAmmo = static_cast<TESBoundObject*>(pweaponObj->GetCurrentAmmo(pPlayer));
				ftotalAmmo =
					static_cast<double>(InventoryChanges::GetInventoryChanges(pPlayer)->GetObjectCount(pCurrentAmmo)) -
					fweaponAmmo;
			}
		} else if (pweaponObj->BGSAmmoForm::IsRockItLauncher()) {
			fweaponAmmo = pPlayer->GetNumRockItAmmo();
			ftotalAmmo = fweaponAmmo;
		}
	}

	if (pweaponObj->IsMeleeWeapon()) {
		this->pTiles[HMMT_AMMO_TEXT]->Set(Tile::kTileValue_Visible, 0);
	} else if (fweaponAmmo != flastWeaponAmmo || ftotalAmmo != flastTotalWeaponAmmo || plastWeaponObj != pweaponObj) {
		BSsprintf(ammo, sizeof(ammo), "%i/%i", static_cast<int32_t>(fweaponAmmo), static_cast<int32_t>(ftotalAmmo));
		this->pTiles[HMMT_AMMO_TEXT]->Set(Tile::kTileValue_String, ammo, true);
		this->pTiles[HMMT_AMMO_TEXT]->Set(Tile::kTileValue_Visible, 1);
		flastWeaponAmmo = fweaponAmmo;
		flastTotalWeaponAmmo = ftotalAmmo;
	}

	if (plastWeaponObj != pweaponObj) {
		this->pTiles[HMMT_WEAPON_CONDITION_TEXT]->Set(Tile::kTileValue_Visible, 1);
		this->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(Tile::kTileValue_Visible, 1);
		this->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND]->Set(Tile::kTileValue_Visible, 1);
		this->pTiles[HMMT_CND_ARROWS]->Set(Tile::kTileValue_Visible, 1);
		plastWeaponObj = pweaponObj;
		if (bAmmoShifted)
			this->ShiftConditionTileOver(50);
		bAmmoShifted = false;
	}

	// The exe reads the absolute health and drops it.
	pweap->GetItemHealth(false);
	const float fweapHealthPercent = pweap->GetItemHealth(true);
	if (fweapHealthPercent < 25.0) {
		// Below 25% the bar pulses: the alpha runs between 0 and the HUD opacity, turning at either end.
		BSClamp(fWeapCondBarAlpha, 0.0f, fLocalHudOpacity);
		if (fWeapCondBarAlpha == 0.0 || fWeapCondBarAlpha == fLocalHudOpacity)
			bWeapFadeDecrease = !bWeapFadeDecrease;
		if (bWeapFadeDecrease)
			fWeapCondBarAlpha = fWeapCondBarAlpha - fLocalHudOpacity / 255.0 * 10.0;
		else
			fWeapCondBarAlpha = fLocalHudOpacity / 255.0 * 10.0 + fWeapCondBarAlpha;
	} else {
		fWeapCondBarAlpha = fLocalHudOpacity;
	}
	this->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(Tile::kTileValue_Alpha, fWeapCondBarAlpha, true);

	if (fweapHealthPercent != flastWeapHealthPercent) {
		const float fCurrentHealth = pweap->GetItemHealth(false);
		float fMaxHealth = static_cast<float>(pweaponObj->GetFormHealth(pweap->HasModEffectActive(10)));
		const int32_t iNumPixelShift = bAmmoShifted ? -50 : 0;
		const float fRatio = fCurrentHealth / fMaxHealth;
		HUDMainMenu::UpdateMeter(this->pTiles[HMMT_WEAPON_CONDITION_METER], BSMIN(1.0f, BSMAX(0.0f, fRatio)), HJ_LEFT,
			0, iNumPixelShift);
		flastWeapHealthPercent = fweapHealthPercent;
	}
}

void HUDMainMenu::UpdateExplosiveIndicator() {
	NiTListItem<Tile*>* pPos = pTiles[HMMT_EXPLOSIVE_INDICATOR]->xChildren.m_pkHead;
	for (uint32_t i = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelHead(PROCESS_TYPE_HIGH);; ++i) {
		uint32_t ProcessLevelTail = (&ReferenceProcessLists.MobProcessArray)->GetProcessLevelTail(PROCESS_TYPE_HIGH);
		if (i >= ProcessLevelTail) {
			break;
		}
		MobileObject* pMob = (&ReferenceProcessLists.MobProcessArray)->GetItem(i);
		if (!pMob || pMob->IsStillLoading() || pMob->GetDelete() || pMob->GetDisabled() ||
			pMob->GetFormType() != PGRE_ID || !pMob->IsProjectile()) {
			continue;
		}
		Projectile* pProjectile = static_cast<Projectile*>(pMob);
		if (!pProjectile || pProjectile->IsEditorCreated()) {
			continue;
		}
		if (!pProjectile->GetProjectileBase()->GetExplosion()) {
			continue;
		}
		if (pProjectile->GetProjectileBase()->GetExplosionProximity() != 0.0) {
			continue;
		}
		const NiPoint3* z = &pPlayer->GetLocationOnReference();
		const NiPoint3* v4 = &pMob->GetLocationOnReference();
		double v17 = (*v4 - *z).Length();
		if (fExplosiveIndicatorDistance.Float() > v17) {
			if (pPos) {
				Tile* pnew = pTiles[HMMT_EXPLOSIVE_INDICATOR]->xChildren.GetNext(pPos);
				if (pnew->GetFadeFinished(Tile::kTileValue_Alpha) && pnew->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
					float afEnd = fHudOpacity.Float() * 255.0;
					pnew->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, afEnd, 0.5f, Tile::TFCT_STANDARD);
				}
				const NiPoint3* za = &pPlayer->GetLocationOnReference();
				const NiPoint3* v6 = &pMob->GetLocationOnReference();
				float fangle = GetZAngleFromVector(*v6 - *za);
				SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
				float fangle2 = GetZAngleFromVector(WorldSceneGraph->GetCamera()->GetWorldDirection());
				float v15 = fangle2 - fangle;
				pnew->GetModel()->SetRotate(v15, 0.0f, 1.0f, 0.0f);
				FNVD_CONSTRUCTED_LOCAL(NiUpdateData, update, nullptr, 0.0f, 1, 0);
				pnew->GetModel()->Update(update);
			}
		}
	}
	while (pPos) {
		Tile* pnew = pTiles[HMMT_EXPLOSIVE_INDICATOR]->xChildren.GetNext(pPos);
		if (pnew->GetFadeFinished(Tile::kTileValue_Alpha) && pnew->GetFloat(Tile::kTileValue_Alpha) > 0.0) {
			float afStart = pnew->GetFloat(Tile::kTileValue_Alpha);
			pnew->AddFadeControl(Tile::kTileValue_Alpha, afStart, 0.0f, 0.5f, Tile::TFCT_STANDARD);
		}
	}
}

// The start position is the tile's on the first call. A frame with fShake moves the tile by fShake in a random
// direction; the frame after the last shake (fShake 0) puts it back.
void HUDMainMenu::UpdateShake() {
	// The HUD tile's x and y before any shake (the PDB's names).
	static float forigX = this->pMenu->GetFloat(Tile::kTileValue_X);
	static float forigY = this->pMenu->GetFloat(Tile::kTileValue_Y);
	float fx = 0.0f;
	float fy = 0.0f;
	if (this->fShake > 0.0) {
		bupdate = true;
		NiSinCos(RandomFloatTwoPi(), fx, fy);
		fx = fx * this->fShake;
		fy = fy * this->fShake;
	}
	if (bupdate) {
		this->pMenu->Set(Tile::kTileValue_X, forigX + fx, true);
		this->pMenu->Set(Tile::kTileValue_Y, forigY + fy, true);
	}
	if (this->fShake == 0.0)
		bupdate = false;
	this->fShake = 0.0f;
}

void HUDMainMenu::ShiftConditionTileOver(int32_t aishift) {
	float v5 = (float)(HUDMainMenu::pme->pTiles[10]->GetFloat(4001) + ((double)aishift));
	pTiles[10]->Set(4001, v5, (bool)1);
	float v4 = (float)(HUDMainMenu::pme->pTiles[11]->GetFloat(4001) + ((double)aishift));
	pTiles[11]->Set(4001, v4, (bool)1);
	float v3 = (float)(HUDMainMenu::pme->pTiles[9]->GetFloat(4001) + ((double)aishift));
	pTiles[9]->Set(4001, v3, (bool)1);
	float v2 = (float)(HUDMainMenu::pme->pTiles[86]->GetFloat(4001) + ((double)aishift));
	pTiles[86]->Set(4001, v2, (bool)1);
}

void HUDMainMenu::UpdateActorValue(ActorValue::Index aeActorValue) {
	if (aeActorValue == ActorValue::ACTION_POINTS) {
		float a2a = pPlayer->GetPermanentActorFloatValue(ActorValue::ACTION_POINTS);
		float fbase_2 = BSMAX(0.0, a2a);
		if (fbase_2 != 0.0) {
			float a2b = pPlayer->GetActorFloatValue(ActorValue::ACTION_POINTS);
			fbase_2 = BSMAX(0.0f, a2b) / fbase_2;
		}
		float afMeterPercent = BSMIN(fbase_2, 1.0f);
		HUDMainMenu::UpdateMeter(pTiles[7], afMeterPercent, HUDMainMenu::HJ_RIGHT, 0, 0);
	} else {
		if (aeActorValue == ActorValue::HEALTH) {
			float fbase = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
			if (fbase != 0.0) {
				float a2 = pPlayer->GetActorFloatValue(ActorValue::HEALTH);
				fbase = BSMAX(0.0f, a2) / fbase;
			}
			if (fbase > 1.0) {
				fbase = 1.0f;
			}
			if (pPlayer->IsDead(0)) {
				HUDMainMenu::UpdateMeter(HUDMainMenu::pme->pTiles[1], fbase, HUDMainMenu::HJ_LEFT, 0, 0);
			} else {
				HUDMainMenu::UpdateMeter(HUDMainMenu::pme->pTiles[1], fbase, HUDMainMenu::HJ_LEFT, 1, 0);
			}
		}
	}
}

// Fades the breath meter in once the player is deep under water and out again once he surfaces, and sets the meter's
// animation to the breath left. The exe computes the "under water at all" flag (a depth of at least 0.01) into a
// local it never reads. The breath fraction is divided and kept at double precision and rounded to float once.
void HUDMainMenu::UpdateBreathMeter() {
	TESObjectCELL* pCell = pPlayer->GetParentCell();
	float fDepth = pPlayer->IsPointDeepUnderWater(pPlayer->GetLocationOnReference().z, pCell);
	bool bDeepUnderWater = fDepth >= 0.875;
	float fBreathTime = GamePlayFormulas::SwimBreathTime(static_cast<int32_t>(pPlayer->GetEndurance()));
	double dBreathLeft;
	if (pPlayer->GetCurrentAIProcess()) {
		BaseProcess* pProcess = pPlayer->GetCurrentAIProcess();
		dBreathLeft = static_cast<double>(pProcess->GetBreathTimer()) / static_cast<double>(fBreathTime);
	} else {
		dBreathLeft = 100.0;
	}
	float fBreathLeft = static_cast<float>(dBreathLeft);
	float fAlpha = pTiles[HMMT_BREATH]->GetFloat(Tile::kTileValue_Alpha);
	if (bDeepUnderWater) {
		if (fAlpha == 0.0) {
			pTiles[HMMT_BREATH]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity, 0.5f,
				Tile::TFCT_STANDARD);
		}
	} else if (fAlpha > 0.0 && pTiles[HMMT_BREATH]->GetFadeFinished(Tile::kTileValue_Alpha)) {
		pTiles[HMMT_BREATH]->AddFadeControl(Tile::kTileValue_Alpha, fCurrentHUDOpacity, 0.0f, 0.5f,
			Tile::TFCT_STANDARD);
	}
	if (pBreathMeterAnim && fAlpha > 0.0) {
		NiUpdateData kUpdateData;
		kUpdateData.~NiUpdateData();
		::new (&kUpdateData) NiUpdateData(0.0f, true, false);
		float fEnd = pBreathMeterAnim->GetEndKeyTime();
		float fBegin = pBreathMeterAnim->GetBeginKeyTime();
		float fEndAgain = pBreathMeterAnim->GetEndKeyTime();
		float fPhase = static_cast<float>((static_cast<double>(fBegin) - static_cast<double>(fEndAgain)) *
				static_cast<double>(fBreathLeft) +
			static_cast<double>(fEnd));
		pBreathMeterAnim->SetPhase(fPhase, false);
		pTiles[HMMT_BREATH]->GetModel()->GetAt(0)->Update(kUpdateData);
	}
}

void HUDMainMenu::UpdateSneak() {
	bool bcombatSearch;
	bcombatSearch = 0;
	bool bblink = 0;
	if (pPlayer->IsPlayerCharacterInCombat(&bcombatSearch) || bcombatSearch) {
		const char* v1;
		HUDMainMenu::pme->pTiles[24]->Set(4084, 2);
		if (bcombatSearch) {
			v1 = sSneakCaution.String();
		} else {
			bblink = 1;
			v1 = sSneakDanger.String();
		}
		HUDMainMenu::pme->pTiles[24]->Set(4036, v1, 1);
	} else {
		if (pPlayer->GetHostileDetection()) {
			HUDMainMenu::pme->pTiles[24]->Set(4084, 2);
			const char* v2 = sSneakCaution.String();
			HUDMainMenu::pme->pTiles[24]->Set(4036, v2, 1);
		} else {
			int32_t ilevel = ReferenceProcessLists.GetHighestDetectionLevelForActor(pPlayer, 0);
			HUDMainMenu::pme->pTiles[24]->Set(4084, 1);
			bblink = 0;
			if (ilevel <= 0) {
				const char* v4 = sSneakHidden.String();
				HUDMainMenu::pme->pTiles[24]->Set(4036, v4, 1);
			} else {
				if (!pPlayer->GetHostileDetection()) {
					const char* v3 = sSneakDetected.String();
					HUDMainMenu::pme->pTiles[24]->Set(4036, v3, 1);
				}
			}
		}
	}
	if (!bblink || HUDMainMenu::pme->pTiles[24]->IsTrue(Tile::kTileValue_User0)) {
		double Float;
		if ((!bblink && HUDMainMenu::pme->pTiles[24]->GetFadeType(4009) == Tile::TFCT_BLINK_THRICE) ||
			(HUDMainMenu::pme->pTiles[24]->GetFadeFinished(4009) &&
				(Float = HUDMainMenu::pme->pTiles[24]->GetFloat(4009), Float != fHudOpacity.Float() * 255.0))) {
			float v7 = fHudOpacity.Float() * 255.0;
			float afStart = HUDMainMenu::pme->pTiles[24]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[24]->AddFadeControl(4009, afStart, v7, 0.5f, Tile::TFCT_STANDARD);
		}
	} else {
		float afEnd = fHudOpacity.Float() * 255.0;
		HUDMainMenu::pme->pTiles[24]->AddFadeControl(4009, 0.0f, afEnd, 0.5f, Tile::TFCT_BLINK_THRICE);
	}
	HUDMainMenu::pme->pTiles[24]->Set(Tile::kTileValue_User0, static_cast<int32_t>(bblink));
}

// The exe's cleanup-only C++ EH frame is left out.
void HUDMainMenu::UpdateQuestText() {
	FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, th_s, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	NiPoint3 textDimensions_2;
	NiPoint3 pos_4;
	FNVD_INLINED_LOCAL(BSSoundHandle, psoundHand, std::nullopt, std::nullopt);
	NiPoint3 subtitleDimensions;
	if (HUDMainMenu::pme &&
		(eMenuMode != HUDMainMenu::HMM_VATS &&
			(eMenuMode != HUDMainMenu::HMM_CONTAINER &&
				(eMenuMode != HUDMainMenu::HMM_VATS_PLAYBACK && appTimer.iLastTime - uiMessageDelayTime >= 2000)))) {
		bool bPipboyUp = InterfaceManager::IsInPipboyMenu();
		if (!IsQuestNameShowing() && !HUDMainMenu::pQuestUpdateManager->IsQuestNamesEmpty() &&
			(!AreObjectivesShowing() || HUDMainMenu::pme->pTiles[13]->xChildren.GetHead()->GetFloat(4009) == 0.0)) {
			InterfaceManager* v1 = InterfaceManager::Instance();
			if ((!v1->IsInMenuMode() ||
					(Interface::IsMenuIDVisible(Interface::eDialogMenu, 0) || Interface::IsConsoleVisible())) &&
				!pVATS.eMode) {
				char Dst[268];
				bQuestNameShowing = 1;
				::new (&psoundHand) BSSoundHandle;
				if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType) {
					if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
						QuestUpdateManager::HQUT_CUSTOM) {
						char* a1 = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomSoundHandleName;
						BSAudio* v3 = &BSAudio::QInstance();
						BSSoundHandle* SoundHandleByName = v3->GetSoundHandleByName(&th_s, a1, 289u);
						psoundHand.operator=(*SoundHandleByName);
					} else {
						BSAudio* v4 = &BSAudio::QInstance();
						BSSoundHandle* v34 = v4->GetSoundHandleByName(&apResult, "UIPopUpQuestComplete", 289u);
						psoundHand.operator=(*v34);
					}
				} else {
					BSAudio* v2 = &BSAudio::QInstance();
					BSSoundHandle* v36 = v2->GetSoundHandleByName(&arHandle, "UIPopUpQuestNew", 289u);
					psoundHand.operator=(*v36);
				}
				psoundHand.Play(0);
				int32_t iWidth = 0;
				uint32_t uifadeInStart = appTimer.iLastTime;
				Tile* pquest = HUDMainMenu::pme->pTiles[14]->xChildren.GetHead();
				if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType) {
					if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
						QuestUpdateManager::HQUT_FAILED) {
						const char* v6 = sQuestFailed.String();
						pquest->Set(4036, v6, 1);
					} else {
						if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
							QuestUpdateManager::HQUT_COMPLETE) {
							const char* v7 = gs_sQuestCompletedText.String();
							pquest->Set(4036, v7, 1);
						} else {
							if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
								QuestUpdateManager::HQUT_CUSTOM) {
								QuestUpdateManager::QuestUpdate* QuestNamesItem =
									HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
								pquest->Set(4036, QuestNamesItem->CustomTextTitle, 1);
							} else {
								Warning(WARN_QUESTS,
									"HUDMainMenu::UpdateQuestText is trying to handle an invalid eUpdateType (%i)",
									HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType);
							}
						}
					}
				} else {
					const char* v5 = gs_sQuestAddedText.String();
					pquest->Set(4036, v5, 1);
				}
				pquest->Set(4003, 1);
				pquest->Set(4009, 0);
				int32_t nFont = 7;
				if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
						QuestUpdateManager::HQUT_CUSTOM &&
					HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextTitleFont != FONT_NONE) {
					nFont = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextTitleFont;
				}
				pquest->Set(4025, nFont);
				float a1a = pquest->GetFloat(4026);
				int32_t Float = static_cast<int32_t>(pquest->GetFloat(4025));
				const char* String = pquest->GetString(4036);
				FontManager* Singleton = FontManager::Instance();
				Singleton->CalculateStringDimensions(&pos_4, String, Float, a1a, 0);
				iWidth = static_cast<int32_t>(pos_4.x);
				float v33 = -(pos_4.y + 15.0);
				pquest->Set(4002, v33, 1);
				int32_t nJustification = 1;
				float fScreenWidthMultiplier = 0.0f;
				int32_t nScreenWidth = static_cast<int32_t>(InterfaceManager::GetScreenWidth());
				int32_t nScreenSafeX = static_cast<int32_t>(InterfaceManager::GetScreenSafeX());
				int32_t nSafeScreenWidth = nScreenWidth - 2 * nScreenSafeX;
				if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
					QuestUpdateManager::HQUT_CUSTOM) {
					QuestUpdateManager::HUD_CUSTOM_QUEST_TEXT_JUSTIFICATION CustomTextJustification =
						HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextJustification;
					if (CustomTextJustification) {
						if (CustomTextJustification == QuestUpdateManager::HCQTJ_CENTER) {
							nJustification = 2;
							fScreenWidthMultiplier = 0.5f;
						} else {
							if (CustomTextJustification == QuestUpdateManager::HCQTJ_RIGHT) {
								nJustification = 4;
								fScreenWidthMultiplier = 1.0f;
							} else {
								Warning(WARN_QUESTS,
									"HUDMainMenu::UpdateQuestText is trying to handle an invalid "
									"CustomTextJustification (%i).  Defaulting to %i.",
									HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextJustification, 0);
								nJustification = 1;
								fScreenWidthMultiplier = 0.0f;
							}
						}
					} else {
						nJustification = 1;
						fScreenWidthMultiplier = 0.0f;
					}
					float v31 = static_cast<double>(nSafeScreenWidth - iWidth / 2) * fScreenWidthMultiplier;
					pquest->Set(4001, v31, 1);
					pquest->Set(4023, nJustification);
				}
				pquest->Set(Tile::kTileValue_User0, uifadeInStart);
				double v30 = static_cast<double>(uifadeInStart);
				float v29 = fQuestCinematicCharacterRemain.Float() * 1000.0 + v30;
				pquest->Set(Tile::kTileValue_User1, v29, 1);
				HUDMainMenu::SetBrightness(pquest, HUDMainMenu::HB_HUD_TEXT);
				if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
					QuestUpdateManager::HQUT_COMPLETE) {
					KillQuestObjectives();
				}
				if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
					QuestUpdateManager::HQUT_CUSTOM) {
					QuestUpdateManager::QuestUpdate* v10 = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
					BSstrcpy(Dst, 260, v10->CustomTextSubtitle);
				} else {
					QuestUpdateManager::QuestUpdate* v11 = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
					BSstrcpy(Dst, 260, v11->pQuest->GetFullName());
				}
				for (uint32_t a = 0; a < 260; ++a) {
					Dst[a] = std::toupper(Dst[a]);
				}
				int32_t iSpace = 32;
				int32_t iQuestNameLength = BSstrlen(Dst);
				if (iQuestNameLength >= 32) {
					for (int32_t i = 32; i >= 0; --i) {
						if (Dst[i] == 32) {
							iSpace = i;
							break;
						}
					}
				}
				nFont = 8;
				if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
						QuestUpdateManager::HQUT_CUSTOM &&
					HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextSubtitleFont != FONT_NONE) {
					nFont = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextSubtitleFont;
				}
				FontManager* v12 = FontManager::Instance();
				v12->CalculateStringDimensions(&subtitleDimensions, Dst, nFont, 3.4028235e+38f, 0);
				int32_t iX = static_cast<int32_t>((static_cast<double>(nSafeScreenWidth) -
													  (fScreenWidthMultiplier + 1.0) * subtitleDimensions.x) *
					fScreenWidthMultiplier);
				NiTListItem<Tile*>* textDimensions = HUDMainMenu::pme->pTiles[14]->xChildren.m_pkHead;
				if (textDimensions) {
					// The exe stores the first child into a local nothing reads ([ebp-0x188]); only the call stays.
					HUDMainMenu::pme->pTiles[14]->xChildren.GetNext(textDimensions);
				}
				for (int32_t i_2 = 0; Dst[i_2]; ++i_2) {
					Tile* pquest_2 = 0;
					if (textDimensions) {
						char Src[4];
						pquest_2 = HUDMainMenu::pme->pTiles[14]->xChildren.GetNext(textDimensions);
						Src[0] = Dst[i_2];
						Src[1] = 0;
						if (iSpace + 1 == i_2) {
							iX = static_cast<int32_t>((static_cast<double>(nSafeScreenWidth) -
														  (fScreenWidthMultiplier + 1.0) * subtitleDimensions.x) *
								fScreenWidthMultiplier);
						}
						pquest_2->Set(4025, nFont);
						pquest_2->Set(4036, Src, 1);
						pquest_2->Set(4003, 1);
						pquest_2->Set(4009, 0);
						pquest_2->Set(4001, iX);
						float a1b = pquest_2->GetFloat(4026);
						int32_t v20 = static_cast<int32_t>(pquest_2->GetFloat(4025));
						FontManager* v13 = FontManager::Instance();
						v13->CalculateStringDimensions(&textDimensions_2, Src, v20, a1b, 0);
						if (i_2 > iSpace) {
							float v28 = textDimensions_2.y + 5.0;
							pquest_2->Set(4002, v28, 1);
						} else {
							pquest_2->Set(4002, 0);
						}
						float* v14 = &fQuestCinematicCharacterFadeInDelay.Float();
						uifadeInStart += static_cast<int64_t>(*v14 * 1000.0);
						pquest_2->Set(Tile::kTileValue_User0, uifadeInStart);
						double v27 = static_cast<double>(uifadeInStart);
						float v26 = fQuestCinematicCharacterRemain.Float() * 1000.0 + v27;
						pquest_2->Set(Tile::kTileValue_User1, v26, 1);
						double v25 = static_cast<double>(uifadeInStart);
						double a1c = fQuestCinematicCharacterRemain.Float() * 1000.0 + v25;
						InitializationFlow("Set show time %s: %f", pquest_2->GetString(4036), a1c);
						HUDMainMenu::SetBrightness(pquest_2, HUDMainMenu::HB_HUD_TEXT);
						iX += static_cast<int32_t>(textDimensions_2.x);
						double v15 = pquest_2->GetFloat(4001);
						if (static_cast<double>(iWidth) < v15 + textDimensions_2.x) {
							double v16 = pquest_2->GetFloat(4001);
							iWidth = static_cast<int32_t>(v16 + textDimensions_2.x);
						}
					}
				}
				QuestUpdateManager::QuestUpdate* pitem = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
				ourFree(pitem);
				HUDMainMenu::pQuestUpdateManager->RemoveQuestNamesHead();
				HandleCinematicFade(HUDMainMenu::pme->pTiles[14]);
			}
		}
		if (IsQuestNameShowing()) {
			HandleCinematicFade(HUDMainMenu::pme->pTiles[14]);
		} else {
			if (!Interface::IsTopMenuID(Interface::eCharGenMenu) &&
				(AreObjectivesShowing() || !xQuestObjectives.IsEmpty()) && !IsQuestNameShowing()) {
				HandleReminderFade(HUDMainMenu::pme->pTiles[13]);
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HUDMainMenu::UpdateXPMeter() {
	FNVD_INLINED_LOCAL(BSSoundHandle, apResult, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, psoundHand, std::nullopt, std::nullopt);
	static int32_t HUDXPMeter_trait_x_min = Tile::AddUserTrait("_x_min", -1);
	static int32_t HUDXPMeter_trait_x_max = Tile::AddUserTrait("_x_max", -1);
	bool bplayerInCombat = pPlayer->GetNumberActorsInCombat() > 0;
	CharacterProgression* CharacterProgressionInfo = pPlayer->GetCharacterProgressionInfo();
	char bplayerReadyToLevel = CharacterProgressionInfo->GetReadyToLevelUp();
	if ((!Interface::IsInMenuMode() ||
			(Interface::GetTopMenuID() == Interface::eContainerMenu ||
				(Interface::GetTopMenuID() == Interface::eHackingMenu ||
					(Interface::GetTopMenuID() == Interface::eComputersMenu ||
						Interface::GetTopMenuID() == Interface::eDialogMenu)))) &&
		(pVATS.eMode != VATS::VATS_PLAYBACK && !HKWheel.GetVisible())) {
		if (fCurrentHUDOpacity == 0.0) {
			bXPMeterFinishedMoving = 1;
		} else {
			float falpha = pTiles[44]->GetFloat(4009);
			bool bxpMeterShowing = falpha > 0.0;
			bool bxpMeterFadeFinished = pTiles[44]->GetFadeFinished(4009);
			if ((g_XPBarState & 1) != 0) {
				if (bxpMeterShowing || (!bxpMeterFadeFinished || xXPUpdates.IsEmpty())) {
					float fxFadeEnd = pTiles[47]->GetFadeEndFor(4001);
					float fxMin = pTiles[47]->GetFloat(HUDXPMeter_trait_x_min);
					float fxMax = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
					bool bfadeFinishedPointer = pTiles[47]->GetFadeFinished(4001);
					if (!pTiles[47]->IsTrue(Tile::kTileValue_User11) && (bxpMeterFadeFinished && bxpMeterShowing)) {
						int32_t icurrXP_2 =
							static_cast<int32_t>(pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS));
						if (xp_currentActorLevel == xp_nextActorLevel) {
							++xp_nextActorLevel;
						}
						int32_t iprevThreshold_2 = GamePlayFormulas::GetRequiredExperiencePoints(xp_currentActorLevel);
						int32_t inextThreshold_2 = GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
						pTiles[47]->Set(Tile::kTileValue_User11, 1);
						if (icurrXP_2 <= inextThreshold_2) {
							int32_t v5 = BSMAX(icurrXP_2, iprevThreshold_2);
							float a2c = static_cast<float>(BSMIN(v5, inextThreshold_2));
							float a2d = FLerp(fxMin, fxMax, static_cast<float>(iprevThreshold_2),
								static_cast<float>(inextThreshold_2), a2c);
							float a1c = pTiles[47]->GetFloat(4001);
							pTiles[47]->AddFadeControl(4001, a1c, a2d, 2.0f, Tile::TFCT_STANDARD);
						} else {
							if (static_cast<double>(fxMax) < pTiles[47]->GetFloat(4001)) {
								pTiles[47]->Set(4001, fxMax, 1);
							}
							float a1a = pTiles[47]->GetFloat(4001);
							pTiles[47]->AddFadeControl(4001, a1a, fxMax, 1.0f, Tile::TFCT_STANDARD);
						}
					} else {
						if (!bxpMeterFadeFinished || bxpMeterShowing) {
							if (!bfadeFinishedPointer && (bxpMeterFadeFinished && bxpMeterShowing)) {
								int32_t icurrXP_3 =
									static_cast<int32_t>(pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS));
								int32_t iprevThreshold_3 =
									GamePlayFormulas::GetRequiredExperiencePoints(xp_currentActorLevel);
								int32_t inextThreshold_3 =
									GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
								if (bplayerReadyToLevel && fxMax == pTiles[47]->GetFloat(4001) &&
									!pPlayer->GetInChargen() && !bplayerInCombat) {
									InterfaceManager::PlayMenuSound(MENU_SOUND_LEVEL_UP);
								}
								if (icurrXP_3 > inextThreshold_3) {
									int32_t* v6 = &iMaxCharacterLevel.Int();
									if (xp_nextActorLevel < *v6 && !bplayerInCombat) {
										char apDest[4];
										++xp_currentActorLevel;
										++xp_nextActorLevel;
										iprevThreshold_3 = inextThreshold_3;
										inextThreshold_3 =
											GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
										pTiles[47]->Set(4001, fxMin, 1);
										BSsprintf(apDest, 3, "%i", xp_currentActorLevel);
										pTiles[48]->Set(4036, apDest, 1);
										BSsprintf(apDest, 3, "%i", xp_nextActorLevel);
										pTiles[49]->Set(4036, apDest, 1);
										if (icurrXP_3 <= inextThreshold_3) {
											float a2f = FLerp(fxMin, fxMax, static_cast<float>(iprevThreshold_3),
												static_cast<float>(inextThreshold_3), static_cast<float>(icurrXP_3));
											pTiles[47]->AddFadeControl(4001, fxMin, a2f, 2.0f, Tile::TFCT_STANDARD);
										} else {
											if (fxMax < static_cast<double>(fxMin)) {
												fxMin = fxMax;
											}
											pTiles[47]->AddFadeControl(4001, fxMin, fxMax, 1.0f, Tile::TFCT_STANDARD);
										}
									}
								}
							} else {
								if (pTiles[47]->GetFadeFinished(4001) && pTiles[47]->IsTrue(Tile::kTileValue_User11) &&
									pTiles[44]->GetFadeFinished(4009) && pTiles[44]->GetFloat(4009) > 0.0) {
									pTiles[47]->Set(Tile::kTileValue_User11, 0);
									float a1e = pTiles[44]->GetFloat(4009);
									pTiles[44]->AddFadeControl(4009, a1e, 0.0f, 2.0f, Tile::TFCT_STANDARD);
									float a1f = pTiles[45]->GetFloat(4009);
									pTiles[45]->AddFadeControl(4009, a1f, 0.0f, 2.0f, Tile::TFCT_STANDARD);
									float a1g = pTiles[46]->GetFloat(4009);
									pTiles[46]->AddFadeControl(4009, a1g, 0.0f, 2.0f, Tile::TFCT_STANDARD);
									float a1h = pTiles[47]->GetFloat(4009);
									pTiles[47]->AddFadeControl(4009, a1h, 0.0f, 2.0f, Tile::TFCT_STANDARD);
									float a1i = pTiles[48]->GetFloat(4009);
									pTiles[48]->AddFadeControl(4009, a1i, 0.0f, 2.0f, Tile::TFCT_STANDARD);
									float a1j = pTiles[49]->GetFloat(4009);
									pTiles[49]->AddFadeControl(4009, a1j, 0.0f, 2.0f, Tile::TFCT_STANDARD);
								}
							}
						} else {
							HUDMainMenu::pme->pTiles[44]->Set(4003, 0);
							HUDMainMenu::pme->pTiles[45]->Set(4003, 0);
							HUDMainMenu::pme->pTiles[46]->Set(4003, 0);
							HUDMainMenu::pme->pTiles[47]->Set(4003, 0);
							HUDMainMenu::pme->pTiles[48]->Set(4003, 0);
							HUDMainMenu::pme->pTiles[49]->Set(4003, 0);
							if ((g_XPBarState & 8) != 0 && !bXPMeterFinishedMoving) {
								bXPMeterFinishedMoving = 1;
								pTiles[50]->Set(Tile::kTileValue_User10, 0);
							}
						}
					}
				} else {
					BSSimpleList<HUDMainMenu::XPUpdate*>* piter = &xXPUpdates;
					int32_t v70 = 0;
					while (piter && !piter->IsEmpty()) {
						v70 += piter->m_item->iAddedXP;
						piter = piter->m_pkNext;
					}
					while ((&xXPUpdates)->m_item) {
						HUDMainMenu::XPUpdate* pitem = (&xXPUpdates)->m_item;
						void* Block = (void*)pitem;
						ourFree(pitem);
						xXPUpdates.RemoveHead();
					}
					if (v70 > 0 && !bXPMeterFinishedMoving) {
						char Src[16];
						::new (&psoundHand) BSSoundHandle;
						BSAudio* v2 = &BSAudio::QInstance();
						BSSoundHandle* SoundHandleByName =
							v2->GetSoundHandleByName(&arHandle, "UIPopUpExperienceUp", 289u);
						psoundHand.operator=(*SoundHandleByName);
						psoundHand.Play(0);
						float fxp = 0.0f;
						int32_t v60 = 0;
						int32_t v63 = 0;
						int32_t icurrXP =
							static_cast<int32_t>(pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS));
						xp_currentActorLevel = pPlayer->GetLevel();
						xp_nextActorLevel = xp_currentActorLevel + 1;
						int32_t iprevThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_currentActorLevel);
						int32_t inextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
						if (icurrXP > inextThreshold && bplayerInCombat) {
							fxp = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
						}
						if (iprevThreshold == inextThreshold) {
							// The same local as fxp in the exe ([ebp-0x28], 0x77C99F).
							fxp = static_cast<float>(iprevThreshold);
						} else {
							double v3 = pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS);
							float value = v3 - static_cast<double>(v70);
							float afLength = static_cast<float>(inextThreshold);
							float a2 = static_cast<float>(iprevThreshold);
							float a1 = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
							float x1 = pTiles[47]->GetFloat(HUDXPMeter_trait_x_min);
							fxp = FLerp(x1, a1, a2, afLength, value);
						}
						float afLengtha = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
						float a2a = pTiles[47]->GetFloat(HUDXPMeter_trait_x_min);
						float a2b = BSMAX(fxp, a2a);
						float afLengthb = BSMIN(a2b, afLengtha);
						pTiles[47]->Set(4001, afLengthb, 1);
						pTiles[44]->Set(4003, 1);
						pTiles[45]->Set(4003, 1);
						pTiles[46]->Set(4003, 1);
						const char* v4 = sStatsXP.String();
						pTiles[46]->Set(4036, v4, 1);
						pTiles[47]->Set(4003, 1);
						pTiles[48]->Set(4003, 1);
						pTiles[49]->Set(4003, 1);
						pTiles[44]->Set(4009, 0.0f, 1);
						pTiles[45]->Set(4009, 0.0f, 1);
						pTiles[46]->Set(4009, 0.0f, 1);
						pTiles[47]->Set(4009, 0.0f, 1);
						pTiles[48]->Set(4009, 0.0f, 1);
						pTiles[49]->Set(4009, 0.0f, 1);
						BSsprintf(Src, 16, "+%i", v70);
						pTiles[45]->Set(4036, Src, 1);
						BSsprintf(Src, 16, "%i", xp_currentActorLevel);
						pTiles[48]->Set(4036, Src, 1);
						BSsprintf(Src, 16, "%i", xp_nextActorLevel);
						pTiles[49]->Set(4036, Src, 1);
						pTiles[44]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
						pTiles[45]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
						pTiles[46]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
						pTiles[47]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
						pTiles[48]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
						pTiles[49]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
						bxpMeterShowing = 1;
					}
				}
				if (bplayerReadyToLevel) {
					if (bplayerInCombat || (bxpMeterShowing || (g_XPBarState & 8) != 0)) {
						g_XPBarState |= 4u;
					} else {
						g_XPBarState = 2;
					}
				}
			} else {
				if ((g_XPBarState & 2) == 0 ||
					((g_XPBarState & 8) != 0 ||
						(bxpMeterShowing || (!bxpMeterFadeFinished || pPlayer->GetInChargen())))) {
					if ((g_XPBarState & 4) != 0) {
						bool balreadyShown = (g_XPBarState & 8) != 0;
						bool bfadeFinished_2 = pTiles[50]->GetFadeFinished(4009);
						bool balphadOut_2 = pTiles[50]->GetFloat(4009) == 0.0;
						if (balreadyShown) {
							if (!bXPMeterFinishedMoving && bfadeFinished_2 && balphadOut_2) {
								bXPMeterFinishedMoving = 1;
								g_XPBarState = 8;
								pTiles[50]->Set(Tile::kTileValue_User10, 0);
							}
						} else {
							if (bplayerInCombat) {
								g_XPBarState |= 1u;
							} else {
								if (!bxpMeterShowing) {
									g_XPBarState = 2;
								}
							}
						}
					}
				} else {
					bool bfadeFinished = pTiles[50]->GetFadeFinished(4009);
					bool balphadOut = pTiles[50]->GetFloat(4009) == 0.0;
					if (!bfadeFinished ||
						(!balphadOut ||
							(pTiles[50]->IsTrue(Tile::kTileValue_User10) || pPlayer->GetNumberActorsInCombat()))) {
						double Time = static_cast<double>(appTimer.iLastTime);
						if (!(Time - pTiles[50]->GetFloat(Tile::kTileValue_User9) > 1500.0) ||
							(balphadOut || !bfadeFinished)) {
							if (!bXPMeterFinishedMoving && bfadeFinished && balphadOut) {
								bXPMeterFinishedMoving = 1;
								g_XPBarState = 8;
								pTiles[50]->Set(Tile::kTileValue_User10, 0);
							}
						} else {
							uint32_t v10 = appTimer.iLastTime;
							pTiles[50]->Set(Tile::kTileValue_User9, v10 + 2000);
							pTiles[50]->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.5f, Tile::TFCT_STANDARD);
						}
					} else {
						BSAudio* v7 = &BSAudio::QInstance();
						BSSoundHandle* v36 = v7->GetSoundHandleByName(&apResult, "UILevelUpText", 289u);
						v36->Play(0);
						pTiles[50]->Set(4009, 0.0f, 1);
						pTiles[50]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 0.5f, Tile::TFCT_STANDARD);
						const char* v8 = sLevelUp.String();
						pTiles[50]->Set(4036, v8, 1);
						pTiles[50]->Set(4003, 1);
						uint32_t v9 = appTimer.iLastTime;
						pTiles[50]->Set(Tile::kTileValue_User9, v9);
						pTiles[50]->Set(Tile::kTileValue_User10, 1);
					}
				}
			}
		}
	} else {
		HUDMainMenu::pme->pTiles[44]->Set(4003, 0);
		HUDMainMenu::pme->pTiles[45]->Set(4003, 0);
		HUDMainMenu::pme->pTiles[46]->Set(4003, 0);
		HUDMainMenu::pme->pTiles[47]->Set(4003, 0);
		HUDMainMenu::pme->pTiles[48]->Set(4003, 0);
		HUDMainMenu::pme->pTiles[49]->Set(4003, 0);
		HUDMainMenu::pme->pTiles[44]->Set(4009, 0.0f, 1);
		HUDMainMenu::pme->pTiles[44]->RemoveFadeControl(4009);
		g_XPBarState |= 1u;
		bXPMeterFinishedMoving = 0;
	}
}

void HUDMainMenu::UpdateRadiationMeter() {
	static int32_t ivisFlag = 4;
	double Float = HUDMainMenu::pme->pTiles[19]->GetFloat(Tile::kTileValue_User6);
	bool bdrankRads = static_cast<double>(appTimer.iLastTime) < Float;
	bool binRadiation = ReferenceProcessLists.IsPlayerInRadiationArea();
	if (binRadiation || (bdrankRads || ivisFlag != 4)) {
		bool brecalcRadsTarget = 0;
		if (HUDMainMenu::pme->pTiles[17]->GetFadeFinished(4009) &&
			HUDMainMenu::pme->pTiles[17]->GetFloat(4009) == 0.0 &&
			HUDMainMenu::pme->pTiles[18]->GetFadeFinished(4009) &&
			HUDMainMenu::pme->pTiles[18]->GetFloat(4009) == 0.0 &&
			HUDMainMenu::pme->pTiles[19]->GetFadeFinished(4009) &&
			HUDMainMenu::pme->pTiles[19]->GetFloat(4009) == 0.0 &&
			HUDMainMenu::pme->pTiles[20]->GetFadeFinished(4009) &&
			HUDMainMenu::pme->pTiles[20]->GetFloat(4009) == 0.0) {
			ivisFlag = 4;
		}
		if ((ivisFlag == 4 || ivisFlag == 2) && (binRadiation || bdrankRads)) {
			ivisFlag = 8;
			brecalcRadsTarget = 1;
		}
		float fstart = 15.0f;
		float fend = 315.0f;
		float fplayersCurrentRads = 0.0f;
		float fradsTotal = 0.0f;
		float ftargetRads = 0.0f;
		bool v24 = HUDMainMenu::pme->pTiles[19]->IsTrue(Tile::kTileValue_User7) || bdrankRads;
		if (binRadiation || (bdrankRads || brecalcRadsTarget)) {
			float fplayerRads = static_cast<float>(pPlayer->GetActorValue(ActorValue::RADIATION_RADS));
			bool bchangeVal = 0;
			if (binRadiation || (brecalcRadsTarget || v24)) {
				bchangeVal = 1;
				fradsTotal = HUDMainMenu::pme->pTiles[19]->GetFloat(Tile::kTileValue_User5);
				float fplayerRads_2 = fplayerRads - fradsTotal;
				float a1 = BSMIN(fplayerRads_2, 1000.0f);
				float a2 = BSMAX(a1, 0.0f);
				fplayersCurrentRads = FLerp(fstart, fend, 0.0f, 1000.0f, a2);
			}
			if ((bdrankRads && brecalcRadsTarget) || (v24 || binRadiation)) {
				bchangeVal = 1;
				float a1a = BSMIN(fplayerRads, 1000.0f);
				float a2a = BSMAX(a1a, 0.0f);
				ftargetRads = FLerp(fstart, fend, 0.0f, 1000.0f, a2a);
			}
			if (bchangeVal) {
				BSsprintf(g_sRadiationTextBuffer, 260u, "+%i", BSMAX(1, static_cast<int32_t>(fradsTotal)));
				HUDMainMenu::pme->pTiles[19]->Set(4036, g_sRadiationTextBuffer, 1);
			}
		}
		if (brecalcRadsTarget) {
			float ftargetRads_2;
			ivisFlag = 8;
			if (binRadiation) {
				ftargetRads_2 = ftargetRads;
			} else {
				ftargetRads_2 = fplayersCurrentRads;
			}
			float a1b = BSMIN(ftargetRads_2, fend);
			HUDMainMenu::pme->pTiles[18]->Set(4001, a1b, 1);
			HUDMainMenu::pme->pTiles[61]->Set(4003, 1);
			float afEnd = fCurrentHUDOpacity;
			float afStart = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[17]->AddFadeControl(4009, afStart, afEnd, 0.25f, Tile::TFCT_STANDARD);
			float afEnda = fCurrentHUDOpacity;
			float afStarta = HUDMainMenu::pme->pTiles[18]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[18]->AddFadeControl(4009, afStarta, afEnda, 0.25f, Tile::TFCT_STANDARD);
			float afEndb = fCurrentHUDOpacity;
			float afStartb = HUDMainMenu::pme->pTiles[19]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[19]->AddFadeControl(4009, afStartb, afEndb, 0.25f, Tile::TFCT_STANDARD);
			float afEndc = fCurrentHUDOpacity;
			float afStartc = HUDMainMenu::pme->pTiles[20]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[20]->AddFadeControl(4009, afStartc, afEndc, 0.25f, Tile::TFCT_STANDARD);
		} else {
			if (binRadiation) {
				float a1c = BSMIN(ftargetRads, fend);
				HUDMainMenu::pme->pTiles[18]->Set(4001, a1c, 1);
			}
		}
		if (v24) {
			HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User7, 0);
			float a1d = BSMIN(ftargetRads, fend);
			HUDMainMenu::pme->pTiles[18]->Set(Tile::kTileValue_User0, a1d, 1);
			float a2b = BSMIN(50.0f, fradsTotal);
			float fpointerMovementTime = FLerp(1.5f, 3.0f, 0.0f, 50.0f, a2b);
			float fcurrentX = HUDMainMenu::pme->pTiles[18]->GetFloat(4001);
			float a1e = HUDMainMenu::pme->pTiles[18]->GetFloat(Tile::kTileValue_User0);
			float fnextX = BSMAX(a1e, fstart);
			if (!binRadiation) {
				HUDMainMenu::pme->pTiles[18]->AddFadeControl(4001, fcurrentX, fnextX, fpointerMovementTime,
					Tile::TFCT_STANDARD);
			}
		}
		if (!binRadiation && !bdrankRads) {
			ivisFlag = 2;
			float afStartd = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[17]->AddFadeControl(4009, afStartd, 0.0f, 0.25f, Tile::TFCT_STANDARD);
			float afStarte = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[18]->AddFadeControl(4009, afStarte, 0.0f, 0.25f, Tile::TFCT_STANDARD);
			float afStartf = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[19]->AddFadeControl(4009, afStartf, 0.0f, 0.25f, Tile::TFCT_STANDARD);
			float afStartg = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
			HUDMainMenu::pme->pTiles[20]->AddFadeControl(4009, afStartg, 0.0f, 0.25f, Tile::TFCT_STANDARD);
		}
	}
}

void HUDMainMenu::UpdateStatusModeUI(bool abIsHardcore) {
	ActorValueOwner* v11;
	ActorValueOwner* v12;
	ActorValueOwner* v13;
	ActorValueOwner* v14;
	ActorValueOwner* apActor;
	ActorValueOwner* a1;
	const char* v2 = sStatsLMBAbbrev.String();
	HUDMainMenu::pme->pTiles[85]->Set(4036, v2, static_cast<bool>(1));
	const char* v3 = sStatsRADAbbrev.String();
	HUDMainMenu::pme->pTiles[84]->Set(4036, v3, static_cast<bool>(1));
	const char* v4 = sStatsH20Abbrev.String();
	HUDMainMenu::pme->pTiles[79]->Set(4036, v4, static_cast<bool>(1));
	const char* v5 = sStatsFODAbbrev.String();
	HUDMainMenu::pme->pTiles[81]->Set(4036, v5, static_cast<bool>(1));
	const char* v6 = sStatsSLPAbbrev.String();
	HUDMainMenu::pme->pTiles[80]->Set(4036, v6, static_cast<bool>(1));
	int32_t iTotalCrippledLimbs = 0;
	if (pPlayer) {
		a1 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
	} else {
		a1 = 0;
	}
	float factorLeftArmCondition = CombatFormulas::GetBodyPartCondition(a1, ActorValue::LEFT_ATTACK_CONDITION, 0);
	if (factorLeftArmCondition == 0.0) {
		iTotalCrippledLimbs = 1;
	}
	if (pPlayer) {
		apActor = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
	} else {
		apActor = 0;
	}
	float factorLeftLegCondition =
		CombatFormulas::GetBodyPartCondition(apActor, ActorValue::LEFT_MOBILITY_CONDITION, 0);
	if (factorLeftLegCondition == 0.0) {
		++iTotalCrippledLimbs;
	}
	if (pPlayer) {
		v14 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
	} else {
		v14 = 0;
	}
	float factorRightLegCondition = CombatFormulas::GetBodyPartCondition(v14, ActorValue::RIGHT_MOBILITY_CONDITION, 0);
	if (factorRightLegCondition == 0.0) {
		++iTotalCrippledLimbs;
	}
	if (pPlayer) {
		v13 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
	} else {
		v13 = 0;
	}
	float factorRightArmCondition = CombatFormulas::GetBodyPartCondition(v13, ActorValue::RIGHT_ATTACK_CONDITION, 0);
	if (factorRightArmCondition == 0.0) {
		++iTotalCrippledLimbs;
	}
	if (pPlayer) {
		v12 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
	} else {
		v12 = 0;
	}
	float factorHeadCondition = CombatFormulas::GetBodyPartCondition(v12, ActorValue::PERCEPTION_CONDITION, 0);
	if (factorHeadCondition == 0.0) {
		++iTotalCrippledLimbs;
	}
	if (pPlayer) {
		v11 = &static_cast<ActorValueOwner&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
	} else {
		v11 = 0;
	}
	float factorBodyCondition = CombatFormulas::GetBodyPartCondition(v11, ActorValue::ENDURANCE_CONDITION, 0);
	if (factorBodyCondition == 0.0) {
		++iTotalCrippledLimbs;
	}
	float fTotalLimbCondition = 0.0f;
	if (iTotalCrippledLimbs < 4) {
		if (iTotalCrippledLimbs < 3) {
			if (iTotalCrippledLimbs < 2) {
				if (iTotalCrippledLimbs >= 1) {
					fTotalLimbCondition = fHCHalfAlphaTextThreshold.Float() + 1.0;
				}
			} else {
				fTotalLimbCondition = fHCFullAlphaTextThreshold.Float() + 1.0;
			}
		} else {
			fTotalLimbCondition = fHCRedTextThreshold.Float() + 1.0;
		}
	} else {
		fTotalLimbCondition = fHCFlashTextThreshold.Float() + 1.0;
	}
	float afCurrValue = static_cast<float>(pPlayer->GetActorValue(ActorValue::RADIATION_RADS));
	StatusModeTileEffects(HUDMainMenu::pme->pTiles[84], afCurrValue, 0);
	StatusModeTileEffects(HUDMainMenu::pme->pTiles[85], fTotalLimbCondition, !abIsHardcore);
	if (abIsHardcore) {
		float afCurrValuea = static_cast<float>(pPlayer->GetActorValue(ActorValue::DEHYDRATION));
		StatusModeTileEffects(HUDMainMenu::pme->pTiles[79], afCurrValuea, 0);
		float afCurrValueb = static_cast<float>(pPlayer->GetActorValue(ActorValue::SLEEPDEPREVATION));
		StatusModeTileEffects(HUDMainMenu::pme->pTiles[80], afCurrValueb, 0);
		float afCurrValuec = static_cast<float>(pPlayer->GetActorValue(ActorValue::HUNGER));
		StatusModeTileEffects(HUDMainMenu::pme->pTiles[81], afCurrValuec, 1);
	} else {
		HUDMainMenu::pme->pTiles[79]->Set(4009, 0.0f, static_cast<bool>(1));
		HUDMainMenu::pme->pTiles[80]->Set(4009, 0.0f, static_cast<bool>(1));
		HUDMainMenu::pme->pTiles[81]->Set(4009, 0.0f, static_cast<bool>(1));
	}
}

void HUDMainMenu::UpdateEnemyHealth(Actor* apActor) {
	int32_t fHasSeeHealthPerk;
	float v8 = static_cast<float>(apActor->GetBaseActorValue(ActorValue::HEALTH));
	float fcurrent_health = static_cast<float>(apActor->GetActorValue(ActorValue::HEALTH));
	float fDDT = GetAdjustedDamageThreshold(pPlayer, apActor);
	float fperhealth = 0.0;
	if (v8 > 0.0) {
		fperhealth = fcurrent_health / v8;
	}
	static float fperhealthlast = fperhealth;
	if (fperhealth != fperhealthlast) {
		iEnemyHealthTimer = HudTimer.iLastTime;
		fperhealthlast = fperhealth;
	}
	if (apActor->GetDestroyed()) {
		HUDMainMenu::UpdateMeter(pTiles[22], 0.0f, HUDMainMenu::HJ_CENTER, 0, 0);
	} else {
		if (apActor->IsDead(0)) {
			HUDMainMenu::UpdateMeter(pTiles[22], fperhealth, HUDMainMenu::HJ_CENTER, 0, 0);
		} else {
			HUDMainMenu::UpdateMeter(pTiles[22], fperhealth, HUDMainMenu::HJ_CENTER, 1, 0);
		}
	}
	*reinterpret_cast<float*>(&fHasSeeHealthPerk) = 0.0;
	BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::SEE_ENEMY_HEALTH, pPlayer, &fHasSeeHealthPerk);
	if (!(*reinterpret_cast<float*>(&fHasSeeHealthPerk) > 0.0)) {
		const char* FullName = apActor->GetFullName();
		HUDMainMenu::pme->pTiles[23]->Set(4036, FullName, 1);
	} else {
		char Src[260];
		double a2 = fDDT;
		const char* v5 = sInventoryDamageThreshold.String();
		int32_t afMeterPercent_4 = static_cast<int32_t>(fcurrent_health);
		const char* afMeterPercent = sStatsHP.String();
		std::sprintf(Src, "%s\n %s:%d %s:%.2f", apActor->GetFullName(), afMeterPercent, afMeterPercent_4, v5, a2);
		HUDMainMenu::pme->pTiles[23]->Set(4036, Src, 1);
	}
}

void HUDMainMenu::KillQuestName() {
	NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[HMMT_QUEST_ADDED]->xChildren.m_pkHead;
	while (pPos) {
		Tile* pTile = HUDMainMenu::pme->pTiles[HMMT_QUEST_ADDED]->xChildren.GetNext(pPos);
		if (pTile) {
			pTile->Set(Tile::kTileValue_Visible, 0);
		}
	}
	if (HUDMainMenu::pme->eMenuMode != HUDMainMenu::HMM_LOAD) {
		while (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()) {
			QuestUpdateManager::QuestUpdate* pitem = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
			ourFree(pitem);
			HUDMainMenu::pQuestUpdateManager->RemoveQuestNamesHead();
		}
		HUDMainMenu::pQuestUpdateManager->RemoveQuestNamesAll();
	}
}

void HUDMainMenu::KillQuestObjectives() {
	NiTListItem<Tile*>* xchildIter = HUDMainMenu::pme->pTiles[13]->xChildren.m_pkTail;
	Tile* v5 = 0;
	while (xchildIter) {
		Tile* ptile = HUDMainMenu::pme->pTiles[13]->xChildren.GetPrev(xchildIter);
		if (ptile) {
			ptile->Set(4003, 0);
			ptile->Set(4009, 0.0f, 1);
			int32_t v1 = Tile::TextToTrait("_WaitComplete");
			ptile->Set(v1, 0.0f, 1);
			int32_t v2 = Tile::TextToTrait("_HoldTime");
			ptile->Set(v2, 0.0f, 1);
		}
	}
	if (HUDMainMenu::pme->eMenuMode != HUDMainMenu::HMM_LOAD) {
		for (BSSimpleList<HUDMainMenu::QueuedQuestText*>* plist = &HUDMainMenu::pme->xQuestObjectives;
			plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
			ourFree(plist->m_item);
		}
		HUDMainMenu::pme->xQuestObjectives.RemoveAll();
	}
}

HUDMainMenu::SubtitleData* HUDMainMenu::GetNextSubtitle() {
	HUDMainMenu::SubtitleData* pResult = 0;
	float fClosestSqrDist = 3.4028235e+38f;
	NiPoint3 playerLoc = pPlayer->GetLocationOnReference();
	for (int32_t isubtitleCt = 0;; ++isubtitleCt) {
		double v3;
		uint32_t FormFlags = xSubtitles.iSize;
		if (isubtitleCt >= FormFlags) {
			break;
		}
		float fcurrSqrDist = (xSubtitles.GetAt(isubtitleCt)->SpeakerLoc - playerLoc).SqrLength();
		if (pResult || (v3 = std::pow(500.0f, 2), !(fcurrSqrDist <= v3))) {
			int32_t iPriority;
			if (!pResult ||
				(!(fClosestSqrDist > static_cast<double>(fcurrSqrDist)) ||
					(iPriority = xSubtitles.GetAt(isubtitleCt)->iPriority, iPriority < pResult->iPriority))) {
				if (xSubtitles.GetAt(isubtitleCt)->iPriority != 0x7FFFFFFF) {
					continue;
				}
			}
		}
		pResult = xSubtitles.GetAt(isubtitleCt);
		fClosestSqrDist = fcurrSqrDist;
	}
	return pResult;
}

void HUDMainMenu::HideMessage() {
	FNVD_INLINED_LOCAL(BSSoundHandle, dst, std::nullopt, std::nullopt);
	if (!xMessages.IsEmpty()) {
		for (HUDMainMenu::MessageData* item : xMessages) {
			if (item) {
				const char* String = HUDMainMenu::pme->pTiles[26]->GetString(4036);
				if (!BSstrcmp(item->strText, String)) {
					HUDMainMenu::MessageData* nd = item;
					xMessages.Remove(item);
					void* Block = nd;
					ourFree(nd);
					if (xMessages.IsEmpty()) {
						HUDMainMenu::pme->pTiles[26]->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.35f,
							Tile::TFCT_STANDARD);
						HUDMainMenu::pme->pTiles[25]->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.35f,
							Tile::TFCT_STANDARD);
						HUDMainMenu::pme->pTiles[27]->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.35f,
							Tile::TFCT_STANDARD);
						uiMessageStartTime = 0;
					} else {
						if ((&xMessages)->m_item->SoundName[0]) {
							char* afStart = (&xMessages)->m_item->SoundName;
							pPlayer->PlaySoundByEditorName(&dst, afStart, 0, 289u, 0);
						}
						HUDMainMenu::MessageData* afLength = (&xMessages)->m_item;
						HUDMainMenu::pme->pTiles[26]->Set(4036, afLength->strText, 1);
						char* afLengtha = (&xMessages)->m_item->IconFilename;
						HUDMainMenu::pme->pTiles[25]->Set(4044, afLengtha, 1);
						uiMessageStartTime = HudTimer.iLastTime;
					}
					return;
				}
			}
		}
	}
}

void HUDMainMenu::KillQuestUpdates() {
	if (HUDMainMenu::pme) {
		HUDMainMenu::pme->bKillQuestUpdates = (bool)1;
	}
}

void HUDMainMenu::ShowCrippledGuy() {
	char Src[268];
	int32_t ifaceImage = 0;
	for (ActiveEffect*& rpItem : pPlayer->GetActiveEffectList()->ItemsUntilNull()) {
		if (rpItem->eSpellType == MagicSystem::ADDICTION) {
			ifaceImage = 10;
		}
	}
	if (!ifaceImage) {
		float fmax = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
		float fdmg = fmax - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
		ifaceImage = static_cast<int32_t>(fdmg / fmax * 5.0);
		if (ifaceImage < 0) {
			ifaceImage = 0;
		}
		if (ifaceImage > 4) {
			ifaceImage = 4;
		}
	}
	BSsprintf(Src, 260, "Interface\\Stats\\face_%02d.dds", ifaceImage);
	pTiles[52]->Set(4044, Src, 1);
	pTiles[52]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 5.0f, Tile::TFCT_FADE_IN_HOLD_FADE_OUT);
	HUDMainMenu::HUD_MAIN_MENU_TILES etile = HUDMainMenu::HMMT_CRIPPLED_HEAD;
	for (ActorValue::Index eindex = ActorValue::PERCEPTION_CONDITION; eindex < ActorValue::BRAIN_CONDITION;
		eindex = static_cast<ActorValue::Index>(eindex + 1)) {
		const char* apFormat;
		float fval = pPlayer->GetActorFloatValue(eindex) / 100.0;
		if (!(fval > 0.0)) {
			apFormat = "Interface\\Stats\\%s_broken.dds";
		} else {
			apFormat = "Interface\\Stats\\%s.dds";
		}
		BSsprintf(Src, 260, apFormat, HUDCrippledLimbNames[eindex - ActorValue::PERCEPTION_CONDITION]);
		pTiles[etile]->Set(4044, Src, 1);
		if (fval == 0.0) {
			pTiles[etile]->Set(4084, 2);
			pTiles[etile]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 5.0f, Tile::TFCT_BLINK_FAST_FADE);
		} else {
			pTiles[etile]->Set(4084, 1);
			pTiles[etile]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 5.0f, Tile::TFCT_FADE_IN_HOLD_FADE_OUT);
		}
		etile = static_cast<HUDMainMenu::HUD_MAIN_MENU_TILES>(etile + 1);
	}
}

// The list head comes through the exe's shared one-field getter (0x559450), a trivial accessor.
bool HUDMainMenu::IsQuestNameShowing() {
	if (!bQuestNameShowing) {
		return 0;
	}
	NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[14]->xChildren.m_pkHead;
	while (pPos) {
		Tile* pChild = HUDMainMenu::pme->pTiles[14]->xChildren.GetNext(pPos);
		if (pChild->Tile::IsTrue(4003)) {
			return 1;
		}
	}
	bQuestNameShowing = 0;
	return 0;
}

bool HUDMainMenu::AreObjectivesShowing() {
	if (!bObjectiveNameShowing) {
		return 0;
	}
	NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[13]->xChildren.m_pkHead;
	while (pPos) {
		Tile* pChild = HUDMainMenu::pme->pTiles[13]->xChildren.GetNext(pPos);
		if (pChild->Tile::IsTrue(4003)) {
			return 1;
		}
	}
	bObjectiveNameShowing = 0;
	return 0;
}

void HUDMainMenu::UpdateRadiationMeterConditions(float a1, float a2) {
	float ftotalRads = a1 + a2;
	if (!(a1 > 0.0)) {
		if (a2 > 0.0) {
			double Float = HUDMainMenu::pme->pTiles[19]->GetFloat(Tile::kTileValue_User6);
			if (static_cast<double>(appTimer.iLastTime) >= Float) {
				HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User5, ftotalRads, static_cast<bool>(1));
			}
		}
	} else {
		float afValue = static_cast<double>(appTimer.iLastTime) + 2000.0;
		HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User6, afValue, static_cast<bool>(1));
		HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User5, ftotalRads, static_cast<bool>(1));
		HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User7, 1);
	}
}

void HUDMainMenu::StatusModeTileEffects(Tile* apTile, float afCurrValue, bool bResetValues) {
	float v7 = 0.5f;
	if (!(fHCFlashTextThreshold.Float() <= static_cast<double>(afCurrValue))) {
		if (!(fHCRedTextThreshold.Float() <= static_cast<double>(afCurrValue))) {
			if (!(fHCFullAlphaTextThreshold.Float() <= static_cast<double>(afCurrValue))) {
				if (!(fHCHalfAlphaTextThreshold.Float() <= static_cast<double>(afCurrValue))) {
					apTile->Set(4084, 1);
					apTile->Set(4009, 0);
				} else {
					apTile->Set(4084, 1);
					apTile->Set(4009, 124);
				}
			} else {
				apTile->Set(4084, 1);
				apTile->Set(4009, 255);
			}
		} else {
			apTile->Set(4084, 2);
			apTile->Set(4009, 255);
		}
	} else {
		apTile->Set(4084, 2);
		apTile->Set(4009, fCurrAlphaHardcore, static_cast<bool>(1));
	}
	if (bResetValues) {
		double v4;
		uint32_t Time = appTimer.GetTime();
		uint32_t v6 = Time - iLastHardcoreFadeTime;
		iLastHardcoreFadeTime = Time;
		if (bAlphaGoingDown) {
			v4 = fCurrAlphaHardcore - static_cast<double>(v6) * v7;
		} else {
			v4 = static_cast<double>(v6) * v7 + fCurrAlphaHardcore;
		}
		fCurrAlphaHardcore = v4;
		if (fCurrAlphaHardcore >= 255.0) {
			bAlphaGoingDown = 1;
		}
		if (fCurrAlphaHardcore <= 128.0) {
			bAlphaGoingDown = 0;
		}
		BSClamp(fCurrAlphaHardcore, 128.0f, 255.0f);
	}
}

// The exe's cleanup-only C++ EH frame (the sound handle and the two trait statics) is left out.
void HUDMainMenu::HandleReminderFade(Tile* apTile) {
	FNVD_INLINED_LOCAL(BSSoundHandle, arHandle, std::nullopt, std::nullopt);
	NiPoint3 textDimensions;
	int32_t iScreenWidth = static_cast<int32_t>(InterfaceManager::GetScreenWidth());
	bool bShowedObjective = false;
	if (!AreObjectivesShowing()) {
		BSSimpleList<QueuedQuestText*>* plist = &xQuestObjectives;
		// The objective text tiles are the quest stage tile's children after its first 31; their check boxes are the
		// children from the tail backwards. The exe reloads pme for each list call.
		NiTListItem<Tile*>* pQuestPos = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.m_pkHead;
		for (int32_t i = 0; i < 31; ++i) {
			HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetNext(pQuestPos);
		}
		NiTListItem<Tile*>* pCheckBoxPos = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.m_pkTail;
		while (plist && !plist->IsEmpty() && pQuestPos) {
			bool bNotShowing = !AreObjectivesShowing();
			bool bTileIsReminder;
			if (bNotShowing) {
				bTileIsReminder = false;
			} else {
				int32_t iTrait = Tile::TextToTrait("_IsReminder");
				bTileIsReminder = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetHead()->IsTrue(iTrait);
			}
			bool bItemIsReminder = plist->m_item->byte105;
			// While objectives show, only texts of the same kind (reminder or not) as the first tile are taken.
			if ((!bNotShowing && bTileIsReminder && !bItemIsReminder) ||
				(!bNotShowing && !bTileIsReminder && bItemIsReminder)) {
				plist = plist->m_pkNext;
				continue;
			}
			bObjectiveNameShowing = true;
			bShowedObjective = true;
			Tile* pQuest = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetPrev(pQuestPos);
			Tile* pCheckBox = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetPrev(pCheckBoxPos);
			if (plist->m_item->byte104) {
				char acText[260];
				BSstrcpy(acText, 260, sHUDQuestCompleted.String());
				BSstrcat(acText, 260, plist->m_item->msg);
				pQuest->Set(Tile::kTileValue_String, acText, true);
				int32_t iIsReminder = bItemIsReminder;
				pQuest->Set(Tile::TextToTrait("_IsReminder"), iIsReminder);
				pQuest->Set(Tile::kTileValue_User3, 1);
				pCheckBox->Set(Tile::kTileValue_Filename, "Interface\\Shared\\Marker\\glow_square_filled_small.dds",
					true);
			} else {
				pQuest->Set(Tile::kTileValue_String, plist->m_item->msg, true);
				pQuest->Set(Tile::kTileValue_User3, 0);
				int32_t iIsReminder = bItemIsReminder;
				pQuest->Set(Tile::TextToTrait("_IsReminder"), iIsReminder);
				pCheckBox->Set(Tile::kTileValue_Filename, "Interface\\Shared\\Marker\\glow_square_small.dds", true);
			}
			float fCheckBoxX = pQuest->GetFloat(Tile::kTileValue_X) - 20.0;
			pCheckBox->Set(Tile::kTileValue_X, fCheckBoxX, true);
			pCheckBox->Set(Tile::kTileValue_Visible, 1);
			pQuest->Set(Tile::kTileValue_Font, 7);
			pQuest->Set(Tile::kTileValue_Visible, 1);
			pQuest->Set(Tile::kTileValue_Alpha, 0);
			pQuest->Set(Tile::kTileValue_WrapWidth, iScreenWidth / 3);
			char acName[260];
			int32_t iObjective = 0;
			BSsprintf(acName, 260, "obj_%d", iObjective);
			++iObjective;
			pQuest->SetName(acName);
			HUDMainMenu::SetBrightness(pQuest, HUDMainMenu::HB_HUD_TEXT);
			BSAudio::QInstance().GetSoundHandleByName(&arHandle, "UIQuestUpdate", 0x121u)->Play(false);
			plist->Remove(plist->m_item);
		}
	}
	if (bShowedObjective) {
		// Lays the shown texts out top to bottom, each check box 3 below its text.
		NiTListItem<Tile*>* pTextPos = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.m_pkHead;
		NiTListItem<Tile*>* pBoxPos = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.m_pkTail;
		for (int32_t i = 0; i < 31; ++i) {
			HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetPrev(pBoxPos);
		}
		int32_t iY = 0;
		int32_t iCount = 0;
		while (pTextPos && iCount < 32) {
			++iCount;
			Tile* pText = apTile->xChildren.GetNext(pTextPos);
			Tile* pBox = apTile->xChildren.GetNext(pBoxPos);
			if (pText && pText->IsTrue(Tile::kTileValue_Visible)) {
				pText->Set(Tile::kTileValue_Y, iY);
				pBox->Set(Tile::kTileValue_Y, iY + 3);
				float fWrapWidth = pText->GetFloat(Tile::kTileValue_WrapWidth);
				int32_t iFont = static_cast<int32_t>(pText->GetFloat(Tile::kTileValue_Font));
				const char* pString = pText->GetString(Tile::kTileValue_String);
				FontManager::Instance()->CalculateStringDimensions(&textDimensions, pString, iFont, fWrapWidth, 0);
				iY += static_cast<int32_t>(textDimensions.y);
			}
		}
	}
	static int32_t trait_WaitCompleted = Tile::AddUserTrait("_WaitComplete", -1);
	NiTListItem<Tile*>* pTilePos = apTile->xChildren.m_pkHead;
	NiTListItem<Tile*>* pCheckBoxPos = apTile->xChildren.m_pkTail;
	for (int32_t i = 0; i < 31; ++i) {
		HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetPrev(pCheckBoxPos);
	}
	// The first (up to) four visible texts and their check boxes.
	int32_t iCurrentChild = 0;
	Tile* aTiles[4] = {};
	Tile* aBoxes[4] = {};
	int32_t iCount = 0;
	while (pTilePos && iCurrentChild < 4 && iCount < 32) {
		++iCount;
		Tile* pTile = apTile->xChildren.GetNext(pTilePos);
		Tile* pBox = apTile->xChildren.GetNext(pCheckBoxPos);
		if (pTile->IsTrue(Tile::kTileValue_Visible)) {
			// The exe compares the next free text entry with the previous check box entry.
			if (iCurrentChild > 0 && aTiles[iCurrentChild] == aBoxes[iCurrentChild - 1]) {
				break;
			}
			aBoxes[iCurrentChild] = pBox;
			aTiles[iCurrentChild] = pTile;
			++iCurrentChild;
		}
	}
	if (!aTiles[0]) {
		return;
	}
	if (aTiles[0]->GetFloat(trait_WaitCompleted) == 0.0) {
		// Fades the first text in and starts the wait of the first three.
		aTiles[0]->Set(trait_WaitCompleted, 0);
		aTiles[0]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity, 0.5f, Tile::TFCT_STANDARD);
		float fDelay0 = fQuestCinematicObjectiveFadeInDelay.Float() * 3.0;
		aTiles[0]->AddFadeControl(trait_WaitCompleted, 0.0f, 1.0f, fDelay0, Tile::TFCT_STANDARD);
		aBoxes[0]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity, 0.5f, Tile::TFCT_STANDARD);
		if (aTiles[1]) {
			float fDelay1 = fQuestCinematicObjectiveFadeInDelay.Float() * 1.0;
			aTiles[1]->AddFadeControl(trait_WaitCompleted, 0.0f, 1.0f, fDelay1, Tile::TFCT_STANDARD);
			if (aTiles[2]) {
				float fDelay2 = fQuestCinematicObjectiveFadeInDelay.Float() * 2.0;
				aTiles[2]->AddFadeControl(trait_WaitCompleted, 0.0f, 1.0f, fDelay2, Tile::TFCT_STANDARD);
			}
		}
	} else if (aTiles[0]->GetFloat(trait_WaitCompleted) == 1.0) {
		static int32_t trait = Tile::AddUserTrait("_HoldTime", -1);
		if (aTiles[0]->GetFloat(trait) == 0.0) {
			// Holds the first text.
			aTiles[0]->Set(trait, 0);
			aTiles[0]->AddFadeControl(trait, 0.0f, 1.0f, fQuestCinematicObjectivePauseTime.Float(),
				Tile::TFCT_STANDARD);
		} else if (aTiles[0]->GetFloat(trait) == 1.0) {
			// The hold is over: fades the first text and its check box out.
			aTiles[0]->AddFadeControl(Tile::kTileValue_Alpha, fCurrentHUDOpacity, 0.0f,
				fQuestCinematicObjectiveFadeOut.Float(), Tile::TFCT_STANDARD);
			aTiles[0]->Set(trait_WaitCompleted, 2);
			aBoxes[0]->AddFadeControl(Tile::kTileValue_Alpha, fCurrentHUDOpacity, 0.0f,
				fQuestCinematicObjectiveFadeOut.Float(), Tile::TFCT_STANDARD);
		}
	} else if (aTiles[0]->GetFadeFinished(Tile::kTileValue_Alpha) &&
		aTiles[0]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
		// The first text has faded out: scrolls the others up into its place and hides it.
		if (aTiles[1]) {
			double fNextY = aTiles[1]->GetFloat(Tile::kTileValue_Y);
			int32_t iDeltaY = static_cast<int32_t>(fNextY - aTiles[0]->GetFloat(Tile::kTileValue_Y));
			pTilePos = apTile->xChildren.m_pkHead;
			Tile* pTempTile = nullptr;
			if (pTilePos) {
				pTempTile = apTile->xChildren.GetNext(pTilePos);
			}
			while (pTilePos) {
				pTempTile = apTile->xChildren.GetNext(pTilePos);
				float fLength = fQuestCinematicObjectiveScrollTime.Float();
				float fEnd = pTempTile->GetFloat(Tile::kTileValue_Y) - iDeltaY;
				float fStart = pTempTile->GetFloat(Tile::kTileValue_Y);
				pTempTile->AddFadeControl(Tile::kTileValue_Y, fStart, fEnd, fLength, Tile::TFCT_STANDARD);
			}
		}
		if (aTiles[3]) {
			aTiles[3]->Set(trait_WaitCompleted, 1);
		}
		aTiles[0]->Set(Tile::kTileValue_Visible, 0);
		aBoxes[0]->Set(Tile::kTileValue_Visible, 0);
		aTiles[0]->Set(trait_WaitCompleted, 0);
		aTiles[0] = nullptr;
	}
	// The second and third texts fade in, with their check boxes, once their wait is over.
	if (aTiles[1] && aTiles[1]->GetFloat(trait_WaitCompleted) == 1.0 &&
		aTiles[1]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
		aTiles[1]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity,
			fQuestCinematicObjectiveFadeIn.Float(), Tile::TFCT_STANDARD);
		aBoxes[1]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity,
			fQuestCinematicObjectiveFadeIn.Float(), Tile::TFCT_STANDARD);
	}
	if (aTiles[2] && aTiles[2]->GetFloat(trait_WaitCompleted) == 1.0 &&
		aTiles[2]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
		aTiles[2]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity,
			fQuestCinematicObjectiveFadeIn.Float(), Tile::TFCT_STANDARD);
		aBoxes[2]->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity,
			fQuestCinematicObjectiveFadeIn.Float(), Tile::TFCT_STANDARD);
	}
}

// The list heads come through the exe's shared one-field getter (0x559450) and the time through BSTimer::GetTime
// (0x825C00), both trivial accessors. The exe reloads pme for each GetNext of the first loop.
void HUDMainMenu::HandleCinematicFade(Tile* th_s) {
	if (th_s->GetFloat(4022) > 0.0) {
		NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[13]->xChildren.m_pkHead;
		while (pPos) {
			Tile* pChild = HUDMainMenu::pme->pTiles[13]->xChildren.GetNext(pPos);
			if (pChild) {
				pChild->Tile::Set(4009, 0.0f, 1);
				pChild->Tile::Set(Tile::TextToTrait("_WaitComplete"), 0);
			}
		}
		pPos = th_s->xChildren.m_pkHead;
		while (pPos) {
			Tile* pChild = th_s->xChildren.GetNext(pPos);
			if (pChild && pChild->Tile::GetFloat(0x1004) > 0.0) {
				double dTime = static_cast<double>(appTimer.iLastTime);
				if (pChild->Tile::GetFloat(0x1004) < dTime) {
					pChild->Tile::SetName(pChild->Tile::GetString(4036));
					float fLength = fQuestCinematicCharacterFadeIn.Float();
					float fEnd = fHudOpacity.Float() * 255.0;
					pChild->Tile::AddFadeControl(4009, 0.0f, fEnd, fLength, Tile::TFCT_STANDARD);
					pChild->Tile::Set(0x1004, 0);
					continue;
				}
			}
			if (pChild && pChild->Tile::GetFloat(0x1005) > 0.0) {
				double dTime = static_cast<double>(appTimer.iLastTime);
				if (pChild->Tile::GetFloat(0x1005) < dTime) {
					float fStart;
					if (!(pChild->Tile::GetFloat(4009) > 175.0)) {
						fStart = 175.0f;
					} else {
						fStart = pChild->Tile::GetFloat(4009);
					}
					pChild->Tile::AddFadeControl(4009, fStart, 0.0f, fQuestCinematicCharacterFadeOut.Float(),
						Tile::TFCT_STANDARD);
					pChild->Tile::Set(0x1005, 0);
					continue;
				}
			}
			if (pChild && pChild->Tile::GetFadeEndFor(4009) == 0.0 && pChild->Tile::GetFloat(4009) == 0.0 &&
				pChild->Tile::GetFloat(0x1005) == 0.0) {
				pChild->Tile::Set(4003, 0);
			}
		}
	}
}

void HUDMainMenu::ClearGunScope() {
	NiNode* v0;
	NiNode* v1;
	NiAVObject* v2;
	NiNode* v3;
	if (HUDMainMenu::pme) {
		if ((&HUDMainMenu::spScopeModel)->m_pObject) {
			v0 = (&HUDMainMenu::spScopeModel)->m_pObject;
			if (v0->GetAtChecked(0)) {
				v3 = (&HUDMainMenu::spScopeModel)->m_pObject;
				v1 = (&HUDMainMenu::spScopeModel)->m_pObject;
				v2 = v1->GetAt(0);
				v3->DetachChild(v2);
				HUDMainMenu::pme->bScopeVisible = (bool)0;
			}
		}
	}
}

int32_t HUDMainMenu::GetMenuID() {
	return 1004;
}

HUDMainMenu::HUD_MENU_MODE HUDMainMenu::GetMenuMode() {
	if (HUDMainMenu::pme)
		return HUDMainMenu::pme->eMenuMode;
	return HMM_NONE;
}

Actor* HUDMainMenu::GetEnemyActor() {
	if (HUDMainMenu::pme)
		return HUDMainMenu::pme->pCurrentEnemy;
	return nullptr;
}

bool HUDMainMenu::IsGunScopeVisible() {
	return HUDMainMenu::pme && HUDMainMenu::pme->bScopeVisible;
}

bool HUDMainMenu::ClearLastObjectIfItIs(TESObjectREFR* apObject) {
	if (!HUDMainMenu::pme || HUDMainMenu::pme->pLastObject != apObject)
		return false;
	HUDMainMenu::pme->pLastObject = nullptr;
	return true;
}

void HUDMainMenu::RefreshInfoForRef() {
	if (HUDMainMenu::pme)
		HUDMainMenu::SetInfoForRef(HUDMainMenu::pme->pLastObject, false, true);
}

// The prompt types are GetRefPromptType's (no enum in the PDB). The exe keeps the flags of the info lines to show
// in a local: 2 the action hint, 4 the name and the door state, 0x8 to 0x80 the info labels and values. The exe's
// cleanup-only C++ EH frame (it destroys the string and item change locals on unwind) and its stack cookie check are
// left out. The furniture mark, the string locals and the item changes are built and destroyed through the exe's
// out-of-line constructors and destructors, as the exe calls them. Each "set the crosshair image" step reads the
// image name before reloading the crosshair tile, as the exe does.
bool HUDMainMenu::SetInfoForRef(TESObjectREFR* apObject, bool abTelekinesis, bool abSameOkay) {
	static_cast<void>(abTelekinesis);
	if (!HUDMainMenu::pme || !pPlayer)
		return false;
	if (apObject == pPlayer)
		apObject = nullptr;

	if (!abSameOkay && apObject == HUDMainMenu::pme->pLastObject) {
		if (bHostileUnderCrosshair && bLongDistancePick) {
			const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 2);
		} else if (!apObject || (apObject->IsActor() && !static_cast<Actor*>(apObject)->IsInCombatWithActor(pPlayer))) {
			if (!apObject) {
				const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
				HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
				HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
			}
			bHideEnemyHealthMeter = false;
			if (HUDMainMenu::GetMenuMode() == HMM_RUN_AND_GUN)
				HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(Tile::kTileValue_Visible, 1);
		}
		return false;
	}

	Actor* pActor = static_cast<Actor*>(apObject);
	bool bIsActor = apObject ? apObject->IsActor() : false;
	TESBoundObject* pBaseObject = apObject ? apObject->GetObjectReference() : nullptr;
	bool bIsTerminal = apObject ? apObject->GetObjectReference()->GetFormType() == TERM_ID : false;

	// The player actions the crime comments react to: sneaking up on an actor, and eyeing an owned locked
	// container, door or terminal.
	if (apObject && apObject != HUDMainMenu::pme->pLastObject) {
		if (bIsActor && pPlayer->IsSneaking()) {
			if (!pActor->IsPlayerTeammate()) {
				pPlayer->AddPlayerAction(PLAYER_ACTION_PICKPOCKET, 10.0f, nullptr);
				ReferenceProcessLists.ResetPlayerActionsCommentTimer();
			}
		} else if (bIsActor) {
			pPlayer->RemovePlayerAction(PLAYER_ACTION_PICKPOCKET, nullptr);
			pPlayer->RemovePlayerAction(PLAYER_ACTION_LOCKED_OBJECT, nullptr);
		} else {
			LockData* pLock = apObject->GetLock();
			if (bIsTerminal) {
				if (!static_cast<BGSTerminal*>(pBaseObject)->IsUnlocked(apObject) && apObject->GetOwner()) {
					pPlayer->AddPlayerAction(PLAYER_ACTION_LOCKED_OBJECT, 10.0f, nullptr);
					ReferenceProcessLists.ResetPlayerActionsCommentTimer();
				}
			} else if (apObject->GetObjectReference()->GetFormType() == CONT_ID && apObject->GetOwner()) {
				int32_t iCount = apObject->GetInventoryCount(false, true);
				if (iCount > 0 && pLock && pLock->IsLocked() &&
					(!pLock->pKey || pPlayer->GetInventoryObjectCount(pLock->pKey) <= 0)) {
					pPlayer->AddPlayerAction(PLAYER_ACTION_LOCKED_OBJECT, 10.0f, nullptr);
					ReferenceProcessLists.ResetPlayerActionsCommentTimer();
				}
			} else if (pLock && pLock->IsLocked() && apObject->GetOwner() &&
				(!pLock->pKey || pPlayer->GetInventoryObjectCount(pLock->pKey) <= 0)) {
				pPlayer->AddPlayerAction(PLAYER_ACTION_LOCKED_OBJECT, 10.0f, nullptr);
				ReferenceProcessLists.ResetPlayerActionsCommentTimer();
			}
		}
	} else {
		pPlayer->RemovePlayerAction(PLAYER_ACTION_PICKPOCKET, nullptr);
		pPlayer->RemovePlayerAction(PLAYER_ACTION_LOCKED_OBJECT, nullptr);
	}

	if (!apObject || !abSameOkay)
		HUDMainMenu::pme->pLastObject = apObject;
	if ((!abSameOkay && !apObject) || !apObject || (apObject->GetDestroyed() && !bIsActor)) {
		HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_SEPARATOR]->Set(Tile::kTileValue_Visible, 0);
		if (!bHostileUnderCrosshair)
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
		const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
		bHideEnemyHealthMeter = false;
		if (HUDMainMenu::GetMenuMode() == HMM_RUN_AND_GUN)
			HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(Tile::kTileValue_Visible, 1);
		return true;
	}

	int32_t iShowFlags = 0;
	int32_t ePromptType = apObject->GetRefPromptType();
	// Sitting and sleeping need a free furniture marker (the exe tests the object again only before the first).
	if ((apObject && ePromptType == 5) || ePromptType == 3) {
		uint32_t uiMarkerIndex = 0;
		// The exe destroys nothing of the mark.
		FNVD_CONSTRUCTED_LOCAL(FurnitureMark, kMark, nullptr, 0xFF);
		if (!apObject->GetClosestFreeMarker(pPlayer->GetLocationOnReference(), true, true, kMark, uiMarkerIndex, true))
			return false;
	}

	TESObjectWEAP* pWeapon = nullptr;
	TESBoundObject* pArmor = nullptr;
	TESAmmo* pAmmo = nullptr;
	bool bHostileTarget = pActor && pPlayer->IsActorPerceived(pActor) && pPlayer->IsActorPerceived(pActor)->bIsHostile;

	// The crosshair: red for a crime or a hostile target, the activate image for anything that can be used (and an
	// actor not in combat).
	if (ePromptType || bHostileTarget) {
		if (apObject->IsCrimeToActivate() && (!bIsActor || !pActor->IsPlayerTeammate())) {
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 2);
			const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User1);
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
		} else if (bHostileTarget) {
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 2);
			const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
		} else if (!bIsActor || !pActor->IsInCombat()) {
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
			const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User1);
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
		} else {
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
			const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
			HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
		}
	} else {
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
		const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
	}

	// Which info lines the base object gets; statics, trees and live projectiles get none.
	int32_t eFormType = apObject->GetObjectReference()->GetFormType();
	bool bNoInfo = false;
	switch (eFormType) {
	case ACTI_ID:
	case TACT_ID:
	case TERM_ID:
	case CONT_ID:
	case DOOR_ID:
	case FURN_ID:
	case NPC__ID:
	case CREA_ID:
		break;
	case ARMO_ID:
		pArmor = apObject->GetObjectReference();
		iShowFlags |= 0xFC;
		break;
	case LIGH_ID: {
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
		const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
		if (!static_cast<TESObjectLIGH*>(apObject->GetObjectReference())->GetCanCarry())
			return true;
		iShowFlags |= 0xFC;
		break;
	}
	case MISC_ID:
	case PWAT_ID:
	case ALCH_ID:
	case IMOD_ID:
		iShowFlags |= 0xFC;
		break;
	case STAT_ID:
	case SCOL_ID:
	case TREE_ID:
		bNoInfo = true;
		break;
	case WEAP_ID:
		pWeapon = static_cast<TESObjectWEAP*>(apObject->GetObjectReference());
		if (pWeapon->GetIsPlayable())
			iShowFlags |= 0xFC;
		break;
	case AMMO_ID:
		pAmmo = static_cast<TESAmmo*>(apObject->GetObjectReference());
		if (pAmmo->GetIsPlayable())
			iShowFlags |= 0xFC;
		break;
	case PROJ_ID: {
		// A projectile carrying an item counts as that item unless the item is a misc item.
		Projectile* pProjectile = static_cast<Projectile*>(apObject);
		if (pProjectile->GetSecondaryItemChange() &&
			pProjectile->GetSecondaryItemChange()->GetItemChangeObject()->GetFormType() != MISC_ID)
			break;
		if (apObject->GetFormType() == PMIS_ID)
			bNoInfo = true;
		else
			iShowFlags |= 0xFC;
		break;
	}
	default:
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
		break;
	}
	if (bNoInfo) {
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
		const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
		HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
		HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_SEPARATOR]->Set(Tile::kTileValue_Visible, 0);
		return true;
	}
	static_cast<void>(pArmor);

	// The door state line: broken, inaccessible, or the lock level (or the key the player has).
	DoorTeleportData* pTeleport = apObject->GetTeleport();
	LockData* pLock = apObject->GetLock();
	// The exe fetches the base object twice more into locals it never reads.
	apObject->GetObjectReference();
	apObject->GetObjectReference();
	PACKAGE_OBJECT_TYPE eMatchType = POTYPE_NONE;
	bool bCanOpen = false;
	if (eFormType != DOOR_ID && eFormType != CONT_ID) {
		bCanOpen = true;
	} else if (eFormType == DOOR_ID && TESObjectDOOR::CanActorIgnoreLock(apObject, pPlayer, false, true)) {
		bCanOpen = true;
	} else if (eFormType == CONT_ID) {
		if (!pLock || !pLock->IsLocked())
			bCanOpen = true;
		else if (pLock->pKey && pPlayer->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eMatchType))
			bCanOpen = true;
	}
	int32_t iTerminalDifficulty =
		bIsTerminal ? static_cast<BGSTerminal*>(pBaseObject)->GetHackDifficultyLockLevel(apObject) : 0;
	bool bTerminalLocked = pPlayer->GetTerminalAccess(apObject) == PlayerCharacter::TAT_LOCKED ||
		pPlayer->GetTerminalAccess(apObject) == PlayerCharacter::TAT_HACK_ELIGIBLE ||
		pPlayer->GetTerminalAccess(apObject) == PlayerCharacter::TAT_HACK_INELIGIBLE;
	if ((bIsTerminal && bTerminalLocked) || apObject->GetInaccessible() || (pLock && pLock->IsLocked())) {
		int32_t iLevel = 0;
		if (bIsTerminal)
			iLevel = iTerminalDifficulty;
		else
			iLevel = GetLockPickLevel(apObject->GetLockLevel());
		iLevel = BSMIN(BSMAX(iLevel, 0), 5);
		int32_t iLockLevel = iLevel;
		PACKAGE_OBJECT_TYPE eKeyMatchType;
		bool bHasKey = pLock && pLock->pKey && pPlayer->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eKeyMatchType);
		if (pLock && pLock->IsBroken()) {
			HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, sBroken.String(), true);
			float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
			HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
		} else if (apObject->GetInaccessible()) {
			HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, sInaccessible.String(), true);
			float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
			HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
		} else if (((eFormType == DOOR_ID || eFormType == CONT_ID) && (!bCanOpen || bHasKey)) || eFormType == TERM_ID) {
			if (bHasKey) {
				const char* pKeyText = sHUDUseKey.String();
				const char* pLocked = sLocked.String();
				BSsprintf(lockLevel, 0x104, "[%s - %s]", pLocked, pKeyText);
				HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, lockLevel, true);
			} else {
				if (iLockLevel == 5) {
					BSstrcpy(lockLevel, 0x104, "[");
					BSstrcat(lockLevel, 0x104, pLockLevelNames[iLockLevel]->String());
					BSstrcat(lockLevel, 0x104, "]");
				} else {
					const char* pLevelName = pLockLevelNames[iLockLevel]->String();
					const char* pLocked = sLocked.String();
					BSsprintf(lockLevel, 0x104, "[%s - %s]", pLocked, pLevelName);
				}
				HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, lockLevel, true);
			}
			float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
			HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
		} else {
			HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, 0);
		}
	} else {
		HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, 0);
	}

	// The container state line: shown for an empty container (or an empty ash pile) that is no live actor.
	bool bAshPileEmpty = false;
	TESObjectREFR* pAshPile = apObject->GetExtra()->GetAshPileRef();
	if (pAshPile && pAshPile->HasContainer() && pAshPile->GetInventoryCount(false, false) <= 0)
		bAshPileEmpty = true;
	TESObjectREFR* pDroppedWeapon = apObject->GetExtra()->GetDroppedWeapon();
	if (!pDroppedWeapon &&
		((apObject->HasContainer() && apObject->GetInventoryCount(false, false) <= 0) || bAshPileEmpty) &&
		(!bIsActor || apObject->IsDead(true))) {
		float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
	} else {
		HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Alpha, 0);
	}

	// The exe works out the player's heading and the heading to the enemy into locals it never reads.
	[[maybe_unused]] float fPlayerHeading = static_cast<float>(pPlayer->GetHeading(false) * 57.2957763671875);
	[[maybe_unused]] float fEnemyHeading = 0.0f;
	if (HUDMainMenu::pme->pCurrentEnemy) {
		const NiPoint3* pEnemyLocation = &HUDMainMenu::pme->pCurrentEnemy->GetLocationOnReference();
		const NiPoint3* pPlayerLocation = &pPlayer->GetLocationOnReference();
		fEnemyHeading = HUDMainMenu::HeadingBetweenLocations(*pPlayerLocation, *pEnemyLocation);
	}
	bool bIsHostileEnemy = HUDMainMenu::pme->pCurrentEnemy && !HUDMainMenu::pme->pCurrentEnemy->IsDead(false) &&
		!HUDMainMenu::pme->pCurrentEnemy->GetDestroyed() && bCurrentEnemyHostile &&
		HUDMainMenu::pme->pCurrentEnemy == pActor;

	// The name: a projectile shows the item it carries or the weapon that fired it.
	if (apObject) {
		Projectile* pProjectile = apObject->IsProjectile() ? static_cast<Projectile*>(apObject) : nullptr;
		if (pProjectile && pProjectile->GetSecondaryItemChange()) {
			TESBoundObject* pItem = pProjectile->GetSecondaryItemChange()->GetItemChangeObject();
			BSstrcpy(g_sCrosshairRefName, 0x104, TESFullName::GetFullName(pItem));
		} else if (pProjectile && pProjectile->GetWeaponSource()) {
			TESObjectWEAP* pSourceWeapon = pProjectile->GetWeaponSource();
			if (pSourceWeapon->GetProjectile(nullptr) &&
				pSourceWeapon->GetProjectile(nullptr)->GetDefaultWeaponSource())
				pSourceWeapon = pSourceWeapon->GetProjectile(nullptr)->GetDefaultWeaponSource();
			BSstrcpy(g_sCrosshairRefName, 0x104, TESFullName::GetFullName(pSourceWeapon));
		} else {
			BSstrcpy(g_sCrosshairRefName, 0x104, apObject->GetFullName());
		}
	} else {
		g_sCrosshairRefName[0] = '\0';
	}
	bool bPlayerInCombat = pPlayer->GetNumberActorsInCombat() > 0;
	if (g_sCrosshairRefName[0]) {
		if (pTeleport) {
			// A door shows where it leads.
			FNVD_CONSTRUCTED_LOCAL(BSStringT<char>, kMapName, nullptr);
			pTeleport->GetTeleportCellMapName(kMapName);
			const char* pMapNameText = kMapName.String();
			const char* pTo = sTo.String();
			char acDoorText[260];
			BSsprintf(acDoorText, 0x104, "%s %s %s", g_sCrosshairRefName, pTo, pMapNameText);
			HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, acDoorText, true);
			iShowFlags |= 4;
		} else if (!(bIsActor && (pActor == HUDMainMenu::pme->pCurrentEnemy || pActor->IsInCombatWithActor(pPlayer)) &&
					   bPlayerInCombat && !pActor->IsDead(false)) &&
			!bIsHostileEnemy) {
			// An actor fighting the player shows no name; a stack shows its count.
			bool bShowName = true;
			if (apObject->GetExtra()->GetCount() == 1) {
				if (bIsActor) {
					HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, g_sCrosshairRefName,
						true);
				} else if ((pAmmo && pAmmo->GetIsPlayable()) || (pWeapon && pWeapon->GetIsPlayable()) ||
					(!pWeapon && !pAmmo)) {
					if (apObject->GetObjectReference()->GetFormType() == AMMO_ID &&
						static_cast<TESAmmo*>(apObject->GetObjectReference())->GetFormClipRounds() > 1) {
						int32_t iCount = apObject->GetCount();
						char acCountText[260];
						BSsprintf(acCountText, 0x104, "%s (%d)", g_sCrosshairRefName, iCount);
						HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, acCountText,
							true);
					} else {
						HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String,
							g_sCrosshairRefName, true);
					}
				} else {
					bShowName = false;
				}
			} else {
				char acStackText[260];
				BSsprintf(acStackText, 0x104, "%s (%d)", g_sCrosshairRefName,
					static_cast<int32_t>(apObject->GetExtra()->GetCount()));
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, acStackText, true);
			}
			HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 0);
			HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 0);
			HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 0);
			if (bShowName)
				iShowFlags |= 4;
		}
	} else {
		HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, " ", true);
	}

	// Whether the compass tracks the actor as hostile.
	int32_t eRefPromptType = apObject->GetRefPromptType();
	bool bCompassHostile = false;
	if (bIsActor) {
		for (BSSimpleList<CompassTarget*>* pIter = pPlayer->GetListofPerceivedActors(); pIter && !pIter->IsEmpty();
			pIter = pIter->GetNext()) {
			CompassTarget* pTarget = pIter->GetItem();
			if (pTarget->pTarget == pActor && pTarget->bIsHostile) {
				bCompassHostile = true;
				break;
			}
		}
	}

	// The action hint: the activate text override or prompt, else the prompt for the reference's prompt type. An
	// actor fighting the player (or one the compass shows as hostile) gets none.
	if (apObject && !bIsHostileEnemy &&
		!(eRefPromptType == 7 && bIsActor &&
			(bCompassHostile || pActor == HUDMainMenu::pme->pCurrentEnemy || pActor->IsInCombatWithActor(pPlayer)) &&
			(pActor->IsInCombat(false) || pActor->IsAttacking() || bCompassHostile)) &&
		!apObject->GetDestroyed()) {
		auto* pActivateRef = static_cast<ExtraActivateRef*>(apObject->GetExtraData(EXTRA_ACTIVATE_REF));
		FNVD_CONSTRUCTED_LOCAL(BSStringT<char>, kPrompt, nullptr, "");
		BSStringT<char>* pPrompt = &kPrompt;
		if (pActivateRef)
			pPrompt->Copy(pActivateRef->ActivateTextOverride);
		if (!pPrompt->GetLength()) {
			auto* pActivator = dynamic_cast<TESObjectACTI*>(apObject->GetObjectReference());
			if (pActivator) {
				// GetActivationString constructs its result in the local (the hidden result pointer).
				FNVD_INLINED_LOCAL(BSStringT<char>, kActivation, std::nullopt, std::nullopt);
				BSStringT<char>* pActivation = pActivator->GetActivationString(&kActivation);
				pPrompt->Copy(*pActivation);
				kActivation.~BSStringT<char>();
			}
		}
		if (pPrompt->GetLength()) {
			HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, pPrompt->String(), true);
			HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, "", true);
			iShowFlags |= 2;
		} else if (eRefPromptType == 1 && apObject->IsCrimeToActivate()) {
			// Taking from someone: talk to a teammate, pickpocket an NPC (or a creature that allows dialogue),
			// steal anything else.
			if (bIsActor && pActor->IsPlayerTeammate()) {
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String,
					psTargetTypes[7]->String(), true);
			} else if (eFormType == NPC__ID) {
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sPickpocket.String(),
					true);
			} else if (eFormType == CREA_ID) {
				TESActorBase* pActorBase = pActor->GetActorBase();
				if (pActorBase->GetAllowPCDialogue())
					HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sPickpocket.String(),
						true);
				else
					HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sSteal.String(),
						true);
			} else {
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sSteal.String(), true);
			}
			iShowFlags |= 2;
		} else if (apObject->GetObjectReference() == pAshPile1 || apObject->GetObjectReference() == pAshPile2 ||
			(eRefPromptType == 2 && (eFormType == NPC__ID || eFormType == CREA_ID))) {
			HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sSearch.String(), true);
			iShowFlags |= 2;
		} else if (eRefPromptType == 8 && eFormType == DOOR_ID && !apObject->GetInaccessible()) {
			// A door: pick its lock, or open or close it.
			bool bOpen = BGSOpenCloseForm::GetOpenState(apObject) == BGSOpenCloseForm::OBJECT_OPEN ||
				BGSOpenCloseForm::GetOpenState(apObject) == BGSOpenCloseForm::OBJECT_OPENING;
			if (pLock && pLock->IsLocked() && !apObject->IsAnOwner(pPlayer, true) && !bCanOpen) {
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sPick.String(), true);
			} else {
				const char* pHint = bOpen ? sCloseButton.String() : psTargetTypes[eRefPromptType]->String();
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, pHint, true);
			}
			iShowFlags |= 2;
		} else if (eRefPromptType == 4 && eFormType == CREA_ID) {
			iShowFlags &= ~3;
		} else if (eRefPromptType == 6) {
			HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, psTargetTypes[1]->String(),
				true);
			iShowFlags |= 2;
		} else if (eRefPromptType && eRefPromptType < 20 && !apObject->GetInaccessible()) {
			// No prompt to talk to an essential actor while sneaking.
			if (eRefPromptType != 1 || !bIsActor || !pPlayer->IsSneaking() || !pActor->GetEssential()) {
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String,
					psTargetTypes[eRefPromptType]->String(), true);
				iShowFlags |= 2;
			}
		} else if (apObject->IsProjectile()) {
			// A mine (a projectile with a proximity trigger): disarm it, or take it once it is turned off. The exe
			// reads the base through Actor::GetActorBase's out-of-line getter (0x4181E0) on the projectile.
			auto* pProjectileBase = static_cast<BGSProjectile*>(static_cast<TESForm*>(pActor->GetActorBase()));
			if (pProjectileBase->GetExplosionProximity() != 0.0) {
				if (!static_cast<Projectile*>(apObject)->IsFlagSet(Projectile::TURNED_OFF))
					HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sDisarmMine.String(),
						true);
				else
					HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String,
						psTargetTypes[1]->String(), true);
				iShowFlags |= 2;
				HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
				const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User1);
				HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
			}
		}
		float fLineAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
		HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_line_alpha"), fLineAlpha, true);

		// The activate button: the gamepad button's icon, or the key's name.
		if (!InterfaceManager::Has360Controller()) {
			uint8_t ucKey = appMain->GetControls()->RemapAction(Controls::UACTN_ACTIVATE, 0);
			if (ucKey < 0xEE && psKeyboard[ucKey]) {
				char acButtonText[32];
				BSstrcpy(acButtonText, 0x20, psKeyboard[ucKey]->String());
				BSstrcat(acButtonText, 0x20, ")");
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_PCButtonText"), acButtonText,
					true);
			} else {
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_PCButtonText"), "", true);
			}
		} else {
			uint8_t ucButton = appMain->GetControls()->RemapAction(Controls::UACTN_ACTIVATE, 3);
			switch (ucButton) {
			case 8:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 17);
				break;
			case 9:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 18);
				break;
			case 10:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 9);
				break;
			case 11:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 10);
				break;
			case 12:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 11);
				break;
			case 13:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 12);
				break;
			case 14:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 16);
				break;
			case 15:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 15);
				break;
			case 16:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 13);
				break;
			case 17:
				HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_xbox_button"), 14);
				break;
			default:
				break;
			}
		}
		// kPrompt is a C++ local, destroyed where its scope ends.
		FNVD_DESTRUCT_IN_PLACE(BSStringT<char>)(pPrompt);
	}

	// The item the info lines describe: a projectile's carried item, else a change for the thrown weapon or the
	// base object (a temporary the exe destroys once it is copied), with the reference's extra data as its instance.
	Projectile* pProjectile = apObject->IsProjectile() ? static_cast<Projectile*>(apObject) : nullptr;
	bool bHasSecondaryItem = pProjectile && pProjectile->GetSecondaryItemChange();
	TESObjectWEAP* pSourceWeapon = pProjectile ? pProjectile->GetWeaponSource() : nullptr;
	TESObjectWEAP* pThrownWeapon = pSourceWeapon && pSourceWeapon->IsThrownWeapon() ? pSourceWeapon : nullptr;
	ItemChange kTempItem;
	bool bTempItemBuilt = false;
	ItemChange* pSourceItem;
	if (bHasSecondaryItem) {
		pSourceItem = pProjectile->GetSecondaryItemChange();
	} else {
		TESBoundObject* pItemObject = pThrownWeapon ? pThrownWeapon : apObject->GetObjectReference();
		pSourceItem = FNVD_RECONSTRUCT(ItemChange)(&kTempItem, pItemObject, 0);
		bTempItemBuilt = true;
	}
	ItemChange kItem;
	kItem.~ItemChange();
	::new (&kItem) ItemChange(pSourceItem);
	if (bTempItemBuilt)
		FNVD_DESTRUCT_IN_PLACE(ItemChange)(&kTempItem);
	if (!kItem.pExtraObjectList) {
		void* pMemory = ourMalloc(sizeof(BSSimpleList<ExtraDataList*>));
		kItem.pExtraObjectList = pMemory ? ::new (pMemory) BSSimpleList<ExtraDataList*> : nullptr;
	}
	ExtraDataList* pExtra = apObject->GetExtra();
	kItem.pExtraObjectList->AddHead(pExtra);

	// Water shows its health and radiation; anything else its weight and value.
	TESWaterForm* pWater = nullptr;
	int32_t eBaseFormType = apObject->GetObjectReference()->GetFormType();
	if (eBaseFormType == ACTI_ID)
		pWater = apObject->GetObjectReference()->GetWaterType();
	else if (eBaseFormType == PWAT_ID)
		pWater = apObject->GetObjectReference()->GetWaterType();
	if (pWater) {
		int32_t iRadiation = pWater->GetRadiationDamage();
		int32_t iHealth = pWater->GetHealthRestoration();
		int32_t iShown = 0;
		int32_t iInfoLabel = HMMT_INFO_LABEL_ONE;
		int32_t iInfoValue = HMMT_INFO_VALUE_ONE;
		HUDMainMenu::DisplayWaterHealth(iHealth, iInfoLabel, iInfoValue);
		iInfoLabel = iInfoLabel == HMMT_INFO_LABEL_ONE ? HMMT_INFO_LABEL_TWO : iInfoLabel;
		iInfoValue = iInfoValue == HMMT_INFO_VALUE_ONE ? HMMT_INFO_VALUE_TWO : iInfoValue;
		++iShown;
		HUDMainMenu::DisplayWaterRadiation(iRadiation, iInfoLabel, iInfoValue);
		iInfoLabel = iInfoLabel == HMMT_INFO_LABEL_ONE ? HMMT_INFO_LABEL_TWO : iInfoLabel;
		iInfoValue = iInfoValue == HMMT_INFO_VALUE_ONE ? HMMT_INFO_VALUE_TWO : iInfoValue;
		++iShown;
		static_cast<void>(iShown);
		iShowFlags |= 0xFC;
	} else {
		// Both texts take "--" for zero, negative and NaN amounts.
		float fValue = kItem.GetItemValue();
		if (fValue > 0.0) {
			if (fValue < 1.0)
				BSsprintf(caps, 8, "%.1f", fValue);
			else
				BSsprintf(caps, 8, "%.0f", fValue);
		} else {
			BSstrcpy(caps, 8, "--");
		}
		float fWeight = 0.0f;
		if (kItem.GetItemChangeObject()->GetFormType() == WEAP_ID) {
			auto* pItemWeapon = static_cast<TESObjectWEAP*>(kItem.GetItemChangeObject());
			fWeight = pItemWeapon->GetFormWeight(kItem.HasModEffectActive(WEAPON_MOD_DECREASE_WEIGHT));
			if (fWeight >= 10.0) {
				// The heavy weapon weight perk entry scales weapons of 10 or more.
				float fMult = 1.0f;
				BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::NOT_YET_IMPLEMENTED, pPlayer, &fMult);
				fWeight = fWeight * fMult;
			}
		} else {
			bool bHardcore = pPlayer->IsHardcore();
			fWeight = static_cast<float>(TESWeightForm::GetFormWeight(kItem.GetItemChangeObject(), bHardcore));
		}
		if (fWeight > 0.0)
			BSsprintf(wg, 8, "%.1f", fWeight);
		else
			BSstrcpy(wg, 8, "--");
		HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(Tile::kTileValue_String, wg, true);
		HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(Tile::kTileValue_String, sInventoryWeightUpper.String(),
			true);
		HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(Tile::kTileValue_String, caps, true);
		HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(Tile::kTileValue_String, sInventoryValue.String(), true);
	}

	static Tile* phintButtonText = Tile::GetTileByName(HUDMainMenu::pme->pTiles[HMMT_INFO], "button_text");

	// The info lines are red for a crime or a hostile actor.
	int32_t iInfoColor;
	if ((apObject->IsCrimeToActivate() && (!bIsActor || !pActor->IsPlayerTeammate())) ||
		(bIsActor && (pActor->IsInCombatWithActor(pPlayer) || bCompassHostile)))
		iInfoColor = 2;
	else
		iInfoColor = 1;
	for (int32_t iTile = HMMT_INFO_ACTION_HINT; iTile <= HMMT_INFO_SEPARATOR; ++iTile)
		HUDMainMenu::pme->pTiles[iTile]->Set(Tile::kTileValue_SystemColor, iInfoColor);
	phintButtonText->AddNeedsUpdate(Tile::CHANGE_COLOR);

	// Lays the hint out above the name, whose height depends on its wrapped text, and the subtitles above both.
	float fNameOffset = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(Tile::kTileValue_User8);
	float fWrapWidth = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(Tile::kTileValue_WrapWidth);
	int32_t iFont =
		static_cast<int32_t>(HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(Tile::kTileValue_Font));
	const char* pNameText = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetString(Tile::kTileValue_String);
	NiPoint3 kDimensions;
	FontManager::Instance()->CalculateStringDimensions(&kDimensions, pNameText, iFont, fWrapWidth, 0);
	float fNameHeight = kDimensions.y;
	float fHintBase = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->GetFloat(Tile::kTileValue_User0);
	float fHintY =
		static_cast<float>(static_cast<double>(fHintBase) - (static_cast<double>(fNameHeight) - fNameOffset));
	HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_y"), fHintY, true);
	float fNameY = static_cast<float>(static_cast<double>(HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
											  ->GetFloat(Tile::kTileValue_Height)) +
		fHintY);
	HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_Y, fNameY, true);
	double dInfoY = static_cast<double>(HUDMainMenu::pme->pTiles[HMMT_INFO]->GetFloat(Tile::kTileValue_Y)) + fHintY;
	float fSubtitleY =
		static_cast<float>(dInfoY - HUDMainMenu::pme->pTiles[HMMT_SUBTITLE]->GetFloat(Tile::kTileValue_Height) - 50.0);
	HUDMainMenu::pme->pTiles[HMMT_SUBTITLE]->Set(Tile::kTileValue_Y, fSubtitleY, true);

	HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_Visible, iShowFlags & 2);
	HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_Visible,
		static_cast<int32_t>((iShowFlags & 4) > 0));
	if (iShowFlags & 6) {
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 0);
	}
	HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Visible,
		static_cast<int32_t>((iShowFlags & 4) > 0));
	// The container state shows with the name while the door state is hidden (alpha 0).
	int32_t iContainerVisible;
	if ((iShowFlags & 4) > 0 && HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->GetFloat(Tile::kTileValue_Alpha) == 0.0)
		iContainerVisible = 1;
	else
		iContainerVisible = 0;
	HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Visible, iContainerVisible);
	HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(Tile::kTileValue_Visible,
		static_cast<int32_t>((iShowFlags & 8) > 0));
	HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(Tile::kTileValue_Visible,
		static_cast<int32_t>((iShowFlags & 0x10) > 0));
	HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(Tile::kTileValue_Visible,
		static_cast<int32_t>((iShowFlags & 0x20) > 0));
	HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(Tile::kTileValue_Visible,
		static_cast<int32_t>((iShowFlags & 0x40) > 0));
	HUDMainMenu::pme->pTiles[HMMT_INFO_SEPARATOR]->Set(Tile::kTileValue_Visible,
		static_cast<int32_t>((iShowFlags & 0x80) > 0));
	return true;
}

void HUDMainMenu::DisplayWaterHealth(int32_t aiHealth, int32_t aiInfoLabel, int32_t aiInfoValue) {
	char Src[268];
	std::sprintf(Src, "%i", aiHealth);
	HUDMainMenu::pme->pTiles[aiInfoLabel]->Set(4036, Src, static_cast<bool>(1));
	const char* v3 = sHitPointsShort.String();
	HUDMainMenu::pme->pTiles[aiInfoValue]->Set(4036, v3, static_cast<bool>(1));
}

void HUDMainMenu::DisplayWaterRadiation(int32_t aiRadiationAmount, int32_t aiInfoLabel, int32_t aiInfoValue) {
	int32_t v4;
	char Src[264];
	float feffectivenessMult;
	float fresistance = pPlayer->GetRadiationResistanceMult();
	feffectivenessMult = 1.0f;
	BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_DRINKING_RADIATION, pPlayer, &feffectivenessMult);
	fresistance = fresistance * feffectivenessMult;
	float fmodifiedMagnitude = static_cast<double>(aiRadiationAmount) * fresistance;
	if (!(fmodifiedMagnitude > 0.0) || !(fmodifiedMagnitude < 1.0)) {
		v4 = static_cast<int32_t>(fmodifiedMagnitude);
	} else {
		v4 = 1;
	}
	std::sprintf(Src, "%i", v4);
	HUDMainMenu::pme->pTiles[aiInfoLabel]->Set(4036, Src, static_cast<bool>(1));
	const char* v3 = sHUDRads.String();
	HUDMainMenu::pme->pTiles[aiInfoValue]->Set(4036, v3, static_cast<bool>(1));
}

void HUDMainMenu::SetBrightness(Tile* apTile, HUD_BRIGHTNESS aeBrightness) {
	switch (aeBrightness) {
	case HB_HUD_BRACKETS:
	case HB_HUD_METERS:
		apTile->Set(Tile::kTileValue_Brightness, 0xAF);
		break;
	case HB_HUD_BRACKETS_GLOW:
		apTile->Set(Tile::kTileValue_Brightness, 0xFF);
		break;
	case HB_HUD_METER_BACKGROUND:
		// HB_HUD_METER_BACKGROUND enters the jump table at 0x77484D and runs into HB_HUD_METERS_GHOST's store
		// (0x77485C).
		apTile->Set(Tile::kTileValue_Brightness, 0x2C);
		[[fallthrough]];
	case HB_HUD_METERS_GHOST:
		apTile->Set(Tile::kTileValue_Brightness, 0x64);
		break;
	default:
		// The exe computes 0xAF + 0x50 from a constant true flag.
		apTile->Set(Tile::kTileValue_Brightness, 0xFF);
		break;
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HUDMainMenu::SetRadiationLevel(float afDrinkinRads, float afStandinRads) {
	FNVD_INLINED_LOCAL(BSSoundHandle, a1, std::nullopt, std::nullopt);
	float ftotalRads = afDrinkinRads + afStandinRads;
	if (HUDMainMenu::pme) {
		if (!(ftotalRads > 0.0)) {
			if (HUDMainMenu::pme->GeigerSound.IsValid()) {
				HUDMainMenu::pme->GeigerSound.FadeOutAndRelease(500u);
			}
		} else {
			if (!HUDMainMenu::pme->GeigerSound.IsValid() && afStandinRads > 0.0) {
				BSAudio* v2 = &BSAudio::QInstance();
				BSSoundHandle* arHandle = v2->GetSoundHandleByName(&a1, "UIPipBoyRadiationTestLP", 17u);
				HUDMainMenu::pme->GeigerSound.operator=(*arHandle);
				HUDMainMenu::pme->GeigerSound.Play(0);
			}
			HUDMainMenu::pme->UpdateRadiationMeterConditions(afDrinkinRads, afStandinRads);
		}
	}
}

// The exe gets the HUD's menu id from the out-of-line GetMenuID (0x706F20, which returns 1004) for the first lookup
// and the class check; the later lookups push 1004 directly. It calls NiPointer's constructor, operator->, operator=
// and destructor (0x633C90, 0x559450, 0x66B0D0, 0x45CEC0) out of line; src's inline copies have the same effects.
// The exe reads the bound's radius through the trivial accessor BSTimer::GetSeconds (0x84D030); src reads m_fRadius.
// Every computed position is rounded to a float before it is set, as the exe does; the sums of several traits are
// kept as doubles in between.
Tile* HUDMainMenu::Create() {
	using enum Tile::eTileValue;

	if (!InterfaceManager::Instance())
		return nullptr;
	TileMenu* pOldMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(GetMenuID()));
	if (pOldMenu)
		delete pOldMenu;
	InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
	const float fMaxDepth = Tile::GetMaximumDepth();
	Tile::GetTileUpdateAllCriticalSectionLock()->Enter(0);
	Tile* pFile = pInterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\Main\\hud_main_menu.xml");
	Menu* pMenu = pFile->GetMenu();
	if (!pMenu || pMenu->GetClass() != GetMenuID()) {
		if (pMenu && pMenu->GetMenuTile())
			delete pMenu;
		Tile::GetTileUpdateAllCriticalSectionLock()->Leave();
		return nullptr;
	}
	pMenu->SetMenuTile(static_cast<TileMenu*>(pFile), false);
	pme = static_cast<HUDMainMenu*>(pMenu);
	const int32_t iScreenWidth = static_cast<int32_t>(InterfaceManager::GetScreenWidth());
	const int32_t iScreenHeight = static_cast<int32_t>(InterfaceManager::GetScreenHeight());
	const int32_t iSafeX = static_cast<int32_t>(InterfaceManager::GetScreenSafeX());
	const int32_t iSafeY = static_cast<int32_t>(InterfaceManager::GetScreenSafeY());
	const int32_t iOriginalX = Tile::AddUserTrait("_OriginalX", -1);
	const int32_t iTotalWidth = Tile::AddUserTrait("_TotalWidth", -1);
	const int32_t iImageWidth = Tile::AddUserTrait("_ImageWidth", -1);
	float fValue;
	double dSum;
	double dSum2;
	double dSum3;

	// Action points, ammo and weapon condition, at the bottom right.
	pme->pTiles[HMMT_AP] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "ActionPoints");
	fValue = static_cast<float>(static_cast<double>(iScreenWidth - iSafeX * 2) -
		pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Width) + 30.0);
	pme->pTiles[HMMT_AP]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(static_cast<double>(iScreenHeight - iSafeY * 2) -
		pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Height));
	pme->pTiles[HMMT_AP]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_AP]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_RIGHT_BRACKET] = pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_right_bracket", nullptr);
	SetBrightness(pme->pTiles[HMMT_RIGHT_BRACKET], HB_HUD_BRACKETS_GLOW);
	pme->pTiles[HMMT_RIGHT_BRACKET]->Set(kTileValue_X, -37);
	pme->pTiles[HMMT_ACTION_POINTS_BAR] = pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_meter", nullptr);
	pme->pTiles[HMMT_ACTION_POINTS_BAR]->Set(iOriginalX, 326);
	pme->pTiles[HMMT_ACTION_POINTS_BAR]->Set(kTileValue_Y, 41.0f, true);
	SetBrightness(pme->pTiles[HMMT_ACTION_POINTS_BAR], HB_HUD_METERS);
	pme->pTiles[HMMT_ACTION_POINTS_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_justify_right_text", nullptr);
	pme->pTiles[HMMT_ACTION_POINTS_TEXT]->Set(kTileValue_X, 315);
	pme->pTiles[HMMT_ACTION_POINTS_TEXT]->Set(kTileValue_Y, 8);
	SetBrightness(pme->pTiles[HMMT_ACTION_POINTS_TEXT], HB_HUD_TEXT);
	pme->pTiles[HMMT_AMMO_TEXT] = pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_justify_right_text", nullptr);
	pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_X, 315);
	pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_Y, 80);
	pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_Font, 7);
	SetBrightness(pme->pTiles[HMMT_AMMO_TEXT], HB_HUD_TEXT);
	pme->pTiles[HMMT_WEAPON_CONDITION_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_justify_right_text", nullptr);
	pme->pTiles[HMMT_WEAPON_CONDITION_TEXT]->Set(kTileValue_X, 80);
	fValue = pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_Y);
	pme->pTiles[HMMT_WEAPON_CONDITION_TEXT]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_WEAPON_CONDITION_TEXT]->Set(kTileValue_Font, 7);
	pme->pTiles[HMMT_WEAPON_CONDITION_TEXT]->Set(kTileValue_String, sInventoryCondition.String(), true);
	SetBrightness(pme->pTiles[HMMT_WEAPON_CONDITION_TEXT], HB_HUD_TEXT);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER] = pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_meter", nullptr);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(iOriginalX, 90);
	fValue = static_cast<float>(pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_Y) + 3.0);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(kTileValue_Filename, "Interface\\VATS\\vats_bar.dds", true);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(iTotalWidth, 60);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(iImageWidth, 1);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(kTileValue_Zoom, 200);
	SetBrightness(pme->pTiles[HMMT_WEAPON_CONDITION_METER], HB_HUD_METERS);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_meter_background", nullptr);
	fValue = static_cast<float>(pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_Y) + 3.0);
	pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND]->Set(kTileValue_Y, fValue, true);
	SetBrightness(pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND], HB_HUD_METER_BACKGROUND);
	pme->pTiles[HMMT_CND_ARROWS] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "CNDArrows");
	dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_X);
	dSum = pme->pTiles[HMMT_WEAPON_CONDITION_TEXT]->GetFloat(kTileValue_X) + dSum;
	dSum = pme->pTiles[HMMT_WEAPON_CONDITION_METER]->GetFloat(kTileValue_X) + dSum;
	fValue = static_cast<float>(pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND]->GetFloat(kTileValue_Width) * 0.75 +
		dSum + 8.0);
	pme->pTiles[HMMT_CND_ARROWS]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Y);
	fValue = static_cast<float>(pme->pTiles[HMMT_WEAPON_CONDITION_METER]->GetFloat(kTileValue_Y) + dSum);
	pme->pTiles[HMMT_CND_ARROWS]->Set(kTileValue_Y, fValue, true);
	fValue = pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND]->GetFloat(kTileValue_Height);
	pme->pTiles[HMMT_CND_ARROWS]->Set(kTileValue_Height, fValue, true);
	pme->pTiles[HMMT_CND_ARROWS]->Set(kTileValue_Visible, 0);

	// Hit points and the compass, at the bottom left.
	pme->pTiles[HMMT_HP] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "HitPoints");
	pme->pTiles[HMMT_HP]->Set(kTileValue_X, iSafeX * 2 + 10);
	fValue = static_cast<float>(static_cast<double>(iScreenHeight - iSafeY * 2) -
		pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Height));
	pme->pTiles[HMMT_HP]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_HP]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_LEFT_BRACKET] = pMenu->RenderTemplate(pme->pTiles[HMMT_HP], "template_left_bracket", nullptr);
	SetBrightness(pme->pTiles[HMMT_LEFT_BRACKET], HB_HUD_BRACKETS_GLOW);
	pme->pTiles[HMMT_HEALTH_BAR] = pMenu->RenderTemplate(pme->pTiles[HMMT_HP], "template_meter", nullptr);
	pme->pTiles[HMMT_HEALTH_BAR]->Set(iOriginalX, 20);
	pme->pTiles[HMMT_HEALTH_BAR]->Set(kTileValue_Y, 41.0f, true);
	SetBrightness(pme->pTiles[HMMT_HEALTH_BAR], HB_HUD_METERS);
	pme->pTiles[HMMT_HIT_POINT_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_HP], "template_justify_right_text", nullptr);
	pme->pTiles[HMMT_HIT_POINT_TEXT]->Set(kTileValue_X, 53);
	pme->pTiles[HMMT_HIT_POINT_TEXT]->Set(kTileValue_Y, 8);
	SetBrightness(pme->pTiles[HMMT_HIT_POINT_TEXT], HB_HUD_TEXT);
	pme->pTiles[HMMT_COMPASS] = pMenu->RenderTemplate(pme->pTiles[HMMT_HP], "template_compass_window", nullptr);
	pme->pTiles[HMMT_COMPASS_GENERAL_ICONS] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_COMPASS], "template_compass_icon_group", nullptr);
	pme->pTiles[HMMT_COMPASS_GENERAL_ICONS]->Set(kTileValue_Y, 30);
	pme->pTiles[HMMT_COMPASS_GENERAL_ICONS]->Set(kTileValue_Depth, 4);
	pme->pTiles[HMMT_COMPASS_MARKER_ICONS] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_COMPASS], "template_compass_icon_group", nullptr);
	pme->pTiles[HMMT_COMPASS_MARKER_ICONS]->Set(kTileValue_Y, 30);
	pme->pTiles[HMMT_COMPASS_MARKER_ICONS]->Set(kTileValue_Depth, 3);
	pme->pTiles[HMMT_COMPASS_NPC_ICONS] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_COMPASS], "template_compass_icon_group", nullptr);
	pme->pTiles[HMMT_COMPASS_NPC_ICONS]->Set(kTileValue_Y, 10);
	pme->pTiles[HMMT_COMPASS_NPC_ICONS]->Set(kTileValue_Depth, 2);
	for (int32_t i = 0; i < static_cast<int32_t>(iHUDMaxCompassNPCTicks.UInt()); ++i)
		pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_NPC_ICONS], "compass_npc_icon", nullptr);
	for (int32_t i = 0; i < 10; ++i)
		pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_MARKER_ICONS], "template_compass_icon_quest", nullptr);
	pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_MARKER_ICONS], "template_compass_icon_player", nullptr);
	for (int32_t i = 0; i < 10; ++i)
		pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_GENERAL_ICONS], "template_compass_icon_marker", nullptr);

	// The radiation meter, at the top right.
	pme->pTiles[HMMT_RADIATION] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "RadiationMeter");
	fValue = static_cast<float>(static_cast<double>(iScreenWidth - iSafeX * 2) -
		pme->pTiles[HMMT_RADIATION]->GetFloat(kTileValue_Width) - 20.0);
	pme->pTiles[HMMT_RADIATION]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_RADIATION]->Set(kTileValue_Y, iSafeY * 2);
	pme->pTiles[HMMT_RADIATION_BRACKET] = Tile::GetTileByName(pme->pTiles[HMMT_RADIATION], "radiation_bracket");
	pme->pTiles[HMMT_RADIATION_POINTER] = Tile::GetTileByName(pme->pTiles[HMMT_RADIATION], "radiation_pointer");
	pme->pTiles[HMMT_RADIATION_TEXT] = Tile::GetTileByName(pme->pTiles[HMMT_RADIATION], "radiation_text");
	pme->pTiles[HMMT_RADIATION_TEXT_VALUE] = Tile::GetTileByName(pme->pTiles[HMMT_RADIATION], "radiation_text_value");
	pme->pTiles[HMMT_RADIATION_TEXT_VALUE]->Set(kTileValue_String, "+0", true);
	pme->pTiles[HMMT_RADIATION_TEXT]->Set(kTileValue_String, sRadsPerSecond.String(), true);

	// The enemy health meter, below the top middle.
	pme->pTiles[HMMT_ENEMY_HEALTH] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "EnemyHealth");
	fValue = static_cast<float>(static_cast<double>(iScreenWidth / 2) -
		pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Y) + 40.0);
	pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_SystemColor, 2);
	pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_ENEMY_HEALTH], "template_enemy_health_bracket", nullptr);
	SetBrightness(pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET], HB_HUD_BRACKETS);
	pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_ENEMY_HEALTH_METER] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_ENEMY_HEALTH], "template_meter", nullptr);
	pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(iOriginalX, 171);
	pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(iTotalWidth, 240);
	pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(kTileValue_Y, 3);
	SetBrightness(pme->pTiles[HMMT_ENEMY_HEALTH_METER], HB_HUD_METERS);
	pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_ENEMY_HEALTH_NAME] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_ENEMY_HEALTH], "template_justify_center_text", nullptr);
	pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(kTileValue_X, 170);
	pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(kTileValue_Y, 35);
	pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(kTileValue_WrapWidth, 350);
	SetBrightness(pme->pTiles[HMMT_ENEMY_HEALTH_NAME], HB_HUD_TEXT);

	// The quest reminder, at the top left.
	const int32_t iQuestReminderY = iSafeY * 2 + 170;
	pme->pTiles[HMMT_QUEST_UPDATE] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "QuestReminder");
	pme->pTiles[HMMT_QUEST_UPDATE]->Set(kTileValue_X, iSafeX * 2 + 15);
	pme->pTiles[HMMT_QUEST_UPDATE]->Set(kTileValue_Y, iQuestReminderY);
	pme->pTiles[HMMT_QUEST_STAGES] = Tile::GetTileByName(pme->pTiles[HMMT_QUEST_UPDATE], "QuestStages");
	pme->pTiles[HMMT_QUEST_ADDED] = Tile::GetTileByName(pme->pTiles[HMMT_QUEST_UPDATE], "QuestAdded");
	for (int32_t i = 0; i < 64; ++i)
		pme->RenderTemplate(pme->pTiles[HMMT_QUEST_ADDED], "template_justify_left_text", nullptr);
	for (int32_t i = 0; i < 32; ++i)
		pme->RenderTemplate(pme->pTiles[HMMT_QUEST_STAGES], "template_filled_checkbox", nullptr);
	for (int32_t i = 0; i < 32; ++i)
		pme->RenderTemplate(pme->pTiles[HMMT_QUEST_STAGES], "template_justify_left_text", nullptr);

	// The region and location names, above the hit points.
	pme->pTiles[HMMT_REGION_LOCATION] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Region_Location");
	pme->pTiles[HMMT_REGION_LOCATION]->Set(kTileValue_X, iSafeX * 2 + 15);
	fValue = static_cast<float>(pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Y) - 75.0);
	pme->pTiles[HMMT_REGION_LOCATION]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_REGION_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_REGION_LOCATION], "template_justify_left_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_REGION_TEXT], HB_HUD_TEXT);
	pme->pTiles[HMMT_REGION_TEXT]->Set(kTileValue_Visible, 1);
	NiPoint3 kTextSize;
	float fWrapWidth = pme->pTiles[HMMT_REGION_TEXT]->GetFloat(kTileValue_WrapWidth);
	int32_t iFont = static_cast<int32_t>(pme->pTiles[HMMT_REGION_TEXT]->GetFloat(kTileValue_Font));
	FontManager::Instance()->CalculateStringDimensions(&kTextSize, "Test Height", iFont, fWrapWidth, 0);
	pme->pTiles[HMMT_REGION_TEXT]->Set(kTileValue_Y, kTextSize.y, true);
	pme->pTiles[HMMT_LOCATION_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_REGION_LOCATION], "template_justify_left_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_LOCATION_TEXT], HB_HUD_TEXT);
	pme->pTiles[HMMT_LOCATION_TEXT]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_LOCATION_TEXT]->Set(kTileValue_Y, kTextSize.y, true);

	// The crosshair, centred on the screen.
	pme->pTiles[HMMT_CROSSHAIR] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "ReticleCenter");
	fValue = static_cast<float>(static_cast<double>(iScreenWidth / 2) -
		pme->pTiles[HMMT_CROSSHAIR]->GetFloat(kTileValue_Width) / 2.0 - 3.0);
	pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(static_cast<double>(iScreenHeight / 2) -
		pme->pTiles[HMMT_CROSSHAIR]->GetFloat(kTileValue_Height) / 2.0 - 3.0);
	pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_CROSSHAIR_CENTER] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_CROSSHAIR], "template_reticle_center", nullptr);
	pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(kTileValue_SystemColor, 1);
	pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(kTileValue_Visible, 1);

	// The sneak meter, at the top middle.
	pme->pTiles[HMMT_SNEAK] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "SneakMeter");
	pme->pTiles[HMMT_SNEAK]->Set(kTileValue_X, iScreenWidth / 2);
	pme->pTiles[HMMT_SNEAK]->Set(kTileValue_Y, iSafeY * 2);
	pme->pTiles[HMMT_SNEAK]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_SNEAK]->Set(kTileValue_Visible, 0);
	pme->pTiles[HMMT_SNEAK_TEXT] = Tile::GetTileByName(pme->pTiles[HMMT_SNEAK], "sneak_nif");
	pme->pTiles[HMMT_SNEAK_TEXT]->Set(kTileValue_Alpha, 0);

	// The messages, at the top left.
	pme->pTiles[HMMT_MESSAGE] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Messages");
	const int32_t iMessageOffsetX = -5;
	pme->pTiles[HMMT_MESSAGE]->Set(kTileValue_X, iMessageOffsetX + iSafeX * 2);
	pme->pTiles[HMMT_MESSAGE]->Set(kTileValue_Y, iSafeY * 2 + 25);
	pme->pTiles[HMMT_MESSAGE]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_MESSAGE_ICON] = pMenu->RenderTemplate(pme->pTiles[HMMT_MESSAGE], "template_message_icon", nullptr);
	SetBrightness(pme->pTiles[HMMT_MESSAGE_ICON], HB_HUD_ICONS);
	pme->pTiles[HMMT_MESSAGE_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_MESSAGE], "template_justify_left_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_MESSAGE_TEXT], HB_HUD_TEXT);
	pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_X, 130);
	pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Y, 5);
	pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_WrapWidth, 320);
	pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Depth, 2);
	pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Alpha, 0);
	pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_MESSAGE_BRACKET] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_MESSAGE], "template_message_bracket", nullptr);
	SetBrightness(pme->pTiles[HMMT_MESSAGE_BRACKET], HB_HUD_BRACKETS);

	// The crosshair info (action, name, door and container state, two label and value pairs), above the bottom.
	pme->pTiles[HMMT_INFO] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Info");
	fValue = static_cast<float>(static_cast<double>(iScreenWidth / 2) -
		pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_INFO]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(static_cast<double>(iScreenHeight - iSafeY * 2) -
		pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Height) - 30.0);
	pme->pTiles[HMMT_INFO]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_INFO]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_INFO_ACTION_HINT] = Tile::GetTileByName(pme->pTiles[HMMT_INFO], "justify_center_hotrect");
	pme->pTiles[HMMT_INFO_ACTION_PC_HINT] = Tile::GetTileByName(pme->pTiles[HMMT_INFO_ACTION_HINT], "PCShortcutLabel");
	pme->pTiles[HMMT_INFO_ACTION_XBOX_HINT] = Tile::GetTileByName(pme->pTiles[HMMT_INFO_ACTION_HINT], "xbox_button");
	SetBrightness(pme->pTiles[HMMT_INFO_ACTION_HINT], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
	const int32_t iXTrait = Tile::TextToTrait("_x");
	pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(iXTrait, fValue, true);
	pme->pTiles[HMMT_INFO_ACTION_NAME] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_justify_center_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_ACTION_NAME], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_X, fValue, true);
	fValue = pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width);
	pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_WrapWidth, fValue, true);
	pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_Y, 0);
	fWrapWidth = pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(kTileValue_WrapWidth);
	iFont = static_cast<int32_t>(pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(kTileValue_Font));
	NiPoint3 kNameSize;
	kTextSize = *FontManager::Instance()->CalculateStringDimensions(&kNameSize, "Test Height", iFont, fWrapWidth, 0);
	pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_User8, kTextSize.y, true);
	pme->pTiles[HMMT_INFO_DOOR_STATE] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_justify_center_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_DOOR_STATE], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_X, fValue, true);
	fValue = pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width);
	pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_WrapWidth, fValue, true);
	pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_Y, 65);
	pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_Alpha, 0);
	pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_String, sLocked.String(), true);
	pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_Visible, 0);
	pme->pTiles[HMMT_INFO_CONTAINER_STATE] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_justify_center_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_CONTAINER_STATE], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(kTileValue_X, fValue, true);
	fValue = pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width);
	pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(kTileValue_WrapWidth, fValue, true);
	pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(kTileValue_Y, 65);
	pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(kTileValue_Alpha, 0);
	pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(kTileValue_String, sEmpty.String(), true);
	pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(kTileValue_Visible, 0);
	const int32_t iInfoLineY = 60;
	const int32_t iLabelOneOffset = 25;
	const int32_t iValueOneX = 10;
	const int32_t iLabelTwoOffset = 30;
	const int32_t iValueTwoOffset = 5;
	pme->pTiles[HMMT_INFO_LABEL_ONE] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_justify_right_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_LABEL_ONE], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0 - iLabelOneOffset);
	pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(kTileValue_Y, iInfoLineY);
	pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(kTileValue_Alpha, fCurrentHUDOpacity, true);
	pme->pTiles[HMMT_INFO_VALUE_ONE] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_justify_left_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_VALUE_ONE], HB_HUD_TEXT);
	pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(kTileValue_X, iValueOneX);
	pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(kTileValue_Y, iInfoLineY);
	pme->pTiles[HMMT_INFO_LABEL_TWO] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_justify_right_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_LABEL_TWO], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) - iLabelTwoOffset);
	pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(kTileValue_Y, iInfoLineY);
	pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(kTileValue_Alpha, fCurrentHUDOpacity, true);
	pme->pTiles[HMMT_INFO_VALUE_TWO] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_justify_left_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_VALUE_TWO], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0 + iValueTwoOffset);
	pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(kTileValue_Y, iInfoLineY);
	pme->pTiles[HMMT_INFO_SEPARATOR] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_INFO], "template_info_seperator", nullptr);
	SetBrightness(pme->pTiles[HMMT_INFO_SEPARATOR], HB_HUD_BRACKETS_GLOW);

	// The subtitles, above the crosshair info.
	pme->pTiles[HMMT_SUBTITLE] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Subtitles");
	fValue = static_cast<float>(static_cast<double>(iScreenWidth / 2) -
		pme->pTiles[HMMT_SUBTITLE]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_SUBTITLE]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Y);
	fValue = static_cast<float>(dSum - pme->pTiles[HMMT_SUBTITLE]->GetFloat(kTileValue_Height) - 50.0);
	pme->pTiles[HMMT_SUBTITLE]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_SUBTITLE]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_SUBTITLE_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_SUBTITLE], "template_justify_center_text", nullptr);
	SetBrightness(pme->pTiles[HMMT_SUBTITLE_TEXT], HB_HUD_TEXT);
	fValue = static_cast<float>(pme->pTiles[HMMT_SUBTITLE]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(kTileValue_X, fValue, true);
	fValue = pme->pTiles[HMMT_SUBTITLE]->GetFloat(kTileValue_Width);
	pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(kTileValue_WrapWidth, fValue, true);

	// The hotkey wheel, above the action points.
	pme->pTiles[HMMT_HOTKEY] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Hokeys");
	fValue = static_cast<float>(pme->pTiles[HMMT_AP]->GetFloat(kTileValue_X) + 15.0);
	pme->pTiles[HMMT_HOTKEY]->Set(kTileValue_X, fValue, true);
	dSum = static_cast<double>(iScreenHeight - iSafeY * 2) - pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Height);
	fValue = static_cast<float>(dSum - pme->pTiles[HMMT_HOTKEY]->GetFloat(kTileValue_Height) - 20.0);
	pme->pTiles[HMMT_HOTKEY]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_HOTKEY]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_HOTKEY_WHEEL] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_HOTKEY], "template_hotkey_selector", nullptr);
	pme->HKWheel.Initialize(static_cast<TileRect*>(pme->pTiles[HMMT_HOTKEY_WHEEL]));
	pme->pTiles[HMMT_HOTKEY_TEXT] =
		pMenu->RenderTemplate(pme->pTiles[HMMT_HOTKEY], "template_justify_center_text", nullptr);
	fValue = static_cast<float>(pme->pTiles[HMMT_HOTKEY]->GetFloat(kTileValue_Width) / 2.0);
	pme->pTiles[HMMT_HOTKEY_TEXT]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_HOTKEY_TEXT]->Set(kTileValue_Y, -50);
	pme->pTiles[HMMT_HOTKEY_TEXT]->Set(kTileValue_Visible, 0);

	// The experience meter; User3 keeps its resting height.
	pme->pTiles[HMMT_XP] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "XPMeter");
	pme->pTiles[HMMT_XP]->Set(kTileValue_X, iScreenWidth - iSafeX * 2 - 390);
	pme->pTiles[HMMT_XP]->Set(kTileValue_Y, iScreenHeight - iSafeY * 2 - 280);
	pme->pTiles[HMMT_XP]->Set(kTileValue_User3, iScreenHeight - iSafeY * 2 - 280);
	pme->pTiles[HMMT_XP_TEXT] = Tile::GetTileByName(pme->pTiles[HMMT_XP], "XPAmount");
	pme->pTiles[HMMT_XP_LABEL] = Tile::GetTileByName(pme->pTiles[HMMT_XP], "XPLabel");
	pme->pTiles[HMMT_XP_POINTER] = Tile::GetTileByName(pme->pTiles[HMMT_XP], "XPPointer");
	pme->pTiles[HMMT_XP_BRACKET] = Tile::GetTileByName(pme->pTiles[HMMT_XP], "XPBracket");
	pme->pTiles[HMMT_XP_LASTLEVEL] = Tile::GetTileByName(pme->pTiles[HMMT_XP], "XPLastLevel");
	pme->pTiles[HMMT_XP_NEXTLEVEL] = Tile::GetTileByName(pme->pTiles[HMMT_XP], "XPNextLevel");
	pme->pTiles[HMMT_XP_LEVELUP_TEXT] = Tile::GetTileByName(pme->pTiles[HMMT_XP], "XPLevelUp");

	// The breath meter, below the sneak meter, and the explosive indicators around the screen's centre.
	pme->pTiles[HMMT_BREATH] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "BreathMeter");
	pme->pTiles[HMMT_BREATH]->Set(kTileValue_X, iScreenWidth / 2);
	fValue = static_cast<float>(pme->pTiles[HMMT_SNEAK]->GetFloat(kTileValue_Y) + 80.0);
	pme->pTiles[HMMT_BREATH]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_EXPLOSIVE_INDICATOR] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Explosive_positioning_rect");
	pme->pTiles[HMMT_EXPLOSIVE_INDICATOR]->Set(kTileValue_X, iScreenWidth / 2);
	pme->pTiles[HMMT_EXPLOSIVE_INDICATOR]->Set(kTileValue_Y, iScreenHeight / 2);
	for (int32_t i = 0; i < 6; ++i)
		pme->RenderTemplate(pme->pTiles[HMMT_EXPLOSIVE_INDICATOR], "template_explosive_indicator", nullptr);

	// The crippled limb figure, at the top right; every part is sized and placed in percent of fScale.
	pme->pTiles[HMMT_CRIPPLED_GUY] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "crippled_limb_indicator");
	const int32_t iCrippledOffsetX = 200;
	const int32_t iCrippledOffsetY = 100;
	const float fScale = 75.0f;
	pme->pTiles[HMMT_CRIPPLED_GUY]->Set(kTileValue_X, iScreenWidth - iSafeX * 2 - iCrippledOffsetX);
	pme->pTiles[HMMT_CRIPPLED_GUY]->Set(kTileValue_Y, iCrippledOffsetY + iSafeY * 2);
	pme->pTiles[HMMT_CRIPPLED_HEAD] = Tile::GetTileByName(pme->pTiles[HMMT_CRIPPLED_GUY], "Head");
	pme->pTiles[HMMT_CRIPPLED_HEAD]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_CRIPPLED_HEAD]->Set(kTileValue_Width, static_cast<float>(fScale * 85.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_HEAD]->Set(kTileValue_Height, static_cast<float>(fScale * 92.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_HEAD]->Set(kTileValue_X, 0);
	pme->pTiles[HMMT_CRIPPLED_HEAD]->Set(kTileValue_Y, 40);
	pme->pTiles[HMMT_CRIPPLED_FACE] = Tile::GetTileByName(pme->pTiles[HMMT_CRIPPLED_GUY], "Face");
	pme->pTiles[HMMT_CRIPPLED_FACE]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_CRIPPLED_FACE]->Set(kTileValue_Width, static_cast<float>(fScale * 48.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_FACE]->Set(kTileValue_Height, static_cast<float>(fScale * 64.0 / 100.0), true);
	dSum = pme->pTiles[HMMT_CRIPPLED_HEAD]->GetFloat(kTileValue_X);
	dSum2 = pme->pTiles[HMMT_CRIPPLED_HEAD]->GetFloat(kTileValue_Width);
	fValue = static_cast<float>((dSum2 - pme->pTiles[HMMT_CRIPPLED_FACE]->GetFloat(kTileValue_Width)) / 2.0 + dSum);
	pme->pTiles[HMMT_CRIPPLED_FACE]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_CRIPPLED_HEAD]->GetFloat(kTileValue_Y);
	dSum2 = pme->pTiles[HMMT_CRIPPLED_HEAD]->GetFloat(kTileValue_Height);
	fValue = static_cast<float>((dSum2 - pme->pTiles[HMMT_CRIPPLED_FACE]->GetFloat(kTileValue_Height)) / 2.0 + dSum +
		fScale * 16.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_FACE]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_CRIPPLED_TORSO] = Tile::GetTileByName(pme->pTiles[HMMT_CRIPPLED_GUY], "Torso");
	pme->pTiles[HMMT_CRIPPLED_TORSO]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_CRIPPLED_TORSO]->Set(kTileValue_Width, static_cast<float>(fScale * 102.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_TORSO]->Set(kTileValue_Height, static_cast<float>(fScale * 128.0 / 100.0), true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_HEAD]->GetFloat(kTileValue_X) + fScale * -14.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_TORSO]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_HEAD]->GetFloat(kTileValue_Y) + fScale * 86.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_TORSO]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_CRIPPLED_LEFT_ARM] = Tile::GetTileByName(pme->pTiles[HMMT_CRIPPLED_GUY], "Left_Arm");
	pme->pTiles[HMMT_CRIPPLED_LEFT_ARM]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_CRIPPLED_LEFT_ARM]->Set(kTileValue_Width, static_cast<float>(fScale * 100.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_LEFT_ARM]->Set(kTileValue_Height, static_cast<float>(fScale * 52.0 / 100.0), true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_X) + fScale * 90.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_LEFT_ARM]->Set(kTileValue_X, fValue, true);
	fValue = pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_Y);
	pme->pTiles[HMMT_CRIPPLED_LEFT_ARM]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_ARM] = Tile::GetTileByName(pme->pTiles[HMMT_CRIPPLED_GUY], "Right_Arm");
	pme->pTiles[HMMT_CRIPPLED_RIGHT_ARM]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_ARM]->Set(kTileValue_Width, static_cast<float>(fScale * 96.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_ARM]->Set(kTileValue_Height, static_cast<float>(fScale * 54.0 / 100.0), true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_X) + fScale * -88.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_ARM]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_Y) + fScale * -5.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_ARM]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_CRIPPLED_LEFT_LEG] = Tile::GetTileByName(pme->pTiles[HMMT_CRIPPLED_GUY], "Left_Leg");
	pme->pTiles[HMMT_CRIPPLED_LEFT_LEG]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_CRIPPLED_LEFT_LEG]->Set(kTileValue_Width, static_cast<float>(fScale * 72.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_LEFT_LEG]->Set(kTileValue_Height, static_cast<float>(fScale * 112.0 / 100.0), true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_X) + fScale * 47.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_LEFT_LEG]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_Y) + fScale * 86.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_LEFT_LEG]->Set(kTileValue_Y, fValue, true);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_LEG] = Tile::GetTileByName(pme->pTiles[HMMT_CRIPPLED_GUY], "Right_Leg");
	pme->pTiles[HMMT_CRIPPLED_RIGHT_LEG]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_LEG]->Set(kTileValue_Width, static_cast<float>(fScale * 84.0 / 100.0), true);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_LEG]->Set(kTileValue_Height, static_cast<float>(fScale * 112.0 / 100.0), true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_X) + fScale * -37.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_LEG]->Set(kTileValue_X, fValue, true);
	fValue = static_cast<float>(pme->pTiles[HMMT_CRIPPLED_TORSO]->GetFloat(kTileValue_Y) + fScale * 83.0 / 100.0);
	pme->pTiles[HMMT_CRIPPLED_RIGHT_LEG]->Set(kTileValue_Y, fValue, true);

	// The damage and damage-threshold icons, beside the hit points and the enemy health meter.
	pme->pTiles[HMMT_DDT_ICON] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "DDTIcon");
	dSum = pme->pTiles[HMMT_HP]->GetFloat(kTileValue_X);
	fValue = static_cast<float>(pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Width) + dSum + 5.0);
	pme->pTiles[HMMT_DDT_ICON]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Y);
	fValue = static_cast<float>(pme->pTiles[HMMT_COMPASS]->GetFloat(kTileValue_Y) + dSum - 16.0);
	pme->pTiles[HMMT_DDT_ICON]->Set(kTileValue_Y, fValue, true);
	SetBrightness(pme->pTiles[HMMT_DDT_ICON], HB_HUD_TEXT);
	pme->pTiles[HMMT_DDT_ICON]->Set(kTileValue_Visible, 0);
	pme->pTiles[HMMT_DDT_ICON_ENEMY] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "DDTIconEnemy");
	dSum = pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(kTileValue_X);
	fValue = static_cast<float>(pme->pTiles[HMMT_ENEMY_HEALTH_METER]->GetFloat(kTileValue_X) + dSum - 32.0);
	pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(kTileValue_Y);
	fValue = static_cast<float>(pme->pTiles[HMMT_ENEMY_HEALTH_METER]->GetFloat(kTileValue_Y) + dSum);
	pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(kTileValue_Y, fValue, true);
	SetBrightness(pme->pTiles[HMMT_DDT_ICON_ENEMY], HB_HUD_TEXT);
	pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(kTileValue_SystemColor, 2);
	pme->pTiles[HMMT_DDT_ICON_AP] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "DDTIcon");
	dSum = pme->pTiles[HMMT_HP]->GetFloat(kTileValue_X);
	fValue = static_cast<float>(pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Width) + dSum + 5.0);
	pme->pTiles[HMMT_DDT_ICON_AP]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Y);
	fValue = static_cast<float>(pme->pTiles[HMMT_COMPASS]->GetFloat(kTileValue_Y) + dSum - 16.0);
	pme->pTiles[HMMT_DDT_ICON_AP]->Set(kTileValue_Y, fValue, true);
	SetBrightness(pme->pTiles[HMMT_DDT_ICON_AP], HB_HUD_TEXT);
	pme->pTiles[HMMT_DDT_ICON_AP]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_DDT_ICON_ENEMY_AP] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "DDTIconEnemyAP");
	dSum = pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(kTileValue_X);
	fValue = static_cast<float>(pme->pTiles[HMMT_ENEMY_HEALTH_METER]->GetFloat(kTileValue_X) + dSum - 32.0);
	pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(kTileValue_Y);
	fValue = static_cast<float>(pme->pTiles[HMMT_ENEMY_HEALTH_METER]->GetFloat(kTileValue_Y) + dSum);
	pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(kTileValue_Y, fValue, true);
	SetBrightness(pme->pTiles[HMMT_DDT_ICON_ENEMY_AP], HB_HUD_TEXT);
	pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(kTileValue_SystemColor, 2);

	// The ammo type label, beside the ammo count.
	pme->pTiles[HMMT_AMMO_SUB_TYPE] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "AmmoTypeLabel");
	dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_X);
	fValue = static_cast<float>(pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_X) + dSum - 165.0);
	pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(kTileValue_X, fValue, true);
	dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Y);
	fValue = static_cast<float>(pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_Y) + dSum);
	pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(kTileValue_Y, fValue, true);
	SetBrightness(pme->pTiles[HMMT_AMMO_SUB_TYPE], HB_HUD_TEXT);
	pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(kTileValue_Visible, 1);

	// The hardcore needs (dehydration, hunger, sleep, radiation, crippled limbs), in a row beside the hit points.
	pme->pTiles[HMMT_HARDCORE_MODE] =
		Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "HardcoreMode");
	fValue = static_cast<float>(pme->pTiles[HMMT_HP]->GetFloat(kTileValue_X) + 150.0);
	pme->pTiles[HMMT_HARDCORE_MODE]->Set(kTileValue_X, fValue, true);
	fValue = pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Y);
	pme->pTiles[HMMT_HARDCORE_MODE]->Set(kTileValue_Y, fValue, true);
	SetBrightness(pme->pTiles[HMMT_HARDCORE_MODE], HB_HUD_TEXT);
	pme->pTiles[HMMT_HARDCORE_MODE]->Set(kTileValue_Visible, 1);
	pme->pTiles[HMMT_HARDCORE_MODE]->Set(kTileValue_Locus, 1);
	pme->pTiles[HMMT_HARDCORE_H20] = Tile::GetTileByName(pme->pTiles[HMMT_HARDCORE_MODE], "Dehydration");
	pme->pTiles[HMMT_HARDCORE_H20]->Set(kTileValue_X, 0);
	pme->pTiles[HMMT_HARDCORE_H20]->Set(kTileValue_Y, 0);
	pme->pTiles[HMMT_HARDCORE_H20]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_HARDCORE_H20]->Set(kTileValue_Alpha, 0);
	pme->pTiles[HMMT_HARDCORE_FOD] = Tile::GetTileByName(pme->pTiles[HMMT_HARDCORE_MODE], "Hunger");
	fValue = static_cast<float>(pme->pTiles[HMMT_HARDCORE_H20]->GetFloat(kTileValue_Width) + 10.0);
	pme->pTiles[HMMT_HARDCORE_FOD]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_HARDCORE_FOD]->Set(kTileValue_Y, 0);
	pme->pTiles[HMMT_HARDCORE_FOD]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_HARDCORE_FOD]->Set(kTileValue_Alpha, 0);
	pme->pTiles[HMMT_HARDCORE_SLP] = Tile::GetTileByName(pme->pTiles[HMMT_HARDCORE_MODE], "Sleep");
	dSum = pme->pTiles[HMMT_HARDCORE_FOD]->GetFloat(kTileValue_Width) + 10.0;
	fValue = static_cast<float>(pme->pTiles[HMMT_HARDCORE_H20]->GetFloat(kTileValue_Width) + dSum + 10.0);
	pme->pTiles[HMMT_HARDCORE_SLP]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_HARDCORE_SLP]->Set(kTileValue_Y, 0);
	pme->pTiles[HMMT_HARDCORE_SLP]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_HARDCORE_SLP]->Set(kTileValue_Alpha, 0);
	pme->pTiles[HMMT_HARDCORE_RAD] = Tile::GetTileByName(pme->pTiles[HMMT_HARDCORE_MODE], "Rads");
	dSum = pme->pTiles[HMMT_HARDCORE_FOD]->GetFloat(kTileValue_Width) + 10.0;
	dSum2 = pme->pTiles[HMMT_HARDCORE_H20]->GetFloat(kTileValue_Width) + dSum + 10.0;
	fValue = static_cast<float>(pme->pTiles[HMMT_HARDCORE_SLP]->GetFloat(kTileValue_Width) + dSum2 + 10.0);
	pme->pTiles[HMMT_HARDCORE_RAD]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_HARDCORE_RAD]->Set(kTileValue_Y, 0);
	pme->pTiles[HMMT_HARDCORE_RAD]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_HARDCORE_RAD]->Set(kTileValue_Alpha, 0);
	pme->pTiles[HMMT_HARDCORE_LMB] = Tile::GetTileByName(pme->pTiles[HMMT_HARDCORE_MODE], "LMBs");
	dSum = pme->pTiles[HMMT_HARDCORE_FOD]->GetFloat(kTileValue_Width) + 10.0;
	dSum2 = pme->pTiles[HMMT_HARDCORE_H20]->GetFloat(kTileValue_Width) + dSum + 10.0;
	dSum3 = pme->pTiles[HMMT_HARDCORE_SLP]->GetFloat(kTileValue_Width) + dSum2 + 10.0;
	fValue = static_cast<float>(pme->pTiles[HMMT_HARDCORE_RAD]->GetFloat(kTileValue_Width) + dSum3 + 10.0);
	pme->pTiles[HMMT_HARDCORE_LMB]->Set(kTileValue_X, fValue, true);
	pme->pTiles[HMMT_HARDCORE_LMB]->Set(kTileValue_Y, 0);
	pme->pTiles[HMMT_HARDCORE_LMB]->Set(kTileValue_Zoom, fScale, true);
	pme->pTiles[HMMT_HARDCORE_LMB]->Set(kTileValue_Alpha, 0);

	if (!pme->IsWellFormedMenu()) {
		InitializationFlow("Hud Menu Creation Failed... This likely means that the archives were not built and the "
						   "xml doesn't match the code.");
		Interface::EmergencyCloseAllMenusAndBreakStuff();
	}
	// Stacking types 6006 and 102 put the HUD at the depth that was the deepest before it was read.
	if (pFile->GetFloat(kTileValue_StackingType) == 6006.0 || pFile->GetFloat(kTileValue_StackingType) == 102.0)
		pFile->Set(kTileValue_Depth, fMaxDepth, true);
	pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_String, sInventoryAmmo.String(), true);
	pme->pTiles[HMMT_ACTION_POINTS_TEXT]->Set(kTileValue_String, sActionPointsShort.String(), true);
	pme->pTiles[HMMT_HIT_POINT_TEXT]->Set(kTileValue_String, sHitPointsShort.String(), true);
	pme->iCompassImageWidth = 856;
	pme->iCompassDisplayWidth = static_cast<int32_t>(pme->pTiles[HMMT_COMPASS]->GetFloat(kTileValue_Width) / 2.0) - 35;
	pme->iCompassHalfAngle = 70;
	pFile->Set(kTileValue_Visible, 1);
	// The exe fetches the menu tile here and does not use it.
	pme->GetMenuTile();
	Tile::UpdateAll(false);

	// The compass's alpha map, which the compass's shader rotates.
	NiPointer<NiSourceTexture> spAlphaMap(nullptr);
	{
		NiFixedString kFilename("Textures\\Interface\\HUD\\hud_compass_alphamap.dds");
		spAlphaMap = NiSourceTexture::Create(kFilename, NiSourceTexture::ms_kDefaultPrefs, true, false);
	}
	NiSourceTexture* pAlphaMap = spAlphaMap.operator->();
	pme->pTiles[HMMT_COMPASS]->GetShaderProperty()->SetAlphaMap(pAlphaMap);
	pme->pTiles[HMMT_COMPASS]->GetShaderProperty()->SetUseTexScroll(true);

	// The gun scope model: an empty node turned to face the scope camera, lit by the 3D menu scene's lights.
	void* pNodeMemory = NiMemObject::operator new(sizeof(NiNode));
	spScopeModel = pNodeMemory ? ::new (pNodeMemory) NiNode(0) : nullptr;
	// The exe runs the empty constructor out of line on both matrices.
	NiMatrix3 kXRotation;
	kXRotation.~NiMatrix3();
	::new (&kXRotation) NiMatrix3;
	NiMatrix3 kZRotation;
	kZRotation.~NiMatrix3();
	::new (&kZRotation) NiMatrix3;
	kXRotation.MakeXRotation(1.5707964f);
	kZRotation.MakeZRotation(1.5707964f);
	// The product is built before the exe fetches the node (operator* 0x43F8D0, then operator->).
	const NiMatrix3 kRotation = kXRotation * kZRotation;
	spScopeModel->SetRotate(kRotation);
	spScopeModel->SetScale(0.1f);
	spScopeModel->SetTranslate(380.0f, 0.0f, 0.0f);
	NiUpdateData kScopeUpdate;
	kScopeUpdate.~NiUpdateData();
	::new (&kScopeUpdate) NiUpdateData(0.0f, false, false);
	spScopeModel->Update(kScopeUpdate);
	spScopeCamera = pme->Create3DCamera();
	pQuestUpdateManager = static_cast<QuestUpdateManager*>(ourMalloc(sizeof(QuestUpdateManager)));
	ShadowSceneNode* pMenuScene = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
	pMenuScene->AttachChild(spScopeModel.operator->(), true);
	const float fLightRadius = static_cast<float>(spScopeModel->GetWorldBound().m_fRadius * 3.0);
	NiNode* pScopeModel = spScopeModel.operator->();
	BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)
		->AddAllLightsFromNode_Recurse(pScopeModel, fLightRadius);

	// The breath meter model stands upright and plays its "Forward" animation from the start.
	NiNode* pBreathModel = pme->pTiles[HMMT_BREATH]->GetModel();
	NiControllerManager* pBreathController =
		static_cast<NiControllerManager*>(pBreathModel->GetAt(0)->GetController(&NiControllerManager::ms_RTTI));
	pBreathModel->SetRotate(-1.5707964f, 1.0f, 0.0f, 0.0f);
	NiUpdateData kBreathUpdate;
	kBreathUpdate.~NiUpdateData();
	::new (&kBreathUpdate) NiUpdateData(0.0f, true, false);
	pBreathModel->GetAt(0)->Update(kBreathUpdate);
	if (pBreathController) {
		{
			NiFixedString kForward("Forward");
			pme->pBreathMeterAnim = pBreathController->GetSequenceByName(kForward);
		}
		pBreathController->DeactivateAll(0.0f);
		if (pme->pBreathMeterAnim)
			pBreathController->ActivateSequence(pme->pBreathMeterAnim, 0, true, 1.0f, 0.0f, nullptr);
	}
	pMenu->PrepForVisibility(true);
	ResetHUDOpacity(fHudOpacity.Float());
	SetMenuMode(HMM_LOAD);
	Tile::GetTileUpdateAllCriticalSectionLock()->Leave();
	return static_cast<TileMenu*>(pFile);
}

// The mode's HUD elements are bits of iHUDVisibilityMenuModeFlags; bit n shows tile HMMT_AP + n (bit 0 also drives
// the ammo sub type and condition arrows, bit 16 the hardcore mode tile).
void HUDMainMenu::SetMenuMode(HUD_MENU_MODE aeMenuMode) {
	if (!HUDMainMenu::pme)
		return;

	HUD_MENU_MODE eMode = aeMenuMode;
	if (HUDMainMenu::pme->GetFadeState() != 1)
		HUDMainMenu::pme->StartFadeIn();

	if (eMode == HMM_CHANGED) {
		switch (Interface::GetTopMenuID()) {
		case Interface::eNoMenu:
		case Interface::eHUDMainMenu:
			if (pPlayer->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
				pPlayer->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
				eMode = HMM_SIT_SLEEP;
			} else if (pPlayer->GetSitSleepState() != SIT_SLEEP_NORMAL) {
				eMode = HMM_SIT_SLEEP_ANIM;
			} else if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
				eMode = HMM_VATS_PLAYBACK;
			} else if (pPlayer->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
				// The exe switches the HUD through a nested call and then goes on with the mode still
				// HMM_CHANGED.
				HUDMainMenu::SetMenuMode(HMM_MOVEMENT_DISABLED);
			} else if (pPlayer->GetIronSights()) {
				eMode = HMM_IRON_SIGHTS;
			} else {
				eMode = HMM_RUN_AND_GUN;
			}
			break;
		case Interface::eInventoryMenu:
		case Interface::eStatsMenu:
		case Interface::eLockPickMenu:
		case Interface::ePipBoyData:
		case Interface::ePipBoyRepair:
		case Interface::eItemModMenu:
			eMode = HMM_PIPBOY;
			break;
		case Interface::eStartMenu:
			eMode = HMM_START;
			break;
		case Interface::eLoadingMenu:
			eMode = HMM_LOAD;
			break;
		case Interface::eDialogMenu:
			eMode = HMM_DIALOGUE;
			break;
		case Interface::eVATSMenu:
			eMode = HMM_VATS;
			break;
		case Interface::eContainerMenu:
			eMode = HMM_CONTAINER;
			break;
		case Interface::eSleepWaitMenu:
			eMode = HMM_SIT_SLEEP;
			break;
		case Interface::eHackingMenu:
			// The exe stores HMM_HACKING and runs on into the computers case's store.
			eMode = HMM_HACKING;
			[[fallthrough]];
		case Interface::eComputersMenu:
			eMode = HMM_TERMINAL;
			break;
		default:
			break;
		}
	}

	if (eMode == HMM_IRON_SIGHTS && HUDMainMenu::spScopeModel.operator->() &&
		HUDMainMenu::spScopeModel.operator->()->GetAtChecked(0) && pPlayer->IsWeaponDrawn()) {
		eMode = HMM_SNIPER_RIFLE;
	}

	if (eMode != HMM_MOVIE && eMode != HMM_START && eMode != HMM_LOAD &&
		pPlayer->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
		if (InterfaceManager::Instance()->QLockpickControlsDisable())
			eMode = HMM_RUN_AND_GUN;
		else if (eMode == HMM_DIALOGUE)
			eMode = HMM_DIALOGUE;
		else
			eMode = HMM_MOVEMENT_DISABLED;
	}

	switch (eMode) {
	case HMM_RUN_AND_GUN:
	case HMM_PIPBOY_DISABLED:
	case HMM_VANITY:
	case HMM_IRON_SIGHTS:
		if (pPlayer->GetControlsDisabled(PlayerCharacter::DPC_ROLL_OVER))
			HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x1FDFF;
		else
			HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x1FFFF;
		HUDMainMenu::RefreshInfoForRef();
		break;
	case HMM_PIPBOY:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x100;
		break;
	case HMM_START:
	case HMM_LOAD:
	case HMM_CHARGEN_MENU:
	case HMM_MESSAGE_MENU:
	case HMM_SPECIAL_BOOK:
	case HMM_LOVE_TESTER:
	case HMM_AUTO_VANITY:
	case HMM_MOVIE:
	case HMM_CASINO_GAME:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0;
		break;
	case HMM_DIALOGUE:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x1110;
		break;
	case HMM_VATS:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x100;
		break;
	case HMM_VATS_PLAYBACK:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x108;
		break;
	case HMM_CONTAINER:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x1000;
		break;
	case HMM_SIT_SLEEP_ANIM:
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x11DBF;
		break;
	case HMM_SIT_SLEEP:
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x11FFF;
		break;
	case HMM_MOVEMENT_DISABLED:
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x1514;
		break;
	case HMM_HACKING:
	case HMM_TERMINAL:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 0x1010;
		break;
	case HMM_SNIPER_RIFLE:
		HUDMainMenu::pme->HideSubtitle();
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = 8;
		break;
	default:
		break;
	}

	HUDMainMenu::pme->iHUDVisibilityMenuModeFlags &= HUDMainMenu::pme->iHUDVisibilityUserSettings;
	if (bHideEnemyHealthMeter)
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags &= ~8;
	if (pPlayer && pPlayer->GetWeaponSightingNode() && !pPlayer->IsActually3rdPerson())
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags &= ~0x40;

	// In dialogue the XP meter moves 330 up from its resting height (kTileValue_User3).
	if (eMode == HMM_DIALOGUE) {
		const float fRestY = HUDMainMenu::pme->pTiles[HMMT_XP]->GetFloat(Tile::kTileValue_User3);
		const float fY = fRestY - 330.0;
		HUDMainMenu::pme->pTiles[HMMT_XP]->Set(Tile::kTileValue_Y, fY, true);
	} else {
		const float fRestY = HUDMainMenu::pme->pTiles[HMMT_XP]->GetFloat(Tile::kTileValue_User3);
		HUDMainMenu::pme->pTiles[HMMT_XP]->Set(Tile::kTileValue_Y, fRestY, true);
	}

	if (HUDMainMenu::pme->eMenuMode != HMM_SNIPER_RIFLE && eMode == HMM_SNIPER_RIFLE)
		HUDMainMenu::SetGunScopeVisible(true);
	else if (HUDMainMenu::pme->eMenuMode == HMM_SNIPER_RIFLE && eMode != HMM_SNIPER_RIFLE)
		HUDMainMenu::SetGunScopeVisible(false);
	HUDMainMenu::pme->eMenuMode = eMode;

	const uint32_t eNewMode = HUDMainMenu::pme->eMenuMode;
	if (eNewMode != HMM_VANITY && eNewMode != HMM_DIALOGUE && eNewMode != HMM_RUN_AND_GUN && eNewMode != HMM_LOAD &&
		eNewMode != HMM_CHARGEN_MENU && eNewMode != HMM_MESSAGE_MENU && eNewMode != HMM_CONTAINER &&
		eNewMode != HMM_SIT_SLEEP && eNewMode != HMM_SPECIAL_BOOK && eNewMode != HMM_LOVE_TESTER &&
		eNewMode != HMM_CASINO_GAME) {
		HUDMainMenu::pme->bKillQuestUpdates = true;
	}

	HUDMainMenu::pme->pTiles[HMMT_AP]->Set(Tile::kTileValue_Visible, HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 1);
	HUDMainMenu::pme->pTiles[HMMT_HP]->Set(Tile::kTileValue_Visible, HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 2);
	HUDMainMenu::pme->pTiles[HMMT_RADIATION]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 4);
	HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 8);
	HUDMainMenu::pme->pTiles[HMMT_QUEST_UPDATE]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x10);
	HUDMainMenu::pme->pTiles[HMMT_REGION_LOCATION]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x20);
	HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x40);
	HUDMainMenu::pme->pTiles[HMMT_SNEAK]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x80);
	HUDMainMenu::pme->pTiles[HMMT_MESSAGE]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x100);
	HUDMainMenu::pme->pTiles[HMMT_INFO]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x200);
	HUDMainMenu::pme->pTiles[HMMT_SUBTITLE]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x400);
	HUDMainMenu::pme->pTiles[HMMT_HOTKEY]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x800);
	HUDMainMenu::pme->pTiles[HMMT_XP]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x1000);
	HUDMainMenu::pme->pTiles[HMMT_BREATH]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x2000);
	HUDMainMenu::pme->pTiles[HMMT_EXPLOSIVE_INDICATOR]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x4000);
	HUDMainMenu::pme->pTiles[HMMT_CRIPPLED_GUY]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x8000);
	HUDMainMenu::pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 1);
	HUDMainMenu::pme->pTiles[HMMT_HARDCORE_MODE]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 0x10000);
	HUDMainMenu::pme->pTiles[HMMT_CND_ARROWS]->Set(Tile::kTileValue_Visible,
		HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 1);

	if (!(HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 1)) {
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_AP]->Set(Tile::kTileValue_Visible, 0);
	}
	if (!(HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 8)) {
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(Tile::kTileValue_Visible, 0);
		HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(Tile::kTileValue_Visible, 0);
	}

	if (HUDMainMenu::pme->GeigerSound.IsValid()) {
		if (HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 4)
			HUDMainMenu::pme->GeigerSound.Play(false);
		else
			HUDMainMenu::pme->GeigerSound.Pause();
	}
}

// cdecl, ret 0; the callers push the flag as a 4-byte 0 or 1.
void HUDMainMenu::SetGunScopeVisible(bool abVisibleFlag) {
	NiNode* v1;
	NiNode* v2;
	if (HUDMainMenu::pme) {
		if ((&HUDMainMenu::spScopeModel)->operator->()) {
			v1 = (&HUDMainMenu::spScopeModel)->operator->();
			if (v1->GetAtChecked(0)) {
				HUDMainMenu::pme->bScopeVisible = (bool)((uint8_t)abVisibleFlag);
				v2 = (&HUDMainMenu::spScopeModel)->operator->();
				v2->NiAVObject::SetAppCulled(!abVisibleFlag);
			}
		}
	}
}

// The skipped ids are the exe's jump table at 0x77E4BC (byte index table at 0x77E4C4, for ids 15 to 58).
void HUDMainMenu::ResetHUDOpacity(float afHUDOpacity) {
	if (!HUDMainMenu::pme)
		return;
	HUDMainMenu::pme->ClearAllSubtitlesAndMessages();
	for (int32_t i = 1; i < HMMT_AP; ++i) {
		switch (i) {
		case HMMT_REGION_TEXT:
		case HMMT_LOCATION_TEXT:
		case HMMT_RADIATION_BRACKET:
		case HMMT_RADIATION_POINTER:
		case HMMT_RADIATION_TEXT_VALUE:
		case HMMT_RADIATION_TEXT:
		case HMMT_SNEAK_TEXT:
		case HMMT_MESSAGE_ICON:
		case HMMT_MESSAGE_TEXT:
		case HMMT_MESSAGE_BRACKET:
		case HMMT_XP_BRACKET:
		case HMMT_XP_LEVELUP_TEXT:
		case HMMT_CRIPPLED_FACE:
		case HMMT_CRIPPLED_HEAD:
		case HMMT_CRIPPLED_TORSO:
		case HMMT_CRIPPLED_LEFT_ARM:
		case HMMT_CRIPPLED_RIGHT_ARM:
		case HMMT_CRIPPLED_LEFT_LEG:
		case HMMT_CRIPPLED_RIGHT_LEG:
			break;
		default: {
			// The factor is the double 255.0; the product is rounded to a float.
			const float fAlpha = static_cast<float>(afHUDOpacity * 255.0);
			HUDMainMenu::pme->pTiles[i]->Set(Tile::kTileValue_Alpha, fAlpha, true);
			break;
		}
		}
	}
}

// The death test is the reference's virtual IsDead (slot 139), not Actor's own body. The VATS mode is read through
// GetFormFlags, which folds with VATS::GetMode.
void HUDMainMenu::SetEnemyActor(Actor* apActor) {
	if (bHealthBarShowing.Bool())
		return;
	if (apActor && apActor->IsDead(false))
		return;
	if (!HUDMainMenu::pme)
		return;
	if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
		Actor* pTarget;
		if (pVATSTarget && pVATSTarget->IsActor())
			pTarget = static_cast<Actor*>(pVATSTarget);
		else
			pTarget = nullptr;
		HUDMainMenu::pme->pCurrentEnemy = pTarget;
	} else {
		HUDMainMenu::pme->pCurrentEnemy = apActor;
	}
	if (HUDMainMenu::pme->pCurrentEnemy) {
		const char* pName = HUDMainMenu::pme->pCurrentEnemy->GetFullName();
		HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_String, pName, true);
		HUDMainMenu::pme->HudTimer.Update();
		HUDMainMenu::pme->iEnemyHealthTimer = HUDMainMenu::pme->HudTimer.GetTime();
	}
}

// The exe takes the scene's culler and the 3D interface accumulator through their out-of-line getters (0x8D80E0,
// 0x7FD640). The accumulator pointer is built and released by NiPointer's out-of-line constructor and destructor
// (0x633C90, 0x45CEC0).
void HUDMainMenu::Draw3DElements() {
	if (!HUDMainMenu::spScopeModel.operator->())
		return;
	if (HUDMainMenu::pme && !HUDMainMenu::pme->bScopeVisible)
		return;
	if (!BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)->GetChildCount()) {
		ShadowSceneNode* pNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
		pNode->AttachChild(HUDMainMenu::spScopeModel.operator->(), true);
	}
	NiRenderer::GetRenderer()->ClearBuffer(nullptr, NiRenderer::CLEAR_ZBUFFER);
	BSSceneGraph* pWorld = Main::GetWorldSceneGraph();
	BSCullingProcess* pCuller = static_cast<BSCullingProcess*>(pWorld->GetCullingProcess());
	pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
	NiCamera* pScopeCamera = HUDMainMenu::spScopeCamera.operator->();
	NiCamera::HandleEyeFinityFOVScale(pScopeCamera, fScopeEyeFinityScale);
	{
		alignas(NiPointer<BSShaderAccumulator>) uint8_t aAccumulatorStorage[sizeof(NiPointer<BSShaderAccumulator>)];
		NiPointer<BSShaderAccumulator>& spAccumulator =
			*::new (aAccumulatorStorage)
				NiPointer<BSShaderAccumulator>(InterfaceManager::Instance()->Get3dAccumulator());
		pCuller->SetAccumulator(spAccumulator.operator->());
		NiCamera* pCamera = pScopeCamera ? pScopeCamera : pWorld->GetCamera();
		ShadowSceneNode* pScene = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
		BSShaderUtil::AccumulateScene(pCamera, pScene, *pCuller);
		BSShaderUtil::RenderScene(pCamera, spAccumulator.operator->(), false);
		pCuller->SetAccumulator(nullptr);
		spAccumulator = nullptr;
		pCuller->PopCullMode();
		spAccumulator.~NiPointer<BSShaderAccumulator>();
	}
}

void HUDMainMenu::HandleHealthbarForCrosshairActor(TESObjectREFR* apRef, bool abShow) {
	char v8 = 0;
	char v7 = 1;
	if (!apRef ||
		(!HUDMainMenu::pme ||
			(gs_bDisableDynamicCrosshair_GamePlay.Bool() ||
				(!abShow || (gs_bTrueIronSights_GamePlay.Bool() && pPlayer->GetIronSights()))))) {
		if (HUDMainMenu::pme->pTiles[62]->GetFloat(4084) == 1.0) {
			HUDMainMenu::pme->pTiles[21]->Set(4003, 0);
			HUDMainMenu::pme->pTiles[22]->Set(4003, 0);
			HUDMainMenu::pme->pTiles[23]->Set(4003, 0);
		}
	} else {
		apRef->GetRefPromptType();
		Actor* a1 = apRef->IsActor() ? static_cast<Actor*>(apRef) : 0;
		bool v5 = a1 && a1->IsDead(0);
		v7 = v5;
		if (a1) {
			if ((pPlayer->IsActorPerceived(a1) && pPlayer->IsActorPerceived(a1)->bIsHostile) ||
				a1->IsInCombatWithActor(pPlayer)) {
				v8 = 1;
			}
		}
		if (a1) {
			if (!v5 && !a1->IsDead(0)) {
				int32_t v2 = a1->GetActorValue(ActorValue::HEALTH);
				if (v2 < a1->GetBaseActorValue(ActorValue::HEALTH) && !HUDMainMenu::pme->pTiles[32]->IsTrue(4003)) {
					HUDMainMenu::SetEnemyActor(a1);
					HUDMainMenu::pme->pTiles[21]->Set(4003, 1);
					HUDMainMenu::pme->pTiles[22]->Set(4003, 1);
					HUDMainMenu::pme->pTiles[23]->Set(4003, 1);
					HUDMainMenu::pme->pTiles[23]->Set(4003, 1);
					HUDMainMenu::pme->pTiles[62]->Set(4084, (v8 != 0) + 1);
					HUDMainMenu::pme->pTiles[21]->AddNeedsUpdate(8);
					HUDMainMenu::pme->pTiles[22]->AddNeedsUpdate(8);
					HUDMainMenu::pme->pTiles[23]->AddNeedsUpdate(8);
					HUDMainMenu::pme->pTiles[23]->AddNeedsUpdate(8);
					HUDMainMenu::pme->UpdateEnemyHealth(a1);
				}
			}
		}
	}
	bool v3 = !v7 && v8;
	bHostileUnderCrosshair = v3;
	bLongDistancePick = abShow;
}

// The exe reads the player singleton again for the null test, the vtable and the call.
void HUDMainMenu::HandleHealthbarForCrosshairActor_(TESObjectREFR* apRef) {
	if (pPlayer && pPlayer->Get3D())
		HUDMainMenu::HandleHealthbarForCrosshairActor(apRef, false);
}

// KEEP, unregistered: the exe reads a byte nothing wrote. For each map marker it passes SetLocationText (0x772AA0) its
// HUDTravelLocData local, whose bForceLastLocation (byte +5, [ebp-0x8F]) it writes only for a marker just
// discovered or a travel marker. For a non-travel marker outside its radius the record goes with that byte
// unwritten, and the callee reads it (0x772C18): on the first such marker it is the exe's stack garbage. The local
// is declared once, outside the loop, as the exe's one stack slot.
// The guarded statics are the three user trait ids under bits 1, 2 and 4 of UpdateCompass_guards (no atexit). The NPC
// ticks are dropped beyond the map marker visible distance too (the exe reuses its square there), and the
// exterior sneak distance reads fSneakMaxDistance through Setting::Int, as the exe does. The quest target's
// distance-scaled alpha (FLerp) is computed and never used, as in the exe. The exe's cleanup-only C++ EH frame is
// left out.
void HUDMainMenu::UpdateCompass(Tile* apCompass, Tile* apGeneralIcons, Tile* apMarkerIcons, Tile* apNPCIcons) {
	static int32_t g_HUDMainMenuTraits_Distance = Tile::AddUserTrait("_Distance", -1);
	static int32_t g_HUDMainMenuTraits_Heading = Tile::AddUserTrait("_Heading", -1);
	static int32_t g_HUDMainMenuTraits_AlphaDown = Tile::AddUserTrait("_AlphaDown", -1);

	// The compass texture scrolls with the player's heading in degrees (the qword 57.2957763671875 at 0x102F248),
	// wrapped into [0, 360) by a truncating division.
	const double fPlayerHeading = pPlayer->GetHeading(false);
	const float fAngle = (pPlayer->GetParentCell()->GetNorthRotation() + fPlayerHeading) * 57.2957763671875;
	const float fWrappedAngle = fAngle - static_cast<double>(360 * static_cast<int32_t>(std::fabs(fAngle) / 360.0));
	const float fPixelHeading = FLerp(0.0f, 1.0f, 0.0f, 360.0f, fWrappedAngle);
	const float fScroll = fPixelHeading - 0.15000000596046448;
	if (apCompass->GetShaderProperty()) {
		double fWrappedScroll;
		if (fScroll < 0.0)
			fWrappedScroll = fScroll + 1.0;
		else
			fWrappedScroll = fScroll;
		const float fTexScroll = fWrappedScroll;
		apCompass->GetShaderProperty()->SetTexScroll(fTexScroll, 0.0f);
	}

	BSSimpleList<PlayerCharacter::MapMarkerInfo*>* pMarkerNode = pPlayer->GetMapMarkerStructList();
	const NiPoint3 kPlayerLoc = pPlayer->GetLocationOnReference();
	Tile* pTile = nullptr;
	// Icons fade out between 0.8 and 1.0 of the compass's double width.
	const double fFadeStartHalf = static_cast<double>(HUDMainMenu::pme->iCompassDisplayWidth) * 0.800000011920929;
	const float fFadeStart = fFadeStartHalf + fFadeStartHalf;
	const float fFadeEnd = static_cast<double>(HUDMainMenu::pme->iCompassDisplayWidth) +
		static_cast<double>(HUDMainMenu::pme->iCompassDisplayWidth);
	float fOpacity;
	if (pVATS.eMode)
		fOpacity = 1.0f;
	else
		fOpacity = fHudOpacity.Float();
	const float fFullBright = fOpacity * 255.0;
	NiTListItem<Tile*>* pPos = apGeneralIcons->xChildren.m_pkHead;
	HUDMainMenu::HUDTravelLocData kTravelLoc;
	bool bFirstDiscovery = true;
	const bool bPlayerIsDead = pPlayer->IsDead(false);
	int32_t iMapMarkerVisibleDistanceSqr = iMapMarkerVisibleDistance.Int();
	iMapMarkerVisibleDistanceSqr *= iMapMarkerVisibleDistanceSqr;

	// Map markers: those in their radius are discovered, and each one in the visible distance gets an icon.
	while (pMarkerNode && pMarkerNode->GetItem()) {
		PlayerCharacter::MapMarkerInfo* pInfo = pMarkerNode->GetItem();
		pMarkerNode = pMarkerNode->GetNext();
		if (pInfo->pMarkerRef->GetDisabled())
			continue;
		const NiPoint3 kItemLoc = pInfo->pMarkerRef->GetLocationOnReference();
		const float fDistSqr = std::fabs((kItemLoc - kPlayerLoc).SqrLength());
		if (fDistSqr > static_cast<double>(iMapMarkerVisibleDistanceSqr))
			continue;

		bool bDiscovered = false;
		const bool bCanFastTravel = pInfo->pMarkerData->GetTravelLoc();
		float fRadiusSqr = pInfo->pMarkerRef->GetRadius();
		fRadiusSqr = fRadiusSqr * fRadiusSqr;
		if (fRadiusSqr > fDistSqr) {
			if (!pInfo->pMarkerData->GetVisible()) {
				bDiscovered = true;
				pInfo->pMarkerData->SetIsVisible(1);
				pInfo->pMarkerRef->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
			}
			if (!bCanFastTravel) {
				bDiscovered = true;
				pInfo->pMarkerData->SetTravelLoc(true);
				pInfo->pMarkerRef->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
			}
		}
		if (!bPlayerIsDead) {
			kTravelLoc.pMarker = pInfo->pMarkerRef;
			kTravelLoc.bInRange = false;
			if (bDiscovered) {
				const char* pLocationName = pInfo->pMarkerData->GetLocationName();
				QuestUpdateManager::SetCustomQuestText(sDiscoveredText.String(), pLocationName,
					QuestUpdateManager::HCQQP_NORMAL, QuestUpdateManager::HCQTJ_LEFT, FONT_NONE, FONT_NONE,
					"UIPopUpQuestNew");
				TESChallengeManager::UpdateChallenge(CHALLENGE_TYPE_DISCOVER, 1, pInfo->pMarkerRef, pInfo->pMarkerRef,
					0, 0, 0);
				MiscStatManager::Increment(MiscStatManager::MS_LOCATIONS_DISCOVERED);
				pPlayer->RewardExperience(iXPRewardDiscoverMapMarker.Int());
				kTravelLoc.bInRange = false;
				kTravelLoc.bForceLastLocation = true;
			} else if (bCanFastTravel) {
				kTravelLoc.bInRange = fRadiusSqr > fDistSqr;
				kTravelLoc.bForceLastLocation = false;
			}
			if (!kTravelLoc.bInRange) {
				HUDMainMenu::SetLocationText(kTravelLoc);
			} else if (bFirstDiscovery && bDiscovered) {
				HUDMainMenu::SetLocationText(kTravelLoc);
				bFirstDiscovery = false;
			}
		}
		if (!pPos)
			continue;
		pTile = apGeneralIcons->xChildren.GetNext(pPos);
		if (bCanFastTravel)
			pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_compass_landmark_discovered.dds", true);
		else
			pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_compass_landmark.dds", true);
		if (!pTile)
			continue;
		const float fAngleDelta = HUDMainMenu::GetCompassAngle(kItemLoc, kPlayerLoc);
		if (static_cast<double>(HUDMainMenu::pme->iCompassHalfAngle) > std::fabs(fAngleDelta)) {
			const float fTargetAngle = FLerp(0.0f, static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
				static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
				static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle), fAngleDelta);
			pTile->Set(g_HUDMainMenuTraits_Heading, fTargetAngle, true);
			pTile->Set(g_HUDMainMenuTraits_Distance, fDistSqr, true);
			pTile->Set(Tile::kTileValue_Visible, 1);
			if (fFadeStart < fTargetAngle) {
				const float fAlpha = FLerp(1.0f, 0.0f, fFadeStart, fFadeEnd, fTargetAngle) * fFullBright;
				pTile->Set(Tile::kTileValue_Alpha, fAlpha, true);
			} else if (fDistSqr < 10000.0) {
				HandleQuestObjectiveBlinking(pTile, fDistSqr, fCurrentHUDOpacity);
			} else {
				pTile->Set(Tile::kTileValue_Alpha, fFullBright, true);
			}
		} else {
			pTile->Set(Tile::kTileValue_Visible, 0);
		}
	}
	while (pPos) {
		Tile* pUnused = apGeneralIcons->xChildren.GetNext(pPos);
		pUnused->Set(Tile::kTileValue_Visible, 0);
	}

	// Quest targets take the marker icons from the tail; a target through a door shows at full brightness.
	HUDMainMenu::pme->pQuestList = pPlayer->GetCurrentTargetList();
	pPos = apMarkerIcons->xChildren.m_pkTail;
	BSSimpleList<TESQuestTarget*>* pQuestNode = HUDMainMenu::pme->pQuestList;
	float fRevealDistanceSqr = static_cast<float>(iMapMarkerRevealDistance.Int());
	fRevealDistanceSqr = fRevealDistanceSqr * fRevealDistanceSqr;
	while (pQuestNode && pQuestNode->GetItem()) {
		bool bIsThroughDoor = false;
		TESQuestTarget* pQuestTarget = pQuestNode->GetItem();
		pQuestNode = pQuestNode->GetNext();
		TESObjectREFR* pTargetRef = pQuestTarget->QTrackingRef();
		if (pTargetRef && pTargetRef->GetFormType() == DOOR_ID)
			bIsThroughDoor = true;
		if (!pTargetRef || !bShowQuestMarkers.Bool())
			continue;
		const NiPoint3 kTargetLoc = pTargetRef->GetLocationOnReference();
		const float fDistSqr = std::fabs((kTargetLoc - kPlayerLoc).SqrLength());
		[[maybe_unused]] float fAlpha = fFullBright;
		if (fDistSqr != 0.0) {
			const float fRevealFraction = BSMIN(fDistSqr, fRevealDistanceSqr) / fRevealDistanceSqr;
			const float fHalfBright = fFullBright / 2.0;
			fAlpha = FLerp(fHalfBright, fFullBright, fRevealFraction, 0.0f, fDistSqr);
		}
		if (!pPos)
			continue;
		pTile = apMarkerIcons->xChildren.GetPrev(pPos);
		pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_compass_objective_marker.dds", true);
		if (!pTile)
			continue;
		float fAngleDelta = HUDMainMenu::GetCompassAngle(kTargetLoc, kPlayerLoc);
		if (fAngleDelta > static_cast<double>(HUDMainMenu::pme->iCompassHalfAngle))
			fAngleDelta = static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle);
		else if (fAngleDelta < static_cast<double>(-HUDMainMenu::pme->iCompassHalfAngle))
			fAngleDelta = static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle);
		float fTargetAngle = FLerp(0.0f, static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
			static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
			static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle), fAngleDelta);
		if (fTargetAngle > fFadeStart + 70.0)
			fTargetAngle = fFadeStart + 70.0;
		pTile->Set(g_HUDMainMenuTraits_Heading, fTargetAngle, true);
		pTile->Set(g_HUDMainMenuTraits_Distance, fDistSqr, true);
		pTile->Set(Tile::kTileValue_Visible, 1);
		if (apCompass->GetMenu()->xFadeState == Interface::FADE_SHOWN) {
			if (bIsThroughDoor)
				pTile->Set(Tile::kTileValue_Alpha, fFullBright, true);
			else
				HandleQuestObjectiveBlinking(pTile, fDistSqr, fCurrentHUDOpacity);
		}
	}

	// The player's placed marker (or teleport link), level with the player.
	if (pPlayer->QPlayerMarkerTrackingRef() && pPos) {
		pTile = apMarkerIcons->xChildren.GetPrev(pPos);
		pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_compass_pc_marker.dds", true);
		if (pTile) {
			NiPoint3 kMarkerLoc = pPlayer->QPlayerMarkerTrackingRef()->GetLocationOnReference();
			kMarkerLoc.z = kPlayerLoc.z;
			const float fDistSqr = std::fabs((kMarkerLoc - kPlayerLoc).SqrLength());
			const float fAngleDelta = HUDMainMenu::GetCompassAngle(kMarkerLoc, kPlayerLoc);
			if (static_cast<double>(HUDMainMenu::pme->iCompassHalfAngle) > std::fabs(fAngleDelta)) {
				const float fTargetAngle = FLerp(0.0f, static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
					static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
					static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle), fAngleDelta);
				pTile->Set(g_HUDMainMenuTraits_Heading, fTargetAngle, true);
				pTile->Set(Tile::kTileValue_Visible, 1);
				if (fFadeStart < fTargetAngle) {
					const float fAlpha = FLerp(fFullBright, 0.0f, fFadeStart, fFadeEnd, fTargetAngle);
					HandleQuestObjectiveBlinking(pTile, fDistSqr, fAlpha);
				} else {
					HandleQuestObjectiveBlinking(pTile, fDistSqr, fCurrentHUDOpacity);
				}
			} else {
				pTile->Set(Tile::kTileValue_Visible, 0);
			}
		}
	}
	while (pPos) {
		Tile* pUnused = apMarkerIcons->xChildren.GetPrev(pPos);
		pUnused->Set(Tile::kTileValue_Visible, 0);
	}

	// NPC ticks: the nearest iHUDMaxCompassNPCTicks detected actors with 3D in attached cells, within the sneak
	// distance (interior or exterior) and the map marker visible distance. Invisible or chameleon actors show only
	// to a player with the improved detection entry point.
	g_bIsEnemyWithinCompassRange = 0;
	pPos = apNPCIcons->xChildren.m_pkHead;
	BSSimpleList<CompassTarget*>* pTargetNode = pPlayer->GetListofPerceivedActors();
	const bool bInterior = pPlayer->GetParentCell()->GetInterior();
	float fSneakMaxDistanceSqr = fSneakMaxDistance.Float();
	fSneakMaxDistanceSqr = fSneakMaxDistanceSqr * fSneakMaxDistanceSqr;
	const float& rExteriorMult = fSneakExteriorDistanceMult.Float();
	float fSneakExteriorDistanceSqr = static_cast<double>(fSneakMaxDistance.Int()) * rExteriorMult;
	fSneakExteriorDistanceSqr = fSneakExteriorDistanceSqr * fSneakExteriorDistanceSqr;
	uint32_t uiMaxNPCTicks = pTargetNode->ItemsInList();
	if (static_cast<uint32_t>(pTargetNode->ItemsInList()) > static_cast<uint32_t>(iHUDMaxCompassNPCTicks.Int())) {
		pTargetNode->Sort(&CompassTarget::CompareForSorting);
		uiMaxNPCTicks = iHUDMaxCompassNPCTicks.Int();
	}
	while (pTargetNode && !pTargetNode->IsEmpty() && uiMaxNPCTicks) {
		--uiMaxNPCTicks;
		CompassTarget* pTarget = pTargetNode->GetItem();
		if (!pTarget || pTarget->bIsUndetected || !pTarget->pTarget->Get3D() || !pTarget->pTarget->GetParentCell() ||
			!pTarget->pTarget->GetParentCell()->GetAttached()) {
			pTargetNode = pTargetNode->GetNext();
			continue;
		}
		if (static_cast<ActorValueOwner*>(pTarget->pTarget)->GetActorValue(ActorValue::CHAMELEON) ||
			static_cast<ActorValueOwner*>(pTarget->pTarget)->GetActorValue(ActorValue::INVISIBILITY)) {
			float fImprovedDetection = 0.0f;
			BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::HAS_IMPROVED_DETECTION, pPlayer, &fImprovedDetection);
			if (fImprovedDetection <= 0.0) {
				pTargetNode = pTargetNode->GetNext();
				continue;
			}
		}
		const NiPoint3 kActorLoc = pTarget->pTarget->GetLocationOnReference();
		const float fDistSqr = std::fabs((kActorLoc - kPlayerLoc).SqrLength());
		if ((bInterior && fSneakMaxDistanceSqr < fDistSqr) || (!bInterior && fSneakExteriorDistanceSqr < fDistSqr)) {
			pTargetNode = pTargetNode->GetNext();
			continue;
		}
		if (fDistSqr < static_cast<double>(iMapMarkerVisibleDistanceSqr)) {
			// Out of icons: the exe goes round again without moving on (the tick count still runs down).
			if (!pPos)
				continue;
			pTile = apNPCIcons->xChildren.GetNext(pPos);
			pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_tick_mark.dds", true);
			if (pTile) {
				const float fAngleDelta = HUDMainMenu::GetCompassAngle(kActorLoc, kPlayerLoc);
				if (static_cast<double>(HUDMainMenu::pme->iCompassHalfAngle) > std::fabs(fAngleDelta)) {
					const float fTargetAngle =
						FLerp(0.0f, static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
							static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
							static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle), fAngleDelta);
					const float fTickAngle = fTargetAngle + 2.0;
					pTile->Set(g_HUDMainMenuTraits_Heading, fTickAngle, true);
					pTile->Set(Tile::kTileValue_Visible, 1);
					if (pTarget->bIsHostile) {
						pTile->Set(Tile::kTileValue_SystemColor, 2);
						g_bIsEnemyWithinCompassRange = 1;
					} else {
						pTile->Set(Tile::kTileValue_SystemColor, 1);
					}
					if (fFadeStart < fTargetAngle) {
						pTile->Set(Tile::kTileValue_Alpha, FLerp(fFullBright, 0.0f, fFadeStart, fFadeEnd, fTargetAngle),
							true);
					} else {
						pTile->Set(Tile::kTileValue_Alpha, fFullBright, true);
					}
				} else {
					pTile->Set(Tile::kTileValue_Visible, 0);
				}
			}
		}
		pTargetNode = pTargetNode->GetNext();
	}
	while (pPos) {
		Tile* pUnused = apNPCIcons->xChildren.GetNext(pPos);
		pUnused->Set(Tile::kTileValue_Visible, 0);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HUDMainMenu::UpdateMeter(Tile* apTile, float afMeterPercent, HUDMainMenu::HUD_JUSTIFY aeJustify,
	int32_t aiMinTicks, int32_t iNumPixelShift) {
	static int32_t iUT_TotalWidth = Tile::AddUserTrait("_TotalWidth", -1);
	static int32_t iUT_ImageWidth = Tile::AddUserTrait("_ImageWidth", -1);
	static int32_t iUT_OriginalX = Tile::AddUserTrait("_OriginalX", -1);
	if (apTile) {
		if (HUDMainMenu::pme) {
			int32_t a3;
			if (!(afMeterPercent > 1.0)) {
				if (afMeterPercent < 0.0) {
					afMeterPercent = 0.0;
				}
			} else {
				afMeterPercent = 1.0;
			}
			int32_t Float = static_cast<int32_t>(apTile->GetFloat(iUT_TotalWidth));
			int32_t v8 = static_cast<int32_t>(apTile->GetFloat(iUT_ImageWidth));
			if (aeJustify == HUDMainMenu::HJ_CENTER) {
				float v6;
				if (!(afMeterPercent >= 0.0)) {
					v6 = 0.0;
				} else {
					v6 = afMeterPercent;
				}
				a3 = 2 * v8 * static_cast<int32_t>(static_cast<double>(Float) * v6 / static_cast<double>(2 * v8));
			} else {
				float v5;
				if (!(afMeterPercent >= 0.0)) {
					v5 = 0.0;
				} else {
					v5 = afMeterPercent;
				}
				a3 = v8 * static_cast<int32_t>(static_cast<double>(Float) * v5 / static_cast<double>(v8));
			}
			if (a3 < aiMinTicks * v8) {
				a3 = aiMinTicks * v8;
			}
			int32_t aiValue = static_cast<int32_t>(apTile->GetFloat(iUT_OriginalX));
			if (aeJustify == HUDMainMenu::HJ_RIGHT) {
				aiValue -= a3;
			} else {
				if (aeJustify == HUDMainMenu::HJ_CENTER) {
					aiValue -= a3 / 2;
				}
			}
			apTile->Set(4017, a3);
			apTile->Set(4001, iNumPixelShift + aiValue);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void HUDMainMenu::UpdateMeter(Tile* apMeterTile, Tile* apGhostMeterTile, float afMeterPercent,
	float afGhostMeterPercent, HUDMainMenu::HUD_JUSTIFY aeJustify, int32_t aiMinTicks) {
	// IDA names; not in the PDB. The meter tiles' user traits.
	static int32_t g_VATSMenuTrait_TotalWidth = Tile::AddUserTrait("_TotalWidth", -1);
	static int32_t g_VATSMenuTrait_ImageWidth = Tile::AddUserTrait("_ImageWidth", -1);
	static int32_t g_VATSMenuTrait_OriginalX = Tile::AddUserTrait("_OriginalX", -1);
	static int32_t g_VATSMenuTrait_Brightness = Tile::AddUserTrait("_Brightness", -1);
	if (apMeterTile) {
		if (apGhostMeterTile) {
			if (HUDMainMenu::pme) {
				int32_t itotalNewWidth;
				int32_t inewWidth;
				if (!(afMeterPercent > 1.0)) {
					if (afMeterPercent < 0.0) {
						afMeterPercent = 0.0f;
					}
				} else {
					afMeterPercent = 1.0f;
				}
				if (!(afGhostMeterPercent > 1.0)) {
					if (afGhostMeterPercent < 0.0) {
						afGhostMeterPercent = 0.0f;
					}
				} else {
					afGhostMeterPercent = 1.0f;
				}
				int32_t imeterWidth = static_cast<int32_t>(apMeterTile->GetFloat(g_VATSMenuTrait_TotalWidth));
				int32_t iimageWidth = static_cast<int32_t>(apMeterTile->GetFloat(g_VATSMenuTrait_ImageWidth));
				if (aeJustify == HUDMainMenu::HJ_CENTER) {
					itotalNewWidth = 2 * iimageWidth *
						static_cast<int32_t>(static_cast<double>(imeterWidth) * afGhostMeterPercent /
							static_cast<double>(2 * iimageWidth));
				} else {
					itotalNewWidth = iimageWidth *
						static_cast<int32_t>(static_cast<double>(imeterWidth) * afGhostMeterPercent /
							static_cast<double>(iimageWidth));
				}
				if (aeJustify == HUDMainMenu::HJ_CENTER) {
					float v8;
					if (!(afMeterPercent >= 0.0)) {
						v8 = 0.0f;
					} else {
						v8 = afMeterPercent;
					}
					inewWidth = 2 * iimageWidth *
						static_cast<int32_t>(static_cast<double>(imeterWidth) * v8 /
							static_cast<double>(2 * iimageWidth));
				} else {
					float v7;
					if (!(afMeterPercent >= 0.0)) {
						v7 = 0.0f;
					} else {
						v7 = afMeterPercent;
					}
					inewWidth = iimageWidth *
						static_cast<int32_t>(static_cast<double>(imeterWidth) * v7 / static_cast<double>(iimageWidth));
				}
				int32_t inewX = static_cast<int32_t>(apMeterTile->GetFloat(g_VATSMenuTrait_OriginalX));
				if (aeJustify == HUDMainMenu::HJ_RIGHT) {
					inewX -= inewWidth;
				} else {
					if (aeJustify == HUDMainMenu::HJ_CENTER) {
						inewX -= inewWidth / 2;
					}
				}
				int32_t ighostWidth = itotalNewWidth;
				int32_t ighostX = static_cast<int32_t>(apGhostMeterTile->GetFloat(g_VATSMenuTrait_OriginalX));
				if (aeJustify == HUDMainMenu::HJ_RIGHT) {
					ighostX = inewX - (itotalNewWidth - inewWidth);
					int32_t Float = static_cast<int32_t>(apGhostMeterTile->GetFloat(g_VATSMenuTrait_OriginalX));
					ighostWidth = BSMIN(itotalNewWidth - inewWidth, Float - ighostX);
				} else {
					if (aeJustify) {
						if (aeJustify == HUDMainMenu::HJ_CENTER) {
							ighostX -= itotalNewWidth / 2;
							ighostWidth = inewX - ighostX;
							Tile* ptile = apGhostMeterTile->xChildren.GetHead();
							ptile->Set(4017, inewX - ighostX);
							ptile->Set(4001, inewWidth + inewX);
							if (!ptile->HasFadeControl(4009)) {
								ptile->AddFadeControl(4009, 0.0f, 255.0f, 1.0f,
									Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
							}
						}
					} else {
						ighostX += inewWidth;
					}
				}
				if (!inewWidth && ighostWidth < aiMinTicks * iimageWidth) {
					ighostWidth = aiMinTicks * iimageWidth;
				}
				apMeterTile->Set(4017, inewWidth);
				apMeterTile->Set(4001, inewX);
				apGhostMeterTile->Set(4017, ighostWidth);
				apGhostMeterTile->Set(4001, ighostX);
				if (!apGhostMeterTile->HasFadeControl(4009)) {
					apGhostMeterTile->AddFadeControl(4009, 0.0f, 255.0f, 1.0f,
						Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
				}
			}
		}
	}
}

// The exe initialises iUT_Hold but looks "_Hold" up again for the hold it sets.
void HUDMainMenu::SetRegionText(const char* aRegionData) {
	// The "_Hold" trait id (the PDB's name).
	static int32_t iUT_Hold = Tile::TextToTrait("_Hold");
	if (!aRegionData)
		return;
	if (HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_Alpha) == 0.0 &&
		HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
		HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::kTileValue_String, aRegionData, true);
		HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::TextToTrait("_Hold"), 1);
	}
}

float HUDMainMenu::HeadingBetweenLocations(const NiPoint3& aStart, const NiPoint3& aDest) {
	float v5;
	NiPoint3 result;
	float afAngle = (float)0.0;
	(result = (&aDest)->operator-(aStart));
	if (result.y == 0.0) {
		if (result.x < 0.0) {
			afAngle = 3.1415927410125732f;
		}
	} else {
		v5 = result.x / result.y;
		afAngle = (float)std::atanf(v5);
		if (result.y < 0.0) {
			afAngle = (float)(afAngle + 3.1415927410125732);
		}
	}
	float v7 = (float)ClampAngle(afAngle);
	return (float)(v7 * 57.2957763671875);
}

void HUDMainMenu::DisplayQuestReminder(TESQuest* apQuest) {
	if (HUDMainMenu::pme) {
		if (apQuest) {
			if (!HUDMainMenu::pme->AreObjectivesShowing() && HUDMainMenu::pme->xQuestObjectives.IsEmpty()) {
				for (BSSimpleList<BGSQuestObjective*>* plist = apQuest->GetObjectiveList(); plist && !plist->IsEmpty();
					plist = plist->m_pkNext) {
					BGSQuestObjective** v1 = &plist->m_item;
					if ((*v1)->GetStatus(1)) {
						BGSQuestObjective** v2 = &plist->m_item;
						if (!(*v2)->GetStatus(2)) {
							BGSQuestObjective** v3 = &plist->m_item;
							const char* DisplayText = (*v3)->GetDisplayText();
							HUDMainMenu::ShowQuestObjective(DisplayText, 0, 1);
						}
					}
				}
			}
		}
	}
}

// With no quest or no HUD the exe returns with the critical section still held.
void HUDMainMenu::ShowQuestName(TESQuest* apQuest) {
	HUDShowQuestNameCritSect.Enter(nullptr);
	if (!apQuest || !HUDMainMenu::pme)
		return;
	uiMessageDelayTime = appTimer.GetTime();
	auto* pQuestUpdate =
		static_cast<QuestUpdateManager::QuestUpdate*>(ourMalloc(sizeof(QuestUpdateManager::QuestUpdate)));
	pQuestUpdate->pQuest = apQuest;
	if (apQuest->GetFailed()) {
		pQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_FAILED;
		if (pPlayer->GetActiveQuest() == apQuest)
			pPlayer->SetActiveQuest(nullptr);
	} else if (apQuest->GetCompleted()) {
		pQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_COMPLETE;
		if (pPlayer->GetActiveQuest() == apQuest)
			pPlayer->SetActiveQuest(nullptr);
	} else {
		pQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_ADD;
		if (!pPlayer->GetActiveQuest())
			pPlayer->SetActiveQuest(apQuest);
	}
	QuestUpdateManager::AddQuestNamesTail(pQuestUpdate);
	HUDShowQuestNameCritSect.Leave();
}

void HUDMainMenu::ShowEnemyDDTIcon(bool abArmorPiercing) {
	HUDMainMenu::pme->uiEnemyDDTdisplayTimer = (&appTimer)->BSTimer::GetTime();
	if ((HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 8) != 0) {
		if (abArmorPiercing) {
			HUDMainMenu::pme->pTiles[83]->Set(4003, 1);
		} else {
			HUDMainMenu::pme->pTiles[76]->Set(4003, 1);
		}
	}
}

void HUDMainMenu::SetHUDShake(float a1) {
	if (HUDMainMenu::pme) {
		HUDMainMenu::pme->fShake = (float)(a1 * ((&HUDMainMenu::fHUDShakeMult)->Float()));
	}
}

void HUDMainMenu::ClearXPMeterFinishedMoving() {
	if (HUDMainMenu::pme) {
		HUDMainMenu::pme->bXPMeterFinishedMoving = (bool)0;
	}
	g_XPBarState = 1;
}

void HUDMainMenu::InitGunScope(TESModel* apModelFilename) {
	if (HUDMainMenu::pme) {
		if (HUDMainMenu::spScopeModel.m_pObject) {
			HUDMainMenu::ClearGunScope();
			if (apModelFilename) {
				if (apModelFilename->GetModelLength()) {
					// The exe fetches the scope node before it loads the model.
					NiNode* pScopeModel = HUDMainMenu::spScopeModel.m_pObject;
					NiNode* pModel =
						pModelLoader->LoadFile(apModelFilename->GetModel(), LOD_MULT_NONE, true, false, false, false);
					pScopeModel->AttachChild(pModel, true);
					HUDMainMenu::spScopeModel.m_pObject->SetAppCulled(true);
					FNVD_CONSTRUCTED_LOCAL(NiUpdateData, UpdateData1, nullptr, 0.0f, 0, 0);
					HUDMainMenu::spScopeModel.m_pObject->Update(UpdateData1);
				}
			}
		}
	}
}

void HUDMainMenu::ForceObjectiveReminders(bool abValue) {
	if (HUDMainMenu::pme) {
		HUDMainMenu::pme->bForceObjectiveReminders = (bool)((uint8_t)abValue);
	}
}

void HUDMainMenu::ShowDDTIcon(bool abArmorPiercing) {
	HUDMainMenu::pme->uiDDTdisplayTimer = (&appTimer)->BSTimer::GetTime();
	if ((HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & 1) != 0) {
		if (abArmorPiercing) {
			HUDMainMenu::pme->pTiles[82]->Set(4003, 1);
		} else {
			HUDMainMenu::pme->pTiles[75]->Set(4003, 1);
		}
	}
}

void HUDMainMenu::SetLocationText(const HUDMainMenu::HUDTravelLocData& aLocationData) {
	// The last travel marker shown: SetLocationText stores HUDTravelLocData::pMarker.
	static TESObjectREFR* pLastLocation;
	if (HUDMainMenu::pme) {
		if (!aLocationData.bInRange || aLocationData.pMarker == pLastLocation) {
			if (aLocationData.bInRange || aLocationData.pMarker != pLastLocation) {
				if (aLocationData.bForceLastLocation) {
					pLastLocation = aLocationData.pMarker;
				}
			} else {
				pLastLocation = 0;
			}
		} else {
			pLastLocation = aLocationData.pMarker;
			if (aLocationData.pMarker) {
				if (aLocationData.pMarker->GetMapMarkerData()) {
					if (HUDMainMenu::pme->pTiles[16]->GetFadeFinished(4009) &&
						HUDMainMenu::pme->pTiles[16]->GetFloat(4009) == 0.0) {
						MapMarkerData* MapMarkerData_ = aLocationData.pMarker->GetMapMarkerData();
						const char* LocationName = MapMarkerData_->GetLocationName();
						HUDMainMenu::pme->pTiles[16]->Set(4036, LocationName, 1);
						float afEnd = fHudOpacity.Float() * 255.0;
						HUDMainMenu::pme->pTiles[16]->AddFadeControl(4009, 0.0f, afEnd, 2.0f, Tile::TFCT_STANDARD);
						uint32_t Time = appTimer.iLastTime;
						HUDMainMenu::pme->pTiles[16]->Set(Tile::kTileValue_User0, Time);
					}
					HUDMainMenu::pme->pTiles[15]->RemoveFadeControl(4009);
					HUDMainMenu::pme->pTiles[15]->Set(4009, 0.0f, 1);
				}
			}
		}
	}
}

void HUDMainMenu::QueueXPMessage(int32_t aiAddedXP) {
	int32_t Level = pPlayer->GetLevel();
	if (Level < iMaxCharacterLevel.Int() && !HUDMainMenu::pme->bXPMeterFinishedMoving) {
		HUDMainMenu::XPUpdate* a2 = static_cast<HUDMainMenu::XPUpdate*>(ourMalloc(sizeof(HUDMainMenu::XPUpdate)));
		a2->iAddedXP = aiAddedXP;
		a2->iPlayerLevel = Level;
		a2->iPlayerXP = static_cast<int32_t>(pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS));
		HUDMainMenu::pme->xXPUpdates.AddHead(a2);
	}
}

void HUDMainMenu::SetVisibilityFlag(int32_t a1, int32_t a2) {
	if (HUDMainMenu::pme) {
		if (a2) {
			HUDMainMenu::pme->iHUDVisibilityUserSettings |= a1;
		} else {
			HUDMainMenu::pme->iHUDVisibilityUserSettings &= ~a1;
		}
	}
}

// The menu GetMenuByClass finds for the HUD's ID is the HUD itself; the exe writes its fields through it.
void HUDMainMenu::Revert() {
	int32_t MenuID_0 = HUDMainMenu::GetMenuID();
	Tile* MenuByClass = Menu::GetMenuByClass(MenuID_0);
	if (MenuByClass) {
		if (MenuByClass->GetMenu()) {
			HUDMainMenu* pHUD = static_cast<HUDMainMenu*>(MenuByClass->GetMenu());
			pHUD->pCurrentEnemy = nullptr;
			pHUD->uiQuestReminderTimer = 0;
			pHUD->uiDDTdisplayTimer = -1;
			pHUD->uiEnemyDDTdisplayTimer = -1;
			pHUD->bForceObjectiveReminders = 0;
			pHUD->pTiles[50]->Set(4003, 0);
			pHUD->bXPMeterFinishedMoving = 0;
			HUDMainMenu::pme->pTiles[44]->Set(4003, 0);
			HUDMainMenu::pme->pTiles[45]->Set(4003, 0);
			HUDMainMenu::pme->pTiles[46]->Set(4003, 0);
			HUDMainMenu::pme->pTiles[47]->Set(4003, 0);
			HUDMainMenu::pme->pTiles[48]->Set(4003, 0);
			HUDMainMenu::pme->pTiles[49]->Set(4003, 0);
			g_XPBarState = 1;
			HUDMainMenu::pme->bKillQuestUpdates = 1;
		}
	}
}

double HUDMainMenu::GetCompassAngle(NiPoint3 a1, NiPoint3 a2) {
	NiPoint3 result;
	result = a1 - a2;
	float UnclampedZAngleFromVector = GetUnclampedZAngleFromVector(result);
	float v5 = 0.0;
	v5 = UnclampedZAngleFromVector - pPlayer->GetHeading(0);
	if (!(v5 <= -3.1415927410125732)) {
		if (v5 > 3.1415927410125732) {
			v5 = 6.2831854820251465 - v5;
		}
	} else {
		v5 = v5 + 6.2831854820251465;
	}
	v5 = v5 * 57.2957763671875;
	return static_cast<float>(v5 * 1.4500000476837158);
}

HUDMainMenu::SubtitleData::SubtitleData() {}

BSTimer::BSTimer() {
	Init(BSGetTickCount());
}

int32_t TESWaterForm::GetHealthRestoration() {
	return iHealthRestoration;
}

bool Projectile::IsEditorCreated() {
	return !pDataHandler->IsFormIDCreated(GetFormID());
}
