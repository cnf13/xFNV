#pragma once

#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A stream reader over a memory buffer.
class hkMemoryStreamReader : public hkStreamReader {
public:
	// Who owns the buffer: a copy and a taken buffer are freed by the destructor, an in-place one is not.
	enum MemoryType : int32_t {
		MEMORY_COPY = 0,
		MEMORY_TAKE = 1,
		MEMORY_INPLACE = 2,
	};

	hkMemoryStreamReader(const void* mem, int32_t memSize, hkMemoryStreamReader::MemoryType t);

	// Not at end of stream: a read past the end sets m_bufCurrent to m_bufSize + 1.
	hkBool* isOk(hkBool* arResult) const override;
	int32_t read(void* buf, int32_t nbytes) override;
	int32_t skip(int32_t nbytes) override;
	// Marks are supported on a non-empty buffer.
	hkBool* markSupported(hkBool* arResult) const override;
	hkResult setMark(int32_t markLimit) override;
	// Returns to m_markPos; fails when no mark is set.
	hkResult rewindToMark() override;
	hkBool* seekTellSupported(hkBool* arResult) const override;
	hkResult seek(int32_t relOffset, SeekWhence whence) override;
	int32_t tell() const override;
	~hkMemoryStreamReader() override;

	char* m_buf;
	int32_t m_bufCurrent;
	int32_t m_bufSize;
	// The mark rewindToMark returns to; negative when none is set.
	int32_t m_markPos;
	MemoryType m_memType;
};
inline constexpr fnvd::Vtable hkMemoryStreamReader_vftable{};
