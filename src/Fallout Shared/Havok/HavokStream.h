#pragma once

#include "Havok/Source/Common/Base/Container/PointerMap/hkPointerMap.h"
#include "Reconstruction.hpp"

// Shuts the Havok system down at the end of the game.
void KillHavokSystem();
// Returns nothing: eax is what easyFree left.
void hkDeallocateChars(void* apPtr);
void destructElements(int32_t a1, int32_t a2);
// The exe calls it thiscall and its class is not known yet: the object comes first, then the unused edx.
uint32_t insert(void* apThis, [[maybe_unused]] void* apEdx, uint32_t key, uint32_t val);
