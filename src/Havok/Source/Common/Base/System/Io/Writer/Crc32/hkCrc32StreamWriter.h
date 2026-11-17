#pragma once

#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Reconstruction.hpp"

#include <stdint.h>

// A stream writer that keeps the CRC-32 of the bytes written to it and stores none.
class hkCrc32StreamWriter : public hkStreamWriter {
public:
	// The hybrid's implicit default constructor: the sites declare the local, then construct it in place.
	hkCrc32StreamWriter() = default;
	hkCrc32StreamWriter(uint32_t startCrc);

	// Says yes.
	hkBool* isOk(hkBool* arResult) const override;
	// Folds the bytes into the CRC; returns aiNumBytes.
	int32_t write(const void* ptr, int32_t nbytes) override;

	// The finished CRC: the running value inverted.
	uint32_t getCrc32() const;

	uint32_t m_crc32;
};
inline constexpr fnvd::Vtable hkCrc32StreamWriter_vftable{};
