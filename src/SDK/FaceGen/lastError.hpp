#pragma once

#include <string>

// The FaceGen SDK's error codes. The exe stores the code as a dword (futSetLastError, 0xAC641F).
enum FutErrorCodeE : int32_t {
	FUTERR_NONE = 0x0,
	FUTERR_FILE_CORRUPT = 0x1,
	FUTERR_FILE_INCORRECT_TYPE = 0x2,
	FUTERR_FILE_NOT_FOUND = 0x3,
	FUTERR_FILE_NOT_READABLE = 0x4,
	FUTERR_FILE_NOT_WRITEABLE = 0x5,
	FUTERR_FILE_WRITE_ERROR = 0x6,
	FUTERR_FILE_SEEK_ERROR = 0x7,
	FUTERR_OGL_INIT = 0x8,
	FUTERR_STATKEY_MISMATCH = 0x9,
	FUTERR_HEAP_NOALLOC = 0xA,
	FUTERR_SEE_TEXT = 0xB,
	FUTERR_IMAGE_NOT_POWER2 = 0xC,
	FUTERR_SIZE = 0xD,
};

// The FaceGen SDK's last-error state: the first error set since the last clear, and its message.

// The last error code, 0 when none is set.
extern int32_t futLastErrorCode;
// The last error's message, NUL-terminated (at most 255 characters).
using futLastErrorText_t = char[0x100];
extern futLastErrorText_t futLastErrorText;

// Records err as the last error, with an empty message, unless an error is already set.
void futSetLastError(FutErrorCodeE err);
// Records err with aMessage (at most 0x100 bytes of it) as the last error, unless an error is already set. The
// message is a std::string by value (0x1C bytes on the stack), which the callee destroys (cdecl, ret 0x20; 0xAC6430).
void futSetLastError(FutErrorCodeE err, std::string aMessage);

int32_t futHasLastError();

int32_t futGetLastError(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);

char futGetLastErrorMessage(std::basic_string<char, std::char_traits<char>, std::allocator<char>>* a1);

void futClearLastError();

void futPrintLastError();

void futAssertLastError();
