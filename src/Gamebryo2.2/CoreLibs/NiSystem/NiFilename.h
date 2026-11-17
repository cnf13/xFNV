#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Reconstruction.hpp"

// strcat_s of pcSrc onto pcDest of stDestSize bytes; returns pcDest.
char* NiStrcat(char* pcDest, size_t stDestSize, const char* pcSrc);

// A file path split into drive, directory, sub-directory, file name and extension.
class NiFilename : public NiMemObject {
public:
	// The hybrid's implicit default constructor (no zero-parameter Construct).
	NiFilename() = default;
	NiFilename(const char* pcFullPath);

	// Builds the path of the parts into pcStr; false when pcStr is null or stStrLen is too small.
	bool Makepath(char* pcStr, size_t stStrLen);
	// Same code as Makepath: a jump to 0xAA2820 (thiscall, ret 8, 0xAA29D0).
	bool GetFullPath(char* pcFullPath, size_t uiStrLen);

	char m_acDir[256];
	char m_acDrive[3];
	char m_acExt[256];
	char m_acFname[256];
	char m_acSubDir[256];
};
