#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/ListBox.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

class BGSNote;
class BGSQuestObjective;
class TeleportPath;
class TESChallenge;
class TESObjectCELL;
class TESObjectREFR;
class TESQuest;
class TESQuestTarget;
class TESWorldSpace;
class TileMenu;
class TileText;

void MapMenu_RequestPlayerPositionAsSelectedMarker();

// The map menu: local and world map tabs, radio stations and challenges.
class MapMenu : public Menu {
public:
    // The menu's tiles (pTiles) and, from TI_QUEST on, the ids of the tiles it builds; the tabs' ids are the pages.
    enum TILE_INDEX : int32_t {
        TI_PLAYER_LOCATION = 0,
        TI_PLAYER_TIME = 1,
        TI_LOCAL_MAP = 2,
        TI_LOCAL_MAP_ARROW = 3,
        TI_WORLD_MAP = 4,
        TI_WORLD_MAP_ARROW = 5,
        TI_MAP_HIGHLIGHT = 6,
        TI_QUESTS_LIST = 7,
        TI_NOTES_LIST = 8,
        TI_NOTE_HIGHLIGHT = 9,
        TI_RADIO_STATIONS_LIST = 10,
        TI_BUTTON_A = 11,
        TI_BUTTON_X = 12,
        TI_DATA_RECT = 13,
        TI_NOTE_TEXT_RECT = 14,
        TI_QUEST_OBJ_LIST = 15,
        TI_WAVEFORM = 16,
        TI_TABLINE = 17,
        TI_BUTTON_Y = 18,
        TI_CHALLENGE_LIST = 19,
        TI_CHALLENGE_HIGHLIGHT = 20,
        TI_CHALLENGE_TYPE = 21,
        NUM_TILES = 22,
        TI_QUEST = 23,
        TI_NOTE = 24,
        TI_RADIO_STATION = 25,
        TI_MAP_MARKER = 26,
        TI_QUEST_MARKER = 27,
        TI_PLAYER_MARKER = 28,
        TI_QUEST_WAYPOINT = 29,
        TI_PLAYER_MARKER_WAYPOINT = 30,
        TI_DEBUG_WAYPOINT = 31,
        TI_TAB_LOCALMAP = 32,
        TI_TAB_WORLDMAP = 33,
        TI_TAB_QUESTS = 34,
        TI_TAB_NOTES = 35,
        TI_TAB_RADIO = 36,
    };

    MapMenu();
    ~MapMenu() override;

    void AttachTileByID(int32_t aiID, Tile* apTile) override;
    void DoDownClick(int32_t aiID, Tile* apTarget) override;
    void DoClick(int32_t aiID, Tile* apTarget) override;
    void DoEnter(int32_t aiID, Tile* apTarget) override;
    void DoLeave(int32_t aiID, Tile* apTarget) override;
    void DoWheelMove(int32_t aiID, Tile* apTarget) override;
    void DoIdle() override;
    int32_t GetClass() override;
    bool DoGamepad(int32_t aiButton, float afValue) override;
    void DoControllerSwitch(bool abHas360Controller) override;

    void ZoomMap(bool abZoomIn, float afZoomDelta);
    // Clicks the tab's tile through DoClick.
    void SetPageNumber(int32_t aiTab);
    // True while every tile of the menu is attached.
    bool AreAllTilesNonNull();
    // Restarts the radio-station waveform animation.
    void ResetWaveform();
    // Highlights the map marker under the highlight cursor.
    void HighlightCenterMarker();
    // Advances the playing holotape sounds and subtitles.
    void UpdatePlayingSounds();
    // Rebuilds the page shown.
    void UpdateCurrentPage();
    // Animates the radio-station waveform.
    void UpdateWaveform();
    // Converts a world position to its position on the shown world map page.
    NiPoint3 WorldToMapCoord(NiPoint3 aWorldCoord, TESWorldSpace* apWorldspace);
    // Marks the path from apStartRef to apTargetRef on the world map: a waypoint at each end and at every exterior
    // door of apPathToTarget between them, joined by path line tiles; every tile gets the id aiID.
    void CreateWaypointPath(
        TESObjectREFR* apStartRef,
        TESObjectREFR* apTargetRef,
        TeleportPath* apPathToTarget,
        int32_t aiID
    );
    // The reference the world map tracks for apRef along apPathToRef.
    TESObjectREFR* GetWorldMapTrackingRef(TESObjectREFR* apRef, TeleportPath* apPathToRef);
    // Replaces the quest marker and quest waypoint tiles of the map shown with the player's current quest targets.
    void AddQuestMarkers();
    void UpdateLocalMap();
    void AddMapIcons();
    void AddCompanionMarkers();
    void CreatePlayerMarker();
    void DeletePlayerMarker();
    TESWorldSpace* CalculatePlayerMarkerSpace(NiPoint3 aWorldPos);
    void StopNoteSound();
    void LoadChallenge(TESChallenge* apChallenge);
    void LoadNote(BGSNote* apNote);
    void UpdateNotes();
    void ToggleNotesChallengeMode(bool bChange);
    void UpdateChallenges();
    void UpdateQuests();
    void UpdateRadioList();
    NiPoint3* MapToWorldCoord(NiPoint3* apResult, NiPoint3 aMapCoord, TESWorldSpace* apWorldspace);
    void UpdateWorldMap();
    void Zoom_RefreshCompanions();

    // Resets the next local-map grid index.
    static void ClearCount128();
    // SetCurrentTab on the open map menu, if any.
    static void SetCurrentMapPage(uint8_t aucTab);
    // True when apObjective's quest is not the quest the map menu shows.
    static bool IsQuestMapMenuSelectedQuest(BGSQuestObjective* apObjective);
    static bool ShouldRenderLocalMap();
    static void RenderLocalMapGrid(bool abSaveToDisk);
    // Empties the local and world maps, their marker lists and the waveform's texture.
    static void ClearMapMemory();
    // Clears the selected note when it is apNote; a null note is ignored.
    static void NoteRemoved(BGSNote* apNote);
    // Releases the world map's texture unless the world map page is up.
    static void ClearWorldMapTexture();
    // Loads map_menu.xml and sets up the menu; null when the file does not build a well-formed map menu.
    static Tile* Create();
    static int32_t GetClassStatic();
    // Refreshes the time, location and button labels, then the current page.
    static void UpdateMenu();
    // Records the worldspace of the last exterior door, walked up to its root worldspace.
    static void SetRootMapSpace(TESWorldSpace* apNewSpace);
    static void RenderNextGridToDisk();
    static void ClearSelectedNote();
    static void SetKnobPosition(bool abPlaySounds);
    // Clears a radio list tile's selection; the list's Traverse passes the station too.
    static void UnselectFunc(Tile* apTile, TESObjectREFR* apStation);
    // Whether the radio list leaves pcurrStation out: no station, a disabled one, or one the Pip-Boy does not list.
    static bool RadioFilterFunc(TESObjectREFR* pcurrStation);
    // Orders the quest objectives list: full-alpha (_TextAlpha 255) items before dimmed (128) ones, then by list
    // index. UpdateQuests sorts both quest lists with this one body.
    static int32_t
    QuestObjectiveSortFunc(ListBoxItem<BGSQuestObjective*>* apItem1, ListBoxItem<BGSQuestObjective*>* apItem2);
    static int32_t QuestSortFunc(ListBoxItem<TESQuest*>* apItem1, ListBoxItem<TESQuest*>* apItem2);
    static void SetLocalMapMagnification(float afMagnification);
    static void SetWorldMapMagnification(float afMagnification);
    // Places the player's marker on the map.
    static void SetPlayerMarker();
    // The shown page as an index from the local map tab, or 4 (the radio page) without the menu.
    static uint8_t GetCurrentMapPage();
    static void operator delete(void* apMemory) {
        ourFree(apMemory);
    }
    static int32_t RadioSortFunc(ListBoxItem<TESObjectREFR*>* apItem1, ListBoxItem<TESObjectREFR*>* apItem2);
    static bool NotesFilterFunc(BGSNote* pcurrNote);
    static void ResetCurrentNoteIfEqualTo(BGSNote* a1);
    static void FastTravelCallback();
    static void SetPlayerMarkerCallback();
    static void ClearAudioLogSubtites();
    static void FindQuestObjectiveTargetFunc(Tile* apTile, BGSQuestObjective* apObjective);
    static bool ChallengeFilterFunc(TESChallenge* pcurrChallenge);
    static int32_t ChallengeSortFunc(ListBoxItem<TESChallenge*>* apItem1, ListBoxItem<TESChallenge*>* apItem2);
    static float GetLocalMapMagnification();
    static float GetWorldMapMagnification();

    // The menu's tiles, indexed by TILE_INDEX.
    Tile* pTiles[21];
    // The challenge-type headline (TI_CHALLENGE_TYPE); the 22nd pointer AreAllTilesNonNull checks.
    TileText* pChallengeType;
    // The page shown: one of the TI_TAB_ ids.
    int32_t iCurrentPage;
    Tile* pMapTile;
    // The time the radio waveform animation started.
    float fWaveformAnimationStartTime;
    bool bShowingAllNotes;
    BGSNote* pSelectedNote;
    // The time the selected note was entered; zero once it has been marked read.
    uint32_t uiNoteEnterStartTime;
    BSSimpleList<BSSoundHandle> SoundsToPlay;
    BSSimpleArray<BSStringT<char>> SubtitlesToShow;
    BSSimpleList<BSSoundHandle>* pCurrentSound;
    bool bSoundsLoading;
    float fSoundDurationMillis;
    int32_t iSoundPlayStartTime;
    float fOldPipboyRadioVolume;
    TESQuest* pDisplayedQuest;
    TESQuestTarget* pDisplayedQuestTarget;
    Tile* pHighlightedMapMarker;
    BSSimpleList<TESObjectREFR*> WorldMapMarkerList;
    BSSimpleList<TESObjectREFR*> LocalMapMarkerList;
    NiPoint2 MapNWCorner;
    NiPoint2 MapSECorner;
    TESForm* PlayerMarkerSpace;
    NiPoint3 PlayerMarkerPos;
    TESForm* pMapSpace;
    TESWorldSpace* pRootMapSpace;
    bool bRootSpaceChanged;
    TESObjectREFR* pMapPlayerRef;
    TESObjectREFR* pFastTravelMarker;
    TESObjectCELL* pAdjustNorthRotationCell;
    NiPoint2 WorldMapDragStart;
    uint32_t uiNextLocalMapGrid;
    bool bFogOfWarEnabled;
    ListBox<TESQuest*> lbQuestsList;
    ListBox<BGSNote*> lbNotesList;
    ListBox<TESObjectREFR*> lbRadioStationsList;
    ListBox<BGSQuestObjective*> lbQuestObjectivesList;
    ListBox<TESChallenge*> lbChallengeList;
    BSSimpleArray<Tile*> debugWaypoints;

    static inline int8_t cChallengeTypeDisplayed;
    // The menu while it exists.
    static inline MapMenu* pMe;
    // Set when ClearWorldMapTexture has dropped the world map's texture.
    static inline bool bWorldMapTextureCleared;
    // The user traits the menu's tiles share.
    static inline int32_t iUT_TextAlpha;
    static inline int32_t iUT_Title;
    static inline int32_t iUT_Selected;
    static inline int32_t iUT_Y;
    static inline int32_t iUT_X;
    static inline bool bNotesChallengeMode;
    // The tile trait id of the map magnification.
    static inline int32_t iUT_Magnification;
    static inline int32_t iUT_CurrentTab;
};

extern SettingT<INISettingCollection> bRenderLocalMapContinually;
// UpdateWorldMap's function-local table, one icon per map marker type from 1, so it is indexed one entry below its
// first.
using MarkerIconFilenameArray = const char* [14];
inline MarkerIconFilenameArray pcMarkerIconFilenamesA{"Interface\\Icons\\World Map\\icon_map_city.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_settlement.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_encampment.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_natural_landmark.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_cave.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_factory.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_monument.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_military.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_office.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_ruins_town.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_ruins_urban.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_ruins_sewer.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_metro.dds",
                                                      "Interface\\Icons\\World Map\\icon_map_vault.dds"};
inline int32_t g_mapMenuTrait_Value;
inline int32_t mapMarkerIndex;
inline int32_t g_lastMapMenuScrollTime;
inline int32_t time_11DA418;

extern SettingT<GameSettingCollection> gs_fLocalfPlayerCursorMinSize;

extern SettingT<GameSettingCollection> gs_fWorldfPlayerCursorMinSize;

extern SettingT<INIPrefSettingCollection> bShowQuestMarkers;

// sChallengeKill, sChallengeDamage and sChallengeOther.
using g_challengeStringsArray_t = SettingT<GameSettingCollection>* [3];
extern g_challengeStringsArray_t g_challengeStringsArray;
