#pragma once

#include <stdint.h>

class hkQuaternion;

// Does nothing; the bytes need no swapping.
void handleEndianSignedQuaternion24(uint8_t* dataInOut);
// The UNCOMPRESSED rotation writer of packfunc, copying in's sixteen bytes to out8.
void packSignedQuaternion128(const hkQuaternion* in, uint8_t* out8);
void handleEndianSignedQuaternion128(uint8_t* dataInOut);
// Does nothing; the bytes need no swapping.
void handleEndianSignedQuaternion16(uint8_t* dataInOut);
// Copies the sixteen bytes of an uncompressed quaternion.
void unpackSignedQuaternion128(const uint8_t* in8, hkQuaternion* out);
// Does nothing; the bytes need no swapping.
void handleEndianSignedQuaternion40(uint8_t* dataInOut);
// Packs in's four components into two bytes at out.
void packSignedQuaternion16(const hkQuaternion* in, uint8_t* out);
// Packs in into four bytes at out8.
void packSignedQuaternion32(const hkQuaternion* in, uint8_t* out8);
// Unpacks two bytes.
void unpackSignedQuaternion16(const uint8_t* in, hkQuaternion* out);
void unpackSignedQuaternion24(const uint8_t* in, hkQuaternion* out);
// Unpacks the four bytes at in8 into out.
void unpackSignedQuaternion32(const uint8_t* in8, hkQuaternion* out);
// Unpacks five bytes.
void unpackSignedQuaternion40(const uint8_t* in, hkQuaternion* out);
// The start of a track's packed NURBS data in block of base.
const uint8_t* computePackedNurbsOffsets(
    const uint8_t* base,
    const uint32_t* m_blockOffsets,
    uint32_t block,
    uint32_t maskAndQuantizationSize
);
// endianfunc's POLAR32 entry, reversing four bytes in place.
void handleEndianSignedQuaternion32(uint8_t* dataInOut);
void handleEndianSignedQuaternion48(uint8_t* dataInOut);
// packfunc's THREECOMP24 rotation writer, packing in into three bytes at out.
void packSignedQuaternion24(const hkQuaternion* in, uint8_t* out);
// packfunc's THREECOMP40 rotation writer, packing in into five bytes at out.
void packSignedQuaternion40(const hkQuaternion* in, uint8_t* out);
// packfunc's THREECOMP48 rotation writer, packing in into three words at out8.
void packSignedQuaternion48(const hkQuaternion* in, uint8_t* out8);
// Unpacks six bytes.
void unpackSignedQuaternion48(const uint8_t* in8, hkQuaternion* out);
