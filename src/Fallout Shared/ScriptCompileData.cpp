#include "Fallout Shared/ScriptCompileData.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESScriptCompiler.h"
#include "Fallout Shared/TESScriptVariable.h"
#include "Reconstruction.hpp"

ScriptCompileData::ScriptCompileData()
	: uiInputOffset(0), eCompilerIndex(DEFAULT_COMPILER), eLastError(ERROR_SCRIPT_NOERROR), bIsPartialScript(false),
	  uiLastLineNumber(0), pOutput(static_cast<char*>(ourCalloc(1u, 0x4000u))), uiOutputOffset(0),
	  pCurrentScript(nullptr) {
	ourMemset(pOutput, 0, 4u);
	ourMemset(&header, 0, sizeof(header));
	cScriptName.Set("", 0u);
}

ScriptCompileData::~ScriptCompileData() {
	ClearAllScriptLines();
	ClearAllReferenceObjects();
	ClearAllVariables();
	ourFree(pOutput);
}

void ScriptCompileData::ClearAllScriptLines() {
	BSSimpleList<SCRIPT_LINE*>* node = QScriptLineList();
	while (node != nullptr && !node->IsEmpty()) {
		// the original frees the line without checking it for null
		ourFree(node->GetItem());
		node->RemoveHead();
	}
}

void ScriptCompileData::ClearAllReferenceObjects() {
	BSSimpleList<Script::RefVariable*>* node = QReferencedObjectList();
	while (node != nullptr && !node->IsEmpty()) {
		Script::RefVariable* refVariable = node->GetItem();
		if (refVariable != nullptr)
			delete refVariable;
		node->RemoveHead();
	}
}

void ScriptCompileData::ClearAllVariables() {
	BSSimpleList<ScriptVariable*>* node = QVariableList();
	while (node != nullptr && !node->IsEmpty()) {
		ScriptVariable* variable = node->GetItem();
		if (variable != nullptr)
			delete variable;
		node->RemoveHead();
	}
}

bool ScriptCompileData::VerifyReferencedObjectList() {
	BSSimpleList<Script::RefVariable*>* node = GetReferencedObjectList();
	while (node != nullptr && node->GetItem() != nullptr) {
		Script::RefVariable* refVariable = node->GetItem();
		node = node->GetNext();
		if (refVariable->form == nullptr && refVariable->varIdx == 0) {
			ScriptCompiler::ScriptError(this, "Unknown referenced object '%s'.", refVariable->name.String());
			return false;
		}
	}
	return true;
}

BSSimpleList<ScriptVariable*>* ScriptCompileData::QVariableList() {
	return &listVariables;
}
