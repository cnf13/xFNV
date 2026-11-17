#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class GameSettingCollection;
template <class T> class NiTPrimitiveArray;
class TESFile;

// The warning printer the setting collections report duplicate keys through: it passes 1, the format and its stack
// arguments as the va_list to SettingWarningV and returns that (cdecl, C-variadic, 0xC335A0).
int32_t SettingWarning(const char* apMessage, ...);
// The va_list form, compiled empty in the shipped exe: it returns 0 (cdecl, 0xC3BE40). apArgs
// is the 32-bit va_list, a pointer to the first variadic argument.
int32_t SettingWarningV(int32_t aiType, const char* apMessage, char* apArgs);

// Base of the setting stores (game settings, INI files): a backing file name and the open file handle.
// Root of the collection hierarchy, which has a vtable on PC: slots 0 to 9 (the destructor and nine pure slots).
class SettingCollection {
public:
	SettingCollection();
	virtual ~SettingCollection();

	// Adds apSetting to the store (vtable slot 1; _purecall in this base).
	virtual void Add(Setting* apSetting) = 0;
	// Removes apSetting from the store (vtable slot 2; _purecall in this base).
	virtual void Remove(Setting* apSetting) = 0;
	// Fills apStrings with the settings' viewer strings (vtable slot 3; _purecall in this base).
	virtual uint32_t GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) = 0;
	// Writes rSetting to the store (vtable slot 4; _purecall in this base).
	virtual bool WriteSetting(Setting& rSetting) = 0;
	// Reads rSetting from the store (vtable slot 5; _purecall in this base).
	virtual bool ReadSetting(Setting& rSetting) = 0;
	// Opens the store (vtable slot 6): this base's opens nothing and returns false.
	virtual bool Open(bool abWrite);
	// Closes the store (vtable slot 7): this base's returns true.
	virtual bool Close();
	// Writes every setting (vtable slot 8): this base's only says whether the store is open (pHandle not null;
	// thiscall, ret 0). Its forwarder stays: the derived WriteSettings call it qualified.
	virtual bool WriteSettings();
	// Reads every setting (vtable slot 9): this base's only says whether the store is open
	// (pHandle not null; thiscall, ret 0, 0x404D30, the body of slots 8 and 9; IDA's
	// SettingCollection<Setting>::WriteReadSetting).
	virtual bool ReadSettings();

	// Reads every setting from the file apFileName, under the game setting memory context; false when it does not
	// open (SettingCollection<Setting>::ReadSettingsFromFile).
	bool ReadSettingsFromFile(const char* apFileName);
	// Writes every setting to the collection's own file: opens pSettingFile, runs the virtual WriteSettings and
	// closes it; false when there is no file name or it does not open.
	bool WriteSettingsToFile();
	// Copies apFileName into pSettingFile; null clears it (SettingCollection<Setting>::SetFileName).
	void SetFileName(const char* apFileName);
	// Sets the file path to apPath and opens it through the virtual Open (slot 6) with
	// abWrite (thiscall, ret 8, 0x5E0830).
	// Inferred type: apPath: char* from 0x5e024e (parameter 0 of SettingCollection::ReadSettingsFromFile)
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
	// Writes every setting through the virtual WriteSetting (thiscall, 0x44F950; the INI and registry collections
	// share it).
	bool WriteSettings() override;
	// Reads every setting through the virtual ReadSetting under the game setting memory
	// context (thiscall, 0x44F9C0; the INI and registry collections share it).
	bool ReadSettings() override;
	~SettingCollectionList() override;

	// The list of settings.
	BSSimpleList<Setting*>* Head();
	// The setting named apID, or null (SettingCollectionList<Setting>::GetPtr; thiscall,
	// ret 4, 0x5E02B0).
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

	// Inferred type: a2: char* from 0x5e01be (passed to SettingCollection::OpenPath 0)
	uint8_t SaveAll(char* a2);

	// Copies the setting's INI section name into apMainKey.
	static void MainKey(Setting& arSetting, char* apMainKey);
	// Copies the setting's INI key name into apSubKey.
	static void SubKey(Setting& arSetting, char* apSubKey);
};

inline constexpr fnvd::Vtable SettingCollection_Setting_vftable{};
inline constexpr fnvd::Vtable SettingCollectionList_Setting_vftable{};
inline constexpr fnvd::Vtable INISettingCollection_vftable{};

// SettingT<INISettingCollection>::pCollection, the template's per-collection static that src's SettingT does not
// model.
inline INISettingCollection* SettingT_INISettingCollection_pCollection;

// SettingT<GameSettingCollection>::pCollection, the template's per-collection static that src's SettingT does not
// model. The class is defined in BSMain/SettingCollectionMap.h, after its base.
inline GameSettingCollection* SettingT_GameSettingCollection_pCollection;
