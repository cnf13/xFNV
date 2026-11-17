#pragma once

#include <cstdint>

// rt: runtime support the game relies on. VC9's <direct.h> _getcwd, with its buffer rules.
namespace rt {

    // _getcwd, 0xECCBDC: the current directory into apBuffer (aiMaxLen bytes, terminator included) and apBuffer; null
    // with errno ERANGE when it does not fit. A null apBuffer gets a malloc'd one of at least aiMaxLen bytes. Defined
    // in direct.cpp.
    char* _getcwd(char* apBuffer, int32_t aiMaxLen);

} // namespace rt
