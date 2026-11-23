#pragma once

#include "Fallout Shared/TESForm.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiColorInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatInterpolator.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinColorKey.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiLinFloatKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "SDK/MSVC/ehvec.h"

class TESSound;

// An image space modifier form: a timed change to the image space.
class TESImageSpaceModifier : public TESForm {
public:
    enum ImageSpaceModifierOperations {
        IMAOP_MULT = 0,
        IMAOP_ADD = 1,
        IMAOP_COUNT = 2,
    };

    enum ImageSpaceOpParameterEnum {
        IMAGESPACEOPPARAM_HDR_FIRST = 0,
        IMAGESPACEOPPARAM_HDR_EYE_ADAPT_SPEED = 0,
        IMAGESPACEOPPARAM_HDR_BLUR_RADIUS = 1,
        IMAGESPACEOPPARAM_HDR_SKIN_DIMMER = 2,
        IMAGESPACEOPPARAM_HDR_EMISSIVE_MULT = 3,
        IMAGESPACEOPPARAM_HDR_TARGET_LUM = 4,
        IMAGESPACEOPPARAM_HDR_UPPER_LUM_CLAMP = 5,
        IMAGESPACEOPPARAM_HDR_BRIGHT_SCALE = 6,
        IMAGESPACEOPPARAM_HDR_BRIGHT_CLAMP = 7,
        IMAGESPACEOPPARAM_HDR_LUM_RAMP_NO_TEX = 8,
        IMAGESPACEOPPARAM_HDR_LUM_RAMP_MIN = 9,
        IMAGESPACEOPPARAM_HDR_LUM_RAMP_MAX = 10,
        IMAGESPACEOPPARAM_HDR_SUNLIGHT_DIMMER = 11,
        IMAGESPACEOPPARAM_HDR_GRASS_DIMMER = 12,
        IMAGESPACEOPPARAM_HDR_TREE_DIMMER = 13,
        IMAGESPACEOPPARAM_HDR_LAST = 13,
        IMAGESPACEOPPARAM_BLOOM_FIRST = 14,
        IMAGESPACEOPPARAM_BLOOM_BLUR_RADIUS = 14,
        IMAGESPACEOPPARAM_BLOOM_ALPHA_ADD_INTERIOR = 15,
        IMAGESPACEOPPARAM_BLOOM_ALPHA_ADD_EXTERIOR = 16,
        IMAGESPACEOPPARAM_BLOOM_LAST = 16,
        IMAGESPACEOPPARAM_CINEMATIC_FIRST = 17,
        IMAGESPACEOPPARAM_CINEMATIC_SATURATION_VALUE = 17,
        IMAGESPACEOPPARAM_CINEMATIC_CONTRAST_AVG_LUM = 18,
        IMAGESPACEOPPARAM_CINEMATIC_CONTRAST_VALUE = 19,
        IMAGESPACEOPPARAM_CINEMATIC_BRIGHTNESS = 20,
        IMAGESPACEOPPARAM_CINEMATIC_LAST = 20,
        NUM_OP_PARAMS = 21,
    };

    // The record's DNAM: timing and the key count of every animated parameter.
    struct ImageSpaceModifierData {
        // Swaps the fields read from a big-endian file.
        void Endian();

        bool bAnimatable;
        float fDuration;
        uint32_t ppuiKeySize[NUM_OP_PARAMS][IMAOP_COUNT];
        uint32_t uiTintColorKeySize;
        uint32_t uiBlurKeySize;
        uint32_t uiDoubleKeySize;
        uint32_t uiRadialBlurStrengthKeySize;
        uint32_t uiRadialBlurRampupKeySize;
        uint32_t uiRadialBlurStartKeySize;
        bool bUseTargetForRadialBlur;
        NiPoint2 RadialBlurCenter;
        uint32_t uiDepthOfFieldStrengthKeySize;
        uint32_t uiDepthOfFieldDistanceKeySize;
        uint32_t uiDepthOfFieldRangeKeySize;
        bool bUseTargetForDepthOfField;
        uint8_t ucDepthOfFieldMode;
        uint32_t uiRadialBlurRampDownKeySize;
        uint32_t uiRadialBlurDownStartKeySize;
        uint32_t uiFadeColorKeySize;
        uint32_t uiMotionBlurStrengthKeySize;
    };

    // The number of keys every default curve starts with.
    static constexpr uint32_t NUM_START_KEYS = 2;
    typedef float StartKeyTimes[NUM_START_KEYS];
    typedef float OpDefaults[IMAOP_COUNT];

    TESImageSpaceModifier();
    ~TESImageSpaceModifier() override;

    void InitializeData() override;
    void ClearData() override;
    bool Load(TESFile* apFile) override;
    void Save() override;
    void InitItem() override;
    void Copy(TESForm* apCopy) override;
    bool Compare(TESForm* apCompare) override;

    using TESForm::Save;

    // Resets the modifier to the engine's defaults, animated, with a slight double vision.
    void MakeDefault();
    void Apply(float afStartTime, float afStrength, NiAVObject* apTarget);
    void ApplyWeather(float afStartTime, float afStrength);
    bool IsExpired(float afStartTime);
    // Copies each interpolator's key count into Data and its key array into the matching key pointer.
    void FlushData();

    // The modifier the game plays when the player is hit.
    static TESImageSpaceModifier* GetGetHit();
    // The default modifier, &sDefault.
    static TESImageSpaceModifier* QDefault();
    static void BlendApply(
        float afPercent,
        TESImageSpaceModifier* apMod1,
        TESImageSpaceModifier* apMod2,
        float afStrength1,
        float afStrength2,
        NiAVObject* apTarget1,
        NiAVObject* apTarget2
    );
    static float GetTickCount();
    static bool GetEnable();
    static void SetEnable(bool abEnable);
    static void AllowGetHit(bool abEnable);
    static void AdvanceCurrentTime(float afAmount);
    static TESImageSpaceModifier* GetExplosionInFace();

    TESSound* pOutroSound;
    TESSound* pIntroSound;
    ImageSpaceModifierData Data;
    NiFloatInterpolator ppInterpolator[NUM_OP_PARAMS][IMAOP_COUNT];
    NiFloatInterpolator BlurInterpolator;
    NiFloatInterpolator DoubleInterpolator;
    NiColorInterpolator TintColorInterpolator;
    NiColorInterpolator FadeColorInterpolator;
    NiFloatInterpolator RadialBlurStrengthInterpolator;
    NiFloatInterpolator RadialBlurRampupInterpolator;
    NiFloatInterpolator RadialBlurStartInterpolator;
    NiFloatInterpolator RadialBlurRampDownInterpolator;
    NiFloatInterpolator RadialBlurDownStartInterpolator;
    NiFloatInterpolator DepthOfFieldStrengthInterpolator;
    NiFloatInterpolator DepthOfFieldDistanceInterpolator;
    NiFloatInterpolator DepthOfFieldRangeInterpolator;
    NiFloatInterpolator MotionBlurStrengthInterpolator;
    NiLinFloatKey* pppFloatKey[NUM_OP_PARAMS][IMAOP_COUNT];
    NiLinFloatKey* pBlurFloatKey;
    NiLinFloatKey* pDoubleFloatKey;
    NiLinColorKey* pTintColorKey;
    NiLinColorKey* pFadeColorKey;
    NiLinFloatKey* pRadialBlurStrengthFloatKey;
    NiLinFloatKey* pRadialBlurRampupFloatKey;
    NiLinFloatKey* pRadialBlurStartFloatKey;
    NiLinFloatKey* pRadialBlurRampDownFloatKey;
    NiLinFloatKey* pRadialBlurDownStartFloatKey;
    NiLinFloatKey* pDepthOfFieldStrengthFloatKey;
    NiLinFloatKey* pDepthOfFieldDistanceFloatKey;
    NiLinFloatKey* pDepthOfFieldRangeFloatKey;
    NiLinFloatKey* pMotionBlurStrengthFloatKey;

    static inline StartKeyTimes fDefaultKeyTimes{0.0f, 1.0f};
    static inline OpDefaults fOpDefault{1.0f};
    static inline bool bEnable = true;
    static inline float fCurrentTime;
    static TESImageSpaceModifier Default;
};

// Whether hit and concussion image space modifiers play.
inline bool g_bEnableHitAndConcussionIMODs = true;

inline TESImageSpaceModifier* g_GetHitIMOD;
inline TESImageSpaceModifier* g_explosionInFaceIMOD;

inline TESImageSpaceModifier* g_ConcussionIMOD;
