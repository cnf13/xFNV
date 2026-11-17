#include "SDK/MSVC/mbstring.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/errno.h"

// The home of the exe's out-of-line VC9 <mbstring.h> bodies, in their single-byte code page forms (mbstring.h).

namespace {

    // The thread's _mbstok context (the CRT's per-thread data member _mtoken, apart from strtok's _token).
    thread_local char* tpMbsTokContext = nullptr;

} // namespace

int32_t _mbscmp(const unsigned char* apStr1, const unsigned char* apStr2) {
    if (!apStr1 || !apStr2) {
        errno = rt::kEINVAL;
        rt::_invalid_parameter_noinfo();
        return 0x7FFFFFFF;
    }
    return std::strcmp(reinterpret_cast<const char*>(apStr1), reinterpret_cast<const char*>(apStr2));
}

unsigned char* _mbsnbcpy(unsigned char* apDest, const unsigned char* apSource, size_t astCount) {
    if ((!apDest || !apSource) && astCount != 0) {
        errno = rt::kEINVAL;
        rt::_invalid_parameter_noinfo();
        return nullptr;
    }
    return reinterpret_cast<unsigned char*>(
        std::strncpy(reinterpret_cast<char*>(apDest), reinterpret_cast<const char*>(apSource), astCount)
    );
}

// _mbstok_s_l's checks: a null delimiter list, or no string and no context left, is EINVAL (null returned).
unsigned char* _mbstok(unsigned char* apStr, const unsigned char* apDelim) {
    if (!apDelim || (!apStr && !tpMbsTokContext)) {
        errno = rt::kEINVAL;
        rt::_invalid_parameter_noinfo();
        return nullptr;
    }
    return reinterpret_cast<unsigned char*>(
        strtok_s(reinterpret_cast<char*>(apStr), reinterpret_cast<const char*>(apDelim), &tpMbsTokContext)
    );
}
