#pragma once

#include <cstdint>

// Byte counts the debug save dump collects while a save is written.
class DebugSaveDumpData {
public:
    DebugSaveDumpData();

    // Counts one more entry of aiData + aiOverhead + aiDebug bytes and widens minSize/maxSize to its size.
    void Add(uint32_t aiData, uint32_t aiOverhead, uint32_t aiDebug);
    // The bytes counted: data + overhead + debug.
    uint32_t CalcSize();
    double CalcAverageSize();

    uint32_t total;
    uint32_t data;
    uint32_t overhead;
    uint32_t debug;
    uint32_t minSize;
    uint32_t maxSize;
};
