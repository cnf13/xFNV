#pragma once

#include "BSCore/ScrapHeap.h"
#include "BSMain/Setting.h"
#include "BSSystem/BSPackedTaskQueue.h"
#include "BSSystemUtilities/BSMsgDialogSystemUtility.h"
#include "BSSystemUtilities/BSSystemUtility.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

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
struct SDL_Window;
class SceneGraph;
class Sky;
class Sun;
class TESObjectREFR;

// GetActiveWindow's counterpart: the main window's native handle while
// it has SDL's input focus, else null.
HWND__* GetActiveWindow_SDL();
// Sets up the shader system's callbacks and settings.
void Main_ShaderInit();
// Applies the blur and sky brightness settings (HDR or not) and the motion blur setting.
void Main_ImagespaceInit();
BSMsgDialogSystemUtility::USER_RESPONSE SystemUtilityDialogIdleCallback();
void MessageMenuCallback();
// IDA name; not in the PDB. The exe returns RegCloseKey's result, which no caller reads.
void GetInstalledPathRegistryKey(char* apPath, uint32_t auiSize);
// Inferred type: lpFileName: char* from 0x876afa (passed to BSsprintf 0)
int32_t TESMain_LoadIniFile(char* lpFileName);
void SetupArchiveSettings_AndLoadBSAs();
void SystemUtilityEventNotificationCallback(BSSystemUtility::EVENT_NOTIFICATION aeEventNotification);
// MessageBoxA's text and caption (const char*, NiSystem.h's MessageBoxA_t); CreateMessageMenu takes the text as const.
int32_t MessageBoxA_0(const char* apText, const char* apCaption, uint32_t aiType);
int32_t WINAPI WinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, char* lpCmdLine, int32_t nShowCmd);

// The game's main loop object (TESMain in the exe's symbols): frame update, scene graphs and the global scene roots.
class Main {
public:
	Main(HWND__* ahWnd, HINSTANCE__* ahInstance);
	~Main();

	// True while a menu is up or the menu background is ready.
	bool IsInMenuMode();
	// Empty in the PC build: the engine's shared empty thiscall function (0x483710). The PC
	// exe cannot tell which of the PDB's Main members this is; the name is the PDB's texture-streaming member that
	// fits both callers (PlayerCharacter::PositionPlayer, BGSSaveLoadManager::EndLoadGame).
	void ForceUpdateTextures();
	// Updates the texture streaming for afTime; the exe's body is the shared "return false"
	// stub (thiscall, ret 4, 0x86A830).
	bool UpdateTextures(float afTime);
	bool QQuit() const;
	bool GetFlyCamera() const;
	// True while a game reset is pending (bResetGame); thiscall, 0x678CE0, a one-byte getter
	// the exe shares with other classes. Defined in LoadingMenu.cpp, its PDB file.
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
	// Draws the menu background texture over the render target (thiscall, ret 0x10, 0x874B90).
	// Inferred type: apImageSpaceManager: ImageSpaceManager* from 0x8703ec (result of
	// BSShaderManager::GetImageSpaceManager); apTexture: BSRenderedTexture* from 0x870887 (parameter 0 of
	// Main::DrawWorldRenderedMenu)
	void DrawWorld_CopyMenuBG(ImageSpaceManager* apImagespaceManager, NiDX9Renderer* apRenderer,
		bool abMultiSampleEnabled, BSRenderedTexture* apDestination);
	// Stops the offscreen render target.
	void DrawWorld_StopRenderTarget(bool abMultiSampleEnabled, BSRenderedTexture* apDestination);
	// Ends the world draw: stops the sun occlusion, returns the render target and the
	// shared textures and clears the accumulators' caches.
	void DrawWorld_End(BSRenderedTexture* apRenderTexture, ImageSpaceManager* apImagespaceManager,
		NiDX9Renderer* apRenderer, Sun* apSun);
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
	BSRenderedTexture* DrawWorld_InitRenderTexture(NiDX9Renderer* apRenderer, BSRenderedTexture* apDestination,
		bool abWireframe, bool abMultiSampleEnabled);
	bool DrawWorld_VATSOcclusionQuery();
	void DrawWorld_VATS(bool abIssuedVATSQuery);
	void DrawWorld_RestartRenderTexture(BSRenderedTexture* apRenderTexture, bool abMultiSampleEnabled,
		uint32_t auiClearMode);
	void DrawWorld_ImageSpace(bool abMultiSampleEnabled, BSRenderedTexture* apRenderTexture, NiDX9Renderer* apRenderer,
		BSRenderedTexture* apDestination);
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
	// Inferred type: apSun: Sun* from 0x870ae8 (result of Main::DrawWorld_Init)
	void DrawWorld_DrawScene(Sun* psun, int32_t abDo1stPerson, bool abWireframe, bool abIssueVATSPartialQuery);
	void DrawWorld_Add1stPersonToDepthBuffer(BSRenderedTexture* apRenderTexture);
	// Inferred type: apAccumulator: BSShaderAccumulator* from 0x870f23 (NiPointer<BSShaderAccumulator>::m_pObject
	// (&Main+0x88 (this of Main::DrawWorldStandard))); apTexture: BSRenderedTexture* from 0x870f23 (result of
	// Main::DrawWorld_InitRenderTexture)
	void DrawWorld_Refraction(BSShaderAccumulator* apShaderAccum, BSRenderedTexture* apRenderTexture);
	void DrawWorld_Setup1stPerson();
	void DrawWorld_MTRender1stPerson(NiDX9Renderer* apRenderer, NiGeometry* apRenderedMenuGeom, Sun* apSun,
		BSRenderedTexture* apRenderTexture);
	void DrawWorld_RenderedMenuScene();
	// Inferred type: apAccumulator: BSShaderAccumulator* from 0x870fbd (NiPointer<BSShaderAccumulator>::m_pObject
	// (&Main+0x88 (this of Main::DrawWorldStandard))); apRenderedTexture: BSRenderedTexture* from 0x870fbd (result of
	// Main::DrawWorld_InitRenderTexture)
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
	// Not named in the PDB or IDA; name chosen here. The lost-device callback WinMain registers (Gamebryo's
	// LOSTDEVICENOTIFYFUNC): always true. Same function as NavMesh::CheckUnconnectedBorderEdges (0x5D4A40), registered
	// there.
	static bool LostDeviceCallback(void* apData);
	// Inferred type: pCellName: char* from 0x876dee (passed to BSstrlen 0)
	static void InitStartCell(char* pCellName, NiPoint3* a2);
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
	// IDA names; not in the PDB.
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
	// The exe calls BSParticleSystemManager::UpdateParallel on it (OnIdle_UpdateAnimationsAndEffects).
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

// The game settings Main.cpp defines (PDB), at their exe addresses.
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

// The joystick button names. PDB: 8 entries of the element type psMouse has; the exe's psMouse starts 0x90 bytes
// on, and GetControlPushTextFor (0x703AD3) loads its pbuttonSetting (PDB: SettingT<GameSettingCollection>*) from it.
using psJoystick_t = SettingT<GameSettingCollection>* [8];
extern psJoystick_t psJoystick;

// The Xbox gamepad button names. PDB: 31 entries; the exe's psKeyboard starts 34 entries on and Main::Destruct
// frees 34. GetControlPushTextFor (0x703A1A) loads its pbuttonSetting (PDB: SettingT<GameSettingCollection>*) from it.
using psXBoxGamepadText_t = SettingT<GameSettingCollection>* [34];
extern psXBoxGamepadText_t psXBoxGamepadText;

// Game settings, deleted as g_joystickNames's are.
using g_PS3ButtonSettingNames_t = Setting* [34];
extern g_PS3ButtonSettingNames_t g_PS3ButtonSettingNames;

// INISettingCollection: 0xfa2f9a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iRetainFilenameStringTable_Archive;

// INISettingCollection: 0xfa2a9d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sDirectoryName_CameraPath;

// INISettingCollection: 0xfa2f6a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iRetainDirectoryStringTable_Archive;

// INISettingCollection: 0xfa25bd calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sArchiveList_Archive;

// INISettingCollection: 0xfa2f3a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bCheckRuntimeCollisions_Archive;

// INISettingCollection: 0xfa2fca calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iRetainFilenameOffsetTable_Archive;

// INISettingCollection: 0xfa302a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bInvalidateOlderFiles_Archive;

// INISettingCollection: 0xfa2ffd calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sInvalidationFile_Archive;

// INISettingCollection: 0xfa2a6a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iFPS_CameraPath;

// INISettingCollection: 0xfa24ca calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bUseArchives_Archive;

// INISettingCollection: 0xfa2aca calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iTake_CameraPath;

// The exe's global cSkipThreadQueue.
inline char cSkipThreadQueue;
using psMouse_t = SettingT<GameSettingCollection>* [9];
extern psMouse_t psMouse;
inline bool bIsCursorVisible = true;

// IDA: MessageBoxA::bOnlyOnce; not in the PDB. A function-local static of the message box wrapper (0x86A580).
inline bool MessageBoxA_bOnlyOnce = true;

// GameSettingCollection: 0xf6547d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sNoText;

// GameSettingCollection: 0xf6544d calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sYesText;

// GameSettingCollection: 0xf654ad calls 0x40c150.
extern SettingT<GameSettingCollection> gs_sYesToAllText;

// INISettingCollection: 0xfa184d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sIntroSequence_General;
extern NiPointer<NiRefObject> spcameraPath;

// INISettingCollection: 0xfa333a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bAllowYesToAll_MESSAGES;

// RegSettingCollection: 0xfa31ad calls 0x877f10.
extern SettingT<RegSettingCollection> gs_sBasePath;

// INISettingCollection: 0xfa258d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sUnessentialFileCacheList_General;

// INISettingCollection: 0xfa339a calls 0x45cf30.
extern SettingT<INISettingCollection> gs_bSkipInitializationFlows_MESSAGES;

// INISettingCollection: 0xfa336a calls 0x45cf30.
extern SettingT<INISettingCollection> bSkipProgramFlows_MESSAGES;

// INISettingCollection: 0xfa305a calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iFileLogging_MESSAGES;

// INISettingCollection: 0xfa255d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sEssentialFileCacheList_General;
inline bool bWindowDeactivated;
inline int32_t iMainValue;

// The program's window, which WinMain creates; appMain->hWnd holds its
// native handle.
extern SDL_Window* pSDLMainWindow;
