#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// Files and folders: what the standard library cannot say on its own (case-insensitive game paths, the order and
// fields of a directory search, the user's folders). src/platform/README says what each function promises.
namespace platform {

    // The native path of a game path (`Data\Meshes\x.nif`, `.\Fallout.ini`). Win32: the path as given. POSIX: `\`
    // becomes `/`; the path as given when it exists, else each component resolved case-insensitively against its
    // folder's entries (a missing component and those after it keep the given case).
    std::filesystem::path NativePath(std::string_view gamePath);

    // FILE_ATTRIBUTE_* bits, as WIN32_FIND_DATAA's dwFileAttributes holds them.
    inline constexpr uint32_t kAttributeReadOnly = 0x1;
    inline constexpr uint32_t kAttributeHidden = 0x2;
    inline constexpr uint32_t kAttributeDirectory = 0x10;
    inline constexpr uint32_t kAttributeArchive = 0x20;
    inline constexpr uint32_t kAttributeNormal = 0x80;

    // One entry of a directory search: WIN32_FIND_DATAA's cFileName, nFileSizeHigh:Low, dwFileAttributes,
    // ftLastWriteTime, ftCreationTime, ftLastAccessTime (each 100 ns units since 1601-01-01 UTC, the FILETIME value)
    // and cAlternateFileName (the 8.3 short name, empty when the file system gives none). POSIX: the creation time is
    // the birth time where the file system records one (statx), else the last write time; no short name.
    struct FileEntry {
        std::string name;
        uint64_t size;
        uint32_t attributes;
        uint64_t writeTime;
        uint64_t creationTime;
        uint64_t accessTime;
        std::string shortName;
    };

    // The entries FindFirstFileA/FindNextFileA give for a game path whose last component may hold `*` and `?`, in
    // their order (NTFS: `.` and `..` first, then by upper-cased name); empty when nothing matches.
    std::vector<FileEntry> FindFiles(std::string_view gamePattern);

    // The folders SHGetFolderPathA gives for CSIDL_LOCAL_APPDATA and CSIDL_PERSONAL, without a trailing separator.
    // POSIX: `$XDG_DATA_HOME` (default `~/.local/share`) and `$XDG_DATA_HOME/FalloutNV/Documents`.
    enum class UserFolderKind { LocalAppData, Documents };
    std::string UserFolder(UserFolderKind kind);

    // The game's install folder with a trailing separator. Win32: the registry's `Installed Path` under
    // HKLM\Software\Bethesda Softworks\FalloutNV (empty when missing). POSIX: the executable's folder.
    std::string InstallFolder();

    // The user's login name (GetUserNameA); empty when unknown.
    std::string UserName();

} // namespace platform
