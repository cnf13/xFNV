#pragma once

#include "SDK/MSVC/stdexcept.h"

std::string IdvFormatString(const char* szFormat, ...);

// The error SpeedTree throws when a tree file cannot be read or parsed. The message is the runtime_error string,
// "<details> [<the C runtime's error text, if asked>]". It has real C++ constructors and an implicit destructor, as
// rt::exception has, because a C++ `throw` needs them.
class IdvFileError : public rt::runtime_error {
public:
    // The four strings the constructor builds the message in, in order: the error text, strDetails + " [", + the error
    // text, + "]".
    using MessageStrings = std::string[4];

    // Builds the runtime_error message from strDetails + " [" + the error text (strerror(errno) when
    // bAppendSystemError, else empty) + "]".
    IdvFileError(const std::string& strDetails, bool bAppendSystemError);
    IdvFileError(const IdvFileError& cThat);

private:
    // The constructor's body, with the message strings in aStrings: a C++ base can only be constructed from the
    // initializer list, and the strings are freed after the runtime_error constructor.
    IdvFileError(const std::string& strDetails, bool bAppendSystemError, MessageStrings&& aStrings);

public:
    // The deleting destructor frees through MemoryManager's Deallocate.
    static void operator delete(void* apMemory);

private:
    // Builds the four message strings in aStrings in the game's order and returns the last.
    static const std::string&
    BuildMessage(const std::string& strDetails, bool bAppendSystemError, MessageStrings& aStrings);
};

// The compiler's ThrowInfo for IdvFileError, which throws of it pass to _CxxThrowException.
extern rt::_s__ThrowInfo IdvFileError_ThrowInfo;

// IdvFormatString(const char* pField, ...) with the result string built in the caller's buffer apResult, which it
// returns.
template <typename... Args>
std::string* IdvFormatString(std::string* apResult, const char* pField, Args... aArgs) {
    ::new (static_cast<void*>(apResult)) std::string(IdvFormatString(pField, aArgs...));
    return apResult;
}
