#pragma once

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"

#include <stdint.h>

// The INI setting collection of the preference settings; it adds no data to the INI collection.
class INIPrefSettingCollection : public INISettingCollection {
public:
    INIPrefSettingCollection();
    ~INIPrefSettingCollection() override;
};
