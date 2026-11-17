#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>

// A stream writer that collects the bytes in a buffer and writes them to another writer when it is full or flushed; it
// can also be a plain writer over a fixed block of memory (m_stream null, optionally a C string). As in hkStreamWriter,
// hkResult is an int (0 is success).
class hkBufferedStreamWriter : public hkStreamWriter {
public:
    hkBufferedStreamWriter(void* apBuffer, int32_t aiBufferSize, hkBool aMemoryIsString);
    hkBufferedStreamWriter(hkStreamWriter* apStream, int32_t aiBufSize);
    ~hkBufferedStreamWriter() override;

    hkBool* isOk(hkBool* arResult) const override;
    int32_t write(const void* mem, int32_t memSize) override;
    void flush() override;
    hkBool* seekTellSupported(hkBool* arResult) const override;
    hkResult seek(int32_t relOffset, hkStreamWriter::SeekWhence whence) override;
    int32_t tell() const override;
    void calcContentStatistics(hkStatisticsCollector* collector, const hkClass* cls) const override;
    const hkClass* getClassType() const override;

    // Writes the buffered bytes to the stream; returns how many.
    int32_t flushBuffer();

    hkStreamWriter* m_stream;
    char* m_buf;
    int32_t m_bufSize;
    int32_t m_bufCapacity;
    hkBool m_ownBuffer;
};

extern hkClass hkBufferedStreamWriterClass;
