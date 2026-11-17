#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

// The localized texts of the system utilities' messages (retry, autosave warning, disk space and corruption
// notices...), eight strings the singleton owns.
class BSSystemUtilitiesStrings {
public:
	// Values from the Xbox PDB.
	enum STRING : int32_t {
		STRING_RETRY = 0x0,
		STRING_AUTOSAVE_WARNING = 0x1,
		STRING_GAME_DATA_INSUFFICIENT_SPACE = 0x2,
		STRING_GAME_DATA_CORRUPT = 0x3,
		STRING_SAVE_DATA_CREATE_NEW = 0x4,
		STRING_SAVE_DATA_INSUFFICIENT_SPACE = 0x5,
		STRING_DISC_EJECT = 0x6,
		STRING_DIRTY_DISC = 0x7,
		NUM_STRINGS = 0x8,
	};

	BSSystemUtilitiesStrings();
	virtual ~BSSystemUtilitiesStrings();

	virtual void Initialize();
	virtual void Destroy();

	void SetString(BSSystemUtilitiesStrings::STRING aeType, const char* apcString);

	static void operator delete(void* apMemory);
	static BSSystemUtilitiesStrings* QInstance();

	char* pcStrings[8];
};
inline constexpr fnvd::Vtable BSSystemUtilitiesStrings_vftable{};
