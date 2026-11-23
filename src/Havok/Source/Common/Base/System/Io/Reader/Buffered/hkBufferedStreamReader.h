#pragma once

#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"

// A stream reader that reads another reader in blocks into a buffer of its own, which also keeps the mark. As in
// hkStreamReader, hkResult is an int (0 is success).
class hkBufferedStreamReader : public hkStreamReader {
public:
    // The buffer: its constructor and destructor allocate and free begin.
    struct Buffer {
        Buffer(int32_t aiCapacity);
        ~Buffer();

        char* begin;
        // The read position, and how many bytes are valid.
        int32_t current;
        int32_t size;
        int32_t capacity;
        // The position to return to, and how far past it the bytes are kept; -1 for no mark.
        int32_t markPos;
        int32_t markLimit;
    };

    hkBufferedStreamReader(hkStreamReader* s, int32_t bufSize);
    ~hkBufferedStreamReader() override;

    int32_t read(void* buf, int32_t nbytes) override;
    int32_t skip(int32_t nbytes) override;
    hkBool isOk() const override;
    hkBool markSupported() const override;
    hkResult setMark(int32_t markLimit) override;
    hkResult rewindToMark() override;
    hkBool seekTellSupported() const override;
    hkResult seek(int32_t offset, hkStreamReader::SeekWhence whence) override;
    int32_t tell() const override;
    // Slot 9, new here: reads the next block into the buffer.
    virtual hkResult refillBuffer();

    // Makes room in the buffer for a refill, keeping the bytes the mark needs.
    void prepareBufferForRefill();

    hkStreamReader* m_stream;
    Buffer m_buf;
};
