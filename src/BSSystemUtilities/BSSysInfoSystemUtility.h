#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

// The platform's system information: the language and the gamepad button assignment.
class BSSysInfoSystemUtility {
public:
	// Values from the Xbox PDB.
	enum LANGUAGE : int32_t {
		LANGUAGE_JAPANESE = 0x0,
		LANGUAGE_ENGLISH = 0x1,
		LANGUAGE_FRENCH = 0x2,
		LANGUAGE_SPANISH = 0x3,
		LANGUAGE_GERMAN = 0x4,
		LANGUAGE_ITALIAN = 0x5,
		LANGUAGE_DUTCH = 0x6,
		LANGUAGE_PORTUGUESE = 0x7,
		LANGUAGE_RUSSIAN = 0x8,
		LANGUAGE_KOREAN = 0x9,
		LANGUAGE_CHINESE_TRADITIONAL = 0xa,
		LANGUAGE_CHINESE_SIMPLIFIED = 0xb,
		LANGUAGE_FINNISH = 0xc,
		LANGUAGE_SWEDISH = 0xd,
		LANGUAGE_DANISH = 0xe,
		LANGUAGE_NORWEGIAN = 0xf,
		LANGUAGE_UNKNOWN = 0x10,
		MAX_LANGUAGES = 0x11,
	};

	BSSysInfoSystemUtility();
	virtual ~BSSysInfoSystemUtility();

	virtual void Initialize();
	virtual void Destroy();

	static void operator delete(void* apMemory);
	// IDA name; the PDB's BSSysInfoSystemUtility::QInstance. The utility, built in a guarded static on first use
	// (cdecl, ret 0, 0xAF3850).
	static BSSysInfoSystemUtility* GetSingleton();

	LANGUAGE eLanguage;
	uint32_t uiGamepadButtonAssignment[16];
};
inline constexpr fnvd::Vtable BSSysInfoSystemUtility_vftable{};
