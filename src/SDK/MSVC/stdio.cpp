#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/errno.h"
#include "platform/Paths.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

// The home of the exe's out-of-line VC9 <stdio.h> bodies: the bodies src reconstructs are defined and registered
// here.

namespace {

// VC9's _VALIDATE_RETURN failure: errno and the invalid-parameter report.
void Invalid(int aiError)
{
	errno = aiError;
	rt::_invalid_parameter_noinfo();
}

} // namespace

namespace rt {

// fopen_s is _fsopen into *appFile: a null name or mode, or an empty mode, is an invalid parameter; an empty name is
// EINVAL alone.
int32_t fopen_s(FILE** appFile, const char* apFilename, const char* apMode)
{
	if (!appFile) {
		Invalid(kEINVAL);
		return kEINVAL;
	}
	*appFile = nullptr;
	if (!apFilename || !apMode || !*apMode) {
		Invalid(kEINVAL);
		return kEINVAL;
	}
	if (!*apFilename) {
		errno = kEINVAL;
		return kEINVAL;
	}
	*appFile = std::fopen(platform::NativePath(apFilename).string().c_str(), apMode);
	return *appFile ? 0 : errno;
}

FILE* fopen(const char* apFilename, const char* apMode)
{
	return std::fopen(platform::NativePath(apFilename).string().c_str(), apMode);
}

char* fgets(char* apBuffer, int32_t aiCount, FILE* apStream)
{
#if defined(_WIN32)
	return std::fgets(apBuffer, aiCount, apStream);
#else
	if (!std::fgets(apBuffer, aiCount, apStream)) {
		return nullptr;
	}
	size_t stLength = std::strlen(apBuffer);
	// Ctrl-Z: the text ends before it, and the stream is at its end (a later read finds none).
	if (char* pEnd = static_cast<char*>(std::memchr(apBuffer, 0x1A, stLength))) {
		*pEnd = '\0';
		std::fseek(apStream, 0, SEEK_END);
		std::fgetc(apStream);
		if (pEnd == apBuffer) {
			return nullptr;
		}
		return apBuffer;
	}
	if (stLength >= 2 && apBuffer[stLength - 2] == '\r' && apBuffer[stLength - 1] == '\n') {
		apBuffer[stLength - 2] = '\n';
		apBuffer[stLength - 1] = '\0';
	} else if (stLength && apBuffer[stLength - 1] == '\r') {
		// A CR in the buffer's last place: with the LF after it, the pair is the LF the line ends with.
		const int32_t iNext = std::fgetc(apStream);
		if (iNext == '\n') {
			apBuffer[stLength - 1] = '\n';
		} else if (iNext != EOF) {
			std::ungetc(iNext, apStream);
		}
	}
	return apBuffer;
#endif
}

int32_t rename(const char* apOldName, const char* apNewName)
{
	return std::rename(platform::NativePath(apOldName).string().c_str(),
		platform::NativePath(apNewName).string().c_str());
}

} // namespace rt

// The Microsoft-only functions, where the C library has none (stdio.h says why not on Windows).
#if !defined(_WIN32)

namespace {

// The length of the text apFormat makes of apArgs; negative on an encoding error.
int FormatLength(const char* apFormat, va_list apArgs)
{
	va_list kArgs;
	va_copy(kArgs, apArgs);
	const int iLength = std::vsnprintf(nullptr, 0, apFormat, kArgs);
	va_end(kArgs);
	return iLength;
}

// The text apFormat makes of apArgs, cut to astSize - 1 characters and terminated (astSize > 0). The printf bodies
// below format into the caller's buffer and never allocate from the game's heap (operator new is MemoryManager's):
// the game's heaps format their messages through them while they allocate (AbstractHeap::BaseAllocate's
// "Committing memory" message), and an allocation there recurses until the stack overflows.
void FormatInto(char* apBuffer, size_t astSize, const char* apFormat, va_list apArgs)
{
	va_list kArgs;
	va_copy(kArgs, apArgs);
	std::vsnprintf(apBuffer, astSize, apFormat, kArgs);
	va_end(kArgs);
}

} // namespace

// VC9's _fread_nolock_s finds a too small buffer while it copies, chunk by chunk; here the whole request is measured
// first. The exe's SEH frame around the locked read is left out; the unlock stays in sequence.
size_t fread_s(void* buffer, size_t bufferSize, size_t elementSize, size_t count, FILE* stream)
{
	if (elementSize == 0 || count == 0) {
		return 0;
	}
	if (!buffer) {
		Invalid(rt::kEINVAL);
		return 0;
	}
	if (!stream || count > SIZE_MAX / elementSize) {
		if (bufferSize != SIZE_MAX) {
			std::memset(buffer, 0, bufferSize);
		}
		Invalid(rt::kEINVAL);
		return 0;
	}
	if (bufferSize != SIZE_MAX && elementSize * count > bufferSize) {
		std::memset(buffer, 0, bufferSize);
		Invalid(rt::kERANGE);
		return 0;
	}
	::flockfile(stream);
	const size_t stRead = std::fread(buffer, elementSize, count, stream);
	::funlockfile(stream);
	return stRead;
}

// _vsnprintf_l: the CRT's string stream of astCount characters (at most INT_MAX) under _output_l; the terminator is
// written only when a character is left for it.
int _vsnprintf(char* apBuffer, size_t astCount, const char* apFormat, va_list apArgs)
{
	if (!apFormat || (astCount != 0 && !apBuffer)) {
		Invalid(rt::kEINVAL);
		return -1;
	}
	const int iLength = FormatLength(apFormat, apArgs);
	if (iLength < 0 || !apBuffer) {
		return iLength;
	}
	const size_t stRoom = astCount > 0x7FFFFFFF ? 0x7FFFFFFF : astCount;
	const size_t stLength = static_cast<size_t>(iLength);
	if (stLength < stRoom) {
		FormatInto(apBuffer, stRoom, apFormat, apArgs);
		return iLength;
	}
	if (stRoom > 0) {
		// The first stRoom characters, no terminator: std::vsnprintf keeps the last place for one, so the whole text
		// is made in the C library's heap (std::malloc, not the game's) and cut here. Without that memory, the last
		// place holds the terminator.
		char* pText = static_cast<char*>(std::malloc(stLength + 1));
		if (pText) {
			FormatInto(pText, stLength + 1, apFormat, apArgs);
			std::memcpy(apBuffer, pText, stRoom);
			std::free(pText);
		} else {
			FormatInto(apBuffer, stRoom, apFormat, apArgs);
		}
	}
	return stLength == stRoom ? iLength : -1;
}

int _snprintf(char* apcBuffer, size_t astCount, const char* apcFormat, ...)
{
	va_list kArgs;
	va_start(kArgs, apcFormat);
	const int iResult = _vsnprintf(apcBuffer, astCount, apcFormat, kArgs);
	va_end(kArgs);
	return iResult;
}

// vsprintf_s: _vsnprintf_helper writes the first astCount characters and, when the terminator has no room, sets the
// last one to 0; vsprintf_s then empties the buffer.
int vsprintf_s(char* apBuffer, size_t astCount, const char* apFormat, va_list apArgs)
{
	if (!apFormat || !apBuffer || astCount == 0) {
		Invalid(rt::kEINVAL);
		return -1;
	}
	const int iLength = FormatLength(apFormat, apArgs);
	if (iLength < 0) {
		apBuffer[astCount - 1] = '\0';
		apBuffer[0] = '\0';
		return -1;
	}
	FormatInto(apBuffer, astCount, apFormat, apArgs);
	if (static_cast<size_t>(iLength) < astCount) {
		return iLength;
	}
	apBuffer[0] = '\0';
	Invalid(rt::kERANGE);
	return -1;
}

#endif
