#pragma once

#include "SDK/FaceGen/MatrixVT.h"
#include "SDK/FaceGen/Vect3.h"

#include <string>
#include <vector>

// The 0x1C-byte mode of a FaceGen mesh: its scale and its vertex offsets.
struct FffModeGS {
    FffModeGS() = default;
    FffModeGS(const FffModeGS& _Right);
    ~FffModeGS();

    // Copy assignment: the scale, then the vertex offsets' vector assignment; returns this. Not declared as operator=:
    // a user-declared copy assignment would make FffModeGS non-trivially copyable (vector's Assign has the same
    // reason).
    FffModeGS& Assign(const FffModeGS& _Right);

    float scale;
    std::vector<FR2Vect3TC<int16_t>, std::allocator<FR2Vect3TC<int16_t>>> vertPos;
};

// FffModeGS::vertPos: the mode's vertex offsets.
using FR2Vect3SVector = std::vector<FR2Vect3TC<int16_t>, std::allocator<FR2Vect3TC<int16_t>>>;

// A .egm file's header (0x38 bytes, the size fffReadEgmFile reads it with): the vertex count, the symmetric and
// asymmetric mode counts and the basis key. The PDB's members.
struct FffEgmFileHeaderS {
    uint32_t numPoints;
    uint32_t numSym;
    uint32_t numAsym;
    uint32_t basisKey;
    char reserve[40];

    // The byte-order fix of a header read from a file: empty on a little-endian build.
    void fix();
};

char fffIsEgmFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);

// Reads the .egm file fname: its basis key into basisKey, then the symmetric and asymmetric modes into symm and asym,
// each mode's scale and numPoints vertex offsets; false on the first failed read (cdecl, 0xAAFC80). Its PDB source
// file is SDK\FaceGen\EgmFileIO.cpp.
bool fffReadEgmFile(
    const std::basic_string<char, std::char_traits<char>, std::allocator<char>>& fname,
    uint32_t& basisKey,
    std::vector<FffModeGS, std::allocator<FffModeGS>>& symm,
    std::vector<FffModeGS, std::allocator<FffModeGS>>& asym
);
