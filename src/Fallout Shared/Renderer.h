#pragma once

#include "BSCore/BSCriticalSection.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiDX9AdapterDesc;
class NiDX9SystemDesc;

// The game's renderer settings and window state; everything here is static.
class Renderer {
public:
	// The last error text (Init and DoResize report failures here).
	using ErrorStringBuffer = char[260];

	// The screen width in pixels.
	static int32_t GetScreenWidth();
	static int32_t GetScreenHeight();
	static bool GetFullScreen();
	// Whether the screen spans an Eyefinity display group.
	static bool is_bEyeFinity();
	// The width of one Eyefinity screen.
	static float Get_1280();
	// The height of one Eyefinity screen.
	static float Get_720();
	// Creates the renderer for the window; null on failure.
	static NiDX9Renderer* Init(HWND__* ahWnd, HINSTANCE__* ahInstance);
	// The text of the last renderer creation error, pErrorStringA.
	static const char* GetReason();
	// Whether this thread may resume rendering.
	static bool QCanResume();
	// The window the game renders into.
	static HWND__* GetHWND();
	static NiDX9Renderer* GetNIRenderer();
	static uint32_t GetAdapterIndex();
	static uint32_t GetFlags();
	// The device type the renderer was created with.
	static int32_t GetDeviceType();
	// The swap effect passed to NiDX9Renderer::Create.
	static NiDX9Renderer::SwapEffect GetSwapEffect();
	// The back buffer count.
	static int32_t GetBackBufferCount();
	// The refresh rate.
	static int32_t GetRefreshRate();
	// The frame buffer format the renderer was created with.
	static NiDX9Renderer::FrameBufferFormat GetFrameBufferFormat();
	static D3DFORMAT GetDepthStencilFormat();
	static NiDX9Renderer::FrameBufferMode GetFramebufferMode();
	static NiDX9Renderer::PresentationInterval GetPresentationInterval();
	static int32_t GetNPatchSegments();
	static bool GetMultiSampleEnabled();
	// Stores the ini window location unless full screen.
	static void Reposition(int32_t aiX, int32_t aiY);
	static uint32_t GetPixelShaderVersion();
	static uint32_t GetVertexShaderVersion();
	// The system's display adapters, created on first use.
	static NiDX9SystemDesc* GetSystemDesc();
	static bool QUseShaders();
	// Returns the bDoCanopyShadowPass:Display setting.
	static bool QDoCanopyShadowPass();
	static bool QDoActorShadows();
	static bool QDoStaticAndArchShadows();
	static bool QDOStaticMenuBackground();
	// Returns the bDrawShadows:Display setting.
	static bool QDoShadows();
	// The fDecalLOD0:Display setting: the distance where decals start to fade.
	static float GetDecalLODStart();
	static float GetSkinnedDecalLODStart();
	// Returns the fSunBrightness:BlurShader setting.
	static float GetBlurSunBrightness();
	static float GetBlurSkyBrightness();
	static float GetBlurSpecularMult();
	static float GetBlurEmmisiveMult();
	// Returns the fSunBrightness:BlurShaderHDR setting.
	static float GetBlurSunBrightnessHDR();
	static float GetBlurSkyBrightnessHDR();
	static float GetBlurSpecularMultHDR();
	static float GetBlurEmmisiveMultHDR();
	static bool GetWidescreen();
	static void SetGamma(float afGamma);
	// Applies the pending resize.
	static void DoResize();
	// Whether the screen settings are usable. abCheckDims is never read.
	static bool ValidateSettings(bool abCheckDims);
	// The adapter auiIndex of GetSystemDesc, null past the last one.
	static NiDX9AdapterDesc* GetAdapterDesc(uint32_t auiIndex);
	// Whether the renderer's adapter supports multisample type aeMultiSample on surfaces of format aeFormat.
	static bool GetMultiSampleTypeSupported(D3DMULTISAMPLE_TYPE aeMultiSample, D3DFORMAT aeFormat);
	// Detects an Eyefinity display group and sets the Eyefinity screen sizes.
	static void SetupEyefinity();
	static bool QDoDiffusePass();

protected:
	// Clears the error text and copies apReason into it.
	static void SetErrorString(const char* apReason);

public:
	static void UpdateGamma();
	static void SetDoCanopyShadowPass(bool abVal);
	static bool QDoAmbientPass();
	static bool QDoSpecularPass();
	static bool QDoTexturePass();

	// The ini settings the Q/Get accessors read (the :Display and :BlurShader sections).
	static SettingT<RendererSettingCollection> bFullScreen;
	static SettingT<INISettingCollection> bUseShaders;
	static SettingT<RendererSettingCollection> bDrawShadows;
	static SettingT<RendererSettingCollection> iAdapter;
	static SettingT<INISettingCollection> iLocationX;
	static SettingT<INISettingCollection> iLocationY;
	static SettingT<INISettingCollection> bIgnoreResolutionCheck;
	static SettingT<INISettingCollection> iNPatchPositionOrder;
	static SettingT<INISettingCollection> iNPatchNormalOrder;
	static SettingT<INISettingCollection> bForceMultiPass;
	static SettingT<INISettingCollection> bDoTallGrassEffect;
	static SettingT<INISettingCollection> iIniBlendType;
	static SettingT<INISettingCollection> iIniBlurPasses;
	static SettingT<INISettingCollection> fIniBlurRadius;
	static SettingT<INISettingCollection> iIniBlurTexSize;
	static SettingT<INISettingCollection> fIniAlphaAddInterior;
	static SettingT<INISettingCollection> fIniAlphaAddExterior;
	static SettingT<INISettingCollection> fIniGetHitShader_HitTexOffset;
	static SettingT<INISettingCollection> fIniGetHitShader_BlockedTexOffset;
	static SettingT<INISettingCollection> fIniGetHitShader_BlurAmount;
	static SettingT<INISettingCollection> bAllow30Shaders;
	static SettingT<INISettingCollection> bReportBadTangentSpace;
	static SettingT<INISettingCollection> bEquippedTorchesCastShadows;
	static SettingT<INISettingCollection> iIniBlendTypeHDR;
	static SettingT<INISettingCollection> iIniBlurPassesHDR;
	static SettingT<INISettingCollection> fIniBlurRadiusHDR;
	static SettingT<INISettingCollection> fBrightClamp;
	static SettingT<INISettingCollection> fBrightScale;
	static SettingT<INISettingCollection> fEyeAdaptSpeed;
	static SettingT<INISettingCollection> fEmissiveHDRMult;
	static SettingT<INISettingCollection> fUpperLUMClamp;
	static SettingT<INISettingCollection> fTargetLUM;
	static SettingT<INISettingCollection> bAllowPartialPrecision;
	static SettingT<INISettingCollection> bDecalsOnSkinnedGeometry;
	static SettingT<INISettingCollection> bTintMipMaps;
	static SettingT<INISettingCollection> bDynamicWindowReflections;
	// The iNPatches:Display setting.
	static SettingT<INISettingCollection> iNPatchSegments;
	static SettingT<INISettingCollection> bDoCanopyShadowPass;
	static SettingT<INISettingCollection> bDoActorShadows;
	static SettingT<INISettingCollection> bDoStaticAndArchShadows;
	// The bStaticMenuBackground:Display setting.
	static SettingT<INISettingCollection> bStaticMenuBG;
	// The fSkinnedDecalLOD0:Display setting.
	static SettingT<INISettingCollection> fSkinnedDecalLODStart;
	// The shadow, light and specular LOD fade settings (Display): the fade start, its range past the start, and
	// the minimum, maximum and default fade starts the settings menu meters span.
	static SettingT<RendererSettingCollection> fShadowLODStartFade;
	static SettingT<INISettingCollection> fShadowLODRange;
	static SettingT<INISettingCollection> fShadowLODMinStartFade;
	static SettingT<INISettingCollection> fShadowLODMaxStartFade;
	static SettingT<INISettingCollection> fShadowLODDefaultStartFade;
	static SettingT<RendererSettingCollection> fLightLODStartFade;
	static SettingT<INISettingCollection> fLightLODRange;
	static SettingT<INISettingCollection> fLightLODMinStartFade;
	static SettingT<INISettingCollection> fLightLODMaxStartFade;
	static SettingT<INISettingCollection> fLightLODDefaultStartFade;
	static SettingT<RendererSettingCollection> fSpecularLODStartFade;
	static SettingT<INISettingCollection> fSpecularLODRange;
	static SettingT<INISettingCollection> fSpecularLODMinStartFade;
	static SettingT<INISettingCollection> fSpecularLODMaxStartFade;
	static SettingT<INISettingCollection> fSpecularLODDefaultStartFade;
	// The fSunBrightness, fSkyBrightness, fSISpecularMult and fSIEmmisiveMult settings of :BlurShader and
	// :BlurShaderHDR.
	static SettingT<INISettingCollection> fIniSunBrightness;
	static SettingT<INISettingCollection> fIniSkyBrightness;
	static SettingT<INISettingCollection> fIniSISpecularMult;
	static SettingT<INISettingCollection> fIniSIEmmisiveMult;
	static SettingT<INISettingCollection> fIniSunBrightnessHDR;
	static SettingT<INISettingCollection> fIniSkyBrightnessHDR;
	static SettingT<INISettingCollection> fIniSISpecularMultHDR;
	static SettingT<INISettingCollection> fIniSIEmmisiveMultHDR;
	// The window the game renders into.
	static inline void* hRendererWnd;
	static inline void* hParentWnd;
	static inline void* hInstance;
	// The NiDX9Renderer Init created.
	static NiPointer<NiDX9Renderer> spRenderer;
	// The pending resize target; DoResize applies it when bDoResize is set.
	static inline bool bDoResize;
	// Signed: DoResize converts them as signed values.
	static inline int32_t iTargetWidth;
	static inline int32_t iTargetHeight;
	static inline bool bGammaDirty;
	static inline bool bGammaSupport;
	static inline bool bNPatch;
	static inline bool bSuspended;
	static inline int8_t bSuspendedRestore;
	// Held while the renderer is suspended.
	static BSCriticalSection SuspendCriticalSection;
	static inline bool bWidescreen;
	static inline bool bLetterbox;
	static inline ErrorStringBuffer pErrorStringA{};
	static inline uint32_t iFlags = 41;
	static inline int32_t ePresentationInterval;
	static inline int32_t eMode;
	// Unsigned because SetShaderVersion takes them as uint32_t.
	static inline uint32_t iPixelShaderVersion = 259;
	static inline uint32_t iVertexShaderVersion = 257;
	static inline int32_t iScreenWidth = 640;
	static inline int32_t iScreenHeight = 480;
	// The screen size read from the ini. Built by SettingT<RendererSettingCollection>'s constructor.
	static SettingT<RendererSettingCollection> iINIScreenWidth;
	static SettingT<RendererSettingCollection> iINIScreenHeight;
	static inline float fCurGamma = 1.0f;
	// Whether actors shadow themselves.
	static SettingT<INISettingCollection> bActorSelfShadowing;
	static inline int32_t g_frameBufferFormat = 3;
	static inline int32_t eStencilFormat = 75;
	// The adapters GetSystemDesc created, kept for the next call.
	static inline NiDX9SystemDesc* m_kAdapters;
	static inline int32_t eDevType;
	static inline int32_t iBackBufferCount = 1;
	// The settings Init reads (Display, BlurShader, BlurShaderHDR, Water, Interface, General and Grass sections).
	// iPresentInterval: SettingT<INISettingCollection>'s int constructor.
	static SettingT<INISettingCollection> iPresentInterval;
	static SettingT<RendererSettingCollection> iMultiSamples;
	static SettingT<INISettingCollection> bAllowScreenShot;
	static SettingT<RendererSettingCollection> bHighDynamicRange;
	static SettingT<RendererSettingCollection> bTransparencyMultisampling;
	static SettingT<RendererSettingCollection> iWaterMultiSamples;
	static SettingT<RendererSettingCollection> iWaterReflectWidth;
	static SettingT<RendererSettingCollection> iWaterReflectHeight;
	static SettingT<INISettingCollection> bForcePow2Textures;
	static SettingT<RendererSettingCollection> iTexMipMapSkip;
	static SettingT<RendererSettingCollection> iTexMipMapMinimum;
	static SettingT<INISettingCollection> bUseResolvableDepth;
	static SettingT<INISettingCollection> bUseFakeFullScreenMotionBlur;
	static SettingT<RendererSettingCollection> bUseBlurShader;
	static SettingT<INISettingCollection> bIniAllow20HairShader;
	static SettingT<INISettingCollection> bForce1XShaders;
	static SettingT<INISettingCollection> bShadowsOnGrass;
	// Built by SettingT<RendererSettingCollection>'s int constructor.
	static SettingT<RendererSettingCollection> iShadowMapResolution;
	static SettingT<RendererSettingCollection> iActorShadowCountInt;
	static SettingT<RendererSettingCollection> iActorShadowCountExt;
	static SettingT<INISettingCollection> bUseRefractionShader;
	static SettingT<INISettingCollection> fSunlightDimmer;
	static SettingT<INISettingCollection> fSunlightDimmerHDR;
	static SettingT<INISettingCollection> fTreeDimmer;
	static SettingT<INISettingCollection> fGrassDimmer;
	static SettingT<INISettingCollection> fEnvMapLODStartFade;
	static SettingT<INISettingCollection> fEnvMapLODEnd;
	static SettingT<INISettingCollection> fEyeEnvMapLODStartFade;
	static SettingT<INISettingCollection> fEyeEnvMapLODEnd;
	static SettingT<RendererSettingCollection> fDecalLODStartFade;
	static SettingT<RendererSettingCollection> fDecalLODEnd;
	static SettingT<INISettingCollection> fSkinnedDecalLODStartFade;
	static SettingT<INISettingCollection> fSkinnedDecalLODEnd;
	static SettingT<INISettingCollection> bCreateShaderPackage;
	static SettingT<INISettingCollection> fShadowFadeTime;
	static SettingT<INISettingCollection> bMTRendering;
	static SettingT<INISettingCollection> bLODNoiseAniso;
	static SettingT<INISettingCollection> fLODNoiseMipBias;
	static SettingT<INISettingCollection> fLandLOFadeSeconds;
	static SettingT<RendererSettingCollection> iShadowMode;
	static SettingT<INISettingCollection> fInterfaceTintR;
	static SettingT<INISettingCollection> fInterfaceTintG;
	static SettingT<INISettingCollection> fInterfaceTintB;
	static SettingT<RendererSettingCollection> iShadowFilter;
	// SettingT<RendererSettingCollection>'s bool constructor.
	static SettingT<RendererSettingCollection> b30GrassVS;
	// Whether the device blends A16B16G16R16F render targets (Init's CheckDeviceFormat).
	static inline bool bFloatBlend;
	static inline char bEyefinityEnabled = 1;
};

// One display of an Eyefinity group (AMD's sample's DisplayInfoStruct): atiEyefinityGetConfigInfo allocates them 0x2C bytes each
// and fills the grid coordinates +0x0/+0x4, displayRect +0x8..+0x14, displayRectVisible +0x18..+0x24 and
// iPreferredDisplay +0x28; Renderer::SetupEyefinity reads displayRect.right +0x10 and bottom +0x14.
struct DisplayInfoStruct {
	int32_t iGridXCoord;
	int32_t iGridYCoord;
	RECT displayRect;
	RECT displayRectVisible;
	int32_t iPreferredDisplay;
};

// Eyefinity configuration of a Windows display, as AMD's Eyefinity sample code returns it. atiEyefinityGetConfigInfo
// writes all six fields.
struct EyefinityInfoStruct {
	int32_t iSLSActive;
	int32_t iSLSGridWidth;
	int32_t iSLSGridHeight;
	int32_t iSLSWidth;
	int32_t iSLSHeight;
	int32_t iBezelCompensatedDisplay;
};

char CreateDisplayDriver();
void* CALLBACK ADLMainMemoryAlloc(uint32_t Size);
void ADLMainMemoryFree(void** a1);
// AMD's sample's atiEyefinityReleaseConfigInfo: frees the display
// array atiEyefinityGetConfigInfo returned and returns 1.
int32_t atiEyefinityReleaseConfigInfo(DisplayInfoStruct** lppDisplaysInfo);
// AMD's sample's atiEyefinityGetConfigInfo, taking the Windows display index (compared with
// AdapterInfo::iOSDisplayIndex) where the sample takes a display name: fills lpEyefinityInfo, the display count and a
// display array the caller frees with atiEyefinityReleaseConfigInfo; returns 1 on success.
uint32_t atiEyefinityGetConfigInfo(int32_t aiDisplayIndex, EyefinityInfoStruct* lpEyefinityInfo,
	int32_t* lpNumDisplaysInfo, DisplayInfoStruct** lppDisplaysInfo);

inline float width_1280 = 1280.0f;
inline float height_720 = 720.0f;
inline float width_1280_ = 1280.0f;
inline float height_720_ = 720.0f;
inline bool bIsEyefinity;
inline bool bIsEyefinityActive;

inline int32_t g_swapEffect;
inline int32_t g_refreshRate;

extern SettingT<RendererSettingCollection> gs_bEnableEyefinity_Display;

// No PC code reads this value.
inline int32_t data_1189460 = 1;
inline float aspectRatio = 1.7777778f;
inline char bEyefinityFallback;
inline float fEyefinityWidth;

// The window class name (ModelLoader.cpp overlays it as pWindowTitle).
inline const char* pAppClassName = "Fallout: New Vegas";

extern SettingT<RendererSettingCollection> gs_sD3DDevice_Display;
