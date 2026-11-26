#pragma once

#include "BSCore/ScrapHeap.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSPackedTaskQueue.h"
#include "BSSystemUtilities/BSMsgDialogSystemUtility.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class BSFogProperty;
class BSParticleSystemManager;
class BSRenderedTexture;
class BSSceneGraph;
class BSShaderAccumulator;
class Controls;
class ImageSpaceManager;
class NiCamera;
class NiDX9Renderer;
class NiGeometry;
class NiNode;
class NiPoint3;
class NiRenderedTexture;
class NiScreenElements;
class SceneGraph;
struct SDL_Window;
class Sky;
class Sun;
class TESObjectREFR;

// GetActiveWindow's counterpart: the main window's native handle while
// it has SDL's input focus, else null (always null on another thread).
HWND__* GetActiveWindow_SDL();
// The shader system's timer (BSShaderManager::SetTimerFunc).
float ShaderTimer(int32_t iDelta);
// Sets up the shader system's callbacks and settings.
void Main_ShaderInit();
// Applies the blur and sky brightness settings (HDR or not) and the motion blur setting.
void Main_ImagespaceInit();
// Replaces the controls singleton with a new Controls for hinst.
void Main_InitControls(HINSTANCE__* hinst);
void Main_InitStartCell(char* pCellName, NiPoint3* apPosition);
BSMsgDialogSystemUtility::USER_RESPONSE SystemUtilityDialogIdleCallback();
void MessageMenuCallback();
void GetInstalledPathRegistryKey(char* apPath, uint32_t auiSize);
int32_t Main_LoadIniFile(char* lpFileName);
void SetupArchiveSettings_AndLoadBSAs();
void SystemUtilityEventNotificationCallback(BSSystemUtility::EVENT_NOTIFICATION aeEventNotification);
// MessageBoxA's text and caption (const char*, NiSystem.h's MessageBoxA_t); CreateMessageMenu takes the text as const.
int32_t MessageBoxCallback(const char* apText, const char* apCaption, uint32_t aiType);
int32_t WINAPI WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int32_t nShowCmd);

// The game's main loop object: frame update, scene graphs and the global scene roots.
class Main {
public:
    Main(HWND__* ahWnd, HINSTANCE__* ahInstance);
    ~Main();

    // True while a menu is up or the menu background is ready.
    bool IsInMenuMode();
    // Empty in the PC build. Called by PlayerCharacter::PositionPlayer and BGSSaveLoadManager::EndLoadGame.
    void ForceUpdateTextures();
    // Updates the texture streaming for afTime; the body only returns false.
    bool UpdateTextures(float afTime);
    bool QQuit() const;
    bool GetFlyCamera() const;
    // True while a game reset is pending (bResetGame). Defined in LoadingMenu.cpp.
    bool GameReset();
    bool GetOnIdle() const;
    BSShaderAccumulator* QDrawWorldAccum() const;
    NiRenderedTexture* GetMenuBGTexture();
    Controls* GetControls();
    // The id of the main thread.
    uint32_t GetThread();
    // Creates the audio system, installs the game's sound callbacks and starts the media player.
    void Main_AudioInit();
    // Creates TES, the player and the managers, loads the plugins and preloads the shared forms and models.
    void InitTES(NiNode* pScene);
    // Creates the renderer; on failure shows the reason and exits the process.
    void InitRenderer();
    // Creates the world, sky, weather and LOD scene roots.
    void InitSceneGraph();
    // One frame of the game loop.
    void OnIdle();
    void OnIdle_ScaleLOD();
    void OnIdle_FixActorBones();
    void OnIdle_UpdateTimer();
    void OnIdle_PollControls();
    void OnIdle_HandleMenuBackground();
    void OnIdle_UpdateAudio();
    void RunScripts();
    void UpdateNonRenderSafeAITasks();
    void OnIdle_UpdateProcessLists();
    void OnIdle_UpdatePlayer();
    void OnIdle_UpdateCurrentGridCell();
    void OnIdle_UpdateAnimationsAndEffects();
    void OnIdle_DoInterfaceIdle();
    void OnIdle_UpdateImageSpace(bool abAdvanceTime);
    // Presents the frame (and handles the screenshot keys).
    void Swap();
    void PostSwapProcess();
    void OnIdle_PostThreadsProcess();
    void DrawWorld(BSRenderedTexture* apDestination, bool abRenderedMenuMode, bool abSkipFirstPerson);
    void DrawWorldRenderedMenu(BSRenderedTexture* apDestination, bool bwireframe);
    void DrawWorldWireframe(BSRenderedTexture* apDestination, bool abSkipFirstPerson);
    void DrawWorldStandard(BSRenderedTexture* apDestination, bool abSkipFirstPerson);
    // Draws the menu background texture over the render target.
    void DrawWorld_CopyMenuBG(
        ImageSpaceManager* apImagespaceManager,
        NiDX9Renderer* apRenderer,
        bool abMultiSampleEnabled,
        BSRenderedTexture* apDestination
    );
    // Stops the offscreen render target.
    void DrawWorld_StopRenderTarget(bool abMultiSampleEnabled, BSRenderedTexture* apDestination);
    // Ends the world draw: stops the sun occlusion, returns the render target and the
    // shared textures and clears the accumulators' caches.
    void DrawWorld_End(
        BSRenderedTexture* apRenderTexture,
        ImageSpaceManager* apImagespaceManager,
        NiDX9Renderer* apRenderer,
        Sun* apSun
    );
    void DrawWorld_UpdateOffscreenBuffers();
    // Frees the menu background texture.
    void KillMenuBGTexture();
    // Draws the world into the menu background texture behind the rendered menus.
    void RenderMenuBackground();
    // Draws the current rendered menu (pip-boy, terminal) into the interface effect texture.
    void UpdateOffscreenInterface();
    // Activates or deactivates the game (the window gained or lost focus); bAlwaysActive keeps it active.
    void SetActive(bool abValue);
    void DrawWorld_UpdateWater();
    void DrawWorld_WaitForParallelUpdate();
    // Draws the 3D parts of the visible minigame and book menus over the menu background into apDestination.
    void DrawWorldMenu3D(BSRenderedTexture* apDestination);
    void DrawWorld_DoVolumetricFog(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture);
    // Draws the screen splatter accumulator with its camera, keeping ScreenSplatter::bResetCameraPos.
    void DrawWorld_ScreenSplatter(NiDX9Renderer* apRenderer);
    BSRenderedTexture* DrawWorld_InitRenderTexture(
        NiDX9Renderer* apRenderer,
        BSRenderedTexture* apDestination,
        bool abWireframe,
        bool abMultiSampleEnabled
    );
    bool DrawWorld_VATSOcclusionQuery();
    void DrawWorld_VATS(bool abIssuedVATSQuery);
    void DrawWorld_RestartRenderTexture(
        BSRenderedTexture* apRenderTexture,
        bool abMultiSampleEnabled,
        uint32_t auiClearMode
    );
    void DrawWorld_ImageSpace(
        bool abMultiSampleEnabled,
        BSRenderedTexture* apRenderTexture,
        NiDX9Renderer* apRenderer,
        BSRenderedTexture* apDestination
    );
    void QuitGame();
    void SetFreezeTime(bool abFreezeTime);
    void SetFlyCamera(bool abFlyCamera);
    void ResetGame();
    void SetGame();

protected:
    // Marks every plugin listed in the file pPath + pFilename as checked; true when one was found.
    bool LoadPluginsFromFile(const char* pPath, const char* pFilename);

public:
    char HandleMouseMove(int32_t x, int32_t y);
    Sun* DrawWorld_Init();
    void DrawWorld_DrawScene(Sun* psun, int32_t abDo1stPerson, bool abWireframe, bool abIssueVATSPartialQuery);
    void DrawWorld_Add1stPersonToDepthBuffer(BSRenderedTexture* apRenderTexture);
    void DrawWorld_Refraction(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture);
    void DrawWorld_Setup1stPerson();
    void DrawWorld_MTRender1stPerson(
        NiDX9Renderer* apRenderer,
        NiGeometry* apRenderedMenuGeom,
        Sun* apSun,
        BSRenderedTexture* apRenderTexture
    );
    void DrawWorld_RenderedMenuScene();
    void DrawWorld_DoDepth(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture);
    int32_t DrawWorld_RenderedMenu(void* apPipboyGeom, NiDX9Renderer* apRenderer);
    void PerformGameReset(bool abCreateMainMenu);

    static TESObjectREFR* GetDebugTextRef();
    static bool GetSceneIsComplex();
    static void SetSceneIsComplex(bool abComplex);
    static bool GetOpenConsole();
    static NiNode* LandLODNode();
    static NiNode* ObjectLODNode();
    static float QFrameAnimTime();
    static BSParticleSystemManager* GetMasterParticleSystemManager();
    static SceneGraph* GetWorldSceneGraph();
    static NiCamera* WorldRootCamera();
    static NiNode* WeatherRootNode();
    static SceneGraph* MenuRootNode();
    static SceneGraph* MenuRootNode3D();
    // Creates the menu scene graph ("Menu") and the 3D menu scene graph ("3DMenu").
    static void CreateMenuRootNode();
    static void CreateMenuRootNode3D();
    // True while the fade-to-black fader is shown.
    static bool QFaderToBlackVisible();
    static void InitSky();
    static void ClearDebugTextRef();
    static void SetOpenConsole(bool abValue);
    static void DestroyInterfaceSceneGraph();
    static void DestroyInterface3DSceneGraph();
    // Gamebryo's RESETNOTIFYFUNC: abBeforeReset, then the registration's data (unused).
    static bool ResetDeviceCallback(bool abBeforeReset, void* apData);
    // The lost-device callback WinMain registers (Gamebryo's LOSTDEVICENOTIFYFUNC): always true.
    static bool LostDeviceCallback(void* apData);
    static void InitPlayerStart(NiPoint3 position);
    static void operator delete(void* apMemory);

    bool oneMore;
    bool bQuitGame;
    bool bResetGame;
    // false while the game window is inactive (SetActive)
    bool bGameActive;
    // true while the main loop runs its idle update
    bool bOnIdle;
    bool bReloadContent;
    bool bFlyCamera;
    // stops the game clock (the tfc console command)
    bool bFreezeTime;
    // the main window
    HWND__* hWnd;
    // the application instance
    HINSTANCE__* hInstance;
    uint32_t uiThreadID;
    // the main thread's handle (a HANDLE)
    void* hThreadHandle;
    ScrapHeap kPackedTaskHeap;
    BSPackedTaskQueue kTaskQueue;
    ScrapHeap kSecondaryPackedTaskHeap;
    BSPackedTaskQueue kSecondaryTaskQueue;
    NiPointer<BSShaderAccumulator> spDrawWorldAccum;
    NiPointer<BSShaderAccumulator> spDraw1stPersonAccum;
    NiPointer<BSShaderAccumulator> spDraw1stPersonDepthAccum;
    NiPointer<BSShaderAccumulator> spScreenSplatterAccum;
    NiPointer<BSShaderAccumulator> spRenderedMenuAccum;
    // set while RenderMenuBackground draws the world into the menu background texture
    bool bRenderingMenuBG;
    NiPointer<NiCamera> sp1stPersonAdjustedCamera;

    // the reference whose debug text is shown
    static inline TESObjectREFR* pDebugTextReference;
    static NiPointer<BSRenderedTexture> spMenuBGTexture;
    // The number of detection formulas run (Actor::GetDetectionLevelAgainstActor counts them).
    static inline int32_t iNumberActorsDetection;
    // The number of line-of-sight picks run this frame; VATSMenu::DoIdle resets it.
    static inline int32_t iNumberLOSPicks;
    static inline bool bOpenConsole;
    static NiPointer<BSSceneGraph> spWorldRoot;
    static NiPointer<NiNode> spMenuRoot;
    static NiPointer<NiNode> sp3DMenuRoot;
    static NiPointer<NiNode> spWeatherRoot;
    static inline NiNode* pLandLOD;
    static inline NiNode* pObjectLOD;
    // BSParticleSystemManager::UpdateParallel is called on it (OnIdle_UpdateAnimationsAndEffects).
    static NiPointer<BSParticleSystemManager> spParticleSystemManager;
    static inline bool bSceneIsComplex;
    static inline float fFrameAnimTime;
    static NiPointer<BSRenderedTexture> spInterfaceFXTex;
    static NiPointer<BSRenderedTexture> spVATSTex;
    static inline bool bMenuBGReady;
    static NiPointer<NiNode> spSkyRoot;
    static NiPointer<BSFogProperty> spFog;
    static inline Sky* pSky;
    static inline bool bInMenuMode;
    static inline bool bFaderToBlackVisible;
    static inline bool bStaticMenuBG;
    static inline bool bInDialog;
    static inline bool bInConsole;
    static inline int32_t iFrameCounter = 1;
    static inline bool bOffsetViewmodelLights;
    static inline NiNode* pWaterLOD;
    static NiPointer<NiScreenElements> spMenuBGGeometry;
};

// The time OnIdle passes to UpdateTextures.
inline float fUpdateTexturesOnIdleTime = 0.5f;

// The head-tracking actors updated this frame; OnIdle resets it.
inline int32_t iHeadTrackingActorUpdateCount;

// Whether OnIdle profiles the pathing around the player.
inline bool bprofilePathing;

// OnIdle's seconds since the last memory-pool sort and the pool
// index it sorts next.
inline float fSortFreeBlocksTimer;
inline uint8_t ucSortFreeBlocksIndex;

// The game's Main object.
inline Main* appMain;

// The game settings Main.cpp defines.
extern SettingT<INISettingCollection> sTestFile9;
extern SettingT<INISettingCollection> fLODFadeOutObjectMultInterior;
extern SettingT<INISettingCollection> fScopeScissorAmount;
extern SettingT<INISettingCollection> bRunVTuneTest;
extern SettingT<INISettingCollection> bUseSunbeams;
extern SettingT<INISettingCollection> bUseThreadedParticleSystem;
extern SettingT<INISettingCollection> fLODFadeOutItemMultComplex;
extern SettingT<INISettingCollection> bBlockMessageBoxes;
extern SettingT<INISettingCollection> sStartingWorld;
extern SettingT<INISettingCollection> bCRTMemoryChecks;
extern SettingT<INISettingCollection> bDisplayBoundingVolumes;
extern SettingT<INISettingCollection> sTestFile2;
extern SettingT<INISettingCollection> iDebugTextTopBottomOffset;
extern SettingT<INISettingCollection> sStartingCellY;
extern SettingT<INISettingCollection> sTestFile1;
extern SettingT<INISettingCollection> fLODFadeOutObjectMultComplex;
extern SettingT<INISettingCollection> sTestFile5;
extern SettingT<INISettingCollection> fLODFadeOutObjectMultCity;
extern SettingT<INISettingCollection> bShowMenuTextureUse;
extern SettingT<INISettingCollection> bUseBackgroundFileLoader;
extern SettingT<INISettingCollection> bUseThreadedTempEffects;
extern SettingT<INISettingCollection> fLODFadeOutActorMultInterior;
extern SettingT<INISettingCollection> bEnableBoundingVolumeOcclusion;
extern SettingT<INISettingCollection> sTestFile3;
extern SettingT<INISettingCollection> sDebugText;
extern SettingT<INISettingCollection> sTestFile7;
extern SettingT<INISettingCollection> bUseThreadedMorpher;
extern SettingT<INISettingCollection> sTestFile10;
extern SettingT<INISettingCollection> sStartingCell;
extern SettingT<INISettingCollection> bPreCullActors;
extern SettingT<INISettingCollection> fLODFadeOutActorMultComplex;
extern SettingT<INISettingCollection> fLODFadeOutActorMultCity;
extern SettingT<INISettingCollection> bChangeTimeMultSlowly;
extern SettingT<INISettingCollection> fLODLandDropAmount;
extern SettingT<INISettingCollection> fLODFadeOutItemMultInterior;
extern SettingT<INISettingCollection> fMenuBGBlurRadius;
extern SettingT<INISettingCollection> bUseWindowsMessageBox;
extern SettingT<INISettingCollection> sTestFile6;
extern SettingT<INISettingCollection> sTestFile8;
extern SettingT<INISettingCollection> fLODFadeOutItemMultCity;
extern SettingT<INISettingCollection> iFPSClamp;
extern SettingT<INISettingCollection> sStartingCellX;
extern SettingT<INISettingCollection> sTestFile4;
extern SettingT<INISettingCollection> iDebugTextLeftRightOffset;
extern SettingT<INISettingCollection> bAlwaysActive;
extern SettingT<INISettingCollection> fLODLandVerticalBias;
using psWeaponTypes_t = Setting* [14];
extern psWeaponTypes_t psWeaponTypes;
inline bool bSavedShadowState;
inline char* pAppININame = const_cast<char*>("FALLOUT.INI");
inline char* pAppCustomININame = const_cast<char*>("Custom.ini");
inline bool bMultiScreenshotQueued;
inline int32_t iIdleSectionCount;
inline float fBlackFaderTime;
inline int32_t iBlackFaderFrames;
inline char* Plugins_txt = const_cast<char*>(".\\Plugins.txt");

// The joystick button names: 8 entries of the element type psMouse has.
using psJoystick_t = SettingT<GameSettingCollection>* [8];
extern psJoystick_t psJoystick;

// The Xbox gamepad button names. Main::Destruct frees 34 entries. GetControlPushTextFor loads its pbuttonSetting from
// it.
using psXBoxGamepadText_t = SettingT<GameSettingCollection>* [34];
extern psXBoxGamepadText_t psXBoxGamepadText;

// Game settings, deleted as g_joystickNames's are.
using g_PS3ButtonSettingNames_t = Setting* [34];
extern g_PS3ButtonSettingNames_t g_PS3ButtonSettingNames;

extern SettingT<INISettingCollection> iRetainFilenameStringTable;

extern SettingT<INISettingCollection> sCameraPathDir;

extern SettingT<INISettingCollection> iRetainDirectoryStringTable;

extern SettingT<INISettingCollection> sMasterArchiveList;

extern SettingT<INISettingCollection> bCheckRuntimeCollisions;

extern SettingT<INISettingCollection> iRetainFilenameOffsetTable;

extern SettingT<INISettingCollection> bInvalidateOlderFiles;

extern SettingT<INISettingCollection> sArchiveInvalidationFile;

extern SettingT<INISettingCollection> iCameraPathFPS;

extern SettingT<INISettingCollection> bUseArchives;

extern SettingT<INISettingCollection> iCameraPathTake;

inline char cSkipThreadQueue;
using psMouse_t = SettingT<GameSettingCollection>* [9];
extern psMouse_t psMouse;
inline bool bIsCursorVisible = true;

// A function-local static of the message box wrapper.
inline bool bMessageBoxOnlyOnce = true;

extern SettingT<GameSettingCollection> pNoText;

extern SettingT<GameSettingCollection> pYesText;

extern SettingT<GameSettingCollection> pYesToAllText;

extern NiPointer<NiRefObject> spcameraPath;

extern SettingT<INISettingCollection> bAllowYesToAll;

extern SettingT<RegSettingCollection> sBasePathS;

extern SettingT<INISettingCollection> sUnessentialFileCacheList;

extern SettingT<INISettingCollection> bSkipInitializationFlows;

extern SettingT<INISettingCollection> bSkipProgramFlows;

extern SettingT<INISettingCollection> iFileLogging;

extern SettingT<INISettingCollection> sEssentialFileCacheList;
inline bool bWindowDeactivated;

// The program's window, which WinMain creates; appMain->hWnd holds its
// native handle.
extern SDL_Window* pSDLMainWindow;
