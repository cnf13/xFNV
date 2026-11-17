#pragma once

#include "BSCore/BSStringT.h"

class TESFile;

// The value and identity of one local variable of a script.
// The pointer after the id is only in the PC layout; nothing here uses it.
struct SCRIPT_LOCAL {
	// From Construct_005E28E0.
	SCRIPT_LOCAL();
	SCRIPT_LOCAL(const SCRIPT_LOCAL* apSource);

	// Swaps the id and the value read from a big-endian file.
	void Endian();

	uint32_t uiID;
	SCRIPT_LOCAL* pNext;
	double fValue;
	bool bIsInteger;
};

// One local variable of a script, as held in a script instance's local variable list.
class ScriptVariable {
public:
	ScriptVariable();
	ScriptVariable(const ScriptVariable* apSource);
	~ScriptVariable();

	// Reads the variable's data from a 'SLSD' chunk of a plugin.
	void Load(TESFile* apFile);
	bool Compare(ScriptVariable* apCompare);

	// The deleting destructor frees through ourFree.
	static void operator delete(void* apMemory);

	SCRIPT_LOCAL data;
	BSStringT<char> cName;
};
