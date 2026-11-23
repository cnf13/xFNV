#pragma once

#include <cstdint>

// rt: runtime support the game relies on. VC9's <io.h> _access, on the game path's native path.
namespace rt {

    // _access, 0xECBF05: 0 when apFileName exists and allows aiMode (0 existence, 2 write, 4 read, 6 both), else -1
    // with errno set (ENOENT: no such file; EACCES: write asked of a read-only file; EINVAL: other mode bits). The file
    // is the game path's native path (platform::NativePath). Defined in io.cpp.
    int32_t _access(const char* apFileName, int32_t aiMode);

} // namespace rt
