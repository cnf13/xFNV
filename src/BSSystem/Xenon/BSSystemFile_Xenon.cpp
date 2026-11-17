#include "BSSystem/BSSystemFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "platform/Paths.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <system_error>

// SetFilePointerEx: the new position goes to aruiPos. An unknown aeWhence (ERROR_INVALID_PARAMETER), no stream
// (ERROR_INVALID_HANDLE) or a position before the start (ERROR_NEGATIVE_SEEK) is EC_FAILED. std::fseek and std::ftell
// take a long, 32 bits on Win32: the game's files are below 2 GB.
BSSystemFile::ErrorCode BSSystemFile::DoSeek(uint64_t auiOffset, BSSystemFile::SeekMode aeWhence, uint64_t& auiPos) {
    if (bAllowAsync) {
        return BSSystemFile::EC_FAILED;
    }
    int32_t iOrigin;
    switch (aeWhence) {
        case BSSystemFile::SM_SET: iOrigin = SEEK_SET; break;
        case BSSystemFile::SM_CUR: iOrigin = SEEK_CUR; break;
        case BSSystemFile::SM_END: iOrigin = SEEK_END; break;
        default: return BSSystemFile::EC_FAILED;
    }
    if (!hFile) {
        return BSSystemFile::EC_FAILED;
    }
    errno = 0;
    if (!std::fseek(hFile, static_cast<long>(static_cast<int64_t>(auiOffset)), iOrigin)) {
        const long iPos = std::ftell(hFile);
        if (iPos >= 0) {
            auiPos = static_cast<uint64_t>(iPos);
            return BSSystemFile::EC_NONE;
        }
    }
    return ErrorCodeOf(errno);
}

// Returns EC_FAILED without a stream or on a file opened read-only (AM_RDONLY), which FlushFileBuffers refuses. The
// stream is unbuffered (DoOpen), so std::fflush has nothing to hand on; the commit to the disk itself has no standard
// C++ form.
BSSystemFile::ErrorCode BSSystemFile::DoFlush() {
    if (!hFile || (eAccessMode != BSSystemFile::AM_RDWR && eAccessMode != BSSystemFile::AM_WRONLY)) {
        return BSSystemFile::EC_FAILED;
    }
    return std::fflush(hFile) ? BSSystemFile::EC_FAILED : BSSystemFile::EC_NONE;
}

// Follows CreateFileA with eAccessMode's access (AM_RDONLY GENERIC_READ, AM_RDWR both, AM_WRONLY GENERIC_WRITE) and
// eOpenMode's disposition: OM_NONE OPEN_EXISTING; OM_APPEND OPEN_EXISTING, retried as CREATE_ALWAYS when that fails,
// then a seek to the end unless bAllowAsync; OM_CREAT CREATE_ALWAYS; OM_TRUNC TRUNCATE_EXISTING, which CreateFileA
// refuses without GENERIC_WRITE (ERROR_INVALID_PARAMETER); OM_EXCL CREATE_NEW; any other mode disposition 0
// (ERROR_INVALID_PARAMETER). Each becomes the std::fopen mode that creates, keeps or truncates the file alike; reads
// and writes the access does not grant are refused by DoRead, DoWrite and DoFlush. bAllowAsync's FILE_FLAG_OVERLAPPED
// and FILE_FLAG_NO_BUFFERING and the share mode have no stdio form. The failure's error goes through the GetLastError
// switch (ErrorCodeOf).
BSSystemFile::ErrorCode BSSystemFile::DoOpen(const char* apcPath) {
    const bool bWrite = eAccessMode == BSSystemFile::AM_RDWR || eAccessMode == BSSystemFile::AM_WRONLY;
    const bool bWriteOnly = eAccessMode == BSSystemFile::AM_WRONLY;
    const std::filesystem::path kPath = platform::NativePath(apcPath);
    const std::string kNativePath = kPath.string();
    std::error_code kError;
    int32_t iErrno = 0;
    hFile = nullptr;
    // CreateFileA refuses a folder (ERROR_ACCESS_DENIED), which POSIX's fopen opens for reading.
    if (std::filesystem::is_directory(kPath, kError)) {
        iErrno = EACCES;
    }
    else {
        switch (eOpenMode) {
            case BSSystemFile::OM_NONE:
            case BSSystemFile::OM_APPEND: {
                hFile = std::fopen(kNativePath.c_str(), bWrite ? "r+b" : "rb");
                if (!hFile && eOpenMode == BSSystemFile::OM_APPEND) {
                    hFile = std::fopen(kNativePath.c_str(), bWriteOnly ? "wb" : "w+b");
                }
                iErrno = errno;
                break;
            }
            case BSSystemFile::OM_CREAT: {
                hFile = std::fopen(kNativePath.c_str(), bWriteOnly ? "wb" : "w+b");
                iErrno = errno;
                break;
            }
            case BSSystemFile::OM_TRUNC: {
                if (!bWrite) {
                    iErrno = EINVAL;
                }
                else if (!std::filesystem::exists(kPath, kError)) {
                    iErrno = ENOENT;
                }
                else {
                    hFile = std::fopen(kNativePath.c_str(), bWriteOnly ? "wb" : "w+b");
                    iErrno = errno;
                }
                break;
            }
            case BSSystemFile::OM_EXCL: {
                // CREATE_NEW: ERROR_FILE_EXISTS on an existing file. fopen's exclusive "x" is C11 only: the Win32
                // build's UCRT rejects it, so the existence is tested first (not atomic against another process
                // creating it).
                if (std::filesystem::exists(kPath, kError)) {
                    iErrno = EEXIST;
                }
                else {
                    hFile = std::fopen(kNativePath.c_str(), bWriteOnly ? "wb" : "w+b");
                    iErrno = errno;
                }
                break;
            }
            default: {
                iErrno = EINVAL;
                break;
            }
        }
    }
    BSSystemFile::ErrorCode eResult = BSSystemFile::EC_NONE;
    if (hFile) {
        // A Win32 handle has no buffer of its own: each write reaches the file at once.
        std::setvbuf(hFile, nullptr, _IONBF, 0);
    }
    else {
        eResult = ErrorCodeOf(iErrno);
        // fopen gives ENOENT for a missing folder too, where CreateFileA gives ERROR_PATH_NOT_FOUND.
        const std::filesystem::path kFolder = kPath.parent_path();
        if (iErrno == ENOENT && !kFolder.empty() && !std::filesystem::is_directory(kFolder, kError)) {
            eResult = BSSystemFile::EC_INVALID_PATH;
        }
    }
    if (eResult == BSSystemFile::EC_NONE && eOpenMode == BSSystemFile::OM_APPEND && !bAllowAsync) {
        uint64_t uiPos;
        return DoSeek(0, BSSystemFile::SM_END, uiPos);
    }
    return eResult;
}

// CopyFileA: 0 when copied, 4 when not; a3 (bFailIfExists) refuses an existing destination. CopyFileA keeps the
// source's last write time, so the copy gets it too.
int32_t BSSystemFile::CopyFileA(const int8_t* lpExistingFileName, const int8_t* lpNewFileName, bool a3) {
    std::error_code kError;
    const std::filesystem::path kFrom =
        platform::NativePath(static_cast<const char*>(static_cast<const void*>(lpExistingFileName)));
    const std::filesystem::path kTo =
        platform::NativePath(static_cast<const char*>(static_cast<const void*>(lpNewFileName)));
    if (!std::filesystem::copy_file(
            kFrom, kTo, a3 ? std::filesystem::copy_options::none : std::filesystem::copy_options::overwrite_existing,
            kError
        )) {
        return 4;
    }
    std::filesystem::last_write_time(kTo, std::filesystem::last_write_time(kFrom, kError), kError);
    return 0;
}
