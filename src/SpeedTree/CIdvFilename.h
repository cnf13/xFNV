#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/iosfwd.h"

#include <memory>
#include <string>

// SpeedTree's file name string: a std::string with the path and extension helpers (not reconstructed yet).
// IDA's CIdvFilename derives from std::string_debug, the PDB's from std::basic_string<char>.
class CIdvFilename : public std::string {
public:
	// The hybrid's implicit default constructor: the base string's. Sites default-construct a name and then
	// FNVD_RECONSTRUCT it, which runs the destructor first, so the base string starts empty (value-initialised).
	CIdvFilename() : std::string() {}
	CIdvFilename(std::basic_string<char, std::char_traits<char>, std::allocator<char>> strFilename);
	~CIdvFilename();

	// The name after the last '/' or '\\'.
	CIdvFilename* NoPath(CIdvFilename* apResult);
	// The exe's copy assignment (thiscall, ret 4, returns this): the base string's _Tidy and
	// assign (0xB2B8D0).
	CIdvFilename& operator=(const CIdvFilename& aRight);
};
