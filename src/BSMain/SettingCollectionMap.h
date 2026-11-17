#pragma once

#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"

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

// The game settings, kept in the map of SettingCollectionMap.
class GameSettingCollection : public SettingCollectionMap {
public:
    GameSettingCollection();

    // The map's GetViewerStrings.
    uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) override;
    // A stub that returns without writing.
    bool WriteSetting(Setting& arSetting) override;
    // Reads rSetting from the DATA chunk of the record the collection's file is on.
    bool ReadSetting(Setting& rSetting) override;
    bool Open(bool abWrite) override;
    bool Close() override;
    // Reads the setting apKey from the record apFile is on.
    virtual bool ReadSetting(TESFile* apFile, const char* apKey);

    bool GetSetting(const char* apKey, Setting** appSetting);
    // The setting named apID, or null.
    Setting* GetPtr(const char* apID);

    static inline uint32_t iMapSize = 401;
};
