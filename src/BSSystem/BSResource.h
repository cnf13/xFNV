#pragma once

#include "Reconstruction.hpp"

// IDA's resource identity types; not in the PDB. IDA's decompiler text uses them only for two locals that hold a
// TESTextureList (BGSDebris::Load 0x4F9F10, TESTextureListArray::Load 0x48A0D0); they are declared, with IDA's names,
// so that text can be translated.
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
