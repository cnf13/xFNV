#pragma once

#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// Inferred type: a1: Setting* from 0xc5d825 (parameter 0 of RegSettingCollection::ReadSetting), its members used at
// 0xc5da23
int32_t GetRegValueType(Setting* a1);

// The SDK's registry key handle (Win32's HKEY): an opaque pointer to this one-int struct. The PDB and IDA both give
// struct HKEY__ { int unused; }, 4 bytes; the exe's ADVAPI32 imports take it as the key pointer (RegSetValueExA
// through 0xFDF010 in RegSettingCollection::WriteSetting 0xC5D670, RegCloseKey in Close 0xC5D9B0).
struct HKEY__ {
	int32_t unused;
};

// The setting collection kept in the Windows registry; it adds no data to the list collection.
// Its destructor is the INI collection's; it overrides slots 3 to 7 with its own registry versions.
class RegSettingCollection : public SettingCollectionList {
public:
	RegSettingCollection();

	// Thiscall, ret 4, 0xC5D610.
	uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) override;
	// Thiscall, ret 4, 0xC5D670.
	bool WriteSetting(Setting& arSetting) override;
	// Thiscall, ret 4, 0xC5D780.
	bool ReadSetting(Setting& arSetting) override;
	// Thiscall, ret 4, 0xC5D920.
	bool Open(bool bWrite) override;
	// Thiscall, 0xC5D9B0.
	bool Close() override;

	static void Init();
};

inline constexpr fnvd::Vtable RegSettingCollection_vftable{};

// SettingT<RegSettingCollection>::pCollection, the template's per-collection static that src's SettingT does not
// model.
inline RegSettingCollection* SettingT_RegSettingCollection_pCollection;

// The registry settings' constructor (vtable 0x10839AC). Defined in XRegSetting.cpp.
inline constexpr fnvd::Vtable SettingT_RegSettingCollection_vftable{};
