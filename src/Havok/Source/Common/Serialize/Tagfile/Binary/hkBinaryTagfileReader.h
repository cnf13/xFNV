#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/System/Io/IArchive/hkIArchive.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"
#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Havok/Source/Common/Serialize/Tagfile/hkTagfileReader.h"

#include <stdint.h>

class hkDataClassImpl;
class hkDataObjectImpl;
class hkDataWorld;
class hkOstreamLogger;
class hkStreamReader;

// The reader of Havok's binary tagfile format. No data of its own. Overrides load.
class hkBinaryTagfileReader : public hkTagfileReader {
public:
	hkBinaryTagfileReader();
	~hkBinaryTagfileReader() override;

	hkDataObject load(hkStreamReader* stream, hkDataWorld& cont) override;
};

// The binary tagfile reader's state and value readers: the archive it reads from, the world it fills, the log, and the
// class, string and object tables it remembers.
struct Reader {
	Reader(hkStreamReader* apStream, hkDataWorld& arWorld);
	~Reader();

	// A length-prefixed string; a non-positive length returns an earlier one from
	// m_prevStrings.
	const char* readString();
	float readFloat();
	// aiBits bits, read as ceil(aiBits / 8) bytes and unpacked one byte per element.
	void readBitfield(int32_t numMembers, hkLocalArray<uint8_t>& bout);
	// aiNumItems elements of the array's element type, appended to it.
	void _readArrayItems(hkDataArray& arr, int32_t nelem);
	// One member's value, by the member's type.
	void readBinaryValue(hkDataObject::Value value, const hkDataClass::MemberInfo& minfo);
	// One object of tag aiType (-1: read the tag first).
	hkDataObject* readObject(hkDataObject* apResult, int32_t aiType);
	// A class name, version and member list, registered with the world.
	hkResult readClass();
	// Checks the magic numbers, reads the version and the classes, then the first object;
	// a null object on failure.
	hkDataObject* readObjectTree(hkDataObject* apResult);
	// Reads a bit-packed signed varint; used for int and __int64.
	template <typename T> T readInt();

	hkIArchive m_ia;
	hkDataWorld& m_world;
	hkOstreamLogger* m_log;
	hkArray<hkDataClassImpl*> m_classes;
	hkArray<char*> m_prevStrings;
	hkArray<hkDataObjectImpl*> m_rememberedObjects;
};

// The first byte holds the sign in bit 0 and the value's low 6 bits above it; while a byte's bit 7 is set, the next
// byte adds 7 more bits.
template <typename T> T Reader::readInt() {
	uint8_t byte;
	m_ia.readArrayGeneric(&byte, 1, 1);
	const bool bNegative = (byte & 1) != 0;
	uint64_t value = (byte >> 1) & 0x3F;
	for (int32_t shift = 6; (byte & 0x80) != 0; shift += 7) {
		m_ia.readArrayGeneric(&byte, 1, 1);
		value |= static_cast<uint64_t>(byte & 0x7F) << shift;
	}
	// The negate wraps (the most negative value stays itself): it is done on the unsigned bits.
	return static_cast<T>(bNegative ? 0 - value : value);
}
