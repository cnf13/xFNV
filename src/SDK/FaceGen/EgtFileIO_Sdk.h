#pragma once

#include <string>
#include "SDK/FaceGen/MatrixVT.h"

// The texture mode of a FaceGen mesh: its scale, width, height and row padding, then its three byte vectors of image
// data. fffReadEgtFile stores the .egt header's column and row counts in width and height.
struct FffModeTexS {
    FffModeTexS();
    FffModeTexS(const FffModeTexS& _Right);
    ~FffModeTexS();

    // Copy assignment: copies the scale and the three dimensions, then assigns each image vector. Not declared as
    // operator=, as FffModeGS::Assign.
    FffModeTexS& Assign(const FffModeTexS& _Right);

    static void operator delete(void* apMemory);

    float scale;
    uint32_t width;
    uint32_t height;
    uint32_t rowPad;
    std::vector<uint8_t, std::allocator<uint8_t>> img[3];
};

// FffModeTexS::img: the three byte vectors of a texture mode's image data.
using FffModeTexImgVector = std::vector<uint8_t, std::allocator<uint8_t>>;

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
