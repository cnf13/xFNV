#pragma once

#include <cstdint>

// Path string helpers.
class NiPath {
public:
    // Standardize's separator rewrite without its result: '/' becomes '\\' and repeated backslashes after the first two
    // characters collapse. BSStream::Load calls it.
    static void StandardizeNoResult(char* pcPath);
    // Rewrites '/' to '\\' and collapses repeated backslashes after the first two characters; true when it changed
    // pcPath.
    static bool Standardize(char* pcPath);
    // Removes every "\\.\\" from pcPath.
    static void RemoveSlashDotSlash(char* pcPath);
    // Returns the part of an absolute pcPath
    // RemoveDotDots works on.
    static char* GetPathRoot(char* pcPath);
    // false for a path starting with a separator or a drive letter and ':'.
    static bool IsRelative(const char* pcPath);
    static bool GetCurrentWorkingDirectory(char* pcPath, uint32_t stDestSize);
    // Joins pcRelativeToHere and pcRelativePath into pcAbsolutePath and returns its length, or 0 when it does not fit.
    static uint32_t ConvertToAbsolute(
        char* pcAbsolutePath,
        uint32_t stAbsBytes,
        const char* pcRelativePath,
        const char* pcRelativeToHere
    );
    // Standardizes pcPath and collapses its "dir\.." pairs.
    static void RemoveDotDots(char* pcPath);
};

using rdata_10A2564_t = const char[4];
inline rdata_10A2564_t rdata_10A2564{"\\\\"};
