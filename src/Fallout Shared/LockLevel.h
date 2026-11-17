#pragma once

#include "Reconstruction.hpp"

// How hard a lock or a terminal is to open (values from the Xbox PDB; IDA spells it LockLevel, kLockLevel_*).
enum LOCK_LEVEL : int32_t {
	LL_VERYEASY = 0x0,
	LL_EASY = 0x1,
	LL_AVERAGE = 0x2,
	LL_HARD = 0x3,
	LL_VERYHARD = 0x4,
	LL_IMPOSSIBLE = 0x5,
	LL_COUNT = 0x6,
};
