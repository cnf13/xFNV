#include "SDK/MSVC/tchar.h"
#include "SDK/MSVC/mbstring.h"

// VC9's <tchar.h> _MBCS form: _mbscmp((const unsigned char*)_s1, (const unsigned char*)_s2), out of line in the exe.
// The casts are the source's: the same bytes read as unsigned char.
int32_t _tcscmp(const char* apStr1, const char* apStr2) {
    return _mbscmp(reinterpret_cast<const unsigned char*>(apStr1), reinterpret_cast<const unsigned char*>(apStr2));
}

// VC9's <tchar.h> _MBCS form: _mbsicmp((const unsigned char*)_s1, (const unsigned char*)_s2), out of line in the exe.
int32_t _tcsicmp(const char* _String1, const char* _String2) {
    return _mbsicmp(reinterpret_cast<const unsigned char*>(_String1), reinterpret_cast<const unsigned char*>(_String2));
}

// VC9's <tchar.h> _MBCS form: _mbsnbcmp((const unsigned char*)_s1, (const unsigned char*)_s2, _n), out of line in
// the exe.
int32_t _tcsncmp(const char* _s1, const char* _s2, size_t _n) {
    return _mbsnbcmp(reinterpret_cast<const unsigned char*>(_s1), reinterpret_cast<const unsigned char*>(_s2), _n);
}

// VC9's <tchar.h> _MBCS form: _mbsnbicmp((const unsigned char*)_s1, (const unsigned char*)_s2, _n), out of line in
// the exe.
int32_t _tcsnicmp(const char* _String1, const char* _String2, size_t _Char_count) {
    return _mbsnbicmp(
        reinterpret_cast<const unsigned char*>(_String1), reinterpret_cast<const unsigned char*>(_String2), _Char_count
    );
}
