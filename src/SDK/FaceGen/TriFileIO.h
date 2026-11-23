#pragma once

#include <cstddef>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include "SDK/MSVC/iosfwd.h"

template <class T>
class FR2Vect2TC;
template <class T>
class FR2Vect3TC;
class FffTriFileDataS;
class FutBinaryFileC;

// The 12-byte FR2Vect3TC<int>: a .tri triangle's three vertex indices (FffTriFileDataS::triList and txtTriList). The
// PDB places it in BSFaceGenManager.cpp beside the float instance, whose vector bodies (resize 0xABA7B0) it shares.
template <>
class FR2Vect3TC<int> {
public:
    int32_t x1;
    int32_t x2;
    int32_t x3;
};

// A .tri file's header: the element counts of the lists that follow it (0x38 bytes, the size fffReadTriFile reads it
// with). The PDB's members; extraInfo's bit 0 says the file carries texture coordinates, and the counts are compared
// unsigned (jae).
struct FffTriHeaderS {
    uint32_t numVtx;
    uint32_t numTri;
    uint32_t numQuad;
    uint32_t numVtxLabel;
    uint32_t numSurfLabel;
    uint32_t numTexPts;
    uint32_t extraInfo;
    uint32_t numDiffMorphs;
    uint32_t numStatMorphs;
    uint32_t numStatMorphVtx;
    uint32_t reserve[4];
    // Byte-swaps the counts on a big-endian build; an empty body on the PC (thiscall, ret 0,
    // 0xAB46E0, which IDA names nullsub_323).
    void fix();
};

// A .tri file's difference morph: its label and one vertex delta per vertex (0x34 bytes, the stride of the exe's
// vector of them, 0x662840). The PDB places it in SDK\FaceGen\TriFileIO.cpp.
struct FffTriDiffMorphS {
    std::string label;
    // The PDB's vtxDelta: the per-vertex offsets, 12-byte FR2Vect3TC<float> elements in the exe. fffReadTriFile
    // (0xAB5EC0) resizes it through the 12-byte elements' resize (0xABA7B0), sizes it with idiv 0xC and stores each
    // element as three floats (the file's int16_t deltas times the morph's scale, 0xAB7752..0xAB7803).
    std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>> vtxDelta;
    FffTriDiffMorphS();
    FffTriDiffMorphS(const FffTriDiffMorphS& _Right);
};

// A .tri file's static morph: its label, the first vertex it moves and the vertices' indices (0x38 bytes, the stride
// of the exe's vector of them, 0x65E5E0). The PDB's members are label, firstVtxOffset and vtxIdxList; it does not
// give the index list's element type, which is 4 bytes by the exe's stride.
struct FffTriStatMorphS {
    std::string label;
    uint32_t firstVtxOffset;
    std::vector<uint32_t, std::allocator<uint32_t>> vtxIdxList;
    FffTriStatMorphS();
    FffTriStatMorphS(const FffTriStatMorphS& _Right);
};

// A .tri file's quad: the four vertex indices of one face. 0x10 bytes, the stride of the exe's vector of them
// (0xABD2E0), which copies one element as four dwords and destroys none. The PDB places it in
// SDK\FaceGen\TriFileIO.cpp and names its members x1..x4; its constructors and operators are not modelled, the
// element being copied and destroyed trivially.
template <class T>
class FutVect4TC {
public:
    T x1;
    T x2;
    T x3;
    T x4;
};

// A .tri file's vertex label: the vertex it names and its name. 0x20 bytes, the stride of the exe's vector of them
// (0xABE310); the PDB's members are idx and label.
struct FffTriVtxLabelS {
    uint32_t idx;
    std::string label;
    // For the vector's resize, which value-initialises the new labels.
    FffTriVtxLabelS() = default;
    FffTriVtxLabelS(const FffTriVtxLabelS& _Right);
};

// A .tri file's surface label: the surface it names, the three vertices it interpolates and its name. 0x2C bytes, the
// stride of the exe's vector of them (0xABEB00); the PDB's members are idx, vtxIntWgt and label.
struct FffTriSurfLabelS {
    uint32_t idx;
    // The PDB's vtxIntWgt: 12 bytes by the stride, not accessed by the vector code that copies the element whole.
    float vtxIntWgt[3];
    std::string label;
    FffTriSurfLabelS();
    FffTriSurfLabelS(const FffTriSurfLabelS& _Right);
    static void operator delete(void* apMemory);
};

// Reads the .tri file fname into triData; true when every list was read and the static morphs' vertices add up.
bool fffReadTriFile(const std::string& fname, FffTriFileDataS& triData);
// Writes triData to the open file; true when every write succeeded and the static morphs'
// indices add up to their vertex count.
bool fffWriteTriFile(FutBinaryFileC& file, const FffTriFileDataS& triData);
// Writes triData to the .tri file fname with the ID "FRTRI003"; false when the file does not open,
// else the open file's overload's result.
bool fffWriteTriFile(const std::string& fname, const FffTriFileDataS& triData);

// The 4-byte element vector of the static morphs' index lists: the exe's own out-of-line members (not reconstructed
// yet), which the copy constructor below calls. Defined in TriFileIO.cpp.
using UIntVector = std::vector<uint32_t, std::allocator<uint32_t>>;
// The byte vector of the .tri file's reader: the exe's own out-of-line members (not reconstructed yet), which
// _Construct_n calls. Defined in TriFileIO.cpp.
using CharVector = std::vector<char, std::allocator<char>>;

char fffIsTriFile(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);
