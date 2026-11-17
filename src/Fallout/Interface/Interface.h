#pragma once

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"

class Actor;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
class FOHUDEffectManager;
class FOPipboyManager;
class FORenderedMenu;
class FOVATSEffectManager;
class InterfaceManager;
class Menu;
class MobileObject;
class Model;
class NiNode;
enum RaceSexMenuContext : int32_t;
class TESModel;
class TESObjectREFR;
class TESRegion;
class TESTopic;
class TESWorldSpace;
class Tile;
class TileMenu;

char GetIsMapMenuFogOfWar();
// Inferred type: a1: int32_t: signed uses from 0x705c4b; a2: char: signed uses from 0x705c44
int32_t BSMAX_2(int32_t a1, char a2);
void ModalMessageMenuCallback();
// IDA name; not in the PDB. Passes abFogOfWar (movzx byte) on to SetMapMenuFogOfWar when the
// interface manager is up and online (cdecl, 0x704420); its one caller ignores the eax IDA reads as a result.
void ToggleMapFogOfWar(bool abFogOfWar);

// The interface layer's global entry points: every menu, the console and the pip-boy hang off the one
// InterfaceManager. Everything here is static; the class carries only static state of its own.
class Interface {
public:
	// The ids the engine gives every menu. The values are the executable's; the names are the ones the
	// debug symbols use for them.
	enum enumMenus : int32_t {
		eNoMenu = 0x0,
		eMainFour = 0x1,
		eOtherRoot = 0x2,
		eConsoleMenu = 0x3,
		eMessageMenu = 0x3E9,
		eFirstMenu = 0x3E9,
		eInventoryMenu = 0x3EA,
		eStatsMenu = 0x3EB,
		eHUDMainMenu = 0x3EC,
		eLoadingMenu = 0x3EF,
		eContainerMenu = 0x3F0,
		eDialogMenu = 0x3F1,
		eSleepWaitMenu = 0x3F4,
		eStartMenu = 0x3F5,
		eLockPickMenu = 0x3F6,
		eQuantityMenu = 0x3F8,
		eAudioMenu = 0x3F9,
		eVideoMenu = 0x3FA,
		eGameplayMenu = 0x3FC,
		ePipBoyData = 0x3FF,
		eMapMenu = 0x3FF,
		eBookMenu = 0x402,
		eLevelUpMenu = 0x403,
		eBirthSignMenu = 0x405,
		ePipBoyRepair = 0x40B,
		eRepairMenu = 0x40B,
		eRaceSexMenu = 0x40C,
		eCreditsMenu = 0x417,
		eCharGenMenu = 0x418,
		eTextEditMenu = 0x41B,
		eBarterMenu = 0x41D,
		eSurgeryMenu = 0x41E,
		eHackingMenu = 0x41F,
		eVATSMenu = 0x420,
		eComputersMenu = 0x421,
		eRepairServicesMenu = 0x422,
		eTutorialMenu = 0x423,
		eSPECIALBookMenu = 0x424,
		eItemModMenu = 0x425,
		eLoveTesterMenu = 0x432,
		eCompanionWheelMenu = 0x433,
		eTraitSelectMenu = 0x434,
		eRecipeMenu = 0x435,
		eSlotMachineMenu = 0x438,
		eBlackJackMenu = 0x439,
		eRouletteMenu = 0x43A,
		eCaravanMenu = 0x43B,
		eTraitMenu = 0x43C,
		eLastMenu = 0x43C,
	};

	// The fade states a menu can be in, as bits (the PDB's values); callers of IsMenuIDVisible pass a mask of the
	// states that count as visible (0: no fade test). Menu stores 1, 2, 4 and 8 in its xFadeState.
	enum FADE_STATE : int32_t {
		FADE_SHOWN = 1,
		FADING_OUT = 2,
		FADE_HIDDEN = 4,
		FADING_IN = 8,
		FADE_ALL = 15,
		FADE_IN_SHOWN = 9,
		FADE_OUT_SHOWN = 3,
		FADE_NOT_HIDDEN = 11,
	};

	// The tutorial messages, indices into InterfaceManager::TutorialManager's flags. The PDB's order; the exe confirms
	// TUT_COUNT = 41 (TutorialManager::ShowMessage's bound) and StatsMenu::Create shows TUT_PIPBOY_STATS (25).
	enum TutorialMessageID : int32_t {
		TUT_PIPBOY_STATUS,
		TUT_PIPBOY_SPECIAL,
		TUT_PIPBOY_SKILLS,
		TUT_PIPBOY_PERKS,
		TUT_PIPBOY_GENERAL,
		TUT_PIPBOY_ITEMS,
		TUT_PIPBOY_REPAIR,
		TUT_PIPBOY_LOCALMAP,
		TUT_PIPBOY_WORLDMAP,
		TUT_PIPBOY_QUESTS,
		TUT_PIPBOY_NOTES,
		TUT_PIPBOY_RADIO,
		TUT_CHARGEN_ATTRIBUTES,
		TUT_CHARGEN_TRAITS,
		TUT_CHARGEN_SKILLS,
		TUT_CHARGEN_RACE,
		TUT_LEVELING,
		TUT_DIALOGUE,
		TUT_SURGERY,
		TUT_HACKING,
		TUT_LOCKPICKING_PC,
		TUT_VATS_PC,
		TUT_CONTAINER,
		TUT_BARTER,
		TUT_TERMINAL,
		TUT_PIPBOY_STATS,
		TUT_PIPBOY_DATA,
		TUT_VATS_XBOX,
		TUT_LOCKPICKING_XBOX,
		TUT_PIPBOY_ITEMMOD,
		TUT_CARAVAN_BET,
		TUT_CARAVAN_DECK,
		TUT_CARAVAN_TRACK,
		TUT_CARAVAN_GAME,
		TUT_WEAPONS,
		TUT_APPAREL,
		TUT_AMMO,
		TUT_AMMO_XBOX,
		TUT_RECIPE,
		TUT_REPUTATION,
		TUT_HARDCORE_NEEDS,
		TUT_COUNT,
	};

	// The 360 controller's buttons, and the states CheckMenuButton reports for one. Values from the Xbox PDB.
	enum enumGamepadButtons : int32_t {
		eButtonNone = 0,
		eButtonDPadUp = 1,
		eButtonDPadDown = 2,
		eButtonDPadRight = 3,
		eButtonDPadLeft = 4,
		eButtonStart = 5,
		eButtonBack = 6,
		eButtonA = 9,
		eButtonB = 10,
		eButtonX = 11,
		eButtonY = 12,
		eButtonLT = 13,
		eButtonRT = 14,
		eButtonLB = 15,
		eButtonRB = 16,
		eButtonLThumb = 17,
		eButtonRThumb = 18,
		eButtonDownEvent = 2,
		eButtonUpEvent = 1,
		eButtonStillDownEvent = 0,
		eButtonIdleEvent = -1,
		eButtonClick = -2,
	};

	// A menu request a thread other than the main one queued; HandleQueuedMenuOpen opens it.
	enum OPEN_REQUEST : int32_t {
		OPEN_NONE = 0,
		OPEN_CONTAINER = 1,
		OPEN_HACKING = 2,
		OPEN_COMPUTERS = 3,
		OPEN_DIALOG = 4,
		OPEN_RACESEX = 5,
		OPEN_PAUSE = 6,
		OPEN_MESSAGE = 7,
		OPEN_COMPANION_WHEEL = 8,
		// Not in the PDB's enum: the PC build's request for the player name entry menu.
		OPEN_PLAYER_NAME_ENTRY = 9,
	};

	// What the container menu is opened for (ContainerMenu::eContainerMode). Values from the Xbox PDB.
	enum CONTAINER_MENU_MODE : int32_t {
		CMM_NONE = 0,
		CMM_STANDARD = 1,
		CMM_PICKPOCKET = 2,
		CMM_TEAMMATE = 3,
		CMM_ROCK_N_LOAD = 4,
	};

	// One queued menu request and its arguments.
	struct QUEUED_OPEN {
		QUEUED_OPEN();

		OPEN_REQUEST eOpenType;
		TESObjectREFR* pMenuRef;
		TESTopic* pGreetTopic;
		MobileObject* pSpeakeras;
		// RaceSexMenu::Create's context.
		RaceSexMenuContext eRSMContext;
		CONTAINER_MENU_MODE eContainerMode;
	};

	// Loads the interface block of a save: the HUD state, the stats, inventory and map tabs, the map zooms and the
	// tutorial flags.
	static void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Shows apString as a subtitle spoken at aSpeakerLoc by apTarget (cdecl, ret 0, result al,
	// 0x705210).
	static bool ShowSubtitle(const char* apString, BSSoundHandle ahandle, NiPoint3 aSpeakerLoc, TESObjectREFR* apTarget,
		bool abForceShow);
	// Shows or hides the pip-boy map tab.
	static void SetMapMenuVisible(bool abVisibility, bool abAnyVisibleState);
	// Opens the console.
	static void OpenConsole();
	// Shows or hides the pip-boy inventory tab.
	static void SetInventoryMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect);
	// Shows or hides the pip-boy stats tab.
	static void SetStatsMenuVisible(bool abVisibility, bool abAnyVisibleState);
	// The inventory menu's page number.
	static char GetInventoryPageNumber();
	// IDA name; not in the PDB. The pip-boy tab last shown, -1 before the interface's first init (cdecl, ret 0,
	// 0x705D90; the result is InterfaceManager::GetLastPipboyTab's eax).
	static int32_t GetLastPipboyTab();
	// The container menu's mode: GetContainerMenuMode's result.
	static uint32_t GetContainerMode();
	static void ForceReload();
	static void UpdateActorValue(ActorValue::Index aeWhich);
	static void SetCursorPos(float afX, float afY);
	// The button the message menu returned.
	static char GetMessageMenuResult();
	// Toggles the debug text once the interface has finished its first init.
	static void ToggleDebugTextVisible();
	// Refreshes the pip-boy's stats, map and surgery state after a load; the buffer is not read.
	static void FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Whether a menu request is queued: QueuedMenuOpen's type is not OPEN_NONE (cdecl, al,
	// 0x70EC00; IDA's IsMenuQueued, the PDB's name by what it reads).
	static bool IsMenuQueuedToOpen();
	// IDA name; not in the PDB. Whether the menu on top of the enter stack is ready for input
	// (cdecl, al, 0x7024E0).
	static bool IsTopOfMenuStackReady();

	// Open the container, hacking, computers and dialog menus (cdecl; 0x704ED0, 0x705EC0,
	// 0x706130, 0x705070). The container and dialog ones return the menu's tile, or null.
	static Tile* CreateContainerMenu(TESObjectREFR* apContainer, Interface::CONTAINER_MENU_MODE aeMode);
	static bool CreateHackingMenu(TESObjectREFR* apTerminalRef);
	static bool CreateComputersMenu(TESObjectREFR* apTerminalRef);
	// Opens the race/sex menu in mode aiMode.
	static void CreateRaceSexMenu(RaceSexMenuContext aeContext);
	// Draw the 3D parts of the lockpick, slot machine, blackjack, roulette and caravan menus
	// (cdecl; 0x709AF0, 0x709B00, 0x709B10, 0x709B20, 0x709B30).
	static void DrawLockpickMenu3D();
	static void DrawSlotMachineMenu3D();
	static void DrawBlackJackMenu3D();
	static void DrawRouletteMenu3D();
	static void DrawCaravanMenu3D();
	static Tile* CreateDialogMenu(TESObjectREFR* apSpeaker, TESTopic* apGreetTopic, MobileObject* apSpeakeras);
	// Opens the request queued in QueuedMenuOpen, and then the one in QueuedMessageOpen, when the pip-boy is
	// closed or the request is the pause or message menu; a request that opened is cleared.
	static void HandleQueuedMenuOpen();
	// Init stages 1 and 2 of the interface manager, inside the interface memory context.
	static void Init(bool abInitialLoad);
	static void InitSecond();
	// Draws the interface into the menu texture, unless a movie is playing; otherwise only steps the render
	// threads past the interface stage.
	static void Click(BSRenderedTexture* ap3DTarget, BSRenderedTexture* pMenuFXTexture);
	// Closes the console when it is open and pops it off the enter stack; returns the new stack top, or 0.
	static void CloseConsole();
	static void QueueMenuCreate(OPEN_REQUEST aeMenuType, TESObjectREFR* apMenuRef, TESTopic* apGreeting,
		MobileObject* apSpeakAs, Interface::CONTAINER_MENU_MODE aeContainerMode, bool bModeless);

	// The folder the interface's art lives in.
	static const char* GetMeshesFolder();

	// True while the loading menu exists.
	static bool IsInGameLoadingMenuOpen();

	// The actor the HUD shows as the current enemy.
	static TESObjectREFR* GetEnemyActor();
	static void SetEnemyActor(Actor* apActor);
	// The detection level the HUD reticle shows.
	static int32_t GetDetectionLevel();
	// Loads apModelFilename's model as the HUD's gun scope.
	static void InitGunScope(TESModel* apModelFilename);
	// The index of the user action whose setting is named apString or holds the text apString; 28 when none does.
	static int32_t MatchTextReplaceConstantToGameSettings(const char* apString);

	// Screen and viewport sizes, in pixels.
	static float GetScreenHeight();
	static float GetScreenWidth();
	// Plays menu sound aiSoundIndex.
	static void PlayMenuSound(int32_t aiSoundIndex);
	static double GetRealScreenHeight();
	static double GetRealScreenWidth();
	static bool IsWideScreen();
	// Expands a "&-name;" menu constant through the game settings into axStringOut; false when it names none.
	static bool TestConstantForGameSettings(const char* apString, char* axStringOut);
	// Writes the replacement text for menu variable apString into apOutBuffer (at most
	// aiBufferSize bytes); false when there is none.
	static bool FindTextReplacementString(const char* apString, char* apOutBuffer, uint32_t aiBufferSize,
		bool abAddPushText);
	// The interface manager's aspect-ratio scale for menus that scale by the resolution ratio.
	static float GetAspectRatioRatio();
	// Shows or hides the HUD's gun scope.
	static void SetGunScopeVisible(bool abVisibleFlag);
	// Clears the HUD's gun scope.
	static void ClearGunScope();
	// The fixed size the pip-boy's rendered menu is laid out at: 1280 wide, and the height that keeps the renderer's
	// aspect ratio.
	static float GetRenderedMenuScreenWidth();
	static float GetRenderedMenuScreenHeight();
	// (cdecl, float in ST(0)). The size of one screen pixel in menu units, computed once into
	// a guarded static.
	static float SizeOfAScreenPixelInCrazyAssMenuCoordinates();

	// Records that a text tile was created this frame.
	static void SetTextWasCreated(bool abSet);

	// The world map's root space, set when the map is opened for a world.
	static void SetRootMapSpace(TESWorldSpace* apNewSpace);
	// Empty in this build: a shared function that returns false.
	static bool PreloadWorldMap();

	// HUD 3D and scope state.
	static void DrawHUDMenu3D();
	static bool IsGunScopeVisible();

	// Gamepad presence.
	static bool Has360Controller();

	// True while the menu with this id is faded in; the fade states that count as visible are a mask.
	static char IsMenuVisible(enumMenus aeMenu);
	static bool IsMenuIDVisible(enumMenus aeMenuID, uint32_t abFadeStatesThatCountAsVisible);

	// The menu on top of the enter stack, and the id at a given depth of it.
	static enumMenus GetTopMenuID();
	static enumMenus GetMenuFromStack(int32_t aiIndex);
	static enumMenus GetMenuModeType();
	// The depth of the enter stack.
	static uint32_t GetMenuCount();
	static bool IsTopMenuID(enumMenus aeMenuID);

	// The enter stack the engine pushes menus onto. Both original callers push abForceRemoval; the PC body
	// never reads it.
	static void AddToEnterStack(int32_t aiID, bool abForceRemoval);
	static int32_t PopFromEnterStack(int32_t aiID, bool abForceRemoval);
	// Stops the fade timer of apPointer.
	static void ClearTimer(void* apPointer);
	// Fades apNode and its children to afAmount.
	static void RecursiveFade(NiNode* apNode, float afAmount, float afLastAlpha);

	// True while any menu is up.
	static bool IsInMenuMode();
	static void ShowMenus();

	// Prints a formatted line to the console while it is shown (cdecl, varargs, 0x703C00).
	template <class... A> static void PrintLine(const char* apFormat, A... aArguments) {
		// The C-variadic overload by pointer: a plain call would pick this template again.
		void (*pVariadic)(const char*, ...) = &PrintLine;
		pVariadic(apFormat, aArguments...);
	}

	// The body of the forwarder above; see Interface.cpp for why it is not registered.
	static void PrintLine(const char* apFormat, ...);
	// The same line, printed from a string object the exe passes by value (cdecl, varargs, 0x703C80); the body is
	// not registered either, see Interface.cpp.
	static void PrintLine(BSStringT<char> aString, ...);

	// Queues a message box with the given text, icon, title, result callback and buttons (axFirst, then more
	// button strings, null-terminated); false when there is no interface (cdecl, varargs, 0x703E80).
	template <class... A>
	static bool CreateMessageMenu(const char* aText, const char* apcIconFilename, const char* apcTitle,
		void (*afpResult)(), int32_t aiFirstButtonResult, int32_t aiWarningContext,
		float afBackgroundTextureOpacityOverride, float afMenuWidthOverride, const char* axFirst, A... aButtons) {
		// The C-variadic overload by pointer: a plain call would pick this template again.
		bool (*pVariadic)(const char*, const char*, const char*, void (*)(), int32_t, int32_t, float, float,
			const char*, ...) = &CreateMessageMenu;
		return pVariadic(aText, apcIconFilename, apcTitle, afpResult, aiFirstButtonResult, aiWarningContext,
			afBackgroundTextureOpacityOverride, afMenuWidthOverride, axFirst, aButtons...);
	}

	// The body of the forwarder above; see Interface.cpp for why it is not registered.
	static bool CreateMessageMenu(const char* aText, const char* apcIconFilename, const char* apcTitle,
		void (*afpResult)(), int32_t aiFirstButtonResult, int32_t aiWarningContext,
		float afBackgroundTextureOpacityOverride, float afMenuWidthOverride, const char* axFirst, ...);
	// The same message box with a sound name for the entry (cdecl, varargs, 0x703F10); see Interface.cpp for why the
	// body is not registered.
	static bool CreateMessageMenuSound(const char* aText, const char* apcIconFilename, const char* apcTitle,
		const char* apcSound, void (*afpResult)(), int32_t aiFirstButtonResult, int32_t aiWarningContext,
		float afBackgroundTextureOpacityOverride, float afMenuWidthOverride, const char* axFirst, ...);
	// Queues a modal message box and pumps the message queue until its result callback answers (cdecl, varargs,
	// 0x704010); returns the callback's byte. See Interface.cpp for why the body is not registered.
	static char CreateModalMessageMenu(const char* aText, int32_t aiFirstButtonResult, int32_t aiWarningContext,
		const char* axFirst, ...);
	// Closes the message box at the head of the queue.
	static void CloseMessageMenu();

	// The roots the menus and the interface's own scene graph hang from.
	static Tile* GetMenusRoot();
	static NiNode* GetInterfaceRoot();

	// True when the interface is rendered into the pip-boy's rendered menu.
	static bool IsInRenderedMenu();
	// True when the current rendered menu is the topmost menu.
	static bool IsCurrentRenderedMenuTopmost();
	// Builds a tab button for each text (axFirstText, then more, null-terminated) under apTabline with ids from
	// aiStartingID (cdecl, varargs).
	static void InitializeTabline(Tile* apTabline, int32_t aiStartingID, const char* axFirstText, ...);

	// The pip-boy manager, the VATS effect manager and the currently rendered menu.
	static FOPipboyManager* GetPipboy();
	static FOVATSEffectManager* GetVATSEffect();
	static FOHUDEffectManager* GetHUDEffects();
	static FORenderedMenu* GetCurrentRenderedMenu();

	// True while a pip-boy screen is opening or already open.
	static bool IsInPipboyMenu();
	static bool IsPipBoyOpening();
	static bool IsPipBoyOpened();

	// The debug text overlay and the console.
	static bool IsDebugTextVisible();
	static bool IsConsoleVisible();
	static TESObjectREFR* GetConsoleSelection();

	// The tile the crosshair is over, and the reference the interface last activated.
	static Tile* GetCursorTile();
	// Drops every reference the interface holds to a tile that is being freed.
	static void TileIsBeingDeleted(Tile* apTile);
	static void RefreshActivateREFR();

	// How far the player's pick reaches.
	static void SetPickLength(uint32_t auiLength);

	// The tile the interface gives focus to.
	static void SetCurrentFocusTarget(Tile* apTile);

	// True while a message menu is up.
	static void DisableMessageMenu(bool abDisable);
	// The loading menu's visibility byte as IsMenuVisible returns it (cdecl, 0x705E80): the exe's callers get its
	// eax, the table's raw byte, so the result is a char, not a bool.
	static char GetLoadingMenuVisible();

	// Fades a menu in or out over the given number of seconds.
	static void AddMenuFade(Menu* apMenu, float afSecs);

	// Drops the interface's per-frame update list and timers.
	static void UpdateAllTimers();
	static void SetNeedToUpdate(bool abSet);

	// Hands the interface's geometry to the renderer, unless a movie is playing.
	static void Accumulate();

	// Attaches the scene's camera to an object, and frees the interface's temporary model.
	static void AddAlphaProperty(NiAVObject* apObject);
	static void ClearTempModel();
	// Loads a model file (through the model loader when the game has one; arbFromModelLoader says so), and
	// copies a model that already has a parent.
	static NiNode* LoadModelFile(const char* apFilename, bool& arbFromModelLoader);
	static NiNode* CopyOrDeepCopyNode(NiNode* apNode);
	// Plays the sounds of an interface model's animation sequence (BSAnimGroupSequence::PlaySounds) and returns
	// its result.
	static bool PlayNifSounds(NiControllerSequence* apSequence);

	// True when a dialog is up.
	static bool InDialog();
	// (IDA's HandlePauseGame; the PDB's SetDialogPause by its place among the dialog
	// functions). Pauses or resumes the dialog menu's game pause (cdecl, 0x705110).
	static void SetDialogPause(bool abPause);
	// Clears the HUD's messages and subtitles once the interface manager has finished its first init.
	static void ClearAllSubtitlesAndMessages();
	// Queues a message for the HUD's message area.
	static bool ShowMessage(const char* apString, int32_t aeEmotion, const char* apIconFilename,
		const char* apSoundName, float afShowSeconds, bool abMoveToHead);
	// Shows xDisplayText as apRegion's name in the HUD's region text once the interface manager has finished its
	// first init (bUpdateLastRegion: only when apRegion is not the last region shown, which it becomes).
	static void SetRegionText(TESRegion* apRegion, BSStringT<char> xDisplayText, bool bUpdateLastRegion);
	// Opens the sleep (abSleep) or wait menu; returns
	// SleepWaitMenu::Create's tile (its menu file tile), null when it is not created.
	static void CreateSleepMenu(bool abSleep);

	// The menus the interface creates on request; each returns null when the interface is not up.
	static Tile* CreateMapMenu();
	static Tile* CreateRepairMenu();
	static Tile* CreateItemModMenu();
	static Tile* CreateInventoryMenu();
	static Tile* CreateStatsMenu();
	static void CreateStartMenu();
	static void CreateLoadingMenu(TESWorldSpace* apWorldSpace, bool abScripted);
	// Destroys the loading menu when one is up, and re-enables the message menu.
	static void CloseLoadingMenu();

	// Closes the container menu, and releases the interface's textures.
	static void CloseContainer();
	static void ForceTextureRelease();
	static bool GetFirstChanceTextureRelease();
	// Sets the interface manager's first-chance (emergency) texture release flag.
	static void SetFirstChanceTextureRelease(bool abSet);
	// Drops the interface manager's pick references to apObject, once the manager has initialised.
	static void ClearReferencePointers(TESObjectREFR* apObject);

	// The per-frame work the interface does around the menus' own updates.
	static void PreIdleStuff();
	static void Idle();
	static void PostIdleStuff();
	static void LastMinuteUpdate();
	// Hides every menu.
	static void HideMenus();
	// Leaves only the glow and/or pip-boy elements of the menus visible.
	static void IsolateMenuElements(bool abGlowOnly, bool abPipBoyOnly);
	// Undoes IsolateMenuElements.
	static void RestoreMenuElements();
	// Has the interface manager, when it exists and is set up, close every menu at once.
	static void EmergencyCloseAllMenusAndBreakStuff();

	// Rebuilds the pip-boy's manager after a game load.
	static void ResetPipboy();

	// Builds a menu of the given class through the menu manager.
	static Menu* CreateMenuByClass(int32_t aiMenuClass);

	// The page each main menu opens on.
	static void SetMainMenuDefaults(char aiStatsTab, char aiInventoryTab, char aiMapTab, int32_t aiWhichMenu);
	// The exe's 0x704DF0 reads no ecx (cdecl).
	static char GetStatsMenuVisible();
	static void SwapBuffers();
	static void UpdateScreenShape(int32_t isizex, int32_t isizey);
	static void SetMenuSystemVisible(bool abVisible);
	static void ToggleSafeZone(int32_t abForceSetTo);
	// Returns IsMenuVisible's byte as is: callers use the whole of eax.
	static char GetInventoryMenuVisible();
	static void DumpMenuPackingInfo();
	static char GetStatsPageNumber();
	static char GetMagicPageNumber();
	static char GetMapPageNumber();
	static void SetSurgeryCursorPos(int32_t aiXPos, int32_t aiYPos);
	static NiNode* GetDebugTextRoot();
	static void ReleaseModelFile(const char* apFilename);
	static void DrawSurgeryMenu3D();
	static bool GetIsVATSRescanning();
	static bool IsActivatePickFuzzy();
	static void ClearMenuQueue();
	// a2 is the reference's address (pCrossHairRef), pointer-sized.
	static void SetCrosshairRef(intptr_t a2);
	static void Free();
	static void CreateRecipeMenu(TESObjectREFR* apVendor, TESForm* pCategory);
	// Inferred type: a2: float from 0x709b62 (passed to InterfaceManager::SetQueuedPipboyTab 1); a3: float from
	// 0x709b5b (passed to InterfaceManager::SetQueuedPipboyTab 2)
	static InterfaceManager* SetQueuedPipboyTab(uint8_t a1, float a2, float a3, int32_t a4);
	static void MessageMenuYesToAll();
	static TESObjectREFR* GetActivateREFR();
	static void CloseSleepMenu();
	static void CloseVATSMenu();
	// Closes every support menu by removing its tile from the menus root. Every caller
	// pushes abExceptSubtitleMenu (0); the PC body never reads it.
	static void CloseAllSupportMenus(bool abExceptSubtitleMenu);
	static void SaveGameTES();
	static bool ContainerIsRecipe();
	static bool ContainerIsBarter();
	static void SetDragDroppingState(int32_t abSetTo);
	static void UpdateInventory();
	static void SetItemModMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect);
	static int32_t GetDragDroppingState();
	static void GetControlPushTextFor(int32_t aiIndex, char* axReturn, bool abAddPushText);
	static void IsolateElementsOfMenu(Tile* apMenuTile, bool abGlowOnly, bool abPipBoyOnly,
		bool abUpdateGlowBranchList);
	static void SetTargetREFR(TESObjectREFR* apRef);
	static NiPoint3* GetActivatePickLocation(NiPoint3* apResult);
	static void SetFirstChanceTextureRelease_00703920();
	static void SetRepairMenuVisible(bool abVisibility, bool abAnyVisibleState, bool abTriggerPipboyEffect);
	static void CreateRepairServicesMenu(Actor* apVendor);
	static void CreateVATSMenu();
	static bool SetInfoForRef(TESObjectREFR* apObject, bool abTelekinesis, bool abSameOkay);
	static void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	static void ShowDDTIcon(bool abArmorPiercing);
	static void ShowEnemyDDTIcon(bool abArmorPiercing);
	static int16_t GetSaveSize();

	// The model a 3D item preview is being built into; ClearTempModel frees it.
	static inline Model* pTmpModel;
	// Set while InterfaceManager::LastMinuteUpdate runs.
	static inline bool bLastMinuteUpdate;
	// Set while a new game is being loaded.
	static inline bool bIsLoadingNewGame;
	// Set while a dropped item's cursor shows; Idle restores the cursor texture and clears it.
	static inline bool bIsDroppingInInventory;
	// Set when the level up menu must open; Idle opens it once the game has the input.
	static inline bool bNeedLevelUpMenu;
	// The service the player is being offered (the GetWhichService condition returns it); -1 for none.
	static inline int32_t iServiceFlag = -1;
	// Whether the escape action may open the pause menu.
	static inline bool bAllowPauseMenu = true;
	// The queued menu request, the queued message request, and the lock that guards both.
	static QUEUED_OPEN QueuedMenuOpen;
	static QUEUED_OPEN QueuedMessageOpen;
	static BSSpinLock QueuedMenuLock;
	static inline bool bIsFastTraveling;
	static inline bool bStartPipboyRenderBackground;
};

// The settings holding the names of the 28 user actions.
using UserActionSettingArray = Setting* [28];
extern UserActionSettingArray psUserActions;

// Set while CloseAllSupportMenus walks the menus root, so a menu's own close code knows why it is going.
inline uint8_t bClosingAllSupportMenus;

// Set while the message menu is disabled.
inline uint8_t bMessageMenuDisabled;
using pSexNameGS_t = SettingT<GameSettingCollection>* [2];
extern pSexNameGS_t pSexNameGS;
using pSexPossessiveGS_t = SettingT<GameSettingCollection>* [2];
extern pSexPossessiveGS_t pSexPossessiveGS;
using pSexPronounGS_t = SettingT<GameSettingCollection>* [2];
extern pSexPronounGS_t pSexPronounGS;

// IDA name; not in the PDB. The byte a modal message box's result callback (ModalMessageMenuCallback, 0x704138)
// stores; -1 until the user answers.
inline int8_t iModalMessageMenuResult = -1;

// IDA names; not in the PDB.
