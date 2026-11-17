#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

int32_t return_0_2();

// One downloadable content package BSDLCSystemUtility::LoadDLC (0xAF2940) found: 8 bytes on the PC (the exe's
// allocation there), the package's display name and a root name.
class BSContentPackage {
public:
	// The PC constructor (thiscall, ret 4, 0xB008B0): stores apDisplayName, the DLC table entry's wide name LoadDLC
	// passes, and returns apMemory. PDB: BSContentPackage(const char* root, const _XCONTENT_DATA& pkg); the PC exe
	// takes one argument.
	BSContentPackage(const char16_t* apDisplayName);
	// The PC body does nothing.
	~BSContentPackage();

	// The display name.
	const char16_t* DisplayName() const;
	// The package's file name (thiscall, ret 0, 0xB008E0; the PDB's `const char* FileName() const`, the public the
	// Xbox build's callers use next to DisplayName). The PC body returns the empty string at 0x10A47B3.
	const char* FileName() const;

	const char16_t* m_contentData;
	char* m_rootName;
};
