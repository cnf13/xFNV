#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class Actor;
class BGSLoadGameBuffer;
class BGSSaveGameBuffer;
// IDA's DetectionData; the PDB's DetectionState, declared in Fallout/AI/HighProcess.h.
class DetectionState;

// IDA name; not in the PDB. The list of detections (the exe walks it with BSSimpleList's GetItem 0x6815C0 and
// GetNext 0x726070).
class DetectionData_tList : public BSSimpleList<DetectionState*> {};

// The actor a deed is blamed on (the shooter of a projectile, the one who set off an explosion), kept by ID so it
// survives saving and the actor's removal. Counted by hand through uiRefCount; every cause is also in kCauseList.
class ActorCause {
public:
	ActorCause();
	ActorCause(Actor* apActor);
	~ActorCause();

	// Takes a reference: an atomic increment of iRefCount, the new count returned (the exe's folded +0x18
	// incrementer).
	void IncRefCount();
	// Releases a reference; the last one deletes the cause.
	void DecRefCount();
	// Reads the cause from a save.
	void LoadGame(BGSLoadGameBuffer* apLoadGameBuffer);
	// Saves the origin, the ID, the actor, then every witness, with their count in front.
	void SaveGame(BGSSaveGameBuffer* apSaveGameBuffer);
	// pActor.
	Actor* GetActor() { return pActor; }
	// Whether the cause still has its actor (pActor not null; thiscall, ret 0, 0x5CA4F0).
	bool IsValid();
	// IDA name; not in the PDB. Returns &kOrigin: the exe leaves the inlined NiPoint3::operator='s result in eax
	// (0x66E28E).
	NiPoint3* FreeDetectionList();
	void InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer);

	// Walks kCauseList on a load: initialises each cause that still has references and
	// frees the others (cdecl, ret 0, 0x66E810; IDA types the argument int, the exe passes the BGSLoadGameBuffer).
	static void InitLoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer);
	// Empties kCauseList on a revert.
	static void RevertActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer);
	static void operator delete(void* apMemory) { ourFree(apMemory); }
	static void LoadActorCausesList(BGSLoadGameBuffer* apLoadGameBuffer);
	static void ClearActor(Actor* apActor);
	static ActorCause* GetActorCauseByID(int32_t aiActorCauseID);
	static void SaveActorCausesList(BGSSaveGameBuffer* apSaveGameBuffer);

	Actor* pActor;
	// IDA's pDetectingActors.
	BSSimpleList<DetectionState*>* pWitnesses;
	NiPoint3 kOrigin;
	uint32_t iActorCauseID;
	// IDA's uiRefCount.
	uint32_t iRefCount;

	// The next ID ActorCause hands out.
	static inline uint32_t iActorCauseIDCounter;
	// Every cause that exists (the list's head node is inline).
	static BSSimpleList<ActorCause*> kCauseList;
};

// NiPointer of a cause takes and releases references through the cause's own IncRefCount and DecRefCount (the
// exe's NiPointer<ActorCause> destructor 0x42F760 calls DecRefCount).
inline void NiIncRefCount(ActorCause* apCause) {
	apCause->IncRefCount();
}

inline void NiDecRefCount(ActorCause* apCause) {
	apCause->DecRefCount();
}
