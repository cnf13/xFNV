#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSemaphore.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/BSThread.h"
#include "BSMain/Setting.h"
#include "BSMenu/Font.h"
#include "BSMenu/Menu.h"
#include "Fallout Shared/MiscStats.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Reconstruction.hpp"

class BGSLoadGameBuffer;
class BGSQuestObjective;
class BGSSaveGameBuffer;
class TESLoadScreen;
class TESWorldSpace;
class TileShaderProperty;

// The position, size, font and justification a loading menu tile has in the XML.
struct LoadScreenDefaultProperties {
	int32_t nOriginX;
	int32_t nOriginY;
	int32_t nHeight;
	int32_t nWidth;
	FONTS eFontType;
	uint8_t eJustification;
};

// The player's current quest objective, level progress and level the loading screen shows, read once per load.
struct LoadingMenuDisplayData {
	// The PDB's LoadingMenuDisplayData(): clears the three fields (thiscall, ret 0); the exe's copy is
	// PlayerActionObject's constructor 0x78D900.
	static LoadingMenuDisplayData* Construct(void* apMemory);

	BGSQuestObjective* pCurrentObjective;
	// The progress to the next level, a fraction.
	float fLevelProgress;
	int32_t iLevel;
};

// The thread that loads in the background while the loading menu (or the main menu) runs.
class LoadingMenuThread : public BSThread {
public:
	LoadingMenuThread();
	~LoadingMenuThread() override;

	uint32_t ThreadProc() override;

	// True while the thread is parked in its suspend wait (bFlagSuspended); thiscall, 0x7CFD00, a one-byte getter
	// the exe shares with other classes.
	bool QIsSuspended();
	// Wakes the suspended thread.
	void Unsuspend();

	// Whether the loading thread exists and is suspended.
	static bool GetByte49();
	static void operator delete(void* apMemory) { ourFree(apMemory); }

	BSSemaphore BackgroundThreadSema;
	BSSemaphore MainThreadSema;
	// Set by the main thread to park the loading thread; bFlagSuspended is the thread's answer.
	bool bFlagSuspend;
	bool bFlagSuspended;
	bool bExit;
};

// The loading menu: level, load and dying screens.
class LoadingMenu : public Menu {
public:
	// The placement of one loading screen text tile (IDA's names; the PDB has none).
	struct TileData {
		uint32_t x;
		uint32_t y;
		uint32_t height;
		uint32_t width;
		uint32_t font;
		uint8_t justify;
	};

	// The bits of usFlags.
	enum Flag : int32_t {
		LM_INITLOAD_COMPLETE = 0,
		LM_FORWARD_ANIM = 1,
		LM_AUDIO_THREADING = 2,
		LM_RESET_MENU = 3,
		LM_FADE_IN = 4,
		LM_USE_MAINMENU_SCREENS = 5,
		LM_SHOW_PLAYER_TILES = 6,
		LM_NEW_LOADSCREEN_LIST = 7,
		LM_STATIC_MENU = 8,
		LM_WHEEL_ONLY_MODE = 9,
		// The flag the script command that enables the loading menu sets.
		LM_ENABLE_LOADING_MENU_COMMAND = 10,
	};

	// The welcome screens of the initial load, indexes into pWelcomeScreensA.
	enum WelcomeScreenID : int32_t {
		LM_INDIAN_HEAD = 0,
		LM_BETHESDA_SOFTWORKS = 1,
		LM_BGS_LOGO = 2,
		LM_LEGAL = 3,
		LM_MAIN_BACKGROUND = 4,
		LM_NUM_WELCOME_SCREENS = 5,
	};

	// The tile ids of the menu's XML, indexes into pTiles; the values are the PDB's.
	enum TileIndex : int32_t {
		eBackground = 0,
		ePlateNumber = 1,
		ePlateText = 2,
		eCurrentObjective = 3,
		eRandomStat1Label = 4,
		eRandomStat1Val = 5,
		eRandomStat2Label = 6,
		eRandomStat2Val = 7,
		eRandomStat3Label = 8,
		eRandomStat3Val = 9,
		eLevelProgress = 10,
		ePinwheel = 11,
		eWorldImage = 12,
		ePlateLabel = 13,
		eCurrentObjectiveLabel = 14,
		eLevelProgressLabel = 15,
		eLevelProgressParentRect = 16,
		eRandomStatLabelRect = 17,
		eRandomStatValRect = 18,
		eRandomStat4Label = 19,
		eRandomStat4Val = 20,
		eRandomStat5Label = 21,
		eRandomStat5Val = 22,
		eRandomStat6Label = 23,
		eRandomStat6Val = 24,
		eRandomStat7Label = 25,
		eRandomStat7Val = 26,
		eCurrentObjectiveRect = 27,
		eLevelProgressCurrentLevel = 28,
		eLevelProgressNextLevel = 29,
		eLevelProgressCharMarker = 30,
		eRandomStatRect = 31,
		eTileSlide01 = 32,
		eTileSlide02 = 33,
		eTipText01 = 34,
		eTipText02 = 35,
		eTipMatte_Slide01 = 36,
		eTipMatte_Slide02 = 37,
		NUM_TILES = 38,
	};

	// The texture settings of the welcome screens, by WelcomeScreenID.
	using WelcomeScreenSettings = SettingT<INISettingCollection>[LM_NUM_WELCOME_SCREENS];

	LoadingMenu();
	~LoadingMenu() override;

	// Remembers a tile the menu's XML created; ids outside the tile table are ignored.
	void AttachTileByID(int32_t aiID, Tile* apTile) override;
	int32_t GetClass() override;

	// True when the given bit of usFlags is set.
	bool GetFlag(Flag aeFlag);
	// Sets or clears a bit of usFlags.
	void SetFlag(Flag aeFlag, uint8_t abOn);
	// Flips a bit of usFlags.
	void ToggleFlag(Flag aeFlag);
	// Rebuilds the lists of tiles the menu fades and shows as groups.
	void InitStatsTilesList();
	void InitObjectiveTilesList();
	void InitTipTilesList();
	// Shows or hides the tiles of the stats, objective and experience groups.
	void UpdateStatsTilesVisibilities(bool bVisible);
	void UpdateObjectiveTilesVisibilities(bool bVisible);
	void UpdateXPProgressTilesVisibilities(bool bVisible);
	// Shows or hides the tip tiles.
	void UpdateTipTilesVisibilities(bool bVisible);
	// Restarts the pinwheel animation.
	void ResetLoadingPinwheelAnimation();
	// Advances the slides, background and text for the frame.
	void Update();
	// Builds the list of load screens to cycle through.
	void MakeLoadScreenList();
	// Moves on to the last slide of the list while the initial load runs and the current slide is not playing;
	// true when it did (IDA's name).
	bool ForceFinish();
	void InitXPProgressTilesList();
	// Loads the texture of the next load screen into the slide tile whose turn it is, holding it in the next slot
	// of spHoldTexture. The PC body never reads apTSP.
	void SetNextSlide(TileShaderProperty* apTSP);
	// Deletes the temporary load screens of the list and empties it (IDA's ClearLoadScreenList).
	void FlushLoadScreenList();
	// True when apLoadScreen is a load screen and none in the list has its texture (IDA's
	// LoadScreenDoesntContainScreen).
	bool OK2Add(TESLoadScreen* apLoadScreen);
	// Shows only the tile aeTileIndex (with eBackground, the slide tiles eTileSlide01 and eTileSlide02 too) and
	// records abShowPlayerTiles.
	void HideAllBut(TileIndex aeTileIndex, bool abShowPlayerTiles);
	// Sets the tile alpha of every geometry under apObject to afPct.
	void FadeIn3D(NiAVObject* apObject, float afPct);
	// Records the XML layout of the player tiles in DefaultTileSettings.
	void GetPlayerTileDefaultSettings();
	// Sets the menu up for a load into apLocation (a worldspace, cell or reference): the tiles to show, the slide
	// shader properties, the load screen list and the audio threading.
	void _Create(TESForm* apLocation);
	// Restarts the menu for a load into apLocation.
	void Reset(TESForm* apLocation);
	// Shows or hides the player tiles.
	void ShowPlayerTiles(bool abShow);
	// Readies the player tiles for the first slide.
	void PrepPlayerTiles();
	// Fills the screen tiles for the current load screen.
	void SetScreenInfo(bool abRefreshCurrent);
	// Moves pNextLoadScreen on.
	void AdvanceScreen();
	// Starts the menu's fade in.
	void FadeIn();
	// Fades apTile in; IDA names the float a3 (thiscall, ret 8, 0x78CE10).
	void FadeIn(Tile* apTile, float afTime);
	// Lays the player tiles out for the type of pLoadScreen (the next load screen when null) while they show.
	void UpdatePlayerTileSettings(TESLoadScreen* pLoadScreen);
	// Fits the text window of apData to the screen.
	void CalcLoadScreenTypeTextWindowFaces(const LoadScreenType_Data* apData);
	// Lay out the tiles of each player tile group from apData (thiscall, ret 4: 0x78BC80,
	// 0x78BF80, 0x78C210, 0x78C770).
	void InitStatsTilesSettings(LoadScreenType_Data* apData);
	void InitObjectiveTilesSettings(LoadScreenType_Data* apData);
	void InitXPProgressTilesSettings(LoadScreenType_Data* apData);
	void InitTipTilesSettings(LoadScreenType_Data* apData);
	// Fills aListOut with the load screens that may show for this load.
	void BuildCandidateList(BSSimpleArray<TESLoadScreen*, 1024>& aListOut);
	// Adds random screens of aCandidateList to LoadScreens up to the configured screen limit.
	void AddCandidates(BSSimpleArray<TESLoadScreen*, 1024>& aCandidateList);
	// Schedules the next background swap from the update interval settings.
	void NextSwap();
	void InitPlayerTileSettings();
	// Shows the player tile group of pLoadScreen's type and hides the others.
	void UpdatePlayerTileVisibilities(TESLoadScreen* pLoadScreen, bool bVisible);
	void UpdatePlayerTileAlphas(TESLoadScreen* pLoadScreen, float afAlpha, int32_t anSlide);
	void UpdateStatsTilesAlphas(float afAlpha, int32_t anSlide);
	void UpdateObjectiveTilesAlphas(float afAlpha, int32_t anSlide);
	void UpdateTipTilesAlphas(float afAlpha, int32_t anSlide);
	void UpdateXPProgressTilesAlphas(float afAlpha, int32_t anSlide);

	// The loading menu's class id.
	static int32_t GetClassStatic();
	// Opens the loading menu for a load into apLocation, or resets the one that is up. abScripted is the PC's
	// extra argument, the LM_ENABLE_LOADING_MENU_COMMAND flag.
	static Tile* Create(TESForm* apLocation, bool abBackgroundUpdate, bool abScripted);
	// Pushes the tile changes to the screen.
	static void ShowChanges();
	// Sets or clears LM_RESET_MENU on the menu that is up.
	static void FlagReset(bool abOn);
	// Parks the background loading thread until ResumeBackgroundThread.
	static void SuspendBackgroundThread();
	// The fraction of the way from the player's level's experience to the next level's, in ST0; 0 at the maximum
	// level (cdecl; IDA: GetLevelXPProgressPercentage).
	static float GetPlayerLevelProgress();

	// Hides every tile but the background. The only caller (StartMenu::MainMenuIdle) pushes 1; the PC body never
	// reads it.
	static void SetupForStartMenu(int32_t aiUnused);
	// The script command that turns the loading menu on or off.
	static void SetScriptMode(uint8_t abScriptMode);
	static bool IsInScriptMode();
	static bool QResetFlagged();
	// Starts a reset to the given worldspace when the menu has none yet.
	static void SetWorldspaceAndChangedFlag(TESWorldSpace* apWorldSpace);
	// Wakes the background loading thread.
	static void ResumeBackgroundThread();
	// Sets up the loading menu behind the main menu, creating it when abCreate.
	static void SetupForMainMenu(bool abCreate);
	// Tells the loading thread to exit and wakes it.
	static void SetThreadExiting();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void KillThread();
	// Stores the player's current quest objective, level progress and level for the loading screen.
	static void FillDisplayData();
	static void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	static void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

	// The tiles of the menu's XML, by tile id.
	Tile* pTiles[NUM_TILES];
	// The PDB's tagRECT rLoadScreenTypeTextWindowFaces (LONG members).
	int32_t iMinX;
	int32_t iMinY;
	int32_t iMaxX;
	int32_t iMaxY;
	// The tile groups the menu shows together.
	NiTPointerList<Tile*> StatsTilesList;
	NiTPointerList<Tile*> ObjectiveTilesList;
	NiTPointerList<Tile*> XPProgressTilesList;
	NiTPointerList<Tile*> TipTilesList;
	NiTPointerList<Tile*> TipTilesList02;
	// The misc stats in display order.
	MiscStatManager::MiscStatID eIDOrder[43];
	// The texture path of the next slide.
	BSStringT<char> strBuffer;
	NiColorA SaveBackgroundColor;
	// The load screens to cycle through, and the node of the one that comes next.
	BSSimpleList<TESLoadScreen*> LoadScreens;
	BSSimpleList<TESLoadScreen*>* pNextLoadScreen;
	TESLoadScreen* pCurrentLoadScreen;
	uint32_t uiNextBackgroundSwap;
	uint32_t uiFadeInComplete;
	// The form being loaded into: a worldspace, cell or reference.
	TESForm* pLocation;
	// The shader properties of the two slide tiles the menu cross-fades between.
	TileShaderProperty* pSlide1TSP;
	TileShaderProperty* pSlide2TSP;
	BSSoundHandle InitialSlideSound;
	BSSoundHandle SlideSound;
	// The last slide textures, held until they are four slides old (one on the Xbox).
	NiPointer<NiTexture> spHoldTexture[4];
	// The slot of spHoldTexture the next slide takes.
	uint32_t uiCurrTextureIndex;
	uint16_t usCurIndex;
	uint16_t usFlags;
	// The XML positions, sizes, fonts and justification of the tiles, by tile id.
	LoadScreenDefaultProperties DefaultTileSettings[38];
	uint32_t m_TileCrossFadeStartTime;
	uint32_t m_SlideCount;
	int32_t m_DelayedTipMatteUpdate;

	// The menu while one is up.
	static inline LoadingMenu* pMe;
	// The background loading thread.
	static inline LoadingMenuThread* pLoadingThread;
	// True until the first load from the main menu has finished.
	static inline bool bInitialLoad = true;
	// Set while Create reads the menu's XML.
	static inline bool bSuppressWarnings;
	// Set when _Create shows only the pinwheel.
	static inline bool bDoTileUpdate = true;
	static WelcomeScreenSettings pWelcomeScreensA;
	static inline bool bLocationSpecificLoadScreensOnly;
	// The most load screens a load cycles through (iMaxScreens:Loading).
	static SettingT<INISettingCollection> iMaxScreens;
	// The load screen limit while the main menu is up (iMaxScreens_MainMenu:Loading).
	static SettingT<INISettingCollection> iMaxScreens_MainMenu;
	// The most location-specific load screens a load shows (iNumLocationSpecificScreens:Loading).
	static SettingT<INISettingCollection> iNumLocationSpecificScreens;
	static SettingT<INISettingCollection> fLoadingTextUpdateInterval;
	static SettingT<INISettingCollection> sInitialSound;
	static inline char bRendering;
	// The current quest objective and the player stats the loading screen shows, read once per load.
	static LoadingMenuDisplayData DisplayData;
};

// Not named in the PDB or IDA; name chosen here. Steps a welcome screen id, returning the old one (cdecl; the
// exe's caller pushes the postfix dummy 0).
LoadingMenu::WelcomeScreenID operator++(LoadingMenu::WelcomeScreenID& aeID, int32_t);
// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
int32_t TryEnter(_RTL_CRITICAL_SECTION* apThis, [[maybe_unused]] void* apEdx);

inline constexpr fnvd::Vtable LoadingMenuThread_vftable{};
inline constexpr fnvd::Vtable LoadingMenu_vftable{};

// IDA names; not in the PDB.
inline bool bMainMenuMovieIntroStarted;

// INISettingCollection: 0xf9054d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sTitleMusic_Loading;

// INISettingCollection: 0xf906a2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fLoadingInitUpdateInterval_Loading;

// INISettingCollection: 0xf90672 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fMainMenuBkgdUpdateInterval_Loading;

// INISettingCollection: 0xf90642 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fLoadingBkgdUpdateInterval_Loading;
