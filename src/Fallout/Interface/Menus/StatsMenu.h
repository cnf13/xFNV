#pragma once

#include "BSCore/MemoryManager.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/MiscStats.h"
#include "Fallout/Interface/ListBox.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class ActiveEffect;
class AlchemyItem;
class EffectSetting;
class ItemChange;
struct PerkRankData;
class TESReputation;

// The pipboy STAT page.
class StatsMenu : public Menu {
public:
    // The slots of the tile table (pTiles), as the menu's XML ids number them.
    enum TileIndex : int32_t {
        eStatusButton = 0,
        eSpecialButton = 1,
        eSkillsButton = 2,
        ePerksButton = 3,
        eGeneralButton = 4,
        eStatusRect = 5,
        eSpecialRect = 6,
        eSkillsRect = 7,
        eGeneralRect = 8,
        eHealingMode = 9,
        eStimpakButton = 10,
        eRadAwayButton = 11,
        eHeadImage = 12,
        eHeadPct = 13,
        eTorsoImage = 14,
        eTorsoPct = 15,
        eLeftArmImage = 16,
        eLeftArmPct = 17,
        eRightArmImage = 18,
        eRightArmPct = 19,
        eLeftLegImage = 20,
        eLeftLegPct = 21,
        eRightLegImage = 22,
        eRightLegPct = 23,
        eFaceImage = 24,
        eItemIcon = 25,
        eItemDesc = 26,
        ePlayerName = 27,
        eConditionButton = 28,
        eRadsButton = 29,
        eEffectsButton = 30,
        eRadXButton = 31,
        eRadEffects = 32,
        eStatusEffects = 33,
        ePerksRect = 34,
        eKarmaAlignment = 35,
        eKarmaImage = 36,
        ePlayerTitle = 37,
        eRadResistLevel = 38,
        eTitle = 39,
        eLevelInfo = 40,
        eHPInfo = 41,
        eAPInfo = 42,
        eXPInfo = 43,
        eRadResistLabel = 44,
        eNoEffects = 45,
        eReputationRect = 46,
        eReputationIcon = 47,
        eReputationTitle = 48,
        eGenRepToggleButton = 49,
        eGenRepRect = 50,
        eReputationName = 51,
        eSkillBadgeImage = 52,
        eH20Button = 53,
        eH20Effects = 54,
        eFODButton = 55,
        eFODEffects = 56,
        eSLPButton = 57,
        eSLPEffects = 58,
        eDrBagButton = 59,
        NUM_TILES = 60,
    };

    enum HealingOptionID : int32_t {
        SM_HO_STIMPAK = 0,
        SM_HO_RADX = 1,
        SM_HO_RADAWAY = 2,
        SM_HO_DRBAG = 3,
        SM_HO_COUNT = 4,
    };

    // The pages of the menu (its user0 trait).
    enum Page : int32_t {
        SM_STATUS = 0,
        SM_FIRST_PAGE = 0,
        SM_SPECIAL = 1,
        SM_SKILLS = 2,
        SM_PERKS = 3,
        SM_GENERAL = 4,
        SM_LAST_PAGE = 4,
        SM_PAGE_COUNT = 5,
    };

    // The pages of the status tab (the status tile's user0 trait); the last three exist only in hardcore mode.
    enum StatusPage : int32_t {
        SM_STATUS_CONDITION = 0,
        SM_FIRST_STATUS_PAGE = 0,
        SM_STATUS_RADS = 1,
        SM_STATUS_EFFECTS = 2,
        SM_LAST_STATUS_PAGE = 2,
        SM_STATUS_H20 = 3,
        SM_STATUS_FOD = 4,
        SM_STATUS_SLP = 5,
        SM_LAST_STATUS_PAGE_HARDCORE = 5,
        SM_STATUS_PAGE_COUNT = 6,
    };

    struct StatusEffect;

    // A healing item button: the item, its tile, the inventory entry used and the check for a valid target.
    struct HealingOption {
        HealingOption();
        ~HealingOption();

        // The constructor's callback: whether aHO has an inventory item.
        static bool DefaultTargetConditionCallback(HealingOption& aHO);

        AlchemyItem* pAlchemyItem;
        Tile* pTile;
        ItemChange* pInventoryItem;
        bool (*pfTargetConditionCallback)(HealingOption& aHO);
    };

    // The effect values an active effect changes, one entry per actor value.
    class StatusDataList : public BSSimpleList<EffectItemData*> {
    public:
        StatusDataList();
        virtual ~StatusDataList();

        // Adds the active effect's actor value change.
        void Register(ActiveEffect* apActiveEffect);
        // The entry whose actor value is aiActorValue, or null.
        EffectItemData* GetEntryByAVCode(int32_t aiActorValue);
        int32_t Flush();
        void Register(EffectItem* arg0);

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
    };

    // The effects of a status effect that are shown by name.
    class NamedEffectsList : public BSSimpleList<EffectSetting*> {
    public:
        NamedEffectsList();
        virtual ~NamedEffectsList();

        void Flush();
        void Register(ActiveEffect* a1);

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
    };

    // The status effects shown in the effects list box, one per source form.
    class StatusEffectList : public BSSimpleList<StatusEffect*> {
    public:
        StatusEffectList();
        virtual ~StatusEffectList();

        // Deletes every status effect.
        void Flush();
        // Adds the active effect to the status effect of its source, creating it when needed.
        void Register(ActiveEffect* apActiveEffect);
        // The status effect whose casting type is aiID, or null.
        StatusEffect* Find(uint32_t aiID);

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }
    };

    // The effects one form (a spell, chem or sickness) has on the player.
    struct StatusEffect {
        // Constructs a status effect for the form in apMemory; returns apMemory.
        StatusEffect(TESForm* apEffectForm);
        ~StatusEffect();

        static void operator delete(void* apMemory) {
            ourFree(apMemory);
        }

        TESForm* pEffectForm;
        StatusDataList xStatsEffected;
        NamedEffectsList xNamedEffects;
    };

    // The limb selector of the healing mode: the tiles that flash for each body part, and for each body part the
    // one each direction leads to (a byte per Direction).
    class BodyPartController {
    public:
        enum BodyPartIndex : int32_t {
            SM_HEAD = 0,
            SM_RIGHTARM = 1,
            SM_TORSO = 2,
            SM_LEFTARM = 3,
            SM_RIGHTLEG = 4,
            SM_LEFTLEG = 5,
            SM_BODYPART_COUNT = 6,
        };

        enum Direction : int32_t {
            DIR_UP = 0,
            DIR_DOWN = 1,
            DIR_LEFT = 2,
            DIR_RIGHT = 3,
        };

        BodyPartController();
        // Empties each body part's list of flashing tiles.
        ~BodyPartController();

        // Returns the body part whose flashing tiles hold apTile, or 6 if none; searches listFlashingTiles. DoClick,
        // DoEnter and DoLeave pass their Tile* apTarget.
        BodyPartIndex Find(Tile* apTile);
        // Adds a tile to the tiles that flash for the body part.
        void AddFlash(BodyPartIndex aeBodyPart, Tile* apTile);
        // Makes direction aeDir lead from body part aeFrom to aeTo.
        void LinkBodyParts(BodyPartIndex aeFrom, Direction aeDir, BodyPartIndex aeTo);
        // Sets the flashing of the current body part's tiles on (aiOn non-zero) or off.
        void SetFlash(uint32_t aiOn);
        void SetCurrentBodyPart(StatsMenu::BodyPartController::BodyPartIndex aeBodyPart, bool abFlash);
        StatsMenu::BodyPartController::BodyPartIndex
        GetLink(StatsMenu::BodyPartController::BodyPartIndex aeFrom, StatsMenu::BodyPartController::Direction aeDir);

        // The controller, created on the first call.
        static BodyPartController* GetInstance();
        static ActorValue::Index Convert(StatsMenu::BodyPartController::BodyPartIndex aeBodyPart);
        static void operator delete(void* apMemory);
        static void* DeleteInstance();

        BodyPartIndex eCurrentBodyPart;
        BSSimpleList<Tile*> listFlashingTiles[SM_BODYPART_COUNT];
        // Not pointers despite the name: per body part, the body part in each direction, one byte per direction (the
        // constructor fills them with 0xFF).
        uint32_t pBodyPartMapA[SM_BODYPART_COUNT];

        // The controller, or null before the first GetInstance.
        static inline BodyPartController* pInstance;
    };

    // One value per page that DoEnter hands to MoveKnob.
    using PageDataArray = uint32_t[SM_PAGE_COUNT];

    StatsMenu();
    ~StatsMenu() override;

    // Remembers a tile the menu's XML created; ids outside the tile table are ignored.
    void AttachTileByID(int32_t aiID, Tile* apTile) override;
    void DoClick(int32_t aiID, Tile* apTarget) override;
    void DoEnter(int32_t aiID, Tile* apTarget) override;
    void DoLeave(int32_t aiID, Tile* apTarget) override;
    void DoIdle() override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t aiButton, float afValue) override;
    bool DoGamepadUpEvent(int32_t aiButton, float afValue) override;
    void DoControllerSwitch(bool abHas360Controller) override;

    // With a 360 controller, refreshes the healing mode tile's button hint and label and the tab buttons' enabled
    // state for the limb mode; then sets the limb selector's flashing to the limb mode.
    void UpdateHealingMode();

    // True when every tile of the menu has been attached.
    bool IsWellFormedMenu();
    // The page chosen on the menu tile (its user0 trait, truncated).
    StatsMenu::Page GetCurrentPage();
    // The page chosen on the status tile (its user0 trait, truncated).
    StatsMenu::StatusPage GetStatusPage();
    // The tile trait that shows an actor value (health, action points or XP); INT32_MIN for other values.
    int32_t GetTraitForValue(int32_t aeActorValue);
    // Refreshes the radiation meter.
    void UpdateRadLevel();
    // Refreshes the tiles that show one actor value of the player.
    void UpdateActorValue(ActorValue::Index aeWhich);
    // Refreshes every row of the general (misc stats) list box.
    void UpdateAllMiscStats();
    // Rebuilds the radiation, dehydration, hunger and sleep effect lists and the status effects list box.
    void UpdateActiveEffects();
    // Refreshes the healing buttons for one healing option.
    void UpdateHealingOptions(HealingOptionID aeID);
    // The number of the face image that shows the player's health.
    int32_t GetFaceFileNumber();
    // Refreshes every actor value the menu shows, and the SPECIAL and skill lists.
    void UpdateAllActorValues();
    // Shows a page (SM_FIRST_PAGE..SM_LAST_PAGE; other values are ignored) with a burst of the pip-boy screen.
    void SetCurrentPage(StatsMenu::Page aePage);
    // Shows the page of a tab.
    void SetCurrentTab(char acTab);
    // Refreshes the dehydration meter.
    void UpdateH20Level();
    // Refreshes the hunger meter.
    void UpdateFODLevel();
    // Refreshes the sleep deprivation meter.
    void UpdateSLPLevel();
    // Switches the limb (healing) mode on or off.
    void ToggleHealingMode();
    // Shows a page of the status tab; false for a page out of range or the page already shown.
    bool SetStatusPage(int32_t aiPage);
    // Shows an actor value's icon and description.
    void SetIconAndDescription(ActorValue::Index aeIndex);
    // Shows a perk's icon and description.
    void SetIconAndDescription(PerkRankData* apPerkRankData);
    // Shows a reputation's icon and description.
    void SetReputation(TESReputation* pRep);
    // Called with an entered list tile and the page's slot of aiPageListData.
    uint32_t MoveKnob(Tile* apTarget, uint32_t& aiLastIndex);
    void UpdateMiscStat(MiscStatManager::MiscStatID aeID);
    void UpdateAll();
    void UpdateReputations();
    void TargetMostDamagedBodyPart();
    bool GetPageFor(StatsMenu::TileIndex aeIndex, StatsMenu::Page& aePage);
    bool GetStatusPageFor(StatsMenu::TileIndex aeIndex, StatsMenu::StatusPage& aePage);
    void ToggleGenRepMode();

    // Whether a body part is being targeted.
    static bool IsTargetingBodyPart();
    // The stats menu's class id.
    static int32_t GetClassStatic();
    // Builds the stats menu from its XML and fills its lists and labels; the menu's tile, or null when the menu
    // cannot be built. An open stats menu is closed first.
    static Tile* Create();
    // Rebuilds the perks list of the open stats menu.
    static void UpdatePerks();
    // Shows page acPage of the open stats menu.
    static void SetCurrentStatsPage(uint8_t acPage);
    // Shows the player's karma title on the open stats menu.
    static void UpdatePlayerTitle();
    // Refreshes a SPECIAL list entry's tile; the SPECIAL list's traversal callback.
    static void SPECIALUpdateFn(Tile* apTile, ActorValue::Index aeIndex);
    // Refreshes a skill list entry's tile; the skill list's traversal callback.
    static void SkillsUpdateFn(Tile* apTile, ActorValue::Index aeIndex);
    // Fades the tile's alpha from 255 to 0 over a second (fade type 1) while aiOn is set; otherwise stops the fade
    // and puts the alpha back to 255. Always true.
    static bool FadeControlCallback(Tile* apTile, uint32_t aiOn);
    // Shows the player's name, the level label and the level on the open stats menu.
    static void UpdatePlayerName();
    // Whether a stimpak can heal the targeted limb, or the health without one (a target, the healing option).
    static bool StimpakTargetConditionCallback(HealingOption& aHO);
    // Whether RadAway has radiation to remove.
    static bool RadAwayTargetConditionCallback(HealingOption& aHO);
    // Turns the pip-boy knob to the current page.
    static void SetKnobPosition(bool abPlaySounds);
    // The actor value of the targeted limb, or -1 when no limb is targeted.
    static ActorValue::Index GetTargetedBodyPart();
    static float GetRadMeterMax();
    static uint8_t GetCurrentStatsPage();
    static void Close();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }

protected:
    static void MiscStatsUpdateFn(Tile* apTile, MiscStatManager::MiscStatID aeID);

public:
    static void ClearPerks();
    static int32_t StatusEffectSortFn(StatsMenu::StatusEffect* apItem1, StatsMenu::StatusEffect* apItem2);
    static int32_t PerkListSortFn(ListBoxItem<PerkRankData*>* apItem1, ListBoxItem<PerkRankData*>* apItem2);
    static int32_t ReputationListSortFn(ListBoxItem<TESReputation*>* apItem1, ListBoxItem<TESReputation*>* apItem2);

    HealingOption pHealingOptionsA[SM_HO_COUNT];
    NamedEffectsList xNamedEffects;
    StatusEffect* pRadiationEffect;
    StatusEffect* pDehydrationEffect;
    StatusEffect* pHungerEffect;
    StatusEffect* pSleepEffect;
    StatusEffectList xStatusEffects;
    Tile* pTiles[60];
    ListBox<ActorValue::Index> lbSpecial;
    ListBox<ActorValue::Index> lbSkills;
    ListBox<PerkRankData*> lbPerks;
    ListBox<MiscStatManager::MiscStatID> lbGeneral;
    ListBox<StatusEffect*> lbStatusEffects;
    ListBox<TESReputation*> lbReputation;
    bool bLimbMode;
    bool bActiveEffectsUpdatedThisFrame;

    static inline PageDataArray aiPageListData{};
    // The open stats menu, or null.
    static inline StatsMenu* pMe;
};

// Steps the page to the next one; returns arPage.
StatsMenu::Page& operator++(StatsMenu::Page& arPage);
// Steps the page to the previous one; returns aePage.
StatsMenu::Page& operator--(StatsMenu::Page& aePage);
// Steps the body part to the next one; returns arIndex.
StatsMenu::BodyPartController::BodyPartIndex& operator++(StatsMenu::BodyPartController::BodyPartIndex& arIndex);
int32_t StatsMenu_sortingFunction(EffectSetting* a1, EffectSetting* a2);
// The Dr. Bag healing option's target condition callback, typed as its siblings and the option's
// pfTargetConditionCallback: it is passed the option and reads its members.
bool DrBagTargetConditionCallback(StatsMenu::HealingOption& aHO);

// Holds 0 or 1; ToggleGenRepMode stores it.
inline uint8_t g_StatsMenuIsShowGeneralNotReputation = 1;
inline int32_t StatsMenuStatusEffectListBox_IsEnabled_iUT_Enabled;
inline int32_t StatsMenuStatusEffectListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t StatsMenuStatusEffectListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t inext;
inline int32_t StatsMenuStatusEffectListBox_NumUnfiltered_iUT_num_filtered;

// The body-part condition images, "head" to "right_leg": one per condition from PERCEPTION_CONDITION to
// RIGHT_MOBILITY_CONDITION.
using BodyPartImageNameTable = const char* [6];
inline BodyPartImageNameTable pBodyPartImageNames{"head", "torso", "left_arm", "right_arm", "left_leg", "right_leg"};
