#pragma once

#include <memory>
#include <string>
#include "SDK/MSVC/iosfwd.h"

#include <cstdint>

// A FaceGen .ctl file's header: the basis keys of the geometry and texture models and the basis sizes (0x18 bytes,
// the size fanReadCtlFile 0xAAA7D0 reads it with). The PDB's members; the exe reads each as a dword.
struct fanCtlFileHeaderS {
    uint32_t basisKeyGeo;
    uint32_t basisKeyTex;
    uint32_t basisSize[2][2];

    // The byte-order fix of a header read from a file: empty on a little-endian build.
    void fix();
};

class FanGenCtlsGrpC;
class FanLinCtlGrpS;

char fanIsCtlFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);

bool fanReadCtlFile(
    const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& fname,
    uint32_t& basisKeyGeo,
    uint32_t& basisKeyTex,
    uint32_t (*basisSize)[2],
    FanLinCtlGrpS& linCtl,
    FanGenCtlsGrpC& genCtl
);
