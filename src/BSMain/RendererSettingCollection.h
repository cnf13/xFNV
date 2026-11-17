#pragma once

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"

#include <stdint.h>

// The renderer's INI settings. It adds no members to INISettingCollection, only its own
// destructor; every other slot is the base's.
class RendererSettingCollection : public INISettingCollection {
public:
    RendererSettingCollection();
    ~RendererSettingCollection() override;

    static void operator delete(void* apMemory);
};

// SettingT<RendererSettingCollection>::pCollection, the template's per-collection static, which SettingT does not
// model.
inline RendererSettingCollection* SettingT_RendererSettingCollection_pCollection;
