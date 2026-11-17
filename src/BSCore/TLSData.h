#pragma once

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "Reconstruction.hpp"

class BSExtraData;
class ExtraDataList;
class NiNode;
struct SCRIPT_LOCAL;
struct SCRIPT_REFERENCED_OBJECT;
class Script;
class ScriptLocals;
class TESForm;
class TESObjectREFR;

// The per-thread game state the exe keeps in a thread-local block (TlsGetValue slot): the cached extra data lists,
// the script run state, the loading flags and the memory context. IDA's name; the PDB has no such class. The default
// member initialisers are the exe's .tls template (0x1272000..0x12722C4): its six nonzero fields.
class TLSData {
public:
	// IDA: UInt32 unk000.
	uint8_t padding00[4];
	// IDA: UInt32 unk004.
	uint8_t padding04[4];
	ExtraDataList* pLastExtraList;
	uint32_t uiDirtyCount;
	BSExtraData* pLastExtraDatas[147];
	// IDA: bool bLoadTexturesDegraded.
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
	// IDA name; not in the PDB. IDA: ScriptVariable*; the exe caches the SCRIPT_LOCAL* it found in the event list's
	// local list (ScriptLocals::GetVariable, 0x5A9140), reading its value at +8.
	SCRIPT_LOCAL* pLastScriptVariable;
	uint32_t uiLastScriptVarID = 0xFFFFFFFF;
	// IDA name; not in the PDB. The event list GetVariable last looked a variable up in (ScriptLocals::GetVariable,
	// 0x5A9140, stores its this).
	ScriptLocals* pLastScriptEventList;
	SCRIPT_REFERENCED_OBJECT* pLastRefVar;
	int32_t iLastVarIndex = -1;
	ScriptLocals* pLastEventList;
	Script* pLastScript;
	uint32_t uiActivateRecursionDepth;
	// IDA name; not in the PDB. The recursion guard of the script commands that run another script from inside one:
	// Cmd_EnterTrigger_Execute (0x5D8CF0) runs the nested script only while it is below 5 (0x5D8DA0, a signed jge),
	// incrementing it around the call (0x5D8DD1) and decrementing it after (0x5D8E07).
	int32_t unk290;
	uint32_t uiThreadSpecificFlags;
	bool bCollectGarbage = true;
	uint8_t padding299[3];
	// The PDB's thread-local static NiTexture::RendererData::iLoadMethod; the template holds 3.
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
	// The mode bhkBlendCollisionObject::hktoNiTransform reads here (0xC81E91, and the second read at 0xC81F24):
	// 1 and 2 select which way the transform is copied.
	bhkNiCollisionObject::SYNCMODE eHavokSyncMode;
};

inline TLSData* GetThreadTLSData() {
	static thread_local TLSData kTLSData = {};
	return &kTLSData;
}
