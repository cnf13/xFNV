#include "SDK/MSVC/sys/stat.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/errno.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

// The home of the exe's out-of-line VC9 <sys/stat.h> bodies.

namespace rt {

    namespace {

        // The drive number VC9 stores in st_dev and st_rdev (A: 0, B: 1, ...) of a path that starts with a drive
        // letter; 0 for any other.
        uint32_t DriveOf(const std::string& arPath) {
            if (arPath.size() >= 2 && arPath[1] == ':') {
                const char cLetter = arPath[0];
                if (cLetter >= 'a' && cLetter <= 'z') {
                    return static_cast<uint32_t>(cLetter - 'a');
                }
                if (cLetter >= 'A' && cLetter <= 'Z') {
                    return static_cast<uint32_t>(cLetter - 'A');
                }
            }
            return 0;
        }

    } // namespace

    // VC9's _stat64i32 (FindFirstFileEx): st_mode from the attributes (__tdtoxmode: a directory is _S_IFDIR | _S_IEXEC,
    // else _S_IFREG; read-only gives _S_IREAD, else _S_IREAD | _S_IWRITE; .exe/.cmd/.bat/.com add _S_IEXEC; the owner's
    // bits are copied to group and other), st_nlink 1, st_size (EOVERFLOW past 2 GB), the times in seconds since 1970.
    // The standard library has no access or creation time: both are the last write time, VC9's own stand-in when the
    // file system has none. The drive is the path's drive letter, else the current directory's (else 0: a POSIX path
    // has none). The file is the game path's native path (platform::NativePath).
    int32_t _stat64i32(const char* apPath, struct _stat64i32* apStat) {
        if (!apStat) {
            errno = kEINVAL;
            _invalid_parameter_noinfo();
            return -1;
        }
        std::memset(apStat, 0, sizeof(*apStat));
        if (!apPath) {
            errno = kEINVAL;
            _invalid_parameter_noinfo();
            return -1;
        }
        // No wildcards: the system would match them.
        if (std::strpbrk(apPath, "?*")) {
            errno = kENOENT;
            return -1;
        }
        std::error_code kError;
        const std::filesystem::path kPath = platform::NativePath(apPath);
        const std::filesystem::file_status kStatus = std::filesystem::status(kPath, kError);
        if (!std::filesystem::exists(kStatus)) {
            errno = kENOENT;
            return -1;
        }
        const bool bDirectory = std::filesystem::is_directory(kStatus);
        uint64_t ullSize = 0;
        if (!bDirectory) {
            ullSize = std::filesystem::file_size(kPath, kError);
            if (kError) {
                errno = kENOENT;
                return -1;
            }
            if (ullSize > 0x7FFFFFFF) {
                errno = kEOVERFLOW;
                return -1;
            }
        }
        const std::filesystem::file_time_type kWriteTime = std::filesystem::last_write_time(kPath, kError);
        if (kError) {
            errno = kENOENT;
            return -1;
        }

        // __tdtoxmode.
        uint32_t uiMode = bDirectory ? 0x4000u | 0x40u : 0x8000u;
        const bool bReadOnly =
            (kStatus.permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none;
        uiMode |= bReadOnly ? 0x100u : 0x100u | 0x80u;
        if (const char* pExtension = std::strrchr(apPath, '.')) {
            if (!_stricmp(pExtension, ".exe") || !_stricmp(pExtension, ".cmd") || !_stricmp(pExtension, ".bat") ||
                !_stricmp(pExtension, ".com")) {
                uiMode |= 0x40u;
            }
        }
        uiMode |= (uiMode & 0700u) >> 3;
        uiMode |= (uiMode & 0700u) >> 6;

        const int64_t iWriteTime = std::chrono::duration_cast<std::chrono::seconds>(
                                       std::chrono::clock_cast<std::chrono::system_clock>(kWriteTime).time_since_epoch()
        )
                                       .count();

        uint32_t uiDrive = DriveOf(apPath);
        if (uiDrive == 0) {
            uiDrive = DriveOf(std::filesystem::current_path(kError).string());
        }
        apStat->st_mode = static_cast<uint16_t>(uiMode);
        apStat->st_nlink = 1;
        apStat->st_size = static_cast<int32_t>(ullSize);
        apStat->st_mtime = iWriteTime;
        apStat->st_atime = iWriteTime;
        apStat->st_ctime = iWriteTime;
        apStat->st_dev = uiDrive;
        apStat->st_rdev = uiDrive;
        return 0;
    }
} // namespace rt
