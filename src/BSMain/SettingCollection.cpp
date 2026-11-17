#include "BSMain/SettingCollection.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollectionMap.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESFile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"
#include "platform/Ini.h"

#include <cstdarg>

int32_t SettingWarning(const char* apMessage, ...) {
	va_list args;
	va_start(args, apMessage);
	// SettingWarningV takes the exe's untyped 32-bit list pointer.
	int32_t iResult = SettingWarningV(1, apMessage, reinterpret_cast<char*>(args));
	va_end(args);
	return iResult;
}

int32_t SettingWarningV(int32_t aiType, const char* apMessage, char* apArgs) {
	return 0;
}

// Closes the store through this base's own Close.
SettingCollection::~SettingCollection() {
	SettingCollection::Close();
}

bool SettingCollection::Open(bool abWrite) {
	return false;
}

bool SettingCollection::Close() {
	return true;
}

// The exe's WriteSettingsToFile: opens pSettingFile for writing, runs the virtual WriteSettings and closes the
// file (thiscall, ret 0, 0x8798E0).
bool SettingCollection::WriteSettingsToFile() {
	bool bResult = false;
	char* pPath = pSettingFile;
	if (pPath) {
		bResult = OpenPath(pPath, true);
		if (bResult) {
			bResult = WriteSettings();
			Close();
		}
	}
	return bResult;
}

bool SettingCollection::OpenPath(const char* apPath, bool abWrite) {
	SetFileName(apPath);
	return (bool)((uint8_t)Open(abWrite));
}

void SettingCollection::SetFileHandle(void* apHandle) {
	SetFileName(0);
	pHandle = apHandle;
}

void SettingCollection::operator delete(void* apMemory) {
	ourFree(apMemory);
}

SettingCollectionList::SettingCollectionList() {}

// The exe's cleanup-only C++ EH frame is left out.
uint32_t SettingCollectionList::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_GAMESETTING, 1,
		"d:\\_fallout3\\platforms\\common\\code\\bsmain\\Setting.h", 806);
	BSSimpleList<Setting*>* pcur = Head();
	uint32_t I = 0;
	while (pcur) {
		Setting** v2 = &pcur->m_item;
		(*v2)->GetViewerStrings(apStringsA);
		pcur = Next(pcur);
		++I;
	}
	return I;
}

// The exe's cleanup-only C++ EH frame is left out.
void SettingCollectionList::Add(Setting* apSetting) {
	FNVD_INLINED_LOCAL(AutoMemContext, tempMemContext, std::nullopt, std::nullopt);
	::new (&tempMemContext)
		AutoMemContext(MEM_CONTEXT_GAMESETTING, 1, "d:\\_fallout3\\platforms\\common\\code\\bsmain\\Setting.h", 765);
	if (!SettingsA.IsEmpty()) {
		for (BSSimpleList<Setting*>* pIter = &SettingsA; pIter; pIter = pIter->m_pkNext) {
			Setting** v2 = &pIter->m_item;
			if (!BSstricmp((*v2)->pKey, apSetting->pKey)) {
				SettingWarning("DEFAULT: Setting key '%s' already used in list.\nSetting keys must be unique.\n",
					apSetting->pKey);
				tempMemContext.~AutoMemContext();
				return;
			}
		}
	}
	SettingsA.AddHead(apSetting);
	tempMemContext.~AutoMemContext();
}

// The exe calls BSSimpleList<void*>::Remove (0x905330) out of line with the address of the argument slot.
void SettingCollectionList::Remove(Setting* apSetting) {
	SettingsA.Remove(apSetting);
}

// The exe calls WriteSetting through vtable slot 4 (offset 0x10) on this; the result is anded into bres.
bool SettingCollectionList::WriteSettings() {
	bool bres = SettingCollection::ReadSettings();
	if (bres) {
		for (BSSimpleList<Setting*>* pcur = Head(); pcur; pcur = Next(pcur)) {
			bres &= WriteSetting(*pcur->m_item);
		}
	}
	return bres;
}

// The exe's cleanup-only C++ EH frame is left out.
bool SettingCollectionList::ReadSettings() {
	FNVD_CONSTRUCTED_LOCAL(AutoMemContext, tempMemContext, &AutoMemContext::Destruct, MEM_CONTEXT_GAMESETTING, 1,
		"d:\\_fallout3\\platforms\\common\\code\\bsmain\\Setting.h", 855);
	bool bres = SettingCollection::ReadSettings();
	if (bres) {
		for (BSSimpleList<Setting*>* pcur = Head(); pcur; pcur = Next(pcur)) {
			if (pcur->m_item) {
				// The virtual ReadSetting (vtable +0x14); the exe zero-extends its al.
				bres &= ReadSetting(*pcur->m_item);
			}
		}
	}
	return bres;
}

// The list of settings is emptied by its own destructor.
SettingCollectionList::~SettingCollectionList() {}

BSSimpleList<Setting*>* SettingCollectionList::Head() {
	return &SettingsA;
}

Setting* SettingCollectionList::GetPtr(const char* apID) {
	Setting* arpGS = 0;
	Get(apID, arpGS);
	return arpGS;
}

BSSimpleList<Setting*>* SettingCollectionList::Next(BSSimpleList<Setting*>* apList) {
	return apList->m_pkNext;
}

bool SettingCollectionList::Get(const char* apID, Setting*& arpGS) {
	if (!apID) {
		return 0;
	}
	for (BSSimpleList<Setting*>* ecx0 = &SettingsA; ecx0 && !ecx0->IsEmpty(); ecx0 = ecx0->m_pkNext) {
		Setting* v4 = ecx0->m_item;
		if (!BSstricmp(v4->pKey, apID)) {
			arpGS = v4;
			return 1;
		}
	}
	return 0;
}

INISettingCollection::INISettingCollection() {}

uint32_t INISettingCollection::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
	char* element = NiGetViewerString("INISettings");
	apStringsA->Add(element);
	uint32_t I = SettingCollectionList::GetViewerStrings(apStringsA);
	char* ViewerString_6 = NiGetViewerString("Total INISettings", I);
	apStringsA->Add(ViewerString_6);
	return I;
}

// Opens the INI file for the write when it is not open (and closes it again after), then writes the value under
// the setting's section and name in the format of its type. The exe's switch biases the type by 1: ST_UCHAR,
// ST_UINT and anything past ST_RGBA take the "%u" default.
bool INISettingCollection::WriteSetting(Setting& arSetting) {
	char AppName[64];
	char KeyName[68];
	char String[256];
	bool bResult = false;
	if (arSetting.pKey) {
		bool bOpened = pHandle == nullptr;
		if (bOpened) {
			Open(true);
		}
		if (pHandle) {
			int32_t iType = Setting::DataType(arSetting.pKey);
			MainKey(arSetting, AppName);
			SubKey(arSetting, KeyName);
			uint32_t uiValue;
			switch (iType) {
			case Setting::ST_STRING:
				// The exe tests the setting's address before it reads the string (an inlined null-safe accessor);
				// a reference is never null, so src reads the string directly.
				bResult = platform::IniWriteString(pSettingFile, AppName, KeyName, arSetting.uValue.str);
				break;
			case Setting::ST_CHAR:
			case Setting::ST_INT:
				BSsprintf(String, 0x100, "%d", arSetting.uValue.i);
				bResult = platform::IniWriteString(pSettingFile, AppName, KeyName, String);
				break;
			case Setting::ST_FLOAT:
				BSsprintf(String, 0x100, "%.4f", static_cast<double>(arSetting.uValue.f));
				bResult = platform::IniWriteString(pSettingFile, AppName, KeyName, String);
				break;
			case Setting::ST_RGB:
				uiValue = arSetting.uValue.uint;
				BSsprintf(String, 0x100, "%u,%u,%u", (uiValue >> 24) & 0xFF, (uiValue >> 16) & 0xFF,
					(uiValue >> 8) & 0xFF);
				bResult = platform::IniWriteString(pSettingFile, AppName, KeyName, String);
				break;
			case Setting::ST_RGBA:
				uiValue = arSetting.uValue.uint;
				BSsprintf(String, 0x100, "%u,%u,%u,%u", (uiValue >> 24) & 0xFF, (uiValue >> 16) & 0xFF,
					(uiValue >> 8) & 0xFF, uiValue & 0xFF);
				bResult = platform::IniWriteString(pSettingFile, AppName, KeyName, String);
				break;
			default:
				BSsprintf(String, 0x100, "%u", arSetting.uValue.uint);
				bResult = platform::IniWriteString(pSettingFile, AppName, KeyName, String);
				break;
			}
		}
		if (bOpened) {
			Close();
		}
	}
	return bResult;
}

// Opens the INI file for the read when it is not open (and closes it again after), then reads the value under the
// setting's section and name in the format of its type; returns whether a float, RGB or RGBA value parsed whole.
// The exe tests the setting's address before each access (inlined null-safe accessors that fall back to
// Setting::DefaultValue); a reference is never null, so src reads the value directly.
// KEEP: the exe reads uninitialised stack: the ST_RGB and ST_RGBA cases build the colour from all of sscanf's
// outputs and store it (0xC446B3, 0xC44797) whatever sscanf returned, so a value with fewer fields reads stack
// garbage (only the ==3 / ==4 result byte depends on the count).
bool INISettingCollection::ReadSetting(Setting& arSetting) {
	bool bResult = false;
	if (arSetting.Key()) {
		bool bOpened = pHandle == nullptr;
		if (bOpened) {
			Open(true);
		}
		if (pHandle) {
			char MainKeyName[64];
			char SubKeyName[68];
			char Value[256];
			uint32_t uiRead;
			Setting::SETTING_TYPE eType = Setting::DataType(arSetting.Key());
			INISettingCollection::MainKey(arSetting, MainKeyName);
			INISettingCollection::SubKey(arSetting, SubKeyName);
			switch (eType) {
			case Setting::ST_BINARY:
				// The exe passes the stored byte made 0 or 1 as the default.
				arSetting.uValue.b =
					platform::IniGetInt(pSettingFile, MainKeyName, SubKeyName, arSetting.uValue.uc != 0) != 0;
				break;
			case Setting::ST_CHAR:
				// The exe passes the whole value dword as the default and stores the result's low byte.
				arSetting.uValue.uc = static_cast<uint8_t>(platform::IniGetInt(pSettingFile, MainKeyName, SubKeyName,
					arSetting.uValue.i));
				break;
			case Setting::ST_INT:
				arSetting.uValue.uint = platform::IniGetInt(pSettingFile, MainKeyName, SubKeyName, arSetting.uValue.i);
				break;
			case Setting::ST_FLOAT: {
				char Default[260];
				BSsprintf(Default, 256, "%f", static_cast<double>(arSetting.uValue.f));
				uiRead =
					platform::IniGetString(pSettingFile, MainKeyName, SubKeyName, Default, std::span<char>(Value, 256));
				if (uiRead) {
					bResult = std::sscanf(Value, "%f", &arSetting.uValue.f) == 1;
				}
				break;
			}
			case Setting::ST_STRING: {
				char Language[1024];
				char String[1024];
				if (!std::strcmp(MainKeyName, "LANGUAGE")) {
					uiRead = platform::IniGetString(pSettingFile, "General", "sLanguage", sLanguage.uValue.str,
						std::span<char>(Language, 256));
					std::strcpy(MainKeyName, Language);
				}
				uiRead = platform::IniGetString(pSettingFile, MainKeyName, SubKeyName, arSetting.uValue.str,
					std::span<char>(String, 256));
				arSetting = static_cast<const char*>(String);
				break;
			}
			case Setting::ST_RGB: {
				uint32_t uiRed;
				uint32_t uiGreen;
				uint32_t uiBlue;
				uiRead = platform::IniGetString(pSettingFile, MainKeyName, SubKeyName, "", std::span<char>(Value, 256));
				if (uiRead) {
					bResult = std::sscanf(Value, "%u,%u,%u", &uiRed, &uiGreen, &uiBlue) == 3;
					arSetting.uValue.uint = uiRed << 24 | (uiGreen & 0xFF) << 16 | (uiBlue & 0xFF) << 8 | 0xFF;
				}
				break;
			}
			case Setting::ST_RGBA: {
				uint32_t uiRed;
				uint32_t uiGreen;
				uint32_t uiBlue;
				uint32_t uiAlpha;
				uiRead = platform::IniGetString(pSettingFile, MainKeyName, SubKeyName, "", std::span<char>(Value, 256));
				if (uiRead) {
					bResult = std::sscanf(Value, "%u,%u,%u,%u", &uiRed, &uiGreen, &uiBlue, &uiAlpha) == 4;
					arSetting.uValue.uint =
						uiRed << 24 | (uiGreen & 0xFF) << 16 | (uiBlue & 0xFF) << 8 | (uiAlpha & 0xFF);
				}
				break;
			}
			default:
				arSetting.uValue.uint = platform::IniGetInt(pSettingFile, MainKeyName, SubKeyName, arSetting.uValue.i);
				break;
			}
		}
		if (bOpened) {
			Close();
		}
	}
	return bResult;
}

bool INISettingCollection::Open(bool abWrite) {
	pHandle = this;
	return true;
}

bool INISettingCollection::Close() {
	pHandle = nullptr;
	return true;
}

uint8_t INISettingCollection::SaveAll(char* a2) {
	uint8_t v4 = (uint8_t)SettingCollection::OpenPath(a2, 1);
	if (v4) {
		v4 = (uint8_t)WriteSettings();
		Close();
	}
	return v4;
}

void INISettingCollection::MainKey(Setting& arSetting, char* apMainKey) {
	const char* pcSrc = "MAIN";
	if (apMainKey) {
		const char* Str = arSetting.pKey;
		if (Str) {
			const char* v2 = std::strchr(Str, 58);
			if (v2) {
				pcSrc = v2 + 1;
			}
		}
		strcpy_s(apMainKey, 64, pcSrc);
	}
}

void INISettingCollection::SubKey(Setting& arSetting, char* apSubKey) {
	if (apSubKey) {
		*apSubKey = 0;
		const char* Str = arSetting.pKey;
		if (Str) {
			size_t v2;
			const char* v3 = std::strchr(Str, 58);
			if (v3) {
				v2 = v3 - Str;
			} else {
				v2 = std::strlen(Str);
			}
			strncpy_s(apSubKey, v2 + 1, Str, v2);
			apSubKey[v2] = 0;
		}
	}
}

GameSettingCollection::GameSettingCollection() : SettingCollectionMap(GameSettingCollection::iMapSize) {}

uint32_t GameSettingCollection::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
	char* element = NiGetViewerString("GameSettings");
	apStringsA->Add(element);
	uint32_t I = SettingCollectionMap::GetViewerStrings(apStringsA);
	char* ViewerString_6 = NiGetViewerString("Total GameSettings", I);
	apStringsA->Add(ViewerString_6);
	return I;
}

bool GameSettingCollection::WriteSetting(Setting& arSetting) {
	return false;
}

bool GameSettingCollection::Open(bool abWrite) {
	return (pHandle != 0);
}

bool GameSettingCollection::Close() {
	pHandle = nullptr;
	return true;
}

bool GameSettingCollection::ReadSetting(TESFile* apFile, const char* apKey) {
	bool bres = 0;
	SetFileHandle(apFile);
	Setting* psetting = GetPtr(apKey);
	if (psetting) {
		bres = ReadSetting(*psetting);
	}
	Close();
	return bres;
}

bool GameSettingCollection::GetSetting(const char* apKey, Setting** appSetting) {
	return Settings.GetAt(apKey, *appSetting);
}

Setting* GameSettingCollection::GetPtr(const char* apID) {
	Setting* appSetting;
	appSetting = 0;
	GetSetting(apID, &appSetting);
	return appSetting;
}
