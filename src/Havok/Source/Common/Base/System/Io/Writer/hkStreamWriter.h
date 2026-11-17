#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

// Havok's byte sink. isOk and seekTellSupported take their hkBool result as their first argument.
class hkStreamWriter : public hkReferencedObject {
public:
    // Where seek counts the offset from.
    enum SeekWhence : int32_t {
        STREAM_SET = 0,
        STREAM_CUR = 1,
        STREAM_END = 2,
    };

    hkStreamWriter();
    ~hkStreamWriter() override;

    virtual hkBool* isOk(hkBool* arResult) const = 0;
    // Writes the bytes; returns how many were written.
    virtual int32_t write(const void* apBuffer, int32_t aiNumBytes) = 0;
    virtual void flush() {}
    // The base says no.
    virtual hkBool* seekTellSupported(hkBool* arResult) const;
    // The base fails (returns 1).
    virtual hkResult seek(int32_t offset, hkStreamWriter::SeekWhence whence);
    // The base returns -1.
    virtual int32_t tell() const;
};

// The class's reflection record.
extern hkClass hkStreamWriterClass;
