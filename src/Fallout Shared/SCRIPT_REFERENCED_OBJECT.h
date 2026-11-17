#pragma once

#include "BSCore/BSStringT.h"
#include "Reconstruction.hpp"

class TESForm;

// An object a script refers to by its editor ID: the form it resolved to and the script variable that holds it.
struct SCRIPT_REFERENCED_OBJECT {
	// The constructor: a null editor ID, no form and no variable (thiscall, ret 0,
	// 0x500910); the exe's only copy, which Script::RefVariable::Construct(void*) also names.
	SCRIPT_REFERENCED_OBJECT();

	BSStringT<char> cEditorID;
	TESForm* pForm;
	uint32_t uiVariableID;
};
