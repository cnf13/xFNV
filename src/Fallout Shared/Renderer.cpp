#include "Fallout Shared/Renderer.h"

#include "BSAudio/BSAudio.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSDevices/Controls.h"
#include "BSDevices/XPrefSettings.h"
#include "BSMain/BSUtilities.h"
#include "BSMain/MessageHandler.h"
#include "BSMain/RendererSettingCollection.h"
#include "BSMain/Setting.h"
#include "BSMovie/MoviePlayer.h"
#include "BSShader/BSRenderState.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/ShaderBuffer.h"
#include "Fallout Shared/BGSDecalManager.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/NiTreeCtrl.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Fallout Shared/ScreenSplatter.h"
#include "Fallout Shared/Sky/SkyTextureManager.h"
#include "Fallout Shared/SpeedTree/BSTreeManager.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESRace.h"
#include "Fallout Shared/TESWaterForm.h"
#include "Fallout Shared/TESWaterSystem.h"
#include "Fallout/AI/Combat/CombatUtilities.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Interface/Menus/LoadingMenu.h"
#include "Fallout/Misc/Main.h"
#include "Fallout/Misc/SceneGraph.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiD3DDefines.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SourceTextureData.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9SystemDesc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiDevImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiImageConverter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"
#include "SDK/ADL/adl_structures.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"
#include "SDK/NVAPI/nvapi.h"
#include "platform/Ini.h"
#include "platform/Paths.h"
#include "platform/System.h"

#include <SDL3/SDL.h>
#include <cmath>
#include <filesystem>
#include <stdint.h>
#include <string>
#include <xmmintrin.h>

SettingT<RendererSettingCollection> Renderer::iAdapter("iAdapter:Display", 0);
SettingT<RendererSettingCollection> Renderer::iINIScreenWidth("iSize W:Display", 640);
SettingT<RendererSettingCollection> Renderer::iINIScreenHeight("iSize H:Display", 480);
SettingT<INISettingCollection> Renderer::bIgnoreResolutionCheck("bIgnoreResolutionCheck:Display", false);
SettingT<RendererSettingCollection> Renderer::bFullScreen("bFull Screen:Display", true);
SettingT<INISettingCollection> Renderer::iLocationX("iLocation X:Display", 0);
SettingT<INISettingCollection> Renderer::iLocationY("iLocation Y:Display", 0);
SettingT<INISettingCollection> Renderer::iNPatchSegments("iNPatches:Display", 0);
SettingT<INISettingCollection> Renderer::iNPatchPositionOrder("iNPatchPOrder:Display", 0);
SettingT<INISettingCollection> Renderer::iNPatchNormalOrder("iNPatchNOrder:Display", 0);
SettingT<INISettingCollection> Renderer::bUseShaders("bUse Shaders:Display", true);
SettingT<INISettingCollection> Renderer::bUseRefractionShader("bUseRefractionShader:Display", true);
SettingT<RendererSettingCollection> Renderer::bDrawShadows("bDrawShadows:Display", false);
SettingT<RendererSettingCollection> Renderer::iShadowMode("iShadowMode:Display", 3);
SettingT<INISettingCollection> Renderer::bDoCanopyShadowPass("bDoCanopyShadowPass:Display", true);
SettingT<INISettingCollection> gs_bDoAmbientPass_Display("bDoAmbientPass:Display", true);
SettingT<INISettingCollection> gs_bDoDiffusePass_Display("bDoDiffusePass:Display", true);
SettingT<INISettingCollection> gs_bDoSpecularPass_Display("bDoSpecularPass:Display", true);
SettingT<INISettingCollection> gs_bDoTexturePass_Display("bDoTexturePass:Display", true);
SettingT<INISettingCollection> Renderer::bDoActorShadows("bDoActorShadows:Display", true);
SettingT<INISettingCollection> Renderer::bDoStaticAndArchShadows("bDoStaticAndArchShadows:Display", false);
SettingT<INISettingCollection> Renderer::bForceMultiPass("bForceMultiPass:Display", true);
SettingT<INISettingCollection> Renderer::bDoTallGrassEffect("bDoTallGrassEffect:Display", true);
SettingT<RendererSettingCollection> Renderer::iMultiSamples("iMultiSample:Display", 0);
SettingT<RendererSettingCollection> Renderer::iWaterMultiSamples("iWaterMultiSamples:Display", 0);
SettingT<RendererSettingCollection> Renderer::iWaterReflectWidth("iWaterReflectWidth:Water", 512);
SettingT<RendererSettingCollection> Renderer::iWaterReflectHeight("iWaterReflectHeight:Water", 512);
SettingT<RendererSettingCollection> Renderer::bTransparencyMultisampling("bTransparencyMultisampling:Display", false);
SettingT<INISettingCollection> Renderer::bAllowScreenShot("bAllowScreenShot:Display", false);
SettingT<RendererSettingCollection> Renderer::iTexMipMapSkip("iTexMipMapSkip:Display", 0);
SettingT<RendererSettingCollection> Renderer::iTexMipMapMinimum("iTexMipMapMinimum:Display", 0);
SettingT<RendererSettingCollection> Renderer::bUseBlurShader("bUseBlurShader:BlurShader", true);
SettingT<INISettingCollection> Renderer::iIniBlendType("iBlendType:BlurShader", 2);
SettingT<INISettingCollection> Renderer::iIniBlurPasses("iNumBlurpasses:BlurShader", 1);
SettingT<INISettingCollection> Renderer::fIniBlurRadius("fBlurRadius:BlurShader", 0.03f);
SettingT<INISettingCollection> Renderer::iIniBlurTexSize("iBlurTexSize:BlurShader", 256);
SettingT<INISettingCollection> Renderer::fIniAlphaAddInterior("fAlphaAddInterior:BlurShader", 0.5f);
SettingT<INISettingCollection> Renderer::fIniAlphaAddExterior("fAlphaAddExterior:BlurShader", 0.2f);
SettingT<INISettingCollection> Renderer::fIniSunBrightness("fSunBrightness:BlurShader", 0.0f);
SettingT<INISettingCollection> Renderer::fIniSkyBrightness("fSkyBrightness:BlurShader", 0.5f);
SettingT<INISettingCollection> Renderer::fIniSISpecularMult("fSISpecularMult:BlurShader", 1.0f);
SettingT<INISettingCollection> Renderer::fIniSIEmmisiveMult("fSIEmmisiveMult:BlurShader", 1.0f);
SettingT<INISettingCollection> Renderer::fIniGetHitShader_HitTexOffset("fHitTexOffset:GethitShader", 0.005f);
SettingT<INISettingCollection> Renderer::fIniGetHitShader_BlockedTexOffset("fBlockedTexOffset:GethitShader", 0.001f);
SettingT<INISettingCollection> Renderer::fIniGetHitShader_BlurAmount("fBlurAmmount:GethitShader", 0.5f);
SettingT<INISettingCollection> Renderer::bIniAllow20HairShader("bAllow20HairShader:Display", true);
SettingT<INISettingCollection> Renderer::bAllow30Shaders("bAllow30Shaders:Display", false);
SettingT<INISettingCollection> Renderer::bForce1XShaders("bForce1XShaders:Display", false);
SettingT<INISettingCollection> Renderer::bForcePow2Textures("bForcePow2Textures:Display", false);
SettingT<INISettingCollection> Renderer::bStaticMenuBG("bStaticMenuBackground:Display", true);
SettingT<INISettingCollection> Renderer::bReportBadTangentSpace("bReportBadTangentSpace:Display", false);
SettingT<INISettingCollection> Renderer::bEquippedTorchesCastShadows("bEquippedTorchesCastShadows:Display", false);
SettingT<INISettingCollection> Renderer::fSunlightDimmer("fSunlightDimmer:BlurShader", 1.0f);
SettingT<RendererSettingCollection> Renderer::bHighDynamicRange("bDoHighDynamicRange:BlurShaderHDR", true);
SettingT<INISettingCollection> Renderer::iIniBlendTypeHDR("iBlendType:BlurShaderHDR", 2);
SettingT<INISettingCollection> Renderer::iIniBlurPassesHDR("iNumBlurpasses:BlurShaderHDR", 2);
SettingT<INISettingCollection> Renderer::fIniBlurRadiusHDR("fBlurRadius:BlurShaderHDR", 4.0f);
SettingT<INISettingCollection> Renderer::fBrightClamp("fBrightClamp:BlurShaderHDR", 0.35f);
SettingT<INISettingCollection> Renderer::fBrightScale("fBrightScale:BlurShaderHDR", 1.5f);
SettingT<INISettingCollection> Renderer::fIniSunBrightnessHDR("fSunBrightness:BlurShaderHDR", 0.0f);
SettingT<INISettingCollection> Renderer::fIniSkyBrightnessHDR("fSkyBrightness:BlurShaderHDR", 0.5f);
SettingT<INISettingCollection> Renderer::fIniSISpecularMultHDR("fSISpecularMult:BlurShaderHDR", 1.0f);
SettingT<INISettingCollection> Renderer::fIniSIEmmisiveMultHDR("fSIEmmisiveMult:BlurShaderHDR", 1.0f);
SettingT<INISettingCollection> Renderer::fSunlightDimmerHDR("fSunlightDimmer:BlurShaderHDR", 1.3f);
SettingT<INISettingCollection> Renderer::fEyeAdaptSpeed("fEyeAdaptSpeed:BlurShaderHDR", 0.7f);
SettingT<INISettingCollection> Renderer::fEmissiveHDRMult("fEmissiveHDRMult:BlurShaderHDR", 1.0f);
SettingT<INISettingCollection> Renderer::fTreeDimmer("fTreeDimmer:BlurShaderHDR", 1.2f);
SettingT<INISettingCollection> Renderer::fGrassDimmer("fGrassDimmer:BlurShaderHDR", 1.3f);
// The BlurShaderHDR and BlurShaderHDRInterior settings below: not named in the PDB or IDA; names chosen here.
static SettingT<INISettingCollection> gs_fUpperLUMClamp_BlurShaderHDR("fUpperLUMClamp:BlurShaderHDR", 1.0f);
static SettingT<INISettingCollection> gs_fTargetLUM_BlurShaderHDR("fTargetLUM:BlurShaderHDR", 1.2f);
static SettingT<INISettingCollection> gs_iNumBlurpasses_BlurShaderHDRInterior("iNumBlurpasses:BlurShaderHDRInterior",
	1);
static SettingT<INISettingCollection> gs_fBlurRadius_BlurShaderHDRInterior("fBlurRadius:BlurShaderHDRInterior", 7.0f);
static SettingT<INISettingCollection> gs_fBrightClamp_BlurShaderHDRInterior("fBrightClamp:BlurShaderHDRInterior",
	0.225f);
static SettingT<INISettingCollection> gs_fBrightScale_BlurShaderHDRInterior("fBrightScale:BlurShaderHDRInterior",
	2.25f);
static SettingT<INISettingCollection> gs_fEyeAdaptSpeed_BlurShaderHDRInterior("fEyeAdaptSpeed:BlurShaderHDRInterior",
	0.5f);
static SettingT<INISettingCollection>
	gs_fEmissiveHDRMult_BlurShaderHDRInterior("fEmissiveHDRMult:BlurShaderHDRInterior", 1.0f);
SettingT<INISettingCollection> Renderer::fUpperLUMClamp("fUpperLUMClamp:BlurShaderHDRInterior", 1.0f);
SettingT<INISettingCollection> Renderer::fTargetLUM("fTargetLUM:BlurShaderHDRInterior", 1.0f);
SettingT<INISettingCollection> Renderer::fLightLODDefaultStartFade("fLightLODDefaultStartFade:Display", 1000.0f);
SettingT<RendererSettingCollection> Renderer::fLightLODStartFade("fLightLODStartFade:Display",
	Renderer::fLightLODDefaultStartFade.Float());
SettingT<INISettingCollection> Renderer::fLightLODRange("fLightLODRange:Display", 500.0f);
SettingT<INISettingCollection> Renderer::fLightLODMinStartFade("fLightLODMinStartFade:Display", 200.0f);
SettingT<INISettingCollection> Renderer::fLightLODMaxStartFade("fLightLODMaxStartFade:Display", 1200.0f);
SettingT<INISettingCollection> Renderer::fShadowLODDefaultStartFade("fShadowLODDefaultStartFade:Display", 200.0f);
SettingT<RendererSettingCollection> Renderer::fShadowLODStartFade("fShadowLODStartFade:Display",
	Renderer::fShadowLODDefaultStartFade.Float());
SettingT<INISettingCollection> Renderer::fShadowLODRange("fShadowLODRange:Display", 200.0f);
SettingT<INISettingCollection> Renderer::fShadowLODMinStartFade("fShadowLODMinStartFade:Display", 100.0f);
SettingT<INISettingCollection> Renderer::fShadowLODMaxStartFade("fShadowLODMaxStartFade:Display", 300.0f);
SettingT<INISettingCollection> Renderer::fSpecularLODDefaultStartFade("fSpecularLODDefaultStartFade:Display", 500.0f);
SettingT<RendererSettingCollection> Renderer::fSpecularLODStartFade("fSpecularLODStartFade:Display",
	Renderer::fSpecularLODDefaultStartFade.Float());
SettingT<INISettingCollection> Renderer::fSpecularLODRange("fSpecularLODRange:Display", 300.0f);
SettingT<INISettingCollection> Renderer::fSpecularLODMinStartFade("fSpecularLODMinStartFade:Display", 200.0f);
SettingT<INISettingCollection> Renderer::fSpecularLODMaxStartFade("fSpecularLODMaxStartFade:Display", 600.0f);
SettingT<INISettingCollection> Renderer::fEnvMapLODStartFade("fEnvMapLOD1:Display", 1500.0f);
SettingT<INISettingCollection> Renderer::fEnvMapLODEnd("fEnvMapLOD2:Display", 1800.0f);
SettingT<INISettingCollection> Renderer::fEyeEnvMapLODStartFade("fEyeEnvMapLOD1:Display", 45.0f);
SettingT<INISettingCollection> Renderer::fEyeEnvMapLODEnd("fEyeEnvMapLOD2:Display", 65.0f);
SettingT<INISettingCollection> gs_fDecalLOD0_Display("fDecalLOD0:Display", 800.0f);
SettingT<RendererSettingCollection> Renderer::fDecalLODStartFade("fDecalLOD1:Display", 1000.0f);
SettingT<RendererSettingCollection> Renderer::fDecalLODEnd("fDecalLOD2:Display", 1500.0f);
SettingT<INISettingCollection> Renderer::fSkinnedDecalLODStart("fSkinnedDecalLOD0:Display", 300.0f);
SettingT<INISettingCollection> Renderer::fSkinnedDecalLODStartFade("fSkinnedDecalLOD1:Display", 500.0f);
SettingT<INISettingCollection> Renderer::fSkinnedDecalLODEnd("fSkinnedDecalLOD2:Display", 800.0f);
SettingT<INISettingCollection> Renderer::bMTRendering("bMTRendering:Display", false);
SettingT<INISettingCollection> Renderer::bLODNoiseAniso("bLODNoiseAniso:Display", false);
SettingT<INISettingCollection> Renderer::fLODNoiseMipBias("fLODNoiseMipBias:Display", 0.0f);
SettingT<INISettingCollection> Renderer::fLandLOFadeSeconds("fLandLOFadeSeconds:Display", 15.0f);
SettingT<RendererSettingCollection> Renderer::iActorShadowCountExt("iActorShadowCountExt:Display", 2);
SettingT<RendererSettingCollection> Renderer::iActorShadowCountInt("iActorShadowCountInt:Display", 4);
SettingT<INISettingCollection> Renderer::bActorSelfShadowing("bActorSelfShadowing:Display", false);
SettingT<INISettingCollection> Renderer::bShadowsOnGrass("bShadowsOnGrass:Display", false);
SettingT<RendererSettingCollection> Renderer::iShadowMapResolution("iShadowMapResolution:Display", 256);
SettingT<INISettingCollection> Renderer::bAllowPartialPrecision("bAllowPartialPrecision:Display", true);
SettingT<RendererSettingCollection> Renderer::iShadowFilter("iShadowFilter:Display", 0);
SettingT<INISettingCollection> Renderer::bUseResolvableDepth("bUseResolvableDepth:Display", true);
SettingT<INISettingCollection> Renderer::bUseFakeFullScreenMotionBlur("bUseFakeFullScreenMotionBlur:Display", false);
SettingT<RendererSettingCollection> Renderer::b30GrassVS("b30GrassVS:Grass", false);
// IDA names gs_uVideoDeviceIdentifierPart1:Display to ...Part4:Display; not in the PDB. Named after their keys.
static SettingT<INISettingCollection> uVideoDeviceIdentifierPart1("uVideoDeviceIdentifierPart1:Display", 0u);
static SettingT<INISettingCollection> uVideoDeviceIdentifierPart2("uVideoDeviceIdentifierPart2:Display", 0u);
static SettingT<INISettingCollection> uVideoDeviceIdentifierPart3("uVideoDeviceIdentifierPart3:Display", 0u);
static SettingT<INISettingCollection> uVideoDeviceIdentifierPart4("uVideoDeviceIdentifierPart4:Display", 0u);
// IDA name gs_iLastHDRSetting; not in the PDB. Named after its key.
static SettingT<INISettingCollection> iLastHDRSetting("iLastHDRSetting", -1);
SettingT<INISettingCollection> Renderer::bDecalsOnSkinnedGeometry("bDecalsOnSkinnedGeometry:Display", true);
SettingT<INIPrefSettingCollection> gs_fGamma_Display("fGamma:Display", 1.0f);
SettingT<INISettingCollection> Renderer::iPresentInterval("iPresentInterval:Display", 1);
SettingT<INISettingCollection> Renderer::bCreateShaderPackage("bCreateShaderPackage:General", false);
SettingT<INISettingCollection> Renderer::fShadowFadeTime("fShadowFadeTime:Display", 1.0f);
SettingT<INISettingCollection> Renderer::bTintMipMaps("bTintMipMaps:General", false);
SettingT<RendererSettingCollection> gs_sD3DDevice_Display("sD3DDevice:Display", "");
SettingT<INISettingCollection> Renderer::fInterfaceTintR("fInterfaceTintR:Interface", 0.6314f);
SettingT<INISettingCollection> Renderer::fInterfaceTintG("fInterfaceTintG:Interface", 0.9843f);
SettingT<INISettingCollection> Renderer::fInterfaceTintB("fInterfaceTintB:Interface", 0.8824f);
SettingT<INISettingCollection> Renderer::bDynamicWindowReflections("bDynamicWindowReflections:Display", false);
BSCriticalSection Renderer::SuspendCriticalSection;
SettingT<RendererSettingCollection> gs_bEnableEyefinity_Display("bEnableEyefinity:Display", true);
NiPointer<NiDX9Renderer> Renderer::spRenderer;
NiColor BSShaderManager::kBackgroundColor(NiColor_BLACK);

// The exe's cleanup-only EH frame (the AutoMemContext, and the NiDevImageConverter allocation) is left out.
NiDX9Renderer* Renderer::Init(HWND__* ahWnd, HINSTANCE__* ahInstance) {
	// The exe destroys the context it constructed on its frame (ecx = the local), not through the constructor's eax.
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, kContext, &AutoMemContext::Destruct, MEM_CONTEXT_RENDERER_OVERHEAD, true,
		"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\Renderer.cpp", 377);
	hParentWnd = ahWnd;
	hInstance = ahInstance;
	// Start a fresh RendererInfo.txt.
	char acFileName[260];
	BSsprintf(acFileName, 260, "%sRendererInfo.txt", BSUtilities::GetPersonalPath());
	std::FILE* pFile = rt::fopen(acFileName, "w");
	if (pFile) {
		std::fclose(pFile);
	}
	ePresentationInterval = static_cast<int32_t>(iPresentInterval.UInt());
	iScreenWidth = static_cast<int32_t>(iINIScreenWidth.operator uint32_t());
	iScreenHeight = static_cast<int32_t>(iINIScreenHeight.operator uint32_t());
	if (gs_bEnableEyefinity_Display.operator bool()) {
		SetupEyefinity();
	}
	if (is_bEyeFinity()) {
		// The exe truncates through a 64-bit fistp and keeps the low dword.
		iScreenWidth = static_cast<int32_t>(static_cast<int64_t>(Get_1280()));
		iScreenHeight = static_cast<int32_t>(static_cast<int64_t>(Get_720()));
		int32_t iMultiSample;
		if (static_cast<int32_t>(iMultiSamples.UInt()) >= 2) {
			iMultiSample = 2;
		} else {
			iMultiSample = static_cast<int32_t>(iMultiSamples.UInt());
		}
		iMultiSamples = iMultiSample;
	}
	if (!ValidateSettings(true)) {
		return 0;
	}
	if (bFullScreen.operator bool()) {
		iFlags |= 4;
		hRendererWnd = hParentWnd;
#if !defined(_WIN32)
		// WinMain created the window hidden on Linux: shown here at the screen's size, for the device to take it.
		SDL_SetWindowSize(pSDLMainWindow, GetScreenWidth(), GetScreenHeight());
		SDL_ShowWindow(pSDLMainWindow);
		SDL_SyncWindow(pSDLMainWindow);
#endif
	} else {
		// The device draws into the program's window itself (the exe creates a child window of the screen's size,
		// CreateWindowExA WS_CHILD | WS_VISIBLE, and fits its parent with AdjustWindowRect and SetWindowPos): its
		// client area is the screen's size, at the saved location, shown (SWP_SHOWWINDOW).
		hRendererWnd = hParentWnd;
		SDL_SetWindowSize(pSDLMainWindow, GetScreenWidth(), GetScreenHeight());
		SDL_SetWindowPosition(pSDLMainWindow, static_cast<int32_t>(iLocationX.operator uint32_t()),
			static_cast<int32_t>(iLocationY.operator uint32_t()));
		SDL_ShowWindow(pSDLMainWindow);
		SDL_SyncWindow(pSDLMainWindow);
	}
	eMode = 0;
	if (bAllowScreenShot.Bool() && !eMode) {
		eMode = 1;
	}
	// A full-screen device on one of several monitors, at a size other than the desktop's, is created as D3D9Ex.
	bool bUseD3D9Ex = false;
	// The exe's SM_CMONITORS, then the desktop's size (SM_CYSCREEN, SM_CXSCREEN): SDL's number of displays and its
	// primary display's desktop mode.
	int32_t iDisplays = 0;
	SDL_free(SDL_GetDisplays(&iDisplays));
	if (bFullScreen.operator bool() && iDisplays > 1) {
		const SDL_DisplayMode* pDesktop = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
		const int32_t iDesktopHeight = pDesktop ? pDesktop->h : 0;
		const int32_t iDesktopWidth = pDesktop ? pDesktop->w : 0;
		if (iDesktopHeight != GetScreenHeight()) {
			bUseD3D9Ex = true;
		} else if (iDesktopWidth != GetScreenWidth()) {
			bUseD3D9Ex = true;
		}
	}
	const int32_t iRefreshRate = GetRefreshRate();
	const int32_t iBackBuffers = GetBackBufferCount();
	const uint32_t uiFBMode = GetFramebufferMode();
	const int32_t iSwapEffect = GetSwapEffect();
	const int32_t iPresentation = GetPresentationInterval();
	const int32_t iDepthStencil = GetDepthStencilFormat();
	const int32_t iFrameBuffer = GetFrameBufferFormat();
	const int32_t iDeviceType = GetDeviceType();
	const int32_t iAdapter = GetAdapterIndex();
	void* const pWndFocus = GetHWND();
	void* const pWndDevice = GetHWND();
	const uint32_t uiUseFlags = GetFlags();
	const int32_t iCreateHeight = GetScreenHeight();
	const int32_t iCreateWidth = GetScreenWidth();
	NiDX9Renderer* pNewRenderer =
		NiDX9Renderer::Create(static_cast<uint32_t>(iCreateWidth), static_cast<uint32_t>(iCreateHeight), uiUseFlags,
			pWndDevice, pWndFocus, static_cast<uint32_t>(iAdapter), static_cast<NiDX9Renderer::DeviceDesc>(iDeviceType),
			static_cast<NiDX9Renderer::FrameBufferFormat>(iFrameBuffer),
			static_cast<NiDX9Renderer::DepthStencilFormat>(iDepthStencil),
			static_cast<NiDX9Renderer::PresentationInterval>(iPresentation),
			static_cast<NiDX9Renderer::SwapEffect>(iSwapEffect), uiFBMode, static_cast<uint32_t>(iBackBuffers),
			static_cast<uint32_t>(iRefreshRate), bUseD3D9Ex);
	spRenderer = pNewRenderer;
	if (static_cast<int32_t>(iMultiSamples.UInt()) > 0) {
		if (!GetMultiSampleTypeSupported(static_cast<D3DMULTISAMPLE_TYPE>(iMultiSamples.UInt()),
				D3DFMT_A16B16G16R16F) &&
			bHighDynamicRange.Bool()) {
			// The exe fetches the shader level name here and never uses it, and leaves the file open.
			BSShaderManager::GetShaderLevelString();
			BSsprintf(acFileName, 260, "%sRendererInfo.txt", BSUtilities::GetPersonalPath());
			pFile = rt::fopen(acFileName, "a");
			if (pFile) {
				std::fprintf(pFile, "Multisample setting [%i] is not supported with the current device settings.\n",
					iMultiSamples.UInt());
			}
			Renderer::bTransparencyMultisampling = false;
			BSShaderManager::bTransparencyMultisampling = false;
		} else {
			switch (iMultiSamples.operator uint32_t()) {
			case 2:
			case 3:
				eMode = 2;
				break;
			case 4:
				eMode = 4;
				break;
			case 8:
				eMode = 8;
				break;
			default:
				eMode = 0;
				break;
			}
		}
	}
	if (!spRenderer.operator->()) {
		SetErrorString("Unknown error creating the Gamebryo Renderer.");
		return 0;
	}
	NiDX9Renderer* pRenderer = spRenderer.operator->();
	pRenderer->SetBackgroundColor(*BSShaderManager::GetBackgroundColor());
	if (bNPatch) {
		IDirect3DDevice9* pDevice = spRenderer.operator->()->GetD3DDevice();
		if (pDevice) {
			// The exe converts the segment count as a signed int (fild dword).
			pDevice->SetNPatchMode(static_cast<float>(static_cast<int32_t>(GetNPatchSegments())));
		}
	}
	void* pMemory = NiMemObject::operator new(sizeof(NiDevImageConverter));
	NiDevImageConverter* pConverter = pMemory ? ::new (pMemory) NiDevImageConverter : nullptr;
	NiImageConverter::SetImageConverter(pConverter);
	NiSourceTexture::SetUsePreloading(true);
	NiTexture::SetMipmapByDefault(false);
	NiDX9AdapterDesc* pAdapter = GetAdapterDesc(static_cast<uint32_t>(GetAdapterIndex()));
	// The exe takes both strings before it tests the adapter.
	char* pDescription = pAdapter->GetDescription();
	D3DADAPTER_IDENTIFIER9* pIdentifier = pAdapter->GetAdapterIdentifier();
	if (!pAdapter) {
		SetErrorString("GetAdapterDesc() failed.");
		return 0;
	}
	D3DPERF_SetOptions(1);
	D3DCAPS9* pCaps = pAdapter->GetDeviceCaps(D3DDEVTYPE_HAL);
	if (!pCaps) {
		SetErrorString("NiXAdapaterDesc::GetDeviceCaps() failed.");
		return 0;
	}
	if (bFullScreen.operator bool()) {
		bGammaSupport = (pCaps->Caps2 & D3DCAPS2_FULLSCREENGAMMA) != 0;
	} else {
		bGammaSupport = false;
	}
	// The exe keeps the ps_2_0 instruction slot count in a 16-bit local.
	uint16_t usInstructionSlots = static_cast<uint16_t>(pCaps->PS20Caps.NumInstructionSlots);
	if (usInstructionSlots > 0x60 && pCaps->PS20Caps.NumTemps < 32) {
		usInstructionSlots = 0x60;
	}
	spRenderer.operator->();
	IDirect3D9* pD3D = NiDX9Renderer::GetDirect3D();
	HRESULT hResult = pD3D->CheckDeviceFormat(0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
		D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F);
	bFloatBlend = hResult >= 0;
	spRenderer.operator->();
	pD3D = NiDX9Renderer::GetDirect3D();
	hResult = pD3D->CheckDeviceFormat(0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE,
		D3DFMT_A16B16G16R16F);
	BSShaderManager::bFPFiltering = hResult >= 0;
	BSShaderManager::iWaterReflectionMultisample = static_cast<int32_t>(iWaterMultiSamples.UInt());
	BSShaderManager::iWaterReflectionWidth = static_cast<int32_t>(iWaterReflectWidth.UInt());
	BSShaderManager::iWaterReflectionHeight = static_cast<int32_t>(iWaterReflectHeight.UInt());
	BSShaderManager::bAnisoMinFiltering = (pCaps->TextureFilterCaps & D3DPTFILTERCAPS_MINFANISOTROPIC) != 0;
	const bool bNonPow2 =
		!(pCaps->TextureCaps & D3DPTEXTURECAPS_POW2) || (pCaps->TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL);
	BSShaderManager::bNonPower2Textures = bNonPow2 && !bForcePow2Textures.Bool();
	NiDX9SourceTextureData::uiMipMapSkip = iTexMipMapSkip.UInt();
	NiDX9SourceTextureData::uiMipMapMinimum = iTexMipMapMinimum.UInt();
	BSShaderManager::bLetterbox = bLetterbox;
	if (bLetterbox) {
		// The bar height as a fraction of the screen, each side. The exe loads the screen height as an unsigned
		// 64-bit integer and the ini height as a signed dword, and divides by the double 2.0.
		const double dScreenHeight = static_cast<double>(static_cast<uint32_t>(iScreenHeight));
		const double dBars = dScreenHeight - static_cast<double>(static_cast<int32_t>(iINIScreenHeight.UInt()));
		const float fBar = static_cast<float>(dBars / static_cast<double>(static_cast<uint32_t>(iScreenHeight)) / 2.0);
		const float fBottom = 1.0f - fBar;
		// The exe builds the viewport with NiColorA's constructor (0x414430) and copies its four dwords.
		FNVD_INLINED_LOCAL(NiColorA, kViewport, std::nullopt, std::nullopt);
		const NiColorA* pViewport = ::new (&kViewport) NiColorA(0.0f, 1.0f, fBottom, fBar);
		BSShaderManager::kFramebufferViewport.m_left = pViewport->r;
		BSShaderManager::kFramebufferViewport.m_right = pViewport->g;
		BSShaderManager::kFramebufferViewport.m_top = pViewport->b;
		BSShaderManager::kFramebufferViewport.m_bottom = pViewport->a;
		BSShaderManager::iLetterboxWidth = static_cast<int32_t>(iINIScreenWidth.UInt());
		BSShaderManager::iLetterboxHeight = static_cast<int32_t>(iINIScreenHeight.UInt());
	}
	BSShaderManager::bHDR = bHighDynamicRange.Bool() && bFloatBlend;
	BSShaderManager::bResolvableDepth = bUseResolvableDepth.Bool();
	BSShaderManager::bFakeFullScreenMotionBlur = bUseFakeFullScreenMotionBlur.Bool();
	BSShaderManager::bBlurShader = !BSShaderManager::bHDR && bUseBlurShader.Bool();
	BSShaderManager::bAllow20ShaderHair = bIniAllow20HairShader.Bool();
	BSShaderManager::SetShaderVersion(static_cast<uint32_t>(iPixelShaderVersion),
		static_cast<uint32_t>(iVertexShaderVersion), bForce1XShaders.Bool(), pDescription, pIdentifier->Driver,
		usInstructionSlots);
	if (BSShaderManager::GetShaderLevel(BSShaderManager::VertexShader) < 5) {
		bShadowsOnGrass = false;
	}
	BSShaderManager::usShadowMapResolution = static_cast<uint16_t>(iShadowMapResolution.UInt());
	if (BSRenderState::GetGPUVendor() == BSRenderState::Nvidia) {
		spRenderer.operator->();
		pD3D = NiDX9Renderer::GetDirect3D();
		// NVIDIA's alpha-to-coverage FOURCC, MAKEFOURCC('A', 'T', 'O', 'C').
		hResult = pD3D->CheckDeviceFormat(0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0, D3DRTYPE_SURFACE, 0x434F5441);
	} else if (BSRenderState::GetGPUVendor() == BSRenderState::AMD) {
		hResult = 0;
	} else if (BSRenderState::GetGPUVendor() == BSRenderState::Intel) {
		hResult = D3DERR_NOTFOUND;
	} else {
		hResult = D3DERR_NOTFOUND;
	}
	BSShaderManager::bTransparencyMultisampling =
		hResult >= 0 && Renderer::bTransparencyMultisampling.Bool() && static_cast<int32_t>(iMultiSamples.UInt()) > 1;
	const uint32_t uiShadowsInterior = iActorShadowCountInt.UInt();
	const uint32_t uiShadowsExterior = iActorShadowCountExt.UInt();
	BSShaderManager::SetMaxShadowMaps(static_cast<uint32_t>(BSMAX(static_cast<int32_t>(uiShadowsExterior),
		static_cast<int32_t>(uiShadowsInterior))));
	// Multi-GPU (CrossFire or SLI) rendering turns occlusion queries off.
	const bool bUseRefraction = true;
	if (BSRenderState::GetGPUVendor() == BSRenderState::AMD) {
		void* const hModule = platform::LibraryLoad("ATIMGPUD.dll");
		if (hModule) {
			FARPROC pAtiQueryMgpuCount =
				reinterpret_cast<FARPROC>(platform::LibrarySymbol(hModule, "AtiQueryMgpuCount"));
			if (pAtiQueryMgpuCount) {
				const int32_t iGPUCount = pAtiQueryMgpuCount();
				if (iGPUCount > 1) {
					BSShaderManager::bSLIMode = true;
					BSShaderManager::bOcclusionQuery = false;
				}
			}
			platform::LibraryFree(hModule);
		}
	} else if (BSRenderState::GetGPUVendor() == BSRenderState::Nvidia) {
		NvAPI_Status eStatus = NvAPI_Initialize();
		NV_DISPLAY_DRIVER_VERSION kVersion;
		// EXE-INLINED none: the aggregate's zero initialisation (its first dword, then memset of the rest).
		kVersion.version = 0;
		std::memset(&kVersion.drvVersion, 0, sizeof(kVersion) - sizeof(kVersion.version));
		// NV_DISPLAY_DRIVER_VERSION_VER: MAKE_NVAPI_VERSION, the struct's size and version 1 (0x1008C).
		kVersion.version = static_cast<uint32_t>(sizeof(NV_DISPLAY_DRIVER_VERSION)) | 1u << 16;
		eStatus = NvAPI_GetDisplayDriverVersion(nullptr, &kVersion);
		if (eStatus == NVAPI_OK) {
			NvDisplayHandle ahDisplays[128];
			// The exe counts the displays and never reads the count.
			[[maybe_unused]] uint32_t uiDisplays = 0;
			eStatus = NVAPI_OK;
			for (int32_t i = 0; eStatus == NVAPI_OK && i < 128; ++i) {
				eStatus = NvAPI_EnumNvidiaDisplayHandle(static_cast<uint32_t>(i), &ahDisplays[i]);
				if (eStatus == NVAPI_OK) {
					++uiDisplays;
				}
			}
			NvLogicalGpuHandle ahLogicalGPUs[64];
			uint32_t uiLogicalGPUs;
			eStatus = NvAPI_EnumLogicalGPUs(ahLogicalGPUs, &uiLogicalGPUs);
			if (eStatus == NVAPI_OK) {
				NvPhysicalGpuHandle ahPhysicalGPUs[64];
				uint32_t uiPhysicalGPUs;
				eStatus = NvAPI_EnumPhysicalGPUs(ahPhysicalGPUs, &uiPhysicalGPUs);
				if (eStatus == NVAPI_OK) {
					NV_GET_CURRENT_SLI_STATE kSLIState;
					// NV_GET_CURRENT_SLI_STATE_VER: MAKE_NVAPI_VERSION, the struct's size and version 1 (0x1001C).
					kSLIState.version = static_cast<uint32_t>(sizeof(NV_GET_CURRENT_SLI_STATE)) | 1u << 16;
					NiRenderer::GetRenderer();
					eStatus = NvAPI_D3D_GetCurrentSLIState(NiDX9Renderer::GetDirect3D(), &kSLIState);
					if (eStatus != NVAPI_OK) {
						if (uiLogicalGPUs < uiPhysicalGPUs) {
							BSShaderManager::bSLIMode = true;
							BSShaderManager::bOcclusionQuery = false;
						}
					} else if (uiPhysicalGPUs > 0) {
						BSShaderManager::bSLIMode = true;
						BSShaderManager::bOcclusionQuery = false;
					}
				}
			}
		}
	} else if (BSRenderState::GetGPUVendor() == BSRenderState::Intel) {
		GetMultiSampleEnabled();
	} else {
		GetMultiSampleEnabled();
	}
	if (bUseRefraction && bUseRefractionShader.Bool()) {
		BSShaderManager::EnableUseRefractionShader(true);
	} else {
		BSShaderManager::EnableUseRefractionShader(false);
	}
	// Anything but 4:3 is widescreen; a NaN ratio counts as widescreen (the exe's jp).
	const double dHeight = static_cast<double>(GetScreenHeight());
	bWidescreen = dHeight / static_cast<double>(GetScreenWidth()) != 0.75;
	for (int32_t i = 0; i < 15; ++i) {
		D3DFORMAT eFormat;
		switch (i) {
		case 0:
			eFormat = D3DFMT_R5G6B5;
			break;
		case 1:
			eFormat = D3DFMT_X1R5G5B5;
			break;
		case 2:
			eFormat = D3DFMT_A1R5G5B5;
			break;
		case 3:
			eFormat = D3DFMT_A4R4G4B4;
			break;
		case 4:
			eFormat = D3DFMT_L16;
			break;
		case 5:
			eFormat = D3DFMT_R8G8B8;
			break;
		case 6:
			eFormat = D3DFMT_A8R8G8B8;
			break;
		case 7:
			eFormat = D3DFMT_X8R8G8B8;
			break;
		case 8:
			eFormat = D3DFMT_R32F;
			break;
		case 9:
			eFormat = D3DFMT_A16B16G16R16F;
			break;
		case 10:
			eFormat = D3DFMT_A16B16G16R16;
			break;
		case 11:
			eFormat = D3DFMT_A32B32G32R32F;
			break;
		case 12:
			eFormat = D3DFMT_L8;
			break;
		case 13:
			eFormat = D3DFMT_G16R16;
			break;
		case 14:
			eFormat = D3DFMT_G16R16F;
			break;
		default:
			eFormat = D3DFMT_UNKNOWN;
			break;
		}
		spRenderer.operator->();
		pD3D = NiDX9Renderer::GetDirect3D();
		const HRESULT hFormatResult =
			pD3D->CheckDeviceFormat(0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8, 0, D3DRTYPE_TEXTURE, eFormat);
		BSShaderManager::bRenderTargetSupported[i] = hFormatResult >= 0;
	}
	spRenderer.operator->();
	pD3D = NiDX9Renderer::GetDirect3D();
	const HRESULT hStencilResult = pD3D->CheckDeviceFormat(0, D3DDEVTYPE_HAL, D3DFMT_X8R8G8B8,
		D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING, D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16);
	BSShaderManager::bUseDisplacementWaterStencil = hStencilResult >= 0;
	SetGamma(gs_fGamma_Display.Float());
	// Append the device report to RendererInfo.txt.
	const char* pShaderLevel = BSShaderManager::GetShaderLevelString();
	BSsprintf(acFileName, 260, "%sRendererInfo.txt", BSUtilities::GetPersonalPath());
	pFile = rt::fopen(acFileName, "a");
	if (pFile) {
		std::fprintf(pFile, "Renderer Device Information:\n\t%s\n\t%s\n", pDescription, pIdentifier->Driver);
		std::fprintf(pFile, "\tRenderPath         \t\t: %s\n", pShaderLevel);
		// The exe prints the low words of the shader versions.
		std::fprintf(pFile, "\tPSversion          \t\t: %X\n", pCaps->PixelShaderVersion & 0xFFFF);
		std::fprintf(pFile, "\tVSversion          \t\t: %X\n", pCaps->VertexShaderVersion & 0xFFFF);
		std::fprintf(pFile, "\tVStarget           \t\t: %s\n", BSShaderManager::GetVSTarget());
		std::fprintf(pFile, "\tPStarget           \t\t: %s\n",
			BSShaderManager::GetPSTarget(BSShaderManager::VertexShader));
		std::fprintf(pFile, "\tPS2xtarget         \t\t: %s\n",
			BSShaderManager::GetPSTarget(BSShaderManager::PixelShader));
		std::fprintf(pFile, "\tmaxPS20inst        \t\t: %i\n", usInstructionSlots);
		std::fprintf(pFile, "\t3.0 Support        \t\t: %s\n", BSShaderManager::b30Shaders ? "yes" : "no");
		std::fprintf(pFile, "\t3.0 Lighting       \t\t: %s\n", BSShaderManager::bUse30Lighting ? "yes" : "no");
		std::fprintf(pFile, "\tNonpowerof2textures\t\t: %s\n", bNonPow2 ? "yes" : "no");
		std::fprintf(pFile, "\tFP16ARGB blending  \t\t: %s\n", bFloatBlend ? "yes" : "no");
		std::fprintf(pFile, "\tFP16ARGB filtering \t\t: %s\n", BSShaderManager::bFPFiltering ? "yes" : "no");
		std::fprintf(pFile, "\tHigh dynamic range \t\t: %s\n", BSShaderManager::bHDR ? "yes" : "no");
		std::fprintf(pFile, "\tBloom lighting     \t\t: %s\n", BSShaderManager::bBlurShader ? "yes" : "no");
		std::fprintf(pFile, "\tRefraction         \t\t: %s\n",
			BSShaderManager::GetUseRefractionShader() ? "yes" : "no");
		std::fprintf(pFile, "\t2.0 hairw           \t\t: %s\n", BSShaderManager::bAllow20ShaderHair ? "yes" : "no");
		std::fprintf(pFile, "\tSLI mode           \t\t: %s\n", BSShaderManager::bSLIMode ? "yes" : "no");
		std::fprintf(pFile, "\tWater shader       \t\t: %s\n",
			TESWaterSystem::bUseWaterShader.operator bool() ? "yes" : "no");
		std::fprintf(pFile, "\tWater reflections  \t\t: %s\n",
			TESWaterSystem::bUseWaterReflections.operator bool() ? "yes" : "no");
		std::fprintf(pFile, "\tWater displacement \t\t: %s\n",
			TESWaterSystem::bUseWaterDisplacements.operator bool() ? "yes" : "no");
		std::fprintf(pFile, "\tWater high res     \t\t: %s\n",
			TESWaterSystem::bUseWaterHiRes.operator bool() ? "yes" : "no");
		std::fprintf(pFile, "\tMultisample Type   \t\t: %i\n", eMode);
		std::fprintf(pFile, "\tTransparency MS    \t\t: %s\n",
			BSShaderManager::bTransparencyMultisampling ? "yes" : "no");
		std::fprintf(pFile, "\tShader Package     \t\t: %d\n", ShaderBuffer::GetCurrentBufferType());
		std::fprintf(pFile, "\tHW Thread Count    \t\t: %d\n", iNumHWThreads.UInt());
		std::fclose(pFile);
	}
	float fSunlight;
	if (bHighDynamicRange.Bool()) {
		fSunlight = fSunlightDimmerHDR.Float();
	} else {
		fSunlight = fSunlightDimmer.Float();
	}
	BSShaderManager::fSunlightDimmer = fSunlight;
	BSShaderManager::fTreeDimmer = fTreeDimmer.Float();
	BSShaderManager::fGrassDimmer = fGrassDimmer.Float();
	BSShaderManager::fLightLODStartFade = fLightLODStartFade.Float();
	const float* pLightStart = &fLightLODStartFade.Float();
	const float* pLightRange = &fLightLODRange.Float();
	BSShaderManager::fLightLODEnd = *pLightStart + *pLightRange;
	BSShaderManager::fShadowLODStartFade = fShadowLODStartFade.Float();
	const float* pShadowStart = &fShadowLODStartFade.Float();
	const float* pShadowRange = &fShadowLODRange.Float();
	BSShaderManager::fShadowLODEnd = *pShadowStart + *pShadowRange;
	BSShaderManager::fSpecularLODStartFade = fSpecularLODStartFade.Float();
	const float* pSpecularStart = &fSpecularLODStartFade.Float();
	const float* pSpecularRange = &fSpecularLODRange.Float();
	BSShaderManager::fSpecularLODEnd = *pSpecularStart + *pSpecularRange;
	BSShaderManager::fEnvMapLODStartFade = fEnvMapLODStartFade.Float();
	BSShaderManager::fEnvMapLODEnd = fEnvMapLODEnd.Float();
	BSShaderManager::fEyeEnvMapLODStartFade = fEyeEnvMapLODStartFade.Float();
	BSShaderManager::fEyeEnvMapLODEnd = fEyeEnvMapLODEnd.Float();
	BSShaderManager::fDecalLODStartFade = fDecalLODStartFade.Float();
	BSShaderManager::fDecalLODEnd = fDecalLODEnd.Float();
	BSShaderManager::fSkinnedDecalLODStartFade = fSkinnedDecalLODStartFade.Float();
	BSShaderManager::fSkinnedDecalLODEnd = fSkinnedDecalLODEnd.Float();
	BSShaderManager::bCreateNewShaderPackage = bCreateShaderPackage.Bool();
	BSShaderManager::fShadowFadeTime = fShadowFadeTime.Float();
	BSShaderManager::bDynamicWindowReflections = false;
	BSShaderManager::bMTRendering = bMTRendering.Bool();
	BSShaderManager::bLODNoiseAniso = bLODNoiseAniso.Bool();
	BSShaderManager::fLODNoiseMipBias = fLODNoiseMipBias.Float();
	BSShaderManager::fLandLOFadeSeconds = fLandLOFadeSeconds.Float();
	int32_t iShadowModeValue;
	if (static_cast<int32_t>(iShadowMode.UInt()) < 0 || static_cast<int32_t>(iShadowMode.UInt()) > 3) {
		iShadowModeValue = 3;
	} else {
		iShadowModeValue = static_cast<int32_t>(iShadowMode.UInt());
	}
	BSShaderManager::eShadowMode = static_cast<BSShaderManager::etShadowMode>(iShadowModeValue);
	BSShaderManager::eFramebufferMode = static_cast<uint32_t>(eMode);
	// The exe reads the tint settings blue first.
	const float fTintB = fInterfaceTintB.Float();
	const float fTintG = fInterfaceTintG.Float();
	const float fTintR = fInterfaceTintR.Float();
	FNVD_INLINED_LOCAL(NiColorA, kTint, std::nullopt, std::nullopt);
	BSShaderManager::fpInterfaceTint = *::new (&kTint) NiColorA(fTintR, fTintG, fTintB, 1.0f);
	BSShaderManager::SetShadowFilter(static_cast<int32_t>(iShadowFilter.UInt()));
	BSShaderManager::b30GrassVS = b30GrassVS.Bool();
	return spRenderer.operator->();
}

// Not in the exe; name chosen here. The launcher's resolution, for a platform with no launcher: [Display] iSize W and
// iSize H of the prefs file at apcPrefsPath, when either is missing, are set to the primary display's desktop mode in
// pixels (the launcher writes the desktop's resolution on a first start; without them the game runs at the settings'
// default, 640x480). True when they were written.
static bool WriteDesktopSize(const char* apcPrefsPath) {
	char acWidth[16];
	char acHeight[16];
	if (platform::IniGetString(apcPrefsPath, "Display", "iSize W", "", acWidth) &&
		platform::IniGetString(apcPrefsPath, "Display", "iSize H", "", acHeight)) {
		return false;
	}
	// WinMain starts SDL's video later; it is started here for the query alone (SDL counts the starts).
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
		return false;
	}
	bool bWritten = false;
	const SDL_DisplayMode* pMode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
	if (pMode) {
		const std::string kWidth = std::to_string(std::lround(pMode->w * pMode->pixel_density));
		const std::string kHeight = std::to_string(std::lround(pMode->h * pMode->pixel_density));
		bWritten = platform::IniWriteString(apcPrefsPath, "Display", "iSize W", kWidth.c_str()) &&
			platform::IniWriteString(apcPrefsPath, "Display", "iSize H", kHeight.c_str());
	}
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	return bWritten;
}

// Not in the exe; name chosen here. The launcher's part of a start, for a platform with no launcher
// (platform::LauncherAvailable). When the display settings do not fit the adapter (apcAdapterDescription not null):
// FalloutPrefs.ini in the personal folder is made from the install folder's preset when it is missing (VeryHigh.ini,
// the one the Windows launcher picks on current hardware, else High.ini, else Medium.ini), and its [Display]
// sD3DDevice is set to the adapter's description (quoted, as the launcher writes it). On every start the resolution
// is added when the file has none (WriteDesktopSize). When the file changed, the two collections WinMain read from it
// read it again.
static void WriteDisplayPrefs(const char* apcAdapterDescription) {
	char acPrefsPath[260];
	BSstrcpy(acPrefsPath, 260, BSUtilities::GetPersonalPath());
	BSstrcat(acPrefsPath, 260, pAppPrefININame);
	bool bChanged = false;
	if (apcAdapterDescription) {
		const std::filesystem::path kPrefsFile = platform::NativePath(acPrefsPath);
		std::error_code kError;
		if (!std::filesystem::exists(kPrefsFile, kError)) {
			const std::string kInstallFolder = platform::InstallFolder();
			for (const char* pcPreset : {"VeryHigh.ini", "High.ini", "Medium.ini"}) {
				if (std::filesystem::copy_file(platform::NativePath(kInstallFolder + pcPreset), kPrefsFile, kError)) {
					// The copy keeps the preset's mode, which may be read-only; the game writes this file.
					std::filesystem::permissions(kPrefsFile, std::filesystem::perms::owner_write,
						std::filesystem::perm_options::add, kError);
					break;
				}
			}
		}
		const std::string kDevice = std::string("\"") + apcAdapterDescription + "\"";
		platform::IniWriteString(acPrefsPath, "Display", "sD3DDevice", kDevice.c_str());
		bChanged = true;
	}
	if (WriteDesktopSize(acPrefsPath)) {
		bChanged = true;
	}
	if (bChanged) {
		SettingT<RendererSettingCollection>::GetCollection()->ReadSettingsFromFile(acPrefsPath);
		SettingT<INIPrefSettingCollection>::GetCollection()->ReadSettingsFromFile(acPrefsPath);
	}
}

// KEEP: the body is iScreenWidth, but the ecx-live scan found an exe caller that reads ecx
// after the call, relying on the exe body leaving it alone; src cannot promise that of a cdecl body.
int32_t Renderer::GetScreenWidth() {
	return Renderer::iScreenWidth;
}

int32_t Renderer::GetScreenHeight() {
	return Renderer::iScreenHeight;
}

bool Renderer::GetFullScreen() {
	return (bool)((uint8_t)((&Renderer::bFullScreen)->Bool()));
}

bool Renderer::is_bEyeFinity() {
	return (bool)((uint8_t)bIsEyefinity);
}

float Renderer::Get_1280() {
	return width_1280;
}

float Renderer::Get_720() {
	return height_720;
}

// IDA name; not in the PDB (the PDB's Renderer::QReason may be this function).
const char* Renderer::GetReason() {
	return pErrorStringA;
}

bool Renderer::QCanResume() {
	return (bool)((uint8_t)((!Renderer::bSuspendedRestore) || ReturnTrue()));
}

HWND__* Renderer::GetHWND() {
	return static_cast<HWND__*>(Renderer::hRendererWnd);
}

NiDX9Renderer* Renderer::GetNIRenderer() {
	return (&Renderer::spRenderer)->operator->();
}

uint32_t Renderer::GetAdapterIndex() {
	return (&Renderer::iAdapter)->operator uint32_t();
}

uint32_t Renderer::GetFlags() {
	return (uint32_t)((int32_t)Renderer::iFlags);
}

int32_t Renderer::GetDeviceType() {
	return Renderer::eDevType;
}

NiDX9Renderer::SwapEffect Renderer::GetSwapEffect() {
	return static_cast<NiDX9Renderer::SwapEffect>(g_swapEffect);
}

int32_t Renderer::GetBackBufferCount() {
	return Renderer::iBackBufferCount;
}

int32_t Renderer::GetRefreshRate() {
	return g_refreshRate;
}

NiDX9Renderer::FrameBufferFormat Renderer::GetFrameBufferFormat() {
	return static_cast<NiDX9Renderer::FrameBufferFormat>(Renderer::g_frameBufferFormat);
}

D3DFORMAT Renderer::GetDepthStencilFormat() {
	return static_cast<D3DFORMAT>(Renderer::eStencilFormat);
}

NiDX9Renderer::FrameBufferMode Renderer::GetFramebufferMode() {
	return static_cast<NiDX9Renderer::FrameBufferMode>(Renderer::eMode);
}

NiDX9Renderer::PresentationInterval Renderer::GetPresentationInterval() {
	return static_cast<NiDX9Renderer::PresentationInterval>(Renderer::ePresentationInterval);
}

int32_t Renderer::GetNPatchSegments() {
	return (int32_t)(&Renderer::iNPatchSegments)->operator uint32_t();
}

bool Renderer::GetMultiSampleEnabled() {
	return (bool)((uint8_t)(Renderer::eMode >= 2));
}

void Renderer::Reposition(int32_t aiX, int32_t aiY) {
	if (!Renderer::GetFullScreen()) {
		Renderer::iLocationX.operator=(aiX);
		Renderer::iLocationY.operator=(aiY);
	}
}

uint32_t Renderer::GetPixelShaderVersion() {
	return (uint32_t)Renderer::iPixelShaderVersion;
}

uint32_t Renderer::GetVertexShaderVersion() {
	return (uint32_t)Renderer::iVertexShaderVersion;
}

NiDX9SystemDesc* Renderer::GetSystemDesc() {
	if (!Renderer::m_kAdapters) {
		Renderer::m_kAdapters = NiDX9SystemDesc::Get();
	}
	return Renderer::m_kAdapters;
}

bool Renderer::QUseShaders() {
	return (bool)((uint8_t)(&Renderer::bUseShaders)->operator bool());
}

bool Renderer::QDoCanopyShadowPass() {
	return (bool)((&Renderer::bDoCanopyShadowPass)->Bool());
}

bool Renderer::QDoActorShadows() {
	return (bool)((uint8_t)((&Renderer::bDoActorShadows)->Bool()));
}

bool Renderer::QDoStaticAndArchShadows() {
	return (bool)((uint8_t)((&Renderer::bDoStaticAndArchShadows)->Bool()));
}

bool Renderer::QDOStaticMenuBackground() {
	return (bool)((uint8_t)((&Renderer::bStaticMenuBG)->Bool()));
}

bool Renderer::QDoShadows() {
	return (bool)((&Renderer::bDrawShadows)->Bool());
}

float Renderer::GetSkinnedDecalLODStart() {
	return (&Renderer::fSkinnedDecalLODStart)->operator float();
}

// KEEP: the body is fIniSunBrightness's operator float (ecx = the setting), but the ecx-live
// scan found an exe caller that reads ecx after the call, relying on the value the exe body leaves there; src
// cannot express that register result.
float Renderer::GetBlurSunBrightness() {
	return Renderer::fIniSunBrightness.operator float();
}

float Renderer::GetBlurSkyBrightness() {
	return (&Renderer::fIniSkyBrightness)->operator float();
}

float Renderer::GetBlurSpecularMult() {
	return (&Renderer::fIniSISpecularMult)->operator float();
}

float Renderer::GetBlurEmmisiveMult() {
	return (&Renderer::fIniSIEmmisiveMult)->operator float();
}

float Renderer::GetBlurSunBrightnessHDR() {
	return (&Renderer::fIniSunBrightnessHDR)->operator float();
}

float Renderer::GetBlurSkyBrightnessHDR() {
	return (&Renderer::fIniSkyBrightnessHDR)->operator float();
}

float Renderer::GetBlurSpecularMultHDR() {
	return (&Renderer::fIniSISpecularMultHDR)->operator float();
}

float Renderer::GetBlurEmmisiveMultHDR() {
	return (&Renderer::fIniSIEmmisiveMultHDR)->operator float();
}

bool Renderer::GetWidescreen() {
	return (bool)((uint8_t)Renderer::bWidescreen);
}

void Renderer::SetGamma(float afGamma) {
	if ((afGamma != Renderer::fCurGamma) && (afGamma > 0.0)) {
		Renderer::fCurGamma = afGamma;
		Renderer::bGammaDirty = (bool)1;
		BSShaderManager::fGamma = afGamma;
	}
}

void Renderer::DoResize() {
	if (Renderer::GetNIRenderer()) {
		if (Renderer::iTargetWidth) {
			if (Renderer::iTargetHeight) {
				BSShaderAccumulator* Accumulator = BSShaderManager::GetAccumulator();
				Accumulator->ClearAllQueries();
				Renderer::iINIScreenWidth = Renderer::iTargetWidth;
				Renderer::iINIScreenHeight = Renderer::iTargetHeight;
				Renderer::bWidescreen =
					static_cast<double>(Renderer::iTargetHeight) / static_cast<double>(Renderer::iTargetWidth) != 0.75;
				pMoviePlayer->Stop();
				Interface::Free();
				Main::DestroyInterfaceSceneGraph();
				Main::DestroyInterface3DSceneGraph();
				Renderer::GetNIRenderer()->SetSorter(0);
				uint32_t v11 = Renderer::iTargetHeight;
				uint32_t v9 = Renderer::iTargetWidth;
				NiDX9Renderer* NIRenderer = Renderer::GetNIRenderer();
				NiDX9Renderer::RecreateStatus v2 = NIRenderer->Recreate(v9, v11);
				bool v16 = v2 != NiDX9Renderer::RECREATESTATUS_FAILED;
				if (v2) {
					// The window's client area at the new size and the saved location, shown (the exe:
					// AdjustWindowRect, SetWindowPos with SWP_SHOWWINDOW).
					SDL_SetWindowSize(pSDLMainWindow,
						static_cast<int32_t>(Renderer::iINIScreenWidth.operator uint32_t()),
						static_cast<int32_t>(Renderer::iINIScreenHeight.operator uint32_t()));
					SDL_SetWindowPosition(pSDLMainWindow,
						static_cast<int32_t>(Renderer::iLocationX.operator uint32_t()),
						static_cast<int32_t>(Renderer::iLocationY.operator uint32_t()));
					SDL_ShowWindow(pSDLMainWindow);
				} else {
					Renderer::SetErrorString("Failed to Recreate Gamebryo Render in desired dimensions.");
				}
				BSTexturePalette::RemoveAll();
				BSShaderManager::Reinitialize();
				BSShaderAccumulator* v5 = BSShaderManager::GetAccumulator();
				Renderer::spRenderer.m_pObject->SetSorter(v5);
				SceneGraph* WorldSceneGraph = Main::GetWorldSceneGraph();
				WorldSceneGraph->UpdateCameraFOV();
				Main::CreateMenuRootNode();
				Main::CreateMenuRootNode3D();
				Interface::Init(0);
				Interface::InitSecond();
				Interface::CreateStartMenu();
			}
		}
	}
}

bool Renderer::ValidateSettings(bool abCheckDims) {
	bool FullScreen = 0;
	// The desktop's size (the exe: GetSystemMetrics SM_CXSCREEN, SM_CYSCREEN): SDL's primary display's desktop mode.
	const SDL_DisplayMode* pDesktop = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
	uint32_t uiDesktopWidth = pDesktop ? static_cast<uint32_t>(pDesktop->w) : 0;
	uint32_t uiDesktopHeight = pDesktop ? static_cast<uint32_t>(pDesktop->h) : 0;
	float aspectRatio_1 = static_cast<double>(uiDesktopWidth) / static_cast<double>(uiDesktopHeight);
	if (Renderer::GetSystemDesc()) {
		uint32_t AdapterIndex = Renderer::GetAdapterIndex();
		NiDX9AdapterDesc* AdapterDesc = Renderer::GetAdapterDesc(AdapterIndex);
		if (AdapterDesc) {
			FullScreen = Renderer::GetFullScreen();
			if (!FullScreen) {
				FullScreen = AdapterDesc->CanDeviceRenderWindowed(D3DDEVTYPE_HAL);
				if (!FullScreen) {
					Renderer::SetErrorString("Windowed mode not supported on this Adapter.");
				}
			}
			if (FullScreen) {
				FullScreen = 0;
				int32_t a2 = AdapterDesc->GetModeCount();
				uint32_t aiIndex = a2 + 1;
				while (a2--) {
					NiDX9AdapterDesc::ModeDesc* Mode = AdapterDesc->GetMode(a2);
					if (Mode->m_uiBPP >= 32) {
						NiDX9Renderer::FrameBufferFormat FrameBufferFormat = Renderer::GetFrameBufferFormat();
						if (Mode->m_eD3DFormat == NiDX9Renderer::GetD3DFormat(FrameBufferFormat)) {
							float v12;
							float v13;
							if (!Renderer::GetFullScreen()) {
								FullScreen = 1;
								break;
							}
							float aspectRatio =
								static_cast<double>(Mode->m_uiWidth) / static_cast<double>(Mode->m_uiHeight);
							float v14 = aspectRatio - aspectRatio_1;
							if (!(std::fabs(v14) < 0.009999999776482582) &&
								((v13 = aspectRatio_1 - 1.25, !(std::fabs(v13) < 0.009999999776482582)) ||
									(v12 = aspectRatio - 1.3333333730697632,
										!(std::fabs(v12) < 0.009999999776482582)))) {
								// The source compares the mode's height signed here (jg), unsigned below (jbe).
								if (Renderer::GetScreenWidth() == Mode->m_uiWidth &&
									Renderer::GetScreenHeight() <= static_cast<int32_t>(Mode->m_uiHeight) &&
									aiIndex > AdapterDesc->GetModeCount()) {
									aiIndex = a2;
								}
							} else {
								if (Renderer::GetScreenWidth() == Mode->m_uiWidth &&
									Renderer::GetScreenHeight() == Mode->m_uiHeight) {
									FullScreen = 1;
									break;
								}
								if (Renderer::GetFullScreen() && Renderer::GetScreenWidth() == Mode->m_uiWidth) {
									float v10;
									float v11 = aspectRatio - 1.25;
									if (!(std::fabs(v11) < 0.009999999776482582) ||
										(v10 = aspectRatio - 1.3333333730697632,
											!(std::fabs(v10) < 0.009999999776482582))) {
										if (Mode->m_uiHeight > Renderer::GetScreenHeight() &&
											(aiIndex > AdapterDesc->GetModeCount() || Mode->m_uiHeight)) {
											aiIndex = a2;
										}
									}
								}
							}
						}
					}
				}
				if (FullScreen || aiIndex <= AdapterDesc->GetModeCount()) {
					if (!FullScreen && aiIndex <= AdapterDesc->GetModeCount()) {
						Renderer::iScreenWidth = AdapterDesc->GetMode(aiIndex)->m_uiWidth;
						Renderer::iScreenHeight = AdapterDesc->GetMode(aiIndex)->m_uiHeight;
						FullScreen = 1;
						Renderer::bLetterbox = 1;
					}
				} else {
					Renderer::
						SetErrorString("Your display doesn't support the selected resolution.  Please set a different "
									   "resolution in Fallout: New Vegas's Launcher under 'Display Settings'.");
				}
			}
			if (FullScreen) {
				FullScreen = 0;
				_D3DCAPS9* DeviceCaps = AdapterDesc->GetDeviceCaps(D3DDEVTYPE_HAL);
				if (DeviceCaps && Renderer::QUseShaders()) {
					FullScreen = (DeviceCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;
					if ((DeviceCaps->DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0) {
						uint32_t VertexShaderVersion = static_cast<uint16_t>(DeviceCaps->VertexShaderVersion);
						bool v9 = false;
						if (VertexShaderVersion >= Renderer::GetVertexShaderVersion()) {
							uint32_t PixelShaderVersion = static_cast<uint16_t>(DeviceCaps->PixelShaderVersion);
							if (PixelShaderVersion >= Renderer::GetPixelShaderVersion()) {
								v9 = true;
							}
						}
						FullScreen = v9;
						if (v9) {
							Renderer::iVertexShaderVersion = static_cast<uint16_t>(DeviceCaps->VertexShaderVersion);
							Renderer::iPixelShaderVersion = static_cast<uint16_t>(DeviceCaps->PixelShaderVersion);
						} else {
							Renderer::SetErrorString("Pixel and Vertex Shader versions incorrect.  Requires a Geforce4 "
													 "4400 or Radeon 8500 or better.");
						}
					} else {
						Renderer::SetErrorString("Hardware T&L required but not supported by Adapter.");
					}
					bool v6 = (DeviceCaps->DevCaps & D3DDEVCAPS_NPATCHES) != 0;
					Renderer::bNPatch = Renderer::GetNPatchSegments() > 1 && v6;
				} else {
					Renderer::iVertexShaderVersion = 0;
					Renderer::iPixelShaderVersion = 0;
					bool* v7 = &Renderer::bForce1XShaders.Bool();
					BSShaderManager::SetShaderVersion(Renderer::iPixelShaderVersion, Renderer::iVertexShaderVersion,
						*v7, 0, 0, 0);
					if (!Renderer::QUseShaders()) {
						return 1;
					}
				}
			}
		} else {
			Renderer::SetErrorString("Bad Adapter Number or Adapter not found.");
		}
	} else {
		Renderer::SetErrorString("No D3D Device description found.");
	}
	return FullScreen;
}

NiDX9AdapterDesc* Renderer::GetAdapterDesc(uint32_t auiIndex) {
	if (Renderer::GetSystemDesc() && auiIndex < Renderer::m_kAdapters->m_uiAdapterCount) {
		return Renderer::m_kAdapters->GetAdapter(auiIndex);
	} else {
		return 0;
	}
}

bool Renderer::GetMultiSampleTypeSupported(D3DMULTISAMPLE_TYPE aeMultiSample, D3DFORMAT aeFormat) {
	bool result = false;
	if (Renderer::spRenderer.m_pObject) {
		_D3DFORMAT D3DFormat;
		if (aeFormat) {
			D3DFormat = aeFormat;
		} else {
			NiDX9Renderer::FrameBufferFormat FrameBufferFormat = Renderer::GetFrameBufferFormat();
			D3DFormat = NiDX9Renderer::GetD3DFormat(FrameBufferFormat);
		}
		IDirect3D9* Direct3D = NiDX9Renderer::GetDirect3D();
		int32_t bWindowed = !Renderer::bFullScreen;
		uint32_t DevType = Renderer::spRenderer.m_pObject->GetDevType();
		uint32_t AdapterIndex = Renderer::GetAdapterIndex();
		if (Direct3D->CheckDeviceMultiSampleType(AdapterIndex, DevType, D3DFormat, bWindowed, aeMultiSample, 0) >= 0) {
			return true;
		}
	}
	return result;
}

void Renderer::SetupEyefinity() {
	int32_t iDisplayIndex = -1;
	EyefinityInfoStruct kEyefinityInfo = {};
	// Not read here: atiEyefinityGetConfigInfo writes it on success only.
	int32_t iNumDisplaysInfo;
	DisplayInfoStruct* pDisplaysInfo = nullptr;
	// The exe tests the index it has just set: AMD's sample looks for the desktop's display only when none is given.
	if (iDisplayIndex == -1) {
		// The desktop's display (the exe: the first EnumDisplayDevicesA entry attached to the desktop): SDL's primary
		// display, by its place among SDL's displays.
		int32_t iDisplays = 0;
		if (SDL_DisplayID* pDisplays = SDL_GetDisplays(&iDisplays)) {
			const SDL_DisplayID uiPrimary = SDL_GetPrimaryDisplay();
			for (int32_t i = 0; i < iDisplays; ++i) {
				if (pDisplays[i] == uiPrimary) {
					iDisplayIndex = i;
					break;
				}
			}
			SDL_free(pDisplays);
		}
	}
	if (atiEyefinityGetConfigInfo(iDisplayIndex, &kEyefinityInfo, &iNumDisplaysInfo, &pDisplaysInfo) == 1) {
		if (kEyefinityInfo.iSLSActive == 1 && bEyefinityEnabled) {
			width_1280 = static_cast<float>(kEyefinityInfo.iSLSWidth);
			height_720 = static_cast<float>(kEyefinityInfo.iSLSHeight);
			width_1280_ = width_1280;
			height_720_ = height_720;
			aspectRatio = width_1280 / height_720;
			bIsEyefinity = true;
			// displayRect.right/bottom hold a display's width/height: a landscape first display marks the group active.
			if (pDisplaysInfo->displayRect.bottom <= pDisplaysInfo->displayRect.right) {
				bIsEyefinityActive = true;
			} else {
				bEyefinityFallback = 1;
			}
			// Three screens side by side: the game draws on the middle one's width.
			if (bIsEyefinityActive && kEyefinityInfo.iSLSGridWidth == 3 && kEyefinityInfo.iSLSGridHeight == 1) {
				width_1280_ = width_1280 / 3.0;
				height_720_ = height_720;
				fEyefinityWidth = width_1280_;
			}
		}
		atiEyefinityReleaseConfigInfo(&pDisplaysInfo);
	} else {
		std::printf("Eyefinity configuration query failed for display index %i.\n", iDisplayIndex);
	}
}

bool Renderer::QDoDiffusePass() {
	return (bool)((&gs_bDoDiffusePass_Display)->Bool());
}

void Renderer::SetErrorString(const char* apReason) {
	Renderer::pErrorStringA[0] = 0;
	if (apReason) {
		ourStrcpy(Renderer::pErrorStringA, apReason);
	}
}

void Renderer::UpdateGamma() {
	D3DGAMMARAMP v3;
	Renderer::bGammaDirty = 0;
	gs_fGamma_Display = Renderer::fCurGamma;
	for (uint16_t us = 0; us < 256u; ++us) {
		float afX = static_cast<double>(us) / 255.0;
		v3.red[us] = static_cast<int32_t>(std::pow(afX, Renderer::fCurGamma) * 65535.0 + 0.5);
		v3.green[us] = static_cast<int32_t>(std::pow(afX, Renderer::fCurGamma) * 65535.0 + 0.5);
		v3.blue[us] = static_cast<int32_t>(std::pow(afX, Renderer::fCurGamma) * 65535.0 + 0.5);
	}
	IDirect3DDevice9* D3DDevice = Renderer::spRenderer.m_pObject->GetD3DDevice();
	D3DDevice->SetGammaRamp(0, 1, &v3);
}

bool Renderer::QDoAmbientPass() {
	return gs_bDoAmbientPass_Display.Bool();
}

bool Renderer::QDoSpecularPass() {
	return gs_bDoSpecularPass_Display.Bool();
}

bool Renderer::QDoTexturePass() {
	return gs_bDoTexturePass_Display.Bool();
}

// The exe also reads the adapter's +4 member through the trivial accessor 0x717E50 and drops it.
char CreateDisplayDriver() {
	char Destination[268];
	if (Renderer::GetSystemDesc()) {
		NiDX9AdapterDesc* AdapterDesc = Renderer::GetAdapterDesc(0);
		if (AdapterDesc) {
			char* String2 = AdapterDesc->GetDescription();
			const char* v0 = gs_sD3DDevice_Display.String();
			if (_stricmp(v0, String2)) {
				// The settings were made for another adapter, or by no launcher yet. The exe returns 1, and WinMain
				// starts FalloutNVLauncher.exe (which writes FalloutPrefs.ini for this adapter) and exits. Where there
				// is no launcher, the program does its part here and goes on with this start, as the exe does once
				// the launcher has written the file.
				if (platform::LauncherAvailable()) {
					return 1;
				}
				WriteDisplayPrefs(String2);
			} else if (!platform::LauncherAvailable()) {
				// Settings a launcher-less start wrote before its resolution was written, or by hand.
				WriteDisplayPrefs(nullptr);
			}
		}
	}
	ourStrcpy(Destination, "\"");
	const char* v2 = gs_sD3DDevice_Display.String();
	std::strcat(Destination, v2);
	std::strcat(Destination, "\"");
	gs_sD3DDevice_Display = static_cast<const char*>(Destination);
	return 0;
}

void* CALLBACK ADLMainMemoryAlloc(uint32_t Size) {
	return ourMalloc(Size);
}

void ADLMainMemoryFree(void** a1) {
	if (*a1) {
		ourFree(*a1);
		*a1 = 0;
	}
}

int32_t atiEyefinityReleaseConfigInfo(DisplayInfoStruct** lppDisplaysInfo) {
	// ADL's free callback takes the address of any buffer pointer (void**): the exe treats it as untyped, and AMD's
	// sample casts it.
	ADLMainMemoryFree(reinterpret_cast<void**>(lppDisplaysInfo));
	return 1;
}

// AMD's Eyefinity sample's atiEyefinityGetConfigInfo: loads ADL, finds the adapter driving Windows display
// aiDisplayIndex and, when its displays form one SLS (Eyefinity) grid, describes the grid and each display.
uint32_t atiEyefinityGetConfigInfo(int32_t aiDisplayIndex, EyefinityInfoStruct* lpEyefinityInfo,
	int32_t* lpNumDisplaysInfo, DisplayInfoStruct** lppDisplaysInfo) {
	ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create = nullptr;
	ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy = nullptr;
	ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = nullptr;
	ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get = nullptr;
	ADL_DISPLAY_SLSMAPINDEX_GET ADL_Display_SLSMapIndex_Get = nullptr;
	ADL_DISPLAY_SLSMAPCONFIG_GET ADL_Display_SLSMapConfig_Get = nullptr;
	ADL_DISPLAY_MODES_GET ADL_Display_Modes_Get = nullptr;
	ADL_DISPLAY_DISPLAYMAPCONFIG_GET ADL_Display_DisplayMapConfig_Get = nullptr;
	int32_t iNumberAdapters = 0;
	int32_t iCurrentAdapter = 0;
	AdapterInfo* lpAdapterInfo = nullptr;
	int32_t iNumDisplayTarget = 0;
	ADLDisplayTarget* lpDisplayTarget = nullptr;
	int32_t iNumDisplayMap = 0;
	ADLDisplayMap* lpDisplayMap = nullptr;
	int32_t iSLSMapIndex = 0;
	int32_t iNumSLSTarget = 0;
	int32_t iCurrentSLSTarget = 0;
	ADLSLSTarget* lpSLSTarget = nullptr;
	int32_t iNumNativeMode = 0;
	int32_t iCurrentNativeMode = 0;
	ADLSLSMode* lpNativeMode = nullptr;
	int32_t iNumBezelMode = 0;
	int32_t iCurrentBezelMode = 0;
	ADLBezelTransientMode* lpBezelMode = nullptr;
	int32_t iNumTransientMode = 0;
	ADLBezelTransientMode* lpTransientMode = nullptr;
	int32_t iNumSLSOffset = 0;
	int32_t iCurrentSLSOffset = 0;
	ADLSLSOffset* lpSLSOffset = nullptr;
	int32_t iNumModes = 0;
	ADLMode* lpModes = nullptr;
	// The sample's `ADLSLSMap slsMap = {0}`: the exe stores the first dword and memsets the rest.
	ADLSLSMap slsMap;
	slsMap.iAdapterIndex = 0;
	std::memset(&slsMap.iSLSMapIndex, 0, sizeof(slsMap) - sizeof(slsMap.iAdapterIndex));
	DisplayInfoStruct* lpDisplayInfo = nullptr;
	int32_t iCurrentDisplayTarget = 0;
	int32_t iMatchedModes = 0;
	int32_t iSLSWidth = 0;
	int32_t iSLSHeight = 0;
	int32_t bRotated = 0;
	int32_t iPreferredX = 0;
	int32_t iPreferredY = 0;
	int32_t bBezel = 0;
	uint32_t iReturn = 1;
	int32_t bFound = 0;
	int32_t iFailedModes = 0;
	if (!lpEyefinityInfo || !lpNumDisplaysInfo || !lppDisplaysInfo) {
		return 0;
	}
	void* hDLL = platform::LibraryLoad("atiadlxx.dll");
	if (!hDLL) {
		hDLL = platform::LibraryLoad("atiadlxy.dll");
	}
	if (!hDLL) {
		return 0;
	}
	// The library's symbols are untyped; the sample casts each entry point to its ADL type.
	ADL_Main_Control_Create =
		reinterpret_cast<ADL_MAIN_CONTROL_CREATE>(platform::LibrarySymbol(hDLL, "ADL_Main_Control_Create"));
	ADL_Main_Control_Destroy =
		reinterpret_cast<ADL_MAIN_CONTROL_DESTROY>(platform::LibrarySymbol(hDLL, "ADL_Main_Control_Destroy"));
	ADL_Adapter_NumberOfAdapters_Get = reinterpret_cast<ADL_ADAPTER_NUMBEROFADAPTERS_GET>(platform::LibrarySymbol(hDLL,
		"ADL_Adapter_NumberOfAdapters_Get"));
	ADL_Adapter_AdapterInfo_Get =
		reinterpret_cast<ADL_ADAPTER_ADAPTERINFO_GET>(platform::LibrarySymbol(hDLL, "ADL_Adapter_AdapterInfo_Get"));
	ADL_Display_SLSMapIndex_Get =
		reinterpret_cast<ADL_DISPLAY_SLSMAPINDEX_GET>(platform::LibrarySymbol(hDLL, "ADL_Display_SLSMapIndex_Get"));
	ADL_Display_SLSMapConfig_Get =
		reinterpret_cast<ADL_DISPLAY_SLSMAPCONFIG_GET>(platform::LibrarySymbol(hDLL, "ADL_Display_SLSMapConfig_Get"));
	ADL_Display_Modes_Get =
		reinterpret_cast<ADL_DISPLAY_MODES_GET>(platform::LibrarySymbol(hDLL, "ADL_Display_Modes_Get"));
	ADL_Display_DisplayMapConfig_Get = reinterpret_cast<ADL_DISPLAY_DISPLAYMAPCONFIG_GET>(platform::LibrarySymbol(hDLL,
		"ADL_Display_DisplayMapConfig_Get"));
	if (!ADL_Main_Control_Create || !ADL_Main_Control_Destroy || !ADL_Adapter_NumberOfAdapters_Get ||
		!ADL_Adapter_AdapterInfo_Get || !ADL_Display_SLSMapIndex_Get || !ADL_Display_SLSMapConfig_Get ||
		!ADL_Display_Modes_Get || !ADL_Display_DisplayMapConfig_Get) {
		return 0;
	}
	// ADL allocates through the exe's ADLMainMemoryAlloc: the exe passes its address.
	if (ADL_Main_Control_Create(&ADLMainMemoryAlloc, 1)) {
		return 0;
	}
	if (ADL_Adapter_NumberOfAdapters_Get(&iNumberAdapters)) {
		return 0;
	}
	if (!iNumberAdapters) {
		return 0;
	}
	lpAdapterInfo =
		static_cast<AdapterInfo*>(ADLMainMemoryAlloc(static_cast<uint32_t>(sizeof(AdapterInfo) * iNumberAdapters)));
	if (!lpAdapterInfo) {
		return 0;
	}
	ourMemset(lpAdapterInfo, 0, static_cast<uint32_t>(sizeof(AdapterInfo) * iNumberAdapters));
	if (ADL_Adapter_AdapterInfo_Get(lpAdapterInfo, static_cast<int32_t>(sizeof(AdapterInfo) * iNumberAdapters))) {
		return 0;
	}
	for (iCurrentAdapter = 0; iCurrentAdapter < iNumberAdapters; ++iCurrentAdapter) {
		if (lpAdapterInfo[iCurrentAdapter].iOSDisplayIndex != aiDisplayIndex) {
			continue;
		}
		if (ADL_Display_DisplayMapConfig_Get(lpAdapterInfo[iCurrentAdapter].iAdapterIndex, &iNumDisplayMap,
				&lpDisplayMap, &iNumDisplayTarget, &lpDisplayTarget, 1)) {
			iReturn = 0;
			break;
		}
		if (iNumDisplayTarget < 2) {
			break;
		}
		if (ADL_Display_SLSMapIndex_Get(lpAdapterInfo[iCurrentAdapter].iAdapterIndex, iNumDisplayTarget,
				lpDisplayTarget, &iSLSMapIndex)) {
			break;
		}
		if (ADL_Display_SLSMapConfig_Get(lpAdapterInfo[iCurrentAdapter].iAdapterIndex, iSLSMapIndex, &slsMap,
				&iNumSLSTarget, &lpSLSTarget, &iNumNativeMode, &lpNativeMode, &iNumBezelMode, &lpBezelMode,
				&iNumTransientMode, &lpTransientMode, &iNumSLSOffset, &lpSLSOffset, 2)) {
			iReturn = 0;
			break;
		}
		if (iNumDisplayTarget != slsMap.grid.iSLSGridColumn * slsMap.grid.iSLSGridRow) {
			iReturn = 0;
			break;
		}
		// The preferred display is the grid's middle column on its bottom row.
		switch (slsMap.grid.iSLSGridColumn) {
		case 3:
		case 4:
			iPreferredX = 1;
			break;
		case 5:
		case 6:
			iPreferredX = 2;
			break;
		default:
			iPreferredX = 0;
			break;
		}
		iPreferredY = slsMap.grid.iSLSGridRow - 1;
		for (iCurrentDisplayTarget = 0; iCurrentDisplayTarget < iNumDisplayTarget; ++iCurrentDisplayTarget) {
			// ADL's free callback takes the address of any buffer pointer (void**), untyped in the exe; the sample
			// casts it, here and below.
			ADLMainMemoryFree(reinterpret_cast<void**>(&lpModes));
			if (ADL_Display_Modes_Get(lpAdapterInfo[iCurrentAdapter].iAdapterIndex,
					lpDisplayTarget[iCurrentDisplayTarget].displayID.iDisplayLogicalIndex, &iNumModes, &lpModes)) {
				iFailedModes |= 1 << iCurrentDisplayTarget;
				continue;
			}
			for (iCurrentNativeMode = 0; iCurrentNativeMode < iNumNativeMode; ++iCurrentNativeMode) {
				if (lpModes->iXRes == lpNativeMode[iCurrentNativeMode].displayMode.iXRes &&
					lpModes->iYRes == lpNativeMode[iCurrentNativeMode].displayMode.iYRes) {
					iMatchedModes |= 1 << iCurrentDisplayTarget;
					break;
				}
			}
			if ((iMatchedModes & 1 << iCurrentDisplayTarget) == 0) {
				for (iCurrentBezelMode = 0; iCurrentBezelMode < iNumBezelMode; ++iCurrentBezelMode) {
					if (lpModes->iXRes == lpBezelMode[iCurrentBezelMode].displayMode.iXRes &&
						lpModes->iYRes == lpBezelMode[iCurrentBezelMode].displayMode.iYRes) {
						iMatchedModes |= 1 << iCurrentDisplayTarget;
						bBezel = 1;
						break;
					}
				}
			}
			if ((iMatchedModes & 1 << iCurrentDisplayTarget) == 0) {
				continue;
			}
			if (!*lppDisplaysInfo) {
				*lppDisplaysInfo =
					static_cast<DisplayInfoStruct*>(ADLMainMemoryAlloc(static_cast<uint32_t>(sizeof(DisplayInfoStruct) *
						iNumDisplayTarget)));
				if (!*lppDisplaysInfo) {
					iReturn = 0;
					break;
				}
				ourMemset(*lppDisplaysInfo, 0, static_cast<uint32_t>(sizeof(DisplayInfoStruct) * iNumDisplayTarget));
			}
			for (iCurrentSLSTarget = 0; iCurrentSLSTarget < iNumSLSTarget; ++iCurrentSLSTarget) {
				if (lpDisplayTarget[iCurrentDisplayTarget].displayID.iDisplayLogicalIndex ==
					lpSLSTarget[iCurrentSLSTarget].displayTarget.displayID.iDisplayLogicalIndex) {
					bFound = 1;
					break;
				}
			}
			if (!bFound) {
				iReturn = 0;
				break;
			}
			iSLSWidth = lpModes->iXRes;
			iSLSHeight = lpModes->iYRes;
			if (lpModes->iOrientation != 0 && lpModes->iOrientation != 180) {
				bRotated = 1;
			}
			lpDisplayInfo = &(*lppDisplaysInfo)[iCurrentDisplayTarget];
			lpDisplayInfo->iGridXCoord = lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX;
			lpDisplayInfo->iGridYCoord = lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY;
			if (!bRotated) {
				lpDisplayInfo->displayRect.right = lpModes->iXRes / slsMap.grid.iSLSGridColumn;
				lpDisplayInfo->displayRect.bottom = lpModes->iYRes / slsMap.grid.iSLSGridRow;
			} else {
				lpDisplayInfo->displayRect.right = lpModes->iYRes / slsMap.grid.iSLSGridColumn;
				lpDisplayInfo->displayRect.bottom = lpModes->iXRes / slsMap.grid.iSLSGridRow;
			}
			lpDisplayInfo->displayRect.left =
				lpDisplayInfo->displayRect.right * lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX;
			lpDisplayInfo->displayRect.top =
				lpDisplayInfo->displayRect.bottom * lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY;
			if (bBezel == 1) {
				for (iCurrentSLSOffset = 0; iCurrentSLSOffset < iNumSLSOffset; ++iCurrentSLSOffset) {
					if (lpBezelMode[iCurrentBezelMode].iSLSModeIndex ==
							lpSLSOffset[iCurrentSLSOffset].iBezelModeIndex &&
						lpDisplayTarget[iCurrentDisplayTarget].displayID.iDisplayLogicalIndex ==
							lpSLSOffset[iCurrentSLSOffset].displayID.iDisplayLogicalIndex) {
						bFound = 1;
						break;
					}
				}
				if (!bFound) {
					iReturn = 0;
					break;
				}
				lpDisplayInfo->displayRectVisible.right = lpSLSOffset[iCurrentSLSOffset].iDisplayWidth;
				lpDisplayInfo->displayRectVisible.bottom = lpSLSOffset[iCurrentSLSOffset].iDisplayHeight;
				lpDisplayInfo->displayRectVisible.left = lpSLSOffset[iCurrentSLSOffset].iBezelOffsetX;
				lpDisplayInfo->displayRectVisible.top = lpSLSOffset[iCurrentSLSOffset].iBezelOffsetY;
			} else {
				lpDisplayInfo->displayRectVisible = lpDisplayInfo->displayRect;
			}
			if (iPreferredX == lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX &&
				iPreferredY == lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY) {
				lpDisplayInfo->iPreferredDisplay = 1;
			}
		}
		if (iMatchedModes && iReturn == 1) {
			lpEyefinityInfo->iSLSActive = 1;
			lpEyefinityInfo->iSLSGridWidth = slsMap.grid.iSLSGridColumn;
			lpEyefinityInfo->iSLSGridHeight = slsMap.grid.iSLSGridRow;
			if (!bRotated) {
				lpEyefinityInfo->iSLSWidth = iSLSWidth;
				lpEyefinityInfo->iSLSHeight = iSLSHeight;
			} else {
				lpEyefinityInfo->iSLSWidth = iSLSHeight;
				lpEyefinityInfo->iSLSHeight = iSLSWidth;
			}
			if (bBezel == 1) {
				lpEyefinityInfo->iBezelCompensatedDisplay = 1;
			}
			*lpNumDisplaysInfo = iNumDisplayTarget;
		}
		break;
	}
	// Displays whose modes ADL did not return take the size of the first display that has one.
	if (iMatchedModes && iFailedModes) {
		DisplayInfoStruct* lpReference = nullptr;
		for (iCurrentDisplayTarget = 0; iCurrentDisplayTarget < iNumDisplayTarget; ++iCurrentDisplayTarget) {
			if ((iFailedModes & 1 << iCurrentDisplayTarget) == 0) {
				lpReference = &(*lppDisplaysInfo)[iCurrentDisplayTarget];
				break;
			}
		}
		if (lpDisplayInfo) {
			for (iCurrentDisplayTarget = 0; iCurrentDisplayTarget < iNumDisplayTarget; ++iCurrentDisplayTarget) {
				if ((iFailedModes & 1 << iCurrentDisplayTarget) == 0) {
					continue;
				}
				for (iCurrentSLSTarget = 0; iCurrentSLSTarget < iNumSLSTarget; ++iCurrentSLSTarget) {
					if (lpDisplayTarget[iCurrentDisplayTarget].displayID.iDisplayLogicalIndex ==
						lpSLSTarget[iCurrentSLSTarget].displayTarget.displayID.iDisplayLogicalIndex) {
						bFound = 1;
						break;
					}
				}
				if (!bFound) {
					iReturn = 0;
					break;
				}
				lpDisplayInfo = &(*lppDisplaysInfo)[iCurrentDisplayTarget];
				lpDisplayInfo->iGridXCoord = lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX;
				lpDisplayInfo->iGridYCoord = lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY;
				lpDisplayInfo->displayRect.right = lpReference->displayRect.right;
				lpDisplayInfo->displayRect.bottom = lpReference->displayRect.bottom;
				lpDisplayInfo->displayRect.left =
					lpReference->displayRect.right * lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX;
				lpDisplayInfo->displayRect.top =
					lpReference->displayRect.bottom * lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY;
				if (bBezel == 1) {
					lpDisplayInfo->displayRectVisible.right = lpReference->displayRectVisible.right;
					lpDisplayInfo->displayRectVisible.bottom = lpReference->displayRectVisible.bottom;
					if (lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX == 0) {
						lpDisplayInfo->displayRectVisible.left = 0;
					} else if (lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX == slsMap.grid.iSLSGridColumn - 1) {
						lpDisplayInfo->displayRectVisible.left = iSLSWidth - lpReference->displayRectVisible.right;
					} else {
						lpDisplayInfo->displayRectVisible.left = lpDisplayInfo->displayRect.left +
							(lpReference->displayRect.right - lpReference->displayRectVisible.right) / 2;
					}
					if (lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY == 0) {
						lpDisplayInfo->displayRectVisible.top = 0;
					} else if (lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY == slsMap.grid.iSLSGridRow - 1) {
						lpDisplayInfo->displayRectVisible.top = iSLSHeight - lpReference->displayRectVisible.bottom;
					} else {
						lpDisplayInfo->displayRectVisible.top = lpDisplayInfo->displayRect.top +
							(lpReference->displayRect.bottom - lpReference->displayRectVisible.bottom) / 2;
					}
				} else {
					lpDisplayInfo->displayRectVisible = lpDisplayInfo->displayRect;
				}
				if (iPreferredX == lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionX &&
					iPreferredY == lpSLSTarget[iCurrentSLSTarget].iSLSGridPositionY) {
					lpDisplayInfo->iPreferredDisplay = 1;
				}
			}
		} else {
			iReturn = 0;
		}
	}
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpModes));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpSLSTarget));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpNativeMode));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpBezelMode));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpTransientMode));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpSLSOffset));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpDisplayMap));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpDisplayTarget));
	ADLMainMemoryFree(reinterpret_cast<void**>(&lpAdapterInfo));
	ADL_Main_Control_Destroy =
		reinterpret_cast<ADL_MAIN_CONTROL_DESTROY>(platform::LibrarySymbol(hDLL, "ADL_Main_Control_Destroy"));
	if (ADL_Main_Control_Destroy) {
		ADL_Main_Control_Destroy();
	}
	platform::LibraryFree(hDLL);
	if (!iReturn) {
		atiEyefinityReleaseConfigInfo(lppDisplaysInfo);
		*lpNumDisplaysInfo = 0;
		// The sample clears sizeof(lpEyefinityInfo) bytes, its pointer's 4 on the exe's x86: iSLSActive only.
		ourMemset(lpEyefinityInfo, 0, sizeof(lpEyefinityInfo->iSLSActive));
	}
	return iReturn;
}

void NiSourceTexture::SetUsePreloading(bool bPreload) {
	NiSourceTexture::ms_bPreload = bPreload;
}

void NiTexture::SetMipmapByDefault(bool bMipmap) {
	NiTexture::ms_bMipmapByDefault = bMipmap;
}
