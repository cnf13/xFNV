#pragma once

#include "BSSystem/BSFile.h"

// A file read from and written to a memory buffer. Only its members are modelled so far.
class BSMemoryFile : public BSFile {
public:
	~BSMemoryFile() override;

	uint32_t uiMemBufferSize;
	uint8_t* pucMemBuffer;
	uint32_t uiCurMemPos;
};
