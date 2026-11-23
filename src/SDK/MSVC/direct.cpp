#include "SDK/MSVC/direct.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/errno.h"

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

// The home of the exe's out-of-line VC9 <direct.h> bodies.

namespace rt {

    // VC9's _getcwd is _getdcwd for the current drive: the directory with its terminator must fit in aiMaxLen bytes,
    // else ERANGE; a null apBuffer gets a zeroed block of at least that length from calloc, which the caller frees.
    char* _getcwd(char* apBuffer, int32_t aiMaxLen) {
        if (apBuffer && aiMaxLen <= 0) {
            errno = kEINVAL;
            _invalid_parameter_noinfo();
            return nullptr;
        }
        std::error_code kError;
        const std::string kDirectory = std::filesystem::current_path(kError).string();
        if (kError) {
            errno = kENOENT;
            return nullptr;
        }
        const size_t stLength = kDirectory.size() + 1;
        size_t stMaxLen = aiMaxLen > 0 ? static_cast<size_t>(aiMaxLen) : 0;
        if (!apBuffer) {
            if (stLength > stMaxLen) {
                stMaxLen = stLength;
            }
            apBuffer = static_cast<char*>(std::calloc(stMaxLen, 1));
            if (!apBuffer) {
                errno = ENOMEM;
                return nullptr;
            }
        }
        if (stLength > stMaxLen) {
            errno = kERANGE;
            return nullptr;
        }
        std::memcpy(apBuffer, kDirectory.c_str(), stLength);
        return apBuffer;
    }
} // namespace rt
