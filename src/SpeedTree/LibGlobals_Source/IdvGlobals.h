#pragma once

#include "Reconstruction.hpp"
#include "SDK/MSVC/stdexcept.h"

// The string szFormat formats with the arguments (FrondEngine.cpp's body; cdecl, the string through the hidden result
// pointer, 0xB18860).
std::string IdvFormatString(const char* szFormat, ...);

// speedtree-2 LibGlobals_Source/IdvGlobals.h. The error SpeedTree throws when a tree file cannot be read or parsed.
// The exe has a vftable and a deleting destructor for it; the message is the runtime_error string, "<details> [<the C
// runtime's error text, if asked>]". (IdvSystemError is in the PDB only: the exe has no vftable or code for it, so it
// is not modelled.)
// Real C++ constructors and an implicit destructor, as rt::exception's (SDK/MSVC/exception.h): a C++ `throw`
// needs C++ constructors and destructors in the ThrowInfo (the user's choice of real C++ EH, 2026-11-04).
class IdvFileError : public rt::runtime_error {
public:
	// The four strings the constructor builds the message in, in the exe's order: the error text, strDetails + " [",
	// + the error text, + "]". Not named in the PDB or IDA; name chosen here.
	using MessageStrings = std::string[4];

	// The default constructor the INLINED_LOCAL sites need (ParseTreeInfo's unused error local): runtime_error's.
	IdvFileError() = default;
	// The runtime_error constructor from strDetails + " [" + the error text (strerror(errno) when
	// bAppendSystemError, else empty) + "]", the strings freed, then IdvFileError's vftable (thiscall, ret 8,
	// 0xB09480). IDA name sub_B09480; the PDB's IdvFileError::IdvFileError.
	IdvFileError(const std::string& strDetails, bool bAppendSystemError);
	// The implicit copy constructor: the runtime_error copy, then IdvFileError's vftable (thiscall, ret 4, 0xB097B0).
	IdvFileError(const IdvFileError& cThat);

private:
	// The constructor's body, with the message strings in aStrings: a C++ base can only be constructed from the
	// initializer list, and the exe frees the strings after the runtime_error constructor.
	IdvFileError(const std::string& strDetails, bool bAppendSystemError, MessageStrings&& aStrings);

public:
	// The implicit destructor is the exe's ~IdvFileError (0xB09780): ~runtime_error inlined.
	// The exe's deleting destructor frees through MemoryManager's Deallocate.
	static void operator delete(void* apMemory);

private:
	// Builds the four message strings in aStrings in the exe's order and returns the last. Not named in the PDB or
	// IDA; name chosen here.
	static const std::string& BuildMessage(const std::string& strDetails, bool bAppendSystemError,
		MessageStrings& aStrings);
};

inline constexpr fnvd::Vtable IdvFileError_vftable{};
// IDA name _TI3?AVIdvFileError@@; not in the PDB. The compiler's ThrowInfo for IdvFileError, which the exe's throws
// of it pass to _CxxThrowException.
extern rt::_s__ThrowInfo IdvFileError_ThrowInfo;

// speedtree-2's IdvFormatString(const char* pField, ...) in the exe's form: the result string built in the caller's
// buffer apResult, which it returns. PDB: std::string IdvFormatString(const char* pField, ...), by value; the exe
// 0xB18860 (cdecl) takes the hidden result pointer apResult first and returns it.
template <typename... Args> std::string* IdvFormatString(std::string* apResult, const char* pField, Args... aArgs) {
	::new (static_cast<void*>(apResult)) std::string(IdvFormatString(pField, aArgs...));
	return apResult;
}
