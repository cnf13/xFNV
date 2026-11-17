#pragma once

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <stdint.h>

int32_t GetRegValueType(Setting* a1);

// The registry key handle (Win32's HKEY): an opaque pointer to this one-int struct.
struct HKEY__ {
    int32_t unused;
};

// The setting collection kept in the Windows registry; it adds no data to the list collection. It overrides the
// collection's virtual functions with its own registry versions.
class RegSettingCollection : public SettingCollectionList {
public:
    RegSettingCollection();

    uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) override;
    bool WriteSetting(Setting& arSetting) override;
    bool ReadSetting(Setting& arSetting) override;
    bool Open(bool bWrite) override;
    bool Close() override;

    static void Init();
};

// SettingT<RegSettingCollection>::pCollection, the template's per-collection static, kept as a free variable because
// SettingT does not model it.
inline RegSettingCollection* SettingT_RegSettingCollection_pCollection;
