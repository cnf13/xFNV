#pragma once

#include <string>
#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Fallout Shared/FaceGen/FR2MatrixVTC.h"

// A .egt file's header (0x38 bytes, the size fffReadEgtFile reads it with): the images' rows and columns, the
// symmetric and asymmetric mode counts and the basis key. The PDB's members.
struct FffEgtFileHeaderS {
    uint32_t nrows;
    uint32_t ncols;
    uint32_t numSym;
    uint32_t numAsym;
    uint32_t basisKey;
    char reserve[36];

    // The byte-order fix of a header read from a file: empty on a little-endian build.
    void fix();
};

// Reads the .egt file fname: its basis key into basisKey, then the symmetric and asymmetric texture modes into symm
// and asym, each mode's scale and three images of nrows rows read bottom row first; false on the first failed read
// (cdecl, 0xAB2640). Its PDB source file is SDK\FaceGen\EgtFileIO_Sdk.cpp.
bool fffReadEgtFile(
    const std::string& fname,
    uint32_t& basisKey,
    std::vector<FffModeTexS, std::allocator<FffModeTexS>>& symm,
    std::vector<FffModeTexS, std::allocator<FffModeTexS>>& asym
);
