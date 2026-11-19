#include "BSShader/ImageSpaceManager.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSShader/BSRenderedTexture.h"
#include "BSShader/BSShader.h"
#include "BSShader/BSShaderManager.h"
#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffect.h"
#include "BSShader/ImageSpaceEffectBloom.h"
#include "BSShader/ImageSpaceEffectBlur.h"
#include "BSShader/ImageSpaceEffectCinematic.h"
#include "BSShader/ImageSpaceEffectDepthOfField.h"
#include "BSShader/ImageSpaceEffectFullScreenBlur.h"
#include "BSShader/ImageSpaceEffectGetHit.h"
#include "BSShader/ImageSpaceEffectHDR.h"
#include "BSShader/ImageSpaceEffectInterface.h"
#include "BSShader/ImageSpaceEffectMap.h"
#include "BSShader/ImageSpaceEffectMenuBG.h"
#include "BSShader/ImageSpaceEffectMotionBlur.h"
#include "BSShader/ImageSpaceEffectNoise.h"
#include "BSShader/ImageSpaceEffectRadialBlur.h"
#include "BSShader/ImageSpaceEffectRefraction.h"
#include "BSShader/ImageSpaceEffectSunbeams.h"
#include "BSShader/ImageSpaceEffectTV.h"
#include "BSShader/ImageSpaceEffectVATSScan.h"
#include "BSShader/ImageSpaceEffectVolumetricFog.h"
#include "BSShader/ImageSpaceEffectWaterDisplacement.h"
#include "BSShader/ImageSpaceEffectWaterFFT.h"
#include "BSShader/ImageSpaceShader.h"
#include "BSShader/ImageSpaceShaderAlphaMult.h"
#include "BSShader/ImageSpaceShaderBPBlur11.h"
#include "BSShader/ImageSpaceShaderBPBlur13.h"
#include "BSShader/ImageSpaceShaderBPBlur15.h"
#include "BSShader/ImageSpaceShaderBPBlur9.h"
#include "BSShader/ImageSpaceShaderBlend.h"
#include "BSShader/ImageSpaceShaderBlur.h"
#include "BSShader/ImageSpaceShaderBlur11.h"
#include "BSShader/ImageSpaceShaderBlur13.h"
#include "BSShader/ImageSpaceShaderBlur15.h"
#include "BSShader/ImageSpaceShaderBlur3.h"
#include "BSShader/ImageSpaceShaderBlur5.h"
#include "BSShader/ImageSpaceShaderBlur7.h"
#include "BSShader/ImageSpaceShaderBlur9.h"
#include "BSShader/ImageSpaceShaderBrightPassBlur3.h"
#include "BSShader/ImageSpaceShaderBrightPassBlur5.h"
#include "BSShader/ImageSpaceShaderBrightPassBlur7.h"
#include "BSShader/ImageSpaceShaderBrightPassFilter.h"
#include "BSShader/ImageSpaceShaderCinematic.h"
#include "BSShader/ImageSpaceShaderCopy.h"
#include "BSShader/ImageSpaceShaderCopyRenderTarget1.h"
#include "BSShader/ImageSpaceShaderDS16LightAdapt.h"
#include "BSShader/ImageSpaceShaderDS16LumClamp.h"
#include "BSShader/ImageSpaceShaderDS4LightAdapt.h"
#include "BSShader/ImageSpaceShaderDS4LumClamp.h"
#include "BSShader/ImageSpaceShaderDepthOfField.h"
#include "BSShader/ImageSpaceShaderDepthOfFieldMask.h"
#include "BSShader/ImageSpaceShaderDoubleVision.h"
#include "BSShader/ImageSpaceShaderDownsample16.h"
#include "BSShader/ImageSpaceShaderDownsample4.h"
#include "BSShader/ImageSpaceShaderDownsample9.h"
#include "BSShader/ImageSpaceShaderFFTDisplayAmplitude.h"
#include "BSShader/ImageSpaceShaderFFTDisplayHighW.h"
#include "BSShader/ImageSpaceShaderFFTDisplayNormals.h"
#include "BSShader/ImageSpaceShaderFFTDisplayWaterHeight.h"
#include "BSShader/ImageSpaceShaderFFTFilter.h"
#include "BSShader/ImageSpaceShaderFFTHorizontalButterfly.h"
#include "BSShader/ImageSpaceShaderFFTHorizontalScramble.h"
#include "BSShader/ImageSpaceShaderFFTNormals.h"
#include "BSShader/ImageSpaceShaderFFTVerticalButterfly.h"
#include "BSShader/ImageSpaceShaderFFTVerticalScramble.h"
#include "BSShader/ImageSpaceShaderFFTWaterSpectrum.h"
#include "BSShader/ImageSpaceShaderHDRBlend.h"
#include "BSShader/ImageSpaceShaderHDRBlendCinematic.h"
#include "BSShader/ImageSpaceShaderHDRBlendCinematicAlphaMask.h"
#include "BSShader/ImageSpaceShaderHDRBlur.h"
#include "BSShader/ImageSpaceShaderLightAdapt.h"
#include "BSShader/ImageSpaceShaderLumClamp.h"
#include "BSShader/ImageSpaceShaderMap.h"
#include "BSShader/ImageSpaceShaderMenuBG.h"
#include "BSShader/ImageSpaceShaderMenuUnpack.h"
#include "BSShader/ImageSpaceShaderMotionBlur.h"
#include "BSShader/ImageSpaceShaderNULL.h"
#include "BSShader/ImageSpaceShaderNoiseNormalmap.h"
#include "BSShader/ImageSpaceShaderNoiseScrollAndBlend.h"
#include "BSShader/ImageSpaceShaderParam.h"
#include "BSShader/ImageSpaceShaderRadialBlur.h"
#include "BSShader/ImageSpaceShaderRadialBlurHigh.h"
#include "BSShader/ImageSpaceShaderRadialBlurMedium.h"
#include "BSShader/ImageSpaceShaderRefraction.h"
#include "BSShader/ImageSpaceShaderRestoreEDRAM.h"
#include "BSShader/ImageSpaceShaderScanlinesBlend.h"
#include "BSShader/ImageSpaceShaderScanlinesBlend_.h"
#include "BSShader/ImageSpaceShaderStencilCopy.h"
#include "BSShader/ImageSpaceShaderStencilCopyNoTextureNoDepth.h"
#include "BSShader/ImageSpaceShaderSunbeams.h"
#include "BSShader/ImageSpaceShaderTV.h"
#include "BSShader/ImageSpaceShaderTextureMask.h"
#include "BSShader/ImageSpaceShaderVATSScan.h"
#include "BSShader/ImageSpaceShaderVolumetricFog.h"
#include "BSShader/ImageSpaceTexture.h"
#include "BSShader/SkyShader.h"
#include "BSShader/WaterBlendHeightmaps.h"
#include "BSShader/WaterDisplaceClearSimulation.h"
#include "BSShader/WaterDisplaceTexOffset.h"
#include "BSShader/WaterDisplacementNormals.h"
#include "BSShader/WaterDisplacementRainRipple.h"
#include "BSShader/WaterDisplacementWadingRipple.h"
#include "BSShader/WaterRainHeightmap.h"
#include "BSShader/WaterSmoothHeightmap.h"
#include "BSShader/WaterWadingHeightmap.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiDX9Renderer/NiDX9Renderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCamera.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderedTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriShape.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPixelShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DRenderStateGroup.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DShader.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DTextureStage.h"
#include "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DVertexShader.h"
#include "SDK/MSVC/string.h"

#include <bit>
#include <cmath>
#include <cstring>
#include <utility>

void ImageSpaceParameterData::Endian() {
    for (float& rfData : pfHDRData) {
        rfData =
            std::bit_cast<float>(static_cast<uint32_t>(__builtin_bswap32((uint32_t)std::bit_cast<uint32_t>(rfData))));
    }
    uint32_t* pfBloomData = reinterpret_cast<uint32_t*>(this->pfBloomData);
    int32_t v3 = 3;
    do {
        *pfBloomData = __builtin_bswap32(*pfBloomData);
        ++pfBloomData;
        --v3;
    }
    while (v3);
    uint32_t* pfGetHitData = reinterpret_cast<uint32_t*>(this->pfGetHitData);
    int32_t v5 = 3;
    do {
        *pfGetHitData = __builtin_bswap32(*pfGetHitData);
        ++pfGetHitData;
        --v5;
    }
    while (v5);
    uint32_t* pfCinematicData = reinterpret_cast<uint32_t*>(this->pfCinematicData);
    int32_t v7 = 12;
    do {
        *pfCinematicData = __builtin_bswap32(*pfCinematicData);
        ++pfCinematicData;
        --v7;
    }
    while (v7);
    uiCinematicsEnabled = __builtin_bswap32(uiCinematicsEnabled);
}

ImageSpaceManager::ImageSpaceManager()
  : pEffectList(116, 1), pBaseParameterData(nullptr), pOverRideBaseParameterData(nullptr), bIsReady(false),
    iLastEffect(ImageSpaceManager::EFFECT_NONE), spScreenTriShape(nullptr), spScreenTriShapeData(nullptr),
    spPartialScreenTriShape(nullptr), bEnablePartialRender(false), spMenuUnpackGeom(nullptr), kTintColor(),
    kWeatherTintColor(), kFadeColor(), kWeatherFadeColor(), kRefractionTint(), CustomViewPort{} {
    std::memset(&CurrentParameterData, 0, sizeof(CurrentParameterData));
    Initialize();
}

// The NiPointer members, the depth texture and the effect list release themselves; the menu unpack geometry is a
// plain pointer the manager holds a reference on.
ImageSpaceManager::~ImageSpaceManager() {
    if (bIsReady) {
        ShutdownEffects();
        ShutdownGeometry();
        ImageSpaceShader::DefaultParam.ClearConstantGroups();
        BSRenderedTexture* pTarget = pSwapTarget;
        bIsReady = false;
        BSShaderManager::pTextureManager->ReturnRenderedTexture(pTarget);
    }
    if (spMenuUnpackGeom) {
        NiDecRefCount(spMenuUnpackGeom);
    }
}

BSRenderedTexture* ImageSpaceManager::GetDepthRenderedTexture() {
    ImageSpaceTexture* p_kDepthTexture = &DepthTexture;
    if (!(&DepthTexture)->GetTexture()) {
        p_kDepthTexture->BorrowRenderedTexture(BSTextureManager::TEX_SCENE_DEPTH_BUFFER);
    }
    return p_kDepthTexture->GetRenderedTexture();
}

void ImageSpaceManager::SetBaseParameterData(const ImageSpaceParameterData* apParameterData) {
    // Only the save and restore of a heap-context guard is kept here, and the new context is never stored, as the game
    // does. The thread's block is reached once.
    int32_t& riMemContext = GetThreadMemContext();
    const int32_t iOldMemContext = riMemContext;
    pBaseParameterData = apParameterData;
    riMemContext = iOldMemContext;
}

void ImageSpaceManager::InitDefaultParameter() {
    // pfHDRData[14] and uiCinematicsEnabled are left as they are.
    DefaultParameterData.pfHDRData[0] = 0.5f;
    DefaultParameterData.pfHDRData[1] = 7.0f;
    DefaultParameterData.pfHDRData[3] = 1.0f;
    DefaultParameterData.pfHDRData[4] = 1.0f;
    DefaultParameterData.pfHDRData[5] = 1.0f;
    DefaultParameterData.pfHDRData[6] = 2.25f;
    DefaultParameterData.pfHDRData[7] = 0.225f;
    DefaultParameterData.pfHDRData[8] = 1.0f;
    DefaultParameterData.pfHDRData[9] = 0.0f;
    DefaultParameterData.pfHDRData[10] = 0.0f;
    DefaultParameterData.pfHDRData[11] = 1.5f;
    DefaultParameterData.pfHDRData[12] = 1.5f;
    DefaultParameterData.pfHDRData[13] = 1.0f;
    DefaultParameterData.pfHDRData[2] = 1.0f;
    DefaultParameterData.pfBloomData[0] = 0.03f;
    DefaultParameterData.pfBloomData[2] = 0.2f;
    DefaultParameterData.pfBloomData[1] = 0.7f;
    DefaultParameterData.pfGetHitData[0] = 0.5f;
    DefaultParameterData.pfGetHitData[1] = 0.5f;
    DefaultParameterData.pfGetHitData[2] = 0.02f;
    DefaultParameterData.pfNightEyeData[3] = 2.5f;
    DefaultParameterData.pfNightEyeData[0] = 0.21f;
    DefaultParameterData.pfNightEyeData[1] = 0.5f;
    DefaultParameterData.pfNightEyeData[2] = 0.78f;
    DefaultParameterData.pfCinematicData[3] = 1.0f;
    DefaultParameterData.pfCinematicData[2] = 1.0f;
    DefaultParameterData.pfCinematicData[0] = 1.0f;
    DefaultParameterData.pfCinematicData[4] = 1.0f;
    DefaultParameterData.pfCinematicData[5] = 1.0f;
    DefaultParameterData.pfCinematicData[6] = 1.0f;
    DefaultParameterData.pfCinematicData[8] = 1.0f;
    DefaultParameterData.pfCinematicData[9] = 1.0f;
    DefaultParameterData.pfCinematicData[10] = 1.0f;
    DefaultParameterData.pfCinematicData[1] = 0.5f;
    DefaultParameterData.pfCinematicData[7] = 0.0f;
    DefaultParameterData.pfCinematicData[11] = 0.0f;
}

void ImageSpaceManager::ResetWeatherData() {
    // Only the HDR, bloom and cinematic parts take part in the weather blend.
    for (int32_t i = 0; i < 15; ++i) {
        WeatherParameterData[0].pfHDRData[i] = 1.0f;
        WeatherParameterData[1].pfHDRData[i] = 0.0f;
    }
    WeatherParameterData[0].pfBloomData[0] = 1.0f;
    WeatherParameterData[0].pfBloomData[1] = 1.0f;
    WeatherParameterData[0].pfBloomData[2] = 1.0f;
    WeatherParameterData[1].pfBloomData[0] = 0.0f;
    WeatherParameterData[1].pfBloomData[1] = 0.0f;
    WeatherParameterData[1].pfBloomData[2] = 0.0f;
    for (int32_t i = 0; i < 12; ++i) {
        WeatherParameterData[0].pfCinematicData[i] = 1.0f;
        WeatherParameterData[1].pfCinematicData[i] = 0.0f;
    }
}

void ImageSpaceManager::ApplyCurrentParameterData(bool abSkyTransition) {
    if (pEffectList.m_pBase[EFFECT_BLOOM]) {
        static_cast<ImageSpaceEffectBloom*>(pEffectList.m_pBase[EFFECT_BLOOM])
            ->SetParams(CurrentParameterData.pfBloomData);
    }
    if (pEffectList.m_pBase[EFFECT_HDR]) {
        static_cast<ImageSpaceEffectHDR*>(pEffectList.m_pBase[EFFECT_HDR])->SetParams(CurrentParameterData.pfHDRData);
    }
    if (pEffectList.m_pBase[EFFECT_GET_HIT]) {
        static_cast<ImageSpaceEffectGetHit*>(pEffectList.m_pBase[EFFECT_GET_HIT])
            ->SetParams(CurrentParameterData.pfGetHitData);
    }
    if (pEffectList.m_pBase[EFFECT_CINEMATIC]) {
        static_cast<ImageSpaceEffectCinematic*>(pEffectList.m_pBase[EFFECT_CINEMATIC])
            ->SetParams(CurrentParameterData.pfCinematicData, CurrentParameterData.uiCinematicsEnabled);
        static_cast<ImageSpaceEffectCinematic*>(pEffectList.m_pBase[EFFECT_CINEMATIC])
            ->SetNightEyeParams(CurrentParameterData.pfNightEyeData);
    }
    // During a sky transition only the ramp's upper end follows the image space.
    float fRampMax = CurrentParameterData.pfHDRData[8];
    if (!abSkyTransition) {
        SkyShader::pLUMRamp[0] = CurrentParameterData.pfHDRData[8];
        fRampMax = 1.0f;
    }
    SkyShader::pLUMRamp[1] = fRampMax;
    BSShaderManager::fSunlightDimmer = CurrentParameterData.pfHDRData[11];
    BSShaderManager::fTreeDimmer = CurrentParameterData.pfHDRData[13];
    BSShaderManager::fGrassDimmer = CurrentParameterData.pfHDRData[12];
    BSShaderManager::fSkinDimmer = CurrentParameterData.pfHDRData[2];
}

void ImageSpaceManager::ApplyTintColorParamModifier(NiColorA& akColor) {
    if (fHighestTintValue < akColor.a) {
        fHighestTintValue = akColor.a;
    }
    akColor.r = akColor.r * akColor.a;
    akColor.g = akColor.g * akColor.a;
    akColor.b = akColor.a * akColor.b;
    kTintColor.r = akColor.r + kTintColor.r;
    kTintColor.g = akColor.g + kTintColor.g;
    kTintColor.b = kTintColor.b + akColor.b;
    kTintColor.a = kTintColor.a + akColor.a;
}

void ImageSpaceManager::ApplyFadeColorParamModifier(NiColorA& akColor) {
    if (fHighestFadeValue < akColor.a) {
        fHighestFadeValue = akColor.a;
    }
    akColor.r = akColor.r * akColor.a;
    akColor.g = akColor.g * akColor.a;
    akColor.b = akColor.a * akColor.b;
    kFadeColor.r = akColor.r + kFadeColor.r;
    kFadeColor.g = akColor.g + kFadeColor.g;
    kFadeColor.b = kFadeColor.b + akColor.b;
    kFadeColor.a = kFadeColor.a + akColor.a;
}

void ImageSpaceManager::BeginParamModifiers() {
    // The image space's own tint (cinematic data 4 to 7) starts the tint; everything else starts cleared.
    const float fTintAlpha = CurrentParameterData.pfCinematicData[7];
    fHighestTintValue = fTintAlpha;
    kTintColor.r = CurrentParameterData.pfCinematicData[4] * fTintAlpha;
    kTintColor.g = CurrentParameterData.pfCinematicData[5] * fTintAlpha;
    kTintColor.b = fTintAlpha * CurrentParameterData.pfCinematicData[6];
    kTintColor.a = CurrentParameterData.pfCinematicData[7];
    kWeatherTintColor.r = 0.0f;
    kWeatherTintColor.g = 0.0f;
    kWeatherTintColor.b = 0.0f;
    kWeatherTintColor.a = 0.0f;
    fHighestFadeValue = 0.0f;
    kFadeColor.r = 0.0f;
    kFadeColor.g = 0.0f;
    kFadeColor.b = 0.0f;
    kFadeColor.a = 0.0f;
    kWeatherFadeColor.r = 0.0f;
    kWeatherFadeColor.g = 0.0f;
    kWeatherFadeColor.b = 0.0f;
    kWeatherFadeColor.a = 0.0f;
    fHighestBlurValue = 0.0f;
    fHighestDoubleValue = 0.0f;
    fHighestRadialBlurStrengthValue = 0.0f;
    fCurrentRadialBlurRampupValue = 0.0f;
    fCurrentRadialBlurStartValue = 0.0f;
    fCurrentRadialBlurRampDownValue = 0.0f;
    fCurrentRadialBlurDownStartValue = 0.0f;
    fCurrentRadialBlurCenter.x = 0.0f;
    fCurrentRadialBlurCenter.y = 0.0f;
    fHighestDOFStrengthValue = 0.0f;
    fCurrentDOFDistanceValue = 0.0f;
    fCurrentDOFRangeValue = 0.0f;
    fCurrentDOFMaskConst = 0.0f;
    fHighestDepthRadialBlurStrengthValue = 0.0f;
    fHighestMotionBlurStrengthValue = 0.0f;
    fCurrentDOFMode = ImageSpaceEffectDepthOfField::DOF_NONE;
    fWeatherBlurValue = 0.0f;
    bCurrentDOFUseMask = false;
    fWeatherDoubleValue = 0.0f;
    fWeatherDOFMode = ImageSpaceEffectDepthOfField::DOF_NONE;
    fWeatherRadialBlurStrengthValue = 0.0f;
    fWeatherRadialBlurRampupValue = 0.0f;
    fWeatherRadialBlurStartValue = 0.0f;
    fWeatherRadialBlurRampDownValue = 0.0f;
    fWeatherRadialBlurDownStartValue = 0.0f;
    bWeatherDOFUseMask = false;
    fWeatherRadialBlurCenter.x = 0.0f;
    fWeatherRadialBlurCenter.y = 0.0f;
    fWeatherDOFStrengthValue = 0.0f;
    fWeatherDOFDistanceValue = 0.0f;
    fWeatherDOFRangeValue = 0.0f;
    fWeatherDOFMaskConst = 0.0f;
    fWeatherMotionBlurStrengthValue = 0.0f;
    std::memset(WeatherParameterData, 0, sizeof(WeatherParameterData));
}

// Creates the effects, the screen geometry and the default shader parameters and takes the swap target; the second call
// and later ones do nothing.
void ImageSpaceManager::Initialize() {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    if (!bIsReady) {
        InitEffects();
        InitGeometry();
        bIsReady = true;
        SetupEffects();
        ImageSpaceShader::DefaultParam.InitConstantMap(0, 0);
        InitDefaultParameter();
        kRefractionTint.r = 0.0f;
        kRefractionTint.g = 0.0f;
        kRefractionTint.b = 0.0f;
        kRefractionTint.a = 0.3f;
        CustomViewPort.m_left = 0.0f;
        CustomViewPort.m_right = 1.0f;
        CustomViewPort.m_top = 1.0f;
        CustomViewPort.m_bottom = 0.0f;
        pEOFDest = nullptr;
        pSwapTarget = BSShaderManager::pTextureManager->BorrowRenderedTexture(
            static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer), BSTextureManager::TEX_IMAGESPACE_SWAP, 0, nullptr, 0
        );
    }
}

// Creates every effect and shader and stores it in pEffectList at its fixed index, then loads the shaders' programs.
// The shaders are stored as their ImageSpaceEffect subobject.
void ImageSpaceManager::InitEffects() {
    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectVolumetricFog));
    ImageSpaceEffect* pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectVolumetricFog : nullptr;
    SetEffectAt(&pEffectList, 13, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectRefraction));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectRefraction : nullptr;
    SetEffectAt(&pEffectList, 2, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectGetHit));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectGetHit : nullptr;
    SetEffectAt(&pEffectList, EFFECT_GET_HIT, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectHDR));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectHDR : nullptr;
    SetEffectAt(&pEffectList, EFFECT_HDR, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectBloom));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectBloom : nullptr;
    SetEffectAt(&pEffectList, EFFECT_BLOOM, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectCinematic));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectCinematic : nullptr;
    SetEffectAt(&pEffectList, EFFECT_CINEMATIC, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectInterface));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectInterface : nullptr;
    SetEffectAt(&pEffectList, 10, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectVATSScan));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectVATSScan : nullptr;
    SetEffectAt(&pEffectList, 11, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectDepthOfField));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectDepthOfField : nullptr;
    SetEffectAt(&pEffectList, 4, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectMotionBlur));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectMotionBlur : nullptr;
    SetEffectAt(&pEffectList, EFFECT_MOTION_BLUR, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectFullScreenBlur));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectFullScreenBlur : nullptr;
    SetEffectAt(&pEffectList, 7, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectDepthRadialBlur));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectDepthRadialBlur : nullptr;
    SetEffectAt(&pEffectList, 5, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectRadialBlur));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectRadialBlur : nullptr;
    SetEffectAt(&pEffectList, 6, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectMap));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectMap : nullptr;
    SetEffectAt(&pEffectList, 14, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectMenuBG));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectMenuBG : nullptr;
    SetEffectAt(&pEffectList, EFFECT_MENU_BG, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectWaterFFT));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectWaterFFT : nullptr;
    SetEffectAt(&pEffectList, 31, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectWaterDisplacement));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectWaterDisplacement : nullptr;
    SetEffectAt(&pEffectList, 32, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectNoise));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectNoise : nullptr;
    SetEffectAt(&pEffectList, 33, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectSunbeams));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectSunbeams : nullptr;
    SetEffectAt(&pEffectList, 9, &pEffect);
    pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectTV));
    pEffect = pMemory ? ::new (pMemory) ImageSpaceEffectTV : nullptr;
    SetEffectAt(&pEffectList, 30, &pEffect);

    // The blur effects of radius 1 to 7: plain ones at 16-22, bright-pass ones at 23-29.
    for (int32_t iRadius = 1; iRadius <= 7; ++iRadius) {
        pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectBlur));
        ImageSpaceEffectBlur* pBlur = pMemory ? ::new (pMemory) ImageSpaceEffectBlur : nullptr;
        pBlur->SetRadius(iRadius);
        pBlur->SetBrightPass(false);
        pEffectList.SetAt(uint32_t(iRadius + 15), pBlur);
        pMemory = MemoryManager_Singleton.Allocate(sizeof(ImageSpaceEffectBlur));
        pBlur = pMemory ? ::new (pMemory) ImageSpaceEffectBlur : nullptr;
        pBlur->SetRadius(iRadius);
        pBlur->SetBrightPass(true);
        pEffectList.SetAt(uint32_t(iRadius + 22), pBlur);
    }

    ImageSpaceShader* pShader = new ImageSpaceShaderCopy;
    pEffectList.SetAt(EFFECT_SHADER_COPY, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderCopyRenderTarget1;
    pEffectList.SetAt(35, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderRestoreEDRAM;
    pEffectList.SetAt(36, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderNULL;
    pEffectList.SetAt(37, static_cast<ImageSpaceEffect*>(pShader));
    // A second plain copy shader.
    pShader = new ImageSpaceShaderCopy;
    pEffectList.SetAt(38, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderStencilCopy;
    pEffectList.SetAt(39, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderStencilCopyNoTextureNoDepth;
    pEffectList.SetAt(EFFECT_SHADER_COPY_STENCIL_NO_TEXTURE_NO_DEPTH, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderRefraction;
    pEffectList.SetAt(41, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderVolumetricFog;
    pEffectList.SetAt(42, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur;
    pEffectList.SetAt(43, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDoubleVision;
    pEffectList.SetAt(44, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderAlphaMult;
    pEffectList.SetAt(45, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlend;
    pEffectList.SetAt(46, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderTextureMask;
    pEffectList.SetAt(47, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderCinematic;
    pEffectList.SetAt(EFFECT_CINEMATIC_SHADER, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDepthOfField;
    pEffectList.SetAt(51, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDepthOfFieldMask;
    pEffectList.SetAt(52, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderMap;
    pEffectList.SetAt(48, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderMenuBG;
    pEffectList.SetAt(49, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderMotionBlur;
    pEffectList.SetAt(53, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderRadialBlur;
    pEffectList.SetAt(54, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderRadialBlurMedium;
    pEffectList.SetAt(55, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderRadialBlurHigh;
    pEffectList.SetAt(56, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderSunbeams;
    pEffectList.SetAt(57, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderHDRBlend;
    pEffectList.SetAt(58, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderHDRBlendCinematic;
    pEffectList.SetAt(59, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderHDRBlendCinematicAlphaMask;
    pEffectList.SetAt(60, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderHDRBlur;
    pEffectList.SetAt(61, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBrightPassFilter;
    pEffectList.SetAt(62, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDownsample16;
    pEffectList.SetAt(63, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDownsample9;
    pEffectList.SetAt(64, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDownsample4;
    pEffectList.SetAt(65, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDS4LightAdapt;
    pEffectList.SetAt(67, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDS4LumClamp;
    pEffectList.SetAt(66, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDS16LightAdapt;
    pEffectList.SetAt(69, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderDS16LumClamp;
    pEffectList.SetAt(68, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderLightAdapt;
    pEffectList.SetAt(70, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderLumClamp;
    pEffectList.SetAt(71, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderScanlinesBlend_;
    pEffectList.SetAt(72, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderScanlinesBlend;
    pEffectList.SetAt(73, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderVATSScan;
    pEffectList.SetAt(EFFECT_VATS_SCAN_SHADER, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderMenuUnpack;
    pEffectList.SetAt(76, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur3;
    pEffectList.SetAt(78, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur5;
    pEffectList.SetAt(79, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur7;
    pEffectList.SetAt(80, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur9;
    pEffectList.SetAt(81, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur11;
    pEffectList.SetAt(82, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur13;
    pEffectList.SetAt(83, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBlur15;
    pEffectList.SetAt(84, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBrightPassBlur3;
    pEffectList.SetAt(85, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBrightPassBlur5;
    pEffectList.SetAt(86, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBrightPassBlur7;
    pEffectList.SetAt(87, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBPBlur9;
    pEffectList.SetAt(88, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBPBlur11;
    pEffectList.SetAt(89, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBPBlur13;
    pEffectList.SetAt(90, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderBPBlur15;
    pEffectList.SetAt(91, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTWaterSpectrum;
    pEffectList.SetAt(93, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTHorizontalButterfly;
    pEffectList.SetAt(94, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTVerticalButterfly;
    pEffectList.SetAt(95, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTHorizontalScramble;
    pEffectList.SetAt(96, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTVerticalScramble;
    pEffectList.SetAt(97, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTNormals;
    pEffectList.SetAt(98, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTFilter;
    pEffectList.SetAt(99, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTDisplayNormals;
    pEffectList.SetAt(100, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTDisplayHighW;
    pEffectList.SetAt(101, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTDisplayAmplitude;
    pEffectList.SetAt(102, static_cast<ImageSpaceEffect*>(pShader));
    // A second water spectrum shader.
    pShader = new ImageSpaceShaderFFTWaterSpectrum;
    pEffectList.SetAt(103, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderFFTDisplayWaterHeight;
    pEffectList.SetAt(104, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterDisplaceClearSimulation;
    pEffectList.SetAt(105, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterDisplaceTexOffset;
    pEffectList.SetAt(106, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterDisplacementWadingRipple;
    pEffectList.SetAt(107, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterDisplacementRainRipple;
    pEffectList.SetAt(108, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterWadingHeightmap;
    pEffectList.SetAt(109, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterRainHeightmap;
    pEffectList.SetAt(110, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterBlendHeightmaps;
    pEffectList.SetAt(111, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterSmoothHeightmap;
    pEffectList.SetAt(112, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new WaterDisplacementNormals;
    pEffectList.SetAt(113, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderNoiseScrollAndBlend;
    pEffectList.SetAt(114, static_cast<ImageSpaceEffect*>(pShader));
    pShader = new ImageSpaceShaderNoiseNormalmap;
    pEffect = pShader;
    SetEffectAt(&pEffectList, 115, &pEffect);
    pShader = new ImageSpaceShaderTV;
    pEffect = pShader;
    SetEffectAt(&pEffectList, 77, &pEffect);

    // Every slot from the copy shader on holds a shader: load its programs (BSShader::ReloadShaders).
    for (int32_t i = EFFECT_SHADER_COPY; i < 116; ++i) {
        if (pEffectList.m_pBase[i]) {
            static_cast<ImageSpaceShader*>(pEffectList.m_pBase[i])->ReloadShaders();
        }
    }
}

void ImageSpaceManager::InitGeometry() {
    NiScreenElementsData* pData = new NiScreenElementsData(false, false, 1, 1, 1, 4, 1, 2, 1);
    spScreenTriShapeData = pData;
    NiScreenElements* pScreen = new NiScreenElements(spScreenTriShapeData.m_pObject);
    spScreenTriShape = pScreen;

    // One full-screen quad in clip space, textured over the whole source.
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)->Insert(4, 0, nullptr);
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)
        ->SetRectangle(0, -1.0f, 1.0f, 2.0f, -2.0f);
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)
        ->SetTextures(0, 0, 0.0f, 0.0f, 1.0f, 1.0f);
    static_cast<NiScreenElementsData*>(spScreenTriShape->m_spModelData.m_pObject)->UpdateBound();

    NiGlobalStringTable::GlobalStringHandle kName = NiGlobalStringTable::AddString("Imagespace Effect Geometry");
    spScreenTriShape->SetName(reinterpret_cast<const NiFixedString&>(kName));
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    spScreenTriShape->UpdateProperties();

    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    spScreenTriShape->Update(kUpdateData);
}

// Applies afValue's multiplier then addend to the effect's current parameter and moves the parameter back toward
// its old value by the change scaled by afStrength: x - (x - f(x)) * afStrength.
void ImageSpaceManager::ApplyParamModifier(
    ImageSpaceManager::EffectID aeEffect,
    uint32_t uiParam,
    const float* afValue,
    float afStrength
) {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    // Any other effect leaves the pointer null, and it is read through deliberately, as the game does.
    float* pfParam = nullptr;
    switch (aeEffect) {
        case EFFECT_BLOOM: pfParam = &CurrentParameterData.pfBloomData[uiParam]; break;
        case EFFECT_HDR: pfParam = &CurrentParameterData.pfHDRData[uiParam]; break;
        case EFFECT_CINEMATIC: pfParam = &CurrentParameterData.pfCinematicData[uiParam]; break;
    }
    // Operation 0 multiplies by afValue[0], operation 1 adds afValue[1].
    float fValue = *pfParam;
    for (int32_t i = 0; i < 2; ++i) {
        const float fOperand = afValue[i];
        float fResult = 0.0f;
        switch (i) {
            case 0: fResult = fOperand * fValue; break;
            case 1: fResult = fOperand + fValue; break;
        }
        fValue = fResult;
    }
    const float fDelta = *pfParam - fValue;
    *pfParam = *pfParam - fDelta * afStrength;
}

void ImageSpaceManager::ApplyParamModifierWeather(
    ImageSpaceManager::EffectID aeEffect,
    uint32_t uiParam,
    const float* afValue,
    float afStrength
) {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    for (int32_t i = 0; i < 2; ++i) {
        ImageSpaceParameterData& rData = WeatherParameterData[i];
        switch (aeEffect) {
            case EFFECT_BLOOM: rData.pfBloomData[uiParam] = afValue[i] * afStrength + rData.pfBloomData[uiParam]; break;
            case EFFECT_HDR: rData.pfHDRData[uiParam] = afValue[i] * afStrength + rData.pfHDRData[uiParam]; break;
            case EFFECT_CINEMATIC:
                rData.pfCinematicData[uiParam] = afValue[i] * afStrength + rData.pfCinematicData[uiParam];
                break;
        }
    }
}

void ImageSpaceManager::EndWeatherModifiers() {
    // Each parameter gets the weather's multiplier and addend at full strength. The steps of ApplyParamModifier are
    // written out here with the strength folded away, and the delta is subtracted unscaled.
    auto ApplyWeatherParam = [](float& rfParam, float fMultiplier, float fAddend) {
        AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
        const float afValue[2] = {fMultiplier, fAddend};
        // Operation 0 multiplies by afValue[0], operation 1 adds afValue[1].
        float fValue = rfParam;
        for (int32_t i = 0; i < 2; ++i) {
            const float fOperand = afValue[i];
            float fResult = 0.0f;
            switch (i) {
                case 0: fResult = fOperand * fValue; break;
                case 1: fResult = fOperand + fValue; break;
            }
            fValue = fResult;
        }
        const float fDelta = rfParam - fValue;
        rfParam = rfParam - fDelta;
    };
    for (uint32_t i = 0; i < 15; ++i) {
        ApplyWeatherParam(
            CurrentParameterData.pfHDRData[i], WeatherParameterData[0].pfHDRData[i],
            WeatherParameterData[1].pfHDRData[i]
        );
    }
    for (uint32_t i = 0; i < 3; ++i) {
        ApplyWeatherParam(
            CurrentParameterData.pfBloomData[i], WeatherParameterData[0].pfBloomData[i],
            WeatherParameterData[1].pfBloomData[i]
        );
    }
    for (uint32_t i = 0; i < 12; ++i) {
        ApplyWeatherParam(
            CurrentParameterData.pfCinematicData[i], WeatherParameterData[0].pfCinematicData[i],
            WeatherParameterData[1].pfCinematicData[i]
        );
    }
    ApplyTintColorParamModifier(kWeatherTintColor);
    ApplyFadeColorParamModifier(kWeatherFadeColor);
    // The weather's depth of field has no mask constant.
    if (fHighestBlurValue < fWeatherBlurValue) {
        fHighestBlurValue = fWeatherBlurValue;
    }
    if (fHighestDoubleValue < fWeatherDoubleValue) {
        fHighestDoubleValue = fWeatherDoubleValue;
    }
    if (fHighestRadialBlurStrengthValue < fWeatherRadialBlurStrengthValue) {
        fHighestRadialBlurStrengthValue = fWeatherRadialBlurStrengthValue;
        fCurrentRadialBlurRampupValue = fWeatherRadialBlurRampupValue;
        fCurrentRadialBlurCenter = fWeatherRadialBlurCenter;
        fCurrentRadialBlurStartValue = fWeatherRadialBlurStartValue;
        fCurrentRadialBlurRampDownValue = fWeatherRadialBlurRampDownValue;
        fCurrentRadialBlurDownStartValue = fWeatherRadialBlurDownStartValue;
    }
    uint32_t eDOFMode = fWeatherDOFMode;
    const float fDOFDistance = fWeatherDOFDistanceValue;
    const float fDOFRange = fWeatherDOFRangeValue;
    if (fHighestDOFStrengthValue < fWeatherDOFStrengthValue) {
        fHighestDOFStrengthValue = fWeatherDOFStrengthValue;
        bCurrentDOFUseMask = false;
        // Modes above 2 are the masked variants of modes 0 to 2.
        if (eDOFMode > 2) {
            bCurrentDOFUseMask = true;
            eDOFMode -= 3;
        }
        fCurrentDOFMode = static_cast<ImageSpaceEffectDepthOfField::DepthOfFieldMode>(eDOFMode);
        fCurrentDOFDistanceValue = fDOFDistance;
        fCurrentDOFRangeValue = fDOFRange;
        fCurrentDOFMaskConst = 0.0f;
    }
    if (fHighestMotionBlurStrengthValue < fWeatherMotionBlurStrengthValue) {
        fHighestMotionBlurStrengthValue = fWeatherMotionBlurStrengthValue;
    }
}

void ImageSpaceManager::RenderEndOfFrameEffects(
    NiDX9Renderer* apRenderer,
    BSRenderedTexture*& apScreenBuffer,
    BSRenderedTexture*& apDestinationBuffer
) {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    PrepareEffectRange();
    pEOFDest = apDestinationBuffer;
    if (int32_t(iLastEffect) == -1) {
        RenderEffect(EFFECT_SHADER_COPY, apRenderer, apScreenBuffer, apDestinationBuffer, nullptr, true);
        return;
    }
    // The active effects ping-pong between the screen buffer and the swap target; from the second effect on, a
    // third buffer (when more than two are active) takes the swap target's place. The last one writes the
    // destination.
    BSRenderedTexture* pSource = apScreenBuffer;
    BSRenderedTexture* pBorrowed = nullptr;
    BSRenderedTexture* pDest;
    if (iActiveEffectsCount == 1) {
        pDest = apDestinationBuffer;
    }
    else {
        pDest = pSwapTarget;
        if (iActiveEffectsCount > 2) {
            pBorrowed = BSShaderManager::pTextureManager->BorrowRenderedTexture(
                apRenderer, BSTextureManager::TEX_IMAGESPACE_SWAP, 0, nullptr, 0
            );
        }
    }
    int32_t iRendered = 0;
    for (int32_t i = 0; i <= int32_t(iLastEffect); ++i) {
        if (iRendered == 1 && pBorrowed) {
            pDest = pBorrowed;
        }
        if (i == int32_t(iLastEffect)) {
            pDest = apDestinationBuffer;
        }
        ImageSpaceEffect* pEffect = pEffectList.m_pBase[i];
        if (pEffect && pEffect->IsActive()) {
            RenderEffect(static_cast<EffectID>(i), apRenderer, pSource, pDest, nullptr, true);
            ++iRendered;
            std::swap(pSource, pDest);
        }
    }
    if (pBorrowed) {
        BSShaderManager::pTextureManager->ReturnRenderedTexture(pBorrowed);
    }
}

// Renders one effect with apDestTarget as its only input: the texture slot is bound as input 0, the effect renders the
// full-screen geometry (the partial one while partial rendering is on) and the input is cleared again.
void ImageSpaceManager::RenderEffect(
    ImageSpaceManager::EffectID aiEffect,
    NiDX9Renderer* apRenderer,
    BSRenderedTexture* apDestTarget,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    ImageSpaceEffect* pEffect = pEffectList.m_pBase[aiEffect];
    ImageSpaceTexture kTexture;
    kTexture.bForceAniso = false;
    kTexture.bIsRenderedTexture = false;
    kTexture.bIsTextureBorrowed = false;
    kTexture.spTexture = nullptr;
    kTexture.eFilterMode = 0;
    kTexture.eClampMode = 0;
    kTexture.SetTexture(apDestTarget);
    if (pEffect) {
        pEffect->SetTexture(0, &kTexture, NiTexturingProperty::FILTER_NEAREST);
        // Tests the flag's whole byte; reading it as bool would assume 0/1.
        NiScreenElements* pGeometry = reinterpret_cast<const uint8_t&>(bEnablePartialRender) != 0
            ? spPartialScreenTriShape.m_pObject
            : spScreenTriShape.m_pObject;
        pEffect->Render(pGeometry, apRenderer, apParam, abEndFrame);
        pEffect->SetTexture(0, nullptr, NiTexturingProperty::FILTER_NEAREST);
    }
}

// Indexes the effect list and hands the effect to the six-argument form, which sets the image space heap context
// and does the work.
void ImageSpaceManager::RenderEffect(
    ImageSpaceManager::EffectID aiEffect,
    NiDX9Renderer* apRenderer,
    BSRenderedTexture* apSourceBuffer,
    BSRenderedTexture* apDestinationBuffer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    // Forwards abEndFrame's whole incoming value.
    RenderEffect(pEffectList.m_pBase[aiEffect], apRenderer, apSourceBuffer, apDestinationBuffer, apParam, abEndFrame);
}

// The source-texture overload: the effect reads apSourceTarget and writes apDestTarget.
void ImageSpaceManager::RenderEffect(
    ImageSpaceManager::EffectID aiEffect,
    NiDX9Renderer* apRenderer,
    NiTexture* apSourceTarget,
    BSRenderedTexture* apDestTarget,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    // Forwards abEndFrame's whole incoming value.
    RenderEffect(pEffectList.m_pBase[aiEffect], apRenderer, apSourceTarget, apDestTarget, apParam, abEndFrame);
}

void ImageSpaceManager::RenderEffect(
    ImageSpaceEffect* apEffect,
    NiDX9Renderer* apRenderer,
    BSRenderedTexture* apSourceBuffer,
    BSRenderedTexture* apDestinationBuffer,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    ImageSpaceTexture dest;
    ImageSpaceTexture src;
    TLSData* v8 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v8->eMemContext;
    v8->eMemContext = MEM_CONTEXT_IMAGE_SPACE;
    src.bForceAniso = false;
    src.bIsRenderedTexture = false;
    src.bIsTextureBorrowed = false;
    src.spTexture = nullptr;
    src.eFilterMode = 0;
    src.eClampMode = 0;
    dest.bForceAniso = false;
    dest.bIsRenderedTexture = false;
    dest.bIsTextureBorrowed = false;
    dest.spTexture = nullptr;
    dest.eFilterMode = 0;
    dest.eClampMode = 0;
    src.SetTexture(apSourceBuffer);
    dest.SetTexture(apDestinationBuffer);
    if (apEffect) {
        apEffect->SetTexture(0, &dest, NiTexturingProperty::FILTER_NEAREST);
        apEffect->SetTexture(1, &src, NiTexturingProperty::FILTER_NEAREST);
        NiPointer<NiScreenElements>* p_spPartialScreenTriShape = &spPartialScreenTriShape;
        if (!bEnablePartialRender) {
            p_spPartialScreenTriShape = &spScreenTriShape;
        }
        apEffect->Render(p_spPartialScreenTriShape->m_pObject, apRenderer, apParam, abEndFrame);
        apEffect->SetTexture(0, nullptr, NiTexturingProperty::FILTER_NEAREST);
        apEffect->SetTexture(1, nullptr, NiTexturingProperty::FILTER_NEAREST);
    }
    v8->eMemContext = eMemContext;
}

void ImageSpaceManager::RenderEffect(
    ImageSpaceEffect* apEffect,
    NiDX9Renderer* apRenderer,
    NiTexture* apSourceTarget,
    BSRenderedTexture* apDestTarget,
    ImageSpaceEffectParam* apParam,
    int32_t abEndFrame
) {
    ImageSpaceTexture dest;
    ImageSpaceTexture src;
    TLSData* v8 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v8->eMemContext;
    v8->eMemContext = MEM_CONTEXT_IMAGE_SPACE;
    src.bForceAniso = false;
    src.bIsRenderedTexture = false;
    src.bIsTextureBorrowed = false;
    src.spTexture = nullptr;
    src.eFilterMode = 0;
    src.eClampMode = 0;
    dest.bForceAniso = false;
    dest.bIsRenderedTexture = false;
    dest.bIsTextureBorrowed = false;
    dest.spTexture = nullptr;
    dest.eFilterMode = 0;
    dest.eClampMode = 0;
    src.SetTexture(apSourceTarget);
    dest.SetTexture(apDestTarget);
    if (apEffect) {
        apEffect->SetTexture(0, &dest, NiTexturingProperty::FILTER_NEAREST);
        apEffect->SetTexture(1, &src, NiTexturingProperty::FILTER_NEAREST);
        NiPointer<NiScreenElements>* p_spPartialScreenTriShape = &spPartialScreenTriShape;
        if (!bEnablePartialRender) {
            p_spPartialScreenTriShape = &spScreenTriShape;
        }
        apEffect->Render(p_spPartialScreenTriShape->m_pObject, apRenderer, apParam, abEndFrame);
        apEffect->SetTexture(0, nullptr, NiTexturingProperty::FILTER_NEAREST);
        apEffect->SetTexture(1, nullptr, NiTexturingProperty::FILTER_NEAREST);
    }
    v8->eMemContext = eMemContext;
}

void ImageSpaceManager::ModBlurRadius(float afRadius) {
    fWeatherBlurValue = afRadius + fWeatherBlurValue;
}

void ImageSpaceManager::ModFullScreenMotionBlurStrength(float afStrength) {
    fWeatherMotionBlurStrengthValue = afStrength + fWeatherMotionBlurStrengthValue;
}

void ImageSpaceManager::ModDoubleVisionStrength(float afStrength) {
    fWeatherDoubleValue = afStrength + fWeatherDoubleValue;
}

void ImageSpaceManager::ApplyTintColorParamModifierWeather(const NiColorA& akColor) {
    kWeatherTintColor.r = kWeatherTintColor.r + akColor.r;
    kWeatherTintColor.g = akColor.g + kWeatherTintColor.g;
    kWeatherTintColor.b = akColor.b + kWeatherTintColor.b;
    kWeatherTintColor.a = akColor.a + kWeatherTintColor.a;
}

void ImageSpaceManager::ModFadeColor(const NiColorA& akColor) {
    kWeatherFadeColor.r = kWeatherFadeColor.r + akColor.r;
    kWeatherFadeColor.g = akColor.g + kWeatherFadeColor.g;
    kWeatherFadeColor.b = akColor.b + kWeatherFadeColor.b;
    kWeatherFadeColor.a = akColor.a + kWeatherFadeColor.a;
}

void ImageSpaceManager::ModDepthOfField(
    float afStrength,
    int32_t aeMode,
    float afMaskConst,
    float afDistance,
    float afRange
) {
    float fStrength = fWeatherDOFStrengthValue + afStrength;
    bWeatherDOFUseMask = false;
    fWeatherDOFMode = static_cast<ImageSpaceEffectDepthOfField::DepthOfFieldMode>(aeMode);
    fWeatherDOFStrengthValue = fStrength;
    fWeatherDOFDistanceValue = fWeatherDOFDistanceValue + afDistance;
    fWeatherDOFRangeValue = fWeatherDOFRangeValue + afRange;
    fWeatherDOFMaskConst = fWeatherDOFMaskConst + afMaskConst;
}

void ImageSpaceManager::ModRadialBlur(
    float afStrength,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiPoint2* apCenter
) {
    fWeatherRadialBlurStrengthValue = afStrength + fWeatherRadialBlurStrengthValue;
    fWeatherRadialBlurRampupValue = fWeatherRadialBlurRampupValue + afRampup;
    fWeatherRadialBlurStartValue = fWeatherRadialBlurStartValue + afStart;
    fWeatherRadialBlurRampDownValue = fWeatherRadialBlurRampDownValue + afRampdown;
    fWeatherRadialBlurDownStartValue = afDownStart + fWeatherRadialBlurDownStartValue;
    fWeatherRadialBlurCenter.x = apCenter->x + fWeatherRadialBlurCenter.x;
    fWeatherRadialBlurCenter.y = apCenter->y + fWeatherRadialBlurCenter.y;
}

void ImageSpaceManager::ApplyBlurRadiusParamModifier(float afBlurRadius) {
    if (fHighestBlurValue < afBlurRadius) {
        fHighestBlurValue = afBlurRadius;
    }
}

void ImageSpaceManager::ApplyMotionBlurStrengthParamModifier(float afBlurRadius) {
    if (fHighestMotionBlurStrengthValue < afBlurRadius) {
        fHighestMotionBlurStrengthValue = afBlurRadius;
    }
}

void ImageSpaceManager::ApplyDoubleVisionParamModifier(float afDoubleVision) {
    if (fHighestDoubleValue < afDoubleVision) {
        fHighestDoubleValue = afDoubleVision;
    }
}

void ImageSpaceManager::ApplyRadialBlurParamModifier(
    float afStrength,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiPoint2& aCenter
) {
    if (fHighestRadialBlurStrengthValue < afStrength) {
        fHighestRadialBlurStrengthValue = afStrength;
        fCurrentRadialBlurRampupValue = afRampup;
        fCurrentRadialBlurStartValue = afStart;
        fCurrentRadialBlurRampDownValue = afRampdown;
        fCurrentRadialBlurDownStartValue = afDownStart;
        fCurrentRadialBlurCenter = aCenter;
    }
}

// The world-bound overload: GetBoundParams reads the bound's screen-space centre and the start bias of the blur's
// ramp, and the bias is added to both start values before the strongest-contribution test.
void ImageSpaceManager::ApplyRadialBlurParamModifier(
    float afStrength,
    float afRampup,
    float afStart,
    float afRampdown,
    float afDownStart,
    const NiBound& aBound
) {
    NiPoint2 kCenter;
    float fStartBias;
    ImageSpaceEffectRadialBlur::GetBoundParams(aBound, kCenter, fStartBias);
    ApplyRadialBlurParamModifier(
        afStrength, afRampup, afStart + fStartBias, afRampdown, afDownStart + fStartBias, kCenter
    );
}

void ImageSpaceManager::ApplyDepthOfFieldParamModifier(
    float afStrength,
    uint32_t auiMode,
    float afMaskConst,
    float afDistance,
    float afRange
) {
    if (fHighestDOFStrengthValue < afStrength) {
        fHighestDOFStrengthValue = afStrength;
        bCurrentDOFUseMask = false;
        // Modes above 2 are the masked variants of modes 0 to 2.
        if (auiMode > 2) {
            bCurrentDOFUseMask = true;
            auiMode -= 3;
        }
        fCurrentDOFMode = static_cast<ImageSpaceEffectDepthOfField::DepthOfFieldMode>(auiMode);
        fCurrentDOFDistanceValue = afDistance;
        fCurrentDOFRangeValue = afRange;
        fCurrentDOFMaskConst = afMaskConst;
    }
}

void ImageSpaceManager::ApplyDepthOfFieldParamModifier_0(
    float afStrength,
    uint32_t auiMode,
    float afMotionBlur,
    const NiPoint3* apBound
) {
    float x = apBound->x - BSShaderManager::spCamera->m_kWorld.m_Translate.x;
    float y = apBound->y - BSShaderManager::spCamera->m_kWorld.m_Translate.y;
    float z = apBound->z - BSShaderManager::spCamera->m_kWorld.m_Translate.z;
    float apBounda = y * y + x * x + z * z;
    float apBoundb = std::sqrt((double)apBounda);
    ApplyDepthOfFieldParamModifier(afStrength, auiMode, afMotionBlur, apBoundb, apBoundb);
}

void ImageSpaceManager::ApplyDepthRadialBlurParamModifier(
    float afStrength,
    uint32_t auiMode,
    float afRampup,
    float afStart,
    const NiBound& arBound
) {
    NiPoint2 center;
    float y = BSShaderManager::spCamera->m_kWorld.m_Translate.y;
    const NiBound* v7 = &arBound;
    float z = BSShaderManager::spCamera->m_kWorld.m_Translate.z;
    float x = BSShaderManager::spCamera->m_kWorld.m_Translate.x;
    center.x = arBound.m_kCenter.x;
    center.y = arBound.m_kCenter.y;
    float v16 = center.x - x;
    float v12 = arBound.m_kCenter.z;
    float v17 = center.y - y;
    float v18 = v12 - z;
    float v19 = v17 * v17 + v16 * v16 + v18 * v18;
    v19 = std::sqrt((double)v19);
    float frange = v19;
    ImageSpaceEffectRadialBlur::GetBoundParams(*v7, center, v19);
    v19 = v19 + afStart;
    ApplyDepthRadialBlurParamModifier(afStrength, auiMode, frange, frange, afRampup, v19, center);
}

void ImageSpaceManager::ApplyDepthRadialBlurParamModifier(
    float afStrength,
    uint32_t auiMode,
    float afDistance,
    float afRange,
    float afRampup,
    float afStart,
    const NiPoint2& arCenter
) {
    float y;
    if (fHighestDepthRadialBlurStrengthValue < ((double)afStrength)) {
        fHighestDepthRadialBlurStrengthValue = afStrength;
        fCurrentDepthRadialBlurRampupValue = afRampup;
        fCurrentDepthRadialBlurStartValue = afStart;
        fCurrentDepthRadialBlurCenter.x = arCenter.x;
        y = arCenter.y;
        fCurrentDepthRadialBlurDistance = afDistance;
        fCurrentDepthRadialBlurRange = afRange;
        fCurrentDepthRadialBlurCenter.y = y;
        fCurrentDepthRadialBlurMode = static_cast<ImageSpaceEffectDepthRadialBlur::RadialBlurMode>(auiMode);
    }
}

bool ImageSpaceManager::IsEffectActive(ImageSpaceManager::EffectID aiEffect) {
    ImageSpaceEffect* pEffect = pEffectList.m_pBase[aiEffect];
    return pEffect && pEffect->IsActive();
}

// The shaders are stored as their ImageSpaceEffect subobject; each one present reloads its programs.
void ImageSpaceManager::ReloadEffectShaders() {
    for (uint32_t i = EFFECT_SHADER_START; i < EFFECT_COUNT; ++i) {
        if (pEffectList.m_pBase[i]) {
            static_cast<ImageSpaceShader*>(pEffectList.m_pBase[i])->ReloadShaders();
        }
    }
}

void ImageSpaceManager::PrepareEffectRange() {
    iLastEffect = static_cast<EffectID>(-1);
    iActiveEffectsCount = 0;
    if (bEOFEnabled) {
        for (int32_t i = 0; i < EFFECT_ENDOFFRAME_END; ++i) {
            ImageSpaceEffect* pEffect = pEffectList.m_pBase[i];
            if (pEffect && pEffect->IsActive()) {
                ++iActiveEffectsCount;
                iLastEffect = static_cast<EffectID>(i);
            }
        }
    }
}

void ImageSpaceManager::SetupEffects() {
    for (int32_t i = 0; i < EFFECT_TOP_LEVEL_END; ++i) {
        ImageSpaceEffect* pEffect = pEffectList.m_pBase[i];
        if (pEffect) {
            pEffect->Setup(this, nullptr);
        }
    }
}

void ImageSpaceManager::ResetToBaseParameterData() {
    const ImageSpaceParameterData* pSource = pOverRideBaseParameterData;
    if (!pSource) {
        pSource = pBaseParameterData;
        if (!pSource) {
            pSource = &DefaultParameterData;
        }
    }
    std::memcpy(&CurrentParameterData, pSource, sizeof(CurrentParameterData));
    ImageSpaceEffectGetHit::fAmplitude = 0.0f;
    ImageSpaceEffectFullScreenBlur_fRadius = 0.0f;
}

void ImageSpaceManager::SetUseSunbeams(bool abUseSunbeams) {
    bUseSunbeams = abUseSunbeams;
}

// The data pointer is released before the heap context is restored, as the game orders them.
void ImageSpaceManager::CreatePartialScreenGeometry(
    float afWidthRatio,
    float afHeightRatio,
    NiPointer<NiScreenElements>& aspScreenElement
) {
    AutoMemContext kMemContext(MEM_CONTEXT_IMAGE_SPACE);
    NiScreenElementsData* pData = new NiScreenElementsData(false, false, 1, 1, 1, 4, 1, 2, 1);
    NiPointer<NiScreenElementsData> spData(pData);
    NiScreenElements* pScreen = new NiScreenElements(pData);
    spPartialScreenTriShape = pScreen;

    float fWidth = afWidthRatio + afWidthRatio;
    // Multiplies by the double -2.0.
    float fHeight = static_cast<float>(afHeightRatio * -2.0);
    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)
        ->Insert(4, 0, nullptr);
    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)
        ->SetRectangle(0, -1.0f, 1.0f, fWidth, fHeight);
    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)->UpdateBound();
    static_cast<NiScreenElementsData*>(spPartialScreenTriShape.m_pObject->m_spModelData.m_pObject)
        ->SetTextures(0, 0, 0.0f, 0.0f, afWidthRatio, afHeightRatio);
    spPartialScreenTriShape.m_pObject->UpdateProperties();

    NiUpdateData kUpdateData;
    kUpdateData.fTime = 0.0f;
    kUpdateData.bUpdateControllers = false;
    kUpdateData.bParallelUpdate = false;
    kUpdateData.bFoundParticles = false;
    kUpdateData.bFoundMorphController = false;
    kUpdateData.bSceneGraphChange = false;
    spPartialScreenTriShape.m_pObject->Update(kUpdateData);

    NiGlobalStringTable::GlobalStringHandle kName =
        NiGlobalStringTable::AddString("Interface Effect Partial Screen Trishape");
    spPartialScreenTriShape.m_pObject->SetName(reinterpret_cast<const NiFixedString&>(kName));
    if (kName != NiGlobalStringTable::NULL_STRING) {
        InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kName) - 8));
    }
    aspScreenElement = spPartialScreenTriShape;
}

void ImageSpaceManager::SetCustomViewPort(float afLeft, float afRight, float afTop, float afBottom) {
    CustomViewPort.m_left = afLeft;
    CustomViewPort.m_right = afRight;
    CustomViewPort.m_top = afTop;
    CustomViewPort.m_bottom = afBottom;
}

void ImageSpaceManager::ResetViewport() {
    CustomViewPort.m_left = (float)0.0;
    CustomViewPort.m_right = (float)1.0;
    CustomViewPort.m_top = (float)1.0;
    CustomViewPort.m_bottom = (float)0.0;
}

float ImageSpaceManager::GetEmissiveHDRMult() {
    ImageSpaceEffectHDR* effect = static_cast<ImageSpaceEffectHDR*>(GetEffect(EFFECT_HDR));
    return effect->GetEmissiveHDRMult();
}

float ImageSpaceManager::GetNumBlurPasses() {
    ImageSpaceEffectHDR* effect = static_cast<ImageSpaceEffectHDR*>(GetEffect(EFFECT_HDR));
    return effect->GetNumBlurPasses();
}

float ImageSpaceManager::GetHDRBrightClamp() {
    ImageSpaceEffectHDR* effect = static_cast<ImageSpaceEffectHDR*>(GetEffect(EFFECT_HDR));
    return effect->GetBrightClamp();
}

float ImageSpaceManager::GetHDRBrightScale() {
    ImageSpaceEffectHDR* effect = static_cast<ImageSpaceEffectHDR*>(GetEffect(EFFECT_HDR));
    return effect->GetBrightScale();
}

float ImageSpaceManager::GetHDREyeAdaptSpeed() {
    ImageSpaceEffectHDR* effect = static_cast<ImageSpaceEffectHDR*>(GetEffect(EFFECT_HDR));
    return effect->GetEyeAdaptSpeed();
}

float ImageSpaceManager::GetHDRTargetLUM() {
    ImageSpaceEffectHDR* effect = static_cast<ImageSpaceEffectHDR*>(GetEffect(EFFECT_HDR));
    return effect->GetTargetLUM();
}

float ImageSpaceManager::GetHDRUpperLUMClamp() {
    ImageSpaceEffectHDR* effect = static_cast<ImageSpaceEffectHDR*>(GetEffect(EFFECT_HDR));
    return effect->GetUpperLUMClamp();
}

void ImageSpaceManager::SetSunPosition(const NiPoint2& aSunPos) {
    ImageSpaceEffectSunbeams::kSunPos = aSunPos;
}

void ImageSpaceManager::SetMotionBlurConst(float afConst) {
    ImageSpaceEffectMotionBlur* effect = static_cast<ImageSpaceEffectMotionBlur*>(GetEffect(EFFECT_MOTION_BLUR));
    effect->SetFullScreenBlurConst(afConst);
}

void ImageSpaceManager::ShutdownGeometry() {
    NiScreenElements* m_pObject;
    NiTriShape* spMenuUnpackGeom;
    m_pObject = spScreenTriShape.m_pObject;
    if (m_pObject) {
        m_pObject->DecRefCount();
        spScreenTriShape.m_pObject = nullptr;
    }
    spPartialScreenTriShape = nullptr;
    spScreenTriShapeData = nullptr;
    spMenuUnpackGeom = this->spMenuUnpackGeom;
    if (spMenuUnpackGeom) {
        spMenuUnpackGeom->DecRefCount();
        this->spMenuUnpackGeom = nullptr;
    }
}

void ImageSpaceManager::GetDepthOfFieldParamsFromPoint(const NiPoint3* a1, NiPoint2* a2) {
    float v3 = a1->x - BSShaderManager::spCamera->m_kWorld.m_Translate.x;
    float v4 = a1->y - BSShaderManager::spCamera->m_kWorld.m_Translate.y;
    float v5 = a1->z - BSShaderManager::spCamera->m_kWorld.m_Translate.z;
    float a1a = v4 * v4 + v3 * v3 + v5 * v5;
    float a1b = std::sqrt((double)a1a);
    a2->x = a1b;
    a2->y = a1b;
}

void ImageSpaceManager::ShutdownEffects() {
    int32_t v5;
    for (int32_t i = 0; i < 116; ++i) {
        ImageSpaceEffect* peffect = pEffectList.m_pBase[i];
        if (peffect) {
            peffect->Shutdown();
            delete peffect;
            ImageSpaceEffect* a3 = nullptr;
            pEffectList.SetAt(i, a3);
        }
    }
    for (int32_t j = 0; static_cast<uint16_t>(j) < pEffectList.m_usSize; pEffectList.m_pBase[v5] = 0) {
        v5 = static_cast<uint16_t>(j++);
    }
    pEffectList.m_usSize = 0;
    pEffectList.m_usESize = 0;
}

void ImageSpaceManager::SetActiveImageSpaceParams(const ImageSpaceParameterData* apData) {
    TLSData* v2 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v2->eMemContext;
    pOverRideBaseParameterData = apData;
    v2->eMemContext = eMemContext;
}

void ImageSpaceManager::EndParamModifiers() {
    if (kTintColor.a > 0.0f) {
        double a = kTintColor.a;
        kTintColor.r = kTintColor.r / a;
        kTintColor.g = kTintColor.g / a;
        kTintColor.b = kTintColor.b / a;
        kTintColor.a = kTintColor.a / a;
    }
    float fHighestTintValue = this->fHighestTintValue;
    kTintColor.a = fHighestTintValue;
    CurrentParameterData.pfCinematicData[4u] = kTintColor.r;
    CurrentParameterData.pfCinematicData[5] = kTintColor.g;
    CurrentParameterData.pfCinematicData[6] = kTintColor.b;
    CurrentParameterData.pfCinematicData[7] = fHighestTintValue;
    if (kFadeColor.a > 0.0f) {
        double v3 = kFadeColor.a;
        kFadeColor.r = kFadeColor.r / v3;
        kFadeColor.g = kFadeColor.g / v3;
        kFadeColor.b = kFadeColor.b / v3;
        kFadeColor.a = kFadeColor.a / v3;
    }
    float fHighestFadeValue = this->fHighestFadeValue;
    kFadeColor.a = fHighestFadeValue;
    CurrentParameterData.pfCinematicData[8] = kFadeColor.r;
    CurrentParameterData.pfCinematicData[9] = kFadeColor.g;
    CurrentParameterData.pfCinematicData[10] = kFadeColor.b;
    CurrentParameterData.pfCinematicData[11] = fHighestFadeValue;
    ImageSpaceEffectGetHit::fAmplitude = fHighestDoubleValue;
    ImageSpaceEffectFullScreenBlur_fRadius = fHighestBlurValue;
    ImageSpaceEffectRadialBlur* pRadialBlur =
        static_cast<ImageSpaceEffectRadialBlur*>(pEffectList.m_pBase[EFFECT_RADIAL_BLUR]);
    float fCurrentRadialBlurRampupValue = this->fCurrentRadialBlurRampupValue;
    float fCurrentRadialBlurStartValue = this->fCurrentRadialBlurStartValue;
    float fCurrentRadialBlurRampDownValue = this->fCurrentRadialBlurRampDownValue;
    float fCurrentRadialBlurDownStartValue = this->fCurrentRadialBlurDownStartValue;
    pRadialBlur->fStrength = fHighestRadialBlurStrengthValue;
    pRadialBlur->fRampup = fCurrentRadialBlurRampupValue;
    pRadialBlur->fStart = fCurrentRadialBlurStartValue;
    pRadialBlur->fRampdown = fCurrentRadialBlurRampDownValue;
    pRadialBlur->fDownstart = fCurrentRadialBlurDownStartValue;
    pRadialBlur->Center = fCurrentRadialBlurCenter;
    float fHighestDOFStrengthValue = this->fHighestDOFStrengthValue;
    ImageSpaceEffectDepthOfField* pdofeffect =
        static_cast<ImageSpaceEffectDepthOfField*>(pEffectList.m_pBase[EFFECT_DEPTH_OF_FIELD]);
    pdofeffect->fStrength = fHighestDOFStrengthValue;
    pdofeffect->bParamsChanged = true;
    double fCurrentDOFDistanceValue = this->fCurrentDOFDistanceValue;
    pdofeffect->bParamsChanged = true;
    float v24 = fCurrentDOFDistanceValue;
    pdofeffect->fDepth = v24;
    double fCurrentDOFRangeValue = this->fCurrentDOFRangeValue;
    pdofeffect->bParamsChanged = true;
    pdofeffect->fRange = fCurrentDOFRangeValue;
    pdofeffect->fMaskConst = fCurrentDOFMaskConst;
    ImageSpaceEffectDepthOfField::DepthOfFieldMode fCurrentDOFMode = this->fCurrentDOFMode;
    if (fCurrentDOFMode != pdofeffect->Mode) {
        pdofeffect->bParamsChanged = true;
        pdofeffect->Mode = fCurrentDOFMode;
    }
    if (bCurrentDOFUseMask) {
        pdofeffect->DoStandardMask();
    }
    else {
        pdofeffect->DoStandard();
    }
    ImageSpaceEffectDepthRadialBlur* pdrbeffect =
        static_cast<ImageSpaceEffectDepthRadialBlur*>(pEffectList.m_pBase[EFFECT_DEPTH_RADIAL_BLUR]);
    double fHighestDepthRadialBlurStrengthValue = this->fHighestDepthRadialBlurStrengthValue;
    pdrbeffect->bParamsChanged = true;
    pdrbeffect->Params2.r = fHighestDepthRadialBlurStrengthValue;
    float fCurrentDepthRadialBlurDistance = this->fCurrentDepthRadialBlurDistance;
    float fRange = pdrbeffect->fRange;
    double v11 = fCurrentDepthRadialBlurDistance;
    if (fCurrentDepthRadialBlurDistance > static_cast<double>(fRange)) {
        fRange = fCurrentDepthRadialBlurDistance;
    }
    pdrbeffect->Params.b = fCurrentDepthRadialBlurDistance;
    pdrbeffect->bParamsChanged = true;
    pdrbeffect->Params.r = v11 - fRange;
    pdrbeffect->Params.g = fRange + v11;
    double fCurrentDepthRadialBlurRange = this->fCurrentDepthRadialBlurRange;
    pdrbeffect->bParamsChanged = true;
    float v26 = fCurrentDepthRadialBlurRange;
    pdrbeffect->fRange = v26;
    pdrbeffect->Params.r = v11 - v26;
    pdrbeffect->Params.g = v11 + v26;
    pdrbeffect->SetMode(fCurrentDepthRadialBlurMode);
    float fCurrentDepthRadialBlurRampupValue = this->fCurrentDepthRadialBlurRampupValue;
    float fCurrentDepthRadialBlurStartValue = this->fCurrentDepthRadialBlurStartValue;
    pdrbeffect->RadialParams.r = this->fHighestDepthRadialBlurStrengthValue;
    pdrbeffect->RadialParams.g = fCurrentDepthRadialBlurRampupValue;
    pdrbeffect->RadialParams.b = fCurrentDepthRadialBlurStartValue;
    ImageSpaceEffectMotionBlur* pMotionBlur =
        static_cast<ImageSpaceEffectMotionBlur*>(pEffectList.m_pBase[EFFECT_MOTION_BLUR]);
    double fHighestMotionBlurStrengthValue = this->fHighestMotionBlurStrengthValue;
    pMotionBlur->bParamsChanged = true;
    pMotionBlur->Params.r = fHighestMotionBlurStrengthValue;
}

void ImageSpaceManager::SetEffectAt(
    NiTPrimitiveArray<ImageSpaceEffect*>* apArray,
    uint32_t auiIndex,
    ImageSpaceEffect* const* appEffect
) {
    apArray->SetAt(auiIndex, *appEffect);
}

// The start bias filled in by GetBoundParams is not used after the call, so it is a local.
void ImageSpaceManager::GetRadialBlurCenterFromWorldBound(const NiBound* apBound, NiPoint2* apCenter) {
    float fStartBias;
    ImageSpaceEffectRadialBlur::GetBoundParams(*apBound, *apCenter, fStartBias);
}

void ImageSpaceEffectDepthOfField::DoStandard() {
    NiTPrimitiveArray<bool>* pEffects = &pEffectOn;
    bool bOn = true;
    pEffects->SetAt(EFFECT_DOWNSAMPLE, bOn);
    bOn = true;
    pEffects->SetAt(EFFECT_BLUR, bOn);
    bOn = true;
    pEffects->SetAt(EFFECT_DEPTH_OF_FIELD, bOn);
    bOn = false;
    pEffects->SetAt(EFFECT_DEPTH_OF_FIELD_MASK_BLUR, bOn);
}

void ImageSpaceEffectDepthOfField::DoStandardMask() {
    NiTPrimitiveArray<bool>* pEffects = &pEffectOn;
    bool bOn = true;
    pEffects->SetAt(EFFECT_DOWNSAMPLE, bOn);
    bOn = true;
    pEffects->SetAt(EFFECT_BLUR, bOn);
    bOn = false;
    pEffects->SetAt(EFFECT_DEPTH_OF_FIELD, bOn);
    bOn = true;
    pEffects->SetAt(EFFECT_DEPTH_OF_FIELD_MASK_BLUR, bOn);
}

void ImageSpaceEffectDepthRadialBlur::SetMode(RadialBlurMode aMode) {
    if (aMode != Mode) {
        bParamsChanged = true;
        Mode = aMode;
        float fFront = (aMode == DOF_FRONT || aMode == DOF_FRONT_BACK) ? 1.0f : 0.0f;
        Params2.g = fFront;
        float fBack = (aMode == DOF_BACK || aMode == DOF_FRONT_BACK) ? 1.0f : 0.0f;
        Params2.b = fBack;
    }
}

void ImageSpaceShaderAlphaMult::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\1x\\p\\copy.p.hlsl";
    pshader.kMacros[0].Name = "ALPHAMULT";
    pshader.kMacros[0].Definition = "";
    std::memset(&pshader.kMacros[1], 0, sizeof(pshader.kMacros) - sizeof(pshader.kMacros[0]));
    LoadShaders(vshader, pshader, "ALPHAMULT", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlend::ReloadShaders() {
    BSShaderMacro vshader;
    BSShaderMacro pshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "TEX2";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(vshader.kMacros) - sizeof(vshader.kMacros[0]));
    pshader.strPath = "imagespace\\2x\\p\\blend.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "BLEND", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\2x\\p\\blur.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "BLUR", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur11::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
    v2.kMacros[0].Name = "TEXTAP";
    v2.kMacros[0].Definition = "11";
    std::memset(&v2.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    LoadShaders(v3, v2, "BLUR11", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur13::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
    pshader.kMacros[0].Name = "TEXTAP";
    pshader.kMacros[0].Definition = "13";
    std::memset(&pshader.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    LoadShaders(vshader, pshader, "BLUR13", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur15::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
    v2.kMacros[0].Name = "TEXTAP";
    v2.kMacros[0].Definition = "15";
    std::memset(&v2.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    LoadShaders(v3, v2, "BLUR15", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur3::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
    pshader.kMacros[0].Name = "TEXTAP";
    pshader.kMacros[0].Definition = "3";
    std::memset(&pshader.kMacros[1], 0, sizeof(pshader.kMacros) - sizeof(pshader.kMacros[0]));
    LoadShaders(vshader, pshader, "BLUR3", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur5::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
    v2.kMacros[0].Name = "TEXTAP";
    v2.kMacros[0].Definition = "5";
    std::memset(&v2.kMacros[1], 0, sizeof(v2.kMacros) - sizeof(v2.kMacros[0]));
    LoadShaders(v3, v2, "BLUR5", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur7::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
    v2.kMacros[0].Name = "TEXTAP";
    v2.kMacros[0].Definition = "7";
    std::memset(&v2.kMacros[1], 0, sizeof(v2.kMacros) - sizeof(v2.kMacros[0]));
    LoadShaders(v3, v2, "BLUR7", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBlur9::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
    pshader.kMacros[0].Name = "TEXTAP";
    pshader.kMacros[0].Definition = "9";
    std::memset(&pshader.kMacros[1], 0, sizeof(pshader.kMacros) - sizeof(pshader.kMacros[0]));
    LoadShaders(vshader, pshader, "BLUR9", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBrightPassBlur3::ReloadShaders() {
    alignas(8) BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = nullptr;
    std::memset(
        &vshader.kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    alignas(8) BSShaderMacro kMacros;
    kMacros.kMacros[0].Name = "BRIGHTPASS";
    kMacros.kMacros[0].Definition = "";
    kMacros.kMacros[1].Name = "TEXTAP";
    kMacros.kMacros[1].Definition = "3";
    kMacros.kMacros[2].Name = nullptr;
    const char* pcName;
    if (bDoTestHDR.uValue.b) {
        kMacros.strPath = "imagespace\\2x\\p\\blur2_TEST.p.hlsl";
        std::memset(
            &kMacros.kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR3T";
    }
    else {
        kMacros.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
        std::memset(
            &kMacros.kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR3";
    }
    alignas(8) BSShaderMacro pshader = kMacros;
    LoadShaders(vshader, pshader, pcName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBrightPassBlur5::ReloadShaders() {
    alignas(8) BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = nullptr;
    std::memset(
        &vshader.kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    alignas(8) BSShaderMacro kMacros;
    kMacros.kMacros[0].Name = "BRIGHTPASS";
    kMacros.kMacros[0].Definition = "";
    kMacros.kMacros[1].Name = "TEXTAP";
    kMacros.kMacros[1].Definition = "5";
    kMacros.kMacros[2].Name = nullptr;
    const char* pcName;
    if (bDoTestHDR.uValue.b) {
        kMacros.strPath = "imagespace\\2x\\p\\blur2_TEST.p.hlsl";
        std::memset(
            &kMacros.kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR5T";
    }
    else {
        kMacros.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
        std::memset(
            &kMacros.kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR5";
    }
    alignas(8) BSShaderMacro pshader = kMacros;
    LoadShaders(vshader, pshader, pcName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderBrightPassBlur7::ReloadShaders() {
    alignas(8) BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = nullptr;
    std::memset(
        &vshader.kMacros[0].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[0].Definition)
    );
    alignas(8) BSShaderMacro kMacros;
    kMacros.kMacros[0].Name = "BRIGHTPASS";
    kMacros.kMacros[0].Definition = "";
    kMacros.kMacros[1].Name = "TEXTAP";
    kMacros.kMacros[1].Definition = "7";
    kMacros.kMacros[2].Name = nullptr;
    const char* pcName;
    if (bDoTestHDR.uValue.b) {
        kMacros.strPath = "imagespace\\2x\\p\\blur2_TEST.p.hlsl";
        std::memset(
            &kMacros.kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR7T";
    }
    else {
        kMacros.strPath = "imagespace\\2x\\p\\blur2.p.hlsl";
        std::memset(
            &kMacros.kMacros[2].Definition, 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[2].Definition)
        );
        pcName = "BPBLUR7";
    }
    alignas(8) BSShaderMacro pshader = kMacros;
    LoadShaders(vshader, pshader, pcName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderCinematic::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.kMacros[0].Definition = "";
    pshader.kMacros[1].Definition = "";
    pshader.kMacros[2].Definition = "";
    pshader.kMacros[3].Definition = "";
    pshader.strPath = "imagespace\\2x\\p\\cinematic.p.hlsl";
    pshader.kMacros[0].Name = "TINT";
    pshader.kMacros[1].Name = "SATURATION";
    pshader.kMacros[2].Name = "BRIGHT";
    pshader.kMacros[3].Name = "CONTRAST";
    std::memset(&pshader.kMacros[4], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[4]));
    this->LoadShaders(vshader, pshader, "Cinematic", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

ImageSpaceShaderCopy::ImageSpaceShaderCopy() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderCopy::~ImageSpaceShaderCopy() {}

void ImageSpaceShaderCopy::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "imagespace\\1x\\p\\copy.p.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    const char* pcptarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "COPY", ShaderTarget_vs_1_1, pcptarget);
    LoadStagesAndPasses();
}

ImageSpaceShaderCopyRenderTarget1::ImageSpaceShaderCopyRenderTarget1() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderCopyRenderTarget1::~ImageSpaceShaderCopyRenderTarget1() {}

void ImageSpaceShaderCopyRenderTarget1::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\1x\\p\\copy.p.hlsl";
    v2.kMacros[0].Name = "TWO_TARGETS";
    v2.kMacros[0].Definition = "";
    std::memset(&v2.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    LoadShaders(v3, v2, "COPYRT1", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderCopyRenderTarget1::SetState() {
    NiD3DPass* m_pObject;
    m_pObject = spPass.m_pObject;
    if (!m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(168, 0, true);
    NiD3DPass* v3 = spPass.m_pObject;
    if (!v3->m_pkRenderStateGroup) {
        v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v3->m_pkRenderStateGroup->SetRenderState(190, 0xFu, true);
}

void ImageSpaceShaderDepthOfField::ReloadShaders() {
    BSShaderMacro vshader;
    BSShaderMacro pshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "TEX2";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    pshader.strPath = "imagespace\\2x\\p\\DepthOfField.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "DOF", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderDepthOfFieldMask::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "TEX2";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(vshader.kMacros) - sizeof(vshader.kMacros[0]));
    pshader.strPath = "imagespace\\2x\\p\\DepthOfField.p.hlsl";
    pshader.kMacros[0].Name = "MASK";
    pshader.kMacros[0].Definition = "";
    std::memset(&pshader.kMacros[1], 0, sizeof(pshader.kMacros) - sizeof(pshader.kMacros[0]));
    LoadShaders(vshader, pshader, "DOFMASK", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderDoubleVision::ReloadShaders() {
    BSShaderMacro vshader;
    BSShaderMacro pshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "TEX2";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(vshader.kMacros) - sizeof(vshader.kMacros[0]));
    pshader.strPath = "imagespace\\2x\\p\\doublevision.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "DOUBLEVIS", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

ImageSpaceShaderFFTDisplayWaterHeight::ImageSpaceShaderFFTDisplayWaterHeight() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderFFTDisplayWaterHeight::~ImageSpaceShaderFFTDisplayWaterHeight() {}

void ImageSpaceShaderFFTDisplayWaterHeight::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "water\\2_ab\\p\\waterhmap.p.hlsl";
    v3.kMacros[0].Name = "DISPLAY_WATER_HEIGHT";
    std::memset(&v3.kMacros[0].Definition, 0, sizeof(v3) - offsetof(BSShaderMacro, kMacros[0].Definition));
    const char* PSTarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "FFTDISPLAYWATERHEIGHT", ShaderTarget_vs_1_1, PSTarget);
    LoadStagesAndPasses();
}

void ImageSpaceShaderHDRBlur::ReloadShaders() {
    const char* pcName;
    // These locals are 8-byte aligned.
    alignas(8) BSShaderMacro kVShader;
    kVShader.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
    std::memset(kVShader.kMacros, 0, sizeof(kVShader.kMacros));
    alignas(8) BSShaderMacro kPShaderTemp;
    kPShaderTemp.kMacros[0].Name = "TEXTAP";
    kPShaderTemp.kMacros[0].Definition = "15";
    kPShaderTemp.kMacros[1].Name = nullptr;
    if (bDoTestHDR.uValue.b) {
        kPShaderTemp.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
        std::memset(
            &kPShaderTemp.kMacros[1].Definition, 0,
            sizeof(kPShaderTemp) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        pcName = "HDRBLURT";
    }
    else {
        kPShaderTemp.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
        std::memset(
            &kPShaderTemp.kMacros[1].Definition, 0,
            sizeof(kPShaderTemp) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        pcName = "HDRBLUR";
    }
    // The pixel shader macros are built in one local and copied to the one that is passed.
    alignas(8) BSShaderMacro kPShader = kPShaderTemp;
    LoadShaders(kVShader, kPShader, pcName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

ImageSpaceShaderLightAdapt::ImageSpaceShaderLightAdapt() {
    pTextures.SetSize(3u);
}

ImageSpaceShaderLightAdapt::~ImageSpaceShaderLightAdapt() {}

void ImageSpaceShaderLightAdapt::ReloadShaders() {
    const char* pcName;
    // These locals are 8-byte aligned.
    alignas(8) BSShaderMacro kVShader;
    kVShader.strPath = "imagespace\\2x\\v\\HDRbase.v.hlsl";
    kVShader.kMacros[0].Name = nullptr;
    std::memset(&kVShader.kMacros[0].Definition, 0, sizeof(kVShader) - offsetof(BSShaderMacro, kMacros[0].Definition));
    alignas(8) BSShaderMacro kPShaderTemp;
    kPShaderTemp.kMacros[0].Name = "ADAPT";
    kPShaderTemp.kMacros[0].Definition = "";
    kPShaderTemp.kMacros[1].Name = nullptr;
    if (bDoTestHDR.uValue.b) {
        kPShaderTemp.strPath = "imagespace\\2x\\p\\HDRBlur_TEST.p.hlsl";
        std::memset(
            &kPShaderTemp.kMacros[1].Definition, 0,
            sizeof(kPShaderTemp) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        pcName = "HDRADAPTT";
    }
    else {
        kPShaderTemp.strPath = "imagespace\\2x\\p\\HDRBlur.p.hlsl";
        std::memset(
            &kPShaderTemp.kMacros[1].Definition, 0,
            sizeof(kPShaderTemp) - offsetof(BSShaderMacro, kMacros[1].Definition)
        );
        pcName = "HDRADAPT";
    }
    // The pixel shader macros are built in one local and copied to the one that is passed.
    alignas(8) BSShaderMacro kPShader = kPShaderTemp;
    LoadShaders(kVShader, kPShader, pcName, ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderMap::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\map_P20.p.hlsl";
    std::memset(v2.kMacros, 0, sizeof(v2.kMacros));
    LoadShaders(v3, v2, reinterpret_cast<char*>(&rdata_10B4F3C), ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderMenuBG::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\menuBG.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\1x\\p\\menuBG.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "MENUBG", ShaderTarget_vs_2_0, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderMenuUnpack::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "imagespace\\1x\\p\\copy.p.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    const char* pcptarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "COPYUNPACK", ShaderTarget_vs_1_1, pcptarget);
    LoadStagesAndPasses();
}

void ImageSpaceShaderMotionBlur::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    v3.kMacros[0].Name = "TEX2";
    v3.kMacros[0].Definition = "";
    std::memset(&v3.kMacros[1], 0, sizeof(v3.kMacros) - sizeof(v3.kMacros[0]));
    v2.strPath = "imagespace\\2x\\p\\MotionBlur.p.hlsl";
    v2.kMacros[0].Name = "NUM_SAMPLES";
    v2.kMacros[0].Definition = "40.0f";
    v2.kMacros[1].Name = "NO_VELOCITY";
    v2.kMacros[1].Definition = "";
    std::memset(&v2.kMacros[2], 0, sizeof(v2.kMacros) - 2 * sizeof(v2.kMacros[0]));
    LoadShaders(v3, v2, "MOTBLUR", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderNULL::ReloadShaders() {
    BSShaderMacro vshader;
    BSShaderMacro v3;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "NOTEX";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    std::memset(&v3, 0, sizeof(v3));
    LoadShaders(vshader, v3, "NULL", ShaderTarget_vs_1_1, nullptr);
    LoadStagesAndPasses();
}

ImageSpaceShaderNoiseNormalmap::ImageSpaceShaderNoiseNormalmap() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderNoiseNormalmap::~ImageSpaceShaderNoiseNormalmap() {}

void ImageSpaceShaderNoiseNormalmap::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "imagespace\\2x\\p\\noise_P20.p.hlsl";
    v3.kMacros[0].Name = "NORMALMAP";
    std::memset(&v3.kMacros[0].Definition, 0, sizeof(v3) - offsetof(BSShaderMacro, kMacros[0].Definition));
    const char* pcptarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "NOISENORMALMAP", ShaderTarget_vs_1_1, pcptarget);
    LoadStagesAndPasses();
}

bool ImageSpaceShaderNoiseNormalmap::LoadStagesAndPasses() {
    NiPointer<NiD3DTextureStage> a1;
    NiPointer<NiD3DPass>* p_spPass = &spPass;
    NiD3DPass::CreateNewPass(spPass);
    for (uint32_t i = 0; i < pTextures.m_usMaxSize; ++i) {
        NiD3DTextureStage::CreateNewStage(a1);
        BSShader::SetupDefaultStage(a1.m_pObject, i, 1u, 2u, true);
        p_spPass->m_pObject->SetStage(p_spPass->m_pObject->m_uiCurrentStage, a1.m_pObject);
    }
    NiD3DPass* m_pObject = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(7u, 0, true);
    NiD3DPass* v5 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v5->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v5->m_pkRenderStateGroup->SetRenderState(14u, 0, true);
    NiD3DPass* v6 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v6->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v6->m_pkRenderStateGroup->SetRenderState(27u, 0, true);
    NiD3DPass* v7 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v7->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v7->m_pkRenderStateGroup->SetRenderState(15u, 0, true);
    NiD3DPass* v8 = p_spPass->m_pObject;
    if (!p_spPass->m_pObject->m_pkRenderStateGroup) {
        v8->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v8->m_pkRenderStateGroup->SetRenderState(168u, 15u, true);
    SetState();
    p_spPass->m_pObject->m_spVertexShader = spVertexShader.m_pObject;
    p_spPass->m_pObject->m_spPixelShader = spPixelShader.m_pObject;
    NiD3DShader::m_spCurrentPass = p_spPass->m_pObject;
    return true;
}

void ImageSpaceShaderNoiseScrollAndBlend::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "imagespace\\2x\\p\\noise_P20.p.hlsl";
    v3.kMacros[0].Name = "SCROLL_AND_BLEND";
    std::memset(&v3.kMacros[0].Definition, 0, sizeof(v3) - offsetof(BSShaderMacro, kMacros[0].Definition));
    const char* pcptarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "NOISESCROLLANDBLEND", ShaderTarget_vs_1_1, pcptarget);
    LoadStagesAndPasses();
}

void ImageSpaceShaderRadialBlur::ReloadShaders() {
    BSShaderMacro vshader;
    BSShaderMacro pshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "TEX2";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    pshader.strPath = "imagespace\\2x\\p\\RadialBlur.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "RBLUR", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderRadialBlurHigh::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "TEX2";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(vshader.kMacros) - sizeof(vshader.kMacros[0]));
    pshader.strPath = "imagespace\\2x\\p\\RadialBlur.p.hlsl";
    pshader.kMacros[0].Name = "NUM_STEPS";
    pshader.kMacros[0].Definition = "10.0f";
    std::memset(&pshader.kMacros[1], 0, sizeof(pshader.kMacros) - sizeof(pshader.kMacros[0]));
    LoadShaders(vshader, pshader, "RBLURH", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderRadialBlurMedium::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    vshader.kMacros[0].Name = "TEX2";
    vshader.kMacros[0].Definition = "";
    std::memset(&vshader.kMacros[1], 0, sizeof(vshader.kMacros) - sizeof(vshader.kMacros[0]));
    pshader.strPath = "imagespace\\2x\\p\\RadialBlur.p.hlsl";
    pshader.kMacros[0].Name = "NUM_STEPS";
    pshader.kMacros[0].Definition = "6.0f";
    std::memset(&pshader.kMacros[1], 0, sizeof(pshader.kMacros) - sizeof(pshader.kMacros[0]));
    LoadShaders(vshader, pshader, "RBLURM", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderRefraction::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v2.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    v2.kMacros[0].Name = "TEX2";
    v2.kMacros[0].Definition = "";
    std::memset(&v2.kMacros[1], 0, sizeof(v2.kMacros) - sizeof(v2.kMacros[0]));
    v3.strPath = "imagespace\\2x\\p\\refraction_P20.p.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    LoadShaders(v2, v3, "REFRACT", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

ImageSpaceShaderStencilCopy::ImageSpaceShaderStencilCopy() {
    pTextures.SetSize(2u);
}

void ImageSpaceShaderStencilCopy::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "imagespace\\1x\\p\\copy.p.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    const char* pcptarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "STENCILCOPY", ShaderTarget_vs_1_1, pcptarget);
    LoadStagesAndPasses();
}

// Stencil test on, D3DCMP_EQUAL against uiStencilRef. uiStencilRef is read before the third group check.
void ImageSpaceShaderStencilCopy::SetState() {
    NiD3DPass* pPass = spPass.m_pObject;
    if (!pPass->m_pkRenderStateGroup) {
        pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILENABLE, 1u, true);
    pPass = spPass.m_pObject;
    if (!pPass->m_pkRenderStateGroup) {
        pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILFUNC, 4u, true);
    pPass = spPass.m_pObject;
    uint32_t uiRef = uiStencilRef;
    if (!pPass->m_pkRenderStateGroup) {
        pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILREF, uiRef, true);
}

ImageSpaceShaderStencilCopyNoTextureNoDepth::ImageSpaceShaderStencilCopyNoTextureNoDepth() {
    pTextures.SetSize(1u);
}

void ImageSpaceShaderStencilCopyNoTextureNoDepth::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v4.kMacros, 0, sizeof(v4.kMacros));
    v3.strPath = "imagespace\\1x\\p\\copy.p.hlsl";
    v3.kMacros[0].Name = "NOTEX";
    v3.kMacros[0].Definition = "";
    std::memset(&v3.kMacros[1], 0, sizeof(BSShaderMacro) - offsetof(BSShaderMacro, kMacros[1]));
    const char* pcptarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "STENCILCOPYNOTEXNODEPTH", ShaderTarget_vs_1_1, pcptarget);
    LoadStagesAndPasses();
}

void ImageSpaceShaderStencilCopyNoTextureNoDepth::SetState() {
    NiD3DPass* m_pObject;
    m_pObject = spPass.m_pObject;
    if (!m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(0xEu, 0, true);
    NiD3DPass* v3 = spPass.m_pObject;
    if (!v3->m_pkRenderStateGroup) {
        v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v3->m_pkRenderStateGroup->SetRenderState(0x34u, 1u, true);
    NiD3DPass* v4 = spPass.m_pObject;
    if (!v4->m_pkRenderStateGroup) {
        v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v4->m_pkRenderStateGroup->SetRenderState(0x38u, 3u, true);
}

ImageSpaceShaderSunbeams::ImageSpaceShaderSunbeams() {
    pTextures.SetSize(2u);
}

ImageSpaceShaderSunbeams::~ImageSpaceShaderSunbeams() {}

void ImageSpaceShaderSunbeams::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\sunbeams.p.hlsl";
    std::memset(v2.kMacros, 0, sizeof(v2.kMacros));
    LoadShaders(v3, v2, "Sunbeams", ShaderTarget_vs_1_1, "ps_2_a");
    LoadStagesAndPasses();
}

void ImageSpaceShaderSunbeams::SetState() {
    NiD3DPass* pPass = spPass.m_pObject;
    if (!pPass->m_pkRenderStateGroup) {
        pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILENABLE, 1u, true);
    pPass = spPass.m_pObject;
    if (!pPass->m_pkRenderStateGroup) {
        pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILFUNC, 7u, true);
    pPass = spPass.m_pObject;
    if (!pPass->m_pkRenderStateGroup) {
        pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILREF, 0x80u, true);
    pPass = spPass.m_pObject;
    if (!pPass->m_pkRenderStateGroup) {
        pPass->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    pPass->m_pkRenderStateGroup->SetRenderState(D3DRS_STENCILPASS, 1u, true);
}

ImageSpaceShaderTV::ImageSpaceShaderTV() {
    pTextures.SetSize(4u);
}

void ImageSpaceShaderTV::ReloadShaders() {
    BSShaderMacro v2;
    BSShaderMacro v3;
    v3.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(v3.kMacros, 0, sizeof(v3.kMacros));
    v2.strPath = "imagespace\\2x\\p\\tv.p.hlsl";
    std::memset(v2.kMacros, 0, sizeof(v2.kMacros));
    LoadShaders(v3, v2, reinterpret_cast<char*>(&rdata_10B048C), ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderTextureMask::ReloadShaders() {
    BSShaderMacro v3;
    BSShaderMacro v4;
    v4.strPath = "imagespace\\1x\\v\\copy.v.hlsl";
    v4.kMacros[0].Name = "MASK";
    v4.kMacros[0].Definition = "";
    std::memset(&v4.kMacros[1], 0, sizeof(v4.kMacros) - sizeof(v4.kMacros[0]));
    v3.strPath = "imagespace\\1x\\p\\copy.p.hlsl";
    v3.kMacros[0].Name = "MASK";
    v3.kMacros[0].Definition = "";
    std::memset(&v3.kMacros[1], 0, sizeof(v3.kMacros) - sizeof(v3.kMacros[0]));
    const char* pcptarget = BSShaderManager::GetPSTarget(true);
    LoadShaders(v4, v3, "TEXMASK", ShaderTarget_vs_1_1, pcptarget);
    LoadStagesAndPasses();
}

void ImageSpaceShaderVATSScan::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\2x\\p\\vats_scan.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "IFVATSSCAN", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
    NiD3DPass* m_pObject = spPass.m_pObject;
    if (!m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(27u, 1u, true);
    NiD3DPass* v3 = spPass.m_pObject;
    if (!v3->m_pkRenderStateGroup) {
        v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v3->m_pkRenderStateGroup->SetRenderState(19u, 5u, true);
    if (!v3->m_pkRenderStateGroup) {
        v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v3->m_pkRenderStateGroup->SetRenderState(20u, 6u, true);
}

ImageSpaceShaderVolumetricFog::ImageSpaceShaderVolumetricFog() {
    pTextures.SetSize(3u);
}

ImageSpaceShaderVolumetricFog::~ImageSpaceShaderVolumetricFog() {
    ReturnVolumetricFogTexture();
    ImageSpaceTexture* pTexture = pTextures.m_pBase[2];
    if (pTexture) {
        delete pTexture;
        ImageSpaceTexture* pNull = nullptr;
        pTextures.SetAt(2u, pNull);
    }
}

void ImageSpaceShaderVolumetricFog::ReloadShaders() {
    BSShaderMacro pshader;
    BSShaderMacro vshader;
    vshader.strPath = "imagespace\\1x\\v\\base.v.hlsl";
    std::memset(vshader.kMacros, 0, sizeof(vshader.kMacros));
    pshader.strPath = "imagespace\\2x\\p\\VolumetricFogBlend.p.hlsl";
    std::memset(pshader.kMacros, 0, sizeof(pshader.kMacros));
    LoadShaders(vshader, pshader, "VFOG", ShaderTarget_vs_1_1, "ps_2_0");
    LoadStagesAndPasses();
}

void ImageSpaceShaderVolumetricFog::SetState() {
    NiD3DPass* m_pObject;
    m_pObject = spPass.m_pObject;
    if (!m_pObject->m_pkRenderStateGroup) {
        m_pObject->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    m_pObject->m_pkRenderStateGroup->SetRenderState(52, 1u, true);
    NiD3DPass* v3 = spPass.m_pObject;
    if (!v3->m_pkRenderStateGroup) {
        v3->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v3->m_pkRenderStateGroup->SetRenderState(56, 3u, true);
    NiD3DPass* v4 = spPass.m_pObject;
    if (!v4->m_pkRenderStateGroup) {
        v4->m_pkRenderStateGroup = NiD3DRenderStateGroup::GetFreeRenderStateGroup();
    }
    v4->m_pkRenderStateGroup->SetRenderState(57, 1u, true);
}

// Active while texture slot 2 holds a texture.
bool ImageSpaceShaderVolumetricFog::IsActive() {
    ImageSpaceTexture** m_pBase = pTextures.m_pBase;
    return m_pBase[2] && m_pBase[2]->GetTexture();
}

BSRenderedTexture* ImageSpaceShaderVolumetricFog::GetTexture() {
    if (!pTextures.m_pBase[2]) {
        ImageSpaceTexture* v2 =
            static_cast<ImageSpaceTexture*>(MemoryManager_Singleton.Allocate(sizeof(ImageSpaceTexture)));
        if (v2) {
            v2->bForceAniso = false;
            v2->bIsRenderedTexture = false;
            v2->bIsTextureBorrowed = false;
            v2->spTexture = nullptr;
            v2->eFilterMode = 0;
            v2->eClampMode = 0;
        }
        else {
            v2 = nullptr;
        }
        ImageSpaceTexture* a3 = v2;
        pTextures.SetAt(2u, a3);
    }
    if (!pTextures.m_pBase[2]->GetRenderedTexture()) {
        ImageSpaceTexture** v3 = pTextures.m_pBase + 2;
        BSRenderedTexture* v4 = BSShaderManager::pTextureManager->BorrowRenderedTexture(
            static_cast<NiDX9Renderer*>(NiRenderer::ms_pkRenderer), BSTextureManager::TEX_VFOG, 0, nullptr, 0
        );
        (*v3)->SetTexture(v4);
    }
    return pTextures.m_pBase[2]->GetRenderedTexture();
}

ImageSpaceTexture::~ImageSpaceTexture() {
    if (bIsTextureBorrowed) {
        ReturnRenderedTexture();
    }
    NiRefObject* pTexture = static_cast<NiRefObject*>(spTexture);
    if (pTexture) {
        pTexture->DecRefCount();
        spTexture = nullptr;
    }
}

void ImageSpaceTexture::ReturnRenderedTexture() {
    if (bIsTextureBorrowed) {
        BSRenderedTexture* spTexture;
        bIsTextureBorrowed = false;
        if (bIsRenderedTexture) {
            spTexture = static_cast<BSRenderedTexture*>(this->spTexture);
        }
        else {
            spTexture = nullptr;
        }
        BSShaderManager::pTextureManager->ReturnRenderedTexture(spTexture);
        NiRefObject* pTexture = static_cast<NiRefObject*>(this->spTexture);
        if (pTexture) {
            pTexture->DecRefCount();
            this->spTexture = nullptr;
        }
        bIsRenderedTexture = false;
        eFilterMode = 0;
        eClampMode = 0;
    }
}

// Releases the texture and resets the slot's flag, filter and clamp modes.
void ImageSpaceTexture::ClearTexture() {
    NiRefObject* pTexture = static_cast<NiRefObject*>(spTexture);
    if (pTexture) {
        pTexture->DecRefCount();
        spTexture = nullptr;
    }
    bIsRenderedTexture = false;
    eFilterMode = 0;
    eClampMode = 0;
}

// Borrows a rendered texture of the type and holds it.
void ImageSpaceTexture::BorrowRenderedTexture(BSTextureManager::eTextureType aeType) {
    BSRenderedTexture* ptex = BSShaderManager::pTextureManager->BorrowRenderedTexture(
        BSShaderManager::pRenderer.m_pObject, aeType, 0, nullptr, 0
    );
    NiRefObject* pOld = static_cast<NiRefObject*>(this->spTexture);
    NiRefObject* pNew = ptex;
    if (pOld != pNew) {
        if (pOld) {
            pOld->DecRefCount();
        }
        this->spTexture = ptex;
        if (ptex) {
            NiIncRefCount(ptex);
        }
    }
    bIsRenderedTexture = true;
    bIsTextureBorrowed = true;
}

// Holds apRendTex in the slot and marks it as not a rendered texture.
void ImageSpaceTexture::SetTexture(NiTexture* apRendTex) {
    NiRefObject* pOld = static_cast<NiRefObject*>(spTexture);
    NiRefObject* pNew = apRendTex;
    if (pOld != pNew) {
        if (pOld) {
            pOld->DecRefCount();
        }
        spTexture = pNew;
        if (pNew) {
            pNew->IncRefCount();
        }
    }
    bIsRenderedTexture = false;
}

void ImageSpaceTexture::SetTexture(BSRenderedTexture* apRendTex) {
    BSRenderedTexture* spTexture = static_cast<BSRenderedTexture*>(this->spTexture);
    if (spTexture != apRendTex) {
        if (spTexture) {
            NiDecRefCount(spTexture);
        }
        this->spTexture = apRendTex;
        if (apRendTex) {
            NiIncRefCount(apRendTex);
        }
    }
    bIsRenderedTexture = true;
}

BSRenderedTexture* ImageSpaceTexture::GetRenderedTexture() {
    if (bIsRenderedTexture) {
        return static_cast<BSRenderedTexture*>(spTexture);
    }

    return nullptr;
}

void ImageSpaceTexture::SetupTexture(NiD3DTextureStage* a2) {
    a2->SetClampMode(static_cast<NiTexturingProperty::ClampMode>((int32_t)eClampMode));
    a2->SetFilterMode(static_cast<NiTexturingProperty::FilterMode>((int32_t)eFilterMode));
    if (bIsRenderedTexture) {
        a2->m_spTexture = static_cast<BSRenderedTexture*>(spTexture)->GetTexture(0);
    }
    else {
        a2->m_spTexture = static_cast<NiTexture*>(spTexture);
    }
}

void ImageSpaceTexture::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
