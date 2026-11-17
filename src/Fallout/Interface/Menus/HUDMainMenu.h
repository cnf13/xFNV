#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSTimer.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout/Interface/HotKeysWheel.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class Actor;
class NiCamera;
class TESModel;
class TESObjectREFR;
class TESQuest;
class TESQuestTarget;
class TESRegion;

// The victim's damage threshold after both actors' perk entry points, at least 0.
float GetAdjustedDamageThreshold(Actor* pAttacker, Actor* pVictim);
void HandleQuestObjectiveBlinking(Tile* th_s, float value, float a3);
int32_t SquareInt(int32_t a1);

// The HUD's queue of quest name and objective updates; it has no data of its own, only statics.
struct QuestUpdateManager {
    // The kind of quest update shown.
    enum HUD_QUEST_UPDATE_TYPE : int32_t {
        HQUT_ADD = 0,
        HQUT_COMPLETE = 1,
        HQUT_FAILED = 2,
        HQUT_CUSTOM = 3,
    };

    enum HUD_CUSTOM_QUEST_TEXT_JUSTIFICATION : int32_t {
        HCQTJ_LEFT = 0,
        HCQTJ_CENTER = 1,
        HCQTJ_RIGHT = 2,
    };

    enum HUD_CUSTOM_QUEST_QUEUE_PRIORITY : int32_t {
        HCQQP_NOW = 0,
        HCQQP_NORMAL = 1,
        HCQQP_LAST = 2,
    };

    // A queued quest name update: the quest, the kind of update and the texts of a custom one.
    struct QuestUpdate {
        TESQuest* pQuest;
        HUD_QUEST_UPDATE_TYPE eUpdateType;
        char CustomTextTitle[0x104];
        char CustomTextSubtitle[0x104];
        HUD_CUSTOM_QUEST_TEXT_JUSTIFICATION CustomTextJustification;
        HUD_CUSTOM_QUEST_QUEUE_PRIORITY CustomQueuePriority;
        FONTS CustomTextTitleFont;
        FONTS CustomTextSubtitleFont;
        char CustomSoundHandleName[0x104];
    };

    // True when no quest name update is queued.
    bool IsQuestNamesEmpty();
    // Drops the first queued quest name update.
    void RemoveQuestNamesHead();
    // Drops every queued quest name update.
    void RemoveQuestNamesAll();
    QuestUpdateManager::QuestUpdate* GetQuestNamesItem();

    // Appends pQuestUpdate to the quest name queue and sorts it by priority.
    static void AddQuestNamesTail(QuestUpdate* pQuestUpdate);
    static int32_t
    SortQuestNamesByPriorityFunction(QuestUpdateManager::QuestUpdate* pQU1, QuestUpdateManager::QuestUpdate* pQU2);
    static void SetCustomQuestText(
        const char* apCustomTextTitle,
        const char* apCustomTextSubtitle,
        QuestUpdateManager::HUD_CUSTOM_QUEST_QUEUE_PRIORITY aiCustomQueuePriority,
        QuestUpdateManager::HUD_CUSTOM_QUEST_TEXT_JUSTIFICATION aiCustomTextJustification,
        FONTS aiCustomTextTitleFont,
        FONTS aiCustomTextSubtitleFont,
        const char* apCustomSoundHandleName
    );
    static void AddQuestNamesHead(QuestUpdateManager::QuestUpdate* pQuestUpdate);

    // The queue of quest name updates the HUD shows one after another.
    static BSSimpleList<QuestUpdate*> xQuestNames;
};

// The heads-up display menu.
class HUDMainMenu : public Menu {
public:
    // The vault boy face a HUD message shows.
    enum HUD_MESSAGE_EMOTION : int32_t {
        HME_NEUTRAL = 0,
        HME_HAPPY = 1,
        HME_SAD = 2,
        HME_PAIN = 3,
    };

    // A queued HUD message and a queued subtitle (defined after the class).
    struct MessageData;
    struct QueuedQuestText;
    struct SubtitleData;
    // A queued experience-meter and radiation-meter update (defined after the class).
    struct XPUpdate;
    struct RadiationUpdate;
    // The region text to show and the compass's travel location (defined after the class).
    struct HUDRegionData;
    struct HUDTravelLocData;

    // The HUD's modes; each one picks the set of HUD elements that are shown.
    enum HUD_MENU_MODE : int32_t {
        HMM_NONE = 0x0,
        // Asks SetMenuMode to work the mode out from the top menu and the player's state.
        HMM_CHANGED = 0x1,
        HMM_RUN_AND_GUN = 0x2,
        HMM_PIPBOY = 0x3,
        HMM_START = 0x4,
        HMM_LOAD = 0x5,
        HMM_DIALOGUE = 0x6,
        HMM_VATS = 0x7,
        HMM_VATS_PLAYBACK = 0x8,
        HMM_CONTAINER = 0x9,
        HMM_SIT_SLEEP_ANIM = 0xA,
        HMM_SIT_SLEEP = 0xB,
        HMM_MOVEMENT_DISABLED = 0xC,
        HMM_PIPBOY_DISABLED = 0xD,
        HMM_CHARGEN_MENU = 0xE,
        HMM_HACKING = 0xF,
        HMM_TERMINAL = 0x10,
        HMM_MESSAGE_MENU = 0x11,
        HMM_SPECIAL_BOOK = 0x12,
        HMM_LOVE_TESTER = 0x13,
        HMM_VANITY = 0x14,
        HMM_AUTO_VANITY = 0x15,
        HMM_IRON_SIGHTS = 0x16,
        HMM_SNIPER_RIFLE = 0x17,
        HMM_MOVIE = 0x18,
        HMM_CASINO_GAME = 0x19,
    };

    // The brightness class of a HUD tile, for SetBrightness.
    enum HUD_BRIGHTNESS : int32_t {
        HB_HUD_TEXT = 0,
        HB_HUD_BRACKETS = 1,
        HB_HUD_BRACKETS_GLOW = 2,
        HB_HUD_METERS = 3,
        HB_HUD_METER_BACKGROUND = 4,
        HB_HUD_METERS_GHOST = 5,
        HB_HUD_ICONS = 6,
        HB_HUD_DEFAULT = 7,
    };

    // How UpdateMeter lines a meter's ticks up in its tile.
    enum HUD_JUSTIFY : int32_t {
        HJ_LEFT = 0,
        HJ_RIGHT = 1,
        HJ_CENTER = 2,
    };

    // The HUD tile ids (the index into pTiles).
    enum HUD_MAIN_MENU_TILES : int32_t {
        HMMT_HEALTH_BAR = 1,
        HMMT_HIT_POINT_TEXT = 2,
        HMMT_COMPASS_GENERAL_ICONS = 3,
        HMMT_COMPASS_MARKER_ICONS = 4,
        HMMT_COMPASS_NPC_ICONS = 5,
        HMMT_COMPASS = 6,
        HMMT_ACTION_POINTS_BAR = 7,
        HMMT_ACTION_POINTS_TEXT = 8,
        HMMT_WEAPON_CONDITION_TEXT = 9,
        HMMT_WEAPON_CONDITION_METER = 10,
        HMMT_WEAPON_CONDITION_METER_BACKGROUND = 11,
        HMMT_AMMO_TEXT = 12,
        HMMT_QUEST_STAGES = 13,
        HMMT_QUEST_ADDED = 14,
        HMMT_REGION_TEXT = 15,
        HMMT_LOCATION_TEXT = 16,
        HMMT_RADIATION_BRACKET = 17,
        HMMT_RADIATION_POINTER = 18,
        HMMT_RADIATION_TEXT_VALUE = 19,
        HMMT_RADIATION_TEXT = 20,
        HMMT_ENEMY_HEALTH_BRACKET = 21,
        HMMT_ENEMY_HEALTH_METER = 22,
        HMMT_ENEMY_HEALTH_NAME = 23,
        HMMT_SNEAK_TEXT = 24,
        HMMT_MESSAGE_ICON = 25,
        HMMT_MESSAGE_TEXT = 26,
        HMMT_MESSAGE_BRACKET = 27,
        HMMT_SUBTITLE_TEXT = 28,
        HMMT_INFO_ACTION_HINT = 29,
        HMMT_INFO_ACTION_PC_HINT = 30,
        HMMT_INFO_ACTION_XBOX_HINT = 31,
        HMMT_INFO_ACTION_NAME = 32,
        HMMT_INFO_DOOR_STATE = 33,
        HMMT_INFO_CONTAINER_STATE = 34,
        HMMT_INFO_LABEL_ONE = 35,
        HMMT_INFO_VALUE_ONE = 36,
        HMMT_INFO_LABEL_TWO = 37,
        HMMT_INFO_VALUE_TWO = 38,
        HMMT_INFO_SEPARATOR = 39,
        HMMT_HOTKEY_WHEEL = 40,
        HMMT_HOTKEY_TEXT = 41,
        HMMT_LEFT_BRACKET = 42,
        HMMT_RIGHT_BRACKET = 43,
        HMMT_XP_BRACKET = 44,
        HMMT_XP_TEXT = 45,
        HMMT_XP_LABEL = 46,
        HMMT_XP_POINTER = 47,
        HMMT_XP_LASTLEVEL = 48,
        HMMT_XP_NEXTLEVEL = 49,
        HMMT_XP_LEVELUP_TEXT = 50,
        HMMT_CROSSHAIR_CENTER = 51,
        HMMT_CRIPPLED_FACE = 52,
        HMMT_CRIPPLED_HEAD = 53,
        HMMT_CRIPPLED_TORSO = 54,
        HMMT_CRIPPLED_LEFT_ARM = 55,
        HMMT_CRIPPLED_RIGHT_ARM = 56,
        HMMT_CRIPPLED_LEFT_LEG = 57,
        HMMT_CRIPPLED_RIGHT_LEG = 58,
        HMMT_AP = 59,
        HMMT_HP = 60,
        HMMT_RADIATION = 61,
        HMMT_ENEMY_HEALTH = 62,
        HMMT_QUEST_UPDATE = 63,
        HMMT_REGION_LOCATION = 64,
        HMMT_CROSSHAIR = 65,
        HMMT_SNEAK = 66,
        HMMT_MESSAGE = 67,
        HMMT_INFO = 68,
        HMMT_SUBTITLE = 69,
        HMMT_HOTKEY = 70,
        HMMT_XP = 71,
        HMMT_BREATH = 72,
        HMMT_EXPLOSIVE_INDICATOR = 73,
        HMMT_CRIPPLED_GUY = 74,
        HMMT_DDT_ICON = 75,
        HMMT_DDT_ICON_ENEMY = 76,
        HMMT_AMMO_SUB_TYPE = 77,
        HMMT_HARDCORE_MODE = 78,
        HMMT_HARDCORE_H20 = 79,
        HMMT_HARDCORE_SLP = 80,
        HMMT_HARDCORE_FOD = 81,
        HMMT_DDT_ICON_AP = 82,
        HMMT_DDT_ICON_ENEMY_AP = 83,
        HMMT_HARDCORE_RAD = 84,
        HMMT_HARDCORE_LMB = 85,
        HMMT_CND_ARROWS = 86,
        HMMT_NUM_TILES = 86,
    };

    HUDMainMenu();
    ~HUDMainMenu() override;

    // Stores the tile for a HUD tile id; ids outside 1..86 are ignored.
    void AttachTileByID(int32_t aiID, Tile* apTile) override;
    void DoIdle() override;
    int32_t GetClass() override;

    // Queues a message for the HUD.
    bool ShowMessage(
        const char* apString,
        HUD_MESSAGE_EMOTION aeEmotion,
        const char* apIconFilename,
        const char* apSoundName,
        float afShowSeconds,
        bool abMoveToHead
    );
    // True when every tile id from 1 to 85 is attached.
    bool IsWellFormedMenu();
    // Builds the gun scope's camera: a perspective camera on the full viewport (the letterbox viewport while
    // letterboxed) with the world scene's far distance and the display near distance, looking up the z axis.
    NiCamera* Create3DCamera();
    // Hides the subtitle being shown.
    void HideSubtitle();
    // Queues apString as a subtitle spoken at aSpeakerLoc by apTarget, timed by the sound ahandle.
    bool ShowSubtitle(
        const char* apString,
        BSSoundHandle ahandle,
        NiPoint3 aSpeakerLoc,
        TESObjectREFR* apTarget,
        bool abForceShow
    );
    // Frees the queued messages, clears the message tiles and hides the subtitle.
    void ClearAllSubtitlesAndMessages();
    // Runs the hotkeys: with a gamepad, opens the wheel once a hotkey is held and uses the highlighted item on
    // release; on the keyboard, uses a hotkey's item when its key is pressed, opens the wheel while it is held and
    // closes it fHotKeyShowTimer seconds later.
    void UpdateHotkeysWheel();
    // Shows the equipped weapon's ammo count, ammo type and condition meter (hidden for melee weapons and with no
    // weapon); the condition meter pulses between 0 and the HUD opacity while the condition is below 25%.
    void UpdateWeaponStatus();
    // Points the explosive indicator tiles at live grenades within fExplosiveIndicatorDistance.
    void UpdateExplosiveIndicator();
    // Moves the HUD tile by a random offset of fShake from where it first was, and clears fShake.
    void UpdateShake();
    // Moves the weapon condition tiles right by aishift pixels.
    void ShiftConditionTileOver(int32_t aishift);
    // Refreshes the meter of an actor value.
    void UpdateActorValue(ActorValue::Index aeActorValue);
    void UpdateBreathMeter();
    // Shows the sneak label.
    void UpdateSneak();
    // Shows the next queued quest name and subtitle.
    void UpdateQuestText();
    void UpdateXPMeter();
    void UpdateRadiationMeter();
    void UpdateStatusModeUI(bool abIsHardcore);
    // Refreshes the enemy health meter for apActor.
    void UpdateEnemyHealth(Actor* apActor);
    // Hides the quest-added tiles and frees the queued quest names.
    void KillQuestName();
    void KillQuestObjectives();
    // The queued subtitle to show next, or null.
    SubtitleData* GetNextSubtitle();
    // Hides the message being shown.
    void HideMessage();
    bool IsHotKeyWheelVisible() {
        return HKWheel.GetVisible();
    }
    void KillQuestUpdates();
    void ShowCrippledGuy();
    bool IsQuestNameShowing();
    bool AreObjectivesShowing();
    void UpdateRadiationMeterConditions(float a1, float a2);
    void StatusModeTileEffects(Tile* apTile, float afCurrValue, bool bResetValues);
    // Fades the quest objective tiles in apTile's children. UpdateQuestText passes pTiles[13], and the body walks
    // apTile->xChildren.
    void HandleReminderFade(Tile* apTile);
    void HandleCinematicFade(Tile* th_s);

    // Hides the gun scope overlay.
    static void ClearGunScope();
    // The menu's class ID.
    static int32_t GetMenuID();
    // The current HUD mode, or 0 while the HUD does not exist.
    static HUD_MENU_MODE GetMenuMode();
    // The enemy the HUD is tracking, or null.
    static Actor* GetEnemyActor();
    // True while the gun scope overlay is up.
    static bool IsGunScopeVisible();
    // Forgets the crosshair reference when it is the given one; true if it was cleared.
    static bool ClearLastObjectIfItIs(TESObjectREFR* apObject);
    // Rebuilds the crosshair info for the crosshair reference.
    static void RefreshInfoForRef();
    // Shows the crosshair info (action prompt, name, lock and container state, weight and value) for apObject and
    // tints the crosshair; false when the HUD is gone, the object is unchanged or no furniture marker is free.
    static bool SetInfoForRef(TESObjectREFR* apObject, bool abTelekinesis, bool abSameOkay);
    // Shows a water's health restoration in the info label and value tiles aiInfoLabel and
    // aiInfoValue.
    static void DisplayWaterHealth(int32_t aiHealth, int32_t aiInfoLabel, int32_t aiInfoValue);
    // Shows a water's radiation in the info label and value tiles aiInfoLabel and aiInfoValue.
    static void DisplayWaterRadiation(int32_t aiRadiationAmount, int32_t aiInfoLabel, int32_t aiInfoValue);
    // Sets a tile's brightness trait for a brightness class (brackets and meters dim, meter backgrounds and ghost
    // meters darker; anything else full).
    static void SetBrightness(Tile* apTile, HUD_BRIGHTNESS aeBrightness);
    // Starts or fades the radiation sound and updates the meter for the ingested and the
    // environmental radiation deltas.
    static void SetRadiationLevel(float afDrinkinRads, float afStandinRads);
    // Builds the HUD menu from its XML (deleting a HUD menu that already exists), lays out and creates its tiles,
    // the gun scope model and the breath meter animation; the menu tile, or null when the XML is no HUD menu.
    static Tile* Create();
    // Switches the HUD to a HUD_MENU_MODE (HMM_CHANGED works it out from the top menu and the player) and shows
    // the HUD elements the mode and the user's settings allow.
    static void SetMenuMode(HUD_MENU_MODE aeMenuMode);
    // Shows or hides the gun scope model when it has a first child.
    static void SetGunScopeVisible(bool abVisibleFlag);
    // Clears the queued messages and subtitles, then sets the alpha of the HUD's tiles from afHUDOpacity (the
    // region, radiation, sneak, message, XP bracket, level-up and crippled tiles keep theirs).
    static void ResetHUDOpacity(float afHUDOpacity);
    // Starts tracking an enemy for the enemy health meter (in VATS playback, the VATS target); a dead actor, or the
    // health bar setting, leaves the HUD alone.
    static void SetEnemyActor(Actor* apActor);
    // Draws the gun scope model with the scope camera while the scope is up.
    static void Draw3DElements();
    // Shows the enemy health meter for the crosshair reference.
    static void HandleHealthbarForCrosshairActor(TESObjectREFR* apRef, bool abShow);
    // Calls HandleHealthbarForCrosshairActor(apRef, false) once the player has 3D.
    static void HandleHealthbarForCrosshairActor_(TESObjectREFR* apRef);
    // Moves the compass and its icons.
    static void UpdateCompass(Tile* apCompass, Tile* apGeneralIcons, Tile* apMarkerIcons, Tile* apNPCIcons);
    // Sets a meter tile to afMeterPercent.
    static void
    UpdateMeter(Tile* apTile, float afMeterPercent, HUD_JUSTIFY aeJustify, int32_t aiMinTicks, int32_t iNumPixelShift);
    // Sets a meter tile to afMeterPercent and its ghost meter tile to afGhostMeterPercent.
    static void UpdateMeter(
        Tile* apMeterTile,
        Tile* apGhostMeterTile,
        float afMeterPercent,
        float afGhostMeterPercent,
        HUD_JUSTIFY aeJustify,
        int32_t aiMinTicks
    );
    // Shows aRegionData in the region text and holds it, while the location text is faded out.
    static void SetRegionText(const char* aRegionData);
    // The compass heading in degrees from aStart to aDest.
    static float HeadingBetweenLocations(const NiPoint3& aStart, const NiPoint3& aDest);
    // Shows the reminder for apQuest's objectives.
    static void DisplayQuestReminder(TESQuest* apQuest);
    // Queues apQuest's name for the HUD as added, completed or failed, and moves the player's active quest to
    // match.
    static void ShowQuestName(TESQuest* apQuest);
    // Queues the HUD's quest update text; abCompleted and abFailed pick the queue. apText is the result of
    // BGSQuestObjective::GetDisplayText.
    static void ShowQuestObjective(const char* apText, bool abCompleted, bool abFailed);
    static void ShowEnemyDDTIcon(bool abArmorPiercing);
    static void SetHUDShake(float a1);
    static void ClearXPMeterFinishedMoving();
    // Whether the XP meter has stopped moving; false without the menu.
    static bool IsXPMeterFinishedMoving();
    // Loads apModelFilename's model as the gun scope model.
    static void InitGunScope(TESModel* apModelFilename);
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    // Stores abValue in the open HUD's byte at +0x205.
    static void ForceObjectiveReminders(bool abValue);
    static void ShowDDTIcon(bool abArmorPiercing);
    // Shows the travel marker's location name when it comes in range. UpdateCompass passes its HUDTravelLocData local.
    static void SetLocationText(const HUDTravelLocData& aLocationData);
    // aiAddedXP is an integer: the body stores it into XPUpdate::iAddedXP.
    static void QueueXPMessage(int32_t aiAddedXP);
    static void SetVisibilityFlag(int32_t a1, int32_t a2);
    static void Revert();
    static double GetCompassAngle(NiPoint3 a1, NiPoint3 a2);

    // The tiles by HUD tile id (slot 0 is not used).
    Tile* pTiles[87];
    Tile* pActive;
    float fSeenLevel;
    // The queued HUD messages.
    BSSimpleList<MessageData*> xMessages;
    // The HUD timer time the shown message's timer started; DoIdle starts it when it is 0.
    uint32_t uiMessageStartTime;
    // The queued subtitles.
    BSSimpleArray<SubtitleData*> xSubtitles;
    // The subtitle being shown.
    SubtitleData* pCurrSubtitle;
    // The Geiger counter's sound, playing while the radiation meter is shown.
    BSSoundHandle GeigerSound;
    // The reference the crosshair is over.
    TESObjectREFR* pLastObject;
    // The HUD elements the current mode shows (bit n for tiles HMMT_AP + n), masked by the user's settings.
    int32_t iHUDVisibilityMenuModeFlags;
    // The HUD elements the user's settings allow.
    int32_t iHUDVisibilityUserSettings;
    HUD_MENU_MODE eMenuMode;
    HotKeysWheel HKWheel;
    bool bScopeVisible;
    uint8_t padding1FD[0x3];
    // The breath meter's "Forward" animation.
    NiControllerSequence* pBreathMeterAnim;
    bool bXPMeterFinishedMoving;
    // Set while the quest reminder repeats on the reminder timer.
    bool bForceObjectiveReminders;
    uint8_t padding206[2];
    uint32_t uiKillRegionDisplay;
    // The player's quest targets (PlayerCharacter::GetQuestTargets).
    BSSimpleList<TESQuestTarget*>* pQuestList;
    // The compass image's width, the shown part's width and the half angle the compass shows.
    int32_t iCompassImageWidth;
    int32_t iCompassDisplayWidth;
    int32_t iCompassHalfAngle;
    Actor* pCurrentEnemy;
    bool bWasSneaking;
    uint8_t padding221[3];
    // The HUD's timer, and its time when the enemy health meter was last shown.
    BSTimer HudTimer;
    uint32_t iEnemyHealthTimer;
    // The global timer's time of the last quest reminder (0 before the first one).
    uint32_t uiQuestReminderTimer;
    // The global timer's time the damage threshold icon and the enemy's went up, or 0xFFFFFFFF while hidden.
    uint32_t uiDDTdisplayTimer;
    uint32_t uiEnemyDDTdisplayTimer;
    // Set when a mode that keeps quest updates up is left.
    bool bKillQuestUpdates;
    uint8_t padding255[0x3];
    // The quest objective texts waiting to be shown.
    BSSimpleList<QueuedQuestText*> xQuestObjectives;
    bool bQuestNameShowing;
    bool bObjectiveNameShowing;
    uint8_t padding262[0x2];
    // The experience-meter updates waiting to be shown (QueueXPMessage adds them).
    BSSimpleList<XPUpdate*> xXPUpdates;
    // The radiation-meter updates waiting to be shown.
    BSSimpleList<RadiationUpdate*> xRadiationUpdates;
    // How far UpdateShake shakes the HUD this frame; it clears it after.
    float fShake;

    // The detection level the HUD reticle shows.
    static inline int32_t iDetectionLevel;
    static inline HUDMainMenu* pme;
    // The gun scope model.
    static NiPointer<NiNode> spScopeModel;
    // The gun scope's camera.
    static NiPointer<NiCamera> spScopeCamera;
    // The quest update queue (an empty object).
    static inline QuestUpdateManager* pQuestUpdateManager;
    // Set while the weapon condition tiles are shifted over for the ammo type text.
    static inline bool bAmmoShifted;
    // Guards the quest name queue.
    static BSCriticalSection HUDShowQuestNameCritSect;
    // The HUD's third critical section: nothing uses it beyond its initialiser and atexit thunk.
    static BSCriticalSection HUDCritSect;
    // Guards the queued quest objective texts (SetQuestUpdateText).
    static BSCriticalSection HUDShowQuestObjectivsCritSect;
    // SetHUDShake's scale ("fHUDShakeMult:Interface").
    static SettingT<INISettingCollection> fHUDShakeMult;
};

// A queued HUD message.
struct HUDMainMenu::MessageData {
    char strText[260];
    char IconFilename[260];
    char SoundName[260];
    // How long the message stays up, in seconds.
    float fDurationSecs;
};

// A queued subtitle: the text, when it was queued, how long it stays up, where the speaker is and its priority.
struct HUDMainMenu::SubtitleData {
    SubtitleData();

    char strText[260];
    // The HUD timer time the subtitle was queued.
    uint32_t uiSubtitleStartTime;
    // How long the subtitle stays up, in seconds.
    float fDurationSecs;
    NiPoint3 SpeakerLoc;
    // Compared signed and against 0x7FFFFFFF.
    int32_t iPriority;
};

// A queued quest objective text: the text and the two flags SetQuestUpdateText stores after it (its second and third
// arguments).
struct HUDMainMenu::QueuedQuestText {
    char msg[260];
    bool byte104;
    bool byte105;
};

// A queued experience-meter update (queued by QueueXPMessage): the player's level and experience when it was queued and
// the experience added.
struct HUDMainMenu::XPUpdate {
    int32_t iPlayerLevel;
    int32_t iPlayerXP;
    int32_t iAddedXP;
};

// A queued radiation-meter update: the player's radiation and the radiation added.
struct HUDMainMenu::RadiationUpdate {
    int32_t iPlayerRads;
    float fRadsAdded;
};

// The region text to show: the region, its display text and whether it becomes the last region shown.
struct HUDMainMenu::HUDRegionData {
    TESRegion* pRegion;
    char strDisplayText[260];
    bool bUpdateLastRegion;
};

// The compass's travel location: the map marker reference, whether it is in range and whether the last location is
// shown again.
struct HUDMainMenu::HUDTravelLocData {
    TESObjectREFR* pMarker;
    bool bInRange;
    bool bForceLastLocation;
};

inline int32_t g_XPBarState = 1;

// The crippled-limb image names ("head", "torso", "left_arm",
// "right_arm", "left_leg", "right_leg"), indexed by the limb's condition actor value minus PERCEPTION_CONDITION.
using HUDCrippledLimbNameArray = const char* [6];
inline HUDCrippledLimbNameArray HUDCrippledLimbNames{"head", "torso", "left_arm", "right_arm", "left_leg", "right_leg"};

// The game settings HUDMainMenu.cpp defines.
extern SettingT<INISettingCollection> fXenonMenuStickSpeedPlayerRotMod;
inline char g_bIsEnemyWithinCompassRange;
inline char bAlphaGoingDown;
inline int32_t iLastHardcoreFadeTime;
inline float fCurrAlphaHardcore;
inline int32_t xp_nextActorLevel;
inline int32_t xp_currentActorLevel;

// The lock level names by lock-pick difficulty (0 very easy to 5 requires a key).
using pLockLevelNames_t = Setting* [6];
extern pLockLevelNames_t pLockLevelNames;

extern SettingT<GameSettingCollection> gs_sQuestCompletedText;

extern SettingT<GameSettingCollection> gs_sQuestAddedText;

// The action prompts by GetRefPromptType's prompt type (1 take, 7 talk).
using psTargetTypes_t = Setting* [20];
extern psTargetTypes_t psTargetTypes;

// The keyboard key names by key code.
using psKeyboard_t = Setting* [238];
extern psKeyboard_t psKeyboard;

extern SettingT<INIPrefSettingCollection> gs_bDisableDynamicCrosshair_GamePlay;

extern SettingT<INIPrefSettingCollection> gs_bCrossHair_GamePlay;

// The HUD timer time of the last quest name update.
inline uint32_t uiMessageDelayTime;

// Set while a hostile actor is under the crosshair from a long-distance pick.
inline bool bLongDistancePick;
inline bool bHostileUnderCrosshair;

extern SettingT<INIPrefSettingCollection> fHudOpacity;

// The HUD's current opacity, applied to the info lines that fade with it.
extern float fCurrentHUDOpacity;
using RadiationTextBuffer = char[260];
inline RadiationTextBuffer g_sRadiationTextBuffer{};
