#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"

#include <stdint.h>

class BGSLoadFormBuffer;
class BGSSaveFormBuffer;
class Script;
struct SCRIPT_LOCAL;
class TESForm;

// State of a script effect (spell effect script) the script instance carries.
struct SCRIPT_EFFECT_DATA {
    bool m_bScriptEffectStart;
    bool m_bScriptEffectFinish;
    float m_fSecondsElapsed;
};

// The script events an action records for its form (ACTION_OBJECT::iFlags), one bit per event block: a block runs
// when its event's bit is set.
enum SCRIPT_ACTION_FLAG : uint32_t {
    SCRIPT_ACTION_ONADD = 0x1,
    SCRIPT_ACTION_ONEQUIP = 0x2,
    SCRIPT_ACTION_ONDROP = 0x4,
    SCRIPT_ACTION_ONUNEQUIP = 0x8,
    SCRIPT_ACTION_ONDEATH = 0x10,
    SCRIPT_ACTION_ONMURDER = 0x20,
    SCRIPT_ACTION_ONCOMBATEND = 0x40,
    SCRIPT_ACTION_ONHIT = 0x80,
    SCRIPT_ACTION_ONHITWITH = 0x100,
    SCRIPT_ACTION_ONPACKAGESTART = 0x200,
    SCRIPT_ACTION_ONPACKAGEEND = 0x400,
    SCRIPT_ACTION_ONPACKAGECHANGE = 0x800,
    SCRIPT_ACTION_ONLOAD = 0x1000,
    SCRIPT_ACTION_ONMAGICEFFECTHIT = 0x2000,
    SCRIPT_ACTION_ONSELL = 0x4000,
    SCRIPT_ACTION_ONSTARTCOMBAT = 0x8000,
    SCRIPT_ACTION_ONOPEN = 0x10000,
    SCRIPT_ACTION_ONCLOSE = 0x20000,
    SCRIPT_ACTION_ONSAYTODONE = 0x40000,
    SCRIPT_ACTION_ONGRAB = 0x80000,
    SCRIPT_ACTION_ONRELEASE = 0x100000,
    SCRIPT_ACTION_ONDESTRUCTIONSTAGECHANGE = 0x200000,
    SCRIPT_ACTION_ONFIRE = 0x400000,
    SCRIPT_ACTION_ONTRIGGER = 0x10000000,
    SCRIPT_ACTION_ONTRIGGERENTER = 0x20000000,
    SCRIPT_ACTION_ONTRIGGERLEAVE = 0x40000000,
    SCRIPT_ACTION_ONRESET = 0x80000000,
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
    // The value of local variable auiID of apScript.
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

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);
    // Forgets the per-thread last script variable lookup.
    static void ClearOptimizations();

    Script* m_pMasterScript;
    uint8_t m_cFlags;
    BSSimpleList<ACTION_OBJECT*>* m_pActionList;
    BSSimpleList<SCRIPT_LOCAL*>* m_pLocalList;
    SCRIPT_EFFECT_DATA* m_pScriptEffectData;
};

extern SettingT<INISettingCollection> fQuestScriptDelayTime;
inline int32_t iCurrentScriptLine;
