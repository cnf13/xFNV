// Paths on POSIX: game paths (Windows separators, any case) resolved against the real file system, and the
// FindFirstFileA view of a folder.
#include "platform/Paths.h"

#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <unordered_map>

#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace platform {

    namespace {

        char Lower(char c) {
            return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
        }

        char Upper(char c) {
            return c >= 'a' && c <= 'z' ? static_cast<char>(c - ('a' - 'A')) : c;
        }

        std::string Lowered(std::string_view text) {
            std::string result(text);
            std::transform(result.begin(), result.end(), result.begin(), Lower);
            return result;
        }

        // One folder's entries by lower-cased name, kept while the folder's identity and modification time are
        // unchanged (every create, delete or rename in it moves the time), so no caller has to report its changes.
        struct FolderCache {
            dev_t device;
            ino_t inode;
            timespec modified;
            // Lower-cased name -> the real name; of names differing only in case, the first in byte order.
            std::unordered_map<std::string, std::string> names;
        };

        std::mutex gFoldersLock;
        // Made on first use and never destroyed, so the game's static constructors and destructors may resolve paths.
        std::unordered_map<std::string, FolderCache>& Folders() {
            static auto* folders = new std::unordered_map<std::string, FolderCache>;
            return *folders;
        }

        bool SameTime(const timespec& a, const timespec& b) {
            return a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec;
        }

        // The real name of `name` in `folder` (a native path, "" for the working folder); empty when there is none.
        std::string RealName(const std::string& folder, std::string_view name) {
            const std::string path = folder.empty() ? std::string(".") : folder;
            struct stat info;
            if (stat(path.c_str(), &info) != 0 || !S_ISDIR(info.st_mode)) {
                return {};
            }
            std::lock_guard lock(gFoldersLock);
            FolderCache& cache = Folders()[path];
            if (cache.device != info.st_dev || cache.inode != info.st_ino || !SameTime(cache.modified, info.st_mtim)) {
                cache.device = info.st_dev;
                cache.inode = info.st_ino;
                cache.modified = info.st_mtim;
                cache.names.clear();
                if (DIR* dir = opendir(path.c_str())) {
                    while (const dirent* entry = readdir(dir)) {
                        const std::string real = entry->d_name;
                        auto [it, added] = cache.names.try_emplace(Lowered(real), real);
                        if (!added && real < it->second) {
                            it->second = real;
                        }
                    }
                    closedir(dir);
                }
            }
            const auto it = cache.names.find(Lowered(name));
            return it == cache.names.end() ? std::string() : it->second;
        }

        std::string Join(const std::string& folder, std::string_view name) {
            if (folder.empty()) {
                return std::string(name);
            }
            std::string result = folder;
            if (result.back() != '/') {
                result += '/';
            }
            result += name;
            return result;
        }

        bool Exists(const std::string& path) {
            struct stat info;
            return stat(path.c_str(), &info) == 0;
        }

        // Upper-cased byte order, NTFS's directory order for the names the game uses (ASCII).
        bool NtfsLess(const std::string& a, const std::string& b) {
            const size_t count = std::min(a.size(), b.size());
            for (size_t i = 0; i < count; ++i) {
                const unsigned char x = static_cast<unsigned char>(Upper(a[i]));
                const unsigned char y = static_cast<unsigned char>(Upper(b[i]));
                if (x != y) {
                    return x < y;
                }
            }
            return a.size() < b.size();
        }

        // FindFirstFileA's wildcards: `*` any run, `?` one character, case-insensitive; `*.*` matches every name.
        bool Matches(std::string_view pattern, std::string_view name) {
            if (pattern == "*.*") {
                pattern = "*";
            }
            size_t p = 0;
            size_t n = 0;
            size_t starP = std::string_view::npos;
            size_t starN = 0;
            while (n < name.size()) {
                if (p < pattern.size() && (pattern[p] == '?' || Lower(pattern[p]) == Lower(name[n]))) {
                    ++p;
                    ++n;
                }
                else if (p < pattern.size() && pattern[p] == '*') {
                    starP = p++;
                    starN = n;
                }
                else if (starP != std::string_view::npos) {
                    p = starP + 1;
                    n = ++starN;
                }
                else {
                    return false;
                }
            }
            while (p < pattern.size() && pattern[p] == '*') {
                ++p;
            }
            return p == pattern.size();
        }

        // The FILETIME of a time: 100 ns units since 1601-01-01.
        uint64_t FileTime(const statx_timestamp& time) {
            return static_cast<uint64_t>(time.tv_sec) * 10000000u + static_cast<uint64_t>(time.tv_nsec) / 100u +
                116444736000000000u;
        }

        // The entry FindFirstFileA reports for a file, with Wine's attributes for POSIX files: directory, else archive;
        // read-only without the owner's write permission; hidden for a dot file. The creation time is the birth time
        // where the file system records one, else the last write time.
        bool Entry(const std::string& path, const std::string& name, FileEntry& entry) {
            struct statx info;
            if (statx(AT_FDCWD, path.c_str(), 0, STATX_BASIC_STATS | STATX_BTIME, &info) != 0) {
                return false;
            }
            entry.name = name;
            const bool directory = S_ISDIR(info.stx_mode);
            entry.size = directory ? 0 : static_cast<uint64_t>(info.stx_size);
            entry.attributes = directory ? kAttributeDirectory : kAttributeArchive;
            if (!(info.stx_mode & S_IWUSR)) {
                entry.attributes |= kAttributeReadOnly;
            }
            if (name.size() > 1 && name[0] == '.' && name != "..") {
                entry.attributes |= kAttributeHidden;
            }
            entry.writeTime = FileTime(info.stx_mtime);
            entry.creationTime = (info.stx_mask & STATX_BTIME) ? FileTime(info.stx_btime) : entry.writeTime;
            entry.accessTime = FileTime(info.stx_atime);
            entry.shortName.clear();
            return true;
        }

        std::string HomeFolder() {
            if (const char* home = std::getenv("HOME"); home && *home) {
                return home;
            }
            if (const passwd* user = getpwuid(getuid())) {
                return user->pw_dir;
            }
            return ".";
        }

        std::mutex gResolvedLock;
        // The paths NativePath resolved whole (every component found), by the given path with `/` separators: a path
        // read again and again (FALLOUT.INI's wrong case on each of the ~1,000 ini reads of a start) costs one stat
        // while its resolution still exists. Made on first use and never destroyed, as Folders().
        std::unordered_map<std::string, std::string>& Resolved() {
            static auto* resolved = new std::unordered_map<std::string, std::string>;
            return *resolved;
        }

    } // namespace

    std::filesystem::path NativePath(std::string_view gamePath) {
        std::string path(gamePath);
        std::replace(path.begin(), path.end(), '\\', '/');
        if (path.empty() || Exists(path)) {
            return path;
        }
        std::string cached;
        {
            std::lock_guard lock(gResolvedLock);
            if (const auto it = Resolved().find(path); it != Resolved().end()) {
                cached = it->second;
            }
        }
        if (!cached.empty() && Exists(cached)) {
            return cached;
        }
        std::string resolved = path[0] == '/' ? "/" : "";
        size_t start = 0;
        while (start < path.size()) {
            size_t end = path.find('/', start);
            if (end == std::string::npos) {
                end = path.size();
            }
            const std::string_view component(path.data() + start, end - start);
            if (!component.empty()) {
                if (component == "." || component == "..") {
                    resolved = Join(resolved, component);
                }
                else {
                    const std::string real = RealName(resolved, component);
                    if (real.empty()) {
                        // Missing from here on: the rest keeps the given case.
                        resolved = Join(resolved, std::string_view(path).substr(start));
                        return resolved;
                    }
                    resolved = Join(resolved, real);
                }
            }
            start = end + 1;
        }
        std::lock_guard lock(gResolvedLock);
        Resolved()[path] = resolved;
        return resolved;
    }

    std::vector<FileEntry> FindFiles(std::string_view gamePattern) {
        std::vector<FileEntry> entries;
        const size_t slash = gamePattern.find_last_of("\\/");
        const std::string_view pattern = slash == std::string_view::npos ? gamePattern : gamePattern.substr(slash + 1);
        if (pattern.empty()) {
            return entries;
        }
        if (pattern.find_first_of("*?") == std::string_view::npos) {
            // One name: the file's own spelling on disk.
            const std::string path = NativePath(gamePattern).string();
            const size_t nameStart = path.find_last_of('/') + 1;
            FileEntry entry;
            if (Entry(path, path.substr(nameStart), entry)) {
                entries.push_back(std::move(entry));
            }
            return entries;
        }
        std::string folder = slash == std::string_view::npos ? std::string()
                                                             : NativePath(gamePattern.substr(0, slash + 1)).string();
        DIR* dir = opendir(folder.empty() ? "." : folder.c_str());
        if (!dir) {
            return entries;
        }
        while (const dirent* found = readdir(dir)) {
            const std::string name = found->d_name;
            FileEntry entry;
            if (Matches(pattern, name) && Entry(Join(folder, name), name, entry)) {
                entries.push_back(std::move(entry));
            }
        }
        closedir(dir);
        std::sort(entries.begin(), entries.end(), [](const FileEntry& a, const FileEntry& b) {
            const int rankA = a.name == "." ? 0 : a.name == ".." ? 1 : 2;
            const int rankB = b.name == "." ? 0 : b.name == ".." ? 1 : 2;
            if (rankA != rankB) {
                return rankA < rankB;
            }
            if (NtfsLess(a.name, b.name)) {
                return true;
            }
            return !NtfsLess(b.name, a.name) && a.name < b.name;
        });
        return entries;
    }

    std::string UserFolder(UserFolderKind kind) {
        std::string data;
        if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg == '/') {
            data = xdg;
        }
        else {
            data = HomeFolder() + "/.local/share";
        }
        while (data.size() > 1 && data.back() == '/') {
            data.pop_back();
        }
        // SHGetFolderPathA's folders exist; the game creates only its own subfolders in them.
        const std::string folder = kind == UserFolderKind::LocalAppData ? data : data + "/FalloutNV/Documents";
        std::error_code error;
        std::filesystem::create_directories(folder, error);
        return folder;
    }

    std::string InstallFolder() {
        std::error_code error;
        const std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", error);
        if (error) {
            return "./";
        }
        std::string folder = exe.parent_path().string();
        if (folder.empty() || folder.back() != '/') {
            folder += '/';
        }
        return folder;
    }

    std::string UserName() {
        if (const passwd* user = getpwuid(getuid())) {
            return user->pw_name;
        }
        if (const char* name = std::getenv("USER")) {
            return name;
        }
        return {};
    }

} // namespace platform
