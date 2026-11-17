#pragma once

#include <cstdint>

namespace BSResource {
// A file: its name word and its extension word.
struct FileID {
	uint32_t uiFile;
	uint32_t uiExt;
};

// A file and its directory word, after the file's two.
struct ID : FileID {
	uint32_t uiDir;
};
} // namespace BSResource
