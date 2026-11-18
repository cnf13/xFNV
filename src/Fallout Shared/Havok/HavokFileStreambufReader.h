#pragma once

#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"

class BSFile;

// Havok's byte source reading from a BSFile. Its seek and tell do not override the base's: the class adds its own seek,
// tell and file size virtual functions, and the rest of its virtual functions are the base's.
class HavokFileStreambufReader : public hkStreamReader {
public:
    // The constructor does not read the path.
    HavokFileStreambufReader(const char* apPath);
    ~HavokFileStreambufReader() override;

    hkBool isOk() const override;
    int32_t read(void* apBuf, int32_t aiBytes) override;
    // These take different parameter types from the base's (seek) and drop the const (tell), so they are new virtual
    // functions rather than overrides. The whence is uint32_t rather than the base's type so that seek does not become
    // an override of the base's seek on Linux, where int and long are spelled the same way.
    virtual hkResult seek(int32_t aiOffset, uint32_t aeWhence);
    virtual int32_t tell();
    // The size of the file; -1 without one.
    virtual int32_t sysGetFileSize();

    // The deleting destructor frees through hkReferencedObject::deleteObject.
    static void operator delete(void* apMemory);

    BSFile* pFile;
    bool bOwned;
    uint8_t padding0D[3];
};
