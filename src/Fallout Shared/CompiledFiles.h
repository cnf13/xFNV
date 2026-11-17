#pragma once

#include "Reconstruction.hpp"

class TESFile;

class CompiledFiles {
public:
	uint32_t uiCompiledFileCount;
	TESFile* pFileArray[255];
};
