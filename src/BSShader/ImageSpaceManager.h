#pragma once

#include "BSShader/BSTextureManager.h"
#include "BSShader/ImageSpaceEffectDepthOfField.h"
#include "BSShader/ImageSpaceEffectRadialBlur.h"
#include "BSShader/ImageSpaceTexture.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRect.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElementsData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexturingProperty.h"

#include <stdint.h>

class NiDX9Renderer;

// The tunable HDR, bloom, hit, night-eye and cinematic values an image space applies to the final image.
struct ImageSpaceParameterData {
    // Byte swaps every value.
    void Endian();

    float pfHDRData[15];
    float pfBloomData[3];
    float pfGetHitData[3];
    float pfNightEyeData[4];
    float pfCinematicData[12];
    uint32_t uiCinematicsEnabled;
};

// Owns the full-screen post-processing effects and the blended image space parameters. The Mod* functions add a
// weather contribution; the Apply*ParamModifier functions keep the strongest of the active contributions.
class ImageSpaceManager {
public:
    // Indices into the effect list (InitEffects fills it). ApplyParamModifier selects a parameter block by the bloom,
    // HDR and cinematic ones.
    enum EffectID : int32_t {
        EFFECT_BLOOM = 0,
        EFFECT_HDR = 1,
        EFFECT_REFRACTION = 2,
        EFFECT_CINEMATIC = 3,
        EFFECT_DEPTH_OF_FIELD = 4,
        EFFECT_DEPTH_RADIAL_BLUR = 5,
        EFFECT_RADIAL_BLUR = 6,
        EFFECT_FULLSCREEN_BLUR = 7,
        EFFECT_GET_HIT = 8,
        EFFECT_GETHIT = 8,
        EFFECT_SUNBEAMS = 9,
        EFFECT_INTERFACE = 10,
        EFFECT_VATS_SCAN = 11,
        EFFECT_MOTION_BLUR = 12,
        EFFECT_ENDOFFRAME_END = 13,
        EFFECT_VOLUMETRIC_FOG = 13,
        EFFECT_MAP = 14,
        // ImageSpaceEffectMenuBG.
        EFFECT_MENU_BG = 15,
        EFFECT_MENUBG = 15,
        EFFECT_BLUR_START = 16,
        EFFECT_BLUR3 = 16,
        EFFECT_BLUR5 = 17,
        EFFECT_BLUR7 = 18,
        EFFECT_BLUR9 = 19,
        EFFECT_BLUR11 = 20,
        EFFECT_BLUR13 = 21,
        EFFECT_BLUR15 = 22,
        EFFECT_BLUR_END = 22,
        EFFECT_BRIGHTPASS_BLUR3 = 23,
        EFFECT_BRIGHTPASS_BLUR5 = 24,
        EFFECT_BRIGHTPASS_BLUR7 = 25,
        EFFECT_BRIGHTPASS_BLUR9 = 26,
        EFFECT_BRIGHTPASS_BLUR11 = 27,
        EFFECT_BRIGHTPASS_BLUR13 = 28,
        EFFECT_BRIGHTPASS_BLUR15 = 29,
        EFFECT_TV = 30,
        EFFECT_WATER_FFT = 31,
        EFFECT_WATER_DISPLACEMENT = 32,
        EFFECT_NOISE = 33,
        EFFECT_TOP_LEVEL_END = 34,
        EFFECT_NIGHTVISION = 34,
        EFFECT_END = 34,
        EFFECT_SHADER_START = 34,
        // The plain copy shader's slot, rendered when no end-of-frame effect is active.
        EFFECT_SHADER_COPY = 34,
        EFFECT_SHADER_COPY_RENDER_TARGET1 = 35,
        EFFECT_SHADER_RESTORE_EDRAM = 36,
        EFFECT_SHADER_NULL = 37,
        EFFECT_SHADER_COPYWITHALPHA = 38,
        EFFECT_SHADER_STENCILCOPY = 39,
        // ImageSpaceShaderStencilCopyNoTextureNoDepth.
        EFFECT_SHADER_COPY_STENCIL_NO_TEXTURE_NO_DEPTH = 40,
        EFFECT_SHADER_STENCILCOPY_NOTEX_NODEPTH = 40,
        EFFECT_SHADER_REFRACTION = 41,
        EFFECT_SHADER_VOLUMETRIC_FOG = 42,
        EFFECT_SHADER_BLUR = 43,
        EFFECT_SHADER_DOUBLEVIS = 44,
        EFFECT_SHADER_ALPHAMULT = 45,
        EFFECT_SHADER_BLEND = 46,
        EFFECT_SHADER_TEXTURE_MASK = 47,
        EFFECT_SHADER_TEXTUREMASK = 47,
        EFFECT_SHADER_MAP = 48,
        EFFECT_SHADER_MENUBG = 49,
        // The shader ImageSpaceEffectCinematic::Setup binds.
        EFFECT_CINEMATIC_SHADER = 50,
        EFFECT_SHADER_CINEMATIC = 50,
        EFFECT_SHADER_DEPTH_OF_FIELD = 51,
        EFFECT_SHADER_DEPTH_OF_FIELD_MASK = 52,
        EFFECT_SHADER_MOTION_BLUR = 53,
        EFFECT_SHADER_RADIAL_BLUR = 54,
        EFFECT_SHADER_RADIAL_BLUR_MED = 55,
        EFFECT_SHADER_RADIAL_BLUR_HIGH = 56,
        EFFECT_SHADER_SUNBEAMS = 57,
        EFFECT_SHADER_HDR_BLENDINSHADER = 58,
        EFFECT_SHADER_HDR_BLENDINSHADER_CINEMATIC = 59,
        EFFECT_SHADER_HDR_BLENDINSHADER_CINEMATIC_ALPHAMASK = 60,
        EFFECT_SHADER_HDR_BLUR = 61,
        EFFECT_SHADER_HDR_BRIGHTPASSFILTER = 62,
        EFFECT_SHADER_HDR_DOWNSAMPLE16 = 63,
        EFFECT_SHADER_HDR_DOWNSAMPLE9 = 64,
        EFFECT_SHADER_HDR_DOWNSAMPLE4 = 65,
        EFFECT_SHADER_HDR_DOWNSAMPLE4_LUMCLAMP = 66,
        EFFECT_SHADER_HDR_DOWNSAMPLE4_LIGHTADAPT = 67,
        EFFECT_SHADER_HDR_DOWNSAMPLE16_LUMCLAMP = 68,
        EFFECT_SHADER_HDR_DOWNSAMPLE16_LIGHTADAPT = 69,
        EFFECT_SHADER_HDR_LIGHTADAPT = 70,
        EFFECT_SHADER_HDR_LUMCLAMP = 71,
        EFFECT_SHADER_INTERFACE_SCANLINES_BLEND = 72,
        EFFECT_SHADER_INTERFACE_VATS_SCANLINES_BLEND = 73,
        EFFECT_SHADER_INTERFACE_SINGLEPASS = 74,
        // The shader ImageSpaceEffectVATSScan::Setup binds.
        EFFECT_VATS_SCAN_SHADER = 75,
        EFFECT_SHADER_INTERFACE_VATS_SCAN = 75,
        EFFECT_SHADER_INTERFACE_MENUUNPACK = 76,
        EFFECT_SHADER_TV = 77,
        EFFECT_SHADER_BLUR_START = 78,
        EFFECT_SHADER_BLUR3 = 78,
        EFFECT_SHADER_BLUR5 = 79,
        EFFECT_SHADER_BLUR7 = 80,
        EFFECT_SHADER_BLUR9 = 81,
        EFFECT_SHADER_BLUR11 = 82,
        EFFECT_SHADER_BLUR13 = 83,
        EFFECT_SHADER_BLUR15 = 84,
        EFFECT_SHADER_BLUR_END = 84,
        EFFECT_SHADER_BRIGHTPASS_BLUR_START = 85,
        EFFECT_SHADER_BRIGHTPASS_BLUR3 = 85,
        EFFECT_SHADER_BRIGHTPASS_BLUR5 = 86,
        EFFECT_SHADER_BRIGHTPASS_BLUR7 = 87,
        EFFECT_SHADER_BRIGHTPASS_BLUR9 = 88,
        EFFECT_SHADER_BRIGHTPASS_BLUR11 = 89,
        EFFECT_SHADER_BRIGHTPASS_BLUR13 = 90,
        EFFECT_SHADER_BRIGHTPASS_BLUR15 = 91,
        EFFECT_SHADER_BRIGHTPASS_BLUR_END = 91,
        EFFECT_SHADER_WATER_FFT_START = 92,
        EFFECT_SHADER_WATER_FFT_WATER_SPECTRUM = 93,
        EFFECT_SHADER_WATER_FFT_HORIZONTAL_BUTTERFLY = 94,
        EFFECT_SHADER_WATER_FFT_VERTICAL_BUTTERFLY = 95,
        EFFECT_SHADER_WATER_FFT_HORIZONTAL_SCRAMBLE = 96,
        EFFECT_SHADER_WATER_FFT_VERTICAL_SCRAMBLE = 97,
        EFFECT_SHADER_WATER_FFT_NORMALS = 98,
        EFFECT_SHADER_WATER_FFT_FILTER = 99,
        EFFECT_SHADER_WATER_FFT_DISPLAY_NORMALS = 100,
        EFFECT_SHADER_WATER_FFT_DISPLAY_HIGHW = 101,
        EFFECT_SHADER_WATER_FFT_DISPLAY_AMPLITUDE = 102,
        EFFECT_SHADER_WATER_FFT_DISPLAY_WATER_SPECTRUM = 103,
        EFFECT_SHADER_WATER_FFT_DISPLAY_WATER_HEIGHT = 104,
        EFFECT_SHADER_WATER_FFT_END = 104,
        EFFECT_SHADER_WATER_DISPLACEMENT_START = 105,
        EFFECT_SHADER_WATER_DISPLACEMENT_CLEAR_SIMULATION = 105,
        EFFECT_SHADER_WATER_DISPLACEMENT_TEX_OFFSET = 106,
        EFFECT_SHADER_WATER_DISPLACEMENT_WADING_RIPPLE = 107,
        EFFECT_SHADER_WATER_DISPLACEMENT_RAIN_RIPPLE = 108,
        EFFECT_SHADER_WATER_DISPLACEMENT_WADING_HEIGHTMAP = 109,
        EFFECT_SHADER_WATER_DISPLACEMENT_RAIN_HEIGHTMAP = 110,
        EFFECT_SHADER_WATER_DISPLACEMENT_BLEND_HEIGHTMAPS = 111,
        EFFECT_SHADER_WATER_DISPLACEMENT_SMOOTH_HEIGHTMAP = 112,
        EFFECT_SHADER_WATER_DISPLACEMENT_NORMALS = 113,
        EFFECT_SHADER_WATER_DISPLACEMENT_END = 113,
        EFFECT_SHADER_NOISE_SCROLL_AND_BLEND = 114,
        EFFECT_SHADER_NOISE_NORMALMAP = 115,
        EFFECT_COUNT = 116,
        EFFECT_NONE = -1,
    };

    ImageSpaceManager();
    ~ImageSpaceManager();

    BSRenderedTexture* GetDepthRenderedTexture();
    ImageSpaceEffect* GetEffect(ImageSpaceManager::EffectID aiIndex);
    void SetBaseParameterData(const ImageSpaceParameterData* apParameterData);
    void InitDefaultParameter();
    void ResetWeatherData();
    void ApplyCurrentParameterData(bool abSkyTransition);
    // Both scale akColor's r, g and b by its alpha in place before adding it.
    void ApplyTintColorParamModifier(NiColorA& akColor);
    void ApplyFadeColorParamModifier(NiColorA& akColor);
    void BeginParamModifiers();
    // Creates the effects and the screen geometry and takes the swap target, once.
    void Initialize();
    // Creates the effect and shader instances and stores them in pEffectList.
    void InitEffects();
    void InitGeometry();
    // afValue holds the multiplier and the addend applied to the current value; afStrength blends the result in.
    void
    ApplyParamModifier(ImageSpaceManager::EffectID aeEffect, uint32_t uiParam, const float* afValue, float afStrength);
    // Adds afValue[0] and afValue[1], scaled by afStrength, to the two weather parameter blocks.
    void ApplyParamModifierWeather(
        ImageSpaceManager::EffectID aeEffect,
        uint32_t uiParam,
        const float* afValue,
        float afStrength
    );
    void EndWeatherModifiers();
    void RenderEndOfFrameEffects(
        NiDX9Renderer* apRenderer,
        BSRenderedTexture*& apScreenBuffer,
        BSRenderedTexture*& apDestinationBuffer
    );
    // Renders effect aiEffect with apDestinationBuffer as its only input texture. abEndFrame is int32_t rather than
    // bool because the caller's whole value is passed on; bool would be zero-extended.
    void RenderEffect(
        ImageSpaceManager::EffectID aiEffect,
        NiDX9Renderer* apRenderer,
        BSRenderedTexture* apDestinationBuffer,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    );
    // Renders effect aiEffect from apSourceBuffer into apDestinationBuffer.
    void RenderEffect(
        ImageSpaceManager::EffectID aiEffect,
        NiDX9Renderer* apRenderer,
        BSRenderedTexture* apSourceBuffer,
        BSRenderedTexture* apDestinationBuffer,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    );
    // Renders effect aiEffect from the texture apSourceTarget into apDestTarget.
    void RenderEffect(
        ImageSpaceManager::EffectID aiEffect,
        NiDX9Renderer* apRenderer,
        NiTexture* apSourceTarget,
        BSRenderedTexture* apDestTarget,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    );
    // abEndFrame is an int32_t rather than a bool because the caller's whole value is passed on unchanged; a bool would
    // be zero-extended.
    void RenderEffect(
        ImageSpaceEffect* apEffect,
        NiDX9Renderer* apRenderer,
        BSRenderedTexture* apSourceBuffer,
        BSRenderedTexture* apDestinationBuffer,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    );
    // abEndFrame is a bool flag declared as int32_t because the caller's whole value is passed on unchanged; a bool
    // would be zero-extended.
    void RenderEffect(
        ImageSpaceEffect* apEffect,
        NiDX9Renderer* apRenderer,
        NiTexture* apSourceTarget,
        BSRenderedTexture* apDestTarget,
        ImageSpaceEffectParam* apParam,
        int32_t abEndFrame
    );

    void ModBlurRadius(float afRadius);
    void ModFullScreenMotionBlurStrength(float afStrength);
    void ModDoubleVisionStrength(float afStrength);
    void ApplyTintColorParamModifierWeather(const NiColorA& akColor);
    void ModFadeColor(const NiColorA& akColor);
    void ModDepthOfField(float afStrength, int32_t aeMode, float afMaskConst, float afDistance, float afRange);
    void ModRadialBlur(
        float afStrength,
        float afRampup,
        float afStart,
        float afRampdown,
        float afDownStart,
        const NiPoint2* apCenter
    );
    void ApplyBlurRadiusParamModifier(float afBlurRadius);
    void ApplyMotionBlurStrengthParamModifier(float afBlurRadius);
    void ApplyDoubleVisionParamModifier(float afDoubleVision);
    void ApplyRadialBlurParamModifier(
        float afStrength,
        float afRampup,
        float afStart,
        float afRampdown,
        float afDownStart,
        const NiPoint2& aCenter
    );
    // Takes the centre and the start bias from aBound (a world bound) instead of from the caller.
    void ApplyRadialBlurParamModifier(
        float afStrength,
        float afRampup,
        float afStart,
        float afRampdown,
        float afDownStart,
        const NiBound& aBound
    );
    void ApplyDepthOfFieldParamModifier(
        float afStrength,
        uint32_t auiMode,
        float afMaskConst,
        float afDistance,
        float afRange
    );
    // Applies the world-bound depth-of-field modifier.
    void
    ApplyDepthOfFieldParamModifier_0(float afStrength, uint32_t auiMode, float afMotionBlur, const NiPoint3* apBound);
    // Applies the depth radial-blur modifier around arBound.
    void ApplyDepthRadialBlurParamModifier(
        float afStrength,
        uint32_t auiMode,
        float afRampup,
        float afStart,
        const NiBound& arBound
    );
    // Applies the depth radial-blur modifier at arCenter.
    void ApplyDepthRadialBlurParamModifier(
        float afStrength,
        uint32_t auiMode,
        float afDistance,
        float afRange,
        float afRampup,
        float afStart,
        const NiPoint2& arCenter
    );
    bool IsEffectActive(ImageSpaceManager::EffectID aiEffect);
    // Reloads the shaders of the effects from the copy shader on;
    // BSShaderManager::ReloadShaders tail-calls it.
    void ReloadEffectShaders();
    void PrepareEffectRange();
    void SetupEffects();
    void ResetToBaseParameterData();
    void SetUseSunbeams(bool abUseSunbeams);
    // Turns the menu background blend effect on or off. It calls the menu background effect's SetBlendEnabled, so it
    // needs the ImageSpaceEffect class family.
    void SetMenuBGBlendEnabled(bool abEnabled);
    // Passes afStrength to the menu background effect (effect 15, ImageSpaceEffectMenuBG).
    void SetMenuBGBlendValue(float afStrength);
    // Sets bEnablePartialRender and holds apPartialScreenGeometry as the partial screen geometry.
    void EnablePartialRender(bool abEnable, NiScreenElements* apPartialScreenGeometry);
    // Builds the partial screen geometry: a clip-space quad from the top left corner, afWidthRatio and
    // afHeightRatio of the screen and its texture, held in spPartialScreenTriShape and aspScreenElement.
    void
    CreatePartialScreenGeometry(float afWidthRatio, float afHeightRatio, NiPointer<NiScreenElements>& aspScreenElement);
    void SetCustomViewPort(float afLeft, float afRight, float afTop, float afBottom);
    void ResetViewport();
    float GetEmissiveHDRMult();
    float GetNumBlurPasses();
    float GetHDRBrightClamp();
    float GetHDRBrightScale();
    float GetHDREyeAdaptSpeed();
    float GetHDRTargetLUM();
    float GetHDRUpperLUMClamp();
    void SetSunPosition(const NiPoint2& aSunPos);
    void SetMotionBlurConst(float afConst);
    void ShutdownGeometry();
    void GetDepthOfFieldParamsFromPoint(const NiPoint3* apPoint, NiPoint2* apParams);
    void ShutdownEffects();
    // Stores the argument into pOverRideBaseParameterData.
    void SetActiveImageSpaceParams(const ImageSpaceParameterData* apData);
    void EndParamModifiers();

    // SetAt on the pEffectList array.
    static void
    SetEffectAt(NiTPrimitiveArray<ImageSpaceEffect*>* apArray, uint32_t auiIndex, ImageSpaceEffect* const* appEffect);
    // The first argument is passed on as GetBoundParams' const NiBound&, and the caller passes the target's world
    // bound.
    static void GetRadialBlurCenterFromWorldBound(const NiBound* apBound, NiPoint2* apCenter);

    BSRenderedTexture* pEOFDest;
    NiTPrimitiveArray<ImageSpaceEffect*> pEffectList;
    ImageSpaceParameterData CurrentParameterData;
    const ImageSpaceParameterData* pBaseParameterData;
    const ImageSpaceParameterData* pOverRideBaseParameterData;
    // [0] multiplies and [1] adds to the current data (ResetWeatherData sets 1 and 0).
    ImageSpaceParameterData WeatherParameterData[2];
    bool bIsReady;
    int32_t iActiveEffectsCount;
    EffectID iLastEffect;
    // InitGeometry builds an NiScreenElements and its data here.
    NiPointer<NiScreenElements> spScreenTriShape;
    NiPointer<NiScreenElementsData> spScreenTriShapeData;
    NiPointer<NiScreenElements> spPartialScreenTriShape;
    bool bEnablePartialRender;
    NiTriShape* spMenuUnpackGeom;
    ImageSpaceTexture DepthTexture;
    NiColorA kTintColor;
    NiColorA kWeatherTintColor;
    float fHighestTintValue;
    NiColorA kFadeColor;
    NiColorA kWeatherFadeColor;
    float fHighestFadeValue;
    float fHighestBlurValue;
    float fWeatherBlurValue;
    float fHighestDoubleValue;
    float fWeatherDoubleValue;
    float fHighestRadialBlurStrengthValue;
    float fCurrentRadialBlurRampupValue;
    float fCurrentRadialBlurStartValue;
    float fCurrentRadialBlurRampDownValue;
    float fCurrentRadialBlurDownStartValue;
    float fWeatherRadialBlurStrengthValue;
    float fWeatherRadialBlurRampupValue;
    float fWeatherRadialBlurStartValue;
    float fWeatherRadialBlurRampDownValue;
    float fWeatherRadialBlurDownStartValue;
    NiPoint2 fCurrentRadialBlurCenter;
    NiPoint2 fWeatherRadialBlurCenter;
    float fHighestDOFStrengthValue;
    float fCurrentDOFDistanceValue;
    float fCurrentDOFRangeValue;
    float fCurrentDOFMaskConst;
    float fWeatherDOFStrengthValue;
    float fWeatherDOFDistanceValue;
    float fWeatherDOFRangeValue;
    float fWeatherDOFMaskConst;
    ImageSpaceEffectDepthOfField::DepthOfFieldMode fCurrentDOFMode;
    ImageSpaceEffectDepthOfField::DepthOfFieldMode fWeatherDOFMode;
    bool bCurrentDOFUseMask;
    bool bWeatherDOFUseMask;
    float fHighestDepthRadialBlurStrengthValue;
    float fCurrentDepthRadialBlurRampupValue;
    float fCurrentDepthRadialBlurStartValue;
    float fCurrentDepthRadialBlurDistance;
    float fCurrentDepthRadialBlurRange;
    NiPoint2 fCurrentDepthRadialBlurCenter;
    ImageSpaceEffectDepthRadialBlur::RadialBlurMode fCurrentDepthRadialBlurMode;
    float fHighestMotionBlurStrengthValue;
    float fWeatherMotionBlurStrengthValue;
    NiColorA kRefractionTint;
    NiRect<float> CustomViewPort;
    BSRenderedTexture* pSwapTarget;

    static inline bool bEOFEnabled = true;
    static inline ImageSpaceParameterData DefaultParameterData{};
    static inline bool bUseSunbeams;
};
