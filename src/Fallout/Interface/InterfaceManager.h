#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSMain/Setting.h"
#include "BSMenu/Menu.h"
#include "Fallout/Interface/FOHUDEffectManager.h"
#include "Fallout/Interface/FOVATSEffectManager.h"
#include "Fallout/Interface/Interface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <stdint.h>

class Actor;
class BSCullingProcess;
class BSShaderAccumulator;
class FOPipboyManager;
class FORenderedMenu;
class NiDirectionalLight;
class SceneGraph;
class ShadowSceneNode;
class TESImageSpaceModifier;
class TileImage;
class TileMenu;
class ViewCaster;

enum KEY_EVENT_TYPE : int32_t;

// Has360Controller widened to a BOOL.
int32_t GetIsControllerConnected();
// Queues a rendered menu for deletion on the main thread.
void QueueRenderedMenuDelete(FORenderedMenu* pRenderedMenu);
// The controller key a special input key (the menus' gamepad codes) stands for.
int32_t ConvertSpecialInputKeyToControllerKey(int32_t aiKey);
// The menu height over the interface height, rounded to float.
float GetMenuHeightRatio();
// Flips the full-help flag on the interface singleton when the menus are shown.
void ToggleFullHelp();
// Whether the full help is on and the menus are shown.
bool GetIsFullHelp();
// Whether the map menu shows the fog of war; true without a map menu.
bool GetMapMenuFogOfWar();
// Sets the fog of war on the open map menu.
void SetMapMenuFogOfWar(bool abFogOfWar);
// The size of the Eyefinity screen the game draws on.
float GetEyefinityScreenWidth();
float GetEyefinityScreenHeight();
// True while the display spans several monitors.
bool IsEyefinity();
// Opens the barter menu for apRef when the menus are shown.
TileMenu* ShowBarterMenu_(TESObjectREFR* apRef, int32_t aiMode);
// The mode of the open container menu, 0 without one.
uint32_t GetContainerMenuMode();
// Opens the lock pick menu when the menus are shown. The script commands pass null as the first argument.
TileMenu* _ShowLockPickMenu(TESObjectREFR* apRef, int32_t aiLockLevel, bool abFlag);
// Deletes the rendered menus queued for deletion, under the rendered-menu critical section.
void ClearRenderedMenuDeleteList();
uint32_t MakeRGBA(uint32_t aiRed, uint32_t aiGreen, uint32_t aiBlue, uint32_t aiAlpha);
int32_t SplitColorComponents(int32_t aiColor, uint32_t* apRed, uint32_t* apGreen, uint32_t* apBlue, uint32_t* apAlpha);
void RecursivelySetVisibilityDirty(Tile* apTile);

// The interface sounds PlayMenuSound plays (an index into the interface's sound list); MENU_SOUND_NONE plays none.
enum MENU_SOUNDS : int32_t {
    MENU_SOUND_NONE = -1,
    MENU_SOUND_OK = 1,
    MENU_SOUND_CANCEL = 2,
    MENU_SOUND_NEXT_PREVIOUS = 3,
    MENU_SOUND_FOCUS = 4,
    MENU_SOUND_BOOK_NEXT_PAGE = 5,
    MENU_SOUND_SPEECH_ROLLOVER = 6,
    MENU_SOUND_SPEECH_ROTATE = 7,
    MENU_SOUND_QUEST_NEW = 8,
    MENU_SOUND_QUEST_UPDATE = 9,
    MENU_SOUND_MESSAGE = 10,
    MENU_SOUND_CLICK = 11,
    MENU_SOUND_WHIR = 12,
    MENU_SOUND_WHIR_CLICK = 13,
    MENU_SOUND_FADE = 14,
    MENU_SOUND_INVENTORY_OPEN = 15,
    MENU_SOUND_INVENTORY_CLOSE = 16,
    MENU_SOUND_POTION_CREATE = 17,
    MENU_SOUND_DOOR_LOCKED = 18,
    MENU_SOUND_PLACE_TOKEN = 19,
    MENU_SOUND_REMOVE_TOKEN = 20,
    MENU_SOUND_LEVEL_UP = 21,
    MENU_SOUND_SPELL_EQUIP = 22,
    MENU_SOUND_ENCHANTMENT_RECHARGE = 23,
    MENU_SOUND_SCROLL_OPEN = 24,
    MENU_SOUND_SCROLL_CLOSE = 25,
    MENU_SOUND_BOOK_OPEN = 26,
    MENU_SOUND_BOOK_CLOSE = 27,
    MENU_SOUND_TAKE_ALL = 28,
    MENU_SOUND_NOTHING_HARVESTED = 29,
    MENU_SOUND_INGREDIENT_SELECTED = 30,
    MENU_SOUND_SOUL_TRAP = 31,
    MENU_SOUND_HAMMER_BREAK = 32,
    MENU_SOUND_SIGIL_CREATE = 33,
    MENU_SOUND_GOLD = 34,
    MENU_SOUND_ENCHANT = 35,
    MENU_SOUND_MODE = 36,
};

// The interface singleton: scene graphs, cursor, picking, the menus and their modes.
class InterfaceManager {
public:
    // The interface modes (cMenuMode). eMenuModeOff is the game itself.
    enum eMenuModeType : int32_t {
        eMenuModeDisabled = 0,
        eMenuModeOff = 1,
        eMenuModeOn = 2,
        eMenuModeEntering = 3,
        eMenuModeLeaving = 4,
        eMenuModeFirstPass = 5,
    };

    // The pip-boy's opening and closing states (ePipboyState).
    enum PIPBOY_STATE : int32_t {
        PS_CLOSED = 0,
        PS_OPENING_SETUP = 1,
        PS_OPENING = 2,
        PS_OPEN = 3,
        PS_CLOSING_SETUP = 4,
        PS_CLOSING = 5,
    };

    // A menu fade timer: the menu, its elapsed and total time, in a doubly linked list.
    struct Timer {
        // Constructs a timer in apMemory (0x14 bytes): no menu, 0 elapsed, 0.001 s.
        Timer();

        Menu* pIndex;
        float fElapsed;
        float fEnd;
        Timer* pPrev;
        Timer* pNext;
    };

    // The tutorial messages: flags, menu and delay per message, and the message waiting to be shown with its time.
    class TutorialManager {
    public:
        // One message's state, packed in 32 bits (TutorialManager::ShowMessage's masks).
        struct MessageData {
            uint32_t bShowMessage : 1;
            uint32_t bMessageShown : 1;
            // The menu class less 0x3E9 (Interface::eMessageMenu).
            uint32_t uiMenuOffset : 6;
            uint32_t uiShowDelay : 24;
        };

        TutorialManager();

        // Queues tutorial message aeID for the menu class aiMenuID after auiShowDelay; returns whether it will be
        // shown.
        bool ShowMessage(Interface::TutorialMessageID aeID, int32_t aiMenuID, uint32_t auiShowDelay);
        // Whether tutorial message aeID has been shown.
        bool HasBeenShown(Interface::TutorialMessageID aeID);
        // Restores the messages' shown flags from the saved string apLoadString.
        void Load(char* apLoadString);
        // The size of the tutorial data in a save.
        uint32_t GetSaveLoadSize();
        void Save(char* apSaveString);
        // Opens the queued tutorial message's menu once it is due.
        void Update();
        void Reset();
        bool FlagAsShown(Interface::TutorialMessageID aeID);
        Interface::TutorialMessageID Find(Interface::enumMenus aeMenu);

        MessageData tMessages[41];
        Interface::TutorialMessageID eDisplayID;
        uint32_t uiNextDisplay;
    };

    // Every tile that needs a per-frame update.
    using TileUpdateArray = BSSimpleArray<Tile*, 1024>;

    InterfaceManager();
    ~InterfaceManager();

    // Toggles the debug text.
    void ToggleDebugTextVisible();
    // Whether the mouse cursor is shown or no controller is connected.
    bool GetMouseVisible();
    InterfaceManager*
    SetQueuedPipboyTab(uint8_t aucOverPipBoyScreen, float afMouseX, float afMouseY, uint32_t auiMenuToSet);
    void HandleControllerStateChange(int32_t aiState);
    char GetMapPageNumber();
    void DumpMenuPackingInfo();
    void ToggleSafeZone(int32_t abForceSetTo);
    void UpdateScreenShape(int32_t isizex, int32_t isizey);
    void SetCursorPos(float afX, float afY);
    void EmergencyCloseAllMenusAndBreakStuff();
    void ClosePipboy(void (*apCallback)());
    int32_t GetMouseWheel();
    // True while a menu (rather than the game, mode 1) has the input focus.
    bool IsInMenuMode();
    // The current interface mode's low byte.
    uint8_t GetCurrentMode();
    // Sets the interface mode (cMenuMode).
    void InternalSetMenuMode(eMenuModeType aiMode);
    // The active stats and magic tab of their menus.
    int8_t GetStatsPageNumber();
    int8_t GetMagicPageNumber();
    void SetStatsPageNumber(char acTab);
    void SetInventoryPageNumber(char acTab);
    // Sets the inventory tab to show; InventoryMenu.cpp's own copy of SetInventoryPageNumber.
    void UpdateInventoryPageNumber(char acTab);
    void SetMapPageNumber(char acTab);
    // The debug text overlay.
    bool IsDebugTextVisible();
    void SetDebugTextVisible(bool abVisible);
    // The interface scene graph roots.
    NiNode* GetCursorRoot();
    NiNode* GetDebugTextRoot();
    // The console's picked reference, and the pipboy manager.
    TESObjectREFR* GetConsoleSelection();
    FOPipboyManager* GetPipboy();
    // True when the renderer asked for the emergency texture release.
    bool GetFirstChanceTextureRelease();
    // How far the player's pick reaches.
    void SetPickDistance(int32_t aiDistance);
    // The cursor's tile.
    TileImage* GetCursorTile();
    // The cursor's x in the pip-boy screen's coordinates.
    float GetPipboyMouseX();
    // The cursor's y in the pip-boy screen's coordinates.
    float GetPipboyMouseY();
    // The root tile of the string (globals) tiles.
    Tile* GetStringRoot();
    // Forgets the tile under the mouse cursor.
    void ClearCurrentFocusTarget();
    // Does not actually set the current main menu: it only rewrites its dead argument.
    void SetCurrentMainMenu(int32_t aiMenu);
    // Whether the interface geometry must be rebuilt this frame.
    void NeedToUpdate(bool abNeedToUpdate);
    // The menu-class stack the interface entered through.
    int32_t GetEnterStack(int32_t aiIndex);
    int32_t GetEnterStackTop();
    // True while the tile's menu accepts input (its menu is topmost, or no menu is).
    bool TileIsAcceptingEvents(Tile* pTile);
    // Closes the support menus queued for closing.
    void UpdateQueuedMenuClosings();
    // The menu the rendered pass is showing, or the pipboy manager while a pipboy menu is up.
    FORenderedMenu* GetCurrentRenderedMenu();
    bool IsInRenderedMenu();
    // The per-frame list of tiles to update.
    void AddTileToUpdateList(Tile* apTile);
    void RemoveTileFromUpdateList(Tile* apTile);
    // Stores the flag at bClickMultithreaded.
    void SetClickMultithreaded(bool abValue);
    // Releases every texture the interface can recreate.
    void ForceTextureRelease();
    // Updates the model of every tile in the per-frame update list; the main thread then empties the list.
    void UpdateTileModels();
    // Starts the multithreaded accumulation of the interface scene graph.
    void Accumulate();
    // Draws the interface into the menu texture (both arguments are unused).
    void Click(BSRenderedTexture* ap3DTarget, BSRenderedTexture* apMenuFXTexture);
    // Pushes a menu class onto the enter stack; its slot index, or -1 when the stack is full.
    int32_t AddToEnterStack(int32_t aiID);
    // Removes a menu class from the enter stack; the new top, 0 when empty, -1 or -2 when refused.
    int32_t PopFromEnterStack(int32_t aiID, bool abForceRemoval);
    // Translates a key event (KEY_EVENT_DOWN or KEY_EVENT_UP) into the menu key code, tracking the shift/alt/control
    // modifiers.
    int32_t GetChar(KEY_EVENT_TYPE aiEventType, int32_t aikey);
    // Forgets the tile the mouse is over, leaving it first when abDoLeave is set.
    void ClearOverTileTarget(bool abDoLeave);
    // Moves the mouse-over or the click focus (aiType is kTileValue_mouseover or kTileValue_clicked).
    void SetCurrentFocusTarget(Tile* apTarget, int32_t aiType, bool abPlaySounds);
    // Focuses the tile with the highest default focus of the front menu.
    void GetDefaultFocus();
    // The target tile with the highest default focus under apTarget (the front menu when null).
    Tile* ScanForMaxFocus(int32_t& aiMaxFocus, Tile* apTarget);
    // Drops every pick reference that is apRef.
    void ClearReferencePointers(TESObjectREFR* apRef);
    // Advances the menu fade timers and frees the finished ones.
    void UpdateAllTimers();
    // Builds the pip-boy menus once and hides them.
    void PreLoadMainMenus();
    // Mouse-over events, offered to the active list boxes first.
    void DoEnter(Menu* apMenu, int32_t aiID, Tile* apTarget);
    void DoLeave(Menu* apMenu, int32_t aiID, Tile* apTarget);
    // A gamepad button for the menu, then for its active list boxes; true when consumed.
    bool DoGamepad(Menu* apMenu, Interface::enumGamepadButtons aiButton, float afValue);
    // Switches the rendered menu (null leaves the current one).
    void EnterRenderedMenu(FORenderedMenu* apRenderedMenu);
    // The imagespace modifier the current menu wants, or null.
    TESImageSpaceModifier* GetCurrentImagespaceMod();
    void SetRepeatingKey(int32_t aiKey);
    // Init stage 1: the menu root, the globals tiles, the debug text root and the pip-boy manager.
    void Init(bool abInitialLoad);
    // Init stage 2: fonts, the cursor tile, the shadow scene nodes and the HUD.
    void InitSecond();
    // Per-frame menu-mode bookkeeping before Idle: entering menu mode and the forced menu mode pauses.
    void PreIdleStuff();
    // The per-frame menu work: input, cursor, menus and tiles.
    void Idle();
    // Picks the reference the player can activate (and grab with telekinesis) along the view ray.
    void RefreshActivateREFR();
    // Steps the pip-boy opening/closing state machine.
    void UpdatePipboy();
    // Routes a 360 controller event (0: read the pad) to the menus.
    void DoGamepadEvent(Interface::enumGamepadButtons aiButton);
    // Turns the 360 controller state into a menu key code; *apbReleased is set for a button release.
    Interface::enumGamepadButtons
    GetTopGamepadButton(bool abRepeatVertical, bool abRepeatHorizontal, bool& abIsUpEvent);
    // Moves the cursor tile by the mouse (or the 360 thumbstick) and records its interface and pixel position.
    void UpdateCursor();
    // Per-frame menu fades after Idle, and leaving menu mode once the fades are done.
    void PostIdleStuff();
    // Fades apObject's tiles' geometry to afAmount of their alpha (afLastAlpha: the parent tile's alpha, 0..1).
    void RecursiveFade(NiAVObject* apObject, float afAmount, float afLastAlpha);
    // The menu tile under the cursor (only a target tile when abExactTargetOnly), or null.
    Tile* PickMenus(bool abExactTargetOnly);
    // Sets up an interface scene graph (a new one named apName when apSceneGraph is null) with its camera and roots.
    SceneGraph* InitSceneGraph(SceneGraph* apSceneGraph, const char* apName, bool abIs3D);
    // Replaces the pip-boy manager with a new, initialised one.
    void ReinitialisePipboyManager();
    // Accumulates and renders the interface scene graph (abPipboyVisible is unused).
    void DrawInterface_Screen(BSCullingProcess* apCuller, bool abPipboyVisible);
    // Updates every tile (and the tile models in the update list) just before the interface is drawn.
    void LastMinuteUpdate();
    // The interface scene graph.
    SceneGraph* GetSceneGraph();
    SceneGraph* Get3DSceneGraph();
    Tile* GetMenusRoot();
    // The 3D interface accumulator, sp3dInterfaceAccum's object.
    BSShaderAccumulator* Get3dAccumulator();
    // True once init stage 1 has run (bFirstInit).
    bool IsDoneFirstInit();
    NiNode* GetInterfaceRoot();
    NiAlphaProperty* GetAlphaProp();
    // The embedded VATS effect manager, VATSEffect.
    FOVATSEffectManager* GetVATSEffect();
    int32_t GetCursorY();
    // Reacts to the console's selected reference changing.
    void DisplayCurrentPickRef();
    // Opens the pip-boy on the menu aeMenuToOpen (0 for the last one) and calls apCallback
    // once it is open.
    void OpenPipboy(void (*apCallback)(), Interface::enumMenus aeMenuToOpen);
    // Collects the references under the cursor into
    // ConsolePickRefs for the console.
    void GetRefsUnderCrosshair();
    // The repeating key's code when its repeat is due, else 0.
    int32_t GetShouldKeyBeRepeated(float afRepeatScale);
    // Scrolls the list box (or the menu) of apTarget by the mouse wheel. apMenu is InterfaceManager::pOverTileMenu;
    // aiID is the result of Tile::GetFloat.
    void HandleMenuAndListboxScroll(Menu* apMenu, float aiID, Tile* apTarget);
    // The tile under the mouse cursor (pMouseOverTarget).
    Tile* GetCurrentFocusTarget();
    // The dragged tile's offset (iCursorOffsetX/Y).
    int32_t GetCursorOffsetX();
    int32_t GetCursorOffsetY() {
        return iCursorOffsetY;
    }
    void SetCursorOffsetX(int32_t aiCursorOffsetX) {
        iCursorOffsetX = aiCursorOffsetX;
    }
    void SetCursorOffsetY(int32_t aiCursorOffsetY) {
        iCursorOffsetY = aiCursorOffsetY;
    }
    bool IsAltDown();
    bool IsShiftDown();
    bool IsActivatePickFuzzy();
    NiPoint3 GetActivatePickLocation();
    void SetFirstChanceTextureRelease(bool abState);
    char GetInventoryPageNumber();
    void SetTargetREFR(TESObjectREFR* apRef);
    bool IsMenuInStack(uint32_t aiMenuID);
    void PreLoadMenus();
    void ClearTargetREFR();
    int32_t GetCursorX();
    // The pip-boy tab last shown: the root tile's value 0x1771, converted to an integer.
    int32_t GetLastPipboyTab();
    // aiCrosshairRef is the reference's address (pCrossHairRef), pointer-sized.
    void SetCrosshairRef(intptr_t aiCrosshairRef);
    void SetLockpickControlsDisable(bool abDisable);
    void SetDragDroppingState(int32_t aiSetTo);
    uint32_t GetDragDroppingState();
    void Disable360Controller(bool abDisable);

protected:
    void FreeTimerList();

public:
    static void Free();
    static TESObjectREFR* GetTelekinesisTarget();
    // The interface singleton, created by InterfaceManager::Initialize.
    static InterfaceManager* Instance();
    // The string tile root that globals render through.
    static Tile* GetGlobalsTile();
    // True while the lockpick controls are disabled for the player.
    static bool QLockpickControlsDisable();
    // True while a container menu waits to open.
    static bool GetContainerMenuQueued();
    // True while an Xbox 360 controller is the active input device.
    static bool Has360Controller();
    // The renderer's screen width and height in pixels, as floats for the tiles.
    static float GetRealScreenWidth();
    static float GetRealScreenHeight();
    // The vertical safe-zone margin in pixels, as a float.
    static float GetScreenSafeY();
    // The horizontal safe-zone margin in pixels, as a float.
    static float GetScreenSafeX();
    // The interface's virtual screen size: 1280x960, widened (or heightened) to the real aspect ratio.
    static float GetScreenWidth();
    static float GetScreenHeight();
    // True while a pipboy menu is on top and the console is closed.
    static bool IsPipboyMenuTopmost();
    // The tile and menu the mouse is over.
    static Tile* GetOverTileTarget();
    static void SetOverTileTarget(Tile* apTile);
    static Menu* GetOverTileMenu();
    static void SetOverTileMenu(Menu* apMenu);
    // Starts (or restarts) the fade timer of a menu. The menu is an opaque pointer; the bodies only compare it with
    // Timer::pIndex.
    static void NewTimer(void* apIndex, float afSecs);
    static void ClearTimer(void* apIndex);
    // How far the menu's fade has run, from 0 to 1; 1 without a timer, -1 for a zero-length one.
    static float GetTimerPercent(void* apIndex);
    static void SetCursorAlpha(int32_t iAlpha);
    // True while a pip-boy menu is visible.
    static bool IsInPipboyMenu();
    static bool IsCurrentRenderedMenuTopmost();
    // The 360 controller state of a button: 0 held, 1 released, 2 pressed, -1 up.
    static int32_t CheckMenuButton(Interface::enumGamepadButtons eAction, bool abUseThumbstickInput);
    // Creates the singleton on first use, then runs init stage 1 (Init) or 2 (InitSecond); resets the pauses.
    static void Initialize(uint32_t istage, bool abInitialLoad);
    // The screen's left and right edges, half the interface width.
    static float GetScreenLeft();
    static float GetScreenRight();
    // The screen's top and bottom edges, half the interface height.
    static float GetScreenTop();
    static float GetScreenBottom();
    // The menus' design height, an int global (768).
    static int32_t GetMenuHeight();
    // Plays one of the interface sounds.
    static void PlayMenuSound(MENU_SOUNDS aeMenuSound);
    // Forgets a 360 controller button press.
    static void ClearMenuButton(Interface::enumGamepadButtons eAction);
    static void operator delete(void* apMemory);
    static void SetContainerMenuQueued(bool abQueued);
    static bool IsPipboyClosed();
    static float GetScreenAspect();
    static void UpdatePipBoyInventoryEffects(
        Tile* pTileFade1,
        Tile* pTileFade2,
        Tile* pTilePulseSTR,
        ItemChange* pItemPulseSTRREQ,
        bool abSkill
    );

    bool bFirstInit;
    bool bSecondInit;
    // The interface scene graphs.
    NiPointer<SceneGraph> spSceneGraph;
    NiPointer<SceneGraph> sp3DSceneGraph;
    // The current interface mode; eMenuModeOff is the game itself.
    eMenuModeType cMenuMode;
    bool bExternalForcedMenuMode;
    bool bLockMenuModeForFade;
    // The active tab of the stats, inventory, magic and map menus.
    int8_t cStatsPageNumber;
    int8_t cInventoryPageNumber;
    int8_t cMagicPageNumber;
    int8_t cMapPageNumber;
    int32_t iPickDistance;
    int32_t iTelekinesisDistance;
    // How many callers paused the animation sounds and the world voices.
    uint8_t iSoundPauses;
    uint8_t iVoicePauses;
    NiDirectionalLight* pPlayerLight;
    TileImage* pCursor;
    NiPoint3 sCursorPos;
    NiPoint3 sRealCursorPos;
    float fMouseWheel;
    float fMouseHeldTime;
    Tile* pDragTarget;
    int32_t iDragStartX;
    float fDragOffsetLastX;
    float fDragOffsetLastY;
    int32_t iDragStartY;
    int32_t iCursorOffsetX;
    int32_t iCursorOffsetY;
    int32_t iTargetID;
    Tile* pTarget;
    // The references the console can pick, and the picked one's index (-1 for none).
    BSSimpleList<TESObjectREFR*> ConsolePickRefs;
    int32_t iCurrentPickIndex;
    bool bDebugTextVisible;
    bool bShowMouse;
    NiNode* pInterfaceRoot;
    NiNode* pCursorRoot;
    ShadowSceneNode* pPlayerRoot;
    NiPointer<BSShaderAccumulator> spInterfaceAccum;
    NiPointer<BSShaderAccumulator> sp3dInterfaceAccum;
    ShadowSceneNode* pShadowNode;
    ShadowSceneNode* pUIPlayerNode;
    // The root tile of the menus.
    Tile* pMenusRoot;
    Tile* pStringRoot;
    NiNode* pDebugTextRoot;
    float fOneToOneDistance;
    NiPointer<NiAlphaProperty> spAlphaProp;
    bool bNeedToUpdate;
    int16_t iCharHit;
    Tile* pTargetReticle;
    Tile* pSafeZone;
    // The tile under the mouse cursor.
    Tile* pMouseOverTarget;
    int32_t iLastXDefault;
    int32_t iDragDroppingState;
    bool bPreLoadMainMenus;
    bool bFirstChanceLoad;
    Tile* pOverTileTarget;
    Menu* pOverTileMenu;
    // The tile being dragged over, and its menu.
    Tile* pDragOverTileTarget;
    Menu* pDragOverTileMenu;
    bool bTextWasCreated;
    bool bFullHelp;
    Menu* pTopMenu;
    char cLastMessageButtonClicked;
    void (*fpResult)();
    bool bTextureRelease;
    bool bMouseInMotion;
    TESObjectREFR* pPickRef;
    TESObjectREFR* pReticleRef;
    TESObjectREFR* pCrossHairRef;
    TESObjectREFR* pActivateRef;
    TESObjectREFR* pTelekinesisRef;
    // Where the activate pick ray hit.
    NiPoint3 pActivatePickLocation;
    bool bFuzzyActivatePick;
    // The classes of the menus the interface entered through (enum Interface::Menus).
    int32_t iEnterStack[10];
    ViewCaster* pViewCaster;
    MobileObject* pJailSpeaker;
    MobileObject* pPayFineSpeaker;
    // True while a click is handled off the main thread.
    bool bClickMultithreaded;
    bool bShiftDown;
    enum : int32_t {
        MODIFIER_ALT = 1,
        MODIFIER_CONTROL = 2,
        MODIFIER_SHIFT = 4,
    };
    int32_t iModifierKeys;
    int32_t iRepeatingKey;
    uint32_t uKeyDownTime;
    uint32_t uKeyRepeatStartTime;
    Interface::enumGamepadButtons iLastGamepadEvent;
    uint32_t uGamepadRepeatStartTime;
    Timer* pTimers;
    bool bIsInRenderedMenu;
    FORenderedMenu* pCurrentRenderedMenu;
    bool bMouseOverRenderedMenu;
    FOPipboyManager* pPipboy;
    // HUD and VATS effects.
    FOHUDEffectManager HUDEffects;
    // The VATS effects.
    FOVATSEffectManager VATSEffect;
    float fPipboyMouseX;
    float fPipboyMouseY;
    bool bOverPipboyScreen;
    int32_t iPipboyMenuToSet;
    // The pip-boy state: PS_OPENING while it is opening, PS_OPEN once it has opened.
    PIPBOY_STATE ePipboyState;
    // Called once the pip-boy has finished opening or closing.
    void (*pPipboyCallback)();
    BSSimpleList<Tile*> xGlowBranchTiles;
    bool bQueueCloseSupportMenus;
    bool bContainerOpenQueued;
    float fAspectRatioRatio;
    TutorialManager xTutorialManager;

    // The open menus' list boxes.
    static BSSimpleList<ListBoxBase*> xActiveListBoxes;
    static inline InterfaceManager* pInstance;
    static inline bool bHas360Controller;
    static TileUpdateArray xTileUpdateList;
    static inline bool bAlphaGoingDown;
    static inline int32_t iLastDPSFadeTime;
    static inline float fCurrAlphaDPS;
};

// The interface's named system colours (HUD, pip-boy, ...), looked up by id. Created on first use.
class SystemColorManager {
public:
    // A named colour of the interface (the hard and soft colours derive from it): the colour and the XML ID it is
    // looked up by.
    class SystemColor {
    public:
        SystemColor(const char* apName, uint32_t aiID);
        virtual ~SystemColor();

        virtual uint32_t GetColor() const = 0;
        virtual void SetColor(uint32_t aInColor) = 0;
        virtual bool IsHard() const;
        virtual bool IsSoft() const;

        // Declared after the virtual overload, which keeps the virtual functions' order in the vtable.
        void SetColor(NiColorA* apColor);
        // strXMLID's text.
        const char* GetXMLID();

        static void operator delete(void* apMemory);

        BSStringT<char> strXMLID;
    };

    // A system colour read from an INI preference.
    class SoftSystemColor : public SystemColor {
    public:
        // Sets the colour up from the name, the setting and the XML ID.
        SoftSystemColor(const char* apName, SettingT<INIPrefSettingCollection>* apSetting, uint32_t aiID);
        ~SoftSystemColor() override;

        uint32_t GetColor() const override;
        void SetColor(uint32_t aiColor) override;
        bool IsSoft() const override;

        // Reads the colour into apOut.
        void GetColor(NiColorA* apOut);

        SettingT<INIPrefSettingCollection>* pSetting;
    };

    // A system colour fixed in the code.
    class HardSystemColor : public SystemColor {
    public:
        HardSystemColor(const char* apName, uint32_t aiColor, uint32_t aiID);
        ~HardSystemColor() override;

        uint32_t GetColor() const override {
            return iRGB;
        }
        void SetColor(uint32_t aiColor) override {
            iRGB = aiColor;
        }
        bool IsHard() const override;

        uint32_t iRGB;
    };

    // Builds the list, sets iNextID and adds the six colours.
    SystemColorManager();

    SystemColor* Find(int32_t aiID);
    // Writes the colour with the given id into aOutColor; false when there is none.
    bool GetColor(int32_t aiID, NiColorA& aOutColor);
    bool SetColor(int32_t aiID, const NiColorA& aColor);
    void Add(const char* apName, uint32_t aiRed, uint32_t aiGreen, uint32_t aiBlue, int32_t aiID);
    // Adds a soft colour read from apSetting, which is first set from the RGB values when it is 0.
    void
    Add(const char* apName,
        uint32_t aiRed,
        uint32_t aiGreen,
        uint32_t aiBlue,
        SettingT<INIPrefSettingCollection>* apSetting,
        int32_t aiID);
    uint32_t GetColor(int32_t aiID);
    void RemoveAll();
    bool SetColor(int32_t aiID, uint32_t aiColor);

    static void operator delete(void* apMemory);
    // The manager, created on the first call.
    static SystemColorManager* GetInstance();
    // Destroys the manager's colours and the manager.
    static void DeleteInstance();

    // Find walks SystemColorList from its head.
    NiTPointerList<SystemColor*> SystemColorList;
    int32_t iNextID;

    static SettingT<INIPrefSettingCollection> uHUDColor;
    static SettingT<INIPrefSettingCollection> uPipboyColor;
};

// True when the 360 controller is disabled in the preferences.
extern SettingT<INIPrefSettingCollection> gs_bDisable360Controller;

// The 360 controller's state of the previous and of the current frame.
inline _XINPUT_STATE lastControllerState{};
inline _XINPUT_STATE pState{};

// Set while a revert runs; Idle stops the menus' idle pass when it is set.
inline bool bIsReverting;

// The game settings InterfaceManager.cpp defines.
extern SettingT<INISettingCollection> fXenonMenuDpadRepeatSpeed;
extern SettingT<INISettingCollection> bEnableMenuRenderDebugMode;
extern SettingT<INISettingCollection> fMenuPlayerLightDiffuseBlue;
extern SettingT<INISettingCollection> fXenonMenuStickSpeedMaxMod;
extern SettingT<INISettingCollection> bActivatePickUseGamebryoPick;
extern SettingT<INISettingCollection> fMenuPlayerLightAmbientBlue;
extern SettingT<INISettingCollection> fXenonMenuStickSpeed;
extern SettingT<INISettingCollection> fKeyRepeatTime;
extern SettingT<INISettingCollection> iXenonMenuStickDeadZone;
extern SettingT<INISettingCollection> fMenuPlayerLightAmbientGreen;
extern SettingT<INISettingCollection> fActivatePickSphereRadius;
extern SettingT<INISettingCollection> fKeyRepeatInterval;
extern SettingT<INISettingCollection> bAllowConsole;
extern SettingT<INISettingCollection> bUseImageSpaceMenuFX;
extern SettingT<INISettingCollection> fMenuPlayerLightDiffuseGreen;
extern SettingT<INISettingCollection> iSafeZoneYWide;
extern SettingT<INISettingCollection> iSafeZoneX;
extern SettingT<INISettingCollection> fMenuPlayerLightAmbientRed;
extern SettingT<INISettingCollection> iSafeZoneY;
extern SettingT<INISettingCollection> iSafeZoneXWide;
extern SettingT<INISettingCollection> fMenuPlayerLightDiffuseRed;
extern SettingT<INISettingCollection> fXenonMenuMouseXYMult;
inline int32_t i768 = 768;
inline SystemColorManager* g_sysColorManager;

extern BSCriticalSection kRenderedMenuCriticalSection;

// A debug progress marker written at checkpoints (PostIdleStuff stores 99 at its end).
inline uint32_t uiInterfaceCheckpoint;

// The other settings the initialisation and the pick read.
extern SettingT<INISettingCollection> sDefaultTexAtlas;

extern NiTPointerList<FORenderedMenu*> s_RenderedMenuDeleteList;
