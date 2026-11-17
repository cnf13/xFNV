#pragma once

#include <cstdint>
#include <string>

// The position of the highest set bit of a: floor(log2(a)). A zero a is reported.
uint32_t fr2Log2Floor(uint32_t a);

// The sum of val1 and val2, saturated to the type's range when it overflows.
template <typename T>
T fr2AddSaturated(T val1, T val2);
#include <limits>

// The exact sum (in double, which holds every integer sum), clamped to T's range.
template <typename T>
T fr2AddSaturated(T val1, T val2) {
    double dSum = static_cast<double>(val1) + static_cast<double>(val2);
    if (dSum < static_cast<double>(std::numeric_limits<T>::lowest())) {
        return std::numeric_limits<T>::lowest();
    }
    if (dSum > static_cast<double>(std::numeric_limits<T>::max())) {
        return std::numeric_limits<T>::max();
    }
    return static_cast<T>(dSum);
}

// The float instance reports a NaN argument and saturates an infinite or NaN sum by its sign; the short
// instance keeps only the clamped sum's low byte, sign-extended. Defined in algs.cpp.
template <>
float fr2AddSaturated<float>(float val1, float val2);
template <>
int16_t fr2AddSaturated<int16_t>(int16_t val1, int16_t val2);
// fname without the characters a file name can't hold (* ? " < > | : \ / and space), built at _Result, which it
// returns: the exe's by-value string return (cdecl, the hidden result first).
std::string* futRemoveBadFnameCharacters(std::string* _Result, const std::string& fname);

// Whether the file fname exists. The exe's 0xAC6E10 (cdecl, the result in al): IDA has no name for it; the PDB name
// is inferred from its file and signature (fr2CheckFileExists(fname) in algs.cpp, one argument, a bool use).
bool fr2CheckFileExists(const std::string& fname);

// outRoot + "." + ext, or the first outRoot + N + "." + ext (N counting from 1) whose file under path does not exist,
// built at _Result, which it returns (cdecl, the hidden result first).
std::string*
futComposeNewFname(std::string* _Result, const std::string& path, const std::string& outRoot, const std::string& ext);

// ceil(log2(a)), through fr2Log2Floor.
uint32_t fr2Log2Ceil(uint32_t a);
