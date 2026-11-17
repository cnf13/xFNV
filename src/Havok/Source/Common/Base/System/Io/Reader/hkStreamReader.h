#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

// Havok's byte source.
class hkStreamReader : public hkReferencedObject {
public:
	// Where seek counts the offset from.
	enum SeekWhence : int32_t {
		STREAM_SET = 0,
		STREAM_CUR = 1,
		STREAM_END = 2,
	};

	hkStreamReader();
	~hkStreamReader() override;

	virtual hkBool* isOk(hkBool* arResult) const = 0;
	// Reads up to aiNumBytes bytes; returns how many were read.
	virtual int32_t read(void* apBuffer, int32_t aiNumBytes) = 0;
	// The base reads and discards the bytes in chunks of 0x200.
	virtual int32_t skip(int32_t nbytes);
	// The base says no.
	virtual hkBool* markSupported(hkBool* arResult) const;
	// The base fails (returns 1).
	virtual hkResult setMark(int32_t markLimit);
	// The base fails (returns 1).
	virtual hkResult rewindToMark();
	// The base says no.
	virtual hkBool* seekTellSupported(hkBool* arResult) const;
	// The base fails (returns 1). aeWhence: 0 from the start, 1 from here, 2 from the end.
	virtual hkResult seek(int32_t offset, SeekWhence whence);
	// The base returns -1.
	virtual int32_t tell() const;

	// Reads one byte; returns it, or -1 at the end of the stream.
	int32_t readChar();
};
