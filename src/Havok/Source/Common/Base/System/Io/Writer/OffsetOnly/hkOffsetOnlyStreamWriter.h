#pragma once

#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"

// Counts output bytes and tracks seeks without storing the written data.
class hkOffsetOnlyStreamWriter : public hkStreamWriter {
public:
	hkOffsetOnlyStreamWriter();
	~hkOffsetOnlyStreamWriter() override;

	hkBool* isOk(hkBool* arResult) const override;
	int32_t write(const void* apBuffer, int32_t n) override;
	hkBool* seekTellSupported(hkBool* arResult) const override;
	hkResult seek(int32_t offset, hkStreamWriter::SeekWhence whence) override;
	// The current offset, m_offset.
	int32_t tell() const override;

	int32_t m_offset;
	int32_t m_eofOffset;
};
