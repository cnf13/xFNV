#pragma once

class hkQuaternion;

// The PDB's form; the exe's code is a bare ret (bytes need no swapping).
void handleEndianSignedQuaternion24(uint8_t* dataInOut);
// The PDB's name (SignedQuaternion.cpp): the UNCOMPRESSED rotation writer of packfunc, copying in's sixteen bytes to
// out8 (cdecl, ret 0, 0xE52860; the exe leaves in in eax, which no caller reads).
void packSignedQuaternion128(const hkQuaternion* in, uint8_t* out8);
void handleEndianSignedQuaternion128(uint8_t* dataInOut);
// The PDB's form; the exe's code is a bare ret (bytes need no swapping).
void handleEndianSignedQuaternion16(uint8_t* dataInOut);
// The PDB's name (Compression\SignedQuaternion\SignedQuaternion.cpp): copies the sixteen bytes of an uncompressed
// quaternion (cdecl, 0xE52EF0; the exe leaves in in eax, which the unpackfunc table's callers do not read).
void unpackSignedQuaternion128(const uint8_t* in8, hkQuaternion* out);
// The PDB's form; the exe's code is a bare ret (bytes need no swapping).
void handleEndianSignedQuaternion40(uint8_t* dataInOut);
// The PDB's form: in's four components packed into two bytes at out (cdecl, ret 0, 0xE520B0).
void packSignedQuaternion16(const hkQuaternion* in, uint8_t* out);
// The PDB's name (Compression\SignedQuaternion\SignedQuaternion.cpp; Havok SDK hkaCompression.h). Packs in into four
// bytes at out8 (cdecl, 0xE52320).
void packSignedQuaternion32(const hkQuaternion* in, uint8_t* out8);
// Havok SDK name (hkaCompression.h); not in the PDB. Unpacks two bytes (cdecl, 0xE52880).
void unpackSignedQuaternion16(const uint8_t* in, hkQuaternion* out);
// Havok SDK name (hkaCompression.h); not in the PDB. Unpacks three bytes (cdecl, 0xE52920).
void unpackSignedQuaternion24(const uint8_t* in, hkQuaternion* out);
// The PDB's form: the four bytes at in8 unpacked into out (cdecl, ret 0, 0xE52A70).
void unpackSignedQuaternion32(const uint8_t* in8, hkQuaternion* out);
// Havok SDK name (hkaCompression.h); not in the PDB. Unpacks five bytes (cdecl, 0xE52C30).
void unpackSignedQuaternion40(const uint8_t* in, hkQuaternion* out);
// The start of a track's packed NURBS data in block of base (the PDB's form; cdecl, ret 0,
// 0xE52FA0).
const uint8_t* computePackedNurbsOffsets(const uint8_t* base, const uint32_t* m_blockOffsets, uint32_t block,
	uint32_t maskAndQuantizationSize);
// The PDB's name (SignedQuaternion.cpp): endianfunc's POLAR32 entry, reversing four bytes in place (cdecl, ret 0,
// 0xE52FE0; the exe leaves dataInOut in eax, which no caller reads).
void handleEndianSignedQuaternion32(uint8_t* dataInOut);
void handleEndianSignedQuaternion48(uint8_t* dataInOut);
// The PDB's name (SignedQuaternion.cpp): packfunc's THREECOMP24 rotation writer, packing in into three bytes at out
// (cdecl, ret 0, 0xE52190; the exe leaves the last byte in al, which no caller reads).
void packSignedQuaternion24(const hkQuaternion* in, uint8_t* out);
// The PDB's name (SignedQuaternion.cpp): packfunc's THREECOMP40 rotation writer, packing in into five bytes at out
// (the exe leaves the last byte, zero-extended, in eax, which no caller reads).
void packSignedQuaternion40(const hkQuaternion* in, uint8_t* out);
// The PDB's name (SignedQuaternion.cpp): packfunc's THREECOMP48 rotation writer, packing in into three words at out8
// (the exe leaves the last word in eax, which no caller reads).
void packSignedQuaternion48(const hkQuaternion* in, uint8_t* out8);
// The PDB's name (Compression\SignedQuaternion\SignedQuaternion.cpp): unpacks six bytes (cdecl, 0xE52D90; the exe
// leaves 0x8000 in eax, which the unpackfunc table's callers do not read).
void unpackSignedQuaternion48(const uint8_t* in8, hkQuaternion* out);
