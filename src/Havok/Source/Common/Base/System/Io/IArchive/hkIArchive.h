#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

class hkStreamReader;

void byteswap(char& a1, char& r_eax);

// Reads binary archive data with optional byte swapping.
class hkIArchive : public hkReferencedObject {
public:
    hkIArchive(const void* mem, int32_t memSize, hkBool byteswap);
    hkIArchive(const char* apFileName, hkBool abByteSwap);
    hkIArchive(hkStreamReader* sb, hkBool bs);
    ~hkIArchive() override;

    void setStreamReader(hkStreamReader* a1);
    hkStreamReader* getStreamReader();
    void readArrayGeneric(void* array, int32_t elemsize, int32_t arraySize);
    int32_t readRaw(void* buf, int32_t nbytes);
    float readFloat32();
    // Whether the stream reader is ok: asks m_streamReader->isOk and returns apResult.
    hkBool* isOk(hkBool* apResult) const;

    hkStreamReader* m_streamReader;
    hkBool m_byteSwap;
    uint8_t padding0D[3];
};
