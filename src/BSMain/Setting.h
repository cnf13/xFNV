#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"

#include <stdint.h>

class BlendSettingCollection;
class GameSettingCollection;
class INIPrefSettingCollection;
class INISettingCollection;
template <class T>
class NiTPrimitiveArray;
class RegSettingCollection;
class RendererSettingCollection;
class TESFile;

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
    // Each constructor stores the key and value, then registers the setting in its collection.
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

template <class T>
SettingT<T>::SettingT(const char* apID, Info aValue) : Setting(apID, aValue) {
    GetCollection()->Add(this);
}

template <class T>
SettingT<T>::SettingT(const char* apID, bool abValue) : Setting(apID, abValue) {
    GetCollection()->Add(this);
}

template <class T>
SettingT<T>::SettingT(const char* apID, float afValue) : Setting(apID, afValue) {
    GetCollection()->Add(this);
}

template <class T>
SettingT<T>::SettingT(const char* apID, int32_t aiValue) : Setting(apID, Info{.i = aiValue}) {
    GetCollection()->Add(this);
}

template <class T>
SettingT<T>::SettingT(const char* apID, uint32_t aiValue) : Setting(apID, Info{.uint = aiValue}) {
    GetCollection()->Add(this);
}

template <class T>
SettingT<T>::SettingT(const char* apID, const char* apString)
  : Setting(apID, Info{.str = const_cast<char*>(apString)}) {
    GetCollection()->Add(this);
}

// Unregisters the setting from its collection.
template <class T>
SettingT<T>::~SettingT() {
    GetCollection()->Remove(this);
}

// Called on the setting's raw storage: does the Info constructor's work (Setting's constructor, SettingT's vtable, the
// registration in the collection) by running that constructor in place.
template <class T>
void SettingT<T>::InitString(const char* apName, Setting::Info aValue) {
    ::new (this) SettingT(apName, aValue);
}

// The collection, created on first use.
template <class T>
T* SettingT<T>::GetCollection() {
    InitCollection();
    return pCollection;
}

// Creates the collection in the engine's heap the first time it is needed.
template <class T>
void SettingT<T>::InitCollection() {
    if (!pCollection) {
        pCollection = new T();
    }
}

template <class T>
SettingT<T>& SettingT<T>::operator=(int32_t aiValue) {
    SetInt(aiValue);
    return *this;
}

template <class T>
SettingT<T>& SettingT<T>::operator=(bool abValue) {
    Setting::operator=(abValue);
    return *this;
}

template <class T>
SettingT<T>& SettingT<T>::operator=(float afValue) {
    Setting::operator=(afValue);
    return *this;
}

template <class T>
SettingT<T>& SettingT<T>::operator=(const char* apString) {
    Setting::operator=(apString);
    return *this;
}

template <class T>
bool SettingT<T>::operator==(bool abValue) {
    return IsEqual(abValue);
}

template <class T>
bool SettingT<T>::operator==(int32_t aiValue) {
    return Setting::operator==(aiValue);
}

// Settings built with SettingT's constructors in their files' dynamic initialisers.
extern SettingT<INISettingCollection> bShowLoadingAreaMessage;
extern SettingT<GameSettingCollection> fCreatureCalcCombat;

extern SettingT<GameSettingCollection> fConfidenceFoolhardy;
extern SettingT<GameSettingCollection> fConfidenceAverage;
extern SettingT<GameSettingCollection> fConfidenceBrave;
extern SettingT<GameSettingCollection> fConfidenceCautious;
extern SettingT<GameSettingCollection> fConfidenceCowardly;

extern SettingT<INISettingCollection> bUseLinear;
extern SettingT<INISettingCollection> bUseQuadratic;
extern SettingT<INISettingCollection> bUseConstant;

extern SettingT<GameSettingCollection> sBetMaxText;

extern SettingT<INISettingCollection> fPulseRate;
extern SettingT<INISettingCollection> fVATSPulseRadius;
extern SettingT<INISettingCollection> fVATSBurstDuration;
extern SettingT<INISettingCollection> fVATSDistortVerticalScale;
extern SettingT<INISettingCollection> fVATSScanDuration;
extern SettingT<INISettingCollection> fVATSBurstIntensity;
extern SettingT<INISettingCollection> iVATSDistortMaxInterval;
extern SettingT<INISettingCollection> fVATSScanVerticalScale;
extern SettingT<INISettingCollection> fVATSPulseIntensity;
extern SettingT<INISettingCollection> fVATSPulseRate;

extern SettingT<GameSettingCollection> fGrenadeHighArcSpeedMult;
extern SettingT<GameSettingCollection> fAVDCarryWeightBase;
extern SettingT<GameSettingCollection> iNumGroups;
extern SettingT<INIPrefSettingCollection> iKillCamera;

// The lip distance and the general-subtitle switch of the dialogue sound callback.
extern SettingT<INIPrefSettingCollection> bGeneralSubtitles;

namespace LOD {
    namespace iFadeNodeMinNearDistance {
        void Set(int32_t aiValue);
        uint32_t Get();
    } // namespace iFadeNodeMinNearDistance
} // namespace LOD

inline uint32_t MakeRGBA(uint32_t aiRed, uint32_t aiGreen, uint32_t aiBlue, uint32_t aiAlpha) {
    return (aiRed << 24) | (static_cast<uint8_t>(aiGreen) << 16) | (static_cast<uint8_t>(aiBlue) << 8) |
        static_cast<uint8_t>(aiAlpha);
}

// The warning printer the setting collections report duplicate keys through: it passes 1, the format and its arguments
// as the va_list to SettingWarningV and returns that.
int32_t SettingWarning(const char* apMessage, ...);
// The va_list form, compiled empty: it returns 0. apArgs is the va_list, a pointer to the first variadic argument.
int32_t SettingWarningV(int32_t aiType, const char* apMessage, char* apArgs);

// Base of the setting stores (game settings, INI files): a backing file name and the open file handle. Root of the
// collection hierarchy.
class SettingCollection {
public:
    SettingCollection();
    virtual ~SettingCollection();

    virtual void Add(Setting* apSetting) = 0;
    // Removes apSetting from the store.
    virtual void Remove(Setting* apSetting) = 0;
    // Fills apStrings with the settings' viewer strings.
    virtual uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) = 0;
    // Writes rSetting to the store.
    virtual bool WriteSetting(Setting& rSetting) = 0;
    // Reads rSetting from the store.
    virtual bool ReadSetting(Setting& rSetting) = 0;
    // Opens the store: this base's opens nothing and returns false.
    virtual bool Open(bool abWrite);
    // Closes the store; this base's version returns true.
    virtual bool Close();
    // Writes every setting: this base's only says whether the store is open (pHandle not null). The derived
    // WriteSettings call it qualified.
    virtual bool WriteSettings();
    // Reads every setting; this base's version only says whether the store is open (pHandle not null).
    virtual bool ReadSettings();

    // Reads every setting from the file apFileName, under the game setting memory context; false when it does not
    // open (SettingCollection<Setting>::ReadSettingsFromFile).
    bool ReadSettingsFromFile(const char* apFileName);
    // Writes every setting to the collection's own file: opens pSettingFile, runs the virtual WriteSettings and
    // closes it; false when there is no file name or it does not open.
    bool WriteSettingsToFile();
    // Copies apFileName into pSettingFile; null clears it (SettingCollection<Setting>::SetFileName).
    void SetFileName(const char* apFileName);
    // Sets the file path to apPath and opens it through the virtual Open with abWrite.
    bool OpenPath(const char* apPath, bool abWrite);
    void SetFileHandle(void* apHandle);

    static void operator delete(void* apMemory);

    char pSettingFile[260];
    void* pHandle;
};

// A collection that keeps its settings in a linked list.
class SettingCollectionList : public SettingCollection {
public:
    SettingCollectionList();

    // SettingCollectionList<Setting>::GetViewerStrings.
    uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) override;
    // SettingCollectionList<Setting>::Add.
    void Add(Setting* apSetting) override;
    // Unlinks apSetting from SettingsA (SettingCollectionList<Setting>::Remove).
    void Remove(Setting* apSetting) override;
    // Writes every setting through the virtual WriteSetting (the INI and registry collections share it).
    bool WriteSettings() override;
    // Reads every setting through the virtual ReadSetting under the game setting memory context; the INI and registry
    // collections share it.
    bool ReadSettings() override;
    ~SettingCollectionList() override;

    // The list of settings.
    BSSimpleList<Setting*>* Head();
    // The setting named apID, or null.
    Setting* GetPtr(const char* apID);
    BSSimpleList<Setting*>* Next(BSSimpleList<Setting*>* apList);
    bool Get(const char* apID, Setting*& arpGS);

    BSSimpleList<Setting*> SettingsA;
};

// The read and the close go through the virtual ReadSettings and Close.
inline bool SettingCollection::ReadSettingsFromFile(const char* apFileName) {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext)
        AutoMemContext(MEM_CONTEXT_GAMESETTING, true, "d:\\_fallout3\\platforms\\common\\code\\bsmain\\Setting.h", 413);
    bool bOpened = OpenPath(apFileName, false);
    if (bOpened) {
        ReadSettings();
        Close();
    }
    pContext->~AutoMemContext();
    return bOpened;
}

inline SettingCollection::SettingCollection() : pHandle(nullptr) {
    pSettingFile[0] = 0;
}

inline bool SettingCollection::WriteSettings() {
    return pHandle != nullptr;
}

inline bool SettingCollection::ReadSettings() {
    return pHandle != nullptr;
}

inline void SettingCollection::SetFileName(const char* apFileName) {
    if (apFileName) {
        BSstrcpy(pSettingFile, 260, apFileName);
    }
    else {
        pSettingFile[0] = 0;
    }
}

// The map of settings by name that a setting collection keeps after SettingCollection's fields.
class SettingCollectionMap : public SettingCollection {
public:
    SettingCollectionMap(uint32_t aiMapSize);

    // Fills apStrings with every setting's viewer string; returns how many were added.
    uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) override;
    // Adds apSetting to the map under its key; a key already there is reported through SettingWarning.
    void Add(Setting* apSetting) override;
    // Removes apSetting's key from the map.
    void Remove(Setting* apSetting) override;
    // Writes every setting of the map through the virtual WriteSetting.
    bool WriteSettings() override;
    // Reads every setting of the map through the virtual ReadSetting, under the game setting memory context.
    bool ReadSettings() override;
    ~SettingCollectionMap() override;

    // The map of settings by name, without regard to case, so `setgs ixpbase` finds iXPBase.
    BSTCaseInsensitiveStringMap<Setting*> Settings;
};

// The settings' viewer strings, one per setting in the collection's map; returns how many were added.
inline uint32_t SettingCollectionMap::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
    AutoMemContext kContext(MEM_CONTEXT_GAMESETTING, true, "..\\BSMain\\Setting.h", 569);
    uint32_t uiCount = 0;
    const char* pKey;
    Setting* pSetting;
    NiTMapItem<const char*, Setting*>* pkPos = Settings.GetFirstPos();
    while (pkPos) {
        Settings.GetNext(pkPos, pKey, pSetting);
        if (pSetting) {
            pSetting->GetViewerStrings(apStringsA);
            ++uiCount;
        }
    }
    return uiCount;
}

inline void SettingCollectionMap::Add(Setting* apSetting) {
    AutoMemContext kContext(MEM_CONTEXT_GAMESETTING, true, "..\\BSMain\\Setting.h", 539);
    if (apSetting && apSetting->pKey) {
        Setting* pExisting;
        if (Settings.GetAt(apSetting->pKey, pExisting)) {
            SettingWarning(
                "DEFAULT: Setting key '%s' already used in map.\nSetting keys must be unique.\n",
                apSetting->pKey
            );
        }
        else {
            Settings.SetAt(apSetting->pKey, apSetting);
        }
    }
}

inline void SettingCollectionMap::Remove(Setting* apSetting) {
    if (apSetting && apSetting->pKey) {
        Settings.RemoveAt(apSetting->pKey);
    }
}

// Writes every setting of the map through the virtual WriteSetting; true when the store is open and every write
// succeeded.
inline bool SettingCollectionMap::WriteSettings() {
    bool bResult = SettingCollection::WriteSettings();
    if (bResult) {
        const char* pKey;
        Setting* pSetting;
        NiTMapItem<const char*, Setting*>* pkPos = Settings.GetFirstPos();
        while (pkPos) {
            Settings.GetNext(pkPos, pKey, pSetting);
            if (pSetting) {
                bResult &= WriteSetting(*pSetting);
            }
        }
    }
    return bResult;
}

// Reads every setting of the map through the virtual ReadSetting, under the game setting memory context; true when the
// store is open and every read succeeded.
inline bool SettingCollectionMap::ReadSettings() {
    AutoMemContext kContext(MEM_CONTEXT_GAMESETTING, true, "..\\BSMain\\Setting.h", 632);
    bool bResult = SettingCollection::ReadSettings();
    if (bResult) {
        const char* pKey;
        Setting* pSetting;
        NiTMapItem<const char*, Setting*>* pkPos = Settings.GetFirstPos();
        while (pkPos) {
            Settings.GetNext(pkPos, pKey, pSetting);
            if (pSetting) {
                bResult &= ReadSetting(*pSetting);
            }
        }
    }
    return bResult;
}
