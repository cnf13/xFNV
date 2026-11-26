#pragma once

#include "BSMain/Setting.h"

#include <stdint.h>

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
