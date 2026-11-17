#pragma once

#include "SDK/MSVC/iosfwd.h"

#include <memory>
#include <string>

// SpeedTree's file name string: a std::string with path and extension helpers, which are not declared here.
class CIdvFilename : public std::string {
public:
	CIdvFilename() : std::string() {}
	CIdvFilename(std::basic_string<char, std::char_traits<char>, std::allocator<char>> strFilename);
	~CIdvFilename();

	// The name after the last '/' or '\\'.
	CIdvFilename* NoPath(CIdvFilename* apResult);
	CIdvFilename& operator=(const CIdvFilename& aRight);
};
