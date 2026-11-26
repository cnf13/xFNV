#include "Fallout/Interface/Menus/HUDMainMenu.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSFurnitureMarker.h"
#include "BSMain/BSSceneGraph.h"
#include "BSMain/BSTaskManager.h"
#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"
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
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/rtti.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"

#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>

// UpdateWeaponStatus's function-local statics: the last shown loaded and remaining ammo and condition percentage
// (FLT_MIN at load: nothing shown yet), whether the bar's pulse is fading down, and the weapon last shown. The three
// text buffers are the ammo type text (cleared every call, then the abbreviation) and the ammo count text.
static float flastWeapHealthPercent = std::numeric_limits<float>::min();
static bool bWeapFadeDecrease = true;
static float flastTotalWeaponAmmo = std::numeric_limits<float>::min();
static float flastWeaponAmmo = std::numeric_limits<float>::min();

// UpdateHotkeysWheel's statics: the hotkey whose name and icon the HUD shows (-1: none), the timer's time when a hotkey
// was last pressed or the wheel last used (0: not timing) and whether a gamepad hotkey is being held.
static int32_t ilastHighlight = -1;

// The scale Draw3DElements passes for the scope camera's Eyefinity adjustment.
static float fScopeEyeFinityScale = 0.1f;

// Set while the enemy health meter is hidden; SetMenuMode then drops it from every mode.
static bool bHideEnemyHealthMeter;

// Whether the compass shows the current enemy as a hostile, detected target.
static bool bCurrentEnemyHostile;

// DoIdle's last GetActivationIsCrime of the crosshair reference (the map's name).
static bool bCheckSneakChanged;

using AmmoTextBuffer = char[260];
static AmmoTextBuffer ammo{};
static TESObjectWEAP* plastWeaponObj;
static AmmoTextBuffer ammosubtype2{};
static AmmoTextBuffer ammosubtype{};

// SetInfoForRef's function-local text buffers: the item weight (wg), the item value (caps), the crosshair reference's
// name as it last built it, and the lock state.
using PickupPromptString = char[8];
static PickupPromptString wg{};
static PickupPromptString caps{};
using InfoString = char[0x104];
static InfoString g_sCrosshairRefName{};
static InfoString lockLevel{};
static int32_t iHotkeyStartTime;
static bool bwheelBeingUsed;

// UpdateShake's function-local static: whether the last frame shook the tile.
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

using HUDMainMenuSubtitleArray = BSSimpleArray<HUDMainMenu::SubtitleData*, 1024>;

// The victim's damage threshold after both actors' perk entry points, at least 0.
float GetAdjustedDamageThreshold(Actor* pAttacker, Actor* pVictim) {
    if (!pAttacker || !pVictim) {
        return 0.0f;
    }
    float fThreshold = BSMAX(
        static_cast<ActorValueOwner*>(pVictim)->GetActorFloatValue(ActorValue::DAMAGE_THRESHOLD),
        0.0f
    );
    BaseProcess* pProcess = pAttacker->GetCurrentAIProcess();
    ItemChange* pWeaponChange = pProcess->GetCurrentWeapon();
    TESForm* pWeapon;
    if (pWeaponChange) {
        pWeapon = pWeaponChange->GetContainerObject();
    }
    else {
        pWeapon = pFists;
    }
    float fAttackerMod = 0.0f;
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_ATTACKER,
        pAttacker,
        pWeapon,
        pVictim,
        &fAttackerMod
    );
    float fDefenderMod = 0.0f;
    BGSEntryPoint::HandleEntryPoint(
        BGSEntryPoint::ADJUST_DAMAGE_THRESHOLD_DEFENDER,
        pVictim,
        pAttacker,
        pWeapon,
        &fDefenderMod
    );
    fThreshold = fThreshold - fAttackerMod + fDefenderMod;
    fThreshold = BSMAX(0.0f, fThreshold);
    return fThreshold;
}

void HandleQuestObjectiveBlinking(Tile* th_s, float value, float afMaxAlpha) {
    float fTimeStart;
    float fTimeEnd;
    float fDistStart;
    float fDistEnd;
    static int32_t iUT_AlphaDown = Tile::AddUserTrait("_AlphaDown", -1);
    int32_t iMaxDistSqr = SquareInt(iHUDCompassIconBlinkMaxDist.Int());
    int32_t iMinDistSqr = SquareInt(iHUDCompassIconBlinkMinDist.Int());
    int32_t iThresholdDistSqr = SquareInt(iHUDCompassIconBlinkThresholdDist.Int());
    if (value > static_cast<double>(iMaxDistSqr)) {
        value = static_cast<float>(iMaxDistSqr);
    }
    else if (value < static_cast<double>(iMinDistSqr)) {
        value = static_cast<float>(iMinDistSqr);
    }
    // Tests below-threshold once (false on NaN) and keeps the result for both lerps.
    bool bBelowThreshold = value < static_cast<double>(iThresholdDistSqr);
    float fAlphaDown = th_s->GetFloat(iUT_AlphaDown);
    if (bBelowThreshold) {
        fDistEnd = static_cast<float>(iMinDistSqr);
        fDistStart = static_cast<float>(iThresholdDistSqr);
        fTimeEnd = static_cast<float>(iHUDCompassIconBlinkFastestBlinkTime.Int());
        fTimeStart = static_cast<float>(iHUDCompassIconBlinkThresholdBlinkTime.Int());
    }
    else {
        fDistEnd = static_cast<float>(iThresholdDistSqr);
        fDistStart = static_cast<float>(iMaxDistSqr);
        fTimeEnd = static_cast<float>(iHUDCompassIconBlinkThresholdBlinkTime.Int());
        fTimeStart = static_cast<float>(iHUDCompassIconBlinkSlowestBlinkTime.Int());
    }
    float fBlinkTime = FLerp(fTimeStart, fTimeEnd, fDistStart, fDistEnd, value);
    if (fAlphaDown == 0.0) {
        double dNewAlpha;
        if (fCurrentHUDOpacity == 0.0) {
            dNewAlpha = 0.0;
        }
        else {
            double dCurrent = th_s->GetFloat(4009);
            double dStep = fBlinkTime * afMaxAlpha / fCurrentHUDOpacity;
            dNewAlpha = appTimer.fDelta * dStep + dCurrent;
        }
        float fAlpha = dNewAlpha;
        th_s->Set(4009, fAlpha, true);
    }
    else if (fAlphaDown == 1.0) {
        double dNewAlpha;
        if (fCurrentHUDOpacity == 0.0) {
            dNewAlpha = 0.0;
        }
        else {
            double dCurrent = th_s->GetFloat(4009);
            double dStep = fBlinkTime * afMaxAlpha / fCurrentHUDOpacity;
            dNewAlpha = dCurrent - appTimer.fDelta * dStep;
        }
        float fAlpha = dNewAlpha;
        th_s->Set(4009, fAlpha, true);
    }
    else {
        float fAlpha = 0.0;
        if (fAlphaDown == 3.0) {
            fAlpha = fCurrentHUDOpacity;
        }
        th_s->Set(4009, fAlpha, true);
    }
    float fCurrentAlpha = th_s->GetFloat(4009);
    if (afMaxAlpha < static_cast<double>(fCurrentAlpha)) {
        th_s->Set(4009, afMaxAlpha, true);
        th_s->Set(iUT_AlphaDown, 3);
    }
    else if (fCurrentAlpha < 0.0) {
        if (bBelowThreshold) {
            fDistEnd = static_cast<float>(iMinDistSqr);
            fDistStart = static_cast<float>(iThresholdDistSqr);
            fTimeEnd = static_cast<float>(iHUDCompassIconBlinkShortestPause.Int());
            fTimeStart = static_cast<float>(iHUDCompassIconBlinkThresholdPause.Int());
        }
        else {
            fDistEnd = static_cast<float>(iThresholdDistSqr);
            fDistStart = static_cast<float>(iMaxDistSqr);
            fTimeEnd = static_cast<float>(iHUDCompassIconBlinkThresholdPause.Int());
            fTimeStart = static_cast<float>(iHUDCompassIconBlinkLongestPause.Int());
        }
        float fPause = FLerp(fTimeStart, fTimeEnd, fDistStart, fDistEnd, value);
        th_s->Set(4009, 0.0f, true);
        th_s->Set(iUT_AlphaDown, 0);
        float fPauseEnd = static_cast<double>(appTimer.iLastTime) + fPause;
        th_s->Set(Tile::kTileValue_User10, fPauseEnd, true);
    }
    else {
        double dStoredPauseEnd = th_s->GetFloat(Tile::kTileValue_User10);
        if (static_cast<double>(appTimer.iLastTime) > dStoredPauseEnd && fAlphaDown == 3.0) {
            th_s->Set(iUT_AlphaDown, 1);
        }
    }
}

int32_t SquareInt(int32_t aiValue) {
    return aiValue * aiValue;
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

int32_t QuestUpdateManager::SortQuestNamesByPriorityFunction(
    QuestUpdateManager::QuestUpdate* pQU1,
    QuestUpdateManager::QuestUpdate* pQU2
) {
    if (pQU1->CustomQueuePriority > pQU2->CustomQueuePriority) {
        return 1;
    }
    if (pQU1->CustomQueuePriority >= pQU2->CustomQueuePriority) {
        return 0;
    }
    return -1;
}

void QuestUpdateManager::SetCustomQuestText(
    const char* apCustomTextTitle,
    const char* apCustomTextSubtitle,
    QuestUpdateManager::HUD_CUSTOM_QUEST_QUEUE_PRIORITY aiCustomQueuePriority,
    QuestUpdateManager::HUD_CUSTOM_QUEST_TEXT_JUSTIFICATION aiCustomTextJustification,
    FONTS aiCustomTextTitleFont,
    FONTS aiCustomTextSubtitleFont,
    const char* apCustomSoundHandleName
) {
    QuestUpdateManager::QuestUpdate* pCustomQuestUpdate = static_cast<QuestUpdateManager::QuestUpdate*>(
        ourMalloc(sizeof(QuestUpdateManager::QuestUpdate))
    );
    pCustomQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_CUSTOM;
    BSstrcpy(pCustomQuestUpdate->CustomTextTitle, 260, apCustomTextTitle);
    BSstrcpy(pCustomQuestUpdate->CustomTextSubtitle, 260, apCustomTextSubtitle);
    pCustomQuestUpdate->CustomTextJustification = aiCustomTextJustification;
    pCustomQuestUpdate->CustomQueuePriority = aiCustomQueuePriority;
    if (aiCustomTextTitleFont == FONT_1) {
        Warning(
            WARN_QUESTS,
            "QuestUpdateManager::SetCustomQuestText() - FONT_1 is an invalid value for aiCustomTextTitleFont.  Setting "
            "to FONT_NONE."
        );
        aiCustomTextTitleFont = FONT_NONE;
    }
    pCustomQuestUpdate->CustomTextTitleFont = aiCustomTextTitleFont;
    if (aiCustomTextSubtitleFont == FONT_1) {
        Warning(
            WARN_QUESTS,
            "QuestUpdateManager::SetCustomQuestText() - FONT_1 is an invalid value for aiCustomTextSubtitleFont.  "
            "Setting to FONT_NONE."
        );
        aiCustomTextSubtitleFont = FONT_NONE;
    }
    pCustomQuestUpdate->CustomTextSubtitleFont = aiCustomTextSubtitleFont;
    BSstrcpy(pCustomQuestUpdate->CustomSoundHandleName, 260, apCustomSoundHandleName);
    if (aiCustomQueuePriority) {
        if (aiCustomQueuePriority <= QuestUpdateManager::HCQQP_NOW ||
            aiCustomQueuePriority > QuestUpdateManager::HCQQP_LAST) {
            Warning(
                WARN_QUESTS,
                "QuestUpdateManager::SetCustomQuestText() - Invalid aiCustomQueuePriority (%i)!  Setting to "
                "HCQQP_NORMAL.",
                aiCustomQueuePriority
            );
            QuestUpdateManager::AddQuestNamesTail(pCustomQuestUpdate);
        }
        else {
            QuestUpdateManager::AddQuestNamesTail(pCustomQuestUpdate);
        }
    }
    else {
        QuestUpdateManager::AddQuestNamesHead(pCustomQuestUpdate);
    }
}

void QuestUpdateManager::AddQuestNamesHead(QuestUpdateManager::QuestUpdate* pQuestUpdate) {
    QuestUpdateManager::xQuestNames.AddHead(pQuestUpdate);
    QuestUpdateManager::xQuestNames.Sort(&QuestUpdateManager::SortQuestNamesByPriorityFunction);
}

HUDMainMenu::HUDMainMenu()
  : Menu(), pTiles{}, pActive(nullptr), fSeenLevel(0.0f), uiMessageStartTime(0), pCurrSubtitle(nullptr),
    pLastObject(nullptr), iHUDVisibilityUserSettings((bCrossHair ? 64 : 0) | 0x1FFBF), eMenuMode(HMM_START),
    bScopeVisible(false), pBreathMeterAnim(nullptr), bXPMeterFinishedMoving(false), bForceObjectiveReminders(false),
    pQuestList(nullptr), pCurrentEnemy(nullptr), iEnemyHealthTimer(0), uiQuestReminderTimer(appTimer.iLastTime),
    uiDDTdisplayTimer(0xFFFFFFFFu), uiEnemyDDTdisplayTimer(0xFFFFFFFFu), bKillQuestUpdates(false),
    bQuestNameShowing(false), bObjectiveNameShowing(false), fShake(0.0f) {
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
    if (aiID > 0 && aiID <= 86) {
        this->pTiles[aiID] = apTile;
    }
}

// Elapsed times are unsigned timer differences; the subtitle and message ones are widened to double and compared
// against the duration in milliseconds.
void HUDMainMenu::DoIdle() {
    if (!pPlayer || !pPlayer->GetParentCell()) {
        return;
    }

    this->HudTimer.Update();
    HUDMainMenu::UpdateCompass(
        this->pTiles[HMMT_COMPASS],
        this->pTiles[HMMT_COMPASS_GENERAL_ICONS],
        this->pTiles[HMMT_COMPASS_MARKER_ICONS],
        this->pTiles[HMMT_COMPASS_NPC_ICONS]
    );
    this->UpdateHotkeysWheel();
    this->UpdateWeaponStatus();
    this->UpdateExplosiveIndicator();
    this->UpdateShake();

    // The HUD timer time of the last refresh.
    static uint32_t uiLastUpdate = this->HudTimer.GetTime();
    // The rest runs at most every 200 ms of HUD time.
    if (this->HudTimer.GetTime() - uiLastUpdate < 200) {
        return;
    }

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

    // The screen sizes are truncated to 64 bits and the low 32 are kept.
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
            HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->AddFadeControl(
                Tile::kTileValue_Alpha,
                HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->GetFloat(Tile::kTileValue_Alpha),
                0.0f,
                2.0f,
                Tile::TFCT_STANDARD
            );
        }
    }
    if (HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_Alpha) > 0.0 &&
        HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
        const double fNow = appTimer.GetTime();
        if (fNow - HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_User0) > 4000.0) {
            const float fOpacity = fHudOpacity.Float() * 255.0;
            HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]
                ->AddFadeControl(Tile::kTileValue_Alpha, fOpacity, 0.0f, 2.0f, Tile::TFCT_STANDARD);
        }
    }
    // A held region text fades back in once the location text is gone, and is stamped and released.
    if (HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->IsTrue(Tile::TextToTrait("_Hold")) &&
        HUDMainMenu::pme->pTiles[HMMT_REGION_LOCATION]->GetFloat(Tile::kTileValue_Visible) != 0.0 &&
        HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_Alpha) == 0.0 &&
        HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
        const float fOpacity = fHudOpacity.Float() * 255.0;
        HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]
            ->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fOpacity, 2.0f, Tile::TFCT_STANDARD);
        HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::kTileValue_User0, appTimer.GetTime());
        HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::TextToTrait("_Hold"), 0);
    }

    if (pPlayer->IsSneaking()) {
        this->UpdateSneak();
    }
    else if (
        HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->GetFloat(Tile::kTileValue_Alpha) > 0.0 &&
        HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)
    ) {
        HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->AddFadeControl(
            Tile::kTileValue_Alpha,
            HUDMainMenu::pme->pTiles[HMMT_SNEAK_TEXT]->GetFloat(Tile::kTileValue_Alpha),
            0.0f,
            0.5f,
            Tile::TFCT_STANDARD
        );
    }

    if (HUDMainMenu::pme->eMenuMode != HMM_SPECIAL_BOOK && HUDMainMenu::pme->eMenuMode != HMM_LOVE_TESTER) {
        this->UpdateQuestText();
    }
    this->UpdateXPMeter();
    this->UpdateRadiationMeter();
    this->UpdateStatusModeUI(pPlayer->IsHardcore());

    // The enemy counts as hostile while the compass shows it as a hostile, detected target.
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
    // 57.2957763671875 is the float radians-to-degrees factor widened to double.
    const float fPlayerHeading = pPlayer->GetHeading(false) * 57.2957763671875;
    float fEnemyHeading = 0.0f;
    if (this->pCurrentEnemy) {
        const NiPoint3* pEnemyLocation = &this->pCurrentEnemy->GetLocationOnReference();
        fEnemyHeading = HUDMainMenu::HeadingBetweenLocations(pPlayer->GetLocationOnReference(), *pEnemyLocation);
    }
    // Within fEnemyHealthBarTimer seconds of the last showing, or while the player faces the enemy (within 45 degrees).
    // The limit is truncated to a 64-bit integer; the compare uses its low 32 bits, unsigned.
    const uint32_t uiSinceEnemyHealth = this->HudTimer.GetTime() - this->iEnemyHealthTimer;
    const uint32_t uiEnemyHealthLimit = static_cast<uint32_t>(
        static_cast<int64_t>(fEnemyHealthBarTimer.Float() * 1000.0)
    );
    const bool bFacingEnemy = uiSinceEnemyHealth <= uiEnemyHealthLimit ||
        std::fabs(fPlayerHeading - fEnemyHeading) < 45.0;

    if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
        if (pVATSTarget && pVATSTarget->IsActor() && !pVATSTarget->IsDead(false)) {
            bShowEnemyHealth = true;
            HUDMainMenu::SetEnemyActor(static_cast<Actor*>(pVATSTarget));
        }
    }
    else if (
        this->pCurrentEnemy &&
        !this->pCurrentEnemy->IsDead(false) &&
        !this->pCurrentEnemy->GetDestroyed() &&
        bCurrentEnemyHostile &&
        !HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->IsTrue(Tile::kTileValue_Visible) &&
        !HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->IsTrue(Tile::kTileValue_Visible) &&
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(Tile::kTileValue_SystemColor) == 2.0 &&
        bFacingEnemy
    ) {
        bShowEnemyHealth = true;
        HUDMainMenu::SetInfoForRef(this->pLastObject, false, true);
    }
    else if (HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(Tile::kTileValue_SystemColor) == 2.0) {
        this->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 0);
        this->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 0);
    }

    if (bShowEnemyHealth) {
        bool bHostile = false;
        if (this->pCurrentEnemy &&
            ((pPlayer->IsActorPerceived(this->pCurrentEnemy) &&
              pPlayer->IsActorPerceived(this->pCurrentEnemy)->bIsHostile) ||
             this->pCurrentEnemy->IsInCombatWithActor(pPlayer))) {
            bHostile = true;
        }

        this->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 1);
        this->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 1);
        this->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 1);
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(Tile::kTileValue_SystemColor, bHostile ? 2 : 1);
        // Flags the name tile twice.
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->AddNeedsUpdate(8);
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_METER]->AddNeedsUpdate(8);
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->AddNeedsUpdate(8);
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->AddNeedsUpdate(8);
    }

    if (this->pCurrentEnemy && (this->pCurrentEnemy->IsInCombat() || pVATS.GetMode() == VATS::VATS_PLAYBACK)) {
        this->UpdateEnemyHealth(this->pCurrentEnemy);
    }

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
            HUDMainMenu::pme->eMenuMode != HMM_SPECIAL_BOOK &&
            HUDMainMenu::pme->eMenuMode != HMM_LOVE_TESTER;
    }
    if (bReminderDue) {
        this->uiQuestReminderTimer = appTimer.GetTime();
        HUDMainMenu::DisplayQuestReminder(pPlayer->GetActiveQuest());
    }
    else if (this->uiQuestReminderTimer == 0 && HUDMainMenu::GetMenuMode() == HMM_RUN_AND_GUN) {
        this->uiQuestReminderTimer = appTimer.GetTime();
        HUDMainMenu::DisplayQuestReminder(pPlayer->GetActiveQuest());
        this->bKillQuestUpdates = false;
    }

    if (this->bKillQuestUpdates) {
        this->KillQuestName();
        this->KillQuestObjectives();
        this->bKillQuestUpdates = false;
    }
    else if (!this->xSubtitles.IsEmpty()) {
        // Expired subtitles are freed and dropped.
        for (uint32_t i = 0; i < this->xSubtitles.iSize; ++i) {
            const double fElapsed = this->HudTimer.GetTime() - this->xSubtitles.GetAt(i)->uiSubtitleStartTime;
            if (this->xSubtitles.GetAt(i)->fDurationSecs * 1000.0 <= fElapsed) {
                if (this->xSubtitles.GetAt(i) == this->pCurrSubtitle) {
                    this->HideSubtitle();
                }
                SubtitleData* pSubtitle = this->xSubtitles.GetAt(i);
                ourFree(pSubtitle);
                this->xSubtitles.Remove(i, true);
                --i;
            }
        }
        if (!this->pCurrSubtitle) {
            this->pCurrSubtitle = this->GetNextSubtitle();
            if (this->pCurrSubtitle) {
                HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]
                    ->Set(Tile::kTileValue_String, this->pCurrSubtitle->strText, true);
                HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(Tile::kTileValue_Visible, 1);
            }
        }
    }

    // A queued message fades in over 0.35 seconds and is hidden once its duration has run out.
    if (this->xMessages.ItemsInList() && HUDMainMenu::pme->pTiles[HMMT_MESSAGE]->IsTrue(Tile::kTileValue_Visible)) {
        if (HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
            HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]
                ->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity, 0.35f, Tile::TFCT_STANDARD);
            HUDMainMenu::pme->pTiles[HMMT_MESSAGE_ICON]
                ->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity, 0.35f, Tile::TFCT_STANDARD);
            HUDMainMenu::pme->pTiles[HMMT_MESSAGE_TEXT]
                ->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity, 0.35f, Tile::TFCT_STANDARD);
        }
        if (HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]->GetFadeFinished(Tile::kTileValue_Alpha)) {
            MessageData* pMessage = this->xMessages.GetItem();
            if (this->uiMessageStartTime == 0) {
                this->uiMessageStartTime = this->HudTimer.GetTime();
            }
            const double fElapsed = this->HudTimer.GetTime() - this->uiMessageStartTime;
            if (pMessage->fDurationSecs * 1000.0 <= fElapsed) {
                this->HideMessage();
            }
        }
    }

    fCurrentHUDOpacity = fHudOpacity.Float() * 255.0;
}

int32_t HUDMainMenu::GetClass() {
    return 1004;
}

bool HUDMainMenu::ShowMessage(
    const char* apString,
    HUDMainMenu::HUD_MESSAGE_EMOTION aeEmotion,
    const char* apIconFilename,
    const char* apSoundName,
    float afShowSeconds,
    bool abMoveToHead
) {
    if (apString) {
        HUDMainMenu::MessageData* pNewMessage = static_cast<HUDMainMenu::MessageData*>(
            ourMalloc(sizeof(HUDMainMenu::MessageData))
        );
        BSstrcpy(pNewMessage->strText, 260, apString);
        pNewMessage->fDurationSecs = afShowSeconds;
        if (apIconFilename && *apIconFilename) {
            BSstrcpy(pNewMessage->IconFilename, 260, apIconFilename);
        }
        else {
            switch (aeEmotion) {
                case HUDMainMenu::HME_NEUTRAL: {
                    BSstrcpy(
                        pNewMessage->IconFilename,
                        260,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_neutral.dds"
                    );
                    break;
                }
                case HUDMainMenu::HME_HAPPY: {
                    BSstrcpy(
                        pNewMessage->IconFilename,
                        260,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_very_happy.dds"
                    );
                    break;
                }
                case HUDMainMenu::HME_SAD: {
                    BSstrcpy(
                        pNewMessage->IconFilename,
                        260,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_sad.dds"
                    );
                    break;
                }
                case HUDMainMenu::HME_PAIN: {
                    BSstrcpy(
                        pNewMessage->IconFilename,
                        260,
                        "Interface\\Icons\\Message Icons\\glow_message_vaultboy_in_pain.dds"
                    );
                    break;
                }
                default: {
                    break;
                }
            }
        }
        if (apSoundName) {
            BSstrcpy(pNewMessage->SoundName, 260, apSoundName);
        }
        else {
            pNewMessage->SoundName[0u] = 0;
        }
        if (xMessages.IsEmpty()) {
            xMessages.AddTail(pNewMessage);
            HUDMainMenu::pme->pTiles[26]->Set(4036, xMessages.m_item->strText, true);
            HUDMainMenu::pme->pTiles[25]->Set(4044, xMessages.m_item->IconFilename, true);
            if (apSoundName) {
                BSAudio::QInstance().GetSoundHandleByName(apSoundName, 289u).Play(false);
            }
        }
        else {
            BSSimpleList<HUDMainMenu::MessageData*>* pTailMessage;
            for (pTailMessage = &xMessages; pTailMessage->m_pkNext; pTailMessage = pTailMessage->m_pkNext) {
                ;
            }
            if (BSstrcmp(pTailMessage->m_item->strText, pNewMessage->strText)) {
                if (abMoveToHead) {
                    xMessages.m_item->fDurationSecs = 0.0f;
                    xMessages.AddHead(pNewMessage);
                }
                else {
                    xMessages.AddTail(pNewMessage);
                }
            }
            else {
                ourFree(pNewMessage);
            }
        }
        return true;
    }
    return false;
}

bool HUDMainMenu::IsWellFormedMenu() {
    for (int32_t i = 1; i < 86; ++i) {
        if (!this->pTiles[i]) {
            return false;
        }
    }
    return true;
}

NiCamera* HUDMainMenu::Create3DCamera() {
    NiCamera* pCamera = new NiCamera;
    // Builds the viewport with NiColorA's constructor.
    NiRect<float> kPort;
    ::new (&kPort) NiColorA(0.0f, 1.0f, 1.0f, 0.0f);
    if (BSShaderManager::bLetterbox) {
        kPort = BSShaderManager::kFramebufferViewport;
    }
    pCamera->SetViewPort(kPort);
    NiFrustum kFrustum(false);
    kFrustum.m_fFar = Main::GetWorldSceneGraph()->GetFarDistance();
    kFrustum.m_fNear = BSSceneGraph::fNearDistance.Float();
    pCamera->SetMaxFarNearRatio(kFrustum.m_fFar / kFrustum.m_fNear);
    int32_t iHeight;
    if (BSShaderManager::bLetterbox) {
        iHeight = BSShaderManager::iLetterboxHeight;
    }
    else {
        iHeight = Renderer::GetScreenHeight();
    }
    const double dHeight = iHeight;
    const float fAspect = static_cast<float>(dHeight / static_cast<double>(Renderer::GetScreenWidth()));
    // The FOV factors are the double pi/180 and the float 0.15 widened to double.
    const float fScale = 0.75f;
    const float fHalfFOV = static_cast<float>(
        BSSceneGraph::fDefaultFOV.Float() * 0.01745329238474369 * 0.15000000596046448
    );
    kFrustum.m_fLeft = -std::tan(fHalfFOV) * fScale;
    kFrustum.m_fRight = std::tan(fHalfFOV) * fScale;
    kFrustum.m_fBottom = -std::tan(fHalfFOV) * fAspect * fScale;
    kFrustum.m_fTop = std::tan(fHalfFOV) * fAspect * fScale;
    kFrustum.m_bOrtho = false;
    pCamera->SetViewFrustum(kFrustum);
    pCamera->LookAtWorldPoint(NiPoint3(0.0f, 0.0f, 1.0f), NiPoint3(0.0f, 0.0f, 1.0f));
    pCamera->SetMinNearPlaneDist(1.0f);
    NiUpdateData kUpdateData(0.0f, false, false);
    pCamera->Update(kUpdateData);
    return pCamera;
}

// The subtitle tile is the HUD singleton's; the cleared subtitle is this menu's.
void HUDMainMenu::HideSubtitle() {
    if (!Interface::IsMenuIDVisible(Interface::eStartMenu, 0)) {
        HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(Tile::kTileValue_String, " ", true);
    }
    HUDMainMenu::pme->pTiles[HMMT_SUBTITLE_TEXT]->Set(Tile::kTileValue_Visible, 0);
    this->pCurrSubtitle = nullptr;
}

bool HUDMainMenu::ShowSubtitle(
    const char* apString,
    BSSoundHandle ahandle,
    NiPoint3 aSpeakerLoc,
    TESObjectREFR* apTarget,
    bool abForceShow
) {
    bool bResult = false;
    Actor* pActor;
    if (apTarget && apTarget->IsActor()) {
        pActor = dynamic_cast<Actor*>(apTarget);
    }
    else {
        pActor = nullptr;
    }
    if (apString && (bGeneralSubtitles.Bool() || (pActor && static_cast<uint8_t>(pActor->GetShowSubtitles())))) {
        HUDMainMenu::SubtitleData* pNewSubtitle = new HUDMainMenu::SubtitleData;
        BSstrcpy(pNewSubtitle->strText, 260, apString);
        pNewSubtitle->SpeakerLoc = aSpeakerLoc;
        pNewSubtitle->fDurationSecs = static_cast<double>(ahandle.GetDuration()) / 1000.0;
        if (pNewSubtitle->fDurationSecs == 0.0) {
            double dLength = static_cast<double>(std::strlen(apString));
            pNewSubtitle->fDurationSecs = fNoticeTextTimePerCharacter.Float() * dLength;
        }
        if (abForceShow) {
            pNewSubtitle->iPriority = std::numeric_limits<int32_t>::max();
        }
        else if (apTarget == pPlayer) {
            pNewSubtitle->iPriority = 0;
        }
        else if (apTarget) {
            pNewSubtitle->iPriority = 2;
        }
        else {
            pNewSubtitle->iPriority = 1;
        }
        pNewSubtitle->uiSubtitleStartTime = HudTimer.iLastTime;
        xSubtitles.Add(pNewSubtitle);
        bResult = true;
    }

    return bResult;
}

void HUDMainMenu::ClearAllSubtitlesAndMessages() {
    for (BSSimpleList<MessageData*>* pNode = &this->xMessages; pNode && !pNode->IsEmpty(); pNode = pNode->GetNext()) {
        ourFree(pNode->GetItem());
    }
    this->xMessages.RemoveAll();
    HUDMainMenu::pme->pTiles[HMMT_MESSAGE_BRACKET]->Set(Tile::kTileValue_Alpha, 0.0f, true);
    HUDMainMenu::pme->pTiles[HMMT_MESSAGE_TEXT]->Set(Tile::kTileValue_String, " ", true);
    HUDMainMenu::pme->pTiles[HMMT_MESSAGE_TEXT]->Set(Tile::kTileValue_Alpha, 0.0f, true);
    HUDMainMenu::pme->pTiles[HMMT_MESSAGE_ICON]->Set(Tile::kTileValue_Alpha, 0.0f, true);
    this->HideSubtitle();
}

// The elapsed times are the unsigned timer difference widened to double, compared against the setting in milliseconds.
void HUDMainMenu::UpdateHotkeysWheel() {
    if (InterfaceManager::Instance()->IsInMenuMode() ||
        pPlayer->IsDead(false) ||
        pPlayer->GetControlsDisabled(PlayerCharacter::DPC_PIPBOY)) {
        iHotkeyStartTime = 0;
        this->HKWheel.SetVisible(false);
        this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 0);
    }
    else if (InterfaceManager::Has360Controller()) {
        const int32_t iHotkey = this->HKWheel.MoveHighlight(-1);
        if (!bwheelBeingUsed && iHotkey >= 0) {
            this->HKWheel.UpdateHotkeyList();
            bwheelBeingUsed = true;
            iHotkeyStartTime = static_cast<int32_t>(appTimer.GetTime());
        }
        if (bwheelBeingUsed) {
            if (iHotkey >= 0) {
                if (iHotkeyStartTime > 0) {
                    const double dElapsed = static_cast<double>(
                        appTimer.GetTime() - static_cast<uint32_t>(iHotkeyStartTime)
                    );
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
                    ItemChange* pItem = pChanges ? pChanges->GetHotKeyItem(iHotkey) : nullptr;
                    bool bHasIcon = false;
                    const char* pIcon = nullptr;
                    if (pItem) {
                        char acName[260];
                        pIcon = pItem->GetIconOne(nullptr);
                        if (pIcon) {
                            bHasIcon = true;
                        }
                        if (pItem->GetCount() > 1) {
                            BSsprintf(acName, 260, "%s (%d)", pItem->GetFullName(), pItem->GetCount());
                        }
                        else {
                            BSstrcpy(acName, 260, pItem->GetFullName());
                        }
                        this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_String, acName, true);
                        this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 1);
                    }
                    else {
                        this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 0);
                    }
                    if (bHasIcon) {
                        this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconFilename"), pIcon, true);
                        this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 1);
                    }
                    else {
                        this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 0);
                    }
                    ilastHighlight = iHotkey;
                    if (pItem) {
                        delete pItem;
                    }
                }
            }
            else {
                this->HKWheel.UseHotkeyItem();
                this->HKWheel.SetVisible(false);
                this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_Visible, 0);
                iHotkeyStartTime = 0;
                bwheelBeingUsed = false;
                ilastHighlight = -1;
            }
        }
    }
    else {
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
                ItemChange* pItem = pChanges ? pChanges->GetHotKeyItem(i) : nullptr;
                const char* pIcon = nullptr;
                if (pItem) {
                    char acName[260];
                    pIcon = pItem->GetIconOne(nullptr);
                    if (pIcon) {
                        bHasIcon = true;
                    }
                    if (pItem->GetCount() > 1) {
                        BSsprintf(acName, 260, "%s (%d)", pItem->GetFullName(), pItem->GetCount());
                    }
                    else {
                        BSstrcpy(acName, 260, pItem->GetFullName());
                    }
                    this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_String, acName, true);
                }
                else {
                    this->pTiles[HMMT_HOTKEY_TEXT]->Set(Tile::kTileValue_String, " ", true);
                }
                if (bHasIcon) {
                    this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconFilename"), pIcon, true);
                    this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 1);
                }
                else {
                    this->pTiles[HMMT_HOTKEY_WHEEL]->Set(Tile::TextToTrait("_IconVisible"), 0);
                }
                if (pItem) {
                    delete pItem;
                }
            }
            else if (appMain->GetControls()->GetUserAction(eAction, Controls::isHeld)) {
                if (!this->HKWheel.GetVisible() && iHotkeyStartTime > 0) {
                    const double dElapsed = static_cast<double>(
                        appTimer.GetTime() - static_cast<uint32_t>(iHotkeyStartTime)
                    );
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
                if (this->HKWheel.GetVisible()) {
                    iHotkeyStartTime = static_cast<int32_t>(appTimer.GetTime());
                }
            }
            else if (appMain->GetControls()->GetUserAction(eAction, Controls::isDepressed)) {
                this->HKWheel.UseHotkeyItem();
                if (!this->HKWheel.GetVisible()) {
                    iHotkeyStartTime = 0;
                }
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

// The no-weapon branch hides the ammo text and the condition text of the HUD singleton (pme), the meter tiles of this
// menu. Counts subtract from the loaded rounds in double.
void HUDMainMenu::UpdateWeaponStatus() {
    if (!pPlayer || !pPlayer->GetCurrentAIProcess()) {
        return;
    }
    const float fLocalHudOpacity = fHudOpacity.Float() * 255.0;
    ItemChange* pweap = pPlayer->GetCurrentAIProcess()->GetCurrentWeapon();
    static float fWeapCondBarAlpha = fLocalHudOpacity;
    BSsprintf(ammosubtype, sizeof(ammosubtype), "%s", "");
    this->pTiles[HMMT_AMMO_SUB_TYPE]->Set(Tile::kTileValue_String, ammosubtype, true);

    if (!pweap) {
        flastWeaponAmmo = std::numeric_limits<float>::min();
        flastTotalWeaponAmmo = std::numeric_limits<float>::min();
        plastWeaponObj = nullptr;
        flastWeapHealthPercent = std::numeric_limits<float>::min();
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
    }
    else {
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
            }
            else if (bAmmoShifted) {
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
                ftotalAmmo = static_cast<double>(
                                 InventoryChanges::GetInventoryChanges(pPlayer)->GetObjectCount(pCurrentAmmo)
                             ) -
                    fweaponAmmo;
            }
        }
        else if (pweaponObj->BGSAmmoForm::IsRockItLauncher()) {
            fweaponAmmo = pPlayer->GetNumRockItAmmo();
            ftotalAmmo = fweaponAmmo;
        }
    }

    if (pweaponObj->IsMeleeWeapon()) {
        this->pTiles[HMMT_AMMO_TEXT]->Set(Tile::kTileValue_Visible, 0);
    }
    else if (fweaponAmmo != flastWeaponAmmo || ftotalAmmo != flastTotalWeaponAmmo || plastWeaponObj != pweaponObj) {
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
        if (bAmmoShifted) {
            this->ShiftConditionTileOver(50);
        }
        bAmmoShifted = false;
    }

    // Reads the absolute health and drops it.
    pweap->GetItemHealth(false);
    const float fweapHealthPercent = pweap->GetItemHealth(true);
    if (fweapHealthPercent < 25.0) {
        // Below 25% the bar pulses: the alpha runs between 0 and the HUD opacity, turning at either end.
        BSClamp(fWeapCondBarAlpha, 0.0f, fLocalHudOpacity);
        if (fWeapCondBarAlpha == 0.0 || fWeapCondBarAlpha == fLocalHudOpacity) {
            bWeapFadeDecrease = !bWeapFadeDecrease;
        }
        if (bWeapFadeDecrease) {
            fWeapCondBarAlpha = fWeapCondBarAlpha - fLocalHudOpacity / 255.0 * 10.0;
        }
        else {
            fWeapCondBarAlpha = fLocalHudOpacity / 255.0 * 10.0 + fWeapCondBarAlpha;
        }
    }
    else {
        fWeapCondBarAlpha = fLocalHudOpacity;
    }
    this->pTiles[HMMT_WEAPON_CONDITION_METER]->Set(Tile::kTileValue_Alpha, fWeapCondBarAlpha, true);

    if (fweapHealthPercent != flastWeapHealthPercent) {
        const float fCurrentHealth = pweap->GetItemHealth(false);
        float fMaxHealth = static_cast<float>(pweaponObj->GetFormHealth(pweap->HasModEffectActive(10)));
        const int32_t iNumPixelShift = bAmmoShifted ? -50 : 0;
        const float fRatio = fCurrentHealth / fMaxHealth;
        HUDMainMenu::UpdateMeter(
            this->pTiles[HMMT_WEAPON_CONDITION_METER],
            BSMIN(1.0f, BSMAX(0.0f, fRatio)),
            HJ_LEFT,
            0,
            iNumPixelShift
        );
        flastWeapHealthPercent = fweapHealthPercent;
    }
}

void HUDMainMenu::UpdateExplosiveIndicator() {
    NiTListItem<Tile*>* pPos = pTiles[HMMT_EXPLOSIVE_INDICATOR]->xChildren.m_pkHead;
    for (uint32_t i = ReferenceProcessLists.MobProcessArray.GetProcessLevelHead(PROCESS_TYPE_HIGH);
         i < ReferenceProcessLists.MobProcessArray.GetProcessLevelTail(PROCESS_TYPE_HIGH);
         ++i) {
        MobileObject* pMob = ReferenceProcessLists.MobProcessArray.GetItem(i);
        if (!pMob ||
            pMob->IsStillLoading() ||
            pMob->GetDelete() ||
            pMob->GetDisabled() ||
            pMob->GetFormType() != PGRE_ID ||
            !pMob->IsProjectile()) {
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
        const NiPoint3& kPlayerPos = pPlayer->GetLocationOnReference();
        const NiPoint3& kMobPos = pMob->GetLocationOnReference();
        double dDistance = (kMobPos - kPlayerPos).Length();
        if (fExplosiveIndicatorDistance.Float() > dDistance && pPos) {
            Tile* pNew = pTiles[HMMT_EXPLOSIVE_INDICATOR]->xChildren.GetNext(pPos);
            if (pNew->GetFadeFinished(Tile::kTileValue_Alpha) && pNew->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
                float fOpacity = fHudOpacity.Float() * 255.0;
                pNew->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fOpacity, 0.5f, Tile::TFCT_STANDARD);
            }
            const NiPoint3& kPlayerLoc = pPlayer->GetLocationOnReference();
            const NiPoint3& kMobLoc = pMob->GetLocationOnReference();
            float fAngle = GetZAngleFromVector(kMobLoc - kPlayerLoc);
            float fCameraAngle = GetZAngleFromVector(Main::GetWorldSceneGraph()->GetCamera()->GetWorldDirection());
            float fRotation = fCameraAngle - fAngle;
            pNew->GetModel()->SetRotate(fRotation, 0.0f, 1.0f, 0.0f);
            NiUpdateData kUpdate(0.0f, true, false);
            pNew->GetModel()->Update(kUpdate);
        }
    }
    while (pPos) {
        Tile* pNew = pTiles[HMMT_EXPLOSIVE_INDICATOR]->xChildren.GetNext(pPos);
        if (pNew->GetFadeFinished(Tile::kTileValue_Alpha) && pNew->GetFloat(Tile::kTileValue_Alpha) > 0.0) {
            float fAlpha = pNew->GetFloat(Tile::kTileValue_Alpha);
            pNew->AddFadeControl(Tile::kTileValue_Alpha, fAlpha, 0.0f, 0.5f, Tile::TFCT_STANDARD);
        }
    }
}

// The start position is the tile's on the first call. A frame with fShake moves the tile by fShake in a random
// direction; the frame after the last shake (fShake 0) puts it back.
void HUDMainMenu::UpdateShake() {
    // The HUD tile's x and y before any shake.
    static float forigX = this->pMenu->GetFloat(Tile::kTileValue_X);
    static float forigY = this->pMenu->GetFloat(Tile::kTileValue_Y);
    float fX = 0.0f;
    float fY = 0.0f;
    if (this->fShake > 0.0) {
        bupdate = true;
        NiSinCos(RandomFloatTwoPi(), fX, fY);
        fX = fX * this->fShake;
        fY = fY * this->fShake;
    }
    if (bupdate) {
        this->pMenu->Set(Tile::kTileValue_X, forigX + fX, true);
        this->pMenu->Set(Tile::kTileValue_Y, forigY + fY, true);
    }
    if (this->fShake == 0.0) {
        bupdate = false;
    }
    this->fShake = 0.0f;
}

void HUDMainMenu::ShiftConditionTileOver(int32_t aishift) {
    float fX = static_cast<float>(HUDMainMenu::pme->pTiles[10]->GetFloat(4001) + static_cast<double>(aishift));
    pTiles[10]->Set(4001, fX, true);
    fX = static_cast<float>(HUDMainMenu::pme->pTiles[11]->GetFloat(4001) + static_cast<double>(aishift));
    pTiles[11]->Set(4001, fX, true);
    fX = static_cast<float>(HUDMainMenu::pme->pTiles[9]->GetFloat(4001) + static_cast<double>(aishift));
    pTiles[9]->Set(4001, fX, true);
    fX = static_cast<float>(HUDMainMenu::pme->pTiles[86]->GetFloat(4001) + static_cast<double>(aishift));
    pTiles[86]->Set(4001, fX, true);
}

void HUDMainMenu::UpdateActorValue(ActorValue::Index aeActorValue) {
    if (aeActorValue == ActorValue::ACTION_POINTS) {
        float fPermanentAP = pPlayer->GetPermanentActorFloatValue(ActorValue::ACTION_POINTS);
        float fBase = BSMAX(0.0, fPermanentAP);
        if (fBase != 0.0) {
            float fCurrentAP = pPlayer->GetActorFloatValue(ActorValue::ACTION_POINTS);
            fBase = BSMAX(0.0f, fCurrentAP) / fBase;
        }
        float fMeterPercent = BSMIN(fBase, 1.0f);
        HUDMainMenu::UpdateMeter(pTiles[7], fMeterPercent, HUDMainMenu::HJ_RIGHT, 0, 0);
    }
    else if (aeActorValue == ActorValue::HEALTH) {
        float fBase = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
        if (fBase != 0.0) {
            float fCurrentHealth = pPlayer->GetActorFloatValue(ActorValue::HEALTH);
            fBase = BSMAX(0.0f, fCurrentHealth) / fBase;
        }
        if (fBase > 1.0) {
            fBase = 1.0f;
        }
        if (pPlayer->IsDead(false)) {
            HUDMainMenu::UpdateMeter(HUDMainMenu::pme->pTiles[1], fBase, HUDMainMenu::HJ_LEFT, 0, 0);
        }
        else {
            HUDMainMenu::UpdateMeter(HUDMainMenu::pme->pTiles[1], fBase, HUDMainMenu::HJ_LEFT, 1, 0);
        }
    }
}

// Fades the breath meter in once the player is deep under water and out again once he surfaces, and sets the meter's
// animation to the breath left. The breath fraction is divided and kept at double precision and rounded to float once.
void HUDMainMenu::UpdateBreathMeter() {
    TESObjectCELL* pCell = pPlayer->GetParentCell();
    float fDepth = pPlayer->IsPointDeepUnderWater(pPlayer->GetLocationOnReference().z, pCell);
    bool bDeepUnderWater = fDepth >= 0.875;
    float fBreathTime = GamePlayFormulas::SwimBreathTime(static_cast<int32_t>(pPlayer->GetEndurance()));
    double dBreathLeft;
    if (pPlayer->GetCurrentAIProcess()) {
        BaseProcess* pProcess = pPlayer->GetCurrentAIProcess();
        dBreathLeft = static_cast<double>(pProcess->GetBreathTimer()) / static_cast<double>(fBreathTime);
    }
    else {
        dBreathLeft = 100.0;
    }
    float fBreathLeft = static_cast<float>(dBreathLeft);
    float fAlpha = pTiles[HMMT_BREATH]->GetFloat(Tile::kTileValue_Alpha);
    if (bDeepUnderWater) {
        if (fAlpha == 0.0) {
            pTiles[HMMT_BREATH]
                ->AddFadeControl(Tile::kTileValue_Alpha, 0.0f, fCurrentHUDOpacity, 0.5f, Tile::TFCT_STANDARD);
        }
    }
    else if (fAlpha > 0.0 && pTiles[HMMT_BREATH]->GetFadeFinished(Tile::kTileValue_Alpha)) {
        pTiles[HMMT_BREATH]
            ->AddFadeControl(Tile::kTileValue_Alpha, fCurrentHUDOpacity, 0.0f, 0.5f, Tile::TFCT_STANDARD);
    }
    if (pBreathMeterAnim && fAlpha > 0.0) {
        NiUpdateData kUpdateData(0.0f, true, false);
        float fEnd = pBreathMeterAnim->GetEndKeyTime();
        float fBegin = pBreathMeterAnim->GetBeginKeyTime();
        float fEndAgain = pBreathMeterAnim->GetEndKeyTime();
        float fPhase = static_cast<float>(
            (static_cast<double>(fBegin) - static_cast<double>(fEndAgain)) * static_cast<double>(fBreathLeft) +
            static_cast<double>(fEnd)
        );
        pBreathMeterAnim->SetPhase(fPhase, false);
        pTiles[HMMT_BREATH]->GetModel()->GetAt(0)->Update(kUpdateData);
    }
}

void HUDMainMenu::UpdateSneak() {
    bool bCombatSearch = false;
    bool bBlink = false;
    if (pPlayer->IsPlayerCharacterInCombat(&bCombatSearch) || bCombatSearch) {
        const char* pText;
        HUDMainMenu::pme->pTiles[24]->Set(4084, 2);
        if (bCombatSearch) {
            pText = sSneakCaution.String();
        }
        else {
            bBlink = true;
            pText = sSneakDanger.String();
        }
        HUDMainMenu::pme->pTiles[24]->Set(4036, pText, true);
    }
    else if (pPlayer->GetHostileDetection()) {
        HUDMainMenu::pme->pTiles[24]->Set(4084, 2);
        const char* pText = sSneakCaution.String();
        HUDMainMenu::pme->pTiles[24]->Set(4036, pText, true);
    }
    else {
        int32_t iLevel = ReferenceProcessLists.GetHighestDetectionLevelForActor(pPlayer, false);
        HUDMainMenu::pme->pTiles[24]->Set(4084, 1);
        bBlink = false;
        if (iLevel <= 0) {
            const char* pText = sSneakHidden.String();
            HUDMainMenu::pme->pTiles[24]->Set(4036, pText, true);
        }
        else if (!pPlayer->GetHostileDetection()) {
            const char* pText = sSneakDetected.String();
            HUDMainMenu::pme->pTiles[24]->Set(4036, pText, true);
        }
    }
    if (!bBlink || HUDMainMenu::pme->pTiles[24]->IsTrue(Tile::kTileValue_User0)) {
        double dCurrentAlpha;
        if ((!bBlink && HUDMainMenu::pme->pTiles[24]->GetFadeType(4009) == Tile::TFCT_BLINK_THRICE) ||
            (HUDMainMenu::pme->pTiles[24]->GetFadeFinished(4009) &&
             (dCurrentAlpha = HUDMainMenu::pme->pTiles[24]->GetFloat(4009),
              dCurrentAlpha != fHudOpacity.Float() * 255.0))) {
            float fOpacity = fHudOpacity.Float() * 255.0;
            float fAlpha = HUDMainMenu::pme->pTiles[24]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[24]->AddFadeControl(4009, fAlpha, fOpacity, 0.5f, Tile::TFCT_STANDARD);
        }
    }
    else {
        float fOpacity = fHudOpacity.Float() * 255.0;
        HUDMainMenu::pme->pTiles[24]->AddFadeControl(4009, 0.0f, fOpacity, 0.5f, Tile::TFCT_BLINK_THRICE);
    }
    HUDMainMenu::pme->pTiles[24]->Set(Tile::kTileValue_User0, static_cast<int32_t>(bBlink));
}

void HUDMainMenu::UpdateQuestText() {
    if (HUDMainMenu::pme &&
        eMenuMode != HUDMainMenu::HMM_VATS &&
        eMenuMode != HUDMainMenu::HMM_CONTAINER &&
        eMenuMode != HUDMainMenu::HMM_VATS_PLAYBACK &&
        appTimer.iLastTime - uiMessageDelayTime >= 2000) {
        if (!IsQuestNameShowing() &&
            !HUDMainMenu::pQuestUpdateManager->IsQuestNamesEmpty() &&
            (!AreObjectivesShowing() || HUDMainMenu::pme->pTiles[13]->xChildren.GetHead()->GetFloat(4009) == 0.0)) {
            if ((!InterfaceManager::Instance()->IsInMenuMode() ||
                 Interface::IsMenuIDVisible(Interface::eDialogMenu, 0) ||
                 Interface::IsConsoleVisible()) &&
                !pVATS.eMode) {
                char acQuestName[268];
                bQuestNameShowing = true;
                BSSoundHandle kSoundHand;
                if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType) {
                    if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                        QuestUpdateManager::HQUT_CUSTOM) {
                        char* pcSoundName = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()
                                                ->CustomSoundHandleName;
                        kSoundHand = BSAudio::QInstance().GetSoundHandleByName(pcSoundName, 289u);
                    }
                    else {
                        kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIPopUpQuestComplete", 289u);
                    }
                }
                else {
                    kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIPopUpQuestNew", 289u);
                }
                kSoundHand.Play(false);
                int32_t iWidth = 0;
                uint32_t uiFadeInStart = appTimer.iLastTime;
                Tile* pQuest = HUDMainMenu::pme->pTiles[14]->xChildren.GetHead();
                if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType) {
                    if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                        QuestUpdateManager::HQUT_FAILED) {
                        pQuest->Set(4036, sQuestFailed.String(), true);
                    }
                    else if (
                        HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                        QuestUpdateManager::HQUT_COMPLETE
                    ) {
                        pQuest->Set(4036, sQuestCompleted.String(), true);
                    }
                    else if (
                        HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                        QuestUpdateManager::HQUT_CUSTOM
                    ) {
                        pQuest->Set(4036, HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextTitle, true);
                    }
                    else {
                        Warning(
                            WARN_QUESTS,
                            "HUDMainMenu::UpdateQuestText is trying to handle an invalid eUpdateType (%i)",
                            HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType
                        );
                    }
                }
                else {
                    pQuest->Set(4036, sQuestAdded.String(), true);
                }
                pQuest->Set(4003, 1);
                pQuest->Set(4009, 0);
                int32_t nFont = 7;
                if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                        QuestUpdateManager::HQUT_CUSTOM &&
                    HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextTitleFont != FONT_NONE) {
                    nFont = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextTitleFont;
                }
                pQuest->Set(4025, nFont);
                float fWrapWidth = pQuest->GetFloat(4026);
                int32_t iFont = static_cast<int32_t>(pQuest->GetFloat(4025));
                const char* pText = pQuest->GetString(4036);
                NiPoint3 kDimensions = FontManager::Instance()->CalculateStringDimensions(pText, iFont, fWrapWidth, 0);
                iWidth = static_cast<int32_t>(kDimensions.x);
                float fY = -(kDimensions.y + 15.0);
                pQuest->Set(4002, fY, true);
                int32_t nJustification = 1;
                float fScreenWidthMultiplier = 0.0f;
                int32_t nScreenWidth = static_cast<int32_t>(InterfaceManager::GetScreenWidth());
                int32_t nScreenSafeX = static_cast<int32_t>(InterfaceManager::GetScreenSafeX());
                int32_t nSafeScreenWidth = nScreenWidth - 2 * nScreenSafeX;
                if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                    QuestUpdateManager::HQUT_CUSTOM) {
                    QuestUpdateManager::HUD_CUSTOM_QUEST_TEXT_JUSTIFICATION
                        eJustification = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextJustification;
                    if (eJustification) {
                        if (eJustification == QuestUpdateManager::HCQTJ_CENTER) {
                            nJustification = 2;
                            fScreenWidthMultiplier = 0.5f;
                        }
                        else if (eJustification == QuestUpdateManager::HCQTJ_RIGHT) {
                            nJustification = 4;
                            fScreenWidthMultiplier = 1.0f;
                        }
                        else {
                            Warning(
                                WARN_QUESTS,
                                "HUDMainMenu::UpdateQuestText is trying to handle an invalid "
                                "CustomTextJustification (%i).  Defaulting to %i.",
                                HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextJustification,
                                0
                            );
                            nJustification = 1;
                            fScreenWidthMultiplier = 0.0f;
                        }
                    }
                    else {
                        nJustification = 1;
                        fScreenWidthMultiplier = 0.0f;
                    }
                    float fX = static_cast<double>(nSafeScreenWidth - iWidth / 2) * fScreenWidthMultiplier;
                    pQuest->Set(4001, fX, true);
                    pQuest->Set(4023, nJustification);
                }
                pQuest->Set(Tile::kTileValue_User0, uiFadeInStart);
                double dFadeInStart = static_cast<double>(uiFadeInStart);
                float fShowEnd = fQuestCinematicCharacterRemain.Float() * 1000.0 + dFadeInStart;
                pQuest->Set(Tile::kTileValue_User1, fShowEnd, true);
                HUDMainMenu::SetBrightness(pQuest, HUDMainMenu::HB_HUD_TEXT);
                if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                    QuestUpdateManager::HQUT_COMPLETE) {
                    KillQuestObjectives();
                }
                if (HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->eUpdateType ==
                    QuestUpdateManager::HQUT_CUSTOM) {
                    BSstrcpy(
                        acQuestName,
                        260,
                        HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem()->CustomTextSubtitle
                    );
                }
                else {
                    QuestUpdateManager::QuestUpdate* pItem = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
                    BSstrcpy(acQuestName, 260, pItem->pQuest->GetFullName());
                }
                for (uint32_t i = 0; i < 260; ++i) {
                    acQuestName[i] = std::toupper(acQuestName[i]);
                }
                int32_t iSpace = 32;
                int32_t iQuestNameLength = BSstrlen(acQuestName);
                if (iQuestNameLength >= 32) {
                    for (int32_t i = 32; i >= 0; --i) {
                        if (acQuestName[i] == ' ') {
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
                NiPoint3 kSubtitleDimensions = FontManager::Instance()->CalculateStringDimensions(
                    acQuestName,
                    nFont,
                    std::numeric_limits<float>::max(),
                    0
                );
                int32_t iX = static_cast<int32_t>(
                    (static_cast<double>(nSafeScreenWidth) - (fScreenWidthMultiplier + 1.0) * kSubtitleDimensions.x) *
                    fScreenWidthMultiplier
                );
                NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[14]->xChildren.m_pkHead;
                if (pPos) {
                    // The first child returned here is never read; only the call is kept.
                    HUDMainMenu::pme->pTiles[14]->xChildren.GetNext(pPos);
                }
                for (int32_t i = 0; acQuestName[i]; ++i) {
                    Tile* pLetter = nullptr;
                    if (pPos) {
                        char acLetter[4];
                        pLetter = HUDMainMenu::pme->pTiles[14]->xChildren.GetNext(pPos);
                        acLetter[0] = acQuestName[i];
                        acLetter[1] = 0;
                        if (iSpace + 1 == i) {
                            iX = static_cast<int32_t>(
                                (static_cast<double>(nSafeScreenWidth) -
                                 (fScreenWidthMultiplier + 1.0) * kSubtitleDimensions.x) *
                                fScreenWidthMultiplier
                            );
                        }
                        pLetter->Set(4025, nFont);
                        pLetter->Set(4036, acLetter, true);
                        pLetter->Set(4003, 1);
                        pLetter->Set(4009, 0);
                        pLetter->Set(4001, iX);
                        fWrapWidth = pLetter->GetFloat(4026);
                        iFont = static_cast<int32_t>(pLetter->GetFloat(4025));
                        NiPoint3 kLetterDimensions = FontManager::Instance()
                                                         ->CalculateStringDimensions(acLetter, iFont, fWrapWidth, 0);
                        if (i > iSpace) {
                            fY = kLetterDimensions.y + 5.0;
                            pLetter->Set(4002, fY, true);
                        }
                        else {
                            pLetter->Set(4002, 0);
                        }
                        uiFadeInStart += static_cast<int64_t>(fQuestCinematicCharacterFadeInDelay.Float() * 1000.0);
                        pLetter->Set(Tile::kTileValue_User0, uiFadeInStart);
                        dFadeInStart = static_cast<double>(uiFadeInStart);
                        fShowEnd = fQuestCinematicCharacterRemain.Float() * 1000.0 + dFadeInStart;
                        pLetter->Set(Tile::kTileValue_User1, fShowEnd, true);
                        dFadeInStart = static_cast<double>(uiFadeInStart);
                        double dLetterShowEnd = fQuestCinematicCharacterRemain.Float() * 1000.0 + dFadeInStart;
                        InitializationFlow("Set show time %s: %f", pLetter->GetString(4036), dLetterShowEnd);
                        HUDMainMenu::SetBrightness(pLetter, HUDMainMenu::HB_HUD_TEXT);
                        iX += static_cast<int32_t>(kLetterDimensions.x);
                        double dLetterX = pLetter->GetFloat(4001);
                        if (static_cast<double>(iWidth) < dLetterX + kLetterDimensions.x) {
                            dLetterX = pLetter->GetFloat(4001);
                            iWidth = static_cast<int32_t>(dLetterX + kLetterDimensions.x);
                        }
                    }
                }
                QuestUpdateManager::QuestUpdate* pItem = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
                ourFree(pItem);
                HUDMainMenu::pQuestUpdateManager->RemoveQuestNamesHead();
                HandleCinematicFade(HUDMainMenu::pme->pTiles[14]);
            }
        }
        if (IsQuestNameShowing()) {
            HandleCinematicFade(HUDMainMenu::pme->pTiles[14]);
        }
        else if (
            !Interface::IsTopMenuID(Interface::eCharGenMenu) &&
            (AreObjectivesShowing() || !xQuestObjectives.IsEmpty()) &&
            !IsQuestNameShowing()
        ) {
            HandleReminderFade(HUDMainMenu::pme->pTiles[13]);
        }
    }
}

void HUDMainMenu::UpdateXPMeter() {
    static int32_t HUDXPMeter_trait_x_min = Tile::AddUserTrait("_x_min", -1);
    static int32_t HUDXPMeter_trait_x_max = Tile::AddUserTrait("_x_max", -1);
    bool bPlayerInCombat = pPlayer->GetNumberActorsInCombat() > 0;
    char bPlayerReadyToLevel = pPlayer->GetCharacterProgressionInfo()->GetReadyToLevelUp();
    if ((!Interface::IsInMenuMode() ||
         Interface::GetTopMenuID() == Interface::eContainerMenu ||
         Interface::GetTopMenuID() == Interface::eHackingMenu ||
         Interface::GetTopMenuID() == Interface::eComputersMenu ||
         Interface::GetTopMenuID() == Interface::eDialogMenu) &&
        pVATS.eMode != VATS::VATS_PLAYBACK &&
        !HKWheel.GetVisible()) {
        if (fCurrentHUDOpacity == 0.0) {
            bXPMeterFinishedMoving = true;
        }
        else {
            float fAlpha = pTiles[44]->GetFloat(4009);
            bool bXPMeterShowing = fAlpha > 0.0;
            bool bXPMeterFadeFinished = pTiles[44]->GetFadeFinished(4009);
            if ((g_XPBarState & 1) != 0) {
                if (bXPMeterShowing || !bXPMeterFadeFinished || xXPUpdates.IsEmpty()) {
                    pTiles[47]->GetFadeEndFor(4001);
                    float fXMin = pTiles[47]->GetFloat(HUDXPMeter_trait_x_min);
                    float fXMax = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
                    bool bFadeFinishedPointer = pTiles[47]->GetFadeFinished(4001);
                    if (!pTiles[47]->IsTrue(Tile::kTileValue_User11) && bXPMeterFadeFinished && bXPMeterShowing) {
                        int32_t iCurrXP = static_cast<int32_t>(
                            pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS)
                        );
                        if (xp_currentActorLevel == xp_nextActorLevel) {
                            ++xp_nextActorLevel;
                        }
                        int32_t iPrevThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_currentActorLevel);
                        int32_t iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
                        pTiles[47]->Set(Tile::kTileValue_User11, 1);
                        if (iCurrXP <= iNextThreshold) {
                            int32_t iClampedXP = BSMAX(iCurrXP, iPrevThreshold);
                            float fCurrXP = static_cast<float>(BSMIN(iClampedXP, iNextThreshold));
                            float fTargetX = FLerp(
                                fXMin,
                                fXMax,
                                static_cast<float>(iPrevThreshold),
                                static_cast<float>(iNextThreshold),
                                fCurrXP
                            );
                            float fX = pTiles[47]->GetFloat(4001);
                            pTiles[47]->AddFadeControl(4001, fX, fTargetX, 2.0f, Tile::TFCT_STANDARD);
                        }
                        else {
                            if (static_cast<double>(fXMax) < pTiles[47]->GetFloat(4001)) {
                                pTiles[47]->Set(4001, fXMax, true);
                            }
                            float fX = pTiles[47]->GetFloat(4001);
                            pTiles[47]->AddFadeControl(4001, fX, fXMax, 1.0f, Tile::TFCT_STANDARD);
                        }
                    }
                    else if (!bXPMeterFadeFinished || bXPMeterShowing) {
                        if (!bFadeFinishedPointer && bXPMeterFadeFinished && bXPMeterShowing) {
                            int32_t iCurrXP = static_cast<int32_t>(
                                pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS)
                            );
                            int32_t iPrevThreshold = GamePlayFormulas::GetRequiredExperiencePoints(
                                xp_currentActorLevel
                            );
                            int32_t iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
                            if (bPlayerReadyToLevel &&
                                fXMax == pTiles[47]->GetFloat(4001) &&
                                !pPlayer->GetInChargen() &&
                                !bPlayerInCombat) {
                                InterfaceManager::PlayMenuSound(MENU_SOUND_LEVEL_UP);
                            }
                            if (iCurrXP > iNextThreshold) {
                                if (xp_nextActorLevel < iMaxCharacterLevel.Int() && !bPlayerInCombat) {
                                    char acLevel[4];
                                    ++xp_currentActorLevel;
                                    ++xp_nextActorLevel;
                                    iPrevThreshold = iNextThreshold;
                                    iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
                                    pTiles[47]->Set(4001, fXMin, true);
                                    BSsprintf(acLevel, 3, "%i", xp_currentActorLevel);
                                    pTiles[48]->Set(4036, acLevel, true);
                                    BSsprintf(acLevel, 3, "%i", xp_nextActorLevel);
                                    pTiles[49]->Set(4036, acLevel, true);
                                    if (iCurrXP <= iNextThreshold) {
                                        float fTargetX = FLerp(
                                            fXMin,
                                            fXMax,
                                            static_cast<float>(iPrevThreshold),
                                            static_cast<float>(iNextThreshold),
                                            static_cast<float>(iCurrXP)
                                        );
                                        pTiles[47]->AddFadeControl(4001, fXMin, fTargetX, 2.0f, Tile::TFCT_STANDARD);
                                    }
                                    else {
                                        if (fXMax < static_cast<double>(fXMin)) {
                                            fXMin = fXMax;
                                        }
                                        pTiles[47]->AddFadeControl(4001, fXMin, fXMax, 1.0f, Tile::TFCT_STANDARD);
                                    }
                                }
                            }
                        }
                        else if (
                            pTiles[47]->GetFadeFinished(4001) &&
                            pTiles[47]->IsTrue(Tile::kTileValue_User11) &&
                            pTiles[44]->GetFadeFinished(4009) &&
                            pTiles[44]->GetFloat(4009) > 0.0
                        ) {
                            pTiles[47]->Set(Tile::kTileValue_User11, 0);
                            fAlpha = pTiles[44]->GetFloat(4009);
                            pTiles[44]->AddFadeControl(4009, fAlpha, 0.0f, 2.0f, Tile::TFCT_STANDARD);
                            fAlpha = pTiles[45]->GetFloat(4009);
                            pTiles[45]->AddFadeControl(4009, fAlpha, 0.0f, 2.0f, Tile::TFCT_STANDARD);
                            fAlpha = pTiles[46]->GetFloat(4009);
                            pTiles[46]->AddFadeControl(4009, fAlpha, 0.0f, 2.0f, Tile::TFCT_STANDARD);
                            fAlpha = pTiles[47]->GetFloat(4009);
                            pTiles[47]->AddFadeControl(4009, fAlpha, 0.0f, 2.0f, Tile::TFCT_STANDARD);
                            fAlpha = pTiles[48]->GetFloat(4009);
                            pTiles[48]->AddFadeControl(4009, fAlpha, 0.0f, 2.0f, Tile::TFCT_STANDARD);
                            fAlpha = pTiles[49]->GetFloat(4009);
                            pTiles[49]->AddFadeControl(4009, fAlpha, 0.0f, 2.0f, Tile::TFCT_STANDARD);
                        }
                    }
                    else {
                        HUDMainMenu::pme->pTiles[44]->Set(4003, 0);
                        HUDMainMenu::pme->pTiles[45]->Set(4003, 0);
                        HUDMainMenu::pme->pTiles[46]->Set(4003, 0);
                        HUDMainMenu::pme->pTiles[47]->Set(4003, 0);
                        HUDMainMenu::pme->pTiles[48]->Set(4003, 0);
                        HUDMainMenu::pme->pTiles[49]->Set(4003, 0);
                        if ((g_XPBarState & 8) != 0 && !bXPMeterFinishedMoving) {
                            bXPMeterFinishedMoving = true;
                            pTiles[50]->Set(Tile::kTileValue_User10, 0);
                        }
                    }
                }
                else {
                    BSSimpleList<HUDMainMenu::XPUpdate*>* pIter = &xXPUpdates;
                    int32_t iTotalAddedXP = 0;
                    while (pIter && !pIter->IsEmpty()) {
                        iTotalAddedXP += pIter->m_item->iAddedXP;
                        pIter = pIter->m_pkNext;
                    }
                    while (xXPUpdates.m_item) {
                        HUDMainMenu::XPUpdate* pItem = xXPUpdates.m_item;
                        ourFree(pItem);
                        xXPUpdates.RemoveHead();
                    }
                    if (iTotalAddedXP > 0 && !bXPMeterFinishedMoving) {
                        char acText[16];
                        BSSoundHandle kSoundHand;
                        kSoundHand = BSAudio::QInstance().GetSoundHandleByName("UIPopUpExperienceUp", 289u);
                        kSoundHand.Play(false);
                        float fXP = 0.0f;
                        int32_t iCurrXP = static_cast<int32_t>(
                            pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS)
                        );
                        xp_currentActorLevel = pPlayer->GetLevel();
                        xp_nextActorLevel = xp_currentActorLevel + 1;
                        int32_t iPrevThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_currentActorLevel);
                        int32_t iNextThreshold = GamePlayFormulas::GetRequiredExperiencePoints(xp_nextActorLevel);
                        if (iCurrXP > iNextThreshold && bPlayerInCombat) {
                            fXP = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
                        }
                        if (iPrevThreshold == iNextThreshold) {
                            fXP = static_cast<float>(iPrevThreshold);
                        }
                        else {
                            double dCurrentXP = pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS);
                            float fPrevXP = dCurrentXP - static_cast<double>(iTotalAddedXP);
                            float fNextThreshold = static_cast<float>(iNextThreshold);
                            float fPrevThreshold = static_cast<float>(iPrevThreshold);
                            float fXMax = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
                            float fXMin = pTiles[47]->GetFloat(HUDXPMeter_trait_x_min);
                            fXP = FLerp(fXMin, fXMax, fPrevThreshold, fNextThreshold, fPrevXP);
                        }
                        float fXMax = pTiles[47]->GetFloat(HUDXPMeter_trait_x_max);
                        float fXMin = pTiles[47]->GetFloat(HUDXPMeter_trait_x_min);
                        float fX = BSMAX(fXP, fXMin);
                        fX = BSMIN(fX, fXMax);
                        pTiles[47]->Set(4001, fX, true);
                        pTiles[44]->Set(4003, 1);
                        pTiles[45]->Set(4003, 1);
                        pTiles[46]->Set(4003, 1);
                        const char* pText = sStatsXP.String();
                        pTiles[46]->Set(4036, pText, true);
                        pTiles[47]->Set(4003, 1);
                        pTiles[48]->Set(4003, 1);
                        pTiles[49]->Set(4003, 1);
                        pTiles[44]->Set(4009, 0.0f, true);
                        pTiles[45]->Set(4009, 0.0f, true);
                        pTiles[46]->Set(4009, 0.0f, true);
                        pTiles[47]->Set(4009, 0.0f, true);
                        pTiles[48]->Set(4009, 0.0f, true);
                        pTiles[49]->Set(4009, 0.0f, true);
                        BSsprintf(acText, 16, "+%i", iTotalAddedXP);
                        pTiles[45]->Set(4036, acText, true);
                        BSsprintf(acText, 16, "%i", xp_currentActorLevel);
                        pTiles[48]->Set(4036, acText, true);
                        BSsprintf(acText, 16, "%i", xp_nextActorLevel);
                        pTiles[49]->Set(4036, acText, true);
                        pTiles[44]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
                        pTiles[45]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
                        pTiles[46]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
                        pTiles[47]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
                        pTiles[48]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
                        pTiles[49]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 2.0f, Tile::TFCT_STANDARD);
                        bXPMeterShowing = true;
                    }
                }
                if (bPlayerReadyToLevel) {
                    if (bPlayerInCombat || bXPMeterShowing || (g_XPBarState & 8) != 0) {
                        g_XPBarState |= 4u;
                    }
                    else {
                        g_XPBarState = 2;
                    }
                }
            }
            else if (
                (g_XPBarState & 2) == 0 ||
                (g_XPBarState & 8) != 0 ||
                bXPMeterShowing ||
                !bXPMeterFadeFinished ||
                pPlayer->GetInChargen()
            ) {
                if ((g_XPBarState & 4) != 0) {
                    bool bAlreadyShown = (g_XPBarState & 8) != 0;
                    bool bFadeFinished = pTiles[50]->GetFadeFinished(4009);
                    bool bAlphadOut = pTiles[50]->GetFloat(4009) == 0.0;
                    if (bAlreadyShown) {
                        if (!bXPMeterFinishedMoving && bFadeFinished && bAlphadOut) {
                            bXPMeterFinishedMoving = true;
                            g_XPBarState = 8;
                            pTiles[50]->Set(Tile::kTileValue_User10, 0);
                        }
                    }
                    else if (bPlayerInCombat) {
                        g_XPBarState |= 1u;
                    }
                    else if (!bXPMeterShowing) {
                        g_XPBarState = 2;
                    }
                }
            }
            else {
                bool bFadeFinished = pTiles[50]->GetFadeFinished(4009);
                bool bAlphadOut = pTiles[50]->GetFloat(4009) == 0.0;
                if (!bFadeFinished ||
                    !bAlphadOut ||
                    pTiles[50]->IsTrue(Tile::kTileValue_User10) ||
                    pPlayer->GetNumberActorsInCombat()) {
                    double dTime = static_cast<double>(appTimer.iLastTime);
                    if (dTime - pTiles[50]->GetFloat(Tile::kTileValue_User9) > 1500.0 && !bAlphadOut && bFadeFinished) {
                        uint32_t uiTime = appTimer.iLastTime;
                        pTiles[50]->Set(Tile::kTileValue_User9, uiTime + 2000);
                        pTiles[50]->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.5f, Tile::TFCT_STANDARD);
                    }
                    else if (!bXPMeterFinishedMoving && bFadeFinished && bAlphadOut) {
                        bXPMeterFinishedMoving = true;
                        g_XPBarState = 8;
                        pTiles[50]->Set(Tile::kTileValue_User10, 0);
                    }
                }
                else {
                    BSAudio::QInstance().GetSoundHandleByName("UILevelUpText", 289u).Play(false);
                    pTiles[50]->Set(4009, 0.0f, true);
                    pTiles[50]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 0.5f, Tile::TFCT_STANDARD);
                    const char* pText = sLevelUp.String();
                    pTiles[50]->Set(4036, pText, true);
                    pTiles[50]->Set(4003, 1);
                    uint32_t uiTime = appTimer.iLastTime;
                    pTiles[50]->Set(Tile::kTileValue_User9, uiTime);
                    pTiles[50]->Set(Tile::kTileValue_User10, 1);
                }
            }
        }
    }
    else {
        HUDMainMenu::pme->pTiles[44]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[45]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[46]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[47]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[48]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[49]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[44]->Set(4009, 0.0f, true);
        HUDMainMenu::pme->pTiles[44]->RemoveFadeControl(4009);
        g_XPBarState |= 1u;
        bXPMeterFinishedMoving = false;
    }
}

void HUDMainMenu::UpdateRadiationMeter() {
    static int32_t ivisFlag = 4;
    double dDrankRadsTime = HUDMainMenu::pme->pTiles[19]->GetFloat(Tile::kTileValue_User6);
    bool bDrankRads = static_cast<double>(appTimer.iLastTime) < dDrankRadsTime;
    bool bInRadiation = ReferenceProcessLists.IsPlayerInRadiationArea();
    if (bInRadiation || bDrankRads || ivisFlag != 4) {
        bool bRecalcRadsTarget = false;
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
        if ((ivisFlag == 4 || ivisFlag == 2) && (bInRadiation || bDrankRads)) {
            ivisFlag = 8;
            bRecalcRadsTarget = true;
        }
        float fStart = 15.0f;
        float fEnd = 315.0f;
        float fPlayersCurrentRads = 0.0f;
        float fRadsTotal = 0.0f;
        float fTargetRads = 0.0f;
        bool bRadsAdded = HUDMainMenu::pme->pTiles[19]->IsTrue(Tile::kTileValue_User7) || bDrankRads;
        if (bInRadiation || bDrankRads || bRecalcRadsTarget) {
            float fPlayerRads = static_cast<float>(pPlayer->GetActorValue(ActorValue::RADIATION_RADS));
            bool bChangeVal = false;
            if (bInRadiation || bRecalcRadsTarget || bRadsAdded) {
                bChangeVal = true;
                fRadsTotal = HUDMainMenu::pme->pTiles[19]->GetFloat(Tile::kTileValue_User5);
                float fPrevRads = fPlayerRads - fRadsTotal;
                float fClampedRads = BSMIN(fPrevRads, 1000.0f);
                fClampedRads = BSMAX(fClampedRads, 0.0f);
                fPlayersCurrentRads = FLerp(fStart, fEnd, 0.0f, 1000.0f, fClampedRads);
            }
            if ((bDrankRads && bRecalcRadsTarget) || bRadsAdded || bInRadiation) {
                bChangeVal = true;
                float fClampedRads = BSMIN(fPlayerRads, 1000.0f);
                fClampedRads = BSMAX(fClampedRads, 0.0f);
                fTargetRads = FLerp(fStart, fEnd, 0.0f, 1000.0f, fClampedRads);
            }
            if (bChangeVal) {
                BSsprintf(g_sRadiationTextBuffer, 260u, "+%i", BSMAX(1, static_cast<int32_t>(fRadsTotal)));
                HUDMainMenu::pme->pTiles[19]->Set(4036, g_sRadiationTextBuffer, true);
            }
        }
        if (bRecalcRadsTarget) {
            ivisFlag = 8;
            float fPointerRads = bInRadiation ? fTargetRads : fPlayersCurrentRads;
            float fX = BSMIN(fPointerRads, fEnd);
            HUDMainMenu::pme->pTiles[18]->Set(4001, fX, true);
            HUDMainMenu::pme->pTiles[61]->Set(4003, 1);
            float fOpacity = fCurrentHUDOpacity;
            float fAlpha = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[17]->AddFadeControl(4009, fAlpha, fOpacity, 0.25f, Tile::TFCT_STANDARD);
            fOpacity = fCurrentHUDOpacity;
            fAlpha = HUDMainMenu::pme->pTiles[18]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[18]->AddFadeControl(4009, fAlpha, fOpacity, 0.25f, Tile::TFCT_STANDARD);
            fOpacity = fCurrentHUDOpacity;
            fAlpha = HUDMainMenu::pme->pTiles[19]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[19]->AddFadeControl(4009, fAlpha, fOpacity, 0.25f, Tile::TFCT_STANDARD);
            fOpacity = fCurrentHUDOpacity;
            fAlpha = HUDMainMenu::pme->pTiles[20]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[20]->AddFadeControl(4009, fAlpha, fOpacity, 0.25f, Tile::TFCT_STANDARD);
        }
        else if (bInRadiation) {
            float fX = BSMIN(fTargetRads, fEnd);
            HUDMainMenu::pme->pTiles[18]->Set(4001, fX, true);
        }
        if (bRadsAdded) {
            HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User7, 0);
            float fX = BSMIN(fTargetRads, fEnd);
            HUDMainMenu::pme->pTiles[18]->Set(Tile::kTileValue_User0, fX, true);
            float fClampedTotal = BSMIN(50.0f, fRadsTotal);
            float fPointerMovementTime = FLerp(1.5f, 3.0f, 0.0f, 50.0f, fClampedTotal);
            float fCurrentX = HUDMainMenu::pme->pTiles[18]->GetFloat(4001);
            float fTargetX = HUDMainMenu::pme->pTiles[18]->GetFloat(Tile::kTileValue_User0);
            float fNextX = BSMAX(fTargetX, fStart);
            if (!bInRadiation) {
                HUDMainMenu::pme->pTiles[18]
                    ->AddFadeControl(4001, fCurrentX, fNextX, fPointerMovementTime, Tile::TFCT_STANDARD);
            }
        }
        if (!bInRadiation && !bDrankRads) {
            ivisFlag = 2;
            float fAlpha = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[17]->AddFadeControl(4009, fAlpha, 0.0f, 0.25f, Tile::TFCT_STANDARD);
            fAlpha = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[18]->AddFadeControl(4009, fAlpha, 0.0f, 0.25f, Tile::TFCT_STANDARD);
            fAlpha = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[19]->AddFadeControl(4009, fAlpha, 0.0f, 0.25f, Tile::TFCT_STANDARD);
            fAlpha = HUDMainMenu::pme->pTiles[17]->GetFloat(4009);
            HUDMainMenu::pme->pTiles[20]->AddFadeControl(4009, fAlpha, 0.0f, 0.25f, Tile::TFCT_STANDARD);
        }
    }
}

void HUDMainMenu::UpdateStatusModeUI(bool abIsHardcore) {
    const char* pText = sStatsLMBAbbrev.String();
    HUDMainMenu::pme->pTiles[85]->Set(4036, pText, true);
    pText = sStatsRADAbbrev.String();
    HUDMainMenu::pme->pTiles[84]->Set(4036, pText, true);
    pText = sStatsH20Abbrev.String();
    HUDMainMenu::pme->pTiles[79]->Set(4036, pText, true);
    pText = sStatsFODAbbrev.String();
    HUDMainMenu::pme->pTiles[81]->Set(4036, pText, true);
    pText = sStatsSLPAbbrev.String();
    HUDMainMenu::pme->pTiles[80]->Set(4036, pText, true);
    int32_t iTotalCrippledLimbs = 0;
    ActorValueOwner* pAVOwner = static_cast<ActorValueOwner*>(pPlayer);
    float factorLeftArmCondition = CombatFormulas::GetBodyPartCondition(
        pAVOwner,
        ActorValue::LEFT_ATTACK_CONDITION,
        false
    );
    if (factorLeftArmCondition == 0.0) {
        iTotalCrippledLimbs = 1;
    }
    pAVOwner = static_cast<ActorValueOwner*>(pPlayer);
    float factorLeftLegCondition = CombatFormulas::GetBodyPartCondition(
        pAVOwner,
        ActorValue::LEFT_MOBILITY_CONDITION,
        false
    );
    if (factorLeftLegCondition == 0.0) {
        ++iTotalCrippledLimbs;
    }
    pAVOwner = static_cast<ActorValueOwner*>(pPlayer);
    float factorRightLegCondition = CombatFormulas::GetBodyPartCondition(
        pAVOwner,
        ActorValue::RIGHT_MOBILITY_CONDITION,
        false
    );
    if (factorRightLegCondition == 0.0) {
        ++iTotalCrippledLimbs;
    }
    pAVOwner = static_cast<ActorValueOwner*>(pPlayer);
    float factorRightArmCondition = CombatFormulas::GetBodyPartCondition(
        pAVOwner,
        ActorValue::RIGHT_ATTACK_CONDITION,
        false
    );
    if (factorRightArmCondition == 0.0) {
        ++iTotalCrippledLimbs;
    }
    pAVOwner = static_cast<ActorValueOwner*>(pPlayer);
    float factorHeadCondition = CombatFormulas::GetBodyPartCondition(pAVOwner, ActorValue::PERCEPTION_CONDITION, false);
    if (factorHeadCondition == 0.0) {
        ++iTotalCrippledLimbs;
    }
    pAVOwner = static_cast<ActorValueOwner*>(pPlayer);
    float factorBodyCondition = CombatFormulas::GetBodyPartCondition(pAVOwner, ActorValue::ENDURANCE_CONDITION, false);
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
            }
            else {
                fTotalLimbCondition = fHCFullAlphaTextThreshold.Float() + 1.0;
            }
        }
        else {
            fTotalLimbCondition = fHCRedTextThreshold.Float() + 1.0;
        }
    }
    else {
        fTotalLimbCondition = fHCFlashTextThreshold.Float() + 1.0;
    }
    float fCurrValue = static_cast<float>(pPlayer->GetActorValue(ActorValue::RADIATION_RADS));
    StatusModeTileEffects(HUDMainMenu::pme->pTiles[84], fCurrValue, false);
    StatusModeTileEffects(HUDMainMenu::pme->pTiles[85], fTotalLimbCondition, !abIsHardcore);
    if (abIsHardcore) {
        fCurrValue = static_cast<float>(pPlayer->GetActorValue(ActorValue::DEHYDRATION));
        StatusModeTileEffects(HUDMainMenu::pme->pTiles[79], fCurrValue, false);
        fCurrValue = static_cast<float>(pPlayer->GetActorValue(ActorValue::SLEEPDEPREVATION));
        StatusModeTileEffects(HUDMainMenu::pme->pTiles[80], fCurrValue, false);
        fCurrValue = static_cast<float>(pPlayer->GetActorValue(ActorValue::HUNGER));
        StatusModeTileEffects(HUDMainMenu::pme->pTiles[81], fCurrValue, true);
    }
    else {
        HUDMainMenu::pme->pTiles[79]->Set(4009, 0.0f, true);
        HUDMainMenu::pme->pTiles[80]->Set(4009, 0.0f, true);
        HUDMainMenu::pme->pTiles[81]->Set(4009, 0.0f, true);
    }
}

void HUDMainMenu::UpdateEnemyHealth(Actor* apActor) {
    int32_t fHasSeeHealthPerk;
    float fBaseHealth = static_cast<float>(apActor->GetBaseActorValue(ActorValue::HEALTH));
    float fCurrentHealth = static_cast<float>(apActor->GetActorValue(ActorValue::HEALTH));
    float fDDT = GetAdjustedDamageThreshold(pPlayer, apActor);
    float fPerHealth = 0.0;
    if (fBaseHealth > 0.0) {
        fPerHealth = fCurrentHealth / fBaseHealth;
    }
    static float fperhealthlast = fPerHealth;
    if (fPerHealth != fperhealthlast) {
        iEnemyHealthTimer = HudTimer.iLastTime;
        fperhealthlast = fPerHealth;
    }
    if (apActor->GetDestroyed()) {
        HUDMainMenu::UpdateMeter(pTiles[22], 0.0f, HUDMainMenu::HJ_CENTER, 0, 0);
    }
    else if (apActor->IsDead(false)) {
        HUDMainMenu::UpdateMeter(pTiles[22], fPerHealth, HUDMainMenu::HJ_CENTER, 0, 0);
    }
    else {
        HUDMainMenu::UpdateMeter(pTiles[22], fPerHealth, HUDMainMenu::HJ_CENTER, 1, 0);
    }
    *reinterpret_cast<float*>(&fHasSeeHealthPerk) = 0.0;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::SEE_ENEMY_HEALTH, pPlayer, &fHasSeeHealthPerk);
    if (*reinterpret_cast<float*>(&fHasSeeHealthPerk) > 0.0) {
        char acText[260];
        double dThreshold = fDDT;
        const char* pDDTLabel = sInventoryDamageThreshold.String();
        int32_t iCurrentHealth = static_cast<int32_t>(fCurrentHealth);
        const char* pHPLabel = sStatsHP.String();
        std::sprintf(
            acText,
            "%s\n %s:%d %s:%.2f",
            apActor->GetFullName(),
            pHPLabel,
            iCurrentHealth,
            pDDTLabel,
            dThreshold
        );
        HUDMainMenu::pme->pTiles[23]->Set(4036, acText, true);
    }
    else {
        const char* pName = apActor->GetFullName();
        HUDMainMenu::pme->pTiles[23]->Set(4036, pName, true);
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
            QuestUpdateManager::QuestUpdate* pItem = HUDMainMenu::pQuestUpdateManager->GetQuestNamesItem();
            ourFree(pItem);
            HUDMainMenu::pQuestUpdateManager->RemoveQuestNamesHead();
        }
        HUDMainMenu::pQuestUpdateManager->RemoveQuestNamesAll();
    }
}

void HUDMainMenu::KillQuestObjectives() {
    NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[13]->xChildren.m_pkTail;
    while (pPos) {
        Tile* pTile = HUDMainMenu::pme->pTiles[13]->xChildren.GetPrev(pPos);
        if (pTile) {
            pTile->Set(4003, 0);
            pTile->Set(4009, 0.0f, true);
            pTile->Set(Tile::TextToTrait("_WaitComplete"), 0.0f, true);
            pTile->Set(Tile::TextToTrait("_HoldTime"), 0.0f, true);
        }
    }
    if (HUDMainMenu::pme->eMenuMode != HUDMainMenu::HMM_LOAD) {
        for (BSSimpleList<HUDMainMenu::QueuedQuestText*>* pList = &HUDMainMenu::pme->xQuestObjectives;
             pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            ourFree(pList->m_item);
        }
        HUDMainMenu::pme->xQuestObjectives.RemoveAll();
    }
}

HUDMainMenu::SubtitleData* HUDMainMenu::GetNextSubtitle() {
    HUDMainMenu::SubtitleData* pResult = nullptr;
    float fClosestSqrDist = std::numeric_limits<float>::max();
    NiPoint3 kPlayerLoc = pPlayer->GetLocationOnReference();
    for (int32_t iSubtitleCt = 0; iSubtitleCt < xSubtitles.iSize; ++iSubtitleCt) {
        float fCurrSqrDist = (xSubtitles.GetAt(iSubtitleCt)->SpeakerLoc - kPlayerLoc).SqrLength();
        if (pResult || !(fCurrSqrDist <= std::pow(500.0f, 2))) {
            if ((!pResult ||
                 !(fClosestSqrDist > static_cast<double>(fCurrSqrDist)) ||
                 xSubtitles.GetAt(iSubtitleCt)->iPriority < pResult->iPriority) &&
                xSubtitles.GetAt(iSubtitleCt)->iPriority != std::numeric_limits<int32_t>::max()) {
                continue;
            }
        }
        pResult = xSubtitles.GetAt(iSubtitleCt);
        fClosestSqrDist = fCurrSqrDist;
    }
    return pResult;
}

void HUDMainMenu::HideMessage() {
    if (!xMessages.IsEmpty()) {
        for (HUDMainMenu::MessageData* pItem : xMessages) {
            if (pItem) {
                const char* pText = HUDMainMenu::pme->pTiles[26]->GetString(4036);
                if (!BSstrcmp(pItem->strText, pText)) {
                    xMessages.Remove(pItem);
                    ourFree(pItem);
                    if (xMessages.IsEmpty()) {
                        HUDMainMenu::pme->pTiles[26]
                            ->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.35f, Tile::TFCT_STANDARD);
                        HUDMainMenu::pme->pTiles[25]
                            ->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.35f, Tile::TFCT_STANDARD);
                        HUDMainMenu::pme->pTiles[27]
                            ->AddFadeControl(4009, fCurrentHUDOpacity, 0.0f, 0.35f, Tile::TFCT_STANDARD);
                        uiMessageStartTime = 0;
                    }
                    else {
                        if (xMessages.m_item->SoundName[0]) {
                            pPlayer->PlaySoundByEditorName(xMessages.m_item->SoundName, false, 289u, false);
                        }
                        HUDMainMenu::pme->pTiles[26]->Set(4036, xMessages.m_item->strText, true);
                        HUDMainMenu::pme->pTiles[25]->Set(4044, xMessages.m_item->IconFilename, true);
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
        HUDMainMenu::pme->bKillQuestUpdates = true;
    }
}

void HUDMainMenu::ShowCrippledGuy() {
    char acPath[268];
    int32_t iFaceImage = 0;
    for (ActiveEffect*& rpItem : pPlayer->GetActiveEffectList()->ItemsUntilNull()) {
        if (rpItem->eSpellType == MagicSystem::ADDICTION) {
            iFaceImage = 10;
        }
    }
    if (!iFaceImage) {
        float fMax = pPlayer->GetPermanentActorFloatValue(ActorValue::HEALTH);
        float fDmg = fMax - pPlayer->GetActorFloatValue(ActorValue::HEALTH);
        iFaceImage = static_cast<int32_t>(fDmg / fMax * 5.0);
        if (iFaceImage < 0) {
            iFaceImage = 0;
        }
        if (iFaceImage > 4) {
            iFaceImage = 4;
        }
    }
    BSsprintf(acPath, 260, "Interface\\Stats\\face_%02d.dds", iFaceImage);
    pTiles[52]->Set(4044, acPath, true);
    pTiles[52]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 5.0f, Tile::TFCT_FADE_IN_HOLD_FADE_OUT);
    HUDMainMenu::HUD_MAIN_MENU_TILES eTile = HUDMainMenu::HMMT_CRIPPLED_HEAD;
    for (ActorValue::Index eIndex = ActorValue::PERCEPTION_CONDITION; eIndex < ActorValue::BRAIN_CONDITION;
         eIndex = static_cast<ActorValue::Index>(eIndex + 1)) {
        const char* pFormat;
        float fVal = pPlayer->GetActorFloatValue(eIndex) / 100.0;
        if (fVal > 0.0) {
            pFormat = "Interface\\Stats\\%s.dds";
        }
        else {
            pFormat = "Interface\\Stats\\%s_broken.dds";
        }
        BSsprintf(acPath, 260, pFormat, HUDCrippledLimbNames[eIndex - ActorValue::PERCEPTION_CONDITION]);
        pTiles[eTile]->Set(4044, acPath, true);
        if (fVal == 0.0) {
            pTiles[eTile]->Set(4084, 2);
            pTiles[eTile]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 5.0f, Tile::TFCT_BLINK_FAST_FADE);
        }
        else {
            pTiles[eTile]->Set(4084, 1);
            pTiles[eTile]->AddFadeControl(4009, 0.0f, fCurrentHUDOpacity, 5.0f, Tile::TFCT_FADE_IN_HOLD_FADE_OUT);
        }
        eTile = static_cast<HUDMainMenu::HUD_MAIN_MENU_TILES>(eTile + 1);
    }
}

bool HUDMainMenu::IsQuestNameShowing() {
    if (!bQuestNameShowing) {
        return false;
    }
    NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[14]->xChildren.m_pkHead;
    while (pPos) {
        Tile* pChild = HUDMainMenu::pme->pTiles[14]->xChildren.GetNext(pPos);
        if (pChild->Tile::IsTrue(4003)) {
            return true;
        }
    }
    bQuestNameShowing = false;
    return false;
}

bool HUDMainMenu::AreObjectivesShowing() {
    if (!bObjectiveNameShowing) {
        return false;
    }
    NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[13]->xChildren.m_pkHead;
    while (pPos) {
        Tile* pChild = HUDMainMenu::pme->pTiles[13]->xChildren.GetNext(pPos);
        if (pChild->Tile::IsTrue(4003)) {
            return true;
        }
    }
    bObjectiveNameShowing = false;
    return false;
}

void HUDMainMenu::UpdateRadiationMeterConditions(float afDrinkinRads, float afStandinRads) {
    float fTotalRads = afDrinkinRads + afStandinRads;
    if (afDrinkinRads > 0.0) {
        float fDrankRadsEnd = static_cast<double>(appTimer.iLastTime) + 2000.0;
        HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User6, fDrankRadsEnd, true);
        HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User5, fTotalRads, true);
        HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User7, 1);
    }
    else if (afStandinRads > 0.0) {
        double dDrankRadsTime = HUDMainMenu::pme->pTiles[19]->GetFloat(Tile::kTileValue_User6);
        if (static_cast<double>(appTimer.iLastTime) >= dDrankRadsTime) {
            HUDMainMenu::pme->pTiles[19]->Set(Tile::kTileValue_User5, fTotalRads, true);
        }
    }
}

void HUDMainMenu::StatusModeTileEffects(Tile* apTile, float afCurrValue, bool bResetValues) {
    float fRate = 0.5f;
    if (fHCFlashTextThreshold.Float() <= static_cast<double>(afCurrValue)) {
        apTile->Set(4084, 2);
        apTile->Set(4009, fCurrAlphaHardcore, true);
    }
    else if (fHCRedTextThreshold.Float() <= static_cast<double>(afCurrValue)) {
        apTile->Set(4084, 2);
        apTile->Set(4009, 255);
    }
    else if (fHCFullAlphaTextThreshold.Float() <= static_cast<double>(afCurrValue)) {
        apTile->Set(4084, 1);
        apTile->Set(4009, 255);
    }
    else if (fHCHalfAlphaTextThreshold.Float() <= static_cast<double>(afCurrValue)) {
        apTile->Set(4084, 1);
        apTile->Set(4009, 124);
    }
    else {
        apTile->Set(4084, 1);
        apTile->Set(4009, 0);
    }
    if (bResetValues) {
        double dAlpha;
        uint32_t uiTime = appTimer.GetTime();
        uint32_t uiElapsed = uiTime - iLastHardcoreFadeTime;
        iLastHardcoreFadeTime = uiTime;
        if (bAlphaGoingDown) {
            dAlpha = fCurrAlphaHardcore - static_cast<double>(uiElapsed) * fRate;
        }
        else {
            dAlpha = static_cast<double>(uiElapsed) * fRate + fCurrAlphaHardcore;
        }
        fCurrAlphaHardcore = dAlpha;
        if (fCurrAlphaHardcore >= 255.0) {
            bAlphaGoingDown = 1;
        }
        if (fCurrAlphaHardcore <= 128.0) {
            bAlphaGoingDown = 0;
        }
        BSClamp(fCurrAlphaHardcore, 128.0f, 255.0f);
    }
}

void HUDMainMenu::HandleReminderFade(Tile* apTile) {
    int32_t iScreenWidth = static_cast<int32_t>(InterfaceManager::GetScreenWidth());
    bool bShowedObjective = false;
    if (!AreObjectivesShowing()) {
        BSSimpleList<QueuedQuestText*>* pList = &xQuestObjectives;
        // The objective text tiles are the quest stage tile's children after its first 31; their check boxes are the
        // children from the tail backwards. pme is reloaded for each list call.
        NiTListItem<Tile*>* pQuestPos = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.m_pkHead;
        for (int32_t i = 0; i < 31; ++i) {
            HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetNext(pQuestPos);
        }
        NiTListItem<Tile*>* pCheckBoxPos = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.m_pkTail;
        while (pList && !pList->IsEmpty() && pQuestPos) {
            bool bNotShowing = !AreObjectivesShowing();
            bool bTileIsReminder;
            if (bNotShowing) {
                bTileIsReminder = false;
            }
            else {
                int32_t iTrait = Tile::TextToTrait("_IsReminder");
                bTileIsReminder = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetHead()->IsTrue(iTrait);
            }
            bool bItemIsReminder = pList->m_item->bReminder;
            // While objectives show, only texts of the same kind (reminder or not) as the first tile are taken.
            if ((!bNotShowing && bTileIsReminder && !bItemIsReminder) ||
                (!bNotShowing && !bTileIsReminder && bItemIsReminder)) {
                pList = pList->m_pkNext;
                continue;
            }
            bObjectiveNameShowing = true;
            bShowedObjective = true;
            Tile* pQuest = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetPrev(pQuestPos);
            Tile* pCheckBox = HUDMainMenu::pme->pTiles[HMMT_QUEST_STAGES]->xChildren.GetPrev(pCheckBoxPos);
            if (pList->m_item->bCompleted) {
                char acText[260];
                BSstrcpy(acText, 260, sHUDQuestCompleted.String());
                BSstrcat(acText, 260, pList->m_item->msg);
                pQuest->Set(Tile::kTileValue_String, acText, true);
                int32_t iIsReminder = bItemIsReminder;
                pQuest->Set(Tile::TextToTrait("_IsReminder"), iIsReminder);
                pQuest->Set(Tile::kTileValue_User3, 1);
                pCheckBox
                    ->Set(Tile::kTileValue_Filename, "Interface\\Shared\\Marker\\glow_square_filled_small.dds", true);
            }
            else {
                pQuest->Set(Tile::kTileValue_String, pList->m_item->msg, true);
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
            BSAudio::QInstance()
                .GetSoundHandleByName(
                    "UIQuestUpdate",
                    BSGameSound::SOUND_FLAG_IS_2D | BSGameSound::SOUND_FLAG_SYSTEM_SOUND |
                        BSGameSound::SOUND_FLAG_ONE_SHOT
                )
                .Play(false);
            pList->Remove(pList->m_item);
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
                NiPoint3 kTextDimensions = FontManager::Instance()
                                               ->CalculateStringDimensions(pString, iFont, fWrapWidth, 0);
                iY += static_cast<int32_t>(kTextDimensions.y);
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
            // Compares the next free text entry with the previous check box entry.
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
    }
    else if (aTiles[0]->GetFloat(trait_WaitCompleted) == 1.0) {
        static int32_t trait = Tile::AddUserTrait("_HoldTime", -1);
        if (aTiles[0]->GetFloat(trait) == 0.0) {
            // Holds the first text.
            aTiles[0]->Set(trait, 0);
            aTiles[0]
                ->AddFadeControl(trait, 0.0f, 1.0f, fQuestCinematicObjectivePauseTime.Float(), Tile::TFCT_STANDARD);
        }
        else if (aTiles[0]->GetFloat(trait) == 1.0) {
            // The hold is over: fades the first text and its check box out.
            aTiles[0]->AddFadeControl(
                Tile::kTileValue_Alpha,
                fCurrentHUDOpacity,
                0.0f,
                fQuestCinematicObjectiveFadeOut.Float(),
                Tile::TFCT_STANDARD
            );
            aTiles[0]->Set(trait_WaitCompleted, 2);
            aBoxes[0]->AddFadeControl(
                Tile::kTileValue_Alpha,
                fCurrentHUDOpacity,
                0.0f,
                fQuestCinematicObjectiveFadeOut.Float(),
                Tile::TFCT_STANDARD
            );
        }
    }
    else if (aTiles[0]->GetFadeFinished(Tile::kTileValue_Alpha) && aTiles[0]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
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
    if (aTiles[1] &&
        aTiles[1]->GetFloat(trait_WaitCompleted) == 1.0 &&
        aTiles[1]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
        aTiles[1]->AddFadeControl(
            Tile::kTileValue_Alpha,
            0.0f,
            fCurrentHUDOpacity,
            fQuestCinematicObjectiveFadeIn.Float(),
            Tile::TFCT_STANDARD
        );
        aBoxes[1]->AddFadeControl(
            Tile::kTileValue_Alpha,
            0.0f,
            fCurrentHUDOpacity,
            fQuestCinematicObjectiveFadeIn.Float(),
            Tile::TFCT_STANDARD
        );
    }
    if (aTiles[2] &&
        aTiles[2]->GetFloat(trait_WaitCompleted) == 1.0 &&
        aTiles[2]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
        aTiles[2]->AddFadeControl(
            Tile::kTileValue_Alpha,
            0.0f,
            fCurrentHUDOpacity,
            fQuestCinematicObjectiveFadeIn.Float(),
            Tile::TFCT_STANDARD
        );
        aBoxes[2]->AddFadeControl(
            Tile::kTileValue_Alpha,
            0.0f,
            fCurrentHUDOpacity,
            fQuestCinematicObjectiveFadeIn.Float(),
            Tile::TFCT_STANDARD
        );
    }
}

void HUDMainMenu::HandleCinematicFade(Tile* th_s) {
    if (th_s->GetFloat(4022) > 0.0) {
        NiTListItem<Tile*>* pPos = HUDMainMenu::pme->pTiles[13]->xChildren.m_pkHead;
        while (pPos) {
            Tile* pChild = HUDMainMenu::pme->pTiles[13]->xChildren.GetNext(pPos);
            if (pChild) {
                pChild->Tile::Set(4009, 0.0f, true);
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
                    if (pChild->Tile::GetFloat(4009) > 175.0) {
                        fStart = pChild->Tile::GetFloat(4009);
                    }
                    else {
                        fStart = 175.0f;
                    }
                    pChild->Tile::AddFadeControl(
                        4009,
                        fStart,
                        0.0f,
                        fQuestCinematicCharacterFadeOut.Float(),
                        Tile::TFCT_STANDARD
                    );
                    pChild->Tile::Set(0x1005, 0);
                    continue;
                }
            }
            if (pChild &&
                pChild->Tile::GetFadeEndFor(4009) == 0.0 &&
                pChild->Tile::GetFloat(4009) == 0.0 &&
                pChild->Tile::GetFloat(0x1005) == 0.0) {
                pChild->Tile::Set(4003, 0);
            }
        }
    }
}

void HUDMainMenu::ClearGunScope() {
    if (HUDMainMenu::pme && HUDMainMenu::spScopeModel.m_pObject) {
        if (HUDMainMenu::spScopeModel->GetAtChecked(0)) {
            HUDMainMenu::spScopeModel->DetachChild(HUDMainMenu::spScopeModel->GetAt(0));
            HUDMainMenu::pme->bScopeVisible = false;
        }
    }
}

int32_t HUDMainMenu::GetMenuID() {
    return 1004;
}

HUDMainMenu::HUD_MENU_MODE HUDMainMenu::GetMenuMode() {
    if (HUDMainMenu::pme) {
        return HUDMainMenu::pme->eMenuMode;
    }
    return HMM_NONE;
}

Actor* HUDMainMenu::GetEnemyActor() {
    if (HUDMainMenu::pme) {
        return HUDMainMenu::pme->pCurrentEnemy;
    }
    return nullptr;
}

bool HUDMainMenu::IsGunScopeVisible() {
    return HUDMainMenu::pme && HUDMainMenu::pme->bScopeVisible;
}

bool HUDMainMenu::ClearLastObjectIfItIs(TESObjectREFR* apObject) {
    if (!HUDMainMenu::pme || HUDMainMenu::pme->pLastObject != apObject) {
        return false;
    }
    HUDMainMenu::pme->pLastObject = nullptr;
    return true;
}

void HUDMainMenu::RefreshInfoForRef() {
    if (HUDMainMenu::pme) {
        HUDMainMenu::SetInfoForRef(HUDMainMenu::pme->pLastObject, false, true);
    }
}

// The prompt types are GetRefPromptType's. The flags of the info lines to show are kept in a local: 2 the action hint,
// 4 the name and the door state, 0x8 to 0x80 the info labels and values. Each "set the crosshair image" step reads the
// image name before reloading the crosshair tile.
bool HUDMainMenu::SetInfoForRef(TESObjectREFR* apObject, [[maybe_unused]] bool abTelekinesis, bool abSameOkay) {
    if (!HUDMainMenu::pme || !pPlayer) {
        return false;
    }
    if (apObject == pPlayer) {
        apObject = nullptr;
    }

    if (!abSameOkay && apObject == HUDMainMenu::pme->pLastObject) {
        if (bHostileUnderCrosshair && bLongDistancePick) {
            const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 2);
        }
        else if (!apObject || (apObject->IsActor() && !static_cast<Actor*>(apObject)->IsInCombatWithActor(pPlayer))) {
            if (!apObject) {
                const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
                HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
                HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
            }
            bHideEnemyHealthMeter = false;
            if (HUDMainMenu::GetMenuMode() == HMM_RUN_AND_GUN) {
                HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(Tile::kTileValue_Visible, 1);
            }
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
        }
        else if (bIsActor) {
            pPlayer->RemovePlayerAction(PLAYER_ACTION_PICKPOCKET, nullptr);
            pPlayer->RemovePlayerAction(PLAYER_ACTION_LOCKED_OBJECT, nullptr);
        }
        else {
            LockData* pLock = apObject->GetLock();
            if (bIsTerminal) {
                if (!static_cast<BGSTerminal*>(pBaseObject)->IsUnlocked(apObject) && apObject->GetOwner()) {
                    pPlayer->AddPlayerAction(PLAYER_ACTION_LOCKED_OBJECT, 10.0f, nullptr);
                    ReferenceProcessLists.ResetPlayerActionsCommentTimer();
                }
            }
            else if (apObject->GetObjectReference()->GetFormType() == CONT_ID && apObject->GetOwner()) {
                int32_t iCount = apObject->GetInventoryCount(false, true);
                if (iCount > 0 &&
                    pLock &&
                    pLock->IsLocked() &&
                    (!pLock->pKey || pPlayer->GetInventoryObjectCount(pLock->pKey) <= 0)) {
                    pPlayer->AddPlayerAction(PLAYER_ACTION_LOCKED_OBJECT, 10.0f, nullptr);
                    ReferenceProcessLists.ResetPlayerActionsCommentTimer();
                }
            }
            else if (
                pLock &&
                pLock->IsLocked() &&
                apObject->GetOwner() &&
                (!pLock->pKey || pPlayer->GetInventoryObjectCount(pLock->pKey) <= 0)
            ) {
                pPlayer->AddPlayerAction(PLAYER_ACTION_LOCKED_OBJECT, 10.0f, nullptr);
                ReferenceProcessLists.ResetPlayerActionsCommentTimer();
            }
        }
    }
    else {
        pPlayer->RemovePlayerAction(PLAYER_ACTION_PICKPOCKET, nullptr);
        pPlayer->RemovePlayerAction(PLAYER_ACTION_LOCKED_OBJECT, nullptr);
    }

    if (!apObject || !abSameOkay) {
        HUDMainMenu::pme->pLastObject = apObject;
    }
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
        if (!bHostileUnderCrosshair) {
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
        }
        const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
        HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
        bHideEnemyHealthMeter = false;
        if (HUDMainMenu::GetMenuMode() == HMM_RUN_AND_GUN) {
            HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(Tile::kTileValue_Visible, 1);
        }
        return true;
    }

    int32_t iShowFlags = 0;
    int32_t ePromptType = apObject->GetRefPromptType();
    // Sitting and sleeping need a free furniture marker (the object is tested again only before the first).
    if ((apObject && ePromptType == 5) || ePromptType == 3) {
        uint32_t uiMarkerIndex = 0;
        // Nothing of the mark is destroyed.
        FurnitureMark kMark(0xFF);
        if (!apObject
                 ->GetClosestFreeMarker(pPlayer->GetLocationOnReference(), true, true, kMark, uiMarkerIndex, true)) {
            return false;
        }
    }

    TESObjectWEAP* pWeapon = nullptr;
    TESAmmo* pAmmo = nullptr;
    bool bHostileTarget = pActor && pPlayer->IsActorPerceived(pActor) && pPlayer->IsActorPerceived(pActor)->bIsHostile;

    // The crosshair: red for a crime or a hostile target, the activate image for anything that can be used (and an
    // actor not in combat).
    if (ePromptType || bHostileTarget) {
        if (apObject->IsCrimeToActivate() && (!bIsActor || !pActor->IsPlayerTeammate())) {
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 2);
            const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User1);
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
        }
        else if (bHostileTarget) {
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 2);
            const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
        }
        else if (!bIsActor || !pActor->IsInCombat()) {
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
            const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User1);
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
        }
        else {
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
            const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
        }
    }
    else {
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
        case CREA_ID: break;
        case ARMO_ID: iShowFlags |= INFO_ITEM; break;
        case LIGH_ID: {
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1);
            const char* pImage = HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->GetString(Tile::kTileValue_User0);
            HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_Filename, pImage, true);
            if (!static_cast<TESObjectLIGH*>(apObject->GetObjectReference())->GetCanCarry()) {
                return true;
            }
            iShowFlags |= INFO_ITEM;
            break;
        }
        case MISC_ID:
        case PWAT_ID:
        case ALCH_ID:
        case IMOD_ID: iShowFlags |= INFO_ITEM; break;
        case STAT_ID:
        case SCOL_ID:
        case TREE_ID: bNoInfo = true; break;
        case WEAP_ID:
            pWeapon = static_cast<TESObjectWEAP*>(apObject->GetObjectReference());
            if (pWeapon->GetIsPlayable()) {
                iShowFlags |= INFO_ITEM;
            }
            break;
        case AMMO_ID:
            pAmmo = static_cast<TESAmmo*>(apObject->GetObjectReference());
            if (pAmmo->GetIsPlayable()) {
                iShowFlags |= INFO_ITEM;
            }
            break;
        case PROJ_ID: {
            // A projectile carrying an item counts as that item unless the item is a misc item.
            Projectile* pProjectile = static_cast<Projectile*>(apObject);
            if (pProjectile->GetSecondaryItemChange() &&
                pProjectile->GetSecondaryItemChange()->GetItemChangeObject()->GetFormType() != MISC_ID) {
                break;
            }
            if (apObject->GetFormType() == PMIS_ID) {
                bNoInfo = true;
            }
            else {
                iShowFlags |= INFO_ITEM;
            }
            break;
        }
        default: HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR_CENTER]->Set(Tile::kTileValue_SystemColor, 1); break;
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

    // The door state line: broken, inaccessible, or the lock level (or the key the player has).
    DoorTeleportData* pTeleport = apObject->GetTeleport();
    LockData* pLock = apObject->GetLock();
    // Fetches the base object twice more and discards the results.
    apObject->GetObjectReference();
    apObject->GetObjectReference();
    PACKAGE_OBJECT_TYPE eMatchType = POTYPE_NONE;
    bool bCanOpen = false;
    if (eFormType != DOOR_ID && eFormType != CONT_ID) {
        bCanOpen = true;
    }
    else if (eFormType == DOOR_ID && TESObjectDOOR::CanActorIgnoreLock(apObject, pPlayer, false, true)) {
        bCanOpen = true;
    }
    else if (eFormType == CONT_ID) {
        if (!pLock || !pLock->IsLocked()) {
            bCanOpen = true;
        }
        else if (pLock->pKey && pPlayer->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eMatchType)) {
            bCanOpen = true;
        }
    }
    int32_t iTerminalDifficulty = bIsTerminal
        ? static_cast<BGSTerminal*>(pBaseObject)->GetHackDifficultyLockLevel(apObject)
        : 0;
    bool bTerminalLocked = pPlayer->GetTerminalAccess(apObject) == PlayerCharacter::TAT_LOCKED ||
        pPlayer->GetTerminalAccess(apObject) == PlayerCharacter::TAT_HACK_ELIGIBLE ||
        pPlayer->GetTerminalAccess(apObject) == PlayerCharacter::TAT_HACK_INELIGIBLE;
    if ((bIsTerminal && bTerminalLocked) || apObject->GetInaccessible() || (pLock && pLock->IsLocked())) {
        int32_t iLevel = 0;
        if (bIsTerminal) {
            iLevel = iTerminalDifficulty;
        }
        else {
            iLevel = GetLockPickLevel(apObject->GetLockLevel());
        }
        iLevel = BSMIN(BSMAX(iLevel, 0), 5);
        int32_t iLockLevel = iLevel;
        PACKAGE_OBJECT_TYPE eKeyMatchType;
        bool bHasKey = pLock && pLock->pKey && pPlayer->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eKeyMatchType);
        if (pLock && pLock->IsBroken()) {
            HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, sBroken.String(), true);
            float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
            HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
        }
        else if (apObject->GetInaccessible()) {
            HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, sInaccessible.String(), true);
            float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
            HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
        }
        else if (((eFormType == DOOR_ID || eFormType == CONT_ID) && (!bCanOpen || bHasKey)) || eFormType == TERM_ID) {
            if (bHasKey) {
                const char* pKeyText = sHUDUseKey.String();
                const char* pLocked = sLocked.String();
                BSsprintf(lockLevel, 0x104, "[%s - %s]", pLocked, pKeyText);
                HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, lockLevel, true);
            }
            else {
                if (iLockLevel == 5) {
                    BSstrcpy(lockLevel, 0x104, "[");
                    BSstrcat(lockLevel, 0x104, pLockLevelNames[iLockLevel]->String());
                    BSstrcat(lockLevel, 0x104, "]");
                }
                else {
                    const char* pLevelName = pLockLevelNames[iLockLevel]->String();
                    const char* pLocked = sLocked.String();
                    BSsprintf(lockLevel, 0x104, "[%s - %s]", pLocked, pLevelName);
                }
                HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_String, lockLevel, true);
            }
            float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
            HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
        }
        else {
            HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, 0);
        }
    }
    else {
        HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(Tile::kTileValue_Alpha, 0);
    }

    // The container state line: shown for an empty container (or an empty ash pile) that is no live actor.
    bool bAshPileEmpty = false;
    TESObjectREFR* pAshPile = apObject->GetExtra()->GetAshPileRef();
    if (pAshPile && pAshPile->HasContainer() && pAshPile->GetInventoryCount(false, false) <= 0) {
        bAshPileEmpty = true;
    }
    TESObjectREFR* pDroppedWeapon = apObject->GetExtra()->GetDroppedWeapon();
    if (!pDroppedWeapon &&
        ((apObject->HasContainer() && apObject->GetInventoryCount(false, false) <= 0) || bAshPileEmpty) &&
        (!bIsActor || apObject->IsDead(true))) {
        float fAlpha = static_cast<float>(fHudOpacity.Float() * 255.0);
        HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Alpha, fAlpha, true);
    }
    else {
        HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Alpha, 0);
    }

    // An actor's location query records the position in its swim history, so both stay.
    if (HUDMainMenu::pme->pCurrentEnemy) {
        HUDMainMenu::pme->pCurrentEnemy->GetLocationOnReference();
        pPlayer->GetLocationOnReference();
    }
    bool bIsHostileEnemy = HUDMainMenu::pme->pCurrentEnemy &&
        !HUDMainMenu::pme->pCurrentEnemy->IsDead(false) &&
        !HUDMainMenu::pme->pCurrentEnemy->GetDestroyed() &&
        bCurrentEnemyHostile &&
        HUDMainMenu::pme->pCurrentEnemy == pActor;

    // The name: a projectile shows the item it carries or the weapon that fired it.
    if (apObject) {
        Projectile* pProjectile = apObject->IsProjectile() ? static_cast<Projectile*>(apObject) : nullptr;
        if (pProjectile && pProjectile->GetSecondaryItemChange()) {
            TESBoundObject* pItem = pProjectile->GetSecondaryItemChange()->GetItemChangeObject();
            BSstrcpy(g_sCrosshairRefName, 0x104, TESFullName::GetFullName(pItem));
        }
        else if (pProjectile && pProjectile->GetWeaponSource()) {
            TESObjectWEAP* pSourceWeapon = pProjectile->GetWeaponSource();
            if (pSourceWeapon->GetProjectile(nullptr) &&
                pSourceWeapon->GetProjectile(nullptr)->GetDefaultWeaponSource()) {
                pSourceWeapon = pSourceWeapon->GetProjectile(nullptr)->GetDefaultWeaponSource();
            }
            BSstrcpy(g_sCrosshairRefName, 0x104, TESFullName::GetFullName(pSourceWeapon));
        }
        else {
            BSstrcpy(g_sCrosshairRefName, 0x104, apObject->GetFullName());
        }
    }
    else {
        g_sCrosshairRefName[0] = '\0';
    }
    bool bPlayerInCombat = pPlayer->GetNumberActorsInCombat() > 0;
    if (g_sCrosshairRefName[0]) {
        if (pTeleport) {
            // A door shows where it leads.
            BSStringT<char> kMapName;
            pTeleport->GetTeleportCellMapName(kMapName);
            const char* pMapNameText = kMapName.String();
            const char* pTo = sTo.String();
            char acDoorText[260];
            BSsprintf(acDoorText, 0x104, "%s %s %s", g_sCrosshairRefName, pTo, pMapNameText);
            HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, acDoorText, true);
            iShowFlags |= INFO_ACTION_NAME;
        }
        else if (
            !(bIsActor &&
              (pActor == HUDMainMenu::pme->pCurrentEnemy || pActor->IsInCombatWithActor(pPlayer)) &&
              bPlayerInCombat &&
              !pActor->IsDead(false)) &&
            !bIsHostileEnemy
        ) {
            // An actor fighting the player shows no name; a stack shows its count.
            bool bShowName = true;
            if (apObject->GetExtra()->GetCount() == 1) {
                if (bIsActor) {
                    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]
                        ->Set(Tile::kTileValue_String, g_sCrosshairRefName, true);
                }
                else if (
                    (pAmmo && pAmmo->GetIsPlayable()) || (pWeapon && pWeapon->GetIsPlayable()) || (!pWeapon && !pAmmo)
                ) {
                    if (apObject->GetObjectReference()->GetFormType() == AMMO_ID &&
                        static_cast<TESAmmo*>(apObject->GetObjectReference())->GetFormClipRounds() > 1) {
                        int32_t iCount = apObject->GetCount();
                        char acCountText[260];
                        BSsprintf(acCountText, 0x104, "%s (%d)", g_sCrosshairRefName, iCount);
                        HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]
                            ->Set(Tile::kTileValue_String, acCountText, true);
                    }
                    else {
                        HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]
                            ->Set(Tile::kTileValue_String, g_sCrosshairRefName, true);
                    }
                }
                else {
                    bShowName = false;
                }
            }
            else {
                char acStackText[260];
                BSsprintf(
                    acStackText,
                    0x104,
                    "%s (%d)",
                    g_sCrosshairRefName,
                    static_cast<int32_t>(apObject->GetExtra()->GetCount())
                );
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, acStackText, true);
            }
            HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 0);
            HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 0);
            HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 0);
            if (bShowName) {
                iShowFlags |= INFO_ACTION_NAME;
            }
        }
    }
    else {
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
    if (apObject &&
        !bIsHostileEnemy &&
        !(eRefPromptType == 7 &&
          bIsActor &&
          (bCompassHostile || pActor == HUDMainMenu::pme->pCurrentEnemy || pActor->IsInCombatWithActor(pPlayer)) &&
          (pActor->IsInCombat(false) || pActor->IsAttacking() || bCompassHostile)) &&
        !apObject->GetDestroyed()) {
        auto* pActivateRef = static_cast<ExtraActivateRef*>(apObject->GetExtraData(EXTRA_ACTIVATE_REF));
        BSStringT<char> kPrompt("");
        BSStringT<char>* pPrompt = &kPrompt;
        if (pActivateRef) {
            pPrompt->Copy(pActivateRef->ActivateTextOverride);
        }
        if (!pPrompt->GetLength()) {
            auto* pActivator = dynamic_cast<TESObjectACTI*>(apObject->GetObjectReference());
            if (pActivator) {
                pPrompt->Copy(pActivator->GetActivationString());
            }
        }
        if (pPrompt->GetLength()) {
            HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, pPrompt->String(), true);
            HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_String, "", true);
            iShowFlags |= INFO_ACTION_HINT;
        }
        else if (eRefPromptType == 1 && apObject->IsCrimeToActivate()) {
            // Taking from someone: talk to a teammate, pickpocket an NPC (or a creature that allows dialogue),
            // steal anything else.
            if (bIsActor && pActor->IsPlayerTeammate()) {
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                    ->Set(Tile::kTileValue_String, psTargetTypes[7]->String(), true);
            }
            else if (eFormType == NPC__ID) {
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                    ->Set(Tile::kTileValue_String, sPickpocket.String(), true);
            }
            else if (eFormType == CREA_ID) {
                TESActorBase* pActorBase = pActor->GetActorBase();
                if (pActorBase->GetAllowPCDialogue()) {
                    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                        ->Set(Tile::kTileValue_String, sPickpocket.String(), true);
                }
                else {
                    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                        ->Set(Tile::kTileValue_String, sSteal.String(), true);
                }
            }
            else {
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sSteal.String(), true);
            }
            iShowFlags |= INFO_ACTION_HINT;
        }
        else if (
            apObject->GetObjectReference() == pAshPile1 ||
            apObject->GetObjectReference() == pAshPile2 ||
            (eRefPromptType == 2 && (eFormType == NPC__ID || eFormType == CREA_ID))
        ) {
            HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sSearch.String(), true);
            iShowFlags |= INFO_ACTION_HINT;
        }
        else if (eRefPromptType == 8 && eFormType == DOOR_ID && !apObject->GetInaccessible()) {
            // A door: pick its lock, or open or close it.
            bool bOpen = BGSOpenCloseForm::GetOpenState(apObject) == BGSOpenCloseForm::OBJECT_OPEN ||
                BGSOpenCloseForm::GetOpenState(apObject) == BGSOpenCloseForm::OBJECT_OPENING;
            if (pLock && pLock->IsLocked() && !apObject->IsAnOwner(pPlayer, true) && !bCanOpen) {
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, sPick.String(), true);
            }
            else {
                const char* pHint = bOpen ? sCloseButton.String() : psTargetTypes[eRefPromptType]->String();
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_String, pHint, true);
            }
            iShowFlags |= INFO_ACTION_HINT;
        }
        else if (eRefPromptType == 4 && eFormType == CREA_ID) {
            iShowFlags &= ~3;
        }
        else if (eRefPromptType == 6) {
            HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                ->Set(Tile::kTileValue_String, psTargetTypes[1]->String(), true);
            iShowFlags |= INFO_ACTION_HINT;
        }
        else if (eRefPromptType && eRefPromptType < 20 && !apObject->GetInaccessible()) {
            // No prompt to talk to an essential actor while sneaking.
            if (eRefPromptType != 1 || !bIsActor || !pPlayer->IsSneaking() || !pActor->GetEssential()) {
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                    ->Set(Tile::kTileValue_String, psTargetTypes[eRefPromptType]->String(), true);
                iShowFlags |= INFO_ACTION_HINT;
            }
        }
        else if (apObject->IsProjectile()) {
            // A mine (a projectile with a proximity trigger): disarm it, or take it once it is turned off. The base is
            // read through Actor::GetActorBase on the projectile.
            auto* pProjectileBase = static_cast<BGSProjectile*>(static_cast<TESForm*>(pActor->GetActorBase()));
            if (pProjectileBase->GetExplosionProximity() != 0.0) {
                if (!static_cast<Projectile*>(apObject)->IsFlagSet(Projectile::TURNED_OFF)) {
                    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                        ->Set(Tile::kTileValue_String, sDisarmMine.String(), true);
                }
                else {
                    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                        ->Set(Tile::kTileValue_String, psTargetTypes[1]->String(), true);
                }
                iShowFlags |= INFO_ACTION_HINT;
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
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]
                    ->Set(Tile::TextToTrait("_PCButtonText"), acButtonText, true);
            }
            else {
                HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_PCButtonText"), "", true);
            }
        }
        else {
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
                default: break;
            }
        }
    }

    // The item the info lines describe: a projectile's carried item, else a change for the thrown weapon or the base
    // object (a temporary destroyed once it is copied), with the reference's extra data as its instance.
    Projectile* pProjectile = apObject->IsProjectile() ? static_cast<Projectile*>(apObject) : nullptr;
    bool bHasSecondaryItem = pProjectile && pProjectile->GetSecondaryItemChange();
    TESObjectWEAP* pSourceWeapon = pProjectile ? pProjectile->GetWeaponSource() : nullptr;
    TESObjectWEAP* pThrownWeapon = pSourceWeapon && pSourceWeapon->IsThrownWeapon() ? pSourceWeapon : nullptr;
    // The temporary is bound to a const reference only to take its address, as the game's `&ItemChange(...)` does.
    ItemChange kItem(
        bHasSecondaryItem ? pProjectile->GetSecondaryItemChange()
                          : &static_cast<const ItemChange&>(
                                ItemChange(pThrownWeapon ? pThrownWeapon : apObject->GetObjectReference(), 0)
                            )
    );
    if (!kItem.pExtraObjectList) {
        kItem.pExtraObjectList = new BSSimpleList<ExtraDataList*>;
    }
    ExtraDataList* pExtra = apObject->GetExtra();
    kItem.pExtraObjectList->AddHead(pExtra);

    // Water shows its health and radiation; anything else its weight and value.
    TESWaterForm* pWater = nullptr;
    int32_t eBaseFormType = apObject->GetObjectReference()->GetFormType();
    if (eBaseFormType == ACTI_ID) {
        pWater = apObject->GetObjectReference()->GetWaterType();
    }
    else if (eBaseFormType == PWAT_ID) {
        pWater = apObject->GetObjectReference()->GetWaterType();
    }
    if (pWater) {
        int32_t iRadiation = pWater->GetRadiationDamage();
        int32_t iHealth = pWater->GetHealthRestoration();
        int32_t iInfoLabel = HMMT_INFO_LABEL_ONE;
        int32_t iInfoValue = HMMT_INFO_VALUE_ONE;
        HUDMainMenu::DisplayWaterHealth(iHealth, iInfoLabel, iInfoValue);
        iInfoLabel = iInfoLabel == HMMT_INFO_LABEL_ONE ? HMMT_INFO_LABEL_TWO : iInfoLabel;
        iInfoValue = iInfoValue == HMMT_INFO_VALUE_ONE ? HMMT_INFO_VALUE_TWO : iInfoValue;
        HUDMainMenu::DisplayWaterRadiation(iRadiation, iInfoLabel, iInfoValue);
        iInfoLabel = iInfoLabel == HMMT_INFO_LABEL_ONE ? HMMT_INFO_LABEL_TWO : iInfoLabel;
        iInfoValue = iInfoValue == HMMT_INFO_VALUE_ONE ? HMMT_INFO_VALUE_TWO : iInfoValue;
        iShowFlags |= INFO_ITEM;
    }
    else {
        // Both texts take "--" for zero, negative and NaN amounts.
        float fValue = kItem.GetItemValue();
        if (fValue > 0.0) {
            if (fValue < 1.0) {
                BSsprintf(caps, 8, "%.1f", fValue);
            }
            else {
                BSsprintf(caps, 8, "%.0f", fValue);
            }
        }
        else {
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
        }
        else {
            bool bHardcore = pPlayer->IsHardcore();
            fWeight = TESWeightForm::GetFormWeight(kItem.GetItemChangeObject(), bHardcore);
        }
        if (fWeight > 0.0) {
            BSsprintf(wg, 8, "%.1f", fWeight);
        }
        else {
            BSstrcpy(wg, 8, "--");
        }
        HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(Tile::kTileValue_String, wg, true);
        HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_ONE]
            ->Set(Tile::kTileValue_String, sInventoryWeightUpper.String(), true);
        HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(Tile::kTileValue_String, caps, true);
        HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(Tile::kTileValue_String, sInventoryValue.String(), true);
    }

    static Tile* phintButtonText = Tile::GetTileByName(HUDMainMenu::pme->pTiles[HMMT_INFO], "button_text");

    // The info lines are red for a crime or a hostile actor.
    int32_t iInfoColor;
    if ((apObject->IsCrimeToActivate() && (!bIsActor || !pActor->IsPlayerTeammate())) ||
        (bIsActor && (pActor->IsInCombatWithActor(pPlayer) || bCompassHostile))) {
        iInfoColor = 2;
    }
    else {
        iInfoColor = 1;
    }
    for (int32_t iTile = HMMT_INFO_ACTION_HINT; iTile <= HMMT_INFO_SEPARATOR; ++iTile) {
        HUDMainMenu::pme->pTiles[iTile]->Set(Tile::kTileValue_SystemColor, iInfoColor);
    }
    phintButtonText->AddNeedsUpdate(Tile::CHANGE_COLOR);

    // Lays the hint out above the name, whose height depends on its wrapped text, and the subtitles above both.
    float fNameOffset = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(Tile::kTileValue_User8);
    float fWrapWidth = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(Tile::kTileValue_WrapWidth);
    int32_t iFont = static_cast<int32_t>(
        HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(Tile::kTileValue_Font)
    );
    const char* pNameText = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->GetString(Tile::kTileValue_String);
    NiPoint3 kDimensions = FontManager::Instance()->CalculateStringDimensions(pNameText, iFont, fWrapWidth, 0);
    float fNameHeight = kDimensions.y;
    float fHintBase = HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->GetFloat(Tile::kTileValue_User0);
    float fHintY = static_cast<float>(
        static_cast<double>(fHintBase) - (static_cast<double>(fNameHeight) - fNameOffset)
    );
    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::TextToTrait("_y"), fHintY, true);
    float fNameY = static_cast<float>(
        static_cast<double>(HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->GetFloat(Tile::kTileValue_Height)) + fHintY
    );
    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(Tile::kTileValue_Y, fNameY, true);
    double dInfoY = static_cast<double>(HUDMainMenu::pme->pTiles[HMMT_INFO]->GetFloat(Tile::kTileValue_Y)) + fHintY;
    float fSubtitleY = static_cast<float>(
        dInfoY - HUDMainMenu::pme->pTiles[HMMT_SUBTITLE]->GetFloat(Tile::kTileValue_Height) - 50.0
    );
    HUDMainMenu::pme->pTiles[HMMT_SUBTITLE]->Set(Tile::kTileValue_Y, fSubtitleY, true);

    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(Tile::kTileValue_Visible, iShowFlags & INFO_ACTION_HINT);
    HUDMainMenu::pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(
        Tile::kTileValue_Visible,
        static_cast<int32_t>((iShowFlags & INFO_ACTION_NAME) > 0)
    );
    if (iShowFlags & (INFO_ACTION_HINT | INFO_ACTION_NAME)) {
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(Tile::kTileValue_Visible, 0);
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(Tile::kTileValue_Visible, 0);
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_Visible, 0);
    }
    HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(
        Tile::kTileValue_Visible,
        static_cast<int32_t>((iShowFlags & INFO_ACTION_NAME) > 0)
    );
    // The container state shows with the name while the door state is hidden (alpha 0).
    int32_t iContainerVisible;
    if ((iShowFlags & INFO_ACTION_NAME) > 0 &&
        HUDMainMenu::pme->pTiles[HMMT_INFO_DOOR_STATE]->GetFloat(Tile::kTileValue_Alpha) == 0.0) {
        iContainerVisible = 1;
    }
    else {
        iContainerVisible = 0;
    }
    HUDMainMenu::pme->pTiles[HMMT_INFO_CONTAINER_STATE]->Set(Tile::kTileValue_Visible, iContainerVisible);
    HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(
        Tile::kTileValue_Visible,
        static_cast<int32_t>((iShowFlags & INFO_LABEL_ONE) > 0)
    );
    HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(
        Tile::kTileValue_Visible,
        static_cast<int32_t>((iShowFlags & INFO_VALUE_ONE) > 0)
    );
    HUDMainMenu::pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(
        Tile::kTileValue_Visible,
        static_cast<int32_t>((iShowFlags & INFO_LABEL_TWO) > 0)
    );
    HUDMainMenu::pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(
        Tile::kTileValue_Visible,
        static_cast<int32_t>((iShowFlags & INFO_VALUE_TWO) > 0)
    );
    HUDMainMenu::pme->pTiles[HMMT_INFO_SEPARATOR]->Set(
        Tile::kTileValue_Visible,
        static_cast<int32_t>((iShowFlags & INFO_SEPARATOR) > 0)
    );
    return true;
}

void HUDMainMenu::DisplayWaterHealth(int32_t aiHealth, int32_t aiInfoLabel, int32_t aiInfoValue) {
    char acText[268];
    std::sprintf(acText, "%i", aiHealth);
    HUDMainMenu::pme->pTiles[aiInfoLabel]->Set(4036, acText, true);
    const char* pText = sHitPointsShort.String();
    HUDMainMenu::pme->pTiles[aiInfoValue]->Set(4036, pText, true);
}

void HUDMainMenu::DisplayWaterRadiation(int32_t aiRadiationAmount, int32_t aiInfoLabel, int32_t aiInfoValue) {
    int32_t iRads;
    char acText[264];
    float fEffectivenessMult;
    float fResistance = pPlayer->GetRadiationResistanceMult();
    fEffectivenessMult = 1.0f;
    BGSEntryPoint::HandleEntryPoint(BGSEntryPoint::ADJUST_DRINKING_RADIATION, pPlayer, &fEffectivenessMult);
    fResistance = fResistance * fEffectivenessMult;
    float fModifiedMagnitude = static_cast<double>(aiRadiationAmount) * fResistance;
    if (fModifiedMagnitude > 0.0 && fModifiedMagnitude < 1.0) {
        iRads = 1;
    }
    else {
        iRads = static_cast<int32_t>(fModifiedMagnitude);
    }
    std::sprintf(acText, "%i", iRads);
    HUDMainMenu::pme->pTiles[aiInfoLabel]->Set(4036, acText, true);
    const char* pText = sHUDRads.String();
    HUDMainMenu::pme->pTiles[aiInfoValue]->Set(4036, pText, true);
}

void HUDMainMenu::SetBrightness(Tile* apTile, HUD_BRIGHTNESS aeBrightness) {
    switch (aeBrightness) {
        case HB_HUD_BRACKETS:
        case HB_HUD_METERS: apTile->Set(Tile::kTileValue_Brightness, 0xAF); break;
        case HB_HUD_BRACKETS_GLOW: apTile->Set(Tile::kTileValue_Brightness, 0xFF); break;
        case HB_HUD_METER_BACKGROUND:
            // HB_HUD_METER_BACKGROUND falls through into HB_HUD_METERS_GHOST's store.
            apTile->Set(Tile::kTileValue_Brightness, 0x2C);
            [[fallthrough]];
        case HB_HUD_METERS_GHOST: apTile->Set(Tile::kTileValue_Brightness, 0x64); break;
        default:
            // 0xFF is 0xAF + 0x50 computed from a constant true flag.
            apTile->Set(Tile::kTileValue_Brightness, 0xFF);
            break;
    }
}

void HUDMainMenu::SetRadiationLevel(float afDrinkinRads, float afStandinRads) {
    float fTotalRads = afDrinkinRads + afStandinRads;
    if (HUDMainMenu::pme) {
        if (fTotalRads > 0.0) {
            if (!HUDMainMenu::pme->GeigerSound.IsValid() && afStandinRads > 0.0) {
                HUDMainMenu::pme->GeigerSound = BSAudio::QInstance().GetSoundHandleByName(
                    "UIPipBoyRadiationTestLP",
                    17u
                );
                HUDMainMenu::pme->GeigerSound.Play(false);
            }
            HUDMainMenu::pme->UpdateRadiationMeterConditions(afDrinkinRads, afStandinRads);
        }
        else if (HUDMainMenu::pme->GeigerSound.IsValid()) {
            HUDMainMenu::pme->GeigerSound.FadeOutAndRelease(500u);
        }
    }
}

// Every computed position is rounded to a float before it is set; the sums of several traits are kept
// as doubles in between.
Tile* HUDMainMenu::Create() {
    using enum Tile::eTileValue;

    if (!InterfaceManager::Instance()) {
        return nullptr;
    }
    TileMenu* pOldMenu = static_cast<TileMenu*>(Menu::GetMenuByClass(GetMenuID()));
    if (pOldMenu) {
        delete pOldMenu;
    }
    InterfaceManager* pInterfaceManager = InterfaceManager::Instance();
    const float fMaxDepth = Tile::GetMaximumDepth();
    Tile::GetTileUpdateAllCriticalSectionLock()->Enter(nullptr);
    Tile* pFile = pInterfaceManager->GetMenusRoot()->ReadFile("Data\\Menus\\Main\\hud_main_menu.xml");
    Menu* pMenu = pFile->GetMenu();
    if (!pMenu || pMenu->GetClass() != GetMenuID()) {
        if (pMenu && pMenu->GetMenuTile()) {
            delete pMenu;
        }
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
    fValue = static_cast<float>(
        static_cast<double>(iScreenWidth - iSafeX * 2) - pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Width) + 30.0
    );
    pme->pTiles[HMMT_AP]->Set(kTileValue_X, fValue, true);
    fValue = static_cast<float>(
        static_cast<double>(iScreenHeight - iSafeY * 2) - pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Height)
    );
    pme->pTiles[HMMT_AP]->Set(kTileValue_Y, fValue, true);
    pme->pTiles[HMMT_AP]->Set(kTileValue_Locus, 1);
    pme->pTiles[HMMT_RIGHT_BRACKET] = pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_right_bracket", nullptr);
    SetBrightness(pme->pTiles[HMMT_RIGHT_BRACKET], HB_HUD_BRACKETS_GLOW);
    pme->pTiles[HMMT_RIGHT_BRACKET]->Set(kTileValue_X, -37);
    pme->pTiles[HMMT_ACTION_POINTS_BAR] = pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_meter", nullptr);
    pme->pTiles[HMMT_ACTION_POINTS_BAR]->Set(iOriginalX, 326);
    pme->pTiles[HMMT_ACTION_POINTS_BAR]->Set(kTileValue_Y, 41.0f, true);
    SetBrightness(pme->pTiles[HMMT_ACTION_POINTS_BAR], HB_HUD_METERS);
    pme->pTiles[HMMT_ACTION_POINTS_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_AP],
        "template_justify_right_text",
        nullptr
    );
    pme->pTiles[HMMT_ACTION_POINTS_TEXT]->Set(kTileValue_X, 315);
    pme->pTiles[HMMT_ACTION_POINTS_TEXT]->Set(kTileValue_Y, 8);
    SetBrightness(pme->pTiles[HMMT_ACTION_POINTS_TEXT], HB_HUD_TEXT);
    pme->pTiles[HMMT_AMMO_TEXT] = pMenu->RenderTemplate(pme->pTiles[HMMT_AP], "template_justify_right_text", nullptr);
    pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_X, 315);
    pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_Y, 80);
    pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_Font, 7);
    SetBrightness(pme->pTiles[HMMT_AMMO_TEXT], HB_HUD_TEXT);
    pme->pTiles[HMMT_WEAPON_CONDITION_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_AP],
        "template_justify_right_text",
        nullptr
    );
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
    pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_AP],
        "template_meter_background",
        nullptr
    );
    fValue = static_cast<float>(pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_Y) + 3.0);
    pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND]->Set(kTileValue_Y, fValue, true);
    SetBrightness(pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND], HB_HUD_METER_BACKGROUND);
    pme->pTiles[HMMT_CND_ARROWS] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "CNDArrows");
    dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_X);
    dSum = pme->pTiles[HMMT_WEAPON_CONDITION_TEXT]->GetFloat(kTileValue_X) + dSum;
    dSum = pme->pTiles[HMMT_WEAPON_CONDITION_METER]->GetFloat(kTileValue_X) + dSum;
    fValue = static_cast<float>(
        pme->pTiles[HMMT_WEAPON_CONDITION_METER_BACKGROUND]->GetFloat(kTileValue_Width) * 0.75 + dSum + 8.0
    );
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
    fValue = static_cast<float>(
        static_cast<double>(iScreenHeight - iSafeY * 2) - pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Height)
    );
    pme->pTiles[HMMT_HP]->Set(kTileValue_Y, fValue, true);
    pme->pTiles[HMMT_HP]->Set(kTileValue_Locus, 1);
    pme->pTiles[HMMT_LEFT_BRACKET] = pMenu->RenderTemplate(pme->pTiles[HMMT_HP], "template_left_bracket", nullptr);
    SetBrightness(pme->pTiles[HMMT_LEFT_BRACKET], HB_HUD_BRACKETS_GLOW);
    pme->pTiles[HMMT_HEALTH_BAR] = pMenu->RenderTemplate(pme->pTiles[HMMT_HP], "template_meter", nullptr);
    pme->pTiles[HMMT_HEALTH_BAR]->Set(iOriginalX, 20);
    pme->pTiles[HMMT_HEALTH_BAR]->Set(kTileValue_Y, 41.0f, true);
    SetBrightness(pme->pTiles[HMMT_HEALTH_BAR], HB_HUD_METERS);
    pme->pTiles[HMMT_HIT_POINT_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_HP],
        "template_justify_right_text",
        nullptr
    );
    pme->pTiles[HMMT_HIT_POINT_TEXT]->Set(kTileValue_X, 53);
    pme->pTiles[HMMT_HIT_POINT_TEXT]->Set(kTileValue_Y, 8);
    SetBrightness(pme->pTiles[HMMT_HIT_POINT_TEXT], HB_HUD_TEXT);
    pme->pTiles[HMMT_COMPASS] = pMenu->RenderTemplate(pme->pTiles[HMMT_HP], "template_compass_window", nullptr);
    pme->pTiles[HMMT_COMPASS_GENERAL_ICONS] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_COMPASS],
        "template_compass_icon_group",
        nullptr
    );
    pme->pTiles[HMMT_COMPASS_GENERAL_ICONS]->Set(kTileValue_Y, 30);
    pme->pTiles[HMMT_COMPASS_GENERAL_ICONS]->Set(kTileValue_Depth, 4);
    pme->pTiles[HMMT_COMPASS_MARKER_ICONS] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_COMPASS],
        "template_compass_icon_group",
        nullptr
    );
    pme->pTiles[HMMT_COMPASS_MARKER_ICONS]->Set(kTileValue_Y, 30);
    pme->pTiles[HMMT_COMPASS_MARKER_ICONS]->Set(kTileValue_Depth, 3);
    pme->pTiles[HMMT_COMPASS_NPC_ICONS] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_COMPASS],
        "template_compass_icon_group",
        nullptr
    );
    pme->pTiles[HMMT_COMPASS_NPC_ICONS]->Set(kTileValue_Y, 10);
    pme->pTiles[HMMT_COMPASS_NPC_ICONS]->Set(kTileValue_Depth, 2);
    for (int32_t i = 0; i < static_cast<int32_t>(iHUDMaxCompassNPCTicks.UInt()); ++i) {
        pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_NPC_ICONS], "compass_npc_icon", nullptr);
    }
    for (int32_t i = 0; i < 10; ++i) {
        pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_MARKER_ICONS], "template_compass_icon_quest", nullptr);
    }
    pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_MARKER_ICONS], "template_compass_icon_player", nullptr);
    for (int32_t i = 0; i < 10; ++i) {
        pme->RenderTemplate(pme->pTiles[HMMT_COMPASS_GENERAL_ICONS], "template_compass_icon_marker", nullptr);
    }

    // The radiation meter, at the top right.
    pme->pTiles[HMMT_RADIATION] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "RadiationMeter");
    fValue = static_cast<float>(
        static_cast<double>(iScreenWidth - iSafeX * 2) - pme->pTiles[HMMT_RADIATION]->GetFloat(kTileValue_Width) - 20.0
    );
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
    fValue = static_cast<float>(
        static_cast<double>(iScreenWidth / 2) - pme->pTiles[HMMT_ENEMY_HEALTH]->GetFloat(kTileValue_Width) / 2.0
    );
    pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_X, fValue, true);
    fValue = static_cast<float>(pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Y) + 40.0);
    pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_Y, fValue, true);
    pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_Locus, 1);
    pme->pTiles[HMMT_ENEMY_HEALTH]->Set(kTileValue_SystemColor, 2);
    pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_ENEMY_HEALTH],
        "template_enemy_health_bracket",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET], HB_HUD_BRACKETS);
    pme->pTiles[HMMT_ENEMY_HEALTH_BRACKET]->Set(kTileValue_Visible, 1);
    pme->pTiles[HMMT_ENEMY_HEALTH_METER] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_ENEMY_HEALTH],
        "template_meter",
        nullptr
    );
    pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(iOriginalX, 171);
    pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(iTotalWidth, 240);
    pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(kTileValue_Y, 3);
    SetBrightness(pme->pTiles[HMMT_ENEMY_HEALTH_METER], HB_HUD_METERS);
    pme->pTiles[HMMT_ENEMY_HEALTH_METER]->Set(kTileValue_Visible, 1);
    pme->pTiles[HMMT_ENEMY_HEALTH_NAME] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_ENEMY_HEALTH],
        "template_justify_center_text",
        nullptr
    );
    pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(kTileValue_X, 170);
    pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(kTileValue_Y, 35);
    pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(kTileValue_WrapWidth, 350);
    SetBrightness(pme->pTiles[HMMT_ENEMY_HEALTH_NAME], HB_HUD_TEXT);

    // The quest reminder, at the top left.
    const int32_t iQuestReminderY = iSafeY * 2 + 170;
    pme->pTiles[HMMT_QUEST_UPDATE] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "QuestReminder"
    );
    pme->pTiles[HMMT_QUEST_UPDATE]->Set(kTileValue_X, iSafeX * 2 + 15);
    pme->pTiles[HMMT_QUEST_UPDATE]->Set(kTileValue_Y, iQuestReminderY);
    pme->pTiles[HMMT_QUEST_STAGES] = Tile::GetTileByName(pme->pTiles[HMMT_QUEST_UPDATE], "QuestStages");
    pme->pTiles[HMMT_QUEST_ADDED] = Tile::GetTileByName(pme->pTiles[HMMT_QUEST_UPDATE], "QuestAdded");
    for (int32_t i = 0; i < 64; ++i) {
        pme->RenderTemplate(pme->pTiles[HMMT_QUEST_ADDED], "template_justify_left_text", nullptr);
    }
    for (int32_t i = 0; i < 32; ++i) {
        pme->RenderTemplate(pme->pTiles[HMMT_QUEST_STAGES], "template_filled_checkbox", nullptr);
    }
    for (int32_t i = 0; i < 32; ++i) {
        pme->RenderTemplate(pme->pTiles[HMMT_QUEST_STAGES], "template_justify_left_text", nullptr);
    }

    // The region and location names, above the hit points.
    pme->pTiles[HMMT_REGION_LOCATION] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "Region_Location"
    );
    pme->pTiles[HMMT_REGION_LOCATION]->Set(kTileValue_X, iSafeX * 2 + 15);
    fValue = static_cast<float>(pme->pTiles[HMMT_HP]->GetFloat(kTileValue_Y) - 75.0);
    pme->pTiles[HMMT_REGION_LOCATION]->Set(kTileValue_Y, fValue, true);
    pme->pTiles[HMMT_REGION_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_REGION_LOCATION],
        "template_justify_left_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_REGION_TEXT], HB_HUD_TEXT);
    pme->pTiles[HMMT_REGION_TEXT]->Set(kTileValue_Visible, 1);
    NiPoint3 kTextSize;
    float fWrapWidth = pme->pTiles[HMMT_REGION_TEXT]->GetFloat(kTileValue_WrapWidth);
    int32_t iFont = static_cast<int32_t>(pme->pTiles[HMMT_REGION_TEXT]->GetFloat(kTileValue_Font));
    kTextSize = FontManager::Instance()->CalculateStringDimensions("Test Height", iFont, fWrapWidth, 0);
    pme->pTiles[HMMT_REGION_TEXT]->Set(kTileValue_Y, kTextSize.y, true);
    pme->pTiles[HMMT_LOCATION_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_REGION_LOCATION],
        "template_justify_left_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_LOCATION_TEXT], HB_HUD_TEXT);
    pme->pTiles[HMMT_LOCATION_TEXT]->Set(kTileValue_Visible, 1);
    pme->pTiles[HMMT_LOCATION_TEXT]->Set(kTileValue_Y, kTextSize.y, true);

    // The crosshair, centred on the screen.
    pme->pTiles[HMMT_CROSSHAIR] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "ReticleCenter");
    fValue = static_cast<float>(
        static_cast<double>(iScreenWidth / 2) - pme->pTiles[HMMT_CROSSHAIR]->GetFloat(kTileValue_Width) / 2.0 - 3.0
    );
    pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_X, fValue, true);
    fValue = static_cast<float>(
        static_cast<double>(iScreenHeight / 2) - pme->pTiles[HMMT_CROSSHAIR]->GetFloat(kTileValue_Height) / 2.0 - 3.0
    );
    pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_Y, fValue, true);
    pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_Locus, 1);
    pme->pTiles[HMMT_CROSSHAIR]->Set(kTileValue_Visible, 1);
    pme->pTiles[HMMT_CROSSHAIR_CENTER] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_CROSSHAIR],
        "template_reticle_center",
        nullptr
    );
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
    pme->pTiles[HMMT_MESSAGE_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_MESSAGE],
        "template_justify_left_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_MESSAGE_TEXT], HB_HUD_TEXT);
    pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_X, 130);
    pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Y, 5);
    pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_WrapWidth, 320);
    pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Depth, 2);
    pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Alpha, 0);
    pme->pTiles[HMMT_MESSAGE_TEXT]->Set(kTileValue_Visible, 1);
    pme->pTiles[HMMT_MESSAGE_BRACKET] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_MESSAGE],
        "template_message_bracket",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_MESSAGE_BRACKET], HB_HUD_BRACKETS);

    // The crosshair info (action, name, door and container state, two label and value pairs), above the bottom.
    pme->pTiles[HMMT_INFO] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Info");
    fValue = static_cast<float>(
        static_cast<double>(iScreenWidth / 2) - pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0
    );
    pme->pTiles[HMMT_INFO]->Set(kTileValue_X, fValue, true);
    fValue = static_cast<float>(
        static_cast<double>(iScreenHeight - iSafeY * 2) - pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Height) - 30.0
    );
    pme->pTiles[HMMT_INFO]->Set(kTileValue_Y, fValue, true);
    pme->pTiles[HMMT_INFO]->Set(kTileValue_Locus, 1);
    pme->pTiles[HMMT_INFO_ACTION_HINT] = Tile::GetTileByName(pme->pTiles[HMMT_INFO], "justify_center_hotrect");
    pme->pTiles[HMMT_INFO_ACTION_PC_HINT] = Tile::GetTileByName(pme->pTiles[HMMT_INFO_ACTION_HINT], "PCShortcutLabel");
    pme->pTiles[HMMT_INFO_ACTION_XBOX_HINT] = Tile::GetTileByName(pme->pTiles[HMMT_INFO_ACTION_HINT], "xbox_button");
    SetBrightness(pme->pTiles[HMMT_INFO_ACTION_HINT], HB_HUD_TEXT);
    fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
    const int32_t iXTrait = Tile::TextToTrait("_x");
    pme->pTiles[HMMT_INFO_ACTION_HINT]->Set(iXTrait, fValue, true);
    pme->pTiles[HMMT_INFO_ACTION_NAME] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_INFO],
        "template_justify_center_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_INFO_ACTION_NAME], HB_HUD_TEXT);
    fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
    pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_X, fValue, true);
    fValue = pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width);
    pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_WrapWidth, fValue, true);
    pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_Y, 0);
    fWrapWidth = pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(kTileValue_WrapWidth);
    iFont = static_cast<int32_t>(pme->pTiles[HMMT_INFO_ACTION_NAME]->GetFloat(kTileValue_Font));
    kTextSize = FontManager::Instance()->CalculateStringDimensions("Test Height", iFont, fWrapWidth, 0);
    pme->pTiles[HMMT_INFO_ACTION_NAME]->Set(kTileValue_User8, kTextSize.y, true);
    pme->pTiles[HMMT_INFO_DOOR_STATE] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_INFO],
        "template_justify_center_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_INFO_DOOR_STATE], HB_HUD_TEXT);
    fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0);
    pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_X, fValue, true);
    fValue = pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width);
    pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_WrapWidth, fValue, true);
    pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_Y, 65);
    pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_Alpha, 0);
    pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_String, sLocked.String(), true);
    pme->pTiles[HMMT_INFO_DOOR_STATE]->Set(kTileValue_Visible, 0);
    pme->pTiles[HMMT_INFO_CONTAINER_STATE] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_INFO],
        "template_justify_center_text",
        nullptr
    );
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
    pme->pTiles[HMMT_INFO_LABEL_ONE] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_INFO],
        "template_justify_right_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_INFO_LABEL_ONE], HB_HUD_TEXT);
    fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0 - iLabelOneOffset);
    pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(kTileValue_X, fValue, true);
    pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(kTileValue_Y, iInfoLineY);
    pme->pTiles[HMMT_INFO_LABEL_ONE]->Set(kTileValue_Alpha, fCurrentHUDOpacity, true);
    pme->pTiles[HMMT_INFO_VALUE_ONE] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_INFO],
        "template_justify_left_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_INFO_VALUE_ONE], HB_HUD_TEXT);
    pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(kTileValue_X, iValueOneX);
    pme->pTiles[HMMT_INFO_VALUE_ONE]->Set(kTileValue_Y, iInfoLineY);
    pme->pTiles[HMMT_INFO_LABEL_TWO] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_INFO],
        "template_justify_right_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_INFO_LABEL_TWO], HB_HUD_TEXT);
    fValue = (pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) - iLabelTwoOffset);
    pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(kTileValue_X, fValue, true);
    pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(kTileValue_Y, iInfoLineY);
    pme->pTiles[HMMT_INFO_LABEL_TWO]->Set(kTileValue_Alpha, fCurrentHUDOpacity, true);
    pme->pTiles[HMMT_INFO_VALUE_TWO] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_INFO],
        "template_justify_left_text",
        nullptr
    );
    SetBrightness(pme->pTiles[HMMT_INFO_VALUE_TWO], HB_HUD_TEXT);
    fValue = static_cast<float>(pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Width) / 2.0 + iValueTwoOffset);
    pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(kTileValue_X, fValue, true);
    pme->pTiles[HMMT_INFO_VALUE_TWO]->Set(kTileValue_Y, iInfoLineY);
    pme->pTiles[HMMT_INFO_SEPARATOR] = pMenu
                                           ->RenderTemplate(pme->pTiles[HMMT_INFO], "template_info_seperator", nullptr);
    SetBrightness(pme->pTiles[HMMT_INFO_SEPARATOR], HB_HUD_BRACKETS_GLOW);

    // The subtitles, above the crosshair info.
    pme->pTiles[HMMT_SUBTITLE] = Tile::GetTileByName(Menu::GetMenuByClass(Interface::eHUDMainMenu), "Subtitles");
    fValue = static_cast<float>(
        static_cast<double>(iScreenWidth / 2) - pme->pTiles[HMMT_SUBTITLE]->GetFloat(kTileValue_Width) / 2.0
    );
    pme->pTiles[HMMT_SUBTITLE]->Set(kTileValue_X, fValue, true);
    dSum = pme->pTiles[HMMT_INFO]->GetFloat(kTileValue_Y);
    fValue = static_cast<float>(dSum - pme->pTiles[HMMT_SUBTITLE]->GetFloat(kTileValue_Height) - 50.0);
    pme->pTiles[HMMT_SUBTITLE]->Set(kTileValue_Y, fValue, true);
    pme->pTiles[HMMT_SUBTITLE]->Set(kTileValue_Locus, 1);
    pme->pTiles[HMMT_SUBTITLE_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_SUBTITLE],
        "template_justify_center_text",
        nullptr
    );
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
    pme->pTiles[HMMT_HOTKEY_WHEEL] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_HOTKEY],
        "template_hotkey_selector",
        nullptr
    );
    pme->HKWheel.Initialize(static_cast<TileRect*>(pme->pTiles[HMMT_HOTKEY_WHEEL]));
    pme->pTiles[HMMT_HOTKEY_TEXT] = pMenu->RenderTemplate(
        pme->pTiles[HMMT_HOTKEY],
        "template_justify_center_text",
        nullptr
    );
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
    pme->pTiles[HMMT_EXPLOSIVE_INDICATOR] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "Explosive_positioning_rect"
    );
    pme->pTiles[HMMT_EXPLOSIVE_INDICATOR]->Set(kTileValue_X, iScreenWidth / 2);
    pme->pTiles[HMMT_EXPLOSIVE_INDICATOR]->Set(kTileValue_Y, iScreenHeight / 2);
    for (int32_t i = 0; i < 6; ++i) {
        pme->RenderTemplate(pme->pTiles[HMMT_EXPLOSIVE_INDICATOR], "template_explosive_indicator", nullptr);
    }

    // The crippled limb figure, at the top right; every part is sized and placed in percent of fScale.
    pme->pTiles[HMMT_CRIPPLED_GUY] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "crippled_limb_indicator"
    );
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
    fValue = static_cast<float>(
        (dSum2 - pme->pTiles[HMMT_CRIPPLED_FACE]->GetFloat(kTileValue_Height)) / 2.0 + dSum + fScale * 16.0 / 100.0
    );
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
    pme->pTiles[HMMT_DDT_ICON_ENEMY] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "DDTIconEnemy"
    );
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
    pme->pTiles[HMMT_DDT_ICON_ENEMY_AP] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "DDTIconEnemyAP"
    );
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
    pme->pTiles[HMMT_AMMO_SUB_TYPE] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "AmmoTypeLabel"
    );
    dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_X);
    fValue = static_cast<float>(pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_X) + dSum - 165.0);
    pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(kTileValue_X, fValue, true);
    dSum = pme->pTiles[HMMT_AP]->GetFloat(kTileValue_Y);
    fValue = static_cast<float>(pme->pTiles[HMMT_AMMO_TEXT]->GetFloat(kTileValue_Y) + dSum);
    pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(kTileValue_Y, fValue, true);
    SetBrightness(pme->pTiles[HMMT_AMMO_SUB_TYPE], HB_HUD_TEXT);
    pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(kTileValue_Visible, 1);

    // The hardcore needs (dehydration, hunger, sleep, radiation, crippled limbs), in a row beside the hit points.
    pme->pTiles[HMMT_HARDCORE_MODE] = Tile::GetTileByName(
        Menu::GetMenuByClass(Interface::eHUDMainMenu),
        "HardcoreMode"
    );
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
        InitializationFlow(
            "Hud Menu Creation Failed... This likely means that the archives were not built and the "
            "xml doesn't match the code."
        );
        Interface::EmergencyCloseAllMenusAndBreakStuff();
    }
    // Stacking types 6006 and 102 put the HUD at the depth that was the deepest before it was read.
    if (pFile->GetFloat(kTileValue_StackingType) == 6006.0 || pFile->GetFloat(kTileValue_StackingType) == 102.0) {
        pFile->Set(kTileValue_Depth, fMaxDepth, true);
    }
    pme->pTiles[HMMT_AMMO_TEXT]->Set(kTileValue_String, sInventoryAmmo.String(), true);
    pme->pTiles[HMMT_ACTION_POINTS_TEXT]->Set(kTileValue_String, sActionPointsShort.String(), true);
    pme->pTiles[HMMT_HIT_POINT_TEXT]->Set(kTileValue_String, sHitPointsShort.String(), true);
    pme->iCompassImageWidth = 856;
    pme->iCompassDisplayWidth = static_cast<int32_t>(pme->pTiles[HMMT_COMPASS]->GetFloat(kTileValue_Width) / 2.0) - 35;
    pme->iCompassHalfAngle = 70;
    pFile->Set(kTileValue_Visible, 1);
    // The menu tile is fetched here and not used.
    pme->GetMenuTile();
    Tile::UpdateAll(false);

    // The compass's alpha map, which the compass's shader rotates.
    NiPointer<NiSourceTexture> spAlphaMap(nullptr);
    {
        NiFixedString kFilename("Textures\\Interface\\HUD\\hud_compass_alphamap.dds");
        spAlphaMap = NiSourceTexture::Create(kFilename, NiSourceTexture::ms_kDefaultPrefs, true, false);
    }
    NiSourceTexture* pAlphaMap = spAlphaMap;
    pme->pTiles[HMMT_COMPASS]->GetShaderProperty()->SetAlphaMap(pAlphaMap);
    pme->pTiles[HMMT_COMPASS]->GetShaderProperty()->SetUseTexScroll(true);

    // The gun scope model: an empty node turned to face the scope camera, lit by the 3D menu scene's lights.
    spScopeModel = new NiNode(0);
    NiMatrix3 kXRotation;
    NiMatrix3 kZRotation;
    kXRotation.MakeXRotation(1.5707964f);
    kZRotation.MakeZRotation(1.5707964f);
    // The product is built before the node is fetched.
    const NiMatrix3 kRotation = kXRotation * kZRotation;
    spScopeModel->SetRotate(kRotation);
    spScopeModel->SetScale(0.1f);
    spScopeModel->SetTranslate(380.0f, 0.0f, 0.0f);
    NiUpdateData kScopeUpdate(0.0f, false, false);
    spScopeModel->Update(kScopeUpdate);
    spScopeCamera = pme->Create3DCamera();
    pQuestUpdateManager = static_cast<QuestUpdateManager*>(ourMalloc(sizeof(QuestUpdateManager)));
    ShadowSceneNode* pMenuScene = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
    pMenuScene->AttachChild(spScopeModel, true);
    const float fLightRadius = static_cast<float>(spScopeModel->GetWorldBound().m_fRadius * 3.0);
    NiNode* pScopeModel = spScopeModel;
    BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)
        ->AddAllLightsFromNode_Recurse(pScopeModel, fLightRadius);

    // The breath meter model stands upright and plays its "Forward" animation from the start.
    NiNode* pBreathModel = pme->pTiles[HMMT_BREATH]->GetModel();
    NiControllerManager* pBreathController = static_cast<NiControllerManager*>(
        pBreathModel->GetAt(0)->GetController(&NiControllerManager::ms_RTTI)
    );
    pBreathModel->SetRotate(-1.5707964f, 1.0f, 0.0f, 0.0f);
    NiUpdateData kBreathUpdate(0.0f, true, false);
    pBreathModel->GetAt(0)->Update(kBreathUpdate);
    if (pBreathController) {
        {
            NiFixedString kForward("Forward");
            pme->pBreathMeterAnim = pBreathController->GetSequenceByName(kForward);
        }
        pBreathController->DeactivateAll(0.0f);
        if (pme->pBreathMeterAnim) {
            pBreathController->ActivateSequence(pme->pBreathMeterAnim, 0, true, 1.0f, 0.0f, nullptr);
        }
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
    if (!HUDMainMenu::pme) {
        return;
    }

    HUD_MENU_MODE eMode = aeMenuMode;
    if (HUDMainMenu::pme->GetFadeState() != 1) {
        HUDMainMenu::pme->StartFadeIn();
    }

    if (eMode == HMM_CHANGED) {
        switch (Interface::GetTopMenuID()) {
            case Interface::eNoMenu:
            case Interface::eHUDMainMenu:
                if (pPlayer->GetSitSleepState() == SIT_SLEEP_IS_SITTING ||
                    pPlayer->GetSitSleepState() == SIT_SLEEP_IS_SLEEPING) {
                    eMode = HMM_SIT_SLEEP;
                }
                else if (pPlayer->GetSitSleepState() != SIT_SLEEP_NORMAL) {
                    eMode = HMM_SIT_SLEEP_ANIM;
                }
                else if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
                    eMode = HMM_VATS_PLAYBACK;
                }
                else if (pPlayer->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
                    // Switches the HUD through a nested call and then goes on with the mode still HMM_CHANGED.
                    HUDMainMenu::SetMenuMode(HMM_MOVEMENT_DISABLED);
                }
                else if (pPlayer->GetIronSights()) {
                    eMode = HMM_IRON_SIGHTS;
                }
                else {
                    eMode = HMM_RUN_AND_GUN;
                }
                break;
            case Interface::eInventoryMenu:
            case Interface::eStatsMenu:
            case Interface::eLockPickMenu:
            case Interface::ePipBoyData:
            case Interface::ePipBoyRepair:
            case Interface::eItemModMenu: eMode = HMM_PIPBOY; break;
            case Interface::eStartMenu: eMode = HMM_START; break;
            case Interface::eLoadingMenu: eMode = HMM_LOAD; break;
            case Interface::eDialogMenu: eMode = HMM_DIALOGUE; break;
            case Interface::eVATSMenu: eMode = HMM_VATS; break;
            case Interface::eContainerMenu: eMode = HMM_CONTAINER; break;
            case Interface::eSleepWaitMenu: eMode = HMM_SIT_SLEEP; break;
            case Interface::eHackingMenu:
                // Stores HMM_HACKING and deliberately falls through into the computers case's store.
                eMode = HMM_HACKING;
                [[fallthrough]];
            case Interface::eComputersMenu: eMode = HMM_TERMINAL; break;
            default: break;
        }
    }

    if (eMode == HMM_IRON_SIGHTS &&
        HUDMainMenu::spScopeModel &&
        HUDMainMenu::spScopeModel->GetAtChecked(0) &&
        pPlayer->IsWeaponDrawn()) {
        eMode = HMM_SNIPER_RIFLE;
    }

    if (eMode != HMM_MOVIE &&
        eMode != HMM_START &&
        eMode != HMM_LOAD &&
        pPlayer->GetControlsDisabled(PlayerCharacter::DPC_MOVEMENT)) {
        if (InterfaceManager::Instance()->QLockpickControlsDisable()) {
            eMode = HMM_RUN_AND_GUN;
        }
        else if (eMode == HMM_DIALOGUE) {
            eMode = HMM_DIALOGUE;
        }
        else {
            eMode = HMM_MOVEMENT_DISABLED;
        }
    }

    switch (eMode) {
        case HMM_RUN_AND_GUN:
        case HMM_PIPBOY_DISABLED:
        case HMM_VANITY:
        case HMM_IRON_SIGHTS:
            if (pPlayer->GetControlsDisabled(PlayerCharacter::DPC_ROLL_OVER)) {
                HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_AP | HV_HP | HV_RADIATION | HV_ENEMY_HEALTH |
                    HV_QUEST_UPDATE | HV_REGION_LOCATION | HV_CROSSHAIR | HV_SNEAK | HV_MESSAGE | HV_SUBTITLE |
                    HV_HOTKEY | HV_XP | HV_BREATH | HV_EXPLOSIVE_INDICATOR | HV_CRIPPLED_GUY | HV_HARDCORE_MODE;
            }
            else {
                HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_AP | HV_HP | HV_RADIATION | HV_ENEMY_HEALTH |
                    HV_QUEST_UPDATE | HV_REGION_LOCATION | HV_CROSSHAIR | HV_SNEAK | HV_MESSAGE | HV_INFO |
                    HV_SUBTITLE | HV_HOTKEY | HV_XP | HV_BREATH | HV_EXPLOSIVE_INDICATOR | HV_CRIPPLED_GUY |
                    HV_HARDCORE_MODE;
            }
            HUDMainMenu::RefreshInfoForRef();
            break;
        case HMM_PIPBOY:
            HUDMainMenu::pme->HideSubtitle();
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_MESSAGE;
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
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_QUEST_UPDATE | HV_MESSAGE | HV_XP;
            break;
        case HMM_VATS:
            HUDMainMenu::pme->HideSubtitle();
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_MESSAGE;
            break;
        case HMM_VATS_PLAYBACK:
            HUDMainMenu::pme->HideSubtitle();
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_ENEMY_HEALTH | HV_MESSAGE;
            break;
        case HMM_CONTAINER:
            HUDMainMenu::pme->HideSubtitle();
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_XP;
            break;
        case HMM_SIT_SLEEP_ANIM:
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_AP | HV_HP | HV_RADIATION | HV_ENEMY_HEALTH |
                HV_QUEST_UPDATE | HV_REGION_LOCATION | HV_SNEAK | HV_MESSAGE | HV_SUBTITLE | HV_HOTKEY | HV_XP |
                HV_HARDCORE_MODE;
            break;
        case HMM_SIT_SLEEP:
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_AP | HV_HP | HV_RADIATION | HV_ENEMY_HEALTH |
                HV_QUEST_UPDATE | HV_REGION_LOCATION | HV_CROSSHAIR | HV_SNEAK | HV_MESSAGE | HV_INFO | HV_SUBTITLE |
                HV_HOTKEY | HV_XP | HV_HARDCORE_MODE;
            break;
        case HMM_MOVEMENT_DISABLED:
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_RADIATION | HV_QUEST_UPDATE | HV_MESSAGE | HV_SUBTITLE |
                HV_XP;
            break;
        case HMM_HACKING:
        case HMM_TERMINAL:
            HUDMainMenu::pme->HideSubtitle();
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_QUEST_UPDATE | HV_XP;
            break;
        case HMM_SNIPER_RIFLE:
            HUDMainMenu::pme->HideSubtitle();
            HUDMainMenu::pme->iHUDVisibilityMenuModeFlags = HV_ENEMY_HEALTH;
            break;
        default: break;
    }

    HUDMainMenu::pme->iHUDVisibilityMenuModeFlags &= HUDMainMenu::pme->iHUDVisibilityUserSettings;
    if (bHideEnemyHealthMeter) {
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags &= ~HV_ENEMY_HEALTH;
    }
    if (pPlayer && pPlayer->GetWeaponSightingNode() && !pPlayer->IsActually3rdPerson()) {
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags &= ~HV_CROSSHAIR;
    }

    // In dialogue the XP meter moves 330 up from its resting height (kTileValue_User3).
    if (eMode == HMM_DIALOGUE) {
        const float fRestY = HUDMainMenu::pme->pTiles[HMMT_XP]->GetFloat(Tile::kTileValue_User3);
        const float fY = fRestY - 330.0;
        HUDMainMenu::pme->pTiles[HMMT_XP]->Set(Tile::kTileValue_Y, fY, true);
    }
    else {
        const float fRestY = HUDMainMenu::pme->pTiles[HMMT_XP]->GetFloat(Tile::kTileValue_User3);
        HUDMainMenu::pme->pTiles[HMMT_XP]->Set(Tile::kTileValue_Y, fRestY, true);
    }

    if (HUDMainMenu::pme->eMenuMode != HMM_SNIPER_RIFLE && eMode == HMM_SNIPER_RIFLE) {
        HUDMainMenu::SetGunScopeVisible(true);
    }
    else if (HUDMainMenu::pme->eMenuMode == HMM_SNIPER_RIFLE && eMode != HMM_SNIPER_RIFLE) {
        HUDMainMenu::SetGunScopeVisible(false);
    }
    HUDMainMenu::pme->eMenuMode = eMode;

    const uint32_t eNewMode = HUDMainMenu::pme->eMenuMode;
    if (eNewMode != HMM_VANITY &&
        eNewMode != HMM_DIALOGUE &&
        eNewMode != HMM_RUN_AND_GUN &&
        eNewMode != HMM_LOAD &&
        eNewMode != HMM_CHARGEN_MENU &&
        eNewMode != HMM_MESSAGE_MENU &&
        eNewMode != HMM_CONTAINER &&
        eNewMode != HMM_SIT_SLEEP &&
        eNewMode != HMM_SPECIAL_BOOK &&
        eNewMode != HMM_LOVE_TESTER &&
        eNewMode != HMM_CASINO_GAME) {
        HUDMainMenu::pme->bKillQuestUpdates = true;
    }

    HUDMainMenu::pme->pTiles[HMMT_AP]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_AP
    );
    HUDMainMenu::pme->pTiles[HMMT_HP]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_HP
    );
    HUDMainMenu::pme->pTiles[HMMT_RADIATION]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_RADIATION
    );
    HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_ENEMY_HEALTH
    );
    HUDMainMenu::pme->pTiles[HMMT_QUEST_UPDATE]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_QUEST_UPDATE
    );
    HUDMainMenu::pme->pTiles[HMMT_REGION_LOCATION]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_REGION_LOCATION
    );
    HUDMainMenu::pme->pTiles[HMMT_CROSSHAIR]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_CROSSHAIR
    );
    HUDMainMenu::pme->pTiles[HMMT_SNEAK]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_SNEAK
    );
    HUDMainMenu::pme->pTiles[HMMT_MESSAGE]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_MESSAGE
    );
    HUDMainMenu::pme->pTiles[HMMT_INFO]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_INFO
    );
    HUDMainMenu::pme->pTiles[HMMT_SUBTITLE]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_SUBTITLE
    );
    HUDMainMenu::pme->pTiles[HMMT_HOTKEY]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_HOTKEY
    );
    HUDMainMenu::pme->pTiles[HMMT_XP]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_XP
    );
    HUDMainMenu::pme->pTiles[HMMT_BREATH]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_BREATH
    );
    HUDMainMenu::pme->pTiles[HMMT_EXPLOSIVE_INDICATOR]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_EXPLOSIVE_INDICATOR
    );
    HUDMainMenu::pme->pTiles[HMMT_CRIPPLED_GUY]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_CRIPPLED_GUY
    );
    HUDMainMenu::pme->pTiles[HMMT_AMMO_SUB_TYPE]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_AP
    );
    HUDMainMenu::pme->pTiles[HMMT_HARDCORE_MODE]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_HARDCORE_MODE
    );
    HUDMainMenu::pme->pTiles[HMMT_CND_ARROWS]->Set(
        Tile::kTileValue_Visible,
        HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_AP
    );

    if (!(HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_AP)) {
        HUDMainMenu::pme->pTiles[HMMT_DDT_ICON]->Set(Tile::kTileValue_Visible, 0);
        HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_AP]->Set(Tile::kTileValue_Visible, 0);
    }
    if (!(HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_ENEMY_HEALTH)) {
        HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY]->Set(Tile::kTileValue_Visible, 0);
        HUDMainMenu::pme->pTiles[HMMT_DDT_ICON_ENEMY_AP]->Set(Tile::kTileValue_Visible, 0);
    }

    if (HUDMainMenu::pme->GeigerSound.IsValid()) {
        if (HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_RADIATION) {
            HUDMainMenu::pme->GeigerSound.Play(false);
        }
        else {
            HUDMainMenu::pme->GeigerSound.Pause();
        }
    }
}

void HUDMainMenu::SetGunScopeVisible(bool abVisibleFlag) {
    if (HUDMainMenu::pme && HUDMainMenu::spScopeModel) {
        if (HUDMainMenu::spScopeModel->GetAtChecked(0)) {
            HUDMainMenu::pme->bScopeVisible = abVisibleFlag;
            HUDMainMenu::spScopeModel->NiAVObject::SetAppCulled(!abVisibleFlag);
        }
    }
}

void HUDMainMenu::ResetHUDOpacity(float afHUDOpacity) {
    if (!HUDMainMenu::pme) {
        return;
    }
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
            case HMMT_CRIPPLED_RIGHT_LEG: break;
            default: {
                // The factor is the double 255.0; the product is rounded to a float.
                const float fAlpha = static_cast<float>(afHUDOpacity * 255.0);
                HUDMainMenu::pme->pTiles[i]->Set(Tile::kTileValue_Alpha, fAlpha, true);
                break;
            }
        }
    }
}

// The death test calls the reference's virtual IsDead, not Actor's own body.
void HUDMainMenu::SetEnemyActor(Actor* apActor) {
    if (bHealthBarShowing.Bool()) {
        return;
    }
    if (apActor && apActor->IsDead(false)) {
        return;
    }
    if (!HUDMainMenu::pme) {
        return;
    }
    if (pVATS.GetMode() == VATS::VATS_PLAYBACK) {
        Actor* pTarget;
        if (pVATSTarget && pVATSTarget->IsActor()) {
            pTarget = static_cast<Actor*>(pVATSTarget);
        }
        else {
            pTarget = nullptr;
        }
        HUDMainMenu::pme->pCurrentEnemy = pTarget;
    }
    else {
        HUDMainMenu::pme->pCurrentEnemy = apActor;
    }
    if (HUDMainMenu::pme->pCurrentEnemy) {
        const char* pName = HUDMainMenu::pme->pCurrentEnemy->GetFullName();
        HUDMainMenu::pme->pTiles[HMMT_ENEMY_HEALTH_NAME]->Set(Tile::kTileValue_String, pName, true);
        HUDMainMenu::pme->HudTimer.Update();
        HUDMainMenu::pme->iEnemyHealthTimer = HUDMainMenu::pme->HudTimer.GetTime();
    }
}

void HUDMainMenu::Draw3DElements() {
    if (!HUDMainMenu::spScopeModel) {
        return;
    }
    if (HUDMainMenu::pme && !HUDMainMenu::pme->bScopeVisible) {
        return;
    }
    if (!BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D)->GetChildCount()) {
        ShadowSceneNode* pNode = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
        pNode->AttachChild(HUDMainMenu::spScopeModel, true);
    }
    NiRenderer::GetRenderer()->ClearBuffer(nullptr, NiRenderer::CLEAR_ZBUFFER);
    BSSceneGraph* pWorld = Main::GetWorldSceneGraph();
    BSCullingProcess* pCuller = static_cast<BSCullingProcess*>(pWorld->GetCullingProcess());
    pCuller->PushCullMode(BSCullingProcess::BSCP_CULL_ALLPASS);
    NiCamera* pScopeCamera = HUDMainMenu::spScopeCamera;
    HandleEyeFinityFOVScale(pScopeCamera, fScopeEyeFinityScale);
    {
        alignas(NiPointer<BSShaderAccumulator>) uint8_t aAccumulatorStorage[sizeof(NiPointer<BSShaderAccumulator>)];
        NiPointer<BSShaderAccumulator>& spAccumulator = *::new (aAccumulatorStorage) NiPointer<BSShaderAccumulator>(
            InterfaceManager::Instance()->Get3dAccumulator()
        );
        pCuller->SetAccumulator(spAccumulator);
        NiCamera* pCamera = pScopeCamera ? pScopeCamera : pWorld->GetCamera();
        ShadowSceneNode* pScene = BSShaderManager::GetShadowSceneNode(BSShaderManager::BSSM_SSN_MENU_OBJECT_3D);
        BSShaderUtil::AccumulateScene(pCamera, pScene, *pCuller);
        BSShaderUtil::RenderScene(pCamera, spAccumulator, false);
        pCuller->SetAccumulator(nullptr);
        spAccumulator = nullptr;
        pCuller->PopCullMode();
        spAccumulator.~NiPointer<BSShaderAccumulator>();
    }
}

void HUDMainMenu::SetTargetType(TESObjectREFR* apRef, bool abShow) {
    char cHostile = 0;
    char cDead = 1;
    if (!apRef ||
        !HUDMainMenu::pme ||
        bDisableDynamicCrosshair.Bool() ||
        !abShow ||
        (bTrueIronSights.Bool() && pPlayer->GetIronSights())) {
        if (HUDMainMenu::pme->pTiles[62]->GetFloat(4084) == 1.0) {
            HUDMainMenu::pme->pTiles[21]->Set(4003, 0);
            HUDMainMenu::pme->pTiles[22]->Set(4003, 0);
            HUDMainMenu::pme->pTiles[23]->Set(4003, 0);
        }
    }
    else {
        apRef->GetRefPromptType();
        Actor* pActor = apRef->IsActor() ? static_cast<Actor*>(apRef) : nullptr;
        bool bDead = pActor && pActor->IsDead(false);
        cDead = bDead;
        if (pActor &&
            ((pPlayer->IsActorPerceived(pActor) && pPlayer->IsActorPerceived(pActor)->bIsHostile) ||
             pActor->IsInCombatWithActor(pPlayer))) {
            cHostile = 1;
        }

        if (pActor && !bDead && !pActor->IsDead(false)) {
            int32_t iHealth = pActor->GetActorValue(ActorValue::HEALTH);
            if (iHealth < pActor->GetBaseActorValue(ActorValue::HEALTH) &&
                !HUDMainMenu::pme->pTiles[32]->IsTrue(4003)) {
                HUDMainMenu::SetEnemyActor(pActor);
                HUDMainMenu::pme->pTiles[21]->Set(4003, 1);
                HUDMainMenu::pme->pTiles[22]->Set(4003, 1);
                HUDMainMenu::pme->pTiles[23]->Set(4003, 1);
                HUDMainMenu::pme->pTiles[23]->Set(4003, 1);
                HUDMainMenu::pme->pTiles[62]->Set(4084, (cHostile != 0) + 1);
                HUDMainMenu::pme->pTiles[21]->AddNeedsUpdate(8);
                HUDMainMenu::pme->pTiles[22]->AddNeedsUpdate(8);
                HUDMainMenu::pme->pTiles[23]->AddNeedsUpdate(8);
                HUDMainMenu::pme->pTiles[23]->AddNeedsUpdate(8);
                HUDMainMenu::pme->UpdateEnemyHealth(pActor);
            }
        }
    }
    bHostileUnderCrosshair = !cDead && cHostile;
    bLongDistancePick = abShow;
}

// For each map marker the HUDTravelLocData local passed to SetLocationText has bForceLastLocation written only for a
// marker just discovered or a travel marker. For a non-travel marker outside its radius the record goes with that byte
// unwritten and the callee reads it, so on the first such marker it is uninitialised. The local is
// deliberately declared once, outside the loop. The NPC ticks are dropped beyond the map marker visible distance too,
// and the exterior sneak distance reads fSneakMaxDistance through Setting::Int.
void HUDMainMenu::UpdateCompass(Tile* apCompass, Tile* apGeneralIcons, Tile* apMarkerIcons, Tile* apNPCIcons) {
    static int32_t g_HUDMainMenuTraits_Distance = Tile::AddUserTrait("_Distance", -1);
    static int32_t g_HUDMainMenuTraits_Heading = Tile::AddUserTrait("_Heading", -1);
    static int32_t g_HUDMainMenuTraits_AlphaDown = Tile::AddUserTrait("_AlphaDown", -1);

    // The compass texture scrolls with the player's heading in degrees (57.2957763671875 converts radians to degrees),
    // wrapped into [0, 360) by a truncating division.
    const double fPlayerHeading = pPlayer->GetHeading(false);
    const float fAngle = (pPlayer->GetParentCell()->GetNorthRotation() + fPlayerHeading) * 57.2957763671875;
    const float fWrappedAngle = fAngle - static_cast<double>(360 * static_cast<int32_t>(std::fabs(fAngle) / 360.0));
    const float fPixelHeading = FLerp(0.0f, 1.0f, 0.0f, 360.0f, fWrappedAngle);
    const float fScroll = fPixelHeading - 0.15000000596046448;
    if (apCompass->GetShaderProperty()) {
        double fWrappedScroll;
        if (fScroll < 0.0) {
            fWrappedScroll = fScroll + 1.0;
        }
        else {
            fWrappedScroll = fScroll;
        }
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
    if (pVATS.eMode) {
        fOpacity = 1.0f;
    }
    else {
        fOpacity = fHudOpacity.Float();
    }
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
        if (pInfo->pMarkerRef->GetDisabled()) {
            continue;
        }
        const NiPoint3 kItemLoc = pInfo->pMarkerRef->GetLocationOnReference();
        const float fDistSqr = std::fabs((kItemLoc - kPlayerLoc).SqrLength());
        if (fDistSqr > static_cast<double>(iMapMarkerVisibleDistanceSqr)) {
            continue;
        }

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
                QuestUpdateManager::SetCustomQuestText(
                    sDiscoveredText.String(),
                    pLocationName,
                    QuestUpdateManager::HCQQP_NORMAL,
                    QuestUpdateManager::HCQTJ_LEFT,
                    FONT_NONE,
                    FONT_NONE,
                    "UIPopUpQuestNew"
                );
                TESChallengeManager::UpdateChallenge(
                    CHALLENGE_TYPE_DISCOVER,
                    1,
                    pInfo->pMarkerRef,
                    pInfo->pMarkerRef,
                    0,
                    0,
                    0
                );
                MiscStatManager::Increment(MiscStatManager::MS_LOCATIONS_DISCOVERED);
                pPlayer->RewardExperience(iXPRewardDiscoverMapMarker.Int());
                kTravelLoc.bInRange = false;
                kTravelLoc.bForceLastLocation = true;
            }
            else if (bCanFastTravel) {
                kTravelLoc.bInRange = fRadiusSqr > fDistSqr;
                kTravelLoc.bForceLastLocation = false;
            }
            if (!kTravelLoc.bInRange) {
                HUDMainMenu::SetLocationText(kTravelLoc);
            }
            else if (bFirstDiscovery && bDiscovered) {
                HUDMainMenu::SetLocationText(kTravelLoc);
                bFirstDiscovery = false;
            }
        }
        if (!pPos) {
            continue;
        }
        pTile = apGeneralIcons->xChildren.GetNext(pPos);
        if (bCanFastTravel) {
            pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_compass_landmark_discovered.dds", true);
        }
        else {
            pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_compass_landmark.dds", true);
        }
        if (!pTile) {
            continue;
        }
        const float fAngleDelta = HUDMainMenu::GetCompassAngle(kItemLoc, kPlayerLoc);
        if (static_cast<double>(HUDMainMenu::pme->iCompassHalfAngle) > std::fabs(fAngleDelta)) {
            const float fTargetAngle = FLerp(
                0.0f,
                static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
                static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
                static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle),
                fAngleDelta
            );
            pTile->Set(g_HUDMainMenuTraits_Heading, fTargetAngle, true);
            pTile->Set(g_HUDMainMenuTraits_Distance, fDistSqr, true);
            pTile->Set(Tile::kTileValue_Visible, 1);
            if (fFadeStart < fTargetAngle) {
                const float fAlpha = FLerp(1.0f, 0.0f, fFadeStart, fFadeEnd, fTargetAngle) * fFullBright;
                pTile->Set(Tile::kTileValue_Alpha, fAlpha, true);
            }
            else if (fDistSqr < 10000.0) {
                HandleQuestObjectiveBlinking(pTile, fDistSqr, fCurrentHUDOpacity);
            }
            else {
                pTile->Set(Tile::kTileValue_Alpha, fFullBright, true);
            }
        }
        else {
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
    while (pQuestNode && pQuestNode->GetItem()) {
        bool bIsThroughDoor = false;
        TESQuestTarget* pQuestTarget = pQuestNode->GetItem();
        pQuestNode = pQuestNode->GetNext();
        TESObjectREFR* pTargetRef = pQuestTarget->QTrackingRef();
        if (pTargetRef && pTargetRef->GetFormType() == DOOR_ID) {
            bIsThroughDoor = true;
        }
        if (!pTargetRef || !bShowQuestMarkers.Bool()) {
            continue;
        }
        const NiPoint3 kTargetLoc = pTargetRef->GetLocationOnReference();
        const float fDistSqr = std::fabs((kTargetLoc - kPlayerLoc).SqrLength());
        if (!pPos) {
            continue;
        }
        pTile = apMarkerIcons->xChildren.GetPrev(pPos);
        pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_compass_objective_marker.dds", true);
        if (!pTile) {
            continue;
        }
        float fAngleDelta = HUDMainMenu::GetCompassAngle(kTargetLoc, kPlayerLoc);
        if (fAngleDelta > static_cast<double>(HUDMainMenu::pme->iCompassHalfAngle)) {
            fAngleDelta = static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle);
        }
        else if (fAngleDelta < static_cast<double>(-HUDMainMenu::pme->iCompassHalfAngle)) {
            fAngleDelta = static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle);
        }
        float fTargetAngle = FLerp(
            0.0f,
            static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
            static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
            static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle),
            fAngleDelta
        );
        if (fTargetAngle > fFadeStart + 70.0) {
            fTargetAngle = fFadeStart + 70.0;
        }
        pTile->Set(g_HUDMainMenuTraits_Heading, fTargetAngle, true);
        pTile->Set(g_HUDMainMenuTraits_Distance, fDistSqr, true);
        pTile->Set(Tile::kTileValue_Visible, 1);
        if (apCompass->GetMenu()->xFadeState == Interface::FADE_SHOWN) {
            if (bIsThroughDoor) {
                pTile->Set(Tile::kTileValue_Alpha, fFullBright, true);
            }
            else {
                HandleQuestObjectiveBlinking(pTile, fDistSqr, fCurrentHUDOpacity);
            }
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
                const float fTargetAngle = FLerp(
                    0.0f,
                    static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
                    static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
                    static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle),
                    fAngleDelta
                );
                pTile->Set(g_HUDMainMenuTraits_Heading, fTargetAngle, true);
                pTile->Set(Tile::kTileValue_Visible, 1);
                if (fFadeStart < fTargetAngle) {
                    const float fAlpha = FLerp(fFullBright, 0.0f, fFadeStart, fFadeEnd, fTargetAngle);
                    HandleQuestObjectiveBlinking(pTile, fDistSqr, fAlpha);
                }
                else {
                    HandleQuestObjectiveBlinking(pTile, fDistSqr, fCurrentHUDOpacity);
                }
            }
            else {
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
        if (!pTarget ||
            pTarget->bIsUndetected ||
            !pTarget->pTarget->Get3D() ||
            !pTarget->pTarget->GetParentCell() ||
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
            // Out of icons: the loop goes round again without moving on (the tick count still runs down).
            if (!pPos) {
                continue;
            }
            pTile = apNPCIcons->xChildren.GetNext(pPos);
            pTile->Set(Tile::kTileValue_Filename, "Interface\\HUD\\glow_hud_tick_mark.dds", true);
            if (pTile) {
                const float fAngleDelta = HUDMainMenu::GetCompassAngle(kActorLoc, kPlayerLoc);
                if (static_cast<double>(HUDMainMenu::pme->iCompassHalfAngle) > std::fabs(fAngleDelta)) {
                    const float fTargetAngle = FLerp(
                        0.0f,
                        static_cast<float>(2 * HUDMainMenu::pme->iCompassDisplayWidth),
                        static_cast<float>(-HUDMainMenu::pme->iCompassHalfAngle),
                        static_cast<float>(HUDMainMenu::pme->iCompassHalfAngle),
                        fAngleDelta
                    );
                    const float fTickAngle = fTargetAngle + 2.0;
                    pTile->Set(g_HUDMainMenuTraits_Heading, fTickAngle, true);
                    pTile->Set(Tile::kTileValue_Visible, 1);
                    if (pTarget->bIsHostile) {
                        pTile->Set(Tile::kTileValue_SystemColor, 2);
                        g_bIsEnemyWithinCompassRange = 1;
                    }
                    else {
                        pTile->Set(Tile::kTileValue_SystemColor, 1);
                    }
                    if (fFadeStart < fTargetAngle) {
                        pTile->Set(
                            Tile::kTileValue_Alpha,
                            FLerp(fFullBright, 0.0f, fFadeStart, fFadeEnd, fTargetAngle),
                            true
                        );
                    }
                    else {
                        pTile->Set(Tile::kTileValue_Alpha, fFullBright, true);
                    }
                }
                else {
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

void HUDMainMenu::UpdateMeter(
    Tile* apTile,
    float afMeterPercent,
    HUDMainMenu::HUD_JUSTIFY aeJustify,
    int32_t aiMinTicks,
    int32_t iNumPixelShift
) {
    static int32_t iUT_TotalWidth = Tile::AddUserTrait("_TotalWidth", -1);
    static int32_t iUT_ImageWidth = Tile::AddUserTrait("_ImageWidth", -1);
    static int32_t iUT_OriginalX = Tile::AddUserTrait("_OriginalX", -1);
    if (apTile && HUDMainMenu::pme) {
        int32_t iWidth;
        if (afMeterPercent > 1.0) {
            afMeterPercent = 1.0;
        }
        else if (afMeterPercent < 0.0) {
            afMeterPercent = 0.0;
        }
        int32_t iTotalWidth = static_cast<int32_t>(apTile->GetFloat(iUT_TotalWidth));
        int32_t iImageWidth = static_cast<int32_t>(apTile->GetFloat(iUT_ImageWidth));
        if (aeJustify == HUDMainMenu::HJ_CENTER) {
            float fPercent;
            if (afMeterPercent >= 0.0) {
                fPercent = afMeterPercent;
            }
            else {
                fPercent = 0.0;
            }
            iWidth = 2 * iImageWidth *
                static_cast<int32_t>(
                         static_cast<double>(iTotalWidth) * fPercent / static_cast<double>(2 * iImageWidth)
                );
        }
        else {
            float fPercent;
            if (afMeterPercent >= 0.0) {
                fPercent = afMeterPercent;
            }
            else {
                fPercent = 0.0;
            }
            iWidth = iImageWidth *
                static_cast<int32_t>(static_cast<double>(iTotalWidth) * fPercent / static_cast<double>(iImageWidth));
        }
        if (iWidth < aiMinTicks * iImageWidth) {
            iWidth = aiMinTicks * iImageWidth;
        }
        int32_t iX = static_cast<int32_t>(apTile->GetFloat(iUT_OriginalX));
        if (aeJustify == HUDMainMenu::HJ_RIGHT) {
            iX -= iWidth;
        }
        else if (aeJustify == HUDMainMenu::HJ_CENTER) {
            iX -= iWidth / 2;
        }
        apTile->Set(4017, iWidth);
        apTile->Set(4001, iNumPixelShift + iX);
    }
}

void HUDMainMenu::UpdateMeter(
    Tile* apMeterTile,
    Tile* apGhostMeterTile,
    float afMeterPercent,
    float afGhostMeterPercent,
    HUDMainMenu::HUD_JUSTIFY aeJustify,
    int32_t aiMinTicks
) {
    // The meter tiles' user traits.
    static int32_t g_VATSMenuTrait_TotalWidth = Tile::AddUserTrait("_TotalWidth", -1);
    static int32_t g_VATSMenuTrait_ImageWidth = Tile::AddUserTrait("_ImageWidth", -1);
    static int32_t g_VATSMenuTrait_OriginalX = Tile::AddUserTrait("_OriginalX", -1);
    static int32_t g_VATSMenuTrait_Brightness = Tile::AddUserTrait("_Brightness", -1);
    if (apMeterTile && apGhostMeterTile && HUDMainMenu::pme) {
        int32_t itotalNewWidth;
        int32_t inewWidth;
        if (afMeterPercent > 1.0) {
            afMeterPercent = 1.0f;
        }
        else if (afMeterPercent < 0.0) {
            afMeterPercent = 0.0f;
        }
        if (afGhostMeterPercent > 1.0) {
            afGhostMeterPercent = 1.0f;
        }
        else if (afGhostMeterPercent < 0.0) {
            afGhostMeterPercent = 0.0f;
        }
        int32_t imeterWidth = static_cast<int32_t>(apMeterTile->GetFloat(g_VATSMenuTrait_TotalWidth));
        int32_t iimageWidth = static_cast<int32_t>(apMeterTile->GetFloat(g_VATSMenuTrait_ImageWidth));
        if (aeJustify == HUDMainMenu::HJ_CENTER) {
            itotalNewWidth = 2 * iimageWidth *
                static_cast<int32_t>(
                                 static_cast<double>(imeterWidth) * afGhostMeterPercent /
                                 static_cast<double>(2 * iimageWidth)
                );
        }
        else {
            itotalNewWidth = iimageWidth *
                static_cast<int32_t>(
                                 static_cast<double>(imeterWidth) * afGhostMeterPercent /
                                 static_cast<double>(iimageWidth)
                );
        }
        if (aeJustify == HUDMainMenu::HJ_CENTER) {
            float fPercent = afMeterPercent >= 0.0 ? afMeterPercent : 0.0f;
            inewWidth = 2 * iimageWidth *
                static_cast<int32_t>(
                            static_cast<double>(imeterWidth) * fPercent / static_cast<double>(2 * iimageWidth)
                );
        }
        else {
            float fPercent = afMeterPercent >= 0.0 ? afMeterPercent : 0.0f;
            inewWidth = iimageWidth *
                static_cast<int32_t>(static_cast<double>(imeterWidth) * fPercent / static_cast<double>(iimageWidth));
        }
        int32_t inewX = static_cast<int32_t>(apMeterTile->GetFloat(g_VATSMenuTrait_OriginalX));
        if (aeJustify == HUDMainMenu::HJ_RIGHT) {
            inewX -= inewWidth;
        }
        else if (aeJustify == HUDMainMenu::HJ_CENTER) {
            inewX -= inewWidth / 2;
        }
        int32_t ighostWidth = itotalNewWidth;
        int32_t ighostX = static_cast<int32_t>(apGhostMeterTile->GetFloat(g_VATSMenuTrait_OriginalX));
        if (aeJustify == HUDMainMenu::HJ_RIGHT) {
            ighostX = inewX - (itotalNewWidth - inewWidth);
            int32_t iOriginalX = static_cast<int32_t>(apGhostMeterTile->GetFloat(g_VATSMenuTrait_OriginalX));
            ighostWidth = BSMIN(itotalNewWidth - inewWidth, iOriginalX - ighostX);
        }
        else if (aeJustify) {
            if (aeJustify == HUDMainMenu::HJ_CENTER) {
                ighostX -= itotalNewWidth / 2;
                ighostWidth = inewX - ighostX;
                Tile* pTile = apGhostMeterTile->xChildren.GetHead();
                pTile->Set(4017, inewX - ighostX);
                pTile->Set(4001, inewWidth + inewX);
                if (!pTile->HasFadeControl(4009)) {
                    pTile->AddFadeControl(4009, 0.0f, 255.0f, 1.0f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
                }
            }
        }
        else {
            ighostX += inewWidth;
        }
        if (!inewWidth && ighostWidth < aiMinTicks * iimageWidth) {
            ighostWidth = aiMinTicks * iimageWidth;
        }
        apMeterTile->Set(4017, inewWidth);
        apMeterTile->Set(4001, inewX);
        apGhostMeterTile->Set(4017, ighostWidth);
        apGhostMeterTile->Set(4001, ighostX);
        if (!apGhostMeterTile->HasFadeControl(4009)) {
            apGhostMeterTile->AddFadeControl(4009, 0.0f, 255.0f, 1.0f, Tile::TFCT_NONLINEAR_LONG_DARK_REPEATING);
        }
    }
}

void HUDMainMenu::SetRegionText(const char* aRegionData) {
    [[maybe_unused]] static int32_t iUT_Hold = Tile::TextToTrait("_Hold");
    if (!aRegionData) {
        return;
    }
    if (HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFloat(Tile::kTileValue_Alpha) == 0.0 &&
        HUDMainMenu::pme->pTiles[HMMT_LOCATION_TEXT]->GetFadeFinished(Tile::kTileValue_Alpha)) {
        HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::kTileValue_String, aRegionData, true);
        HUDMainMenu::pme->pTiles[HMMT_REGION_TEXT]->Set(Tile::TextToTrait("_Hold"), 1);
    }
}

float HUDMainMenu::HeadingBetweenLocations(const NiPoint3& aStart, const NiPoint3& aDest) {
    float fRatio;
    NiPoint3 kDelta;
    float fAngle = 0.0f;
    kDelta = aDest - aStart;
    if (kDelta.y == 0.0) {
        if (kDelta.x < 0.0) {
            fAngle = 3.1415927410125732f;
        }
    }
    else {
        fRatio = kDelta.x / kDelta.y;
        fAngle = std::atanf(fRatio);
        if (kDelta.y < 0.0) {
            fAngle = static_cast<float>(fAngle + 3.1415927410125732);
        }
    }
    float fClampedAngle = ClampAngle(fAngle);
    return static_cast<float>(fClampedAngle * 57.2957763671875);
}

void HUDMainMenu::DisplayQuestReminder(TESQuest* apQuest) {
    if (HUDMainMenu::pme &&
        apQuest &&
        !HUDMainMenu::pme->AreObjectivesShowing() &&
        HUDMainMenu::pme->xQuestObjectives.IsEmpty()) {
        for (BSSimpleList<BGSQuestObjective*>* pList = apQuest->GetObjectiveList(); pList && !pList->IsEmpty();
             pList = pList->m_pkNext) {
            if (pList->m_item->GetStatus(1)) {
                if (!pList->m_item->GetStatus(2)) {
                    HUDMainMenu::ShowQuestObjective(pList->m_item->GetDisplayText(), false, true);
                }
            }
        }
    }
}

// With no quest or no HUD this returns with the critical section still held.
void HUDMainMenu::ShowQuestName(TESQuest* apQuest) {
    HUDShowQuestNameCritSect.Enter(nullptr);
    if (!apQuest || !HUDMainMenu::pme) {
        return;
    }
    uiMessageDelayTime = appTimer.GetTime();
    auto* pQuestUpdate = static_cast<QuestUpdateManager::QuestUpdate*>(
        ourMalloc(sizeof(QuestUpdateManager::QuestUpdate))
    );
    pQuestUpdate->pQuest = apQuest;
    if (apQuest->GetFailed()) {
        pQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_FAILED;
        if (pPlayer->GetActiveQuest() == apQuest) {
            pPlayer->SetActiveQuest(nullptr);
        }
    }
    else if (apQuest->GetCompleted()) {
        pQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_COMPLETE;
        if (pPlayer->GetActiveQuest() == apQuest) {
            pPlayer->SetActiveQuest(nullptr);
        }
    }
    else {
        pQuestUpdate->eUpdateType = QuestUpdateManager::HQUT_ADD;
        if (!pPlayer->GetActiveQuest()) {
            pPlayer->SetActiveQuest(apQuest);
        }
    }
    QuestUpdateManager::AddQuestNamesTail(pQuestUpdate);
    HUDShowQuestNameCritSect.Leave();
}

void HUDMainMenu::ShowEnemyDDTIcon(bool abArmorPiercing) {
    HUDMainMenu::pme->uiEnemyDDTdisplayTimer = appTimer.BSTimer::GetTime();
    if ((HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_ENEMY_HEALTH) != 0) {
        if (abArmorPiercing) {
            HUDMainMenu::pme->pTiles[83]->Set(4003, 1);
        }
        else {
            HUDMainMenu::pme->pTiles[76]->Set(4003, 1);
        }
    }
}

void HUDMainMenu::SetHUDShake(float afShake) {
    if (HUDMainMenu::pme) {
        HUDMainMenu::pme->fShake = afShake * HUDMainMenu::fHUDShakeMult.Float();
    }
}

void HUDMainMenu::ClearXPMeterFinishedMoving() {
    if (HUDMainMenu::pme) {
        HUDMainMenu::pme->bXPMeterFinishedMoving = false;
    }
    g_XPBarState = 1;
}

void HUDMainMenu::InitGunScope(TESModel* apModelFilename) {
    if (HUDMainMenu::pme && HUDMainMenu::spScopeModel.m_pObject) {
        HUDMainMenu::ClearGunScope();
        if (apModelFilename && apModelFilename->GetModelLength()) {
            // Fetches the scope node before it loads the model.
            NiNode* pScopeModel = HUDMainMenu::spScopeModel.m_pObject;
            NiNode* pModel = pModelLoader
                                 ->LoadFile(apModelFilename->GetModel(), LOD_MULT_NONE, true, false, false, false);
            pScopeModel->AttachChild(pModel, true);
            HUDMainMenu::spScopeModel->SetAppCulled(true);
            NiUpdateData kUpdateData(0.0f, false, false);
            HUDMainMenu::spScopeModel->Update(kUpdateData);
        }
    }
}

void HUDMainMenu::ForceObjectiveReminders(bool abValue) {
    if (HUDMainMenu::pme) {
        HUDMainMenu::pme->bForceObjectiveReminders = abValue;
    }
}

void HUDMainMenu::ShowDDTIcon(bool abArmorPiercing) {
    HUDMainMenu::pme->uiDDTdisplayTimer = appTimer.BSTimer::GetTime();
    if ((HUDMainMenu::pme->iHUDVisibilityMenuModeFlags & HV_AP) != 0) {
        if (abArmorPiercing) {
            HUDMainMenu::pme->pTiles[82]->Set(4003, 1);
        }
        else {
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
            }
            else {
                pLastLocation = nullptr;
            }
        }
        else {
            pLastLocation = aLocationData.pMarker;
            if (aLocationData.pMarker && aLocationData.pMarker->GetMapMarkerData()) {
                if (HUDMainMenu::pme->pTiles[16]->GetFadeFinished(4009) &&
                    HUDMainMenu::pme->pTiles[16]->GetFloat(4009) == 0.0) {
                    MapMarkerData* pMapMarkerData = aLocationData.pMarker->GetMapMarkerData();
                    const char* pLocationName = pMapMarkerData->GetLocationName();
                    HUDMainMenu::pme->pTiles[16]->Set(4036, pLocationName, true);
                    float fEnd = fHudOpacity.Float() * 255.0;
                    HUDMainMenu::pme->pTiles[16]->AddFadeControl(4009, 0.0f, fEnd, 2.0f, Tile::TFCT_STANDARD);
                    HUDMainMenu::pme->pTiles[16]->Set(Tile::kTileValue_User0, appTimer.iLastTime);
                }
                HUDMainMenu::pme->pTiles[15]->RemoveFadeControl(4009);
                HUDMainMenu::pme->pTiles[15]->Set(4009, 0.0f, true);
            }
        }
    }
}

void HUDMainMenu::QueueXPMessage(int32_t aiAddedXP) {
    int32_t iLevel = pPlayer->GetLevel();
    if (iLevel < iMaxCharacterLevel.Int() && !HUDMainMenu::pme->bXPMeterFinishedMoving) {
        HUDMainMenu::XPUpdate* pUpdate = static_cast<HUDMainMenu::XPUpdate*>(ourMalloc(sizeof(HUDMainMenu::XPUpdate)));
        pUpdate->iAddedXP = aiAddedXP;
        pUpdate->iPlayerLevel = iLevel;
        pUpdate->iPlayerXP = static_cast<int32_t>(pPlayer->GetActorFloatValue(ActorValue::EXPERIENCE_POINTS));
        HUDMainMenu::pme->xXPUpdates.AddHead(pUpdate);
    }
}

void HUDMainMenu::SetVisibilityFlag(int32_t aiFlag, int32_t aiVisible) {
    if (HUDMainMenu::pme) {
        if (aiVisible) {
            HUDMainMenu::pme->iHUDVisibilityUserSettings |= aiFlag;
        }
        else {
            HUDMainMenu::pme->iHUDVisibilityUserSettings &= ~aiFlag;
        }
    }
}

// The menu GetMenuByClass finds for the HUD's ID is the HUD itself; its fields are written through it.
void HUDMainMenu::Revert() {
    int32_t iMenuID = HUDMainMenu::GetMenuID();
    Tile* pMenuTile = Menu::GetMenuByClass(iMenuID);
    if (pMenuTile && pMenuTile->GetMenu()) {
        HUDMainMenu* pHUD = static_cast<HUDMainMenu*>(pMenuTile->GetMenu());
        pHUD->pCurrentEnemy = nullptr;
        pHUD->uiQuestReminderTimer = 0;
        pHUD->uiDDTdisplayTimer = -1;
        pHUD->uiEnemyDDTdisplayTimer = -1;
        pHUD->bForceObjectiveReminders = false;
        pHUD->pTiles[50]->Set(4003, 0);
        pHUD->bXPMeterFinishedMoving = false;
        HUDMainMenu::pme->pTiles[44]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[45]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[46]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[47]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[48]->Set(4003, 0);
        HUDMainMenu::pme->pTiles[49]->Set(4003, 0);
        g_XPBarState = 1;
        HUDMainMenu::pme->bKillQuestUpdates = true;
    }
}

double HUDMainMenu::GetCompassAngle(NiPoint3 aDest, NiPoint3 aStart) {
    NiPoint3 kDelta;
    kDelta = aDest - aStart;
    float fZAngle = GetUnclampedZAngleFromVector(kDelta);
    float fAngle = 0.0;
    fAngle = fZAngle - pPlayer->GetHeading(false);
    if (fAngle <= -3.1415927410125732) {
        fAngle = fAngle + 6.2831854820251465;
    }
    else if (fAngle > 3.1415927410125732) {
        fAngle = 6.2831854820251465 - fAngle;
    }
    fAngle = fAngle * 57.2957763671875;
    return static_cast<float>(fAngle * 1.4500000476837158);
}

HUDMainMenu::SubtitleData::SubtitleData() {}

int32_t TESWaterForm::GetHealthRestoration() {
    return iHealthRestoration;
}

bool Projectile::IsEditorCreated() {
    return !pDataHandler->IsFormIDCreated(GetFormID());
}
