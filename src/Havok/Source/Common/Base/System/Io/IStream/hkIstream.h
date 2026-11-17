#pragma once

#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkMemoryTrack;
class hkStreamReader;
class hkStringBuf;

// Not named in the PDB or IDA; name chosen here. Skips white space in the stream: peeks
// (vtable +0x1C) and reads (+0x10) through the hkStreamReader in edi (hkIstream::m_streamReader).
// Inferred type: r_edi: hkStreamReader* from 0xdcc4aa (hkIstream::m_streamReader (this of hkIstream::operator>>)), its
// members used at 0xdcc1b0
int32_t eatWhiteSpace(hkStreamReader* r_edi);
// a1 is the stream, as for eatWhiteSpace.
// Inferred type: a1: hkStreamReader* from 0xdcc74c (hkIstream::m_streamReader (this of hkIstream::operator>>)), its
// members used at 0xdcc25a
uint64_t readInteger(hkStreamReader* a1, char* r_eax);
// Inferred type: return: double from 0xdcc486 (result of std::strtod); r_eax: hkStreamReader* from 0xdcc5c6
// (hkIstream::m_streamReader (this of hkIstream::operator>>)), its members used at 0xdcc3bf
double readFloat(hkStreamReader* r_eax);

// Reference-counted formatted input stream backed by an hkStreamReader.
class hkIstream : public hkReferencedObject {
public:
	// A stream with no reader, safe to destroy: the sites declare the local, then construct it again in place.
	hkIstream() : m_streamReader(nullptr) {}
	hkIstream(hkStreamReader* a1);
	hkIstream(const hkMemoryTrack* apTrack);
	hkIstream(const char* fname);
	hkIstream(const void* mem, int32_t memSize);
	~hkIstream() override;

	hkBool* isOk(hkBool* apResult);
	int32_t getline(char* str, int32_t maxlen, char delim);
	// The PDB's operator>>(hkStringBuf&): reads one white-space-delimited word into str (thiscall, ret 4, 0xDCC8E0).
	hkIstream& operator>>(hkStringBuf& str);
	// Reads one byte through the stream reader.
	hkIstream& operator>>(char& c);
	hkIstream& operator>>(float& f);
	hkIstream& operator>>(double& d);
	int32_t read(void* buf, int32_t nbytes);
	// The integer extractors: the magnitude read, negated (with the type's wrap) when a minus sign was read.
	hkIstream& operator>>(int16_t& s);
	hkIstream& operator>>(uint16_t& s);
	hkIstream& operator>>(int32_t& i);
	hkIstream& operator>>(uint32_t& u);
	hkIstream& operator>>(int64_t& i);
	hkIstream& operator>>(uint64_t& u);
	hkIstream& operator>>(hkStringPtr& str);
	// Reads the word "true" or "false" through the stream reader, stores the bool and skips the
	// word, else rewinds the mark (thiscall, ret 4, 0xDCC4A0; the PDB's operator>>(hkBool&)).
	hkIstream& operator>>(hkBool& b);

	hkStreamReader* m_streamReader;
};
inline constexpr fnvd::Vtable hkIstream_vftable{};
