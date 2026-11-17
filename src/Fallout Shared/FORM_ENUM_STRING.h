#pragma once

#include "Reconstruction.hpp"

// A form type's id and its name strings (the PDB's FORM_ENUM_STRING; the PDB names no source file).
struct FORM_ENUM_STRING {
	uint8_t cFormID;
	uint8_t padding01[3];
	char* pFormString;
	uint32_t iFormString;
};
