#pragma once

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "BSSystemUtilities/BSDLCSystemUtility.h"
#include "BSSystemUtilities/BSNetworkSystemUtility.h"
#include "BSSystemUtilities/BSSaveDataSystemUtility.h"
#include "Fallout/Interface/ListBox.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "BSDevices/Controls.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSContentPackage;
class BSSaveDataSystemUtilityFile;
class TextEditMenu;

// The main menu and the in-game pause menu.
class StartMenu : public Menu {
public:
	// The indices of pTiles (the PDB's StartMenu::TileIndex), and the ids of an option's arrow and meter tiles.
	enum TileIndex : int32_t {
		eMainContainer = 0,
		eSettingsContainer = 1,
		eOptionsContainer = 2,
		eMainTitle = 3,
		eDefaultsButton = 4,
		eBackButton = 5,
		eSaveLoadContainer = 6,
		eConfirmContainer = 7,
		eConfirmQuestion = 8,
		eWelcomeText = 9,
		eVersionText = 10,
		eScreenShot = 11,
		ePlayerName = 12,
		ePauseBackground = 13,
		eDLCContainer = 14,
		eDeviceButton = 15,
		eControlsHeader = 16,
		eBrightnessGradient = 17,
		ePlayerLevel = 18,
		ePlayTime = 19,
		eConfirmWarning = 20,
		eRedeemCodeButton = 21,
		eSaveVersion = 22,
		NUM_TILES = 23,
		eLeftArrow = 100,
		eRightArrow = 101,
		eMeter = 102,
	};

	// Bits of uiFlags (the PDB's StartMenu::Flag).
	enum Flag : uint32_t {
		FLAG_INGAME = 0x1,
		FLAG_OPTIONS_DIRTY = 0x2,
		FLAG_FORCE_RELOAD = 0x4,
		FLAG_SAVE_MODE = 0x8,
		FLAG_FADE_IN = 0x10,
		FLAG_SAVE_OPTIONS = 0x20,
		FLAG_RESET_HUD_OPACITY = 0x40,
		FLAG_DLC_NEEDS_UPDATE = 0x80,
		FLAG_NEW_DLC_INSTALLED = 0x100,
		FLAG_SEARCHING_DLC = 0x200,
		FLAG_DLC_WAIT_MESSAGE = 0x400,
		FLAG_SYSUTILSAVECOMPLETE = 0x800,
		FLAG_ACTION_MAPPING = 0x1000,
		FLAG_DELETE_MODE = 0x2000,
		FLAG_SAVE_WAIT_MESSAGE = 0x4000,
		FLAG_SAVE_WAIT_SAVING = 0x8000,
		FLAG_SHOWING_CREDITS = 0x10000,
		FLAG_DONT_FORCE_BACK = 0x20000,
		FLAG_SAVELOAD_BACKOUT = 0x40000,
		FLAG_IIS_PREFADE = 0x80000,
		FLAG_LOADING_SAVEGAMES = 0x100000,
		FLAG_LIVE_MENU = 0x200000,
		FLAG_RESTART_REQUIRED = 0x400000,
		FLAG_MOUNT_DLC = 0x800000,
		FLAG_DLC_CORRUPT_MESSAGE = 0x1000000,
		FLAG_USER_PRESSED_START = 0x2000000,
		FLAG_USER_IS_ACTION_MAPPING = 0x4000000,
		FLAG_MEM_CARD_REMOVED = 0x8000000,
		FLAG_SAVE_SAVING = 0x10000000,
		// SetFlag20000000 sets it, ConfirmLoadGame
		// tests it after a failed load.
		FLAG_20000000 = 0x20000000,
		// CreditsMenu::Close tests and clears it before offering to continue
		// playing after the credits.
		kSomethingSave = 0x40000000,
	};

	// The menu modes an option shows in (Option::iModes; the PDB's unnamed enum of StartMenu).
	enum : uint32_t {
		MODE_MAIN = 0x1,
		MODE_PAUSE = 0x2,
		MODE_SETTINGS = 0x4,
		MODE_GAMEPLAY = 0x8,
		MODE_DISPLAY = 0x10,
		MODE_AUDIO = 0x20,
		MODE_CONTROLS = 0x40,
		MODE_CFM_NEW = 0x80,
		MODE_CFM_CONT = 0x100,
		MODE_CFM_LOAD = 0x200,
		MODE_CFM_SAVE = 0x400,
		MODE_CFM_DELETE = 0x800,
		MODE_CFM_QUITM = 0x1000,
		MODE_CFM_QUITP = 0x2000,
	};

	// An entry of the option list boxes: its label, the function choosing it runs and the menu modes it shows in.
	// The action-mapping entries keep their action in iModes.
	class Option {
	public:
		// Constructs an option in apMem; returns apMem.
		Option(const char* apDisplayText, void (*apf)(), uint32_t aiModes);
		virtual ~Option();

		// True when the option shows in any of the menu modes aiModes.
		bool IsForModes(uint32_t aiModes);

		static void operator delete(void* apMemory) { ourFree(apMemory); }

		const char* pDisplayText;
		void (*pfChooseFn)();
		uint32_t iModes;
	};

	// A settings entry with a value: a toggle, a wrapping list of values or a meter.
	class UserOption : public Option {
	public:
		enum UserOptionType : int32_t {
			UOPT_TOGGLE = 0,
			UOPT_TOGGLE_NOWRAP = 1,
			UOPT_METER = 2,
		};

		// Constructs a user option in apMem, followed by its iNumSettings value strings for a toggle; returns apMem
		// (cdecl and variadic, apMem first, 0x7D6350).
		UserOption(const char* apDisplayText, void (*apCallBack)(UserOption*), uint32_t aiModes, UserOptionType aeType,
			uint32_t aiNumSettings, uint32_t aiArrowSpacing, ...);
		~UserOption() override;

		const char* pTemplate;
		int32_t iVal;
		int32_t iDefaultVal;
		UserOptionType eType;
		uint32_t iNumSettings;
		uint32_t iArrowSpacing;
		const char** pValueStrings;
		// Applies the value; called with the option.
		void (*pfChangeFn)(UserOption*);
	};

	// The list box class of the main, settings, confirmation and options lists.
	class OptionListBox : public ListBox<Option*> {
	public:
		OptionListBox();

		Tile* GetHighlightTarget(int32_t aiButton) override;

		static void operator delete(void* apMemory) { ourFree(apMemory); }
	};

	// Measures how long the menu has been left alone (the 360 build's attract screen; the PC uses it for the
	// save-game screenshot).
	class IdleTimer {
	public:
		// Clears both times.
		IdleTimer();

		// Milliseconds since the start (up to the pause while paused), 0 when stopped.
		uint32_t GetElapsedTime();
		// Clears the start and pause times.
		void Stop();
		void Start();
		int32_t GetStarted();
		int32_t GetPaused();

		// The timer, allocated and started on first use.
		static IdleTimer* Instance();
		static void Delete();

		uint32_t uiT0;
		uint32_t uiPaused;

		static inline IdleTimer* pInstance;
	};

	// A display resolution of the PC's settings (the exe's RTTI name; no PDB type). 12 bytes: the array's shared
	// 12-byte bodies, and its _DestructItems (0x7D9E80) destroys each element through the deleting destructor. No
	// code of the exe adds an element (kVideoResolutions is only constructed, cleared and destroyed), so the two
	// words at +8 are only ever zeroed, never read.
	class VideoRes {
	public:
		// The default constructor (thiscall, 0x7DA240): an empty string through 0x4037B0, then
		// two zero words at +8 and +0xA.
		VideoRes();
		~VideoRes();

		static void operator delete(void* apMemory) { ourFree(apMemory); }

		BSStringT<char> unk00;
		// A resolution's two 16-bit dimensions (IDA types +0x8 as one dword, unk08). The constructor zeroes each
		// with its own 16-bit store (0x7DA240: `mov word ptr [ecx+8], ax`, `mov word ptr [eax+0xA], dx`).
		uint16_t unk08;
		uint16_t unk0A;
	};

	StartMenu();
	~StartMenu() override;

	void AttachTileByID(int32_t aiID, Tile* apTile) override;
	void DoDownClick(int32_t aiID, Tile* apTarget) override;
	void DoClick(int32_t aiID, Tile* apTarget) override;
	void DoEnter(int32_t aiID, Tile* apTarget) override;
	void DoLeave(int32_t aiID, Tile* apTarget) override;
	void DoStillDown(int32_t aiID, Tile* apTarget) override;
	void DoIdle() override;
	bool DoKey(int32_t aiKey) override;
	int32_t GetClass() override;
	bool DoGamepad(int32_t aiButton, float afValue) override;
	bool DoGamepadUpEvent(int32_t aiButton, float afValue) override;

	// IDA name PrepSaveLoad; not in the PDB. Prepares the save and load list (thiscall, ret 0,
	// 0x7D3B60).
	void PrepSaveLoad();
	// IDA name NextControlsDevice; not in the PDB. Steps to the next controls device until a usable one, at most four times.
	void NextControlsDevice();
	// IDA name IsControlsDeviceValid; not in the PDB. Whether the current controls device is usable: with a 360 controller
	// only the controller (3) is; otherwise any other device that has input interfaces.
	bool IsControlsDeviceValid();
	// Sets up the lists, the option buttons and the pause or main menu look of the new menu.
	void _Create(bool abInGame, bool abForceReload);
	// True when any of the given flag bits is set.
	bool IsFlagSet(Flag aeFlag);
	// Sets or clears the given flag bits.
	void SetFlag(Flag aeFlag, uint8_t abOn);
	// Shows the sub menu options for the menu modes auiModes (thiscall, ret 4, 0x7D42B0). Not in the PDB: the exe
	// passes the value on to Option::IsForModes, whose PDB parameter is uint32_t aiModes, and tests its MODE_* bits.
	void PopulateOptionsListBox(uint32_t auiModes);
	// Enables the continue and load buttons.
	void UpdateCanLoadStatus();
	// Runs the action-mapping screen: fades the options in and takes the key, button or hat pressed for the
	// highlighted action; false when the screen is not up.
	bool ActionMappingIdle();
	// Runs the main menu's fades and DLC messages; false in the pause menu.
	bool MainMenuIdle();
	// Runs the save and load list: the save in progress, the list's lazy fill and the screenshot; false while the
	// list is hidden.
	bool SaveLoadIdle();
	// True while the action-mapping list waits for the key of its selected action.
	bool WaitingForControlRemapButtonPress();
	// Refreshes the profile-specific settings after the main menu has faded in.
	void _UpdateProfileSpecificData();
	// Fills the action-mapping list for the current device.
	void PopulateControlsListBox();
	// Fills the save and load list from the save manager.
	void PopulateSaveLoadListBox();
	void PopulateConfirmationListBox(uint32_t auiModes, uint8_t aucFlags);
	// Populates the confirmation list at aiRealX, aiRealY: flag 1 picks the wide tile value, flag 2 shows the
	// warning.
	void PopulateConfirmationListBox(uint32_t auiModes, int32_t aiRealX, int32_t aiRealY, uint8_t aucFlags);
	// Hides the options container, disables the action-mapping list and, after action mapping, deletes its options.
	void HideOptionsContainer();
	// Populates a confirmation list at the requested position (thiscall, ret 0x14, 0x7D49A0).
	// auiModes and abVisible: the PDB's PopulateConfirmationListBox(unsigned, int, int, unsigned, bool); the exe reads
	// abVisible with movzx byte.
	uint32_t PopulateConfirmationListBox_0(uint32_t auiModes, int32_t aiX, int32_t aiY, uint32_t aiTileValue,
		bool abVisible);
	// Finishes opening the save and load list once its container has faded in.
	void FinishPrepSaveLoad();
	// Shows an option's current value on its tile (apTile, or the option's own when null).
	void DisplayOption(UserOption* apOption, Tile* apTile);
	// Shows the screenshot of apSaveGame (the highlighted save when null); true when one is shown.
	bool SetSaveGameScreenshot(BGSSaveLoadFileEntry* apSaveGame);
	// Fades the trait of a tile in or out over afFadeTime seconds; the trait 0x80000000 stands for "_alpha", a
	// negative time for a quarter second.
	void Fade(TileIndex aeIndex, bool abFadeIn, int32_t aiTrait, float afFadeTime);
	// Goes back from the confirmation list to the save and load list.
	void ReturnToSaveLoad();
	// Runs every user option's change function to load its value, and finds the brightness option.
	void InitializeUserOptions();
	// Shows the details of the highlighted save.
	void UpdateHighlightedSaveGameInfo();
	// Backs out until the main list is enabled, unless FLAG_DONT_FORCE_BACK is set or the list is hidden.
	void _BackOutToMainListbox();
	void ConfirmSaveLoad();
	void ShowDLCList();
	void Music();

	// The start menu's class id.
	static int32_t GetClassStatic();
	// Builds the menu from start_menu.xml unless a menu is fading in or out; false when it is not built.
	static bool Create(bool abInGame, bool abForceReload);
	// True for the menus the pause menu hides behind it.
	static bool ShouldHideMenu(int32_t aiMenuID);
	// IDA name; not in the PDB. Sets or clears the open menu's flag 0x20000000 (no PDB StartMenu::Flag holds it).
	static void SetFlag20000000(uint8_t abOn);
	// True while the pause menu (the start menu opened in game) exists.
	static bool PauseMenuIsActive();
	// True while the main menu (the start menu outside the game) exists.
	static bool MainMenuIsActive();
	// IDA name; not in the PDB. Whether the start menu refuses to close: two of its flags
	// (0x8000 and 8) are set (cdecl, al, 0x70EE30).
	static bool IsStartMenuSaving();
	// True while the start menu is showing the credits.
	static bool CreditsMenuIsActive();
	static bool IsInitialLoading();
	// The save and load list's filter: false hides apSaveGame. Everything shows without a menu; otherwise only the
	// null entry outside save mode, while the list holds more than one item.
	static bool SaveLoadFilterFn(BGSSaveLoadFileEntry* apSaveGame);
	// Closes the menu.
	static void Close(bool abDeleteOnFade);
	// An action-mapping option's callback: while the menu maps a control, shows "press the control".
	static void ChooseUserAction();
	// Loads the newly installed downloadable content.
	static void LoadDownloadedContent();
	// Writes the settings to the ini files.
	static void SaveSettings();
	// Builds the options of pOptionsA and pUserOptionsA (after deleting the old ones), and fills g_strBlue and
	// g_strGreen on first use.
	static void CreateMenuElements();
	// Deletes the options of pOptionsA and pUserOptionsA and empties both arrays and kVideoResolutions (IDA's
	// FreeButtons).
	static void DestroyMenuElements();
	// The rumble option's change function: applies the value while FLAG_SAVE_OPTIONS is set, else loads the
	// value and the default.
	static void SetRumble(UserOption* apUOpt);
	// The settings change functions below follow SetRumble: with FLAG_SAVE_OPTIONS set they apply the meter's value,
	// else they load the value and the default into the option.
	// The HUD opacity meter (fHudOpacity, default 1); applying it sets FLAG_RESET_HUD_OPACITY.
	static void SetHUDOpacity(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The item fade meter: the items' LOD fade multiplier between fItemLODMin
	// and fItemLODMax.
	static void SetItemFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The object fade meter: the objects' LOD fade multiplier between
	// fObjectLODMin and fObjectLODMax.
	static void SetObjectFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The grass fade meter: fGrassStartFadeDistance between its minimum and
	// maximum.
	static void SetGrassFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The shadow fade meter: the shadow LOD fade start and end.
	static void SetShadowFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The light fade meter: the light LOD fade start and end.
	static void SetLightFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The specularity fade meter: the specular LOD fade start and end.
	static void SetSpecularityFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The object LOD fade meter: fBlockLoadDistanceLow between its low and high.
	static void SetObjectLODFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The tree LOD fade meter: fTreeLoadDistance between its low and high.
	static void SetTreeLODFade(UserOption* apUOpt);
	// IDA name; not in the PDB (PC only). The mouse sensitivity meter: fMouseSensitivity from 0.0005 to 0.01.
	static void SetMouseSensitivity(UserOption* apUOpt);
	// Backs out of the current list one level.
	static void ChooseBack();
	// The network status callback; does nothing on the PC.
	static void OnNetworkEvent(BSNetworkSystemUtility::NetEvent apEvent, uint32_t apParam);
	static bool IsInPreIIS();
	static void ConfirmResetToDefaults();
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void ResetToDefaults(Tile* apTile, StartMenu::Option* apOption);
	static void GoBackIfInActionMapping();
	static void SetActionForDevice(Tile* apTile, StartMenu::Option* apOption);
	static void ChooseContinue();
	static void ChooseNew();
	static void ChooseDownloads();
	static void ChooseLoad();
	static void ChooseSave();
	static void ChooseSettings();
	static void ChooseHelp();
	static void ChooseQuit();
	static void ChooseMainMenu();
	static void ChooseExitGame();
	static void ChooseActionMap();
	static void ChooseGameplay();
	static void ChooseControls();
	static void SetDifficulty(StartMenu::UserOption* apUOpt);
	static void SetKillCam(StartMenu::UserOption* apUOpt);
	static void SetCrosshair(StartMenu::UserOption* apUOpt);
	static void SetSaveOnRest(StartMenu::UserOption* apUOpt);
	static void SetSaveOnWait(StartMenu::UserOption* apUOpt);
	static void SetSaveOnTravel(StartMenu::UserOption* apUOpt);
	static void SetDialogSubtitles(StartMenu::UserOption* apUOpt);
	static void SetGeneralSubtitles(StartMenu::UserOption* apUOpt);
	static void SetTrueIronSights(StartMenu::UserOption* apUOpt);
	static void SetHUDColor(StartMenu::UserOption* apUOpt);
	static void SetPipboyColor(StartMenu::UserOption* apUOpt);
	static void SetBrightness(StartMenu::UserOption* apUOpt);
	static void SetMasterVol(StartMenu::UserOption* apUOpt);
	static void SetFootstepVol(StartMenu::UserOption* apUOpt);
	static void SetMusicVol(StartMenu::UserOption* apUOpt);
	static void SetVoiceVol(StartMenu::UserOption* apUOpt);
	static void SetFXVol(StartMenu::UserOption* apUOpt);
	static void SetRadioVol(StartMenu::UserOption* apUOpt);
	static void SetCameraPitch(StartMenu::UserOption* apUOpt);
	static void ConfirmContinue();
	static void ConfirmNewGame();
	static void ConfirmLoadGame();
	static void ConfirmSaveGame();
	static void ConfirmDeleteGm();
	static void MountDLCCallback(uint32_t which, uint32_t last, bool valid, const BSContentPackage* package);
	static void FailedSaveCallbackFunction();
	static void InitializeSaveLoadListItem(Tile* apTile, BGSSaveLoadFileEntry* apSaveGame);
	static void SysUtilSaveCompleteCallback(BSSaveDataSystemUtility::RESULT aeResult,
		BSSaveDataSystemUtilityFile*& apSaveDataFile);

	// Deletes a traversed option through its vtable's deleting destructor with flag 1; the
	// tile is unused (cdecl, ret 0, 0x7CE970; IDA misnames it UserOption's vector deleting destructor).
	static void DeleteOption(Tile* apTile, Option* apOption);

	Tile* pTiles[NUM_TILES];
	OptionListBox lbMain;
	OptionListBox lbSettings;
	OptionListBox lbConfirm;
	// The action-mapping list.
	OptionListBox lbOptions;
	ListBox<int32_t> lbDownloads;
	ListBox<BGSSaveLoadFileEntry*> lbSaveLoad;
	// The option whose value changed and still has to be shown and applied.
	UserOption* pDirtyOption;
	uint32_t uiFlags;
	// How far the save and load list has been filled, in scroll units.
	float fSaveLoadThreshold;
	// The save the save list overwrites, or null for a new save.
	BGSSaveLoadFileEntry* pSaveGame;
	UserOption* pBrightnessOption;
	NiPointer<NiSourceTexture> spAlphaMap;
	Tile* pDownloadsOption;
	// The load and continue buttons' tiles (the main list's AddItem results).
	Tile* pLoadOption;
	Tile* pContinueOption;
	// The device the action-mapping list shows (0 keyboard, 1 mouse, 2 joystick, 3 360 controller); PC only,
	// IDA's name.
	uint32_t uiCurrentControlsDevice;
	// The rumble option's tile, which SetSubMenuOptionsVisibleForFlags stores when it adds that option (PC only;
	// IDA's name).
	Tile* unk1CC;
	// The trait id that enables a button.
	uint32_t ENABLED_TRAIT;

	static inline StartMenu* pMe;
	// The redeem-code entry menu while it is open.
	static inline TextEditMenu* pTextEntryMenu;
	// The main and pause menu options and the settings options CreateMenuElements builds.
	static BSSimpleArray<Option*> pOptionsA;
	static BSSimpleArray<UserOption*> pUserOptionsA;
	// The display resolutions (PC only; IDA's name).
	static BSSimpleArray<VideoRes> kVideoResolutions;
	// Set while the game restarts from the main menu.
	static inline bool bRestartingGame;
	// The PC only writes it, when the DLC wait message comes up. The PDB's only other bool static of StartMenu;
	// IDA leaves the byte unnamed.
	static inline bool bDLCListConstructed;
	// The menus the pause menu hid when it opened.
	static NiTPointerList<Tile*> xHiddenTiles;
};

void MenuButton_Credits();
void MenuButton_Display();
void MenuButton_Audio();
bool GetIsControllerDisabled();
// Steps the device index apDevice to the next of the four,
// wrapping to 0, and returns apDevice.
uint32_t* IncrementControlsDevice(uint32_t* apDevice);
void SetXboxController(StartMenu::UserOption* a1);
// Inferred type: return: float from 0x7d1b3d (result of Setting::operator float)
float Get_fGamma();
void SetTextureSize(StartMenu::UserOption* a1);
void SetActorFade(StartMenu::UserOption* a1);

inline constexpr fnvd::Vtable StartMenu_vftable{};
inline constexpr fnvd::Vtable StartMenu_OptionListBox_vftable{};
inline constexpr fnvd::Vtable StartMenu_Option_vftable{};
inline constexpr fnvd::Vtable StartMenu_UserOption_vftable{};

// The game settings StartMenu.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fActorLODMax;
extern SettingT<INISettingCollection> fActorLODMin;
extern SettingT<INISettingCollection> fObjectLODMin;
extern SettingT<INISettingCollection> fGammaMax;
extern SettingT<INISettingCollection> fObjectLODDefault;
extern SettingT<INISettingCollection> fObjectLODMax;
extern SettingT<INISettingCollection> fGammaMin;
extern SettingT<INISettingCollection> bFadeInScreenShot;
extern SettingT<INISettingCollection> fItemLODDefault;
extern SettingT<INISettingCollection> fItemLODMin;
extern SettingT<INISettingCollection> sCharGenQuest;
extern SettingT<INISettingCollection> fItemLODMax;
extern SettingT<INISettingCollection> fActorLODDefault;

// IDA name; not in the PDB. The HUD colour choices the options' HUD colour setting indexes, ARGB words a static
// initializer (0xF92D50) fills with MakeRGBA: four entries, 0x11DAC50 to 0x11DAC5C.
using HUDColorTable = uint32_t[4];
extern HUDColorTable g_HUDColors;
inline int32_t StartMenuOptionListBox_SetEnabled_iUT_Enabled;
inline int32_t StartMenuOptionListBox_IsEnabled_iUT_Enabled;
inline int32_t StartMenuOptionListBox_IsSelected_iUT_Selected;
inline int32_t BGSSaveLoadFileEntryListBox_IsEnabled_iUT_Enabled;
inline int32_t selected;
inline int32_t StartMenuOptionListBox_UpdateHighlightBoxPos_iUT_SelectedHeight;
inline int32_t StartMenuOptionListBox_UpdateHighlightBoxPos_iUT_Highlight_Y;
inline int32_t StartMenuOptionListBox_NumUnfiltered_iUT_num_filtered;

// The control device names (keyboard, mouse, joystick, 360 controller), indexed by uiCurrentControlsDevice.
using pDeviceNamesA_t = Setting* [4];
extern pDeviceNamesA_t pDeviceNamesA;

// The tick counts Create stores (IDA's names; the PC only).
inline uint32_t g_startMenuLastControllerInputTime;

// The terrain manager's object and tree LOD distances the LOD fade meters set (TerrainManager).
// INIPrefSettingCollection: 0xfa8bd2 calls 0x4de370.
extern SettingT<INIPrefSettingCollection> fMouseSensitivity;
