#pragma once

#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

class BSFile;

// Havok's byte sink writing to a BSFile. Its seek and tell do not override the base's, and its flush is the base's.
class HavokFileStreambufWriter : public hkStreamWriter {
public:
    // The constructor, writing to the file apFilename.
    HavokFileStreambufWriter(const char* apFilename);
    ~HavokFileStreambufWriter() override;

    hkBool isOk() const override;
    int32_t write(const void* apBuf, int32_t aiBytes) override;
    // seek and tell do not override the base's: seek has different parameter types and tell is not const, so both are
    // new virtual functions. The whence is uint32_t rather than the enum because with int32_t the signature would match
    // the base's seek on Linux and override it.
    virtual hkResult seek(int32_t aiOffset, uint32_t aeWhence);
    virtual int32_t tell();

    // The deleting destructor frees through hkReferencedObject::deleteObject.
    static void operator delete(void* apMemory);

    BSFile* pFile;
    bool bOwned;
    uint8_t padding0D[3];
};
