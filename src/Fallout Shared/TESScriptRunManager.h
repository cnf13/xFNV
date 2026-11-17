#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

enum EXPRESSION_ERROR : int32_t;
enum SCRIPT_ERROR : int32_t;
enum SCRIPT_OUTPUT : int32_t;
class Script;
class ScriptLocals;
class TESGlobal;
class TESObject;
class TESObjectREFR;
class TESTopicInfo;

// The state of the script pass running on a thread: the objects it runs on, the running script
// and its nested if/while loop stacks.
class ScriptRunner {
public:
	// Clears the runner's whole state (0x5E0C60; the exe calls it out of line).
	ScriptRunner();

	// True while a script is running.
	bool IsRunning();

	// Runs apScript on apOwnerObject with its locals (cloned for the run when none are given), line by line; times
	// the run when the debug text shows. Returns whether the script stopped its object's script execution.
	bool Run(Script* apScript, TESObjectREFR* apOwnerObject, ScriptLocals* apOwnerVars, TESObjectREFR* apOwnerContainer,
		bool abIsPartialScript, bool abScriptEffectStart, bool abScriptEffectFinish, float afSecondsElapsed);
	// Runs apScript's lines as an action list init on apOwnerObject with apOwnerVars.
	bool InitActionList(Script* apScript, TESObjectREFR* apOwnerObject, ScriptLocals* apOwnerVars);
	// Reads the opcode of the line at auiDataOffset, the reference it is called on (for a dot operator) and its
	// size; false after a script error when the reference is unknown.
	bool GetNextLine(Script* apScript, SCRIPT_OUTPUT& aeNextExp, uint32_t& auiDataOffset, int32_t& aiLineSize,
		TESObjectREFR** appRefObject, bool abSkip);

	// Evaluates one line: a block header, a SET, an if/else flow line or a function call.
	bool EvaluateLine(Script* apScript, SCRIPT_OUTPUT aeExp, TESObjectREFR* apFunctionOwner, int32_t& aiLineSize,
		uint32_t auiDataOffset, uint32_t& aiSkipLines, uint32_t auiCurrentLine, bool abEndian, bool abActionInit);
	// Prints the formatted error to the console (or as a warning), clears the script's compiled data length and
	// flags the error (cdecl member, varargs).
	void ScriptError(Script* apScript, const char* text, ...);

protected:
	void SetError(Script* apScript, SCRIPT_ERROR errorCode);

public:
	// the container the current script runs on
	TESObjectREFR* m_currentContainer;
	// the object the current script runs on
	TESObject* m_currentObject;
	// the object's script variables
	ScriptLocals* m_currentVars;
	// the dialog info a script runs from, when it does
	TESTopicInfo* m_currentInfo;
	// the runner's global scope
	TESGlobal* m_global;
	// the script being executed
	Script* m_scriptRunning;
	// the last script error
	SCRIPT_ERROR m_error;
	// the last expression error
	EXPRESSION_ERROR m_expError;
	// current if nesting depth
	uint32_t m_ifNested;
	// one flag per open if block
	uint32_t m_ifFlags[10];
	// current while nesting depth
	uint32_t m_whileNested;
	// one flag per open while block
	uint32_t m_whileFlags[10];
	// one resume offset per open while block
	uint32_t m_whileOffset[10];
	bool m_bScriptErrorFound;
	bool m_bStopObjectScriptExecution;
	uint8_t paddingA2[2];
};

// Runs scripts through the ScriptRunner of the calling thread.
class ScriptRunManager {
public:
	ScriptRunManager();
	~ScriptRunManager();

	// Runs apScript on apOwnerObject with its locals.
	bool Run(Script* apScript, TESObjectREFR* apOwnerObject, ScriptLocals* apOwnerVars, TESObjectREFR* apOwnerContainer,
		bool abIsPartialScript, bool abScriptEffectStart, bool abScriptEffectFinish, float afSecondsElapsed);
	// Runs apScript's action list init on apOwnerObject with apOwnerVars, creating the runner on first use. A runner
	// that is already running gets a temporary one. Always true.
	bool InitActionList(Script* apScript, TESObjectREFR* apOwnerObject, ScriptLocals* apOwnerVars);

	// The singleton, a guarded function-local static.
	static ScriptRunManager* Instance();

	ScriptRunner* m_pScriptRunner;
	ScriptRunner* pCurrentRunner;
};

inline int32_t bIsScriptExecuting;
