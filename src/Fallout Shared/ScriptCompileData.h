#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "Fallout Shared/Script.h"
#include "Reconstruction.hpp"

enum SCRIPT_ERROR : int32_t;
struct SCRIPT_LINE;
struct SCRIPT_WORD;
class ScriptVariable;

// The script compiler's working data for one script: its input text, compiled output and lists.
class ScriptCompileData {
public:
	// Runs ScriptCompileData's constructor (0x5AE490, thiscall, ret) on apMemory and returns
	// it.
	ScriptCompileData();
	~ScriptCompileData();

	// Frees every compiled script line buffer.
	void ClearAllScriptLines();

	// Deletes every referenced object and empties the list.
	void ClearAllReferenceObjects();

	// Deletes every local variable and empties the list.
	void ClearAllVariables();

	// Fails when a referenced object never resolved to a form.
	bool VerifyReferencedObjectList();

	// Resolves apCurrentWord against the local variables; true when it names one.
	bool GetVariable(SCRIPT_WORD* apCurrentWord);

	// Resolves apCurrentWord against the referenced objects, adding one when allowed.
	bool GetReferencedObject(SCRIPT_WORD* apCurrentWord, bool abAddRefVariable, bool abAllowUnknownRef);

	// The referenced object at index aiIndex.
	Script::RefVariable* GetReferencedObject(uint32_t aiIndex);

	BSSimpleList<ScriptVariable*>* GetVariableList() { return &listVariables; }
	BSSimpleList<Script::RefVariable*>* GetReferencedObjectList() { return &listRefObjects; }
	BSSimpleList<SCRIPT_LINE*>* GetScriptLineList() { return &listLines; }

	// &listLines. The exe calls it out of line where src's inline GetScriptLineList would read the field.
	BSSimpleList<SCRIPT_LINE*>* QScriptLineList() { return &listLines; }

	// &listRefObjects. The exe calls it out of line where src's inline GetReferencedObjectList would read the field.
	BSSimpleList<Script::RefVariable*>* QReferencedObjectList() { return &listRefObjects; }

	// The exe's shared +0x3C address getter (0x5A8080, thiscall): &listVariables. The exe calls it out of line where
	// src's inline GetVariableList would read the field.
	BSSimpleList<ScriptVariable*>* QVariableList();

	// the source text being compiled
	char* input;
	uint32_t uiInputOffset;
	// the compiler pass this data belongs to
	COMPILER_NAME eCompilerIndex;
	BSStringT<char> cScriptName;
	SCRIPT_ERROR eLastError;
	bool bIsPartialScript;
	uint32_t uiLastLineNumber;
	// the compiled output buffer
	char* pOutput;
	uint32_t uiOutputOffset;
	SCRIPT_HEADER header;
	// the script's local variables
	BSSimpleList<ScriptVariable*> listVariables;
	// the forms the source references
	BSSimpleList<Script::RefVariable*> listRefObjects;
	// the script being compiled
	Script* pCurrentScript;
	// the source lines holding compiled code
	BSSimpleList<SCRIPT_LINE*> listLines;
};
