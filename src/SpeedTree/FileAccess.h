#pragma once

#include "SpeedTree/EFileToken.h"

#include <string>
#include <vector>

class CIdvBezierSpline;
class CVec3;

// The reader and writer of a SpeedTree file: the position in it and the memory it reads from. The vector and spline
// parsers are not declared yet.
class CTreeFileAccess {
public:
	// Default constructor for LoadTree's local: the destructor runs first before it is constructed again, so the memory
	// vector starts empty (value-initialised).
	CTreeFileAccess() : m_nIndex(0), m_vMemory() {}
	CTreeFileAccess(const uint8_t* pMemory, int32_t nNumBytes);

	// Reads the next token of the file and returns its EFileToken.
	EFileToken ParseToken() { return static_cast<EFileToken>(ParseInt()); }
	// The next token, without moving past it.
	EFileToken PeekToken();
	// Reads a one-byte bool and a four-byte float.
	bool ParseBool();
	float ParseFloat();
	// The remaining speedtree-2 accessors.
	uint8_t ParseByte();
	int32_t ParseInt();
	uint32_t ParseLong();
	int16_t ParseShort();
	std::string ParseString();
	CVec3 ParseVector3();
	void Rewind();
	bool EndOfFile() const;
	void SaveBool(bool bBool);
	void SaveByte(uint8_t chByte);
	void SaveFloat(float fFloat);
	void SaveInt(int32_t nInt);
	void SaveLong(uint32_t ulLong);
	void SaveShort(int16_t sShort);
	void SaveString(std::string strString);
	void SaveToken(EFileToken eToken);
	void SaveVector3(const CVec3& cVec);
	// `new CIdvBezierSpline(ParseString())`.
	CIdvBezierSpline* ParseBranchParameter();
	// Reads a vector of three floats: a zeroed CVec3 filled by three ParseFloat calls, returned through a1.
	float* ParseVector3(float* a1);
	// The int at the read position; the position moves on by 8.
	int32_t ParseLongSkipPadding();

	// The read position, a byte offset into m_vMemory.
	int32_t m_nIndex;
	std::vector<uint8_t, std::allocator<uint8_t>> m_vMemory;
};
