#include "Havok/Source/Common/Base/System/Io/OArchive/hkOArchive.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Array/hkArrayStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/Buffered/hkBufferedStreamWriter.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

void localbyteswap(char& a1, char& r_eax) {
    char v2 = a1;
    a1 = r_eax;
    r_eax = v2;
}

hkOArchive::hkOArchive(void* mem, int32_t memSize, hkBool bs) : m_byteSwap(bs) {
    m_writer = new (hkReferencedObject::newObject(sizeof(hkBufferedStreamWriter)))
        hkBufferedStreamWriter(mem, memSize, hkBool(false));
}

hkOArchive::hkOArchive(const char* apFileName, hkBool abByteSwap)
  : m_writer(hkFileSystem::s_instance->openWriter(apFileName)), m_byteSwap(abByteSwap) {}

hkOArchive::hkOArchive(hkStreamWriter* sw, hkBool bs) : m_writer(sw), m_byteSwap(bs) {
    sw->addReference();
}

hkOArchive::hkOArchive(hkArray<char, hkContainerHeapAllocator>& arr, hkBool bs) : m_byteSwap(bs) {
    m_writer = new (hkReferencedObject::newObject(sizeof(hkArrayStreamWriter)))
        hkArrayStreamWriter(&arr, hkArrayStreamWriter::ARRAY_BORROW);
}

hkOArchive::~hkOArchive() {
    m_writer->removeReference();
}

// Without byte swapping the array goes to the writer in one write. With it, the bytes go through a 512-byte buffer:
// full buffers of 512 / elemSize elements, then the remainder (the bytes past the last multiple of 512, computed
// once up front, signed). Element sizes other than 2, 4 and 8 are copied unswapped. elemSize is reread from the
// argument for each buffer.
void hkOArchive::writeArrayGeneric(const void* ptr, int32_t elemSize, int32_t arraySize) {
    if (!m_byteSwap.m_bool) {
        m_writer->write(ptr, elemSize * arraySize);
        return;
    }
    char buffer[512];
    int32_t bufferBytes = 512;
    int32_t bufferElems = 512 / elemSize;
    int32_t bytesLeft = elemSize * arraySize;
    const char* src = static_cast<const char*>(ptr);
    const int32_t tailBytes = bytesLeft % 512;
    const int32_t tailElems = tailBytes / elemSize;
    while (bytesLeft > 0) {
        if (bytesLeft < 512) {
            bufferElems = tailElems;
            bufferBytes = tailBytes;
        }
        hkString::memCpy(buffer, src, bufferBytes);
        if (elemSize == 2) {
            char* p = buffer;
            for (int32_t i = bufferElems; i > 0; --i) {
                const char t = p[0];
                p[0] = p[1];
                p[1] = t;
                p += 2;
            }
        }
        else if (elemSize == 4) {
            char* p = buffer;
            for (int32_t i = bufferElems; i > 0; --i) {
                char t = p[0];
                p[0] = p[3];
                p[3] = t;
                t = p[1];
                p[1] = p[2];
                p[2] = t;
                p += 4;
            }
        }
        else if (elemSize == 8) {
            char* p = buffer;
            for (int32_t i = bufferElems; i > 0; --i) {
                char t = p[0];
                p[0] = p[7];
                p[7] = t;
                t = p[1];
                p[1] = p[6];
                p[6] = t;
                t = p[2];
                p[2] = p[5];
                p[5] = t;
                t = p[3];
                p[3] = p[4];
                p[4] = t;
                p += 8;
            }
        }
        m_writer->write(buffer, bufferBytes);
        src += bufferBytes;
        bytesLeft -= bufferBytes;
    }
}

void hkOArchive::write32u(uint32_t u) {
    writeArrayGeneric(&u, 4, 1);
}

int32_t hkOArchive::writeRaw(const void* buf, int32_t nbytes) {
    return m_writer->write(buf, nbytes);
}

void hkOArchive::writeArray8u(const uint8_t* a1, int32_t a2) {
    writeArrayGeneric(a1, 1, a2);
}

void hkOArchive::writeArray64(const int64_t* a1, int32_t a2) {
    writeArrayGeneric(a1, 8, a2);
}

void hkOArchive::setStreamWriter(hkStreamWriter* a1) {
    a1->addReference();
    m_writer->removeReference();
    m_writer = a1;
}

void hkOArchive::setByteSwap(hkBool a1) {
    m_byteSwap = a1;
    a1.m_bool;
}

void hkOArchive::writeDouble64(double a1) {
    writeArrayGeneric(&a1, 8, 1);
}

void hkOArchive::writeArray16u(const uint16_t* a1, int32_t a2) {
    writeArrayGeneric(a1, 2, a2);
}

void hkOArchive::writeArray16(const int16_t* a1, int32_t a2) {
    writeArrayGeneric(a1, 2, a2);
}

void hkOArchive::writeArray64u(const uint64_t* a1, int32_t a2) {
    writeArrayGeneric(a1, 8, a2);
}

void hkOArchive::writeArrayDouble64(const double* a1, int32_t a2) {
    writeArrayGeneric(a1, 8, a2);
}

void hkOArchive::write64(int64_t a1) {
    writeArrayGeneric(&a1, 8, 1);
}

hkBool hkOArchive::getByteSwap() {
    return static_cast<char>(m_byteSwap);
}

void hkOArchive::writeArray8(const char* array, int32_t nelem) {
    writeArrayGeneric(array, 1, nelem);
}

void hkOArchive::writeArray32(const int32_t* array, int32_t nelem) {
    writeArrayGeneric(array, 4, nelem);
}

// One byte from the argument's own slot: its low byte (the address of the stack argument goes to the writer).
void hkOArchive::write8(int32_t c) {
    m_writer->write(&c, 1);
}

// One byte from the argument's own slot: its low byte (the address of the stack argument goes to the writer).
void hkOArchive::write8u(int32_t u) {
    m_writer->write(&u, 1);
}

hkBool hkOArchive::isOk() {
    return m_writer->isOk();
}

hkStreamWriter* hkOArchive::getStreamWriter() {
    return m_writer;
}

void hkOArchive::write16(int16_t i) {
    writeArrayGeneric(&i, 2, 1);
}

// Two bytes from the argument's own slot: its low word (the address of the stack argument goes to the writer).
void hkOArchive::write16u(int32_t u) {
    writeArrayGeneric(&u, 2, 1);
}

void hkOArchive::write32(int32_t i) {
    writeArrayGeneric(&i, 4, 1);
}

void hkOArchive::write64u(uint64_t u) {
    writeArrayGeneric(&u, 8, 1);
}

void hkOArchive::writeFloat32(float f) {
    writeArrayGeneric(&f, 4, 1);
}

void hkOArchive::writeArray32u(const uint32_t* array, int32_t nelem) {
    writeArrayGeneric(array, 4, nelem);
}

void hkOArchive::writeArrayFloat32(const float* array, int32_t nelem) {
    writeArrayGeneric(array, 4, nelem);
}
