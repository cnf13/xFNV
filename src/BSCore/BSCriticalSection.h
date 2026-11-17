#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Reconstruction.hpp"

// The engine's critical section. On PC it holds only the Win32 section: Enter (0x4538A0) enters it at offset 0
// through the EnterCriticalSection import. The Xbox PDB's owner and lock-count fields are absent (Tile's static
// at 0x11F3330 is 0x18 bytes).
class BSCriticalSection {
public:
	BSCriticalSection();
	~BSCriticalSection();

	// Takes the lock through Lock; apName (a debug name) is passed on. thiscall, ret 4.
	void Enter(const char* apName);
	// Releases the lock through Unlock. thiscall, ret 0.
	void Leave();
	// Enters the section; apName is unused on PC. thiscall, ret 4, 0x4538E0.
	void Lock(const char* apName);
	// Enters the section. thiscall, ret 0, 0x82F1B0 (an out-of-line copy shared with
	// NiCriticalSection::Lock).
	void Lock();
	// Leaves the section. thiscall, ret 0, 0x82F1F0 (an out-of-line copy shared with
	// NiCriticalSection::Unlock).
	void Unlock();
	// Takes the lock when it is free; true when taken. thiscall, ret 0, 0x44B130.
	bool TryLock();

	// The Win32 section (0x18 bytes on x86), handed to the critical-section imports as is.
	_RTL_CRITICAL_SECTION CriticalSection;
};
