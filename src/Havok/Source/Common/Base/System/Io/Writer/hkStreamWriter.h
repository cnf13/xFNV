#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Reconstruction.hpp"

// Havok's byte sink. The exe returns an hkBool through a hidden pointer, so isOk and seekTellSupported take it as
// their first argument.
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
	// The base does nothing (the SDK's inline body; the exe's slot holds the empty function 0x483710).
	virtual void flush() {}
	// The base says no.
	virtual hkBool* seekTellSupported(hkBool* arResult) const;
	// The base fails (returns 1).
	virtual hkResult seek(int32_t offset, hkStreamWriter::SeekWhence whence);
	// The base returns -1.
	virtual int32_t tell() const;
};
inline constexpr fnvd::Vtable hkStreamWriter_vftable{};

// The class's reflection record.
extern hkClass hkStreamWriterClass;
