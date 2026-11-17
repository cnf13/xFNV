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

// SettingT<INIPrefSettingCollection>::pCollection, the template's per-collection static, which SettingT does not model.
inline INIPrefSettingCollection* SettingT_INIPrefSettingCollection_pCollection;
