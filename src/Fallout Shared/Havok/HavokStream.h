#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"

// Shuts the Havok system down at the end of the game.
void KillHavokSystem();
void hkDeallocateChars(void* apPtr);
void destructElements(int32_t a1, int32_t a2);
uint32_t insert(void* apThis, [[maybe_unused]] void* apEdx, uint32_t key, uint32_t val);
