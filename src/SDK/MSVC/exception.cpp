#include "SDK/MSVC/exception.h"
#include "BSCore/NewOverloads.h"
#include "SDK/MSVC/stdexcept.h"
#include "SDK/MSVC/string.h"
#include <cstdlib>
#include <cstring>
#include <string>

rt::UnknownExceptionText rt::exception_UnknownText{"Unknown exception"};

// The home of the exe's out-of-line VC9 exception, bad_alloc, logic_error and runtime_error members: the bodies src
// reconstructs are defined and registered here.

namespace rt {

// The C++ constructors and destructor (stdexcept.h says why). The exe's cleanup-only EH frame (the string's
// destruction when the assign throws) is left out.
runtime_error::runtime_error(const runtime_error& _That) : exception(_That)
{
	_Str = _That._Str;
}

runtime_error::runtime_error(const std::string& _Message) : exception()
{
	_Str = _Message;
}

// The C++ constructors and destructor (exception.h says why).
exception::exception() : _m_what(nullptr), _m_doFree(0)
{
}

// The exe reads _What again for the copy, after the malloc.
exception::exception(const char* const& _What)
{
	if (_What) {
		size_t uiSize = std::strlen(_What) + 1;
		char* pcText = static_cast<char*>(std::malloc(uiSize));
		_m_what = pcText;
		if (pcText) {
			strcpy_s(pcText, uiSize, _What);
		}
	} else {
		_m_what = nullptr;
	}
	_m_doFree = 1;
}

// An owned text is copied (_That's text read again for the copy, after the malloc); a borrowed one is shared.
exception::exception(const exception& _That)
{
	_m_doFree = _That._m_doFree;
	const char* pcThatText = _That._m_what;
	if (_m_doFree) {
		if (pcThatText) {
			size_t uiSize = std::strlen(pcThatText) + 1;
			char* pcText = static_cast<char*>(std::malloc(uiSize));
			_m_what = pcText;
			if (pcText) {
				strcpy_s(pcText, uiSize, _That._m_what);
			}
		} else {
			_m_what = nullptr;
		}
	} else {
		_m_what = pcThatText;
	}
}

exception::~exception()
{
	if (_m_doFree) {
		std::free(const_cast<char*>(_m_what));
	}
}

const char* exception::what() const
{
	return _m_what ? _m_what : exception_UnknownText;
}
} // namespace rt

// exception's destructor runs after it (exception.h says why these are C++ destructors).
rt::bad_alloc::~bad_alloc() {
}

rt::bad_alloc* rt::bad_alloc::DeletingDestruct(rt::bad_alloc* apThis, uint32_t auiFlags) {
	apThis->bad_alloc::~bad_alloc();
	if ((auiFlags & 1) != 0) {
		ourFree(apThis);
	}
	return apThis;
}

const char* rt::runtime_error::what() const {
	return _Str.c_str();
}

// exception's copy constructor (exception.h says why these are C++ constructors).
rt::bad_alloc::bad_alloc(const bad_alloc& _That) : exception(_That) {
}
