#pragma once

#include <stdint.h>

class BlendSettingCollection;
class GameSettingCollection;
class INIPrefSettingCollection;
class INISettingCollection;
class RegSettingCollection;
class RendererSettingCollection;

template <class T>
class NiTPrimitiveArray;

// A named configuration value: a union of the supported value types plus the setting's key.
class Setting {
public:
    union Info {
        uint32_t uint;
        int32_t i;
        float f;
        char* str;
        bool b;
        uint16_t us;
        // The value's low byte as a raw byte: ST_BINARY and ST_CHAR values are read and written as a byte.
        uint8_t uc;
    };

    // The value type GetType derives from the key's prefix.
    enum SETTING_TYPE {
        ST_BINARY = 0,
        ST_CHAR = 1,
        ST_UCHAR = 2,
        ST_INT = 3,
        ST_UINT = 4,
        ST_FLOAT = 5,
        ST_STRING = 6,
        ST_RGB = 7,
        ST_RGBA = 8,
        ST_NONE = 9,
    };

    Setting(const char* apID, Info aValue);
    Setting(const char* apID, bool abValue);
    Setting(const char* apID, float afValue);
    virtual ~Setting();

    // false for a plain setting; the ini preference settings override it
    virtual bool IsPrefSetting() {
        return false;
    }

    // Stores aiValue as the integer value.
    void SetInt(int32_t aiValue);
    // The string value, or null for a null setting.
    const char* String();

    // The accessors below return the stored value; for a null setting they zero DefaultValue and return that.
    int32_t& Int();
    uint32_t& UInt();
    float& Float();
    bool& Bool();

    operator uint32_t();
    operator float();
    operator bool();
    // The string value through String(). Explicit so the existing implicit conversions stay
    // unambiguous.
    explicit operator const char*();
    // Stores a copy of apValue as the string value, reallocating a Setting key; returns this.
    Setting& operator=(const char* apValue);
    // Stores a float value; returns this.
    Setting& operator=(float afValue);
    // Stores a boolean value, a null this storing nothing; returns this.
    Setting& operator=(bool abValue);

    // True when the setting exists and its boolean value equals abValue.
    bool IsEqual(bool abValue);

    // The setting's type, derived from its key.
    int32_t GetType();

    // Fills apStrings with this setting's viewer string. Defined in Setting.cpp.
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA);
    // The setting's name (pKey).
    const char* Key() {
        return pKey;
    }
    // Whether the setting exists and its value, read as a 32-bit integer, equals aiValue.
    bool operator==(int32_t aiValue);
    uint32_t DataSize();

    static void operator delete(void* apMemory);
    // The type letter of the key, as the engine classifies setting names; callers outside Setting use it.
    static SETTING_TYPE DataType(const char* apKey);

    Info uValue;
    const char* pKey;

    // The engine's shared scratch value handed out for null settings.
    static inline Info DefaultValue{};
};

// A setting registered in one settings collection.
template <class T>
class SettingT : public Setting {
public:
    // Each constructor stores the key and value, then registers the setting in its collection. Defined in
    // Setting.cpp and instantiated there for every collection: the collections' headers include this one.
    SettingT(const char* apID, Info aValue);
    SettingT(const char* apID, bool abValue);
    SettingT(const char* apID, float afValue);
    // The int, unsigned and string values are stored directly in the value, as the game's Info constructor does.
    SettingT(const char* apID, int32_t aiValue);
    SettingT(const char* apID, uint32_t aiValue);
    SettingT(const char* apID, const char* apString);
    // A double value is stored as a float.
    SettingT(const char* apID, double adValue) : SettingT(apID, static_cast<float>(adValue)) {}
    ~SettingT() override;

    // Stores an integer value; returns this.
    SettingT& operator=(int32_t aiValue);
    // Stores a boolean value; returns this.
    SettingT& operator=(bool abValue);
    // Stores a float value; returns this.
    SettingT& operator=(float afValue);
    // Stores a copy of apString as the string value; returns this.
    SettingT& operator=(const char* apString);
    // Stores the setting's key and default value in the collection. The value is the whole Info union, not a bool.
    void InitString(const char* apName, Setting::Info aValue);
    bool operator!() const {
        return !uValue.b;
    }
    // Whether the setting exists and its boolean value equals abValue.
    bool operator==(bool abValue);
    // Whether the setting exists and its value equals aiValue.
    bool operator==(int32_t aiValue);

    // The collection the settings of this kind are registered in, created on first use.
    static T* GetCollection();
    // Creates the collection the first time it is needed (one body per collection type).
    static void InitCollection();

    // The collection the settings of this kind are registered in.
    static T* pCollection;
};

template <class T>
inline T* SettingT<T>::pCollection;

// Settings built with SettingT's constructors in their files' dynamic initialisers.
extern SettingT<INISettingCollection> gs_bShowLoadingAreaMessage_General;
extern SettingT<INISettingCollection> gs_bSelectivePurgeUnusedOnFastTravel_BackgroundLoad;
extern SettingT<GameSettingCollection> gs_iLevCharLevelDifferenceMax;
extern SettingT<GameSettingCollection> gs_iLevCreaLevelDifferenceMax;
extern SettingT<GameSettingCollection> gs_iLevItemLevelDifferenceMax;
extern SettingT<GameSettingCollection> gs_fCreatureCalcCombat;

// Settings built with SettingT<INISettingCollection>'s float and bool constructors in Renderer.cpp's dynamic
// initialisers.
extern SettingT<INISettingCollection> gs_fDecalLOD0_Display;
extern SettingT<INISettingCollection> gs_bDoAmbientPass_Display;
extern SettingT<INISettingCollection> gs_bDoDiffusePass_Display;
extern SettingT<INISettingCollection> gs_bDoSpecularPass_Display;
extern SettingT<INISettingCollection> gs_bDoTexturePass_Display;

// BSFadeNode's fLODMultTrees; BSFadeNode.cpp's initialiser builds it with SettingT<INISettingCollection>'s float
// constructor.
extern SettingT<INISettingCollection> gs_fLODMultTrees_LOD;

// Variable names use the gs_ prefix; a ":Section" suffix of a setting name is spelled _Section.
extern SettingT<GameSettingCollection> gs_fConfidenceFoolhardy;
extern SettingT<GameSettingCollection> gs_fConfidenceAverage;
extern SettingT<GameSettingCollection> gs_fConfidenceBrave;
extern SettingT<GameSettingCollection> gs_fConfidenceCautious;
extern SettingT<GameSettingCollection> gs_fConfidenceCowardly;

// The [LightAttenuation] ini settings: each is constructed by SettingT<INISettingCollection>'s constructors and
// destroyed by its destructor at exit.
extern SettingT<INISettingCollection> gs_fLinearRadiusMult_bLightAttenuation;
extern SettingT<INISettingCollection> gs_bOutQuadInLin_bLightAttenuation;
extern SettingT<INISettingCollection> gs_bUseLinear_bLightAttenuation;
extern SettingT<INISettingCollection> gs_fConstantValue_bLightAttenuation;
extern SettingT<INISettingCollection> gs_bUseQuadratic_bLightAttenuation;
extern SettingT<INISettingCollection> gs_fQuadraticValue_bLightAttenuation;
extern SettingT<INISettingCollection> gs_uLinearMethod_bLightAttenuation;
extern SettingT<INISettingCollection> gs_bUseConstant_bLightAttenuation;
extern SettingT<INISettingCollection> gs_fQuadraticRadiusMult_bLightAttenuation;
extern SettingT<INISettingCollection> gs_uQuadraticMethod_bLightAttenuation;
extern SettingT<INISettingCollection> gs_fLinearValue_bLightAttenuation;

extern SettingT<GameSettingCollection> gs_sBetMax;

// These are constructed with SettingT<GameSettingCollection>'s float constructor.
extern SettingT<GameSettingCollection> gs_fLeafRustleTimeScale;
extern SettingT<GameSettingCollection> gs_fLeafRockSpeedSwayInfluence;
extern SettingT<GameSettingCollection> gs_fLeafRustleSpeedSwayInfluence;
extern SettingT<GameSettingCollection> gs_fLeafRustleAmountSwayInfluence;
extern SettingT<GameSettingCollection> gs_fLeafRockAmountSwayInfluence;
extern SettingT<GameSettingCollection> gs_fTreeNearDistanceBase;
extern SettingT<GameSettingCollection> gs_fTreeSizeConversion;

extern SettingT<INISettingCollection> gs_bEnableTrees_SpeedTree;
extern SettingT<INISettingCollection> gs_fTreeForceCS_SpeedTree;
extern SettingT<INISettingCollection> gs_fTreeForceBranchDimming_SpeedTree;
extern SettingT<INISettingCollection> gs_fTreeForceLeafDimming_SpeedTree;
extern SettingT<INISettingCollection> gs_fTreeForceMinBudAngle_SpeedTree;
extern SettingT<INISettingCollection> gs_fTreeForceMaxBudAngle_SpeedTree;

extern SettingT<INISettingCollection> gs_fPulseRate_InterfaceFX;
extern SettingT<INISettingCollection> gs_fPulseRadius_VATS;
extern SettingT<INISettingCollection> gs_fBurstDuration_VATS;
extern SettingT<INISettingCollection> gs_fDistortVerticalScale_VATS;
extern SettingT<INISettingCollection> gs_fScanModeDuration_VATS;
extern SettingT<INISettingCollection> gs_fBurstIntensity_VATS;
extern SettingT<INISettingCollection> gs_iDistortMaxInterval_VATS;
extern SettingT<INISettingCollection> gs_fScanModeVerticalScale_VATS;
extern SettingT<INISettingCollection> gs_fPulseIntensity_VATS;
extern SettingT<INISettingCollection> gs_fPulseRate_VATS;
extern SettingT<INISettingCollection> gs_fASFadeOutTime_Audio;

extern SettingT<INIPrefSettingCollection> gs_fGamma_Display;

// Variable names use the gs_ prefix; a ":Section" suffix of a setting name is spelled _Section.
extern SettingT<GameSettingCollection> gs_fGrenadeHighArcSpeedPercentage;
extern SettingT<GameSettingCollection> gs_fAVDCarryWeightsBase;
extern SettingT<GameSettingCollection> gs_fSpeechDelay;
extern SettingT<GameSettingCollection> gs_fTreeFarDistanceBase;
extern SettingT<GameSettingCollection> gs_iUpdateGroups;
extern SettingT<INIPrefSettingCollection> gs_iKillCamera_GamePlay;
extern SettingT<GameSettingCollection> gs_iHoursToRespawnCell;

// The lip distance and the general-subtitle switch of the dialogue sound callback.
extern SettingT<INIPrefSettingCollection> gs_bGeneralSubtitles_Gameplay;

namespace LOD {
    namespace iFadeNodeMinNearDistance {
        void Set(int32_t aiValue);
        uint32_t Get();
    } // namespace iFadeNodeMinNearDistance
} // namespace LOD
