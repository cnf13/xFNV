#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"

#include <stdint.h>

class TESFile;

template <class T>
class NiTPrimitiveArray;

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

class INISettingCollection : public SettingCollectionList {
public:
    INISettingCollection();

    // Returns the count of viewer strings it adds.
    uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) override;
    // Writes rSetting to the INI file.
    bool WriteSetting(Setting& arSetting) override;
    // Reads rSetting from the INI file.
    bool ReadSetting(Setting& arSetting) override;
    bool Open(bool abWrite) override;
    bool Close() override;

    uint8_t SaveAll(char* apPath);

    // Copies the setting's INI section name into apMainKey.
    static void MainKey(Setting& arSetting, char* apMainKey);
    // Copies the setting's INI key name into apSubKey.
    static void SubKey(Setting& arSetting, char* apSubKey);
};
