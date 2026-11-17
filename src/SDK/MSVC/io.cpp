#include "SDK/MSVC/io.h"
#include "SDK/MSVC/crtdefs.h"
#include "SDK/MSVC/errno.h"
#include "platform/Paths.h"

#include <filesystem>
#include <system_error>

// The home of the exe's out-of-line VC9 <io.h> bodies.

namespace rt {

// VC9's _access is _access_s (GetFileAttributesA) with -1 for any error. A directory allows every mode; a file
// allows writing unless it is read-only (on POSIX: its owner may not write it). The file is the game path's native
// path (platform::NativePath).
int32_t _access(const char* apFileName, int32_t aiMode)
{
	if (!apFileName || (aiMode & ~6)) {
		errno = kEINVAL;
		_invalid_parameter_noinfo();
		return -1;
	}
	std::error_code kError;
	const std::filesystem::file_status kStatus = std::filesystem::status(platform::NativePath(apFileName), kError);
	if (!std::filesystem::exists(kStatus)) {
		errno = kENOENT;
		return -1;
	}
	if (std::filesystem::is_directory(kStatus)) {
		return 0;
	}
	if ((aiMode & 2) && (kStatus.permissions() & std::filesystem::perms::owner_write) == std::filesystem::perms::none) {
		errno = kEACCES;
		return -1;
	}
	return 0;
}
} // namespace rt
