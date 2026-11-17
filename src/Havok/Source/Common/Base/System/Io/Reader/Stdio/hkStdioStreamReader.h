#pragma once

#include "Havok/Source/Common/Base/System/Io/Reader/Seekable/hkSeekableStreamReader.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "SDK/MSVC/stdio.h"

// A stream reader over a C stdio file, which the destructor closes. DefaultFileReader is its other name in the SDK.
class hkStdioStreamReader : public hkSeekableStreamReader {
public:
	hkStdioStreamReader(const char* nameIn);
	~hkStdioStreamReader() override;

	int32_t read(void* buf, int32_t nbytes) override;
	hkBool* isOk(hkBool* arResult) const override;
	hkBool* markSupported(hkBool* arResult) const override;
	hkBool* seekTellSupported(hkBool* arResult) const override;
	hkResult seek(int32_t offset, hkStreamReader::SeekWhence whence) override;
	int32_t tell() const override;

	// The SDK's std::FILE*.
	std::FILE* m_handle;
	hkBool m_isOk;
};

using DefaultFileReader = hkStdioStreamReader;
