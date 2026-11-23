#pragma once

#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"

// A stream writer over a child writer that counts positions from where it started (m_startOffset): every call goes to
// the child.
class hkSubStreamWriter : public hkStreamWriter {
public:
    hkSubStreamWriter(hkStreamWriter* apChildStream)
      : m_childStream(apChildStream), m_startOffset(apChildStream->tell()) {}

    hkBool isOk() const override {
        return m_childStream->isOk();
    }

    int32_t write(const void* buf, int32_t nbytes) override {
        return m_childStream->write(buf, nbytes);
    }

    void flush() override {
        m_childStream->flush();
    }

    hkBool seekTellSupported() const override {
        return m_childStream->seekTellSupported();
    }

    // A seek from the start is moved by m_startOffset.
    hkResult seek(int32_t offset, hkStreamWriter::SeekWhence whence) override {
        int32_t realOffset = (whence == STREAM_SET) ? m_startOffset + offset : offset;
        return m_childStream->seek(realOffset, whence);
    }

    // The child's position less m_startOffset.
    int32_t tell() const override {
        return m_childStream->tell() - m_startOffset;
    }

    hkStreamWriter* m_childStream;
    int32_t m_startOffset;
};
