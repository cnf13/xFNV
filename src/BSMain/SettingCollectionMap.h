#pragma once

#include "BSMain/SettingCollection.h"
#include "BSMain/BSTCaseInsensitiveStringMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTStringMap.h"
#include "Reconstruction.hpp"

// The map of settings by name that a setting collection keeps after SettingCollection's fields (the PDB's is the
// template SettingCollectionMap<Setting>). IDA-only instance.
class SettingCollectionMap : public SettingCollection {
public:
	SettingCollectionMap(uint32_t aiMapSize);

	// Fills apStrings with every setting's viewer string; returns how many were added
	// (SettingCollectionMap<Setting>::GetViewerStrings, vtable slot 3; thiscall, ret 4, 0x404FD0).
	uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) override;
	// Adds apSetting to the map under its key; a key already there is reported through
	// SettingWarning (vtable slot 1; thiscall, ret 4, 0x404DE0).
	void Add(Setting* apSetting) override;
	// Removes apSetting's key from the map (vtable slot 2; thiscall, ret 4, 0x404F90).
	void Remove(Setting* apSetting) override;
	// Writes every setting of the map through the virtual WriteSetting (vtable slot 8;
	// thiscall, ret 0, 0x4050A0).
	bool WriteSettings() override;
	// Reads every setting of the map through the virtual ReadSetting, under the game setting
	// memory context (vtable slot 9; thiscall, ret 0, 0x405120).
	bool ReadSettings() override;
	~SettingCollectionMap() override;

	// The map of settings by name, without regard to case: the exe constructs a
	// BSTCaseInsensitiveStringMap<Setting*> here (0x404BDD), so `setgs ixpbase` finds iXPBase.
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
	// Reads the setting apKey from the record apFile is on (vtable slot 10, the class's own
	// virtual; thiscall, ret 8).
	virtual bool ReadSetting(TESFile* apFile, const char* apKey);

	bool GetSetting(const char* apKey, Setting** appSetting);
	// The setting named apID, or null (the exe's SettingCollectionMap<Setting>::GetPtr; thiscall, ret 4, 0x4F8A30).
	Setting* GetPtr(const char* apID);

	static inline uint32_t iMapSize = 401;
};
inline constexpr fnvd::Vtable GameSettingCollection_vftable{};

inline constexpr fnvd::Vtable SettingCollectionMap_Setting_vftable{};
