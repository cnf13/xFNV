#pragma once

#include "Havok/Source/Common/Base/System/Io/FileSystem/hkFileSystem.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "SDK/MSVC/stdio.h"

// A byte sink that writes to a C stdio file handle, which it closes when it owns it.
class hkStdioStreamWriter : public hkStreamWriter {
public:
    hkStdioStreamWriter(hkFileSystem::StdStream s);
    hkStdioStreamWriter(const char* nameIn);
    ~hkStdioStreamWriter() override;

    hkBool isOk() const override;
    int32_t write(const void* buf, int32_t nbytes) override;
    void flush() override;
    hkBool seekTellSupported() const override;
    hkResult seek(int32_t offset, hkStreamWriter::SeekWhence whence) override;
    int32_t tell() const override;
    virtual hkBool markSupported() const;

    // Closes the handle when the writer owns it and forgets it.
    void close();
    // Havok SDK signature (hkBase.lib, tied by bytes,reloc).
    void open(const char* nameIn, const char* mode);

    std::FILE* m_handle;
    hkBool m_owned;
};
