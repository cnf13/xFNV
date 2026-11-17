#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/string.h"

#include <cctype>

// Standardize without the result flag. The "previous was a backslash" flag starts false.
void NiPath::StandardizeNoResult(char* pcPath) {
    if (pcPath) {
        size_t stLen = strlen(pcPath);
        size_t stDest = 0;
        bool bLastSlash = false;
        for (size_t i = 0; i < stLen; ++i) {
            char c = pcPath[i];
            if (c == '/') {
                c = '\\';
            }
            bool bSlash = c == '\\';
            if (!bSlash || !bLastSlash || i <= 1) {
                pcPath[stDest] = c;
                bLastSlash = bSlash;
                ++stDest;
            }
        }
        pcPath[stDest] = '\0';
    }
}

bool NiPath::Standardize(char* pcPath) {
    bool bModified = false;
    if (!pcPath) {
        return bModified;
    }
    size_t stLen = strlen(pcPath);
    size_t stDest = 0;
    bool bLastSlash = false;
    for (size_t i = 0; i < stLen; ++i) {
        char c = pcPath[i];
        if (c == '/') {
            bModified = true;
            c = '\\';
        }
        bool bSlash = c == '\\';
        if (bSlash && bLastSlash && i > 1) {
            bModified = true;
        }
        else {
            pcPath[stDest] = c;
            bLastSlash = bSlash;
            ++stDest;
        }
    }
    pcPath[stDest] = '\0';
    return bModified;
}

// Each match is shifted out with strcpy_s over its own length, overlapping as it is, as the game does.
void NiPath::RemoveSlashDotSlash(char* pcPath) {
    char acSubStr[4] = "\\.\\";
    char* pcFound = const_cast<char*>(std::strstr(pcPath, acSubStr));
    while (pcFound) {
        strcpy_s(pcFound, strlen(pcFound), pcFound + 2);
        pcFound = const_cast<char*>(std::strstr(pcPath, acSubStr));
    }
}

char* NiPath::GetPathRoot(char* pcPath) {
    char v3;
    char* v1 = pcPath;
    if (!NiStrnicmp(pcPath, rdata_10A2564, 2u)) {
        return std::strchr(pcPath + 2, 92) + 1;
    }
    if (!NiStrnicmp(pcPath, "\\", 1u) ||
        ((v3 = std::toupper(*pcPath), pcPath[1] == 58) &&
         (static_cast<uint8_t>(v3 - 65) <= 25u && (v1 = pcPath + 2, pcPath[2] == 92)))) {
        ++v1;
    }
    return v1;
}

uint32_t NiPath::ConvertToAbsolute(
    char* pcAbsolutePath,
    uint32_t stAbsBytes,
    const char* pcRelativePath,
    const char* pcRelativeToHere
) {
    // EXE-INLINED none: strlen
    uint32_t stLenRelativeToHere = 0;
    while (pcRelativeToHere[stLenRelativeToHere]) {
        ++stLenRelativeToHere;
    }
    // EXE-INLINED none: strlen
    uint32_t stLenRelativePath = 0;
    while (pcRelativePath[stLenRelativePath]) {
        ++stLenRelativePath;
    }
    bool bInsertDelimiter = pcRelativeToHere[stLenRelativeToHere - 1] != '\\';
    uint32_t stRequiredSize = stLenRelativePath + (bInsertDelimiter ? 1 : 0) + stLenRelativeToHere + 1;
    if (stAbsBytes < stRequiredSize) {
        if (stAbsBytes) {
            pcAbsolutePath[0] = '\0';
        }
        return 0;
    }
    strcpy_s(pcAbsolutePath, stAbsBytes, pcRelativeToHere);
    if (bInsertDelimiter) {
        strcat_s(pcAbsolutePath, stAbsBytes, "\\");
    }
    strcat_s(pcAbsolutePath, stAbsBytes, pcRelativePath);
    NiPath::RemoveDotDots(pcAbsolutePath);
    // EXE-INLINED none: strlen
    uint32_t stLen = 0;
    while (pcAbsolutePath[stLen]) {
        ++stLen;
    }
    return stLen;
}

// A relative path skips its leading '.' and '\\' characters; an absolute one starts where GetPathRoot says. Each
// "\\.." is cut together with the directory before it by a forward byte copy that includes the terminator.
void NiPath::RemoveDotDots(char* pcPath) {
    char* pcStart = pcPath;
    Standardize(pcPath);
    RemoveSlashDotSlash(pcPath);
    if (IsRelative(pcPath)) {
        if (!pcStart) {
            return;
        }
        while (*pcStart == '.' || *pcStart == '\\') {
            if (!++pcStart) {
                return;
            }
        }
    }
    else {
        pcStart = GetPathRoot(pcPath);
    }
    if (!pcStart) {
        return;
    }
    char* pcEnd = pcStart + strlen(pcStart) + 1;
    char acSubStr[4] = "\\..";
    char* pcFound = const_cast<char*>(std::strstr(pcStart, acSubStr));
    while (pcFound) {
        *pcFound = '\0';
        // Calls the C strrchr, not the C++ non-const overload.
        char* pcDest = const_cast<char*>(std::strrchr(static_cast<const char*>(pcStart), '\\'));
        char* pcSrc;
        if (pcDest) {
            pcSrc = pcFound + 3;
        }
        else {
            pcSrc = (pcFound + 3) ? pcFound + 4 : nullptr;
            pcDest = pcStart;
        }
        int32_t iCount = static_cast<int32_t>(pcEnd - pcSrc);
        if (iCount > 0) {
            do {
                *pcDest = *pcSrc;
                ++pcDest;
                ++pcSrc;
            }
            while (--iCount);
        }
        pcFound = const_cast<char*>(std::strstr(pcStart, acSubStr));
    }
}
