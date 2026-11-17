#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

// A path shorter than two characters is relative. The exe upper-cases the first character before it tests for the
// ':' and compares the upper-cased byte unsigned against 'A'..'Z'.
bool NiPath::IsRelative(const char* pcPath) {
	if (strlen(pcPath) < 2) {
		return true;
	}
	char c = pcPath[0];
	if (c == '\\' || c == '/') {
		return false;
	}
	char cUpper = static_cast<char>(std::toupper(c));
	if (pcPath[1] != ':') {
		return true;
	}
	if (static_cast<uint8_t>(cUpper - 'A') > 0x19) {
		return true;
	}
	return false;
}

bool NiPath::GetCurrentWorkingDirectory(char* pcPath, uint32_t stDestSize) {
	return (reinterpret_cast<uintptr_t>(rt::_getcwd(pcPath, stDestSize)) != 0);
}
