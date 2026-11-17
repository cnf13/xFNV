#pragma once

#include "Reconstruction.hpp"

// Names the thread dwThreadID for the debugger (the thread-naming exception 0x406D1388).
void SetThreadName(uint32_t dwThreadID, const char* szThreadName);
