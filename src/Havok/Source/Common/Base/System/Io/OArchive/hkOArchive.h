#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

#include <stdint.h>

class hkStreamWriter;

void localbyteswap(char& a1, char& r_eax);

// Writes binary archive data with optional byte swapping.
class hkOArchive : public hkReferencedObject {
public:
    // An archive with no writer, safe to destroy: the sites declare the local, then construct it again in place.
    hkOArchive() : m_writer(nullptr) {}
    hkOArchive(void* mem, int32_t memSize, hkBool bs);
    hkOArchive(const char* apFileName, hkBool abByteSwap);
    hkOArchive(hkStreamWriter* sw, hkBool bs);
    hkOArchive(hkArray<char>& arr, hkBool bs);
    ~hkOArchive() override;

    // Writes arraySize elements of elemSize bytes from array; with m_byteSwap set, they go through a 512-byte buffer
    // in which each 2, 4 or 8 byte element is reversed.
    void writeArrayGeneric(const void* ptr, int32_t elemSize, int32_t arraySize);
    // Writes the value through writeArrayGeneric.
    void write32u(uint32_t u);
    // Writes the bytes unswapped: a tail call to m_writer->write.
    int32_t writeRaw(const void* buf, int32_t nbytes);
    void writeArray8u(const uint8_t* a1, int32_t a2);
    void writeArray64(const int64_t* a1, int32_t a2);
    void setStreamWriter(hkStreamWriter* a1);
    void setByteSwap(hkBool a1);
    void writeDouble64(double a1);
    void writeArray16u(const uint16_t* a1, int32_t a2);
    void writeArray16(const int16_t* a1, int32_t a2);
    void writeArray64u(const uint64_t* a1, int32_t a2);
    void writeArrayDouble64(const double* a1, int32_t a2);
    void write64(int64_t a1);
    hkBool* getByteSwap(hkBool* apResult);
    void writeArray8(const char* array, int32_t nelem);
    void writeArray32(const int32_t* array, int32_t nelem);
    void write8(int32_t c);
    void write8u(int32_t u);
    hkBool* isOk(hkBool* apResult);
    hkStreamWriter* getStreamWriter();
    void write16(int16_t i);
    void write16u(int32_t u);
    void write32(int32_t i);
    void write64u(uint64_t u);
    void writeFloat32(float f);
    void writeArray32u(const uint32_t* array, int32_t nelem);
    void writeArrayFloat32(const float* array, int32_t nelem);

    hkStreamWriter* m_writer;
    hkBool m_byteSwap;
    uint8_t padding0D[3];
};
