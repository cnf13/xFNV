#include "Reconstruction.hpp"
#include "SDK/FaceGen/assert.hpp"
#include "SDK/FaceGen/lastError.hpp"
#include "SDK/MSVC/iosfwd.h"
#include "SDK/MSVC/string.h"
#include <memory>
#include <string>
#include <stdint.h>

int32_t futLastErrorCode = 0;
futLastErrorText_t futLastErrorText = {};

// The first error wins: a later one is dropped until the error is cleared. The exe leaves err in eax only on the
// storing path; the PDB's function returns nothing.
void futSetLastError(FutErrorCodeE err)
{
	if (futLastErrorCode == 0) {
		futLastErrorCode = err;
		futLastErrorText[0] = 0;
	}
}

// The same state with the message copied into the global buffer (memcpy of at most 0x100
// bytes, then its terminator); the by-value message the callee destroys is the caller's copy of it.
// The exe's cleanup-only C++ EH frame is left out.
void futSetLastError(FutErrorCodeE err, std::string aMessage) {
	if (!futLastErrorCode) {
		size_t v2;
		size_t stCount = aMessage.size();
		futLastErrorCode = err;
		if (aMessage.size() >= 256) {
			fr2Assert("D:\\_Fallout3\\Platforms\\Common\\Code\\SDK\\FaceGen\\lastError.cpp", 65);
		}
		if (stCount >= 256) {
			v2 = 255;
		} else {
			v2 = stCount;
		}
		rt::memcpy(futLastErrorText, aMessage.c_str(), v2);
		futLastErrorText[v2] = 0;
	}
}

int32_t futHasLastError()
{
	return (int32_t)((uint8_t)(futLastErrorCode != 0));
}

int32_t futGetLastError(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1) {
	uint32_t SourceSize = std::strlen(futLastErrorText);
	a1->assign(futLastErrorText, SourceSize);
	return futLastErrorCode;
}

char futGetLastErrorMessage(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1) {
	switch (futLastErrorCode) {
	case 0: {
		uint32_t SourceSize = std::strlen("No errors");
		a1->assign("No errors", SourceSize);
		break;
	}
	case 1: {
		uint32_t _Count = std::strlen("File corrupt");
		a1->assign("File corrupt", _Count);
		break;
	}
	case 2: {
		uint32_t v15 = std::strlen("File of incorrect type");
		a1->assign("File of incorrect type", v15);
		break;
	}
	case 3: {
		uint32_t v14 = std::strlen("File not found");
		a1->assign("File not found", v14);
		break;
	}
	case 4: {
		uint32_t v13 = std::strlen("File not readable");
		a1->assign("File not readable", v13);
		break;
	}
	case 5: {
		uint32_t v12 = std::strlen("File not writeable");
		a1->assign("File not writeable", v12);
		break;
	}
	case 6: {
		uint32_t v11 = std::strlen("File write error");
		a1->assign("File write error", v11);
		break;
	}
	case 7: {
		uint32_t v10 = std::strlen("File seek error");
		a1->assign("File seek error", v10);
		break;
	}
	case 8: {
		uint32_t v9 = std::strlen("OpenGL initialization error");
		a1->assign("OpenGL initialization error", v9);
		break;
	}
	case 9: {
		uint32_t v8 = std::strlen("Cannot load incompatible model");
		a1->assign("Cannot load incompatible model", v8);
		break;
	}
	case 10: {
		uint32_t v7 = std::strlen("Out of memory");
		a1->assign("Out of memory", v7);
		break;
	}
	case 11: {
		uint32_t v6 = std::strlen("Error");
		a1->assign("Error", v6);
		break;
	}
	case 12: {
		uint32_t v5 = std::strlen("Dimension of the image is not power of 2");
		a1->assign("Dimension of the image is not power of 2", v5);
		break;
	}
	default: {
		uint32_t v4 = std::strlen("Undocumented error");
		a1->assign("Undocumented error", v4);
		break;
	}
	}
	if (!futLastErrorCode) {
		return 1;
	}
	if (std::strlen(futLastErrorText)) {
		uint32_t v3 = std::strlen(" : ");
		a1->append(" : ", v3);
		uint32_t v2 = std::strlen(futLastErrorText);
		a1->append(futLastErrorText, v2);
	}
	return 0;
}

void futClearLastError() {
	futLastErrorText[0] = 0;
	futLastErrorCode = 0;
}

// Writes the FUT library's last error to the log: the error text into a local string (0xAC6540), then WriteLog.
// The exe's cleanup-only C++ EH frame is left out.
void futPrintLastError() {
	FNVD_CONSTRUCTED_LOCAL(std::string, kText, nullptr);
	futGetLastErrorMessage(&kText);
	WriteLog("FUT System", 0, "Error");
}

void futAssertLastError() {
	if (futLastErrorCode) {
		futPrintLastError();
		std::exit(-1);
	}
}
