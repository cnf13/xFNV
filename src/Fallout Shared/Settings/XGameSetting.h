#pragma once

#include "BSMain/Setting.h"

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

inline GameSettingCollection::GameSettingCollection() : SettingCollectionMap(GameSettingCollection::iMapSize) {}
