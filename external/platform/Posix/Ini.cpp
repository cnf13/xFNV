// Ini files on POSIX: GetPrivateProfile*/WritePrivateProfile* to kernel32's rules, as Wine implements them (its
// profile.c follows Windows' observed behaviour). Reads see the file as Wine parses it; a write edits the file in
// place, keeping every other line, as Windows does (Wine rewrites the whole file).
#include "platform/Ini.h"
#include "platform/Paths.h"

#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <sys/stat.h>

namespace platform {

namespace {

// Wine's PROFILE_isspaceW for the ASCII range: C's spaces and the DOS end-of-file mark.
bool IsSpace(char c) {
	return c == ' ' || (c >= '\t' && c <= '\r') || c == 0x1A;
}

std::string_view Trimmed(std::string_view text) {
	while (!text.empty() && IsSpace(text.front())) {
		text.remove_prefix(1);
	}
	while (!text.empty() && IsSpace(text.back())) {
		text.remove_suffix(1);
	}
	return text;
}

char Lower(char c) {
	return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
}

bool EqualNoCase(std::string_view a, std::string_view b) {
	if (a.size() != b.size()) {
		return false;
	}
	for (size_t i = 0; i < a.size(); ++i) {
		if (Lower(a[i]) != Lower(b[i])) {
			return false;
		}
	}
	return true;
}

struct Key {
	std::string name;
	// Missing for a line without `=`.
	std::optional<std::string> value;
	size_t line;
};

struct Section {
	std::string name;
	// The `[name]` line; npos for the nameless section before the first header.
	size_t header;
	std::vector<Key> keys;
	// One past the section's last line.
	size_t end;
};

// A file as lines (without their line ends) and the sections and keys Wine's PROFILE_Load reads from them.
struct Profile {
	std::vector<std::string> lines;
	std::vector<Section> sections;

	void Parse() {
		sections.clear();
		sections.push_back({"", std::string::npos, {}, 0});
		// Whether the section has an entry yet, and whether its last one has a name.
		bool hasPrevious = false;
		bool previousNamed = false;
		for (size_t i = 0; i < lines.size(); ++i) {
			const std::string_view line = Trimmed(lines[i]);
			sections.back().end = i + 1;
			if (line.empty()) {
				continue;
			}
			if (line.front() == '[') {
				const size_t close = line.rfind(']');
				if (close != std::string_view::npos) {
					sections.back().end = i;
					// The name between the brackets, trimmed as Wine does.
					sections.push_back({std::string(Trimmed(line.substr(1, close - 1))), i, {}, i + 1});
					hasPrevious = false;
					continue;
				}
			}
			std::string_view name = line;
			std::optional<std::string> value;
			const size_t equals = line.find('=');
			if (equals != std::string_view::npos) {
				name = Trimmed(line.substr(0, equals));
				value = std::string(Trimmed(line.substr(equals + 1)));
			}
			// Wine keeps a nameless entry only when the one before it has a name.
			if (!name.empty() || !hasPrevious || previousNamed) {
				sections.back().keys.push_back({std::string(name), std::move(value), i});
				hasPrevious = true;
				previousNamed = !name.empty();
			}
		}
	}

	Section* FindSection(std::string_view name) {
		for (Section& section : sections) {
			if (EqualNoCase(section.name, name)) {
				return &section;
			}
		}
		return nullptr;
	}

	// The first matching section's first matching key: a later section of the same name is not searched.
	Key* FindKey(std::string_view sectionName, std::string_view keyName) {
		Section* section = FindSection(sectionName);
		if (!section) {
			return nullptr;
		}
		for (Key& key : section->keys) {
			if (EqualNoCase(key.name, keyName)) {
				return &key;
			}
		}
		return nullptr;
	}
};

struct CachedFile {
	bool exists;
	dev_t device;
	ino_t inode;
	off_t size;
	timespec modified;
	Profile profile;
};

std::mutex gLock;
// Made on first use and never destroyed, so the game's static constructors and destructors may read ini files.
std::unordered_map<std::string, CachedFile>& Files() {
	static auto* files = new std::unordered_map<std::string, CachedFile>;
	return *files;
}

// Lines split as Wine's PROFILE_Load does: at `\n`, or at `\r` where no `\n` follows; one `\r` before a `\n` is
// part of the line end.
void SplitLines(const std::string& text, std::vector<std::string>& lines) {
	size_t start = 0;
	while (start < text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string::npos) {
			end = text.find('\r', start);
		}
		if (end == std::string::npos) {
			end = text.size();
		}
		size_t lineEnd = end;
		if (lineEnd > start && text[end] == '\n' && text[lineEnd - 1] == '\r') {
			--lineEnd;
		}
		lines.emplace_back(text, start, lineEnd - start);
		start = end + 1;
	}
}

// The file's profile, read again when it changed since the last call; nullptr when it cannot be read. A missing
// file reads as empty; a missing folder makes it unreadable (Wine's PROFILE_Open fails on ERROR_PATH_NOT_FOUND).
Profile* Load(const std::string& path) {
	struct stat info;
	const bool exists = stat(path.c_str(), &info) == 0;
	if (!exists) {
		const size_t slash = path.find_last_of('/');
		const std::string folder = slash == std::string::npos ? "." : slash == 0 ? "/" : path.substr(0, slash);
		struct stat folderInfo;
		if (stat(folder.c_str(), &folderInfo) != 0 || !S_ISDIR(folderInfo.st_mode)) {
			return nullptr;
		}
	}
	CachedFile& cached = Files()[path];
	if (exists == cached.exists && (!exists || (cached.device == info.st_dev && cached.inode == info.st_ino
			&& cached.size == info.st_size && cached.modified.tv_sec == info.st_mtim.tv_sec
			&& cached.modified.tv_nsec == info.st_mtim.tv_nsec)) && !cached.profile.sections.empty()) {
		return &cached.profile;
	}
	cached.profile.lines.clear();
	cached.exists = exists;
	if (exists) {
		std::FILE* file = std::fopen(path.c_str(), "rb");
		if (!file) {
			Files().erase(path);
			return nullptr;
		}
		std::string text;
		char buffer[16384];
		size_t count;
		while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0) {
			text.append(buffer, count);
		}
		std::fclose(file);
		SplitLines(text, cached.profile.lines);
		cached.device = info.st_dev;
		cached.inode = info.st_ino;
		cached.size = info.st_size;
		cached.modified = info.st_mtim;
	}
	cached.profile.Parse();
	return &cached.profile;
}

bool Save(const std::string& path, Profile& profile) {
	std::string text;
	for (const std::string& line : profile.lines) {
		text += line;
		text += "\r\n";
	}
	std::FILE* file = std::fopen(path.c_str(), "wb");
	if (!file) {
		Files().erase(path);
		return false;
	}
	const bool written = std::fwrite(text.data(), 1, text.size(), file) == text.size();
	const bool closed = std::fclose(file) == 0;
	// The cache's stamp is the file's new one, so the next read keeps the edited profile.
	struct stat info;
	CachedFile& cached = Files()[path];
	if (stat(path.c_str(), &info) == 0) {
		cached.exists = true;
		cached.device = info.st_dev;
		cached.inode = info.st_ino;
		cached.size = info.st_size;
		cached.modified = info.st_mtim;
	} else {
		Files().erase(path);
	}
	profile.Parse();
	return written && closed;
}

// lstrcpynA: at most count - 1 characters and the NUL.
void CopyN(char* out, const char* text, size_t count) {
	while (count > 1 && *text) {
		*out++ = *text++;
		--count;
	}
	if (count) {
		*out = '\0';
	}
}

// Wine's PROFILE_CopyEntry: the value without one pair of enclosing quotes (the closing one only when it fits).
void CopyEntry(std::span<char> out, const char* value) {
	char quote = '\0';
	if ((*value == '\'' || *value == '"') && value[1] && value[std::strlen(value) - 1] == *value) {
		quote = *value++;
	}
	CopyN(out.data(), value, out.size());
	if (quote && out.size() >= std::strlen(value)) {
		out[std::strlen(out.data()) - 1] = '\0';
	}
}

// Wine's PROFILE_GetSection for key names: each name truncated to the room left, the list ending in two NULs;
// truncated tells whether the list did not fit.
uint32_t ListKeys(Profile& profile, const char* sectionName, std::span<char> out, bool& truncated) {
	Section* section = profile.FindSection(sectionName);
	if (!section || out.size() < 2) {
		out[0] = '\0';
		if (out.size() > 1) {
			out[1] = '\0';
		}
		return 0;
	}
	char* buffer = out.data();
	size_t length = out.size();
	for (const Key& key : section->keys) {
		if (length <= 2) {
			break;
		}
		if ((key.name.empty() && !key.value) || (!key.name.empty() && key.name[0] == ';') || !key.value) {
			continue;
		}
		CopyN(buffer, key.name.c_str(), length - 1);
		const size_t copied = std::strlen(buffer);
		length -= copied + 1;
		buffer += copied + 1;
		if (length < 2) {
			break;
		}
	}
	*buffer = '\0';
	if (length <= 1) {
		buffer[-1] = '\0';
		truncated = true;
		return static_cast<uint32_t>(out.size() - 2);
	}
	return static_cast<uint32_t>(out.size() - length);
}

// GetPrivateProfileStringA's default: trailing spaces removed (a lone character is kept).
std::string Default(const char* value) {
	std::string result = value ? value : "";
	size_t end = result.size();
	while (end > 1 && result[end - 1] == ' ') {
		--end;
	}
	result.resize(end);
	return result;
}


// The A function's copy of the wide result (Wine's GetPrivateProfileStringA): the result's characters and a NUL,
// nothing past them. A truncated key list also gets its documented second NUL in the last byte.
uint32_t Deliver(const std::string& scratch, uint32_t result, std::span<char> out, bool truncatedList) {
	std::memcpy(out.data(), scratch.data(), result);
	out[result] = '\0';
	if (truncatedList) {
		out[out.size() - 1] = '\0';
	}
	return result;
}

thread_local std::string tScratch;

}  // namespace

uint32_t IniGetString(const char* file, const char* section, const char* key, const char* defaultValue,
	std::span<char> out) {
	if (out.empty()) {
		return 0;
	}
	if (!section) {
		// Section names: not used by the game.
		out[0] = '\0';
		return 0;
	}
	if (!key) {
		return IniGetKeys(file, section, defaultValue, out);
	}
	const std::string fallback = Default(defaultValue);
	tScratch.assign(out.size(), '\0');
	const std::span<char> scratch(tScratch.data(), out.size());
	std::lock_guard lock(gLock);
	Profile* profile = Load(NativePath(file).string());
	if (!profile) {
		// An unreadable file (or a missing folder): the default as it is, quotes kept.
		CopyN(scratch.data(), fallback.c_str(), scratch.size());
	} else {
		const Key* found = profile->FindKey(Trimmed(section), Trimmed(key));
		CopyEntry(scratch, found && found->value ? found->value->c_str() : fallback.c_str());
	}
	return Deliver(tScratch, static_cast<uint32_t>(std::strlen(scratch.data())), out, false);
}

uint32_t IniGetKeys(const char* file, const char* section, const char* defaultValue, std::span<char> out) {
	if (out.empty()) {
		return 0;
	}
	const std::string fallback = Default(defaultValue);
	tScratch.assign(out.size(), '\0');
	const std::span<char> scratch(tScratch.data(), out.size());
	std::lock_guard lock(gLock);
	Profile* profile = Load(NativePath(file).string());
	if (!profile) {
		CopyN(scratch.data(), fallback.c_str(), scratch.size());
		return Deliver(tScratch, static_cast<uint32_t>(std::strlen(scratch.data())), out, false);
	}
	bool truncated = false;
	const uint32_t result = ListKeys(*profile, section, scratch, truncated);
	if (!scratch[0]) {
		CopyEntry(scratch, fallback.c_str());
		return Deliver(tScratch, static_cast<uint32_t>(std::strlen(scratch.data())), out, false);
	}
	return Deliver(tScratch, result, out, truncated);
}

uint32_t IniGetInt(const char* file, const char* section, const char* key, int32_t defaultValue) {
	char buffer[30];
	if (IniGetString(file, section, key, "", buffer) == 0) {
		return static_cast<uint32_t>(defaultValue);
	}
	// RtlCharToInteger with base 0, as Wine's GetPrivateProfileIntW reads the value.
	const char* text = buffer;
	while (*text && static_cast<unsigned char>(*text) <= ' ') {
		++text;
	}
	bool negative = false;
	if (*text == '+') {
		++text;
	} else if (*text == '-') {
		negative = true;
		++text;
	}
	uint32_t base = 10;
	if (text[0] == '0' && (text[1] == 'b' || text[1] == 'o' || text[1] == 'x')) {
		base = text[1] == 'b' ? 2 : text[1] == 'o' ? 8 : 16;
		text += 2;
	}
	uint32_t total = 0;
	for (; *text; ++text) {
		const char c = *text;
		uint32_t digit;
		if (c >= '0' && c <= '9') {
			digit = static_cast<uint32_t>(c - '0');
		} else if (c >= 'A' && c <= 'Z') {
			digit = static_cast<uint32_t>(c - 'A' + 10);
		} else if (c >= 'a' && c <= 'z') {
			digit = static_cast<uint32_t>(c - 'a' + 10);
		} else {
			break;
		}
		if (digit >= base) {
			break;
		}
		total = total * base + digit;
	}
	return negative ? 0u - total : total;
}

bool IniWriteString(const char* file, const char* section, const char* key, const char* value) {
	if (!section) {
		return false;
	}
	std::lock_guard lock(gLock);
	const std::string path = NativePath(file).string();
	Profile* profile = Load(path);
	if (!profile) {
		return false;
	}
	const std::string_view sectionName = Trimmed(section);
	Section* found = profile->FindSection(sectionName);
	std::vector<std::string>& lines = profile->lines;
	if (!key) {
		// Deletes the first section of the name: its header and every line up to the next one.
		if (!found) {
			return true;
		}
		if (found->header == std::string::npos) {
			lines.erase(lines.begin(), lines.begin() + static_cast<ptrdiff_t>(found->end));
		} else {
			lines.erase(lines.begin() + static_cast<ptrdiff_t>(found->header),
				lines.begin() + static_cast<ptrdiff_t>(found->end));
		}
		return Save(path, *profile);
	}
	const std::string_view keyName = Trimmed(key);
	Key* existing = profile->FindKey(sectionName, keyName);
	if (!value) {
		if (!existing) {
			return true;
		}
		lines.erase(lines.begin() + static_cast<ptrdiff_t>(existing->line));
		return Save(path, *profile);
	}
	std::string_view text = value;
	while (!text.empty() && IsSpace(text.front())) {
		text.remove_prefix(1);
	}
	if (existing) {
		if (existing->value && *existing->value == text) {
			return true;
		}
		lines[existing->line] = existing->name + "=" + std::string(text);
	} else {
		std::string line = std::string(keyName) + "=" + std::string(text);
		if (found) {
			// After the section's last entry, or right after its header when it has none.
			size_t at = found->keys.empty() ? (found->header == std::string::npos ? 0 : found->header + 1)
				: found->keys.back().line + 1;
			lines.insert(lines.begin() + static_cast<ptrdiff_t>(at), std::move(line));
		} else {
			lines.push_back("[" + std::string(sectionName) + "]");
			lines.push_back(std::move(line));
		}
	}
	return Save(path, *profile);
}

}  // namespace platform
