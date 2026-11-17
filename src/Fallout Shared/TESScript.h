#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
struct SCRIPT_LOCAL;
class Script;
class TESForm;

// State of a script effect (spell effect script) the script instance carries.
struct SCRIPT_EFFECT_DATA {
	bool m_bScriptEffectStart;
	bool m_bScriptEffectFinish;
	float m_fSecondsElapsed;
};

// One form a script instance has action flags for, and those flags.
struct ACTION_OBJECT {
	TESForm* pForm;
	uint32_t iFlags;
};

// The action flags and local variables a script instance keeps in its extra data list.
class ScriptLocals {
public:
	ScriptLocals();
	~ScriptLocals();

	// The bytes the locals take in a save.
	uint16_t GetSaveSize();
	// Clears the flags of every action the script has.
	void ClearActionFlags();
	bool CheckActionFlag(TESForm* apActionForm, uint32_t aiFlag);
	bool SetActionFlag(TESForm* apActionForm, uint32_t aiFlag);
	// Finds the action of a form, or, when asked, the action whose form is not known yet.
	ACTION_OBJECT* GetActionObject(TESForm* apActionForm, bool abReturnDefault);
	// Deletes every action and the list itself.
	void ClearActionList();
	// Deletes every local variable and the list itself.
	void ClearVariableList();
	// Adds an action for the form.
	void AddActionObject(TESForm* apActionForm);
	// The value of local variable auiID of apScript (thiscall, ret 8, ST0, 0x5A9140).
	double GetVariable(uint32_t auiID, Script* apScript);
	void Revert(BGSLoadFormBuffer* apLoadGameBuffer);
	// Writes the locals to the old-format save.
	void SaveGame();
	// Reads the locals from the old-format save.
	void LoadGame();
	// Writes the locals to the save buffer.
	void SaveGame(BGSSaveFormBuffer* apBuffer);
	void SetVariable(uint32_t auiID, double value);
	bool IsReferenceVariable(uint32_t auiID);
	// Replaces the actions with copies of apCopy's.
	void CopyActionList(ScriptLocals* pCopy);
	void Revert_0();
	void LoadGame(BGSLoadFormBuffer* apLoadGameBuffer);

	// The exe's deleting destructor (0x41AF70) frees through ourFree.
	static void operator delete(void* apMemory);
	// Forgets the per-thread last script variable lookup.
	static void ClearOptimizations();

	Script* m_pMasterScript;
	uint8_t m_cFlags;
	BSSimpleList<ACTION_OBJECT*>* m_pActionList;
	BSSimpleList<SCRIPT_LOCAL*>* m_pLocalList;
	SCRIPT_EFFECT_DATA* m_pScriptEffectData;
};

// The game settings TESScript.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fQuestScriptDelayTime;
inline int32_t iCurrentScriptLine;
