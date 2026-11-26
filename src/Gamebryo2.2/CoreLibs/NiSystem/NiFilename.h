#pragma once

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"

// strcat_s of pcSrc onto pcDest of stDestSize bytes; returns pcDest.
char* NiStrcat(char* pcDest, size_t stDestSize, const char* pcSrc);

// A file path split into drive, directory, sub-directory, file name and extension.
class NiFilename : public NiMemObject {
public:
    NiFilename() = default;
    NiFilename(const char* pcFullPath);

    // Builds the path of the parts into pcStr; false when pcStr is null or stStrLen is too small.
    bool Makepath(char* pcStr, size_t stStrLen);
    bool GetFullPath(char* pcFullPath, size_t uiStrLen);

    char m_acDir[256];
    char m_acDrive[3];
    char m_acExt[256];
    char m_acFname[256];
    char m_acSubDir[256];
};
