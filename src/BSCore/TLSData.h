#pragma once

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkNiCollisionObject.h"

class BSExtraData;
class ExtraDataList;
class NiNode;
class Script;
struct SCRIPT_LOCAL;
struct SCRIPT_REFERENCED_OBJECT;
class ScriptLocals;
class TESForm;
class TESObjectREFR;

// The per-thread game state kept in a thread-local block: the cached extra data lists, the script run state, the
// loading flags and the memory context. The default member initialisers are the game's initial thread-local values: its
// six nonzero fields.
class TLSData {
public:
    uint8_t padding00[4];
    uint8_t padding04[4];
    ExtraDataList* pLastExtraList;
    uint32_t uiDirtyCount;
    BSExtraData* pLastExtraDatas[147];
    uint8_t padding25C;
    bool bLoadingTempCellData;
    uint8_t padding25E[2];
    // The 3D the background loading thread is building: TESObjectREFR::Get3D loads it
    // from +0x260.
    NiNode* pBackgroundLoading3D;
    // The reference that 3D belongs to: TESObjectREFR::Get3D compares the receiver with
    // it, and ModelLoader's setters store it.
    TESObjectREFR* pBackgroundLoadingRef;
    bool bSystemOutput;
    uint8_t padding269[3];
    TESForm* pCrimeVictim;
    // Caches the SCRIPT_LOCAL* last found in the event list's local list by ScriptLocals::GetVariable.
    SCRIPT_LOCAL* pLastScriptVariable;
    uint32_t uiLastScriptVarID = 0xFFFFFFFF;
    // The event list GetVariable last looked a variable up in (ScriptLocals::GetVariable stores its this).
    ScriptLocals* pLastScriptEventList;
    SCRIPT_REFERENCED_OBJECT* pLastRefVar;
    int32_t iLastVarIndex = -1;
    ScriptLocals* pLastEventList;
    Script* pLastScript;
    uint32_t uiActivateRecursionDepth;
    // The recursion guard of the script commands that run another script from inside one: Cmd_EnterTrigger_Execute runs
    // the nested script only while it is below 5 (a signed compare), incrementing it around the call and decrementing
    // it after.
    int32_t unk290;
    uint32_t uiThreadSpecificFlags;
    bool bCollectGarbage = true;
    uint8_t padding299[3];
    // Thread-local static NiTexture::RendererData::iLoadMethod; the template holds 3.
    int32_t iLoadMethod = 3;
    float fLastScaledTime;
    // The template holds 3, NiTimeController::MAX_CYCLE_TYPES (no cycle type yet).
    uint32_t eLastCycle = 3;
    float fLastWeightedPhaseTime;
    float fLastLoKeyTime;
    float fLastHiKeyTime;
    MEM_CONTEXT eMemContext = MEM_CONTEXT_UNKNOWN;
    uint32_t iWarningCount;
    uint32_t iBatchRendererIndex;
    // The mode bhkBlendCollisionObject::hktoNiTransform reads here: 1 and 2 select which way the transform is copied.
    bhkNiCollisionObject::SYNCMODE eHavokSyncMode;
};

inline TLSData* GetThreadTLSData() {
    static thread_local TLSData kTLSData = {};
    return &kTLSData;
}
